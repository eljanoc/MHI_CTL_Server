# MHI-AC-CtrlServer (V4.34-trace)

Firmware ESP8266 / ESP32 pour piloter une climatisation Mitsubishi Heavy (SPI) via MQTT et interface web.
Dossier de travail unique : ce dossier. Les versions se gèrent avec Git (branches/tags), pas avec des copies de dossier.

## Système de trace des trames
- Capture : `MHI-AC-Ctrl-core.cpp` / `.h`
  - file `trace_queue` (16 trames, publication MQTT) et historique `trace_history` / `trace_history_miso` (64 trames MOSI + MISO de 20 octets).
  - API : `set_trace`, `pop_trace`, `get_trace_dropped`, `is_trace_enabled`, `copy_trace_history`.
- Publication : `MHI-AC-CtrlServer.ino` (`publishTraceFrames`, `setTrace`, `traceDropped`, `getTraceData`). Trames MOSI en hex sur `<debug>/trame`.
- MQTT : commandes `traceon` / `traceoff` dans `loop.cpp`.
- Web : `webserver.cpp` routes `POST /trace?enable=0|1` et `GET /trace_data` (JSON) ; UI dans `mainHTML.h` (case « Détection des trames », rafraîchissement 1 s).
- `maxFrames` dans le `.ino` doit rester égal à `TRACE_HISTORY_SIZE`.

## Compilation / téléversement
- Outil : `arduino-cli` d'Arduino IDE (`C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe`).
- ESP8266 : carte `esp8266:esp8266:d1_mini`. OTA via la tâche VS Code « OTA ESP8266 141 » (`.vscode\tasks.json`, IP 192.168.0.141, build dans `%TEMP%\MHI-ota-8266\build`).
- ESP32 : carte `esp32:esp32:esp32doit-devkit-v1` (la carte générique `esp32` échoue : `LED_BUILTIN` absent). Build dans `%TEMP%\MHI-ota-32\build` (core 3.3.12 ; la première compilation après un changement de core prend plus de 5 min, laisser tourner en arrière-plan). Téléversement USB sur COM6 (CH9102) : utiliser `UploadSpeed=115200` (921600 échoue).
- OTA ESP32 : `arduino-cli upload --protocol network` coupe la connexion (WinError 10053) ; utiliser `espota.exe` directement avec un port local fixe, via les tâches « OTA ESP32 143 » / « OTA ESP32 152 » : `-i <IP> -p 3232 -P 41234 -a climatiseur -t 20 -f ...\MHI-AC-CtrlServer.ino.bin`. Compiler avant de lancer la tâche.
- Binaires produits dans `bin\` (ignorés par Git) : `MHI8266-V4.34.bin`, `MHI32-V4.34.bin`, `MHI32-V4.34-merged.bin`.

## Historique / décisions
- WiFiManager 2.0.17 modifiée pour ESP8266 uniquement : correction dans `getMenuOut()` (cast `__FlashStringHelper`), évite un bouclage au démarrage environ une minute après le boot. Voir README. Garder la correction dans `libs/WiFiManager` et `Documents\Arduino\libraries\WiFiManager`; elle répond au problème observé sur ESP8266, pas à un problème ESP32.
- RemoteDebug 3.0.5 modifiée pour ESP32 (core 3.x) : `libs/RemoteDebug/src/utility/WebSockets.cpp` inclut `<esp32/sha.h>` au lieu de `<hwcrypto/sha.h>`. Copie dans `libs/RemoteDebug` et `Documents\Arduino\libraries\RemoteDebug`.
- CI GitHub (`.github/workflows/build.yml`) : compile ESP8266 + ESP32 à chaque push ; un tag `v*` publie une release avec les `.bin`. Piège : Linux est sensible à la casse des `#include` (`include.h`, pas `Include.h`).
- V4.26 : la boucle web est servie avant les reconnexions MQTT, espacées de 5 s ; sur ESP8266, l'attente de connexion TCP est limitée à 250 ms, et le délai de réponse MQTT à 1 s.
- Les anciennes copies (V4.20, V4.20-trace) sont remplacées par l'historique Git.
- Ajouter ici chaque décision importante et piège rencontré.

