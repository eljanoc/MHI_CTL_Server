# MHI-AC-CtrlServer (V4.31-trace)

[English](README.md) | **Français**

Firmware **ESP8266 / ESP32** qui pilote une climatisation **Mitsubishi Heavy Industries (MHI)** en se branchant sur le bus SPI de son unité intérieure. Il la rend contrôlable et observable via **MQTT** (domotique, par exemple Jeedom) et via une **interface web** embarquée.

Il peut aussi fonctionner **en autonome**, sans Jeedom ni broker MQTT : l'ESP seul sert l'interface web pour piloter et surveiller la clim (depuis la V4.26, la page reste réactive même quand aucun broker n'est joignable).

Ce projet est une variante serveur de [MHI-AC-Ctrl](https://github.com/absalom-muc/MHI-AC-Ctrl) enrichie d'un serveur web, d'une configuration en ligne et d'un système de trace des trames.

## Buts du projet
- Piloter la climatisation depuis une box domotique sans passer par la télécommande infrarouge.
- Remonter en continu l'état de la clim et les données de fonctionnement des unités intérieure et extérieure.
- Offrir une configuration simple, sans recompiler : WiFi, MQTT et paramètres depuis une page web.
- Aider au diagnostic et à la compréhension du protocole grâce à la capture des trames brutes.

## Fonctionnalités

### Commandes (MQTT, topics `set/...`)
- Marche / arrêt (`Power`)
- Mode (`Mode`) : Auto, Cool, Dry, Fan, Heat, Stop
- Température de consigne (`Tsetpoint`)
- Vitesse du ventilateur (`Fan`)
- Ailettes (`Vanes`, y compris Swing)
- Température de la pièce fournie par une sonde externe (`Troom`)
- Demande de données d'erreur / fonctionnement (`ErrOpData`) et `reset`

### Mesures et états publiés (MQTT)
- État : `Power`, `Mode`, `Fan`, `Vanes`, `Troom`, `Tsetpoint`, `Errorcode`
- Unité intérieure : `RETURN-AIR`, `THI-R1/R2/R3`, `IU-FANSPEED`, `TOTAL-IU-RUN`
- Unité extérieure : `OUTDOOR`, `COMP`, `TD`, `THO-R1`, `CT`, `TDSH`, `PROTECTION-NO`, `OU-FANSPEED`, `DEFROST`, `TOTAL-COMP-RUN`, `OU-EEV1`, `KWH`
- Supervision du module : `connected`, `Version`, `RSSI`, `WIFI_LOST`, `MQTT_LOST`, `WIFI_BSSID`, `cmd_received`
- Sonde de température optionnelle DS18B20 (`Tds1820`)

### Interface web embarquée
- Page principale : état et commandes de la clim.
- Page de paramètres : configuration WiFi / MQTT, sauvegardée dans le système de fichiers (LittleFS), protégée par identifiant et mot de passe.
- Actions : redémarrage, réinitialisation, ouverture de WiFiManager.

### Connectivité et maintenance
- Configuration WiFi par portail captif (WiFiManager).
- Mise à jour du firmware à distance (OTA).
- Heure par NTP.
- Débogage à distance par Telnet (RemoteDebug).

### Système de trace des trames
Capture les trames SPI échangées avec l'unité intérieure (20 octets) : trames MOSI et MISO de chaque échange.
- Activation par la case « Détection des trames » de la page web, ou par MQTT : `traceon` / `traceoff`.
- Affichage des trames en direct dans la page web (historique de 64 trames, rafraîchi chaque seconde).
- Publication des trames MOSI en hexadécimal sur le topic de debug `<debug>/trame`.
- Compteur de trames perdues, indiqué à l'arrêt de la trace.

## Matériel
- ESP8266 (testé sur Wemos D1 mini) ou ESP32.
- Raccordement au connecteur SPI de l'unité intérieure MHI (voir le projet MHI-AC-Ctrl pour le détail du connecteur de la clim).

| Signal | ESP8266 (D1 mini) | ESP32 |
|---|---|---|
| SCK | GPIO 14 (D5) | GPIO 18 |
| MOSI | GPIO 13 (D7) | GPIO 23 |
| MISO | GPIO 12 (D6) | GPIO 19 |

Les broches sont définies dans `MHI-AC-Ctrl-core.h`. La sonde de température DS18B20 optionnelle se branche sur le GPIO 4 (D2 sur D1 mini), défini dans `support.h`.

## Premier démarrage
1. Sur une carte vierge, l'ESP ouvre un point d'accès WiFi nommé `Clim<nom>` (le nom par défaut est `RoomName`, donc `ClimRoomName`), avec le mot de passe `climatiseur`.
2. Connecte-toi à ce point d'accès : le portail de configuration (WiFiManager) s'ouvre.
3. Renseigne le réseau WiFi, l'adresse du serveur MQTT et ses identifiants, ainsi que le nom de la pièce et de l'appareil.
4. L'ESP redémarre, se connecte au WiFi et publie ses données sur MQTT. L'interface web est alors accessible à son adresse IP.

Les réglages sont conservés dans la mémoire de l'ESP (LittleFS), pas dans le dépôt Git.

## Sécurité
Des valeurs par défaut sont codées en dur dans le firmware. Change-les avant tout usage hors d'un réseau local de confiance :
- **Page web** : identifiant `John` et mot de passe `Pass` (`webserver.cpp`, `aswebserver.cpp`).
- **Point d'accès WiFi de configuration et mise à jour OTA** : mot de passe `climatiseur` (`setup.cpp`, variable `configPW`). Le même mot de passe sert à l'OTA.
- Les identifiants MQTT sont saisis dans le portail de configuration et ne figurent pas dans le dépôt.
- Si le broker MQTT est indisponible, l'interface web est servie avant les tentatives de reconnexion, espacées de 5 secondes. Sur ESP8266, le délai de connexion TCP est limité à 250 ms ; le délai d'attente de réponse MQTT est d'une seconde.

N'expose pas l'ESP directement sur internet.

## Télécharger le firmware (Releases)

Des binaires prêts à flasher sont compilés par GitHub Actions et joints à chaque [release](https://github.com/eljanoc/MHI_CTL_Server/releases) (tags `v*`) :

- `MHI-esp8266-<tag>.bin` : firmware ESP8266 (OTA ou premier flash).
- `MHI-esp32-<tag>.bin` : application ESP32 seule, pour les mises à jour (OTA) d'une carte déjà configurée.
- `MHI-esp32-<tag>-merged.bin` : image flash complète ESP32 (bootloader + partitions + application, 4 Mo). À utiliser pour un ESP32 neuf ou vierge, ou pour réparer une flash corrompue : `esptool.py --chip esp32 write_flash 0x0 MHI-esp32-<tag>-merged.bin`

## Compilation et téléversement
- Outil : Arduino IDE / `arduino-cli`.
- Carte ESP8266 : `esp8266:esp8266:d1_mini`.
- Bibliothèques : ArduinoJson (7.4.3), PubSubClient (2.8), OneWire (2.3.8), DallasTemperature (4.0.6), plus trois bibliothèques modifiées fournies dans `libs/` : WiFiManager 2.0.17 (`libs/WiFiManager`), RemoteDebug 3.0.5 (`libs/RemoteDebug`) et NTPClient 3.2.1 (`libs/NTPClient`, `byte` remplacé par `uint8_t` dans `NTPClient.h`, ambiguïté avec `std::byte` sous `using namespace std`).
- Cartes : esp32 3.1.3 (carte `esp32:esp32:esp32doit-devkit-v1`) et esp8266 2.7.4 (`esp8266:esp8266:d1_mini`). La carte générique `esp32` ne compile pas (`LED_BUILTIN` absent).
- Le dossier du projet doit s'appeler `MHI-AC-CtrlServer` (comme le fichier `.ino`) : `git clone https://github.com/eljanoc/MHI_CTL_Server.git MHI-AC-CtrlServer`.

### Correction de WiFiManager pour ESP8266 uniquement
Cette correction concerne uniquement les builds ESP8266. Avec WiFiManager 2.0.17 officielle, `getMenuOut()` compare directement `_menutokens[menuId]` (chaîne en mémoire flash) à une chaîne, ce qui provoquait un redémarrage en boucle environ une minute après le démarrage de l'ESP8266. Il faut convertir via `__FlashStringHelper`. Dans `WiFiManager.cpp`, fonction `WiFiManager::getMenuOut()` (vers la ligne 1442), remplacer :
```cpp
if((String)_menutokens[menuId] == "param" && _paramsCount == 0) continue;
if((String)_menutokens[menuId] == "custom" && _customMenuHTML!=NULL){
```
par :
```cpp
if((String)(__FlashStringHelper *)(_menutokens[menuId]) == "param" && _paramsCount == 0) continue;
if((String)(__FlashStringHelper *)(_menutokens[menuId]) == "custom" && _customMenuHTML!=NULL){
```
La correction est déjà appliquée dans `libs/WiFiManager` du projet et dans la bibliothèque installée (`Documents\Arduino\libraries\WiFiManager`). Si WiFiManager est réinstallée ou mise à jour, il faut la refaire.
Cette modification n'est pas nécessaire pour l'ESP32 : elle répond au problème observé sur ESP8266 uniquement.

### Correction de RemoteDebug pour ESP32
La RemoteDebug 3.0.5 officielle ne compile pas avec le core ESP32 3.x : `src/utility/WebSockets.cpp` inclut `<hwcrypto/sha.h>`, qui n'existe plus. Le seul changement est cette ligne, remplacée par :
```cpp
#include <esp32/sha.h> // <hwcrypto/sha.h>
```
La correction est déjà appliquée dans la copie `libs/RemoteDebug` du projet (licence MIT conservée). Si tu utilises la version du registre, applique-la toi-même.

### Mise à jour OTA
La mise à jour OTA est possible via la tâche VS Code « OTA ESP8266 141 » (voir `.vscode/tasks.json`). Elle contient l'adresse IP de l'appareil de l'auteur et un mot de passe masqué (`******`) : adapte les deux à ton installation.

## Organisation du code
| Fichier | Rôle |
|---|---|
| `MHI-AC-CtrlServer.ino` | Programme principal, callbacks MQTT, publication |
| `MHI-AC-Ctrl-core.cpp/.h` | Dialogue SPI avec la clim, capture des trames |
| `MHI-AC-Ctrl.h` | Noms des topics et textes MQTT |
| `setup.cpp`, `loop.cpp` | Initialisation et boucle (WiFi, MQTT, commandes) |
| `webserver.cpp`, `aswebserver.cpp` | Serveur web (synchrone / asynchrone) |
| `mainHTML.h`, `paramHTML.h` | Pages web embarquées |
| `support.cpp/.h`, `include.h` | Fonctions utilitaires et options de compilation |

## Versions
Les versions sont gérées avec des tags Git, par exemple `v4.25-trace`.

### V4.31
Nouvelle case « MQTT actif » dans la page de configuration (clé `mqtt_enabled` du `config.json`, activée par défaut). Décochée, le module fonctionne en autonome sans aucune tentative MQTT (affiché `off`). Un serveur vide ou `0.0.0.0` désactive toujours MQTT. `/param_data` et `/NewParam` ont un 10e champ (après `Thing`) ; la liste Wi-Fi commence maintenant à l'index 10.

### V4.30
Correction de débordements de tampon : la liste des réseaux Wi-Fi (`/param_data`) débordait d'un tampon de 200 octets dès que plusieurs réseaux étaient à portée, la route de réglage (`/SetValue`) utilisait un tampon de 30 octets pour le topic, et `/NewParam` plantait sur une requête mal formée. Toutes les copies de chaînes sont maintenant bornées, et `/NewParam` répond 400 si les données sont invalides.

### V4.29
Nettoyage : suppression d'une trace de debug exécutée à chaque requête web, et protection des traces de consigne par `TELNET_DEBUG`.

### V4.28
Corrigé : le champ « Last reboot » affichait 00/01/1900 0h00 sans MQTT, car la date n'était enregistrée qu'à la première connexion MQTT. Elle est maintenant fixée dès que l'heure NTP est connue.

### V4.27
Nouveau : mode autonome. Un serveur MQTT vide ou `0.0.0.0` désactive MQTT : aucune tentative de connexion, et plus de redémarrage automatique après 30 échecs (avec un broker injoignable, l'ESP redémarrait environ toutes les 2,5 minutes).

### V4.26
Corrigé : la page web était extrêmement lente quand le broker MQTT était injoignable. La boucle principale retentait la connexion MQTT à chaque passage (blocage sur la connexion TCP et un `delay` d'1 s) et le serveur web n'était interrogé que toutes les 100 ms après.
- Le serveur web est maintenant interrogé à chaque passage de boucle, avant la reconnexion MQTT.
- Les tentatives de reconnexion MQTT sont espacées de 5 secondes, sans `delay` bloquant en cas d'échec.
- ESP8266 : délai de connexion TCP limité à 250 ms ; délai MQTT 1 s ; Nagle désactivé pour le serveur web.

## Remerciements
Un grand merci à **absalom-muc** pour [MHI-AC-Ctrl](https://github.com/absalom-muc/MHI-AC-Ctrl) : son travail de décodage du protocole SPI des climatisations Mitsubishi Heavy, et sa publication en open source, sont à l'origine de ce projet. Merci aussi à **tzapu** pour WiFiManager et aux auteurs des bibliothèques utilisées.

## Crédits et licence
- Ce projet est dérivé de [MHI-AC-Ctrl](https://github.com/absalom-muc/MHI-AC-Ctrl) d'absalom-muc, publié sous licence MIT. Le décodage du protocole SPI MHI lui doit beaucoup.
- [WiFiManager](https://github.com/tzapu/WiFiManager) de tzapu (licence MIT), copié dans `libs/WiFiManager` avec la correction ESP8266 décrite plus haut.
- [RemoteDebug](https://github.com/JoaoLopesF/RemoteDebug) de Joao Lopes (licence MIT), copié dans `libs/RemoteDebug` avec la correction ESP32 décrite plus haut.
- Bibliothèques tierces : ArduinoJson, PubSubClient, NTPClient, OneWire, DallasTemperature.
- Ce dépôt est publié sous licence MIT (voir [LICENSE](LICENSE)), qui conserve la mention de copyright du projet d'origine.
