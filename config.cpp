#include "include.h"

#ifdef ESP32
#include <WebServer.h>
extern WebServer webServer;
#else
#include <ESP8266WebServer.h>
extern ESP8266WebServer webServer;
#endif

// Renvoie false si config.json n'a pas pu être écrit entièrement
bool saveJson (bool f){
#ifdef SERIAL_DEBUG
  Serial.printf("saveJson\n");
  Serial.println("saving config");
#endif
  DynamicJsonDocument json(512);


  json["staticIP"]= config.staticIP;
  json["masqueIP"]= config.masqueIP;
  json["gatewayIP"]= config.gatewayIP;
  json["dnsIP"]= config.dnsIP;
  json["mqtt_server"]= config.mqtt_server;
  json["mqtt_port"]= config.mqtt_port;
  json["mqtt_enabled"]= config.mqtt_enabled;
  json["mqtt_user"]= config.mqtt_user;
  json["mqtt_pass"]= config.mqtt_pass;
  json["wifiTimeout"]= config.wifiTimeout;
  json["Room"]= config.Room;
  json["Thing"]= config.Thing;
  json["PreloadSSID"]= config.PreloadSSID;
  json["PreloadPSW"]= config.PreloadPSW;
  if (f)
    json["PreloadFlag"]= config.PreloadFlag;
  else
    json["PreloadFlag"]= "";
  
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile)
    DBG_PRINTLN("failed to open config file for writing");
  else
    DBG_PRINTLN("succes writing config file");

  serializeJson(json,Serial);
  if (!configFile)
    return false;
  size_t written = serializeJson(json,configFile);
  configFile.close();
  bool ok = (written == measureJson(json));
  if (!ok)
    DBG_PRINTF("config.json incomplete: %u/%u bytes\n", (unsigned)written, (unsigned)measureJson(json));
  return ok;
}

// Monte le FS, lit config.json dans config ; formate le FS s'il est absent ou incomplet
void loadConfig() {
  //Read configuration from FS json
  DBG_PRINTLN("Mounting FS...");
  bool formatFlag=true;
  if (LittleFS.begin()) {
    DBG_PRINTLN("Mounted file system");

    if (LittleFS.exists("/config.json")) 
    {
      //File exists, reading and loading
      DBG_PRINTLN("Reading config file");
      File configFile = LittleFS.open("/config.json", "r");

      if (configFile) {
        size_t size = configFile.size();
        DBG_PRINTF("Opened config file, size:%d\n",size);

        bool error;
        DynamicJsonDocument json(size*2);
        auto err = deserializeJson(json, configFile);
        if (err)
          error=true;
        else
          error=false;

#ifdef SERIAL_DEBUG
   Serial.printf("Opened config file 1:\n");
   size_t len = measureJson(json);
   Serial.printf ("............................Json length : %d\n",len);
#endif
        if (!error) {
#ifdef SERIAL_DEBUG
          Serial.printf("Opened config file 2:\n");
        serializeJson(json, Serial);
#endif

          DBG_PRINTLN("\nParsed json");
          if (!json.containsKey("PreloadSSID") || !json.containsKey("PreloadPSW") || !json.containsKey("PreloadFlag") || !json.containsKey("staticIP") || !json.containsKey("masqueIP") || !json.containsKey("gatewayIP") || !json.containsKey("dnsIP") ||
              !json.containsKey("mqtt_server") || !json.containsKey("mqtt_port") || !json.containsKey("mqtt_user") || !json.containsKey("mqtt_pass") ||  !json.containsKey("wifiTimeout") || !json.containsKey("Room") || !json.containsKey("Thing"))
          {
            formatFlag=true;
#ifdef SERIAL_DEBUG
            Serial.println("\njson not found format true");
#endif
          }
          else 
          {
            COPY_STR(config.staticIP, json["staticIP"] | "");
            COPY_STR(config.masqueIP, json["masqueIP"] | "");
            COPY_STR(config.gatewayIP, json["gatewayIP"] | "");
            COPY_STR(config.dnsIP, json["dnsIP"] | "");
            COPY_STR(config.mqtt_server, json["mqtt_server"] | "");
            COPY_STR(config.mqtt_port, json["mqtt_port"] | "");
            // Absent des anciens config.json : MQTT actif par défaut
            COPY_STR(config.mqtt_enabled, (json["mqtt_enabled"] | "1")[0] == '0' ? "0" : "1");
            COPY_STR(config.mqtt_user, json["mqtt_user"] | "");
            COPY_STR(config.mqtt_pass, json["mqtt_pass"] | "");
            COPY_STR(config.wifiTimeout, json["wifiTimeout"] | "");
            COPY_STR(config.Room, json["Room"] | "");
            COPY_STR(config.Thing, json["Thing"] | "");
            COPY_STR(config.PreloadSSID, json["PreloadSSID"] | "");
            COPY_STR(config.PreloadPSW, json["PreloadPSW"] | "");
            COPY_STR(config.PreloadFlag, json["PreloadFlag"] | "");
            formatFlag=false;
          }
        } 
        else 
        {
          DBG_PRINTLN("Failed to load json config");
        }
      }
    }
  } 
  else 
  {
    DBG_PRINTLN("Failed to mount FS");
  }
#ifdef SERIAL_DEBUG
  Serial.printf("Opened config file 3:\n");
#endif 
  if (formatFlag){
#ifdef SERIAL_DEBUG
  Serial.println("Format LittleFS");
#endif
    LittleFS.format();
    // Flash vierge : begin() a échoué, il faut monter le FS formaté pour que saveJson() puisse écrire
    LittleFS.begin();
  }
}

void newParams() {
  int nb = webServer.args();
  DBG_PRINTF("*******************Receive New params nb=%d!!!!!!!!!!!!!!!!!!!\n",nb);
  for (int i = 0; i < nb; i++) {
    DBG_PRINTLN("Param:" + webServer.argName(i));
    DBG_PRINTLN("value:" + webServer.arg(i));
  }
  mhi::NewParams np;
  if (!mhi::parseNewParams(webServer.arg(0).c_str(), np)) {
    webServer.send(400, "text/plain", "400: Invalid Request");
    return;
  }
  mhi::applyNewParams(config, np);

#ifdef SERIAL_DEBUG
Serial.printf("staticIP:%s\n",config.staticIP);
Serial.printf("masqueIP:%s\n",config.masqueIP);
Serial.printf("gatewayIP:%s\n",config.gatewayIP);
Serial.printf("dnsIP:%s\n",config.dnsIP);
Serial.printf("mqtt_server:%s\n",config.mqtt_server);
Serial.printf("mqtt_port:%s\n",config.mqtt_port);
Serial.printf("wifiTimeout:%s\n",config.wifiTimeout);
Serial.printf("Room:%s\n",config.Room);
Serial.printf("Thing:%s\n",config.Thing);
Serial.printf("PreloadSSID:%s\n",config.PreloadSSID);
Serial.printf("PreloadPSW:%s\n",config.PreloadPSW);
#endif

  if (!saveJson(true)) {
    webServer.send(500, "text/plain", "500: config.json write failed");
    return;
  }
  webServer.send(200, "text/plain", "restarting...");
  delay(200);  // laisse partir la réponse avant le redémarrage
  ESP.restart();
}
