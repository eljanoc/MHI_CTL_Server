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

#include "params_logic.h"
#include "app_state.h"

extern mhi::Config config;  // Définie dans setup.cpp
extern mhi::WebValues web;
extern mhi::NetStats net;
extern mhi::LoopState loopState;
extern mhi::AppControl app;  // Définies dans loop.cpp

// Traces de debug : écrivent sur Serial (SERIAL_DEBUG) et/ou Telnet (TELNET_DEBUG), sinon rien
#ifdef SERIAL_DEBUG
#define DBG_SERIAL(call) Serial.call
#else
#define DBG_SERIAL(call)
#endif
#ifdef TELNET_DEBUG
#define DBG_TELNET(call) Debug.call
#else
#define DBG_TELNET(call)
#endif
#define DBG_PRINT(...) do { DBG_SERIAL(print(__VA_ARGS__)); DBG_TELNET(print(__VA_ARGS__)); } while (0)
#define DBG_PRINTLN(...) do { DBG_SERIAL(println(__VA_ARGS__)); DBG_TELNET(println(__VA_ARGS__)); } while (0)
#define DBG_PRINTF(...) do { DBG_SERIAL(printf(__VA_ARGS__)); DBG_TELNET(printf(__VA_ARGS__)); } while (0)
#define DBG_PRINTF_P(...) do { DBG_SERIAL(printf_P(__VA_ARGS__)); DBG_TELNET(printf_P(__VA_ARGS__)); } while (0)

// Copie bornée vers un tableau de char (tronque au lieu de déborder)
#define COPY_STR(dst, src) strlcpy((dst), (src), sizeof(dst))
