#include "include.h"
#include "params_logic.h"
#include <WiFiManager.h>   //https://github.com/tzapu/WiFiManager

//Initial values (see mhi::Config) can be changed at any time from the WiFiManager configuration portal and will be stored in flash memory. If there are different values in config.json, they are overwritten.
mhi::Config config;
char topic_prefix[42] = "";
char MQTT_SET_PREFIX[60] = "set/";
char MQTT_OP_PREFIX[60] = "OpData";
char MQTT_ERR_OP_PREFIX[60] = "ErrOpData";
char debug[60] = "debug";      //Send only
char service[60] = "service";  //Receive only

String savedWiFiSSID;
String savedWiFiPass;
char configSSID[50] = "";
char configPW[] = "climatiseur";  //Password to connect to the AP

WiFiClient espClient;
PubSubClient MQTTclient(espClient);
#ifdef TELNET_DEBUG
RemoteDebug Debug;
#endif
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200);  //pool.ntp.org");

#if defined(WEB_SERVER) || defined (AS_WEB_SERVER)
void loadWebServer ();
void setApplicationWebServerEnabled(bool enabled);
#endif

void MQTT_subscribe_callback(const char *topic, byte *payload, unsigned int length);
extern void WiFiDisconnectAndRestart();
extern void setupMHI();
extern void setupTime();

//Flag for saving data
bool shouldSaveConfig = false;

struct WifiPortal {
  WiFiManager manager;
  WiFiManagerParameter staticIPParameter;
  WiFiManagerParameter masqueIPParameter;
  WiFiManagerParameter gatewayIPParameter;
  WiFiManagerParameter dnsIPParameter;
  WiFiManagerParameter mqttServerParameter;
  WiFiManagerParameter mqttPortParameter;
  WiFiManagerParameter mqttUserParameter;
  WiFiManagerParameter mqttPassParameter;
  WiFiManagerParameter wifiTimeoutParameter;
  WiFiManagerParameter roomParameter;
  WiFiManagerParameter thingParameter;

  WifiPortal()
      : staticIPParameter("staticIP", "Static IP", config.staticIP, 16),
        masqueIPParameter("masqueIP", "Masque IP", config.masqueIP, 16),
        gatewayIPParameter("gatewayIP", "Gateway IP", config.gatewayIP, 16),
        dnsIPParameter("dnsIP", "DNS IP", config.dnsIP, 16),
        mqttServerParameter("webServer", "MQTT Server", config.mqtt_server, 16),
        mqttPortParameter("port", "MQTT Port", config.mqtt_port, 9),
        mqttUserParameter("user", "MQTT Username", config.mqtt_user, 20),
        mqttPassParameter("pass", "MQTT Password", config.mqtt_pass, 20),
        wifiTimeoutParameter("timeout", "WiFi recovery delay (min)", config.wifiTimeout, 3),
        roomParameter("Room", "Room name", config.Room, 20),
        thingParameter("Thing", "Thing name", config.Thing, 20) {}
};

static WifiPortal& getWifiPortal() {
  static WifiPortal portal;
  return portal;
}

void startWifiRecoveryPortal();
void handleWifiRecoveryPortal();
static bool wifiRecoveryPortalActive = false;

//Callback notifying us of the need to save WiFiManager config to FS
void saveConfigCallback() {
#ifdef SERIAL_DEBUG
  Serial.println("Should save config");
#endif
#ifdef TELNET_DEBUG
  Debug.println("Should save config");
#endif
  shouldSaveConfig = true;
}

void listNetworks() {
  // scan for nearby networks:
#ifdef SERIAL_DEBUG
  Serial.println("** Scan Networks **");
#endif
  int numSsid = WiFi.scanNetworks();
#ifdef SERIAL_DEBUG
  if (numSsid == -1)
    Serial.println("Couldn't get a WiFi connection");
  else {
  // print the network number and name for each network found:
    for (int thisNet = 0; thisNet < numSsid; thisNet++)
      Serial.printf("\(%d\) %s \tSignal:%d dBm\n",thisNet,WiFi.SSID(thisNet).c_str(),WiFi.RSSI(thisNet));
  }
#endif
}

// Le scan Wi-Fi bloquant prenait plusieurs secondes à chaque affichage de la page Paramètres :
// scan asynchrone, résultats copiés dans un cache réutilisé par /param_data.
#define WIFI_CACHE_MAX 20
#define WIFI_CACHE_AGE_MS 30000UL
static struct { String ssid; int rssi; } wifiCache[WIFI_CACHE_MAX];
static int wifiCacheCount = 0;
static unsigned long wifiCacheTime = 0;
static bool wifiCacheValid = false;

