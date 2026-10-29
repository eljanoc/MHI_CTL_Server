//#define SERIAL_DEBUG
//#define AS_WEB_SERVER
#define WEB_SERVER
#define TELNET_DEBUG
#define JSON_6

#ifdef ESP32
//#define MULTICORE
#endif

#ifdef ESP32  //ESP32
#define LED_HIGH 1
#define LED_LOW 0
#include <WiFi.h>
#ifdef MULTICORE
#define WDT_TIMEOUT 120
#include <esp_task_wdt.h>  //Pour un Watchdog
#endif
#else  //ESP8266
#define LED_HIGH 0
#define LED_LOW 1
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif  //ALL
#include <LittleFS.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>   //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>  //https://github.com/knolleary/pubsubclient
#ifdef TELNET_DEBUG
#include <RemoteDebug.h>
#endif

#define GO_REINIT 1
#define GO_RESTART 2
#define GO_WIFIMANAGER 3
#define GO_CHANGE 4
