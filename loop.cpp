/*
##############################################################################################################################################################################################
MHI ESP2MQTT Interface v1.0.0
Arduino-based communication interface for Mitsubishi Heavy Industries (MHI) SRK/SRF series air conditioners.
Connects to the MHI CNS connector and synchronizes to its Serial Peripheral Interface (SPI). Updates from the MHI are processed and sent via serial to an ESP8266 running an MQTT MQTTclient.
Updates received via MQTT are sent from the ESP8266 to the Arduino over serial and injected into the SPI data frames to update the MHI.
R.J. Dekker, June 2018
##############################################################################################################################################################################################
*/
#include "include.h"
#include "params_logic.h"

//Access point that WiFiManager starts for configuration. Name and password should be set below before flashing.
//This is hardcoded and cannot be changed later.
//#define configSSID "ESPClim"  //AP name (give every unit a unique name before flashing)

mhi::WebValues web;
mhi::NetStats net;
mhi::LoopState loopState;
mhi::AppControl app;

void WiFiDisconnectAndRestart() {  
#ifdef ESP32
  WiFi.disconnect(true, true);  //Start WiFiManager  for reconfiguration
#else
  WiFi.disconnect(true);  //Start WiFiManager  for reconfiguration
#endif
  delay(1000);
  ESP.restart();
  delay(1000);
}

void stateTest(){
  if (!app.stateFlag)
    return;
  int state=app.stateFlag;
  app.stateFlag=0;
  switch (state)
  {
    case GO_REINIT:{
      DBG_PRINTLN("Reinit");

      bool formated = LittleFS.format();  //Erase flash
      if (formated) {
        DBG_PRINTF("format succes\n");
      } else {
        DBG_PRINTF("format error\n");
      }
    }
    case GO_WIFIMANAGER:
      WiFiDisconnectAndRestart();
      break;
    case GO_CHANGE:
#ifdef SERIAL_DEBUG
        Serial.printf("detecting change\n");
#endif
      saveJson (false);
      break;
    case GO_RESTART:
      ESP.restart();
      delay(1000);
  }
}

//void callback(char* topic, byte* payload, unsigned int length);  //Callback function header
void MQTT_subscribe_callback(const char *topic, byte *payload, unsigned int length);

template<typename Generic> void debug2mqtt(Generic text) {
  if (app.debugit) {
    MQTTclient.publish(debug, text, false);
  }
}


void sendConnexion2mqtt(bool start) {
  char msg[300];
  time_t rawtime = timeClient.getEpochTime();
  struct tm *tcur = localtime(&rawtime);
  if (app.tReboot.tm_year == 0)
    app.tReboot = *tcur;

  String ip = WiFi.localIP().toString();
  snprintf(msg, sizeof(msg), "%s num:%d,%dh:%02d  %d/%d/%d, last reboot:%dh:%02d  %d/%d/%d\n SSID:%s, ip:%s, MQTT:%s:%s, deconnect:%d\n",
          config.Thing, net.connectionCount++,
          tcur->tm_hour,    /* Hours.   [0-23]      */
          tcur->tm_min,     /* Minutes. [0-59]      */
          tcur->tm_mday,    /* Day.     [1-31]      */
          tcur->tm_mon + 1, /* Month.   [0-11]      */
          tcur->tm_year + 1900,

          app.tReboot.tm_hour,    /* Hours.   [0-23]      */
          app.tReboot.tm_min,     /* Minutes. [0-59]      */
          app.tReboot.tm_mday,    /* Day.     [1-31]      */
          app.tReboot.tm_mon + 1, /* Month.   [0-11]      */
          app.tReboot.tm_year + 1900,
          savedWiFiSSID.c_str(), ip.c_str(), config.mqtt_server, config.mqtt_port, net.nbDeconnect);
#ifdef SERIAL_DEBUG
  Serial.printf("publish topic:%s\npayload:%s\n", debug, msg);
#endif
  bool b = MQTTclient.publish(debug, msg, true);  //Publish message to debug topic to test/notify MQTT connection
#ifdef TELNET_DEBUG
  Debug.printf("ret publish:%d,payloadlen:%d, publish topic:%s\npayload:%s\n", b, strlen(msg), debug, msg);
#endif
}

