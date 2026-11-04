#pragma once
#include "include.h"
#include <WiFiManager.h>   //https://github.com/tzapu/WiFiManager

// Portail WiFiManager et ses paramètres (définis dans portal.cpp)
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

WifiPortal& getWifiPortal();
extern bool shouldSaveConfig;          // Positionné par le portail quand l'utilisateur enregistre
void saveConfigCallback();
void applyWifiPortalParameters();      // Recopie les champs du portail dans config
