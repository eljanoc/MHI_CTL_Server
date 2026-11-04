#include "include.h"
#include "params_logic.h"
#include <WiFiManager.h>   //https://github.com/tzapu/WiFiManager

//Variables below are initial values that can be changed at any time from the WiFiManager configuration portal and will be stored in flash memory. If there are different values in config.json, they are overwritten.
char staticIP[16] = "0.0.0.0";
char masqueIP[16] = "0.0.0.0";
char gatewayIP[16] = "0.0.0.0";
char dnsIP[16] = "0.0.0.0";
char mqtt_server[16] = "192.168.0.42";
char mqtt_port[9] = "1883";
char mqtt_enabled[2] = "1";  //"1" = MQTT actif, "0" = désactivé (mode autonome)
char mqtt_user[20] = "";
char mqtt_pass[20] = "";
char wifiTimeout[3] = "5";  //Timeout in minutes (max. 99) before WiFi configuration portal is turned off and the ESP tries to connect again to the previously configured AP (if any)
char Room[20] = "Aircon";
char Thing[20] = "RoomName";
char PreloadSSID[20]="";
char PreloadPSW[20]="";
char PreloadFlag[3]="";
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
extern void WiFiDisconnectAndRestart();
extern void setupMHI();
extern void setupTime();

//Flag for saving data
bool shouldSaveConfig = false;

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
  mhi::buildParamsBase(tmp, size, staticIP, masqueIP, gatewayIP, dnsIP, mqtt_server, mqtt_port,
                       wifiTimeout, Room, Thing, mqtt_enabled);
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


  json["staticIP"]= staticIP;
  json["masqueIP"]= masqueIP;
  json["gatewayIP"]= gatewayIP;
  json["dnsIP"]= dnsIP;
  json["mqtt_server"]= mqtt_server;
  json["mqtt_port"]= mqtt_port;
  json["mqtt_enabled"]= mqtt_enabled;
  json["mqtt_user"]= mqtt_user;
  json["mqtt_pass"]= mqtt_pass;
  json["wifiTimeout"]= wifiTimeout;
  json["Room"]= Room;
  json["Thing"]= Thing;
  json["PreloadSSID"]= PreloadSSID;
  json["PreloadPSW"]= PreloadPSW;
  if (f)
    json["PreloadFlag"]= PreloadFlag;
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
  COPY_STR(staticIP, np.staticIP);
  COPY_STR(masqueIP, np.masqueIP);
  COPY_STR(gatewayIP, np.gatewayIP);
  COPY_STR(dnsIP, np.dnsIP);
  COPY_STR(mqtt_server, np.mqtt_server);
  COPY_STR(mqtt_port, np.mqtt_port);
  COPY_STR(wifiTimeout, np.wifiTimeout);
  COPY_STR(Room, np.Room);
  COPY_STR(Thing, np.Thing);
  COPY_STR(mqtt_enabled, np.mqtt_enabled);
  COPY_STR(PreloadSSID, np.PreloadSSID);
  COPY_STR(PreloadPSW, np.PreloadPSW);
  COPY_STR(PreloadFlag, np.PreloadFlag);

#ifdef SERIAL_DEBUG
Serial.printf("staticIP:%s\n",staticIP);
Serial.printf("masqueIP:%s\n",masqueIP);
Serial.printf("gatewayIP:%s\n",gatewayIP);
Serial.printf("dnsIP:%s\n",dnsIP);
Serial.printf("mqtt_server:%s\n",mqtt_server);
Serial.printf("mqtt_port:%s\n",mqtt_port);
Serial.printf("wifiTimeout:%s\n",wifiTimeout);
Serial.printf("Room:%s\n",Room);
Serial.printf("Thing:%s\n",Thing);
Serial.printf("PreloadSSID:%s\n",PreloadSSID);
Serial.printf("PreloadPSW:%s\n",PreloadPSW);
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
            COPY_STR(staticIP, json["staticIP"] | "");
            COPY_STR(masqueIP, json["masqueIP"] | "");
            COPY_STR(gatewayIP, json["gatewayIP"] | "");
            COPY_STR(dnsIP, json["dnsIP"] | "");
            COPY_STR(mqtt_server, json["mqtt_server"] | "");
            COPY_STR(mqtt_port, json["mqtt_port"] | "");
            // Absent des anciens config.json : MQTT actif par défaut
            COPY_STR(mqtt_enabled, (json["mqtt_enabled"] | "1")[0] == '0' ? "0" : "1");
            COPY_STR(mqtt_user, json["mqtt_user"] | "");
            COPY_STR(mqtt_pass, json["mqtt_pass"] | "");
            COPY_STR(wifiTimeout, json["wifiTimeout"] | "");
            COPY_STR(Room, json["Room"] | "");
            COPY_STR(Thing, json["Thing"] | "");
            COPY_STR(PreloadSSID, json["PreloadSSID"] | "");
            COPY_STR(PreloadPSW, json["PreloadPSW"] | "");
            COPY_STR(PreloadFlag, json["PreloadFlag"] | "");
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
  }

  //The extra parameters to be configured (can be either global or just in the setup)
  //After connecting, parameter.getValue() will get you the configured value
  //id/name placeholder/prompt default length
  WiFiManagerParameter custom_staticIP("staticIP", "Static IP", staticIP, 16);
  WiFiManagerParameter custom_masqueIP("masqueIP", "Masque IP", masqueIP, 16);
  WiFiManagerParameter custom_gatewayIP("gatewayIP", "Gateway IP", gatewayIP, 16);
  WiFiManagerParameter custom_dnsIP("dnsIP", "DNS IP", dnsIP, 16);

  WiFiManagerParameter custom_mqtt_server("webServer", "MQTT Server", mqtt_server, 16);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 9);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT Username", mqtt_user, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Password", mqtt_pass, 20);
  WiFiManagerParameter custom_wifiTimeout("timeout", "5", wifiTimeout, 3);
  WiFiManagerParameter custom_topic_Room("Room", "Room name", Room, 20);
  WiFiManagerParameter custom_topic_Thing("Thing", "Thing name", Thing, 20);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

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
  Serial.printf("staticIP:%s, masqueIP:%s, gatewayIP:%s, dnsIP:%s\n", staticIP, masqueIP, gatewayIP, dnsIP);
