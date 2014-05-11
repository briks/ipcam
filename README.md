ipcam
=====

Arduino IP Cam

Hello,

The purpose of this project is to build a camera which is able to take picture up to 640*480 JPG remotely via Internet.
One time the hw and sw setup is done, you can access your camera via an URL and take shoot, control 4 outputs of Arduino and read 2 analog values.
The picture is directly uploaded from Arduino to your web browser.
--------------------------------------------------------------------------------------------------------------------------
So how to do the hardware conf ?
The camera is made of three parts:
* an Arduino UNO rev3
* an serail JPG camera from Adafruit
* an ethernet shield

* simply connect together the Arduino and the Ethernet shield
* connect power to Adafruit camera (+5VDC and GND, can be directly sourced by Arduino
* connect serial link from camera to arduino, RX from cam to digital 2 of Arduino, TX from cam to digital 3 of Arduino, this is a good idea to insert a resistor divider between digital pin 2 of arduino and RX pin of cam
* insert a µSD into ethernet shield with a FAT16/32 volume and the file index.htm
* modify the sketch with your no-ip account and encrypted password (see inside sketch), modify the sketch with yout ISP setting
* upload sketch into Arduino and start play!
--------------------------------------------------------------------------------------------------------------------------
Note about the sketch:
the major issue is the RAM stack and heap usage, I've to use F() function to save precious bytes in order the sketch works
the second major issue is the way I did the button 'cam shoot', a ugly way didn't, if an html knower wants improve it :)

I did this sketch, using lots of already written code (see inside sketch for more details), to see how is capable a tiny 8 bit MCU to do with IOT.
I also did it to learn about Arduino
I've some ideas to improve it like:
remove the µSD utilisation to save RAM bytes for using an tiny OS like arduos or NIL RTOS, this enable low power optimisations possibilities
add basic security to the web page access

Have fun with this toys!

__BriKs__
