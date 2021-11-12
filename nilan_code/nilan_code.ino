/**
  Nilan Modbus firmware for D1 Mini (ESP8266) together with a TTL to RS485 Converter https://www.aliexpress.com/item/32836213346.html?spm=a2g0s.9042311.0.0.27424c4dqnr5i7
  
  Written by Dan Gunvald
    https://github.com/DanGunvald/NilanModbus

  Modified to use with Home Assistant by Anders Kvist, Jacob Scherrebeck and other great people :) 
    https://github.com/anderskvist/Nilan_Homeassistant
    https://github.com/jascdk/Nilan_Homeassistant
    
  Read from a Nilan Air Vent System (Danish Brand) using a Wemos D1
  Mini (or other ESP8266-based board) and report the values to an MQTT
  broker. Then use it for your home-automation system like Home Assistant.
  
  External dependencies. Install using the Arduino library manager:
  
     "Arduino JSON V6 by Benoît Blanchon https://github.com/bblanchon/ArduinoJson - IMPORTANT - Use latest V.6 !!! This code won´t compile with V.5 
     "ModbusMaster by Doc Walker https://github.com/4-20ma/ModbusMaster
     "PubSubClient" by Nick O'Leary https://github.com/knolleary/pubsubclient
     
  Porject inspired by https://github.com/DanGunvald/NilanModbus

  Join this Danish Facebook Page for inspiration :) https://www.facebook.com/groups/667765647316443/
REV 28122020
For Discovery use https://github.com/plapointe6/HAMqttDevice
*/
#include <FS.h>  
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ModbusMaster.h>
#include <PubSubClient.h>
#include "configuration.h"
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
WiFiManager wifiManager;

#define HOST CUSTOM_HOSTNAME // Change this to whatever you like. 
#define MAXREGSIZE 28
#define SENDINTERVAL 30000 // normally set to 180000 milliseconds = 3 minutes. Define as you like
#define VENTSET 1003
#define RUNSET 1001
#define MODESET 1002
#define TEMPSET 1004
#define USERSET 600
#define USERFUNCSET 601
#define USERTIMESET 602
#define USERVENTSET 603
#define USERTEMPSET 604
#define USEROFFSSET 605
#define PROGRAMSET 500
 


char chipid[12];
char mqttserver[40]=MQTTSERVER;
char mqttusername[20]=MQTTUSERNAME;
char mqttpassword[20]=MQTTPASSWORD;
WiFiServer server(80);
WiFiClient client;
PubSubClient mqttclient(client);
static long lastMsg = -SENDINTERVAL;
static int16_t rsbuffer[MAXREGSIZE];
ModbusMaster node;
const int funcact1 =  0;
const int funcact2 =  2;

int16_t AlarmListNumber[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,70,71,90,91,92};
String AlarmListText[] = {"NONE","HARDWARE","TIMEOUT","FIRE","PRESSURE","DOOR","DEFROST","FROST","FROST","OVERTEMP","OVERHEAT","AIRFLOW","THERMO","BOILING","SENSOR","ROOM LOW","SOFTWARE","WATCHDOG","CONFIG","FILTER","LEGIONEL","POWER","T AIR","T WATER","T HEAT","MODEM","INSTABUS","T1SHORT","T1OPEN","T2SHORT","T2OPEN","T3SHORT","T3OPEN","T4SHORT","T4OPEN","T5SHORT","T5OPEN","T6SHORT","T6OPEN","T7SHORT","T7OPEN","T8SHORT","T8OPEN","T9SHORT","T9OPEN","T10SHORT","T10OPEN","T11SHORT","T11OPEN","T12SHORT","T12OPEN","T13SHORT","T13OPEN","T14SHORT","T14OPEN","T15SHORT","T15OPEN","T16SHORT","T16OPEN","ANODE","EXCH INFO","SLAVE IO","OPT IO","PRESET","INSTABUS"};