- NTPClient 3.2.1 modifiee : \yte\ -> \uint8_t\ dans NTPClient.h (ambiguite std::byte avec using namespace std). Copie dans libs/NTPClient, utilisee par la CI.
- V4.27 : serveur MQTT vide ou `0.0.0.0` = MQTT désactivé (mode autonome) : aucune tentative de connexion, donc plus de redémarrage automatique après 30 échecs (avec un broker injoignable, l'ESP redémarrait toutes les ~2,5 min).
- V4.28 : le champ « Last reboot » est fixé dès que l'heure NTP est connue (avant, il n'était renseigné qu'à la première connexion MQTT : 00/01/1900 en mode autonome).

- V4.29 : suppression de la trace `Debug.printf` appelée à chaque requête web dans `getWebValuesString`; les traces `tsetpoint` (uniquement au changement de consigne) sont protégées par `#ifdef TELNET_DEBUG`.
- V4.31 : case « MQTT actif » (`mqtt_enabled`, optionnel dans config.json). `/param_data` et `/NewParam` ont un 10e champ après `Thing` ; la liste Wi-Fi commence à l'index 10 (JS `paramHTML.h` et `newParams` à garder synchronisés). PIÈGE : un `NewParam` envoyé à la main avec un mot de passe Wi-Fi vide écrase le mot de passe et le module disparaît du réseau (sans point d'accès) ; il faut alors reflasher en USB avec des identifiants forcés temporairement dans `setup.cpp`. Toujours envoyer le vrai mot de passe (ne jamais le commiter).
- V4.32 : `/param_data` envoie la liste Wi-Fi en triplets `ssid?mdp?rssi` à partir de l'index 10 (`mhi::appendNetwork`, tampon de 4096 octets alloué dans `webserver.cpp`). La page paramètres (`paramHTML.h`) affiche un `<select id="ssid2">` à côté du champ SSID ; il faut garder JS et format synchronisés.
- V4.33 : les scans Wi-Fi sont asynchrones et leurs résultats sont mis en cache 30 s pour accélérer `/param_data`. La page principale affiche l'adresse du broker MQTT à la place de `yes` quand il est connecté ; `no` et `off` restent inchangés.
- V4.34 : la page paramètres compare les champs à leurs valeurs chargées et avertit en cas de départ avec des modifications non enregistrées. Elle ne désactive l'avertissement qu'après confirmation HTTP du serveur lors de l'enregistrement.
- Tests PC : logique pure dans `params_logic.h` (namespace `mhi`, utilisée par `setup.cpp` / `loop.cpp`), tests dans `test/test_params.cpp`, job `test` de la CI. Pas de g++ local : `python -m ziglang c++ -Wall -Wextra -Werror -I. test\test_params.cpp -o "$env:TEMP\test_params.exe"` (sans `-std=c++11`). Vérifié sur le 152 : `/NewParam` répond 400 (trop court/long/vide) et 200 (valide).- V4.30 : correction des débordements de tampon. Utiliser `COPY_STR(dst, src)` (`include.h`, strlcpy borné) et `snprintf` ; plus de `strcpy`/`sprintf` non bornés. `getWebValuesString` et `getParametersString` prennent maintenant la taille du tampon. `/NewParam` renvoie 400 si la chaîne est trop longue ou incomplète (avant : plantage).
## Pièges pratiques (à relire avant d'agir)
- OTA ESP8266 qui marche : `arduino-cli upload` échoue ("No Answer"). Appeler `espota.py` directement avec l'IP du PC (`-I`, ex. 192.168.0.88, à vérifier avec `Get-NetIPAddress`) :
  `python3.exe espota.py -i 192.168.0.141 -I <IP PC> -a climatiseur -f <build>\MHI-AC-CtrlServer.ino.bin`
  - python3 : `%LOCALAPPDATA%\Arduino15\packages\esp8266\tools\python3\3.7.2-post1\python3.exe`
  - espota.py : `%LOCALAPPDATA%\Arduino15\packages\esp8266\hardware\esp8266\2.7.4\tools\espota.py`
  - 192.168.0.141 est un ESP8266 : ne jamais lui flasher le `.bin` ESP32.
- Compilation locale : ajouter `--library .\libs\WiFiManager --library .\libs\RemoteDebug --library .\libs\NTPClient` et un `--build-path` explicite (sinon cache partagé entre cartes qui échoue).
- Ne JAMAIS réécrire `MHI-AC-CtrlServer.ino` avec `Set-Content`/`Get-Content` PowerShell : l'encodage est corrompu (BOM ajouté, « ° » abîmé). Utiliser l'outil d'édition.
- Version : changer `WEB_VERSION` (ligne 9 du `.ino`), les titres des deux README et ce fichier, et ajouter une section dans « Versions » des README.
- Git : pas d'identité globale, utiliser `-c user.name="jeanl" -c user.email="jeanl@localhost"` et le trailer `Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>`. Le message de commit multi-ligne passe par `-m` multiples ou un fichier (`-F -` ne marche pas en PowerShell). Le push affiche une erreur PowerShell inoffensive.
- Release : pousser un tag annoté `vX.Y-trace` après CI vert sur le commit ; le CI publie 3 `.bin` (esp8266, esp32, esp32-merged). Vérifier via l'API publique `api.github.com/repos/eljanoc/MHI_CTL_Server/actions/runs`.
- Ne pas committer `MESSAGE-ABSALOM.md` (brouillon local ; message déjà posté en issue absalom-muc/MHI-AC-Ctrl#225).
- Sans MQTT, la page doit rester réactive : tester en mettant `0.0.0.0` (MQTT désactivé) ou un broker injoignable.
- Valeurs par défaut en dur : web `John`/`Pass`, mot de passe AP/OTA `climatiseur`, broker `192.168.0.42`.