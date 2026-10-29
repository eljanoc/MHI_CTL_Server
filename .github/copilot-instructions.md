# MHI-AC-CtrlServer (V4.25-trace)

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
- ESP32 : carte `esp32:esp32:esp32doit-devkit-v1` (la carte générique `esp32` échoue : `LED_BUILTIN` absent). Build dans `%TEMP%\MHI-trace-32\build`. Téléversement USB sur COM3 : utiliser `UploadSpeed=115200` (921600 échoue).
- Binaires produits dans `bin\` (ignorés par Git) : `MHI8266-V4.25.bin`, `MHI32-V4.25.bin`, `MHI32-V4.25-merged.bin`.

## Historique / décisions
- WiFiManager 2.0.17 modifiée pour ESP8266 uniquement : correction dans `getMenuOut()` (cast `__FlashStringHelper`), évite un bouclage au démarrage environ une minute après le boot. Voir README. Garder la correction dans `libs/WiFiManager` et `Documents\Arduino\libraries\WiFiManager`; elle répond au problème observé sur ESP8266, pas à un problème ESP32.
- RemoteDebug 3.0.5 modifiée pour ESP32 (core 3.x) : `libs/RemoteDebug/src/utility/WebSockets.cpp` inclut `<esp32/sha.h>` au lieu de `<hwcrypto/sha.h>`. Copie dans `libs/RemoteDebug` et `Documents\Arduino\libraries\RemoteDebug`.
- CI GitHub (`.github/workflows/build.yml`) : compile ESP8266 + ESP32 à chaque push ; un tag `v*` publie une release avec les `.bin`. Piège : Linux est sensible à la casse des `#include` (`include.h`, pas `Include.h`).
- Les anciennes copies (V4.20, V4.20-trace) sont remplacées par l'historique Git.
- Ajouter ici chaque décision importante et piège rencontré.