void setWiFi() {
  static bool disconnected = false;
  static bool attemptedConnection = false;
  static unsigned long disconnectedAt = 0;
  static unsigned long lastAttemptAt = 0;
  unsigned long now = millis();

  if (WiFi.isConnected()) {
    if (disconnected) {
      disconnected = false;
      attemptedConnection = false;
      digitalWrite(LED_BUILTIN, LED_LOW);
      sendConnexion2mqtt(false);
    }
    return;
  }

  if (!disconnected) {
    disconnected = true;
    disconnectedAt = now;
    attemptedConnection = false;
    ++net.nbDeconnect;
  }

  if (savedWiFiSSID.length() > 0 &&
      (!attemptedConnection || now - lastAttemptAt >= 10000)) {
    WiFi.begin(savedWiFiSSID.c_str(), savedWiFiPass.c_str());
    lastAttemptAt = now;
    attemptedConnection = true;
  }

  unsigned long delayMinutes = strtoul(config.wifiTimeout, NULL, 10);
  if (savedWiFiSSID.length() == 0 || delayMinutes == 0 ||
      now - disconnectedAt >= delayMinutes * 60000UL) {
    startWifiRecoveryPortal();
  }
}

void printInfo();

void connect() {
  // Loop until we're (re)connected
  if (!MQTTclient.connected()) {
#ifdef SERIAL_DEBUG
    Serial.print("Attempting MQTT connection...");
#endif
#ifdef TELNET_DEBUG
    Debug.printf("Attempting MQTT connection...: %s\n", configSSID);
#endif

    if (!WiFi.isConnected())
      setWiFi();
    if (!WiFi.isConnected()) {
      strcpy(web.mqttState, "no");
      return;
    }

    // MQTT retries only begin after WiFi has been restored.
#ifdef ESP8266
    espClient.setTimeout(250);
#endif
    MQTTclient.setSocketTimeout(1);
    if (MQTTclient.connect(configSSID, config.mqtt_user, config.mqtt_pass)) 
    {
      DBG_PRINTLN("connected!");

      //Subscribe to topics that control the MHI state
      char tmp[100];
      sprintf(tmp, "%s#", MQTT_SET_PREFIX);
      MQTTclient.subscribe(tmp, 1);
      DBG_PRINTF("Subscribe to :%s\n", tmp);

      sprintf(tmp, "%s/#", MQTT_OP_PREFIX);
      MQTTclient.subscribe(tmp, 1);
      DBG_PRINTF("Subscribe to :%s\n", tmp);

      sprintf(tmp, "%s/#", MQTT_ERR_OP_PREFIX);
      MQTTclient.subscribe(tmp, 1);
      DBG_PRINTF("Subscribe to :%s\n", tmp);

      sprintf(tmp, "%s/#", service);
      MQTTclient.subscribe(tmp, 1);
      DBG_PRINTF("Subsribe to %s\n", tmp);

      printInfo();
      sendConnexion2mqtt(true);
      strcpy(web.mqttState,"yes");
      net.connectionFails = 0;
    } 
    else 
    {
      DBG_PRINT("failed, rc = ");
      DBG_PRINTLN(MQTTclient.state());
      DBG_PRINT("Failed connection attempts: ");
      DBG_PRINTLN(net.connectionFails);

      if (++net.connectionFails == 30) {
        DBG_PRINTLN("MQTT broker connection timeout...restarting!");
        ESP.restart();
      }
      DBG_PRINTLN("Try again in 5 seconds...");
      strcpy(web.mqttState,"no");
    }
  }
}

#if !defined(WEB_SERVER) || defined(MULTICORE)
static void loopWebServer(void){}
#else
  void loopWebServer(void);
#endif

void listNetworks();

