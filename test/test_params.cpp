// Tests sur PC de la logique pure (params_logic.h).
// Compilation : g++ -std=c++11 -Wall -Wextra -I.. test_params.cpp -o test_params && ./test_params
#include <stdio.h>
#include <string.h>

#include "params_logic.h"

static int checks = 0;
static int failures = 0;

#define CHECK(cond)                                                \
  do {                                                             \
    checks++;                                                      \
    if (!(cond)) {                                                 \
      failures++;                                                  \
      printf("ECHEC ligne %d : %s\n", __LINE__, #cond);            \
    }                                                              \
  } while (0)

#define CHECK_STR(actual, expected)                                          \
  do {                                                                       \
    checks++;                                                                \
    if (strcmp((actual), (expected)) != 0) {                                 \
      failures++;                                                            \
      printf("ECHEC ligne %d : '%s' attendu '%s'\n", __LINE__, (actual), (expected)); \
    }                                                                        \
  } while (0)

static const char *FULL =
    "192.168.0.141?255.255.255.0?192.168.0.254?192.168.0.254?192.168.0.42?1883?5?Salon?Clim?1?maBox?monMdp?";

static void test_parse_champs() {
  mhi::NewParams p;
  CHECK(mhi::parseNewParams(FULL, p));
  CHECK_STR(p.staticIP, "192.168.0.141");
  CHECK_STR(p.masqueIP, "255.255.255.0");
  CHECK_STR(p.gatewayIP, "192.168.0.254");
  CHECK_STR(p.dnsIP, "192.168.0.254");
  CHECK_STR(p.mqtt_server, "192.168.0.42");
  CHECK_STR(p.mqtt_port, "1883");
  CHECK_STR(p.wifiTimeout, "5");
  CHECK_STR(p.Room, "Salon");
  CHECK_STR(p.Thing, "Clim");
  CHECK_STR(p.mqtt_enabled, "1");
  CHECK_STR(p.PreloadSSID, "maBox");
  CHECK_STR(p.PreloadPSW, "monMdp");
  CHECK_STR(p.PreloadFlag, "o");
}

static void test_mqtt_enabled() {
  mhi::NewParams p;
  CHECK(mhi::parseNewParams("0.0.0.0?0.0.0.0?0.0.0.0?0.0.0.0?1.2.3.4?1883?5?R?T?0?", p));
  CHECK_STR(p.mqtt_enabled, "0");
  CHECK(mhi::parseNewParams("0.0.0.0?0.0.0.0?0.0.0.0?0.0.0.0?1.2.3.4?1883?5?R?T?1?", p));
  CHECK_STR(p.mqtt_enabled, "1");
  // toute valeur autre que '0' active MQTT
  CHECK(mhi::parseNewParams("0.0.0.0?0.0.0.0?0.0.0.0?0.0.0.0?1.2.3.4?1883?5?R?T?x?", p));
  CHECK_STR(p.mqtt_enabled, "1");
}

static void test_sans_wifi() {
  mhi::NewParams p;
  // 10 champs sans SSID : accepté, aucune config Wi-Fi préchargée
  CHECK(mhi::parseNewParams("0.0.0.0?0.0.0.0?0.0.0.0?0.0.0.0?1.2.3.4?1883?5?R?T?1?", p));
  CHECK_STR(p.PreloadSSID, "");
  CHECK_STR(p.PreloadPSW, "");
  CHECK_STR(p.PreloadFlag, "");
  // SSID vide et mot de passe vide : pas de Wi-Fi préchargé
  CHECK(mhi::parseNewParams("0.0.0.0?0.0.0.0?0.0.0.0?0.0.0.0?1.2.3.4?1883?5?R?T?1???", p));
  CHECK_STR(p.PreloadSSID, "");
  CHECK_STR(p.PreloadFlag, "");
  // SSID sans mot de passe
  CHECK(mhi::parseNewParams("0.0.0.0?0.0.0.0?0.0.0.0?0.0.0.0?1.2.3.4?1883?5?R?T?1?maBox??", p));
  CHECK_STR(p.PreloadSSID, "maBox");
  CHECK_STR(p.PreloadPSW, "");
}

static void test_refus_trop_court() {
  mhi::NewParams p;
  CHECK(!mhi::parseNewParams("", p));
  CHECK(!mhi::parseNewParams("abc", p));
  CHECK(!mhi::parseNewParams(nullptr, p));
  // 9 séparateurs seulement : le 10e champ (mqtt_enabled) manque
  CHECK(!mhi::parseNewParams("0.0.0.0?0.0.0.0?0.0.0.0?0.0.0.0?1.2.3.4?1883?5?R?T?", p));
  // ancien format (V4.30, 9 champs + ssid + mdp) : le SSID serait pris pour mqtt_enabled
  CHECK(mhi::parseNewParams("0.0.0.0?0.0.0.0?0.0.0.0?0.0.0.0?1.2.3.4?1883?5?R?T?maBox?monMdp?", p));
  CHECK_STR(p.PreloadSSID, "monMdp");
}

static void test_refus_trop_long() {
  mhi::NewParams p;
  char big[600];
  memset(big, 'a', sizeof(big) - 1);
  big[sizeof(big) - 1] = 0;
  CHECK(!mhi::parseNewParams(big, p));
  // limite exacte : 255 caractères acceptés, 256 refusés
  char s[300];
  const char *debut = "0.0.0.0?0.0.0.0?0.0.0.0?0.0.0.0?1.2.3.4?1883?5?R?T?1?";
  size_t n = strlen(debut);
  memcpy(s, debut, n);
  memset(s + n, 'z', 255 - n);
  s[255] = 0;
  CHECK(mhi::parseNewParams(s, p));
  s[255] = 'z';
  s[256] = 0;
  CHECK(!mhi::parseNewParams(s, p));
}

static void test_troncature_sans_debordement() {
  mhi::NewParams p;
  CHECK(mhi::parseNewParams(
      "1234567890123456789?0.0.0.0?0.0.0.0?0.0.0.0?1.2.3.4?123456789012?12345?"
      "RoomRoomRoomRoomRoomRoom?ThingThingThingThingThing?1?SsidSsidSsidSsidSsidSsid?PswPswPswPswPswPswPsw?",
      p));
  CHECK(strlen(p.staticIP) == sizeof(p.staticIP) - 1);
  CHECK(strlen(p.mqtt_port) == sizeof(p.mqtt_port) - 1);
  CHECK(strlen(p.wifiTimeout) == sizeof(p.wifiTimeout) - 1);
  CHECK(strlen(p.Room) == sizeof(p.Room) - 1);
  CHECK(strlen(p.Thing) == sizeof(p.Thing) - 1);
  CHECK(strlen(p.PreloadSSID) == sizeof(p.PreloadSSID) - 1);
  CHECK(strlen(p.PreloadPSW) == sizeof(p.PreloadPSW) - 1);
  CHECK_STR(p.staticIP, "123456789012345");
}

static void test_copyStr() {
  char d[4];
  mhi::copyStr(d, sizeof(d), "abcdef");
  CHECK_STR(d, "abc");
  mhi::copyStr(d, sizeof(d), "ab");
  CHECK_STR(d, "ab");
  mhi::copyStr(d, sizeof(d), "");
  CHECK_STR(d, "");
}

static void test_mqtt_disabled() {
  CHECK(mhi::mqttDisabled("0", "192.168.0.42"));
  CHECK(mhi::mqttDisabled("1", ""));
  CHECK(mhi::mqttDisabled("1", "0.0.0.0"));
  CHECK(mhi::mqttDisabled("0", "0.0.0.0"));
  CHECK(!mhi::mqttDisabled("1", "192.168.0.42"));
}

static int champs(const char *s) {
  int n = 1;
  for (; *s; s++)
    if (*s == '?')
      n++;
  return n;
}

static const char *champ(const char *s, int index, char *buf, size_t size) {
  int i = 0;
  const char *debut = s;
  for (const char *c = s;; c++) {
    if (*c == '?' || *c == 0) {
      if (i == index) {
        size_t n = (size_t)(c - debut);
        if (n >= size)
          n = size - 1;
        memcpy(buf, debut, n);
        buf[n] = 0;
        return buf;
      }
      if (*c == 0)
        break;
      i++;
      debut = c + 1;
    }
  }
  buf[0] = 0;
  return buf;
}

static mhi::Config makeConfig(const char *ip, const char *masque, const char *gw, const char *dns,
                              const char *srv, const char *port, const char *timeout,
                              const char *room, const char *thing, const char *enabled) {
  mhi::Config c;
  MHI_COPY(c.staticIP, ip);
  MHI_COPY(c.masqueIP, masque);
  MHI_COPY(c.gatewayIP, gw);
  MHI_COPY(c.dnsIP, dns);
  MHI_COPY(c.mqtt_server, srv);
  MHI_COPY(c.mqtt_port, port);
  MHI_COPY(c.wifiTimeout, timeout);
  MHI_COPY(c.Room, room);
  MHI_COPY(c.Thing, thing);
  MHI_COPY(c.mqtt_enabled, enabled);
  return c;
}

static void test_param_data_ordre() {
  char out[400];
  char b[64];
  mhi::buildParamsBase(out, sizeof(out),
                       makeConfig("ip", "masque", "gw", "dns", "srv", "port", "to", "room", "thing", "1"));
  CHECK(champs(out) == 10);
  CHECK_STR(champ(out, 0, b, sizeof(b)), "ip");
  CHECK_STR(champ(out, 4, b, sizeof(b)), "srv");
  CHECK_STR(champ(out, 5, b, sizeof(b)), "port");
  CHECK_STR(champ(out, 8, b, sizeof(b)), "thing");
  // mqtt_enabled est le 10e champ (index 9)
  CHECK_STR(champ(out, 9, b, sizeof(b)), "1");

  // la liste Wi-Fi commence à l'index 10 : triplets (ssid, mot de passe, signal)
  CHECK(mhi::appendNetwork(out, sizeof(out), "autre", "maBox", "monMdp", -61));
  CHECK(mhi::appendNetwork(out, sizeof(out), "maBox", "maBox", "monMdp", -42));
  CHECK(champs(out) == 16);
  CHECK_STR(champ(out, 10, b, sizeof(b)), "autre");
  CHECK_STR(champ(out, 11, b, sizeof(b)), "");
  CHECK_STR(champ(out, 12, b, sizeof(b)), "-61");
  CHECK_STR(champ(out, 13, b, sizeof(b)), "maBox");
  CHECK_STR(champ(out, 14, b, sizeof(b)), "monMdp");
  CHECK_STR(champ(out, 15, b, sizeof(b)), "-42");
}

static void test_param_data_tampon_plein() {
  char out[60];
  mhi::buildParamsBase(out, sizeof(out), makeConfig("ip", "m", "g", "d", "s", "p", "t", "r", "th", "1"));
  size_t avant = strlen(out);
  CHECK(!mhi::appendNetwork(out, sizeof(out), "un-reseau-wifi-avec-un-nom-bien-trop-long-pour-le-tampon",
                            "x", "y", -100));
  CHECK(strlen(out) == avant);
  CHECK(strlen(out) < sizeof(out));
}

static void test_aller_retour() {
  // Ce que /param_data envoie puis ce que la page renvoie à /NewParam : mêmes champs
  char out[400];
  mhi::buildParamsBase(out, sizeof(out),
                       makeConfig("1.1.1.1", "2.2.2.2", "3.3.3.3", "4.4.4.4", "5.5.5.5", "1883", "7",
                                  "Salon", "Clim", "0"));
  strcat(out, "?monSsid?monMdp?");
  mhi::NewParams p;
  CHECK(mhi::parseNewParams(out, p));
  CHECK_STR(p.staticIP, "1.1.1.1");
  CHECK_STR(p.mqtt_server, "5.5.5.5");
  CHECK_STR(p.Thing, "Clim");
  CHECK_STR(p.mqtt_enabled, "0");
  CHECK_STR(p.PreloadSSID, "monSsid");
  CHECK_STR(p.PreloadPSW, "monMdp");
}

static void test_config_defauts_et_application() {
  mhi::Config c;
  CHECK_STR(c.staticIP, "0.0.0.0");
  CHECK_STR(c.mqtt_server, "192.168.0.42");
  CHECK_STR(c.mqtt_port, "1883");
  CHECK_STR(c.mqtt_enabled, "1");
  CHECK_STR(c.wifiTimeout, "2");
  CHECK_STR(c.Room, "Aircon");
  CHECK_STR(c.Thing, "RoomName");
  CHECK_STR(c.PreloadSSID, "");

  // applyNewParams ne touche pas aux identifiants MQTT
  MHI_COPY(c.mqtt_user, "user");
  MHI_COPY(c.mqtt_pass, "pass");
  mhi::NewParams p;
  CHECK(mhi::parseNewParams(FULL, p));
  mhi::applyNewParams(c, p);
  CHECK_STR(c.staticIP, "192.168.0.141");
  CHECK_STR(c.Thing, "Clim");
  CHECK_STR(c.PreloadSSID, "maBox");
  CHECK_STR(c.PreloadPSW, "monMdp");
  CHECK_STR(c.PreloadFlag, "o");
  CHECK_STR(c.mqtt_user, "user");
  CHECK_STR(c.mqtt_pass, "pass");
}

int main() {
  test_config_defauts_et_application();
  test_parse_champs();
  test_mqtt_enabled();
  test_sans_wifi();
  test_refus_trop_court();
  test_refus_trop_long();
  test_troncature_sans_debordement();
  test_copyStr();
  test_mqtt_disabled();
  test_param_data_ordre();
  test_param_data_tampon_plein();
  test_aller_retour();
  printf("%d verifications, %d echec(s)\n", checks, failures);
  return failures ? 1 : 0;
}