String req[4]; //operation, group, address, value
enum reqtypes
{
  reqtempone = 0,
  reqtemptwo = 1,
  reqtemptre = 2,
  reqtempfour = 3,
  reqalarm,
  reqtime,
  reqcontrol,
  reqspeed,
  reqairtemp,
  reqairflow,
  reqairheat,
  reqprogram,
  requser,
  requser2,
  reqinfo1,
  reqinfo2,
  reqinfo3,
  reqinfo4,
  reqinputairtemp,
  reqapp,
  reqoutput1,
  reqoutput2,
  reqoutput3,
  reqdisplay1,
  reqdisplay2,
  reqdisplay,
  reqairbypass, //27
  reqmax
};
 //                    1      2        3         4        5       6         7        8         9           10         11         12       13      14        15       16       17       18        19           20       21         22          23        24          25           26         27            28        
String groups[] = {"temp1", "temp2", "temp3", "temp4", "alarm", "time", "control", "speed", "airtemp", "airflow", "airheat", "program", "user", "user2", "info1", "info2", "info3", "info4", "inputairtemp", "app", "output1", "output2", "output3", "display1", "display2", "display", "airbypass"};
byte regsizes[] = {1, 2, 2, 2, 10, 6, 8, 2, 8, 2, 0, 1, 6, 6, 2, 1, 1, 3, 7, 4, 6, 1, 1, 1, 0, 0, 1, 1}; //{1, 2, 2, 2, 10, 6, 8, 2, 8, 5, 0, 1, 6, 6, 2, 1, 1, 3, 7, 4, 6, 1, 1, 1, 0, 0, 1, 1};
int regaddresses[] = {200, 203, 207, 221, 400, 300, 1000, 200, 1200, 1103, 0, 500, 600, 610, 100, 103, 105, 113, 1200, 0, 100, 122, 126, 2002, 2002, 2002, 3000, 1600};
byte regtypes[] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0};
char *regnames[][MAXREGSIZE] = {
    //temp1 OK1 
    {"T0_Controller"},
    //temp2 OK2
    {"T3_Exhaust", "T4_Outlet"},
    //temp3 OK3
    {"T7_Inlet", "T8_Outdoor"},
    //temp4 OK4
    {"RH", "CO2"},
    //alarm OK5
    {"Status", "List_1_ID", "List_1_Date", "List_1_Time", "List_2_ID", "List_2_Date", "List_2_Time", "List_3_ID", "List_3_Date", "List_3_Time"},
    //time OK6
    {"Second", "Minute", "Hour", "Day", "Month", "Year"},
    //control OK7
    {"Type", "RunSet", "ModeSet", "VentSet", "TempSet", "ServiceMode", "ServicePct", "Preset"},
    //speed OK8
    {"ExhaustSpeed", "InletSpeed"},
    //airtemp OK9
    {"CoolSet", "TempMinSum", "TempMinWin", "TempMaxSum", "TempMaxWin", "TempSummer", "TempNightDayLim", "TempNightSet"}, //Tilføjet sidste to for Light Nicklas
    //airflow OK10
    {"SinceFiltDay", "ToFiltDay"}, //Fuldstændig ændret for Light Nicklas Tdiligere
    //airheat OK11
    {""},
    //program12
    {"Program"},
    //program.user OK13
    {"UserFuncAct", "UserFuncSet", "UserTimeSet", "UserVentSet", "UserTempSet", "UserOffsSet"},
    //program.user2 OK14
    {"User2FuncAct", "User2FuncSet", "User2TimeSet", "User2VentSet", "UserTempSet", "UserOffsSet"},
    //info1 OK15
    {"UserFunc", "AirFilter"},
    //info2 OK16
    {"Smoke"},
    //info3 OK17
    {"Frost_overht"},
    //info4 OK18
    {"UserFunc_2", "DamperClosed", "DamperOpened"}, //Tilføjet to Damper? Nicklas
    //inputairtemp OK19
    {"IsSummer", "TempInletSet", "TempControl", "TempRoom", "EffPct", "CapSet", "CapAct"},
    //app OK20
    {"Bus.Version", "VersionMajor", "VersionMinor", "VersionRelease"},
    //output1 OK21
    {"AirFlap", "SmokeFlap", "BypassOpen", "BypassClose", "AirCircPump", "AirHeatAllow"},
    //output2 OK22
    {"CenHeatExt"},
    //output323
    {"AlarmRelay"},
    //display124
    {"UserMenuOpen"},
    //display125
    {""},
    //display226
    {""},
    //airbypass27
    {"AirBypassIsOpen"}
    };
    