#endif
#ifdef TELNET_DEBUG
  Debug.printf("staticIP:%s, masqueIP:%s, gatewayIP:%s, dnsIP:%s\n", staticIP, masqueIP, gatewayIP, dnsIP);
#endif

  if (strcmp(staticIP, "0.0.0.0") && strcmp(masqueIP, "0.0.0.0") && strcmp(gatewayIP, "0.0.0.0") && strcmp(dnsIP, "0.0.0.0")) {
    IPAddress ip, mask, gate, dns;
    ip.fromString(staticIP);
    mask.fromString(masqueIP);
    gate.fromString(gatewayIP);
    dns.fromString(dnsIP);
    Serial.printf("call wifiManager.setSTAStaticIPConfig\n");
    wifiManager.setSTAStaticIPConfig(ip, gate, mask, dns);
  }
  //Add all parameters here
  wifiManager.addParameter(&custom_staticIP);
  wifiManager.addParameter(&custom_masqueIP);
  wifiManager.addParameter(&custom_gatewayIP);
  wifiManager.addParameter(&custom_dnsIP);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_wifiTimeout);
  wifiManager.addParameter(&custom_topic_Room);
  wifiManager.addParameter(&custom_topic_Thing);


  //Set minimum quality of signal so it ignores AP's under that quality
  //Defaults to 8%
  wifiManager.setMinimumSignalQuality(5);

  // Portail captif : niveau du signal en texte (dBm) au lieu des seules icônes
  wifiManager.setScanDispPerc(true);

  //Sets timeout until configuration portal gets turned off
  //and retries connecting to the preconfigured AP
  wifiManager.setConfigPortalTimeout(atoi(wifiTimeout) * 60);  //Convert minutes to seconds

  //Fetches ssid and pass and tries to connect
  //If it does not connect it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
  snprintf(configSSID, sizeof(configSSID), "Clim%s", Thing);
#ifdef SERIAL_DEBUG
  Serial.printf("wifiManager.autoConnect(%s,%s)\n",configSSID, configPW);
#endif


#if 1
  if (PreloadSSID[0] && PreloadFlag[0])
  {
#ifdef SERIAL_DEBUG
    Serial.printf("PreloadWiFi %s,%s\n",PreloadSSID, PreloadPSW);
#endif
    wifiManager.preloadWiFi(PreloadSSID, PreloadPSW);
  }
#ifdef SERIAL_DEBUG
  else
    Serial.printf("No PreloadWiFi\n");
#endif
#endif
  saveJson(false);
  if (!wifiManager.autoConnect(configSSID, configPW)) {
#ifdef SERIAL_DEBUG
    Serial.println("Failed to connect and hit timeout");
#endif
#ifdef TELNET_DEBUG
    Debug.println("Failed to connect and hit timeout");
#endif
    delay(3000);
    //Reset and try again, or maybe put it to deep sleep
#ifndef ESP32
    ESP.reset();
#else
    ESP.restart();
#endif
    delay(3000);
  }
  saveJson(false);
#ifdef SERIAL_DEBUG
  Serial.printf("after wifiManager.autoConnect(%s,%s) OK!!!!\n",configSSID, configPW);
#endif
#ifdef TELNET_DEBUG
  Debug.begin(configSSID);
#endif
  savedWiFiSSID = wifiManager.getWiFiSSID();
  savedWiFiPass = wifiManager.getWiFiPass();
  //If you get here you have connected to the WiFi
#ifdef SERIAL_DEBUG
  Serial.printf("Connected...yeey savedWiFiSSID:%s, configSSID:%s\n",savedWiFiSSID.c_str(), configSSID);
#endif

  //Read updated parameters
  COPY_STR(staticIP, custom_staticIP.getValue());
  COPY_STR(masqueIP, custom_masqueIP.getValue());
  COPY_STR(gatewayIP, custom_gatewayIP.getValue());
  COPY_STR(dnsIP, custom_dnsIP.getValue());

  COPY_STR(mqtt_server, custom_mqtt_server.getValue());
  COPY_STR(mqtt_port, custom_mqtt_port.getValue());
  COPY_STR(mqtt_user, custom_mqtt_user.getValue());
  COPY_STR(mqtt_pass, custom_mqtt_pass.getValue());
  COPY_STR(wifiTimeout, custom_wifiTimeout.getValue());
  COPY_STR(Room, custom_topic_Room.getValue());
  COPY_STR(Thing, custom_topic_Thing.getValue());


  //Construct topic names
  //Topic prefix = "Room/Thing/"
  snprintf(topic_prefix, sizeof(topic_prefix), "%.20s/%.20s/", Room, Thing);

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
    MQTTclient.setServer(mqtt_server, atoi(mqtt_port));
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