void refreshWifiCache() {
  int n = WiFi.scanComplete();
  if (n >= 0) {
    wifiCacheCount = (n > WIFI_CACHE_MAX) ? WIFI_CACHE_MAX : n;
    for (int i = 0; i < wifiCacheCount; i++) {
      wifiCache[i].ssid = WiFi.SSID(i);
      wifiCache[i].rssi = WiFi.RSSI(i);
    }
    WiFi.scanDelete();
    wifiCacheTime = millis();
    wifiCacheValid = true;
  } else if (n == WIFI_SCAN_RUNNING) {
    return;
  }
  if (!wifiCacheValid || millis() - wifiCacheTime > WIFI_CACHE_AGE_MS)
    WiFi.scanNetworks(true);
}

char *getParametersString (char *tmp, size_t size)
{
  mhi::buildParamsBase(tmp, size, config);
  refreshWifiCache();
  for (int thisNet = 0; thisNet < wifiCacheCount; thisNet++) {
#ifdef SERIAL_DEBUG
    Serial.printf("\(%d\) %s \tSignal:%d dBm\n",thisNet,wifiCache[thisNet].ssid.c_str(),wifiCache[thisNet].rssi);
#endif
    // Réseau suivant ignoré s'il ne tient plus dans le tampon
    if (!mhi::appendNetwork(tmp, size, wifiCache[thisNet].ssid.c_str(), savedWiFiSSID.c_str(),
                            savedWiFiPass.c_str(), wifiCache[thisNet].rssi))
      break;
  }
#ifdef SERIAL_DEBUG
  Serial.printf("parametersString len:%d\n",strlen(tmp));
#endif
  return tmp;
}

void saveJson (bool f){
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
#ifdef SERIAL_DEBUG
  if (!configFile) 
    Serial.println("failed to open config file for writing");
  else
    Serial.println("succes writing config file");
#endif
#ifdef TELNET_DEBUG
  if (!configFile) 
    Debug.println("failed to open config file for writing");
  else
    Debug.println("succes writing config file");
#endif

  serializeJson(json,Serial);
  serializeJson(json,configFile);
  configFile.close();
    //end save
}

static void applyWifiPortalParameters() {
  WifiPortal& portal = getWifiPortal();
  COPY_STR(config.staticIP, portal.staticIPParameter.getValue());
  COPY_STR(config.masqueIP, portal.masqueIPParameter.getValue());
  COPY_STR(config.gatewayIP, portal.gatewayIPParameter.getValue());
  COPY_STR(config.dnsIP, portal.dnsIPParameter.getValue());
  COPY_STR(config.mqtt_server, portal.mqttServerParameter.getValue());
  COPY_STR(config.mqtt_port, portal.mqttPortParameter.getValue());
  COPY_STR(config.mqtt_user, portal.mqttUserParameter.getValue());
  COPY_STR(config.mqtt_pass, portal.mqttPassParameter.getValue());
  COPY_STR(config.wifiTimeout, portal.wifiTimeoutParameter.getValue());
  COPY_STR(config.Room, portal.roomParameter.getValue());
  COPY_STR(config.Thing, portal.thingParameter.getValue());
}

void startWifiRecoveryPortal() {
  if (wifiRecoveryPortalActive)
    return;

  WifiPortal& portal = getWifiPortal();
  portal.manager.setDisableSTAConn(false);
  portal.manager.setConfigPortalBlocking(false);
  portal.manager.setConfigPortalTimeout(0);
#if defined(WEB_SERVER) || defined(AS_WEB_SERVER)
  setApplicationWebServerEnabled(false);
#endif
  portal.manager.startConfigPortal(configSSID, configPW);
  if (!portal.manager.getConfigPortalActive()) {
#if defined(WEB_SERVER) || defined(AS_WEB_SERVER)
    setApplicationWebServerEnabled(true);
#endif
    Serial.println("Unable to start WiFi recovery portal");
    return;
  }
  wifiRecoveryPortalActive = true;
}

