// Attention!!! Settings for Arduino:
// Go to Tools -> set Upload Speed to 115200

#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiUdp.h>
#include "TimedTask.h"
#include "GoProCam.h"

//--------------------- GoPro MAC and IP declarations ------------------------------------------------------------
//--- change these to yours and just leave unused MACs as is (ending with 3 times 0x00) ---
uint8_t Cam1Mac[6] = {0x04, 0x41, 0x69, 0x0, 0x0, 0x0};
uint8_t Cam2Mac[6] = {0x04, 0x41, 0x69, 0x0, 0x0, 0x0};
uint8_t Cam3Mac[6] = {0x04, 0x41, 0x69, 0x0, 0x0, 0x0};
uint8_t Cam4Mac[6] = {0x04, 0x41, 0x69, 0x0, 0x0, 0x0};
uint8_t Cam5Mac[6] = {0x04, 0x41, 0x69, 0x0, 0x0, 0x0};
uint8_t Cam6Mac[6] = {0x04, 0x41, 0x69, 0x0, 0x0, 0x0};
uint8_t Cam7Mac[6] = {0x04, 0x41, 0x69, 0x0, 0x0, 0x0};
uint8_t Cam8Mac[6] = {0x04, 0x41, 0x69, 0x0, 0x0, 0x0};
uint8_t Cam9Mac[6] = {0x04, 0x41, 0x69, 0x0, 0x0, 0x0};
uint8_t Cam10Mac[6] = {0x04, 0x41, 0x69, 0x0, 0x0, 0x0};
//--- don't change the rest ---

const uint8_t maxCams = 10;
int numConnected = 0;
GoProCam cams[maxCams] = {
    GoProCam(Cam1Mac),
    GoProCam(Cam2Mac),
    GoProCam(Cam3Mac),
    GoProCam(Cam4Mac),
    GoProCam(Cam5Mac),
    GoProCam(Cam6Mac),
    GoProCam(Cam7Mac),
    GoProCam(Cam8Mac),
    GoProCam(Cam9Mac),
    GoProCam(Cam10Mac),
};

//--------------------- defines ---------------------------------------------------------------------------
#define MAX_CMD_LENGTH 60
//

//--------------------- heart beat declarations -----------------------------------------------------------
uint8_t lowCounter = 0;  // msg counter 1
uint8_t highCounter = 0; // msg counter 2
int cmdIndicator = 0;    // last sent cmd indicator
//

//--------------------- HT Tasks ------------------------------------------------------------------------
void heartBeat();
TimedAction heartBeatThread = TimedAction(700, heartBeat); // 700ms is fast as possible
//

//--------------------- Cam-Commands ----------------------------------------------------------------------
uint8_t PW0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x57, 0x00}; // power off
uint8_t SH1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x48, 0x02}; // shutter start
uint8_t SH0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x48, 0x00}; // shutter off
uint8_t CMv[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x4D, 0x00}; // camera mode (0: 'video')
uint8_t CMp[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x4D, 0x01}; // camera mode (1: 'photo')
uint8_t CMb[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x4D, 0x02}; // camera mode (2: 'burst')
uint8_t CMl[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x4D, 0x03}; // camera mode (3: 'timelapse')
uint8_t CMd[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x4D, 0x06}; // camera mode (6: 'default mode')
// uint8_t OO0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x4F, 0x4F, 0x00}; // One on One, used by rc, keeps connected
uint8_t OO1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4F, 0x4F, 0x01}; // One on One, used by rc, keeps connected
uint8_t st[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x74};        // status request
uint8_t lc[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6C, 0x63, 0x05};  // get status display (w=60px, h=75px, 1bpp)
//

//--------------------- other declarations ----------------------------------------------------------------
const unsigned int rcUdpPort = 8383;                     // UDP-Port der Fernbedienung 8383
const unsigned int camUdpPort = 8484;                    // UDP-Port der Kamera 8484
const unsigned int wifiChannel = 1;                      // WiFi-Channel of Smart-Remote = 1
uint8_t ap_mac[] = {0x86, 0xF3, 0xEB, 0xE4, 0x23, 0xDD}; // MAC-Adsress of Smart-Remote
const char *ssid = "HERO-RC-A1111425435131";             // SSID of my Smart-Remote
const char *g_hostname = "ESP_E423DD";                   // Hostname of my Smart-Remote
struct station_info *stat_info;
uint8_t packetBuffer[1024];         // buffer to hold incoming and outgoing packets
IPAddress ip(10, 71, 79, 1);        // IP of my Smart-Remote
IPAddress gateway(10, 71, 79, 1);   // GW of my Smart-Remote
IPAddress subnet(255, 255, 255, 0); // SM of my Smart-Remote
//

//--------------------- instances -------------------------------------------------------------------------
WiFiUDP Udp;
//