void loopMHI();
void loop() {
  mhi::LoopState& ls = loopState;
  unsigned long curMillis = millis();
  if (ls.warmupCount>10)
  {
    ls.timeLoop=curMillis-ls.timeLoop;
    if (ls.timeLoop){
      if (ls.timeLoop>ls.maxLoop)
        ls.maxLoop=ls.timeLoop;
      if (ls.timeLoop<ls.minLoop)
        ls.minLoop=ls.timeLoop;
      ls.moyLoop+=ls.timeLoop;
    }
  }  
  else
    ls.warmupCount++;
  ls.timeLoop=curMillis;

  ArduinoOTA.handle();  //Handle Arduino OTA updates
#ifdef TELNET_DEBUG
  Debug.handle();
#endif
  digitalWrite(LED_BUILTIN, LED_LOW);

  setWiFi();
  handleWifiRecoveryPortal();

  if ((curMillis - ls.lastUpdateMillis) > ls.tWait) {
    ls.lastUpdateMillis = curMillis;
    unsigned long beforeTimeMillis = millis();
#ifdef TELNET_DEBUG
    Debug.printf("Before NTPClient.update\n");
#endif
    bool b = timeClient.update();
    unsigned long int tmp = millis() - beforeTimeMillis;
    if (b)
      ls.tWait = 600000;
    else
      ls.tWait = 60000;
#ifdef TELNET_DEBUG
    Debug.printf("Update NTP:%d, durée NTPClient: %dms\n", b, tmp);
#endif
  }

  // Date de reboot fixée dès que l'heure NTP est connue, indépendamment de MQTT
  if (app.tReboot.tm_year == 0 && timeClient.isTimeSet()) {
    time_t rawtime = timeClient.getEpochTime();
    app.tReboot = *localtime(&rawtime);
  }

  if (ls.nbLoop&&(curMillis - ls.lastPrintMillis) > 10000) {
    int core = 1;
#ifdef MULTICORE
    esp_task_wdt_reset();  //Reset du Watchdog toutes  les 10s
    core = xPortGetCoreID();
#endif
    ls.lastPrintMillis = curMillis;
    ls.moyLoop/=ls.nbLoop;
#ifdef SERIAL_DEBUG
    Serial.printf("Core:%d, configSSID:%s, Room:%s, SSID:%s, local IP:%s, Deconnexions:%d\n",
                core, configSSID, config.Thing, savedWiFiSSID.c_str(), WiFi.localIP().toString().c_str(), net.nbDeconnect);
#endif
#ifdef TELNET_DEBUG
    Debug.printf("Core:%d, configSSID:%s, Room:%s, SSID:%s, local IP:%s,  Deconnexions:%d\n", 
                core, configSSID, config.Thing, savedWiFiSSID.c_str(), WiFi.localIP().toString().c_str(), net.nbDeconnect);
#endif
    ls.minLoop=100000;
    ls.nbLoop=ls.maxLoop=ls.moyLoop=0;
  }

  loopWebServer();

  static unsigned long lastMQTTReconnectMillis = 0;
  static bool mqttReconnectAttempted = false;
  // MQTT désactivé (case décochée), serveur vide ou 0.0.0.0 : mode autonome, aucune tentative ni redémarrage
  bool mqttDisabled = mhi::mqttDisabled(config.mqtt_enabled, config.mqtt_server);
  if (mqttDisabled && strcmp(web.mqttState, "off") != 0)
    strcpy(web.mqttState, "off");
  if (!mqttDisabled && !MQTTclient.connected() &&
      (!mqttReconnectAttempted || curMillis - lastMQTTReconnectMillis >= 5000)) {
    DBG_PRINTF("MQTT deconnected, call connect()\n");
    mqttReconnectAttempted = true;
    lastMQTTReconnectMillis = curMillis;
    connect();  //Connect first time. Reconnect when connection lost.
  }
  if ((curMillis - ls.lastMQTTMillis) > 100) {
    ls.lastMQTTMillis = curMillis;
    MQTTclient.loop();
  }
  loopMHI();

  stateTest();
  ls.nbLoop++;
}


void setTrace(bool enable);
uint32_t traceDropped();

