/*
##############################################################################################################################################################################################
MHI ESP2MQTT Interface v1.0.0
Arduino-based communication interface for Mitsubishi Heavy Industries (MHI) SRK/SRF series air conditioners.
Connects to the MHI CNS connector and synchronizes to its Serial Peripheral Interface (SPI). Updates from the MHI are processed and sent via serial to an ESP8266 running an MQTT MQTTclient.
Updates received via MQTT are sent from the ESP8266 to the Arduino over serial and injected into the SPI data frames to update the MHI.
R.J. Dekker, June 2018
##############################################################################################################################################################################################
*/
#include "Include.h"

//Access point that WiFiManager starts for configuration. Name and password should be set below before flashing.
//This is hardcoded and cannot be changed later.
//#define configSSID "ESPClim"  //AP name (give every unit a unique name before flashing)
extern char configSSID[50];
extern char configPW[];  //Password to connect to the AP

extern char Thing[20];
extern char staticIP[16];
extern char masqueIP[16];
extern char gatewayIP[16];
extern char mqtt_server[16];
extern char mqtt_port[9];
extern char mqtt_user[20];
extern char mqtt_pass[20];
extern char wifiTimeout[3];  //Timeout in minutes (max. 99) before WiFi configuration portal is turned off and the ESP tries to connect again to the previously configured AP (if any)
extern char Room[20];
extern char topic_prefix[42];
extern char MQTT_SET_PREFIX[60];
extern char MQTT_OP_PREFIX[60];
extern char MQTT_ERR_OP_PREFIX[60];
extern char debug[60];    //Send only
extern char service[60];  //Receive only

extern PubSubClient MQTTclient;
#ifdef TELNET_DEBUG
extern RemoteDebug Debug;
#endif
extern NTPClient timeClient;

extern String savedWiFiSSID;
extern String savedWiFiPass;
char webPuissance[6]="";
char webIpSalon[40]="";
char webIpChNO[40]="";
char webIpCuisine[40]="";

struct tm tReboot;
bool debugit = false;  //Send some info (eg. MHI SPI bit field updates and errors) to debug topic
bool modeInfo = false;
int connectionFails = 0;  //Count number of failed MQTT connection attempts for restart

//                               HEAT        COOL        AUTO        DRY         FAN
static byte modeValues[5] = { 0b00010001, 0b00001001, 0b00000001, 0b00000101, 0b00001101 };  //Used to extract current mode from bit field 4

void startConfigPortal();

void WiFiDisconnectAndRestart() {  
//  WiFi.persistent(false);
#ifdef ESP32
  WiFi.disconnect(true, true);  //Start WiFiManager  for reconfiguration
#else
  WiFi.disconnect(true);  //Start WiFiManager  for reconfiguration
#endif
  delay(1000);
  ESP.restart();
  delay(1000);
}
int stateFlag=0;

