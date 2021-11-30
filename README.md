# Make your Nilan Air ventilating system way more cool ;)

## Disclaimer
Do this at your own risk ! You are interfacing with hardware that you can potentially damage if you do not connect things as required !
Using the hardware and code presented here is done at you own risk. The hardware and software has been tested on Wavin AHC9000 and Nilan Comfort 300 without issues.

## Info

My (nic6911) contribution here is a hardware design, a slimmed version of the ESP code to match the hardware I have made and some 3D files for a nice casing.
The rest is left unchanged including much of this readme.


This little cool project lets you use you Home Assistant to control and read values from your Nilan air vent system. I have the Nilan Comfort 300 combined with the CTS602 panel. It works great, but do not know if it is compatible with other models.

The code for the project is not developed by me, but I made some adjustmenst to it, so it would integrate better with Home Assistant. The project is originally made for use with OpenHab.

For the original project look here: https://github.com/DanGunvald/NilanModbus

Please proceed this project at your own risk!!!

UPDATE 19/2-2020 : Got the code working with ArduinoJson version 6 (updated from version 5). Version 6 had some big breaking changes.

UPDATE 1/1-2020 : Now added a .ino file for use with a Nilan VPL15 system. Creates some others sensors over the Comfort 300 system. thanks to Martin Skeem for editing / coding :)

## Okay lets get to it!

### Installing the firmware:

I used the arduino editor to upload the code to my ESP8266 (ESP-01). If your sketch wont compile please check if you use the arduino.json V. 5 or V.6 library. This code uses V.6 and wont build with V.5. 
I (nic6911) have adapted the code in this fork to fit with the hardware I have constructed and thus some generic features have been removed.

For setting up your wifi and mqtt broker provide your credentials in the configuration.h file

#### SW addition 30/11/2021 !

Added ESPHome yaml to the repo. The module can use ESPHome instead of the traditional Arduino SW. This then only requires you to install the esphome integration and you are good to go ! No MQTT server setup or anything like that :)
I also included a fallback Wi-Fi hotspot in case you get a pre-programmed module. This then pops up as an access point. Connect to it and it'll ask you for Wi-Fi credentials. Type them in and you have the system online ! 

#### SW change 12/11/2021 !

In addition to the WiFi setting dialog I also added MQTT settings to the dialog. This means that if you have a module programmed for Wavin and your are to use it on a Wavin then you simply just enter your credentials and then you are done !
If you have a module programmed for Wavin but need it to work on a Nilan then you have to upload code OTA as shown below in the 8/11/2021 update 

![mqttwifisetting](/OTA/mqttwifisetting.PNG)

#### SW change 8/11/2021 !

The latest SW commit implements a WiFi manager enabling easy connection to your WiFi network and subsequently upload of Arduino code wirelessly through the Arduino IDE with your own MQTT settings ! 
So, when you connect the module to power it will show up as an access point:
![AP](/OTA/AP.png)

When connecting to the AP it will on most computers automaticlly open up a browser dialog. If not, go to 192.168.4.1 to see the WiFi manager dialog:
![wifimanager](/OTA/wifi_setting.png)

When done you will now be able to see the device in the Arduino IDE and thus able to upload code to it without a programmer:
![upload](/OTA/upload.png)

The things marked in red is settings you'll need to have.

Remember to edit the MQTT settings befor uploading:
![mqttsetting](/OTA/mqttsetting.png)

Enjoy !

#### Video Tutorials
These tutorials was made for my Wavin code, but the exact same applies for this Nilan code. The Nilan code also supports OTA update as the Wavin.

For setting up Home Assistant for MQTT, finding the Wavin client and adding zones looke here:
https://youtu.be/kwnt9SaQ6Jc

For the above to work you have to have a programmed ESP-01 talking modbus (like my module with ESP-01) which is shown next.

For programming the ESP-01 using a programmer look here:
https://youtu.be/PWJ3N4B8Pc4

If you have a pre-programmed ESP-01 with OTA support then you have to install the library dependencies as above but do not have to use a programmer. You can then program it like shown here:
https://youtu.be/2H5gkzoha98


### Setup the Hardware:

The Hardware used here is a design done by me (nic6911) and is a mutli-purpose ESP-01 Modbus module that was intended for Wavin AHC9000 and Nilan ventilation. But since it is pretty generic it will suit most modus applications.
The hardware includes buck converter supplying the ESP-01 and Modbus module with 3.3V from anything going from 8-24V (28V absolute max rating) as 12V and 24V are usually available on these systems for powering something like this.

#### Revision 2.1
To facilitate code versions using Modbus converters without the data direction controlled from the ESP I have implemented Automatic Direction Control. This also makes one more IO available for other uses.
I have decided to add 2 x Optocoupler, one on each available IO, to have isolated outputs which I intend to use for my Nilan system.
This effectively means that the rev 2.1 is a more general purpose hardware platform that in my case will be used for both my Wavin and Nilan setups.

The following schematic shows how my board is constructed in rev 2.1
![Schematic](/electronics/schematic.PNG)

My board design rev 2.1 is seen here:
![Bottom](/electronics/Bottom.PNG)
![Top](/electronics/Top.PNG)

A wiring example on a Comfort 300 and Wavin AHC9000 is shown here (NOTE: you might need external supply for the board as Nilan might not be able to serve power during peaks - so when sending over WiFi):
![Top](/electronics/Connections.png)

On the Wavin you simply use a patch cable (straight) and connect it from the module to the Modbus port and then you are done :)

### Getting values by HTTP:

You can get some json values from the Nilan by calling to it via HTTP. Just use your browser and type:

`DEVICE` - corresponds to the IP adress you you device (esp8266)

`http://[device]/help` - This should give you som registers

`http://[device]/read/output` - This would for example give you some status of the output

`http://[device]/set/[group]/[adress]/[value]`- This would make you able to send commands through HTTP 

e.g

`http://10.0.1.16/set/control/1004/2700` This will set your temperature to 27 degrees. 


### Getting values by MQTT:

Here is where it all shines - the code puts out som useful MQTT topics to monitor different thing.

Any MQTT-Tool (I use on my mac a tool called "MQTT Box") can be used to get the values by subscribing to :

`ventilation/temp/#`- This will give the temperatures from all the sensors.

`ventilation/moist/#`- This will give the humidity from the systems humidity sensor.

`ventilation/#` - This gives the output of the system - fan speed etc. Remember the payloads are given in values not text.

### Integrate with Home Assistant.

For my integration i use a package with all my Nilan config yaml in just one file. The file can be downloaded above (config.yaml).

After a restart of Home Assistant you will get alot of new sensors. These can be integrated in Home Assistant in different ways. I use the integrated Lovelace UI to make my UI. You can see below, how it can look like:)

![HA_GUI](https://github.com/jascdk/Nilan_Homeassistant/blob/master/Home%20Assistant/HA_GUI.png)

### Making External displays, that shows you the Nilan Data:

I have tried to make some LCD´s using some 4x16 rows displays together with an ESP32 running ESP-Home www.esphome.io .

If you wanna try it out you can use my provided .yaml code for ESP-Home above:)

### SPECIAL THANKS for contribution to this project goes to: @anderskvist https://github.com/anderskvist :)
















