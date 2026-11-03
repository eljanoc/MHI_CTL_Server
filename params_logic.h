// Logique pure des paramètres (aucune dépendance Arduino) : utilisée par le firmware
// et testée sur PC par test/test_params.cpp.
#pragma once
#include <stddef.h>
#include <stdio.h>
#include <string.h>

namespace mhi {

// Copie bornée : tronque au lieu de déborder, termine toujours par '\0'
inline void copyStr(char *dst, size_t size, const char *src) {
  if (size == 0)
    return;
  size_t i = 0;
  for (; i + 1 < size && src[i]; i++)
    dst[i] = src[i];
  dst[i] = 0;
}
#define MHI_COPY(dst, src) mhi::copyStr((dst), sizeof(dst), (src))

// Taille maximale (terminateur compris) d'une requête /NewParam
const size_t NEWPARAM_MAX_LEN = 256;
// Séparateurs '?' obligatoires : 10 champs de configuration
const int NEWPARAM_MIN_SEPARATORS = 10;

// Tailles identiques aux variables globales de setup.cpp
struct NewParams {
  char staticIP[16];
  char masqueIP[16];
  char gatewayIP[16];
  char dnsIP[16];
  char mqtt_server[16];
  char mqtt_port[9];
  char wifiTimeout[3];
  char Room[20];
  char Thing[20];
  char mqtt_enabled[2];
  char PreloadSSID[20];
  char PreloadPSW[20];
  char PreloadFlag[3];
};

// Isole le champ courant (jusqu'au prochain '?') et avance. nullptr s'il n'y a plus de '?'.
inline char *nextField(char *&cursor) {
  char *sep = strchr(cursor, '?');
  if (!sep)
    return nullptr;
  *sep = 0;
  char *field = cursor;
  cursor = sep + 1;
  return field;
}

// Format : staticIP?masque?gateway?dns?mqttIP?mqttPort?wifiTimeout?room?thing?mqttActif?[ssid?[mdp?]]
// Retourne false (-> HTTP 400) si la chaîne est trop longue ou incomplète.
inline bool parseNewParams(const char *in, NewParams &out) {
  memset(&out, 0, sizeof(out));
  if (!in || strlen(in) >= NEWPARAM_MAX_LEN)
    return false;
  char param[NEWPARAM_MAX_LEN];
  strcpy(param, in);
  int nbSep = 0;
  for (const char *c = param; *c; c++)
    if (*c == '?')
      nbSep++;
  if (nbSep < NEWPARAM_MIN_SEPARATORS)
    return false;

  char *cur = param;
  MHI_COPY(out.staticIP, nextField(cur));
  MHI_COPY(out.masqueIP, nextField(cur));
  MHI_COPY(out.gatewayIP, nextField(cur));
  MHI_COPY(out.dnsIP, nextField(cur));
  MHI_COPY(out.mqtt_server, nextField(cur));
  MHI_COPY(out.mqtt_port, nextField(cur));
  MHI_COPY(out.wifiTimeout, nextField(cur));
  MHI_COPY(out.Room, nextField(cur));
  MHI_COPY(out.Thing, nextField(cur));
  const char *enabled = nextField(cur);
  if (!enabled)
    return false;
  MHI_COPY(out.mqtt_enabled, (enabled[0] == '0') ? "0" : "1");

  const char *ssid = nextField(cur);
  if (ssid && *ssid) {
    MHI_COPY(out.PreloadSSID, ssid);
    MHI_COPY(out.PreloadFlag, "o");
    const char *psw = nextField(cur);
    if (psw)
      MHI_COPY(out.PreloadPSW, psw);
  }
  return true;
}

// MQTT désactivé : case décochée, serveur vide ou 0.0.0.0
inline bool mqttDisabled(const char *enabled, const char *server) {
  return enabled[0] == '0' || server[0] == '\0' || strcmp(server, "0.0.0.0") == 0;
}

// Début de /param_data : les 10 champs, mqtt_enabled en 10e position
inline void buildParamsBase(char *out, size_t size, const char *staticIP, const char *masqueIP,
                            const char *gatewayIP, const char *dnsIP, const char *mqtt_server,
                            const char *mqtt_port, const char *wifiTimeout, const char *Room,
                            const char *Thing, const char *mqtt_enabled) {
  snprintf(out, size, "%s?%s?%s?%s?%s?%s?%s?%s?%s?%s", staticIP, masqueIP, gatewayIP, dnsIP,
           mqtt_server, mqtt_port, wifiTimeout, Room, Thing, mqtt_enabled);
}

// Ajoute "?ssid?mdp?rssi" (mdp vide sauf pour le réseau enregistré).
// false si ça ne tient pas dans le tampon.
inline bool appendNetwork(char *out, size_t size, const char *netSsid, const char *savedSsid,
                          const char *savedPass, int rssi) {
  size_t len = strlen(out);
  char signal[12];
  snprintf(signal, sizeof(signal), "%d", rssi);
  const char *password = strcmp(netSsid, savedSsid) == 0 ? savedPass : "";
  size_t required = len + strlen(netSsid) + strlen(password) + strlen(signal) + 4;
  if (required > size)
    return false;
  snprintf(&out[len], size - len, "?%s?%s?%s", netSsid, password, signal);
  return true;
}

}  // namespace mhi
