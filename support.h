#pragma once

#include "MHI-AC-Ctrl-core.h"
#include "MHI-AC-Ctrl.h"

#define VERSION "2.7R4"

#ifdef TELNET_DEBUG
extern RemoteDebug Debug;
#endif

#if 0  //old
#define OTA_HOSTNAME HOSTNAME                       // default for the OTA_HOSTNAME is the HOSTNAME
#define OTA_PASSWORD ""                             // Enter an OTA password if required
#endif

#define TEMP_MEASURE_PERIOD 0                       // period in seconds for temperature measurement with the external DS18x20 temperature sensor
                                                    // enter 0 if you don't use the DS18x20 
#define ONE_WIRE_BUS 4                              // D2, PIN for connecting temperature sensor DS18x20 DQ pin
#define ROOM_TEMP_DS18X20_OFFSET 0.0                // Temperature offset for DS18x20 sensor, can be positive or negative (examples: 0.0, -1.0, 1.5)

//#define ROOM_TEMP_DS18X20                           // use room temperature from DS18x20

#define ROOM_TEMP_MQTT_SET_TIMEOUT  40              // time in seconds, after this time w/o receiving a valid room temperature
                                                    // via MQTT fallback to IU temperature sensor value

//#define POWERON_WHEN_CHANGING_MODE true           // uncomment it to switch on the AC when the mode (heat, cool, dry etc.) is changed
                                                    // used e.g. for home assistant support

#define TROOM_FILTER_LIMIT 0.25                     // Defines from which Troom delta value a new Troom value is pubslised. Resolution 0.25°C.
                                                    // With a smaller resolution, Troom could toggle more. So deactivate the filter use 0.
//#define ENHANCED_RESOLUTION true                    // when using Tsetpoint with x.5 degrees, airco will use (x+1).0 setpoint
                                                    // uncomment this to compensatie (offset) Troom for this.
                                                    // this will simulate .x degrees resolution
//#define CONTINUE_WITHOUT_MQTT true                  // uncomment if communication with AC has to continue when MQTT or WiFi connection is disconnected.
                                                    // When Troom is supplied from external, it will fallback to AC internal Troom temperature sensor
                                                    // When ROOM_TEMP_DS18X20 is used, it will use room temperature from DS18x20



// *** The configuration ends here ***

#ifdef ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#define ENC_TYPE_NONE WIFI_AUTH_OPEN
#else
#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#endif
#include <PubSubClient.h>       // https://github.com/knolleary/pubsubclient
#include <ArduinoOTA.h>         // https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

#if TEMP_MEASURE_PERIOD > 0
#include <OneWire.h>            // https://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>  // https://github.com/milesburton/Arduino-Temperature-Control-Library
#endif

#ifdef ROOM_TEMP_DS18X20
#if (TEMP_MEASURE_PERIOD == 0)
#error "You have to use a value>0 for TEMP_MEASURE_PERIOD when you want to use DS18x20 as an external temperature sensor"
#endif
#endif

extern PubSubClient MQTTclient;

void MeasureFrequency();                                      // measures the frequency of the SPI pins
void initWiFi();                                              // basic WiFi initialization
void setupWiFi(int& WiFiStatus);                              // setup WIFi connection to AP
int MQTTreconnect();                                          // (re)connect to MQTT broker
void publish_cmd_ok();                                        // last MQTT cmd was o.k.
void publish_cmd_unknown();                                   // last MQTT cmd was unknown
void publish_cmd_invalidparameter();                          // a paramter of the last MQTT was wrong
void output_P(ACStatus status, PGM_P topic, PGM_P payload);   // publish via MQTT

void setupOTA();                                              // initialize and start OTA
void setup_ds18x20();                                         // setup the temperature measurement
byte getDs18x20Temperature(int temp_hysterese);               // read the temperature from the DS18x20 sensor

extern char topic_prefix[42];
extern char MQTT_SET_PREFIX[60];
extern char MQTT_OP_PREFIX[60];
extern char MQTT_ERR_OP_PREFIX[60];
 
