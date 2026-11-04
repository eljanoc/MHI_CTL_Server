#include "include.h"
#include "portal.h"

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
#endif

void MQTT_subscribe_callback(const char *topic, byte *payload, unsigned int length);

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
  loadConfig();

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
  DBG_PRINTF("staticIP:%s, masqueIP:%s, gatewayIP:%s, dnsIP:%s\n", config.staticIP, config.masqueIP, config.gatewayIP, config.dnsIP);

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
    DBG_PRINTLN("WiFi unavailable; retrying in background");
  }
  saveJson(false);
  DBG_SERIAL(printf("after wifiManager.autoConnect(%s,%s), connected=%d\n",configSSID, configPW, wifiConnected));
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
  DBG_PRINTLN("Constructed topics:MQTT_SET_PREFIX,MQTT_OP_PREFIX,MQTT_ERR_OP_PREFIX\n");


  //Save the custom parameters to FS
  if (shouldSaveConfig){
    saveJson(false);
    ESP.restart();
  }

  DBG_PRINTF("\nLocal IP adres:%s\n",WiFi.localIP().toString().c_str());

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
