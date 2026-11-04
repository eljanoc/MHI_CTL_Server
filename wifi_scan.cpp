#include "include.h"

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