//--------------------- program ---------------------------------------------------------------------------
void setup()
{
  WiFi.mode(WIFI_AP); // Set WiFi in AP mode

  esp_wifi_set_mac(ESP_IF_WIFI_AP, &ap_mac[0]);

  WiFi.onEvent(onIpAssign, WiFiEvent_t::SYSTEM_EVENT_AP_STAIPASSIGNED);
  WiFi.onEvent(onStationDisconnected, WiFiEvent_t::SYSTEM_EVENT_AP_STADISCONNECTED);

  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);

  Serial.begin(115200);
  while (!Serial); // wait for serial attach

  // setup is done
  Serial.flush();
  Serial.println("");
  Serial.println("Ready!");
}

void loop()
{
  receiveFromSerial();

  if (numConnected > 0)
  {
    heartBeatThread.check();
  }
}

void startAP()
{
  WiFi.mode(WIFI_AP); // Set WiFi in AP mode

  tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP, g_hostname);

  lowCounter = 0;   // msg counter counter 1 reset
  highCounter = 0;  // msg counter counter 2 reset
  cmdIndicator = 0; // indicator reset
  WiFi.softAPConfig(ip, gateway, subnet);

  // Start AP
  WiFi.softAP(ssid, NULL, wifiChannel, 0, maxCams);

  // Start UDP
  Udp.begin(rcUdpPort);

  Serial.print("<rcOn>");
  Serial.print(1);
  Serial.println("</rcOn>");
}

void stopAP()
{
  numConnected = 0;
  Udp.stop();
  WiFi.softAPdisconnect(true);

  for (uint8_t i = 0; i < maxCams; i++)
  {
    if (cams[i].getIp() != 0)
    {
      cams[i].resetIp();
      Serial.print("Cam ");
      Serial.print(i);
      Serial.println(" disconnected from ap");
    }
  }

  Serial.print("<rcOn>");
  Serial.print(0);
  Serial.println("</rcOn>");
}

void onStationDisconnected(WiFiEvent_t evt, WiFiEventInfo_t info)
{
  for (uint8_t i = 0; i < maxCams; i++)
  {
    if (memcmp(info.sta_disconnected.mac, cams[i].getMac(), 6) == 0)
    {
      if (cams[i].getIp() != 0)
      {
        cams[i].resetIp();
        Serial.print("Cam ");
        Serial.print(i);
        Serial.println(" disconnected from AP");
      }
      break;
    }
  }
}

void onIpAssign(WiFiEvent_t evt, WiFiEventInfo_t info)
{
  ip4_addr_t IPaddress;

  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;

  memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

  esp_wifi_ap_get_sta_list(&wifi_sta_list);
  tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

  for (uint8_t i = 0; i < adapter_sta_list.num; i++)
  {
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];

    for (uint8_t x = 0; x < maxCams; x++)
    {
      if (memcmp(station.mac, cams[x].getMac(), 6) == 0)
      {
        if (cams[x].getIp() != station.ip.addr)
        {
          cams[x].setIp(station.ip.addr);
          Serial.print("Cam ");
          Serial.print(x);
          Serial.println(" connected to AP");
        }
        break;
      }
    }
  }
  numConnected = adapter_sta_list.num;
}

byte ReadSerialMonitorString(char *sString)
{
  byte nCount = 0;

  if (Serial.available() > 0)
  {
    Serial.setTimeout(50);
    nCount = Serial.readBytes(sString, MAX_CMD_LENGTH);
  }
  sString[nCount] = 0; // String terminator
  return nCount;
}

void sendToCam(uint8_t *req, int numBytes)
{
  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;

  memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

  esp_wifi_ap_get_sta_list(&wifi_sta_list);
  tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
  numConnected = adapter_sta_list.num;

  for (uint8_t i = 0; i < numConnected; i++)
  {
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];

    req[9] = highCounter;
    req[10] = lowCounter;

    Udp.beginPacket(station.ip.addr, camUdpPort);
    Udp.write(req, numBytes);
    Udp.endPacket();
  }

  // count up
  if (lowCounter >= 255)
  {
    highCounter++;
    lowCounter = 0;
  }
  if (highCounter >= 255)
  {
    highCounter = 0;
  }
  lowCounter++;

  for (uint8_t i = 0; i < numConnected; i++)
  {
    receiveFromCam();
  }
}