char *getName(reqtypes type, int address)
{
  if (address >= 0 && address <= regsizes[type])
  {
    return regnames[type][address];
  }
  return NULL;
}

JsonObject HandleRequest(JsonDocument& doc)
{
  JsonObject root = doc.to<JsonObject>();
  reqtypes r = reqmax;
  char type = 0;
  if (req[1] != "")
  {
    for (int i = 0; i < reqmax; i++)
    {
      if (groups[i] == req[1])
      {
        r = (reqtypes)i;
      }
    }
  }
  type = regtypes[r];
  if (req[0] == "read")
  {
    int address = 0;
    int nums = 0;
    char result = -1;
    address = regaddresses[r];
    nums = regsizes[r];

    result = ReadModbus(address, nums, rsbuffer, type & 1);
    if (result == 0)
    {
      root["status"] = "Modbus connection OK";
      for (int i = 0; i < nums; i++)
      {
        char *name = getName(r, i);
        if (name != NULL && strlen(name) > 0)
        {
          if ((type & 2 && i > 0) || type & 4)
          {
            String str = "";
            str += (char)(rsbuffer[i] & 0x00ff);
            str += (char)(rsbuffer[i] >> 8);
            root[name] = str;
          }
          else if (type & 8)
          {
            root[name] = rsbuffer[i] / 100.0;
          }
          else
          {
            root[name] = rsbuffer[i];
          }
        }
      }
    }
    else {
      root["status"] = "Modbus connection failed";
    }
    root["requestaddress"] = address;
    root["requestnum"] = nums;
  }
  if (req[0] == "set" && req[2] != "" && req[3] != "")
  {
    int address = atoi(req[2].c_str());
    int value = atoi(req[3].c_str());
    char result = WriteModbus(address, value);
    root["result"] = result;
    root["address"] = address;
    root["value"] = value;
  }
  if (req[0] == "help")
  {
    for (int i = 0; i < reqmax; i++)
    {
      root[groups[i]] = 0;
    }
  }
  root["operation"] = req[0];
  root["group"] = req[1];
  return root;
}

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}

void setup()
{
  WiFi.mode(WIFI_STA);
  //read configuration from FS json
  if (SPIFFS.begin()) {
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
#ifdef ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          strcpy(mqttserver, json["mqtt_server"]);
          strcpy(mqttusername, json["mqtt_user"]);
          strcpy(mqttpassword, json["mqtt_pass"]);
        }
      }
    }
  } 
  //end read
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqttserver, 40);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt username", mqttusername, 20);
  WiFiManagerParameter custom_mqtt_pass("password", "mqtt password", mqttpassword, 20);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("MODBUS-MODULE")) {
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    //read updated parameters
    strcpy(mqttserver, custom_mqtt_server.getValue());
    strcpy(mqttusername, custom_mqtt_user.getValue());
    strcpy(mqttpassword, custom_mqtt_pass.getValue());
    
    //save the custom parameters to FS
    if (shouldSaveConfig) {
    #ifdef ARDUINOJSON_VERSION_MAJOR >= 6
      DynamicJsonDocument json(1024);
    #else
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
    #endif
      json["mqtt_server"] = mqttserver;
      json["mqtt_user"] = mqttusername;
      json["mqtt_pass"] = mqttpassword;
    
      File configFile = SPIFFS.open("/config.json", "w");
    
    #ifdef ARDUINOJSON_VERSION_MAJOR >= 6
      serializeJson(json, Serial);
      serializeJson(json, configFile);
    #else
      json.printTo(Serial);
      json.printTo(configFile);
    #endif
      configFile.close();
      //end save
    }
  pinMode(funcact1, OUTPUT);
  pinMode(funcact2, OUTPUT);  
  digitalWrite(funcact1, LOW);
  digitalWrite(funcact2, LOW);
  char host[64];
  uint32_t chipID = ESP.getChipId();
  sprintf(chipid, "%08X", chipID);
  sprintf(host, HOST, chipid);
  delay(500);
  WiFi.hostname(host);
  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() {
  });
  ArduinoOTA.onEnd([]() {
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  });
  ArduinoOTA.onError([](ota_error_t error) {
  });
  ArduinoOTA.begin();
  server.begin();
  Serial.begin(19200, SERIAL_8E1);
  delay(100);
  node.begin(30, Serial);
  mqttclient.setServer(mqttserver, 1883);
  mqttclient.setCallback(mqttcallback);
}

