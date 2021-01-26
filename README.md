# GoProRC_ESP32
Allows simultaneous control of up to 8 GoPro cameras

This remote control provides an access point to which the cameras can connect. Simply pair with "Smart Remote" on the camera while the remote control is running.
<br>
Look also https://github.com/sepp89117/GoPro_ESP32_TFT_Touch-Control

<br>
You need an ESP32 module for this.<br>
You have to enter the MAC addresses for Cam1Mac to Cam8Mac in the .ino. MACs that are not required can be zeroed.<br>
<br>
I use GoEasyPro for control. However, you can test it via the serial monitor.<br>
<br>
Commands for the serial monitor are:<br>
&#60;rc1> - to activate the remote control<br>
&#60;rc0> - to deactivate the remote control<br>
<br>
&#60;sh1> - trigger start<br>
&#60;sh0> - trigger stop<br>
<br>
&#60;cmv> - Switch mode to video<br>
&#60;cmp> - Switch mode to photo<br>
&#60;cmb> - Switch mode to burst<br>
&#60;cml> - Switch mode to timelapse<br>
<br>
&#60;pw0> - Turn off cameras<br>
<br>
"???" - returns "GPRC" for com-port identification<br>
<br>
<b>ToDo:</b> Using GoEasyPro, I load the video lists from the SD and can watch the recorded videos. The cameras' AP mode switches off after 10 minutes. It is desirable to find a way to keep the camera's WiFi turned on via UDP. This is the only way to connect to the cameras WiFi after 10 minutes and watch the videos. If anyone has any idea how to keep AP mode alive with this remote control, tell me. Thanks
<br><br>
With GoEasyPro I can show the small display of the original remote and much more.<br>
Go to <a href="https://github.com/sepp89117/GoEasyPro">GoEasyPro</a><br>
<img src="https://github.com/sepp89117/GoEasyPro/blob/master/GoEasyPro_preview.png">