void handleWifiRecoveryPortal() {
  if (!wifiRecoveryPortalActive)
    return;

  WifiPortal& portal = getWifiPortal();
  portal.manager.process();

  if (WiFi.isConnected() && shouldSaveConfig) {
    applyWifiPortalParameters();
    saveJson(false);
    ESP.restart();
    return;
  }

  if (!WiFi.isConnected() && portal.manager.getConfigPortalActive())
    return;

  portal.manager.stopConfigPortal();
  wifiRecoveryPortalActive = false;
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
#if defined(WEB_SERVER) || defined(AS_WEB_SERVER)
  setApplicationWebServerEnabled(true);
#endif
}

#ifdef ESP32
#include <WebServer.h>
extern WebServer webServer;
#else
#include <ESP8266WebServer.h>
extern ESP8266WebServer webServer;
#endif

void newParams() {
  int nb = webServer.args();
#ifdef TELNET_DEBUG
  Debug.println("*******************Receive New params !!!!!!!!!!!!!!!!!!!");
#endif
#ifdef SERIAL_DEBUG
  Serial.printf("*******************Receive New params nb=%d!!!!!!!!!!!!!!!!!!!\n",nb);
#endif
  for (int i = 0; i < nb; i++) {
#ifdef TELNET_DEBUG
    Debug.println("Param:" + webServer.argName(i));
#endif
#ifdef SERIAL_DEBUG
    Serial.println("Param:" + webServer.argName(i));
    Serial.println("value:" + webServer.arg(i));
#endif
  }
  mhi::NewParams np;
  if (!mhi::parseNewParams(webServer.arg(0).c_str(), np)) {
    webServer.send(400, "text/plain", "400: Invalid Request");
    return;
  }
  webServer.send(200, "text/plain", "restarting...");
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

  saveJson(true);
  ESP.restart();
}


void printEspInfo() {
#ifdef SERIAL_DEBUG
  Serial.println();
  Serial.printf_P(PSTR("CPU frequency[Hz]=%lu\n"), F_CPU);
#ifndef ESP32
  Serial.printf("ESP.getCoreVersion()=%s\n", ESP.getCoreVersion().c_str());
  Serial.printf("ESP.checkFlashCRC()=%i\n", ESP.checkFlashCRC());
#endif
  Serial.printf("ESP.getSdkVersion()=%s\n", ESP.getSdkVersion());
#endif
#ifdef TELNET_DEBUG
  Debug.println();
  Debug.printf_P(PSTR("CPU frequency[Hz]=%lu\n"), F_CPU);
#ifndef ESP32
  Debug.printf("ESP.getCoreVersion()=%s\n", ESP.getCoreVersion().c_str());
  Debug.printf("ESP.checkFlashCRC()=%i\n", ESP.checkFlashCRC());
#endif
  Debug.printf("ESP.getSdkVersion()=%s\n", ESP.getSdkVersion());
#endif
}
  
/************************************** setup ************************************/
void setup() {
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
  Serial.setDebugOutput(true);
#endif
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_LOW);
  //Read configuration from FS json
#ifdef SERIAL_DEBUG
  Serial.println("Mounting FS...");
#endif
#ifdef TELNET_DEBUG
  Debug.println("Mounting FS...");