void saveJson (bool);
void stateTest(){
  if (!stateFlag)
    return;
  int state=stateFlag;
  stateFlag=0;
  switch (state)
  {
    case GO_REINIT:{
#ifdef SERIAL_DEBUG
      Serial.println("Reinit");
#endif
#ifdef TELNET_DEBUG
      Debug.println("Reinit");
#endif

      bool formated = LittleFS.format();  //Erase flash
      if (formated) {
#ifdef SERIAL_DEBUG
        Serial.printf("format succes\n");
#endif
#ifdef TELNET_DEBUG
        Debug.printf("format succes\n");
#endif
      } else {
#ifdef SERIAL_DEBUG
        Serial.printf("format error\n");
#endif
#ifdef TELNET_DEBUG
        Debug.printf("format error\n");
#endif
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
  if (debugit) {
    MQTTclient.publish(debug, text, false);
  }
}


int nbDeconnect = 0;
int myConnexionCount = 0;
void sendConnexion2mqtt(bool start) {
  char msg[300];
  time_t rawtime = timeClient.getEpochTime();
  struct tm *tcur = localtime(&rawtime);
  if (myConnexionCount == 0)
    tReboot = *tcur;

  String ip = WiFi.localIP().toString();
  sprintf(msg, "%s num:%d,%dh:%02d  %d/%d/%d, last reboot:%dh:%02d  %d/%d/%d\n SSID:%s, ip:%s, MQTT:%s:%s, deconnect:%d\n",
          Thing, myConnexionCount++,
          tcur->tm_hour,    /* Hours.   [0-23]      */
          tcur->tm_min,     /* Minutes. [0-59]      */
          tcur->tm_mday,    /* Day.     [1-31]      */
          tcur->tm_mon + 1, /* Month.   [0-11]      */
          tcur->tm_year + 1900,

          tReboot.tm_hour,    /* Hours.   [0-23]      */
          tReboot.tm_min,     /* Minutes. [0-59]      */
          tReboot.tm_mday,    /* Day.     [1-31]      */
          tReboot.tm_mon + 1, /* Month.   [0-11]      */
          tReboot.tm_year + 1900,
          savedWiFiSSID.c_str(), ip.c_str(), mqtt_server, mqtt_port, nbDeconnect);
#ifdef SERIAL_DEBUG
  Serial.printf("publish topic:%s\npayload:%s\n", debug, msg);
#endif
  bool b = MQTTclient.publish(debug, msg, true);  //Publish message to debug topic to test/notify MQTT connection
#ifdef TELNET_DEBUG
  Debug.printf("ret publish:%d,payloadlen:%d, publish topic:%s\npayload:%s\n", b, strlen(msg), debug, msg);
#endif
}

long unsigned int lastDiscMillis = 0;
long unsigned int lastPrintMillis = 0;
extern void startWebPortal();

void setWiFi() {
#ifdef TELNET_DEBUG
  Debug.println("Start setWiFI");
#endif
  char cSSID[30];
  savedWiFiSSID.toCharArray(cSSID, savedWiFiSSID.length() + 1);
  char cPass[30];
  savedWiFiPass.toCharArray(cPass, savedWiFiPass.length() + 1);

#ifdef SERIAL_DEBUG
  Serial.printf("Wifi disconnect in Loop -> reconnecting\n");
#endif

  bool ledValue = LED_LOW;
  int status;
  nbDeconnect++;
  for (int i = 0; i < 6*2; i++) { //12 fois 10 secondes = 2mn
    WiFi.disconnect();
    status = WiFi.begin(cSSID, cPass);
#ifdef SERIAL_DEBUG
    Serial.printf("\nStatus=%d\n", status);
#endif
    if (status == WL_CONNECTED) {
      digitalWrite(LED_BUILTIN, LED_LOW);
      sendConnexion2mqtt (false);
      return;
    }
    for (int j = 0; j < 30; j++) {  //30 fois 325ms = 10 secondes
#ifdef SERIAL_DEBUG
      Serial.print(".");
#endif
      digitalWrite(LED_BUILTIN, ledValue);
      status = WiFi.status();
      if ((ledValue == LED_LOW) && (status == WL_CONNECTED))
        return;
      if (ledValue==LED_LOW)
        delay(600);
      else
        delay(50);
      ledValue ^= 1;
    }
  }
  ESP.restart();
}

void printInfo();
char MQTTconnectedFlag[4];

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

    //MQTT connection: Attempt to connect to MQTT broker 3 times: SUCCESS -> continue | FAILED restart ESP
    //On restart it will first try to connect to the previously set AP. If that fails the config portal will be started.
    //If the config portal is not used within wifiTimeout (set in portal), the ESP will retry connecting to the previously set AP again.
    if (MQTTclient.connect(configSSID, mqtt_user, mqtt_pass)) 
    {
#ifdef SERIAL_DEBUG
      Serial.println("connected!");
#endif
#ifdef TELNET_DEBUG
      Debug.println("connected!");
#endif

      //Subscribe to topics that control the MHI state
      char tmp[100];
      sprintf(tmp, "%s#", MQTT_SET_PREFIX);
      MQTTclient.subscribe(tmp, 1);
#ifdef SERIAL_DEBUG
      Serial.printf("Subscribe to :%s\n", tmp);
#endif
#ifdef TELNET_DEBUG
      Debug.printf("Subscribe to :%s\n", tmp);
#endif

      sprintf(tmp, "%s/#", MQTT_OP_PREFIX);
      MQTTclient.subscribe(tmp, 1);
#ifdef SERIAL_DEBUG
      Serial.printf("Subscribe to :%s\n", tmp);
#endif
#ifdef TELNET_DEBUG
      Debug.printf("Subscribe to :%s\n", tmp);
#endif

      sprintf(tmp, "%s/#", MQTT_ERR_OP_PREFIX);
      MQTTclient.subscribe(tmp, 1);
#ifdef SERIAL_DEBUG
      Serial.printf("Subscribe to :%s\n", tmp);
#endif
#ifdef TELNET_DEBUG
      Debug.printf("Subscribe to :%s\n", tmp);
#endif

      sprintf(tmp, "%s/#", service);
      MQTTclient.subscribe(tmp, 1);
#ifdef SERIAL_DEBUG
      Serial.printf("Subsribe to %s\n", tmp);
#endif
#ifdef TELNET_DEBUG
      Debug.printf("Subsribe to %s\n", tmp);
#endif

      printInfo();
      sendConnexion2mqtt(true);
      strcpy(MQTTconnectedFlag,"yes");
      connectionFails = 0;
    } 
    else 
    {
#ifdef SERIAL_DEBUG
      Serial.print("failed, rc = ");
#endif
#ifdef TELNET_DEBUG
      Debug.print("failed, rc = ");
#endif
#ifdef SERIAL_DEBUG
      Serial.println(MQTTclient.state());
#endif
#ifdef TELNET_DEBUG
      Debug.println(MQTTclient.state());
#endif
#ifdef SERIAL_DEBUG
      Serial.print("Failed connection attempts: ");
#endif
#ifdef TELNET_DEBUG
      Debug.print("Failed connection attempts: ");
#endif
#ifdef SERIAL_DEBUG
      Serial.println(connectionFails);
#endif
#ifdef TELNET_DEBUG
      Debug.println(connectionFails);
#endif

      if (++connectionFails == 30) {
#ifdef SERIAL_DEBUG
        Serial.println("MQTT broker connection timeout...restarting!");
#endif
#ifdef TELNET_DEBUG
        Debug.println("MQTT broker connection timeout...restarting!");
#endif
        ESP.restart();
        //WiFiDisconnectAndRestart();
      }
#ifdef SERIAL_DEBUG
      Serial.println("Try again in 5 seconds...");
#endif
#ifdef TELNET_DEBUG
      Debug.println("Try again in 5 seconds...");
#endif
      strcpy(MQTTconnectedFlag,"no");
      delay(1000);
    }
  }
}

#if !defined(WEB_SERVER) || defined(MULTICORE)
static void loopWebServer(void){}
#else
  void loopWebServer(void);
#endif

extern char ssid[];
extern char password[];
void listNetworks();

void loopMHI();
int mycount = 0;
unsigned long int tWait=60000;
unsigned long int lastTimeUpdateMillis=0,lastTimeServerMillis=0,lastTimeMQTTMillis=0;
unsigned long int timeLoop,minLoop=100000,maxLoop=0,moyLoop=0,nbLoop=0;
void loop() {
  unsigned long curMillis = millis();
  if (mycount>10)
  {
    timeLoop=curMillis-timeLoop;
    if (timeLoop){
      if (timeLoop>maxLoop)
        maxLoop=timeLoop;
      if (timeLoop<minLoop)
        minLoop=timeLoop;
      moyLoop+=timeLoop;
    }
  }  
  else
    mycount++;
  timeLoop=curMillis;

  ArduinoOTA.handle();  //Handle Arduino OTA updates
#ifdef TELNET_DEBUG
  Debug.handle();
#endif
  digitalWrite(LED_BUILTIN, LED_LOW);

  if (!WiFi.isConnected())
    setWiFi();

  if ((curMillis - lastTimeUpdateMillis) > tWait) {
    lastTimeUpdateMillis = curMillis;
    unsigned long beforeTimeMillis = millis();
#ifdef TELNET_DEBUG
    Debug.printf("Before NTPClient.update\n");
#endif
    bool b = timeClient.update();
    unsigned long int tmp = millis() - beforeTimeMillis;
    if (b)
      tWait = 600000;
    else
      tWait = 60000;
#ifdef TELNET_DEBUG
    Debug.printf("Update NTP:%d, durée NTPClient: %dms\n", b, tmp);
#endif
  }

  if (nbLoop&&(curMillis - lastPrintMillis) > 10000) {
    int core = 1;
#ifdef MULTICORE
    esp_task_wdt_reset();  //Reset du Watchdog toutes  les 10s
    core = xPortGetCoreID();
#endif
    lastPrintMillis = curMillis;
    moyLoop/=nbLoop;
#ifdef SERIAL_DEBUG
    Serial.printf("Core:%d, configSSID:%s, Room:%s, SSID:%s, local IP:%s, Deconnexions:%d\n",
                core, configSSID, Thing, savedWiFiSSID.c_str(), WiFi.localIP().toString().c_str(), nbDeconnect);
#endif
#ifdef TELNET_DEBUG
    Debug.printf("Core:%d, configSSID:%s, Room:%s, SSID:%s, local IP,  Deconnexions:%d\n", 
                core, configSSID, Thing, savedWiFiSSID.c_str(), WiFi.localIP().toString().c_str(), nbDeconnect);
#endif
  //  listNetworks();
    nbLoop=0;
    minLoop=100000;
    nbLoop=maxLoop=moyLoop=nbLoop=0;
  }

  if (!MQTTclient.connected())  //Check MQTT connection
  {
#ifdef SERIAL_DEBUG
    Serial.printf("MQTT deconnected, call connect()\n");
#endif
#ifdef TELNET_DEBUG
    Debug.printf("MQTT deconnected, call connect()\n");
#endif
    connect();  //Connect first time. Reconnect when connection lost.
  }
  if ((curMillis - lastTimeMQTTMillis) > 100) {
    lastTimeMQTTMillis = curMillis;
    MQTTclient.loop();
  }
  loopMHI();

  loopWebServer();
  stateTest();
  nbLoop++;
}


void setTrace(bool enable);
uint32_t traceDropped();

void TopicService(unsigned char *payload, unsigned int length) {
  //SERVICE COMMANDS
  if (strcmp((const char *)payload, "reboot") == 0) {
#ifdef SERIAL_DEBUG
    Serial.printf("reboot\n");
#endif
#ifdef TELNET_DEBUG
    Debug.printf("reboot\n");
#endif
    MQTTclient.publish(debug, " << Rebooting... >>", false);
    delay(5000);
    ESP.restart();  //Now restart ESP
    delay(5000);

    return;
  }

  if (char *value=strstr((const char *)payload, "puissance=")) {
    sprintf(webPuissance,"%sW",value+strlen("puissance="));
#ifdef SERIAL_DEBUG
    Serial.printf("webPuissance=%s\n",webPuissance);
#endif
#ifdef TELNET_DEBUG
    Debug.printf("webPuissance=%s\n",webPuissance);
#endif
    return;
  }

  if (strstr((const char *)payload, "ipSalon:\"")) {
    strcpy(webIpSalon,(const char *)&payload[strlen("ipSalon:\"")]);
    webIpSalon[strlen(webIpSalon)-1]=0;
#ifdef SERIAL_DEBUG
    Serial.printf("webIpSalon:%s\n",webIpSalon);
#endif
#ifdef TELNET_DEBUG
    Debug.printf("webIpSalon:%s\n",webIpSalon);
#endif
    return;
  }

  if (strstr((const char *)payload, "ipChNO:\"")) {
    strcpy(webIpChNO,(const char *)&payload[strlen("ipChNO:\"")]);
    webIpChNO[strlen(webIpChNO)-1]=0;
#ifdef SERIAL_DEBUG
    Serial.printf("webIpChNO:%s\n",webIpChNO);
#endif
#ifdef TELNET_DEBUG
    Debug.printf("webIpChNO:%s\n",webIpChNO);
#endif
    return;
  }

  if (strstr((const char *)payload, "ipCuisine:\"")) {
    strcpy(webIpCuisine,(const char *)&payload[strlen("ipCuisine:\"")]);
    webIpCuisine[strlen(webIpCuisine)-1]=0;
#ifdef SERIAL_DEBUG
    Serial.printf("webIpCuisine:%s\n",webIpCuisine);
#endif
#ifdef TELNET_DEBUG
    Debug.printf("webIpCuisine:%s\n",webIpCuisine);
#endif
    return;
  }

  if (strcmp((const char *)payload, "reinit") == 0)  //Start WiFiManager after erasing previously stored settings
  {
#ifdef SERIAL_DEBUG
    Serial.println("Reinit");
#endif
#ifdef TELNET_DEBUG
    Debug.println("Reinit");
#endif
    MQTTclient.publish(debug, " << Reinitializing...(erase flash, reboot and start WiFiManager) >>", false);
    delay(500);
    bool formated = LittleFS.format();  //Erase flash
    if (formated) {
#ifdef SERIAL_DEBUG
      Serial.printf("format succes\n");
#endif
#ifdef TELNET_DEBUG
      Debug.printf("format succes\n");
#endif
    } else {
#ifdef SERIAL_DEBUG
      Serial.printf("format error\n");
#endif
#ifdef TELNET_DEBUG
      Debug.printf("format error\n");
#endif
    }
    WiFiDisconnectAndRestart();
    return;
  }

  if (strcmp((const char *)payload, "wifidisconnect") == 0)  //Start WiFiManager after erasing previously stored settings
  {
#ifdef SERIAL_DEBUG
    Serial.println("wifi disconnect");
#endif
#ifdef TELNET_DEBUG
    Debug.println("wifi disconnect");
#endif
    MQTTclient.publish(debug, " << WiFi disconnect >>", false);
    delay(500);
    WiFi.disconnect();  //Start WiFiManager  for reconfiguration
    return;
  }

  if (strcmp((const char *)payload, "wifimanager") == 0)  //Start WiFiManager with previously stored settings
  {
#ifdef SERIAL_DEBUG
    Serial.println("Wifimanager");
#endif
#ifdef TELNET_DEBUG
    Debug.println("Wifimanager");
#endif
    MQTTclient.publish(debug, " << Starting WiFiManager... >>", false);

    WiFiDisconnectAndRestart();
    return;
  }

  if (strcmp((const char *)payload, "debugon") == 0)  //Send some info on debug payload, see debug2mqtt() in the code
  {
#ifdef SERIAL_DEBUG
    Serial.println("debugon");
#endif
#ifdef TELNET_DEBUG
    Debug.println("debugon");
#endif
    MQTTclient.publish(debug, " << Debug ON >>", false);
    debugit = true;
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
#ifdef SERIAL_DEBUG
    Serial.println("connexion");
#endif
#ifdef TELNET_DEBUG
    Debug.println("connexion");
#endif
    sendConnexion2mqtt(false);
    return;
  }

  if (strcmp((const char *)payload, "debugoff") == 0) {
#ifdef SERIAL_DEBUG
    Serial.println("debugoff");
#endif
#ifdef TELNET_DEBUG
    Debug.println("debugoff");
#endif
    MQTTclient.publish(debug, " << Debug OFF >>", false);
    debugit = false;
    return;
  }

  if (strcmp((const char *)payload, "help") == 0) {
    MQTTclient.publish(debug, "<reboot> -> restart Arduino & ESP8266 | <reinit> -> erase flash and start WiFiManager", false);
    MQTTclient.publish(debug, "<wifimanager> -> Start WiFiManager | <debugon><debugoff> -> Show some info on debug topic", false);
    return;
  }

  if (strcmp((const char *)payload, "infoOn") == 0) {
#ifdef SERIAL_DEBUG
    Serial.printf("infOn\n");
#endif
#ifdef TELNET_DEBUG
    Debug.printf("infOn\n");
#endif

    MQTTclient.publish(debug, "Info On", false);
    modeInfo = true;
    return;
  }

  if (strcmp((const char *)payload, "infoOff") == 0) {
#ifdef SERIAL_DEBUG
    Serial.printf("infOff\n");
#endif
#ifdef TELNET_DEBUG
    Debug.printf("infOff\n");
#endif
    MQTTclient.publish(debug, "Info Off", false);
    modeInfo = false;
    return;
  }
  MQTTclient.publish(debug, " << Unknown service command >>", false);
#ifdef SERIAL_DEBUG
  Serial.println("unknown");
#endif
#ifdef TELNET_DEBUG
  Debug.println("unknown");
#endif
}