void TopicService(unsigned char *payload, unsigned int length) {
  //SERVICE COMMANDS
  if (strcmp((const char *)payload, "reboot") == 0) {
    DBG_PRINTF("reboot\n");
    MQTTclient.publish(debug, " << Rebooting... >>", false);
    delay(5000);
    ESP.restart();  //Now restart ESP
    delay(5000);

    return;
  }

  if (char *value=strstr((const char *)payload, "puissance=")) {
    snprintf(web.puissance, sizeof(web.puissance), "%sW", value+strlen("puissance="));
    DBG_PRINTF("webPuissance=%s\n",web.puissance);
    return;
  }

  if (strstr((const char *)payload, "ipSalon:\"")) {
    COPY_STR(web.ipSalon,(const char *)&payload[strlen("ipSalon:\"")]);
    if (web.ipSalon[0])
      web.ipSalon[strlen(web.ipSalon)-1]=0;
    DBG_PRINTF("webIpSalon:%s\n",web.ipSalon);
    return;
  }

  if (strstr((const char *)payload, "ipChNO:\"")) {
    COPY_STR(web.ipChNO,(const char *)&payload[strlen("ipChNO:\"")]);
    if (web.ipChNO[0])
      web.ipChNO[strlen(web.ipChNO)-1]=0;
    DBG_PRINTF("webIpChNO:%s\n",web.ipChNO);
    return;
  }

  if (strstr((const char *)payload, "ipCuisine:\"")) {
    COPY_STR(web.ipCuisine,(const char *)&payload[strlen("ipCuisine:\"")]);
    if (web.ipCuisine[0])
      web.ipCuisine[strlen(web.ipCuisine)-1]=0;
    DBG_PRINTF("webIpCuisine:%s\n",web.ipCuisine);
    return;
  }

  if (strcmp((const char *)payload, "reinit") == 0)  //Start WiFiManager after erasing previously stored settings
  {
    DBG_PRINTLN("Reinit");
    MQTTclient.publish(debug, " << Reinitializing...(erase flash, reboot and start WiFiManager) >>", false);
    delay(500);
    bool formated = LittleFS.format();  //Erase flash
    if (formated) {
      DBG_PRINTF("format succes\n");
    } else {
      DBG_PRINTF("format error\n");
    }
    WiFiDisconnectAndRestart();
    return;
  }

  if (strcmp((const char *)payload, "wifidisconnect") == 0)  //Start WiFiManager after erasing previously stored settings
  {
    DBG_PRINTLN("wifi disconnect");
    MQTTclient.publish(debug, " << WiFi disconnect >>", false);
    delay(500);
    WiFi.disconnect();  //Start WiFiManager  for reconfiguration
    return;
  }

  if (strcmp((const char *)payload, "wifimanager") == 0)  //Start WiFiManager with previously stored settings
  {
    DBG_PRINTLN("Wifimanager");
    MQTTclient.publish(debug, " << Starting WiFiManager... >>", false);

    WiFiDisconnectAndRestart();
    return;
  }

  if (strcmp((const char *)payload, "debugon") == 0)  //Send some info on debug payload, see debug2mqtt() in the code
  {
    DBG_PRINTLN("debugon");
    MQTTclient.publish(debug, " << Debug ON >>", false);
    app.debugit = true;
    return;
  }

  if (strcmp((const char *)payload, "traceon") == 0) {  //Publish valid MOSI frames (hex) on <debug>/trame
    setTrace(true);
    MQTTclient.publish(debug, " << Trace ON >>", false);
    return;
  }

  if (strcmp((const char *)payload, "traceoff") == 0) {
    char msg[60];
    sprintf(msg, " << Trace OFF, dropped frames: %u >>", (unsigned)traceDropped());
    setTrace(false);
    MQTTclient.publish(debug, msg, false);
    return;
  }

  if (strcmp((const char *)payload, "connexion") == 0) {
    DBG_PRINTLN("connexion");
    sendConnexion2mqtt(false);
    return;
  }

  if (strcmp((const char *)payload, "debugoff") == 0) {
    DBG_PRINTLN("debugoff");
    MQTTclient.publish(debug, " << Debug OFF >>", false);
    app.debugit = false;
    return;
  }

  if (strcmp((const char *)payload, "help") == 0) {
    MQTTclient.publish(debug, "<reboot> -> restart Arduino & ESP8266 | <reinit> -> erase flash and start WiFiManager", false);
    MQTTclient.publish(debug, "<wifimanager> -> Start WiFiManager | <debugon><debugoff> -> Show some info on debug topic", false);
    return;
  }

  if (strcmp((const char *)payload, "infoOn") == 0) {
    DBG_PRINTF("infOn\n");

    MQTTclient.publish(debug, "Info On", false);
    app.modeInfo = true;
    return;
  }

  if (strcmp((const char *)payload, "infoOff") == 0) {
    DBG_PRINTF("infOff\n");
    MQTTclient.publish(debug, "Info Off", false);
    app.modeInfo = false;
    return;
  }
  MQTTclient.publish(debug, " << Unknown service command >>", false);
  DBG_PRINTLN("unknown");
}