#endif
  bool formatFlag=true;
  if (LittleFS.begin()) {
#ifdef SERIAL_DEBUG
    Serial.println("Mounted file system");
#endif
#ifdef TELNET_DEBUG
    Debug.println("Mounted file system");
#endif

    if (LittleFS.exists("/config.json")) 
    {
      //File exists, reading and loading
#ifdef SERIAL_DEBUG
      Serial.println("Reading config file");
#endif
#ifdef TELNET_DEBUG
      Debug.println("Reading config file");
#endif
      File configFile = LittleFS.open("/config.json", "r");

      if (configFile) {
        size_t size = configFile.size();
#ifdef SERIAL_DEBUG
        Serial.printf("Opened config file, size:%d\n",size);
#endif
#ifdef TELNET_DEBUG
        Debug.printf("Opened config file, size:%d\n",size);
#endif

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

#ifdef SERIAL_DEBUG
          Serial.println("\nParsed json");
#endif
#ifdef TELNET_DEBUG
          Debug.println("\nParsed json");
#endif
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
#ifdef SERIAL_DEBUG
          Serial.println("Failed to load json config");
#endif
#ifdef TELNET_DEBUG
          Debug.println("Failed to load json config");
#endif
        }
      }
    }
  } 
  else 
  {
#ifdef SERIAL_DEBUG
    Serial.println("Failed to mount FS");
#endif
#ifdef TELNET_DEBUG
    Debug.println("Failed to mount FS");
#endif
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

  WifiPortal& portal = getWifiPortal();
  WiFiManager& wifiManager = portal.manager;

  wifiManager.setDebugOutput(false);

  //Set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  /*  bool res = WiFi.hostname("MonClimatiseur");
  if (MDNS.begin("MonClimatiseur")) {
    MDNS.addService("http", "tcp", 80);
  }
*/

  //Optional: Set static ip
  // wifiManager.setSTAStaticIPConfig(IPAddress(0,0,0,0), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
#ifdef SERIAL_DEBUG
  Serial.printf("staticIP:%s, masqueIP:%s, gatewayIP:%s, dnsIP:%s\n", config.staticIP, config.masqueIP, config.gatewayIP, config.dnsIP);
#endif
#ifdef TELNET_DEBUG
  Debug.printf("staticIP:%s, masqueIP:%s, gatewayIP:%s, dnsIP:%s\n", config.staticIP, config.masqueIP, config.gatewayIP, config.dnsIP);
#endif

  if (strcmp(config.staticIP, "0.0.0.0") && strcmp(config.masqueIP, "0.0.0.0") && strcmp(config.gatewayIP, "0.0.0.0") && strcmp(config.dnsIP, "0.0.0.0")) {
    IPAddress ip, mask, gate, dns;
    ip.fromString(config.staticIP);
    mask.fromString(config.masqueIP);
    gate.fromString(config.gatewayIP);
    dns.fromString(config.dnsIP);
    Serial.printf("call wifiManager.setSTAStaticIPConfig\n");
    wifiManager.setSTAStaticIPConfig(ip, gate, mask, dns);
  }
  //Add all parameters here
  wifiManager.addParameter(&portal.staticIPParameter);
  wifiManager.addParameter(&portal.masqueIPParameter);
  wifiManager.addParameter(&portal.gatewayIPParameter);
  wifiManager.addParameter(&portal.dnsIPParameter);

  wifiManager.addParameter(&portal.mqttServerParameter);
  wifiManager.addParameter(&portal.mqttPortParameter);
  wifiManager.addParameter(&portal.mqttUserParameter);
  wifiManager.addParameter(&portal.mqttPassParameter);
  wifiManager.addParameter(&portal.wifiTimeoutParameter);
  wifiManager.addParameter(&portal.roomParameter);
  wifiManager.addParameter(&portal.thingParameter);


  //Set minimum quality of signal so it ignores AP's under that quality
  //Defaults to 8%
  wifiManager.setMinimumSignalQuality(5);

  // Portail captif : niveau du signal en texte (dBm) au lieu des seules icônes
  wifiManager.setScanDispPerc(true);

  //Fetches ssid and pass and tries to connect
  //The recovery portal is opened later, while the firmware keeps retrying WiFi.
  snprintf(configSSID, sizeof(configSSID), "Clim%s", config.Thing);
#ifdef SERIAL_DEBUG
  Serial.printf("wifiManager.autoConnect(%s,%s)\n",configSSID, configPW);
#endif


#if 1
  if (config.PreloadSSID[0] && config.PreloadFlag[0])
  {
#ifdef SERIAL_DEBUG
    Serial.printf("PreloadWiFi %s,%s\n",config.PreloadSSID, config.PreloadPSW);
#endif
    wifiManager.preloadWiFi(config.PreloadSSID, config.PreloadPSW);
  }
#ifdef SERIAL_DEBUG
  else
    Serial.printf("No PreloadWiFi\n");
#endif
#endif
  saveJson(false);
  wifiManager.setEnableConfigPortal(false);
  bool wifiConnected = wifiManager.autoConnect(configSSID, configPW);
  wifiManager.setEnableConfigPortal(true);
  if (!wifiConnected) {
#ifdef SERIAL_DEBUG
    Serial.println("WiFi unavailable; retrying in background");
#endif
#ifdef TELNET_DEBUG
    Debug.println("WiFi unavailable; retrying in background");
#endif
  }
  saveJson(false);
#ifdef SERIAL_DEBUG
  Serial.printf("after wifiManager.autoConnect(%s,%s), connected=%d\n",configSSID, configPW, wifiConnected);
#endif
#ifdef TELNET_DEBUG
  Debug.begin(configSSID);
#endif
  if (config.PreloadSSID[0] && config.PreloadFlag[0]) {
    savedWiFiSSID = config.PreloadSSID;
    savedWiFiPass = config.PreloadPSW;
  } else {
    savedWiFiSSID = wifiManager.getWiFiSSID();
    savedWiFiPass = wifiManager.getWiFiPass();
  }
#ifdef SERIAL_DEBUG
  Serial.printf("WiFi connected=%d, savedWiFiSSID:%s, configSSID:%s\n",wifiConnected,savedWiFiSSID.c_str(),configSSID);
#endif

  //Read updated parameters
  applyWifiPortalParameters();


  //Construct topic names
  //Topic prefix = "Room/Thing/"
  snprintf(topic_prefix, sizeof(topic_prefix), "%.20s/%.20s/", config.Room, config.Thing);

  //Start all topics with topic prefix
  {
    char tmp[60];
    snprintf(tmp, sizeof(tmp), "%s%s", topic_prefix, MQTT_SET_PREFIX);
    COPY_STR(MQTT_SET_PREFIX, tmp);
    snprintf(tmp, sizeof(tmp), "%s%s", topic_prefix, MQTT_OP_PREFIX);
    COPY_STR(MQTT_OP_PREFIX, tmp);
    snprintf(tmp, sizeof(tmp), "%s%s", topic_prefix, MQTT_ERR_OP_PREFIX);
    COPY_STR(MQTT_ERR_OP_PREFIX, tmp);
    snprintf(tmp, sizeof(tmp), "%s%s", topic_prefix, debug);
    COPY_STR(debug, tmp);
    snprintf(tmp, sizeof(tmp), "%s%s", topic_prefix, service);
    COPY_STR(service, tmp);
  }

  //Debug resulting topics to serial
#ifdef SERIAL_DEBUG
  Serial.println("Constructed topics:MQTT_SET_PREFIX,MQTT_OP_PREFIX,MQTT_ERR_OP_PREFIX\n");
#endif
#ifdef TELNET_DEBUG
  Debug.println("Constructed topics:MQTT_SET_PREFIX,MQTT_OP_PREFIX,MQTT_ERR_OP_PREFIX\n");
#endif


  //Save the custom parameters to FS
  if (shouldSaveConfig){
    saveJson(false);
    ESP.restart();
  }

#ifdef SERIAL_DEBUG
  Serial.printf("\nLocal IP adres:%s\n",WiFi.localIP().toString().c_str());
#endif
#ifdef TELNET_DEBUG
  Debug.printf("\nLocal IP adres:%s\n",WiFi.localIP().toString().c_str());
#endif

  {  //Connect to MQTT broker and set callback
    MQTTclient.setServer(config.mqtt_server, atoi(config.mqtt_port));
    MQTTclient.setCallback(MQTT_subscribe_callback);
  }
  //Configure Arduino OTA updater
  ArduinoOTA.setHostname(configSSID);  //Set OTA hostname and password (same as local access point for WiFiManager)
  ArduinoOTA.setPassword((const char *)configPW);
  //ArduinoOTA.setPort(8266);                                                                                      //Port defaults to 8266

  ArduinoOTA.onError([](ota_error_t error)  //Send OTA error messages to MQTT debug topic
                     {
                       if (error == OTA_AUTH_ERROR) MQTTclient.publish(debug, "<OTA> ERROR -> Auth failed", false);
                       else if (error == OTA_BEGIN_ERROR) MQTTclient.publish(debug, "<OTA> ERROR -> Begin failed", false);
                       else if (error == OTA_CONNECT_ERROR) MQTTclient.publish(debug, "<OTA> ERROR -> Connect failed", false);
                       else if (error == OTA_RECEIVE_ERROR) MQTTclient.publish(debug, "<OTA> ERROR -> Receive failed", false);
                       else if (error == OTA_END_ERROR) MQTTclient.publish(debug, "<OTA> ERROR -> End failed", false);
                     });

  ArduinoOTA.begin();
  ArduinoOTA.onEnd([]() {
#ifdef SERIAL_DEBUG
    Serial.println("\nEnd");
#endif
    ESP.restart();
  });

  timeClient.begin();
  timeClient.setTimeOffset(3600);   //GMT+1 Heure d'hiver
  timeClient.update();

  printEspInfo();
  // softAPdisconnect() seul laissait le mode AP+STA actif (SSID ESP_xxxxxx visible) : on repasse en station seule
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
#if defined(WEB_SERVER) || defined (AS_WEB_SERVER)
  loadWebServer ();
#endif
  refreshWifiCache();
  setupMHI();
}