void receiveFromCam()
{
  yield();
  unsigned long receiveStart = millis();

  int numBytes = Udp.parsePacket();

  while (!numBytes && 350 > millis() - receiveStart)
  { // 350 is the receive timeout
    yield();
    numBytes = Udp.parsePacket();
  }

  if (numBytes)
  {
    char inCmd[3];

    Udp.read(packetBuffer, numBytes); // read the packet into the buffer

    inCmd[0] = packetBuffer[11];
    inCmd[1] = packetBuffer[12];
    inCmd[2] = 0; // terminate string

    for (uint8_t i = 0; i < maxCams; i++)
    {
      IPAddress iAdr(cams[i].getIp());
      if (Udp.remoteIP() == iAdr)
      {
        if (packetBuffer[13] == 0x1)
        {
          // illegal command for camera
          Serial.print("<illegal command \"");
          Serial.print(inCmd);
          Serial.print("\" in ");
          serialPrintHex(packetBuffer, numBytes);
          Serial.println(">");
        }
        else
        {
          if (strstr_P(inCmd, PSTR("lc")) != NULL)
          { // Screen for RC
            Serial.print("<lc>");
            serialPrintHex(packetBuffer, numBytes);
            Serial.print("</lc>@");
            serialPrintMac((uint8_t *)cams[i].getMac());
            Serial.println();
          }
          else if (strstr_P(inCmd, PSTR("st")) != NULL)
          {
            Serial.print("<st>");
            serialPrintHex(packetBuffer, numBytes);
            Serial.print("</st>@");
            serialPrintMac((uint8_t *)cams[i].getMac());
            Serial.println();
          }
          else if (strstr_P(inCmd, PSTR("pw")) != NULL || strstr_P(inCmd, PSTR("wt")) != NULL)
          {
            Serial.print("<pw1>");
            serialPrintMac((uint8_t *)cams[i].getMac());
            Serial.println("</pw1>");
          }
          else
          {
            Serial.print("<uknw>");
            serialPrintHex(packetBuffer, numBytes);
            Serial.println("</uknw>");
          }
        }
        break;
      }
    }
  }
}

void receiveFromSerial()
{ // void * parameters
  char sString[MAX_CMD_LENGTH + 1];

  // Check for command from Serial Monitor
  if (ReadSerialMonitorString(sString) > 0)
  {
    if (strstr_P(sString, PSTR("<rc1>")) != NULL)
    { // strstr_P keeps sString in flash; PSTR avoid ram using
      startAP();
    }
    else if (strstr_P(sString, PSTR("<rc0>")) != NULL)
    {
      stopAP();
    }
    else if (strstr_P(sString, PSTR("<sh1>")) != NULL)
    {
      // send record command
      sendToCam(SH1, 14);
      delay(50);
    }
    else if (strstr_P(sString, PSTR("<sh0>")) != NULL)
    {
      // send stop recording command
      sendToCam(SH0, 14);
      delay(50);
    }
    else if (strstr_P(sString, PSTR("<cmv>")) != NULL)
    {
      sendToCam(CMv, 14); // change mode to video
      delay(50);
    }
    else if (strstr_P(sString, PSTR("<cmp>")) != NULL)
    {
      sendToCam(CMp, 14); // change mode to photo
      delay(50);
    }
    else if (strstr_P(sString, PSTR("<cmb>")) != NULL)
    {
      sendToCam(CMb, 14); // change mode to burst
      delay(50);
    }
    else if (strstr_P(sString, PSTR("<cml>")) != NULL)
    {
      sendToCam(CMl, 14); // change mode to timelapse
      delay(50);
    }
    else if (strstr_P(sString, PSTR("<pw0>")) != NULL)
    {
      sendToCam(PW0, 14);
      delay(200);
      sendToCam(PW0, 14);
      delay(200);
      sendToCam(PW0, 14);
      delay(500);
      sendToCam(PW0, 14);
      delay(200);
      sendToCam(PW0, 14);
      delay(200);
      sendToCam(PW0, 14);
    }
    else if (strstr_P(sString, PSTR("???")) != NULL)
    {
      // send whoAmI
      Serial.println("GPRC");
    }
    else
    {
      // undefined
      Serial.println("unknown command");
    }
  }
}

void heartBeat()
{
  // rc sends 1x OO0, 1x OO1, 5x lc, 1x st
  if (cmdIndicator == 0)
  {
    sendToCam(lc, 14);

    cmdIndicator++;
  }
  else if (cmdIndicator == 1)
  {
    sendToCam(st, 13);

    cmdIndicator++;
  }
  else if (cmdIndicator >= 2)
  {
    sendToCam(OO1, 14);

    cmdIndicator = 0;
  }
}

void serialPrintHex(uint8_t msg[], int numBytes)
{
  for (int i = 0; i < numBytes; i++)
  {
    Serial.print(msg[i], HEX);
    if (i != numBytes - 1)
      Serial.print(" ");
  }
}

void serialPrintMac(uint8_t *bssid)
{
  Serial.print(bssid[0], HEX);
  Serial.print(":");
  Serial.print(bssid[1], HEX);
  Serial.print(":");
  Serial.print(bssid[2], HEX);
  Serial.print(":");
  Serial.print(bssid[3], HEX);
  Serial.print(":");
  Serial.print(bssid[4], HEX);
  Serial.print(":");
  Serial.print(bssid[5], HEX);
  Serial.print("");
}