void mqttcallback(char *topic, byte *payload, unsigned int length)
{
  if (strcmp(topic, "ventilation/ventset") == 0)
  {
    if (length == 1 && payload[0] >= '0' && payload[0] <= '4')
    {
      int16_t speed = payload[0] - '0';
      WriteModbus(VENTSET, speed);
    }
  }
  if (strcmp(topic, "ventilation/modeset") == 0)
  {
    if (length == 1 && payload[0] >= '0' && payload[0] <= '4')
    {
      int16_t mode = payload[0] - '0';
      WriteModbus(MODESET, mode);
    }
  }
  if (strcmp(topic, "ventilation/runset") == 0)
  {
    if (length == 1 && payload[0] >= '0' && payload[0] <= '1')
    {
      int16_t run = payload[0] - '0';
      WriteModbus(RUNSET, run);
    }
  }
  if (strcmp(topic, "ventilation/tempset") == 0)
  {
    if (length == 4 && payload[0] >= '0' && payload[0] <= '2')
    {
      String str;
      for (int i = 0; i < length; i++)
      {
        str += (char)payload[i];
      }
      WriteModbus(TEMPSET, str.toInt());
    }
  }
   if (strcmp(topic, "ventilation/userfuncset") == 0)
  {
    if (length == 1 && payload[0] >= '0' && payload[0] <= '6')
    {
      int16_t program = payload[0] - '0';
      WriteModbus(USERFUNCSET, program);
    }
   }
   if (strcmp(topic, "ventilation/userset") == 0)
  {
    if (length == 1 && payload[0] == '0')
    {
      mqttclient.publish("ventilation/user/relay1", "0");
      digitalWrite(funcact1, LOW);
    } else if (length == 1 && payload[0] == '1') {
      mqttclient.publish("ventilation/user/relay1", "1");
      digitalWrite(funcact1, HIGH);
    }
   }
   if (strcmp(topic, "ventilation/userset2") == 0)
  {
    if (length == 1 && payload[0] == '0')
    {
      mqttclient.publish("ventilation/user/UserFuncAct2", "0");
      digitalWrite(funcact2, LOW);
    } else if (length == 1 && payload[0] == '1') {
      mqttclient.publish("ventilation/user/UserFuncAct2", "1");
      digitalWrite(funcact2, HIGH);
    }
   }
if (strcmp(topic, "ventilation/usertimeset") == 0)
  {
    if (length == 3 && payload[0] >= '0' && payload[0] <= '800')
    {
      String str;
      for (int i = 0; i < length; i++)
      {
        str += (char)payload[i];
      }
      WriteModbus(USERTIMESET, str.toInt());
    }
  }
   if (strcmp(topic, "ventilation/userventset") == 0)
  {
    if (length == 1 && payload[0] >= '0' && payload[0] <= '4')
    {
      int16_t program = payload[0] - '0';
      WriteModbus(USERVENTSET, program);
    }
   }
if (strcmp(topic, "ventilation/usertempset") == 0)
  {
    if (length == 4 && payload[0] >= '0' && payload[0] <= '2')
    {
      String str;
      for (int i = 0; i < length; i++)
      {
        str += (char)payload[i];
      }
      WriteModbus(USERTEMPSET, str.toInt());
    }
  }
 if (strcmp(topic, "ventilation/useroffsset") == 0)
  {
    if (length == 4 && payload[0] >= '0' && payload[0] <= '2')
    {
      String str;
      for (int i = 0; i < length; i++)
      {
        str += (char)payload[i];
      }
      WriteModbus(USEROFFSSET, str.toInt());
    }
  }
  if (strcmp(topic, "ventilation/selectset") == 0)
  {
    if (length == 1 && payload[0] >= '0' && payload[0] <= '3')
    {
      int16_t program = payload[0] - '0';
      WriteModbus(PROGRAMSET, program);
    }
   }
  lastMsg = -SENDINTERVAL;
}

