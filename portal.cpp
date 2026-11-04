#include "portal.h"

#if defined(WEB_SERVER) || defined (AS_WEB_SERVER)
void setApplicationWebServerEnabled(bool enabled);
#endif

//Flag for saving data
bool shouldSaveConfig = false;

static bool wifiRecoveryPortalActive = false;

WifiPortal& getWifiPortal() {
  static WifiPortal portal;
  return portal;
}

//Callback notifying us of the need to save WiFiManager config to FS
void saveConfigCallback() {
  DBG_PRINTLN("Should save config");
  shouldSaveConfig = true;
}

void applyWifiPortalParameters() {
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
