#pragma once
#include <time.h>

namespace mhi {

// Valeurs affichées sur la page principale, reçues par MQTT
struct WebValues {
  char puissance[6] = "";
  char ipSalon[40] = "";
  char ipChNO[40] = "";
  char ipCuisine[40] = "";
  char mqttState[4] = "";  // "yes", "no" ou "off"
};

// Compteurs de connexion
struct NetStats {
  int nbDeconnect = 0;
  int connectionCount = 0;
  int connectionFails = 0;  // échecs MQTT consécutifs avant redémarrage
};

// Chronométrage de la boucle principale
struct LoopState {
  int warmupCount = 0;
  unsigned long tWait = 60000;
  unsigned long lastUpdateMillis = 0;
  unsigned long lastMQTTMillis = 0;
  unsigned long lastPrintMillis = 0;
  unsigned long timeLoop = 0;
  unsigned long minLoop = 100000;
  unsigned long maxLoop = 0;
  unsigned long moyLoop = 0;
  unsigned long nbLoop = 0;
};

// Commandes et état général
struct AppControl {
  bool debugit = false;   // infos SPI et erreurs publiées sur le topic debug
  bool modeInfo = false;
  int stateFlag = 0;      // action demandée par le serveur web (GO_*)
  struct tm tReboot = {};
};

}  // namespace mhi