bool readRequest(WiFiClient &client)
{
  req[0] = "";
  req[1] = "";
  req[2] = "";
  req[3] = "";

  int n = -1;
  bool readstring = false;
  while (client.connected())
  {
    if (client.available())
    {
      char c = client.read();
      if (c == '\n')
      {
        return false;
      }
      else if (c == '/')
      {
        n++;
      }
      else if (c != ' ' && n >= 0 && n < 4)
      {
        req[n] += c;
      }
      else if (c == ' ' && n >= 0 && n < 4)
      {
        return true;
      }
    }
  }

  return false;
}

void writeResponse(WiFiClient& client, const JsonDocument& doc)  
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  serializeJsonPretty(doc,client);
}
//readHoldingRegisters  readInputRegisters
char ReadModbus(uint16_t addr, uint8_t sizer, int16_t *vals, int type)
{
  char result = 0;
  switch (type)
  {
  case 0:
    result = node.readInputRegisters(addr, sizer);
    break;
  case 1:
    result = node.readHoldingRegisters(addr, sizer);
    break;
  }
  if (result == node.ku8MBSuccess)
  {
    for (int j = 0; j < sizer; j++)
    {
      vals[j] = node.getResponseBuffer(j);
    }
    return result;
  }
  return result;
}
char WriteModbus(uint16_t addr, int16_t val)
{
  node.setTransmitBuffer(0, val);
  char result = 0;
  result = node.writeMultipleRegisters(addr, 1);
  return result;
}

void mqttreconnect()
{
  int numretries = 0;
  while (!mqttclient.connected() && numretries < 3)
  {
    if (mqttclient.connect(chipid, mqttusername, mqttpassword))
    {
      mqttclient.subscribe("ventilation/ventset");
      mqttclient.subscribe("ventilation/modeset");
      mqttclient.subscribe("ventilation/runset");
      mqttclient.subscribe("ventilation/tempset");
      mqttclient.subscribe("ventilation/userfuncset");
      mqttclient.subscribe("ventilation/userset");
      mqttclient.subscribe("ventilation/userset2");
      mqttclient.subscribe("ventilation/usertimeset");
      mqttclient.subscribe("ventilation/userventset");
      mqttclient.subscribe("ventilation/usertempset");
      mqttclient.subscribe("ventilation/useroffsset");
      mqttclient.subscribe("ventilation/selectset");
    }
    else
    {
      delay(1000);
    }
    numretries++;
  }
}

void loop()
{
  ArduinoOTA.handle();
  WiFiClient client = server.available();
  if (client)
  {
    bool success = readRequest(client);
    if (success)
    {
      StaticJsonDocument<1000> doc;
      HandleRequest(doc);
 
      writeResponse(client, doc);
    }
    client.stop();
  }

  if (!mqttclient.connected())
  {
    mqttreconnect();
  }

  if (mqttclient.connected())
  {
    mqttclient.loop();
    long now = millis();
    if (now - lastMsg > SENDINTERVAL)
    {
       reqtypes rr[] = {reqcontrol, reqtime, reqoutput1, reqoutput2, reqoutput3, reqspeed, reqalarm, reqinputairtemp, reqprogram, requser, reqinfo1, reqinfo2, reqinfo3, reqinfo4, reqtempone, reqtemptwo, reqtemptre, reqtempfour, reqairflow, reqairbypass}; // put another register in this line to subscribe
      for (int i = 0; i < (sizeof(rr)/sizeof(rr[0])); i++)
      {
        reqtypes r = rr[i];
        char result = ReadModbus(regaddresses[r], regsizes[r], rsbuffer, regtypes[r] & 1); 
        if (result == 0)
        {
          mqttclient.publish("ventilation/error/modbus/", "0"); //no error when connecting through modbus
          for (int i = 0; i < regsizes[r]; i++)
          {
            char *name = getName(r, i);
            char numstr[10];
            if (name != NULL && strlen(name) > 0)
            {
              String mqname = "temp/";
              switch (r)
              {
              case reqcontrol:
                mqname = "ventilation/control/"; // Subscribe to the "control" register
                itoa((rsbuffer[i]), numstr, 10);
                break;
              case reqtime:
                mqname = "ventilation/time/"; // Subscribe to the "output" register
                itoa((rsbuffer[i]), numstr, 10);
                break;
              case reqoutput1:
                mqname = "ventilation/output/"; // Subscribe to the "output" register
                itoa((rsbuffer[i]), numstr, 10);
                break;
              case reqoutput2:
                mqname = "ventilation/output/"; // Subscribe to the "output" register
                itoa((rsbuffer[i]), numstr, 10);
                break;
              case reqoutput3:
                mqname = "ventilation/output/"; // Subscribe to the "output" register
                itoa((rsbuffer[i]), numstr, 10);
                break;
//              case reqdisplay:
//                mqname = "ventilation/display/"; // Subscribe to the "input display" register
//                itoa((rsbuffer[i]), numstr, 10);
//                break;
              case reqspeed:
                mqname = "ventilation/speed/"; // Subscribe to the "speed" register
                itoa((rsbuffer[i]), numstr, 10);
                break;
              case reqalarm:
                mqname = "ventilation/alarm/"; // Subscribe to the "alarm" register

                switch (i) 
                {
                  case 1: // Alarm.List_1_ID
                  case 4: // Alarm.List_2_ID
                  case 7: // Alarm.List_3_ID
                    if (rsbuffer[i] > 0) 
                    {
                      //itoa((rsbuffer[i]), numstr, 10); 
                      sprintf(numstr,"UNKNOWN"); // Preallocate unknown if no match if found
                      for (int p = 0; p < (sizeof(AlarmListNumber)); p++)
                      {
                        if (AlarmListNumber[p] == rsbuffer[i])                        
                        {
                      //   memset(numstr, 0, sizeof numstr);
                      //   strcpy (numstr,AlarmListText[p].c_str());
                         sprintf(numstr,AlarmListText[p].c_str());
                         break; 
                        } 
                      }
                    } else
                    {
                      sprintf(numstr,"None"); // No alarm, output None   
                    }
                    break;
                  case 2: // Alarm.List_1_Date
                  case 5: // Alarm.List_2_Date
                  case 8: // Alarm.List_3_Date
                    if (rsbuffer[i] > 0) 
                    {
                      sprintf(numstr,"%d",(rsbuffer[i] >> 9) + 1980); 
                      sprintf(numstr + strlen(numstr),"-%02d",(rsbuffer[i] & 0x1E0) >> 5);
                      sprintf(numstr + strlen(numstr),"-%02d",(rsbuffer[i] & 0x1F));
                    } else
                    {
                      sprintf(numstr,"N/A"); // No alarm, output N/A 
                    }
                    break;
                  case 3: // Alarm.List_1_Time
                  case 6: // Alarm.List_2_Time
                  case 9: // Alarm.List_3_Time
                    if (rsbuffer[i] > 0) 
                    {                  
                      sprintf(numstr,"%02d",rsbuffer[i] >> 11); 
                      sprintf(numstr + strlen(numstr),":%02d",(rsbuffer[i] & 0x7E0) >> 5);
                      sprintf(numstr + strlen(numstr),":%02d",(rsbuffer[i] & 0x11F)* 2);   
                    } else
                    {
                      sprintf(numstr,"N/A"); // No alarm, output N/A  
                    }
                    
                    break;                   
                  default: // used for Status bit (case 0)
                    itoa((rsbuffer[i]), numstr, 10); 
                }
                break;
              case reqinputairtemp:
                mqname = "ventilation/inputairtemp/"; // Subscribe to the "inputairtemp" register
                itoa((rsbuffer[i]), numstr, 10);
                break;    
              case reqprogram:
                mqname = "ventilation/weekprogram/"; // Subscribe to the "week program" register
                itoa((rsbuffer[i]), numstr, 10);
                break;     
              case requser:
                mqname = "ventilation/user/"; // Subscribe to the "user" register
                itoa((rsbuffer[i]), numstr, 10);
                break;         
              case reqinfo1:
                mqname = "ventilation/info/"; // Subscribe to the "info" register
                itoa((rsbuffer[i]), numstr, 10);
                break;   
              case reqinfo2:
                mqname = "ventilation/info/"; // Subscribe to the "info" register
                itoa((rsbuffer[i]), numstr, 10);
                break;  
              case reqinfo3:
                mqname = "ventilation/info/"; // Subscribe to the "info" register
                itoa((rsbuffer[i]), numstr, 10);
                break;  
              case reqinfo4:
                mqname = "ventilation/info/"; // Subscribe to the "info" register
                itoa((rsbuffer[i]), numstr, 10);
                break;        
              case reqtempone:
                  mqname = "ventilation/temp/"; // Subscribe to "temp" register
                dtostrf((rsbuffer[i] / 100.0), 5, 2, numstr);
                break;
              case reqtemptwo:
                  mqname = "ventilation/temp/"; // Subscribe to "temp" register
                dtostrf((rsbuffer[i] / 100.0), 5, 2, numstr);
                break;
              case reqtemptre:
                mqname = "ventilation/temp/"; // Subscribe to "temp" register
                dtostrf((rsbuffer[i] / 100.0), 5, 2, numstr);
                break;
              case reqtempfour:
                if (strncmp("RH", name, 2) == 0) {
                  mqname = "ventilation/moist/"; // Subscribe to moisture-level
                } else {
                  mqname = "ventilation/temp/"; // Subscribe to "temp" register
                }
                dtostrf((rsbuffer[i] / 100.0), 5, 2, numstr);
                break;
               case reqairflow:
                mqname = "ventilation/airflow/"; // Subscribe to the "airflow" register  Tilføjet af Nicklas så vi kan aflæse hvornår vores filter skal udskiftes
                itoa((rsbuffer[i]), numstr, 10);
                break;
               case reqairbypass:
                mqname = "ventilation/airbypass/"; // Subscribe to the "airflow" register  Tilføjet af Nicklas så vi kan aflæse hvornår vores filter skal udskiftes
                itoa((rsbuffer[i]), numstr, 10);
                break;
              }
              mqname += (char *)name;
              mqttclient.publish(mqname.c_str(), numstr);
            }
          }
        }
        else {
           char numstr1[10];
           char numstr2[10];
          char* failnr = itoa(regaddresses[r], numstr1, 10);
          char* failtype = itoa(regtypes[r], numstr2, 10);
          mqttclient.publish("ventilation/error/modbus/", "1"); //error when connecting through modbus
          mqttclient.publish("ventilation/error/modbuscase/address/", failnr); //error when connecting through modbus
          mqttclient.publish("ventilation/error/modbuscase/type/", failtype); //error when connecting through modbus
        }       
      }

      // Handle text fields
      reqtypes rr2[] = {reqdisplay1, reqdisplay2}; // put another register in this line to subscribe
      for (int i = 0; i < (sizeof(rr2)/sizeof(rr2[0])); i++) 
      {
        reqtypes r = rr2[i];

        char result = ReadModbus(regaddresses[r], regsizes[r], rsbuffer, regtypes[r] & 1);
        if (result == 0)
        {
          String text = "";
          String mqname = "ventilation/text/";

          for (int i = 0; i < regsizes[r]; i++)
          {
              char *name = getName(r, i);

              if ((rsbuffer[i] & 0x00ff) == 0xDF) {
                text += (char)0x20; // replace degree sign with space
              } else {
                text += (char)(rsbuffer[i] & 0x00ff);
              }
              if ((rsbuffer[i] >> 8) == 0xDF) {
                text += (char)0x20; // replace degree sign with space
              } else { 
                text += (char)(rsbuffer[i] >> 8);
              }
              mqname += (char *)name;
          }
          mqttclient.publish(mqname.c_str(), text.c_str());
        }
      }
      lastMsg = now;
    }
  }
}
