# Make your Nilan Air ventilating system way more cool ;)

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
















