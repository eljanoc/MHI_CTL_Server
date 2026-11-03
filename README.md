# MHI-AC-CtrlServer (V4.32-trace)

**English** | [Français](README.fr.md)

**ESP8266 / ESP32** firmware that controls a **Mitsubishi Heavy Industries (MHI)** air conditioner by connecting to the SPI bus of its indoor unit. It makes the unit controllable and observable through **MQTT** (home automation, e.g. Jeedom) and an embedded **web interface**.

It can also run **standalone**, without Jeedom or any MQTT broker: the ESP alone serves the web interface to control and monitor the unit (since V4.26 the page stays responsive even when no broker is reachable).

This project is a server variant of [MHI-AC-Ctrl](https://github.com/absalom-muc/MHI-AC-Ctrl), extended with a web server, online configuration and a frame trace system.

## Goals
- Control the air conditioner from a home automation box without using the infrared remote.
- Continuously report the state of the unit and the operating data of the indoor and outdoor units.
- Offer simple configuration without recompiling: WiFi, MQTT and settings from a web page.
- Help with diagnosis and understanding of the protocol by capturing raw frames.

## Features

### Commands (MQTT, `set/...` topics)
- Power on / off (`Power`)
- Mode (`Mode`): Auto, Cool, Dry, Fan, Heat, Stop
- Setpoint temperature (`Tsetpoint`)
- Fan speed (`Fan`)
- Vanes (`Vanes`, including Swing)
- Room temperature provided by an external sensor (`Troom`)
- Request for error / operating data (`ErrOpData`) and `reset`

### Published measurements and states (MQTT)
- State: `Power`, `Mode`, `Fan`, `Vanes`, `Troom`, `Tsetpoint`, `Errorcode`
- Indoor unit: `RETURN-AIR`, `THI-R1/R2/R3`, `IU-FANSPEED`, `TOTAL-IU-RUN`
- Outdoor unit: `OUTDOOR`, `COMP`, `TD`, `THO-R1`, `CT`, `TDSH`, `PROTECTION-NO`, `OU-FANSPEED`, `DEFROST`, `TOTAL-COMP-RUN`, `OU-EEV1`, `KWH`
- Module supervision: `connected`, `Version`, `RSSI`, `WIFI_LOST`, `MQTT_LOST`, `WIFI_BSSID`, `cmd_received`
- Optional DS18B20 temperature sensor (`Tds1820`)

### Embedded web interface
- Main page: state and controls of the air conditioner.
- Settings page: WiFi / MQTT configuration, stored in the file system (LittleFS), protected by a login and password.
- Actions: restart, reset, open WiFiManager.

### Connectivity and maintenance
- WiFi configuration through a captive portal (WiFiManager).
- Remote firmware update (OTA).
- Time synchronization through NTP.
- Remote debugging over Telnet (RemoteDebug).

### Frame trace system
Captures the SPI frames exchanged with the indoor unit (20 bytes): the MOSI and MISO frames of each exchange.
- Enabled with the "Détection des trames" checkbox on the web page, or through MQTT: `traceon` / `traceoff`.
- Live display of the frames in the web page (history of 64 frames, refreshed every second).
- MOSI frames published in hexadecimal on the debug topic `<debug>/trame`.
- Dropped frames counter, reported when the trace is stopped.

## Hardware
- ESP8266 (tested on Wemos D1 mini) or ESP32.
- Connection to the SPI connector of the MHI indoor unit (see the MHI-AC-Ctrl project for the details of the air conditioner connector).

| Signal | ESP8266 (D1 mini) | ESP32 |
|---|---|---|
| SCK | GPIO 14 (D5) | GPIO 18 |
| MOSI | GPIO 13 (D7) | GPIO 23 |
| MISO | GPIO 12 (D6) | GPIO 19 |

The pins are defined in `MHI-AC-Ctrl-core.h`. The optional DS18B20 temperature sensor connects to GPIO 4 (D2 on the D1 mini), defined in `support.h`.

## First start
1. On a blank board, the ESP opens a WiFi access point named `Clim<name>` (the default name is `RoomName`, so `ClimRoomName`), with the password `climatiseur`.
2. Connect to this access point: the configuration portal (WiFiManager) opens.
3. Enter the WiFi network, the MQTT server address and its credentials, as well as the room and device names.
4. The ESP restarts, connects to the WiFi and publishes its data on MQTT. The web interface is then available at its IP address.

The settings are stored in the ESP memory (LittleFS), not in the Git repository.

## Security
Default values are hard-coded in the firmware. Change them before any use outside a trusted local network:
- **Web page**: login `John` and password `Pass` (`webserver.cpp`, `aswebserver.cpp`).
- **Configuration WiFi access point and OTA update**: password `climatiseur` (`setup.cpp`, variable `configPW`). The same password is used for OTA.
- MQTT credentials are entered in the configuration portal and are not part of the repository.
- If the MQTT broker is unavailable, the web interface is serviced before MQTT reconnect attempts, which are spaced 5 seconds apart. On ESP8266, the TCP connection timeout is limited to 250 ms; the MQTT response timeout is 1 second.

Do not expose the ESP directly to the internet.

## Download firmware (Releases)

Ready-to-flash binaries are built by GitHub Actions and attached to each [release](https://github.com/eljanoc/MHI_CTL_Server/releases) (tags `v*`):

- `MHI-esp8266-<tag>.bin`: ESP8266 firmware (OTA or first flash).
- `MHI-esp32-<tag>.bin`: ESP32 application only, for updates (OTA) of an already configured board.
- `MHI-esp32-<tag>-merged.bin`: full ESP32 flash image (bootloader + partitions + application, 4 MB). Use it for a new or blank ESP32, or to recover a corrupted one: `esptool.py --chip esp32 write_flash 0x0 MHI-esp32-<tag>-merged.bin`

## Build and upload
- Tool: Arduino IDE / `arduino-cli`.
- ESP8266 board: `esp8266:esp8266:d1_mini`.
- Libraries: ArduinoJson (7.4.3), PubSubClient (2.8), OneWire (2.3.8), DallasTemperature (4.0.6), plus three modified libraries provided in `libs/`: WiFiManager 2.0.17 (`libs/WiFiManager`) RemoteDebug 3.0.5 (`libs/RemoteDebug`) and NTPClient 3.2.1 (`libs/NTPClient`, `byte` replaced by `uint8_t` in `NTPClient.h`, ambiguity with `std::byte` under `using namespace std`).
- Board packages: esp32 3.1.3 (board `esp32:esp32:esp32doit-devkit-v1`) and esp8266 2.7.4 (`esp8266:esp8266:d1_mini`). The generic `esp32` board does not compile (`LED_BUILTIN` is missing).
- The project folder must be named `MHI-AC-CtrlServer` (same as the `.ino` file): `git clone https://github.com/eljanoc/MHI_CTL_Server.git MHI-AC-CtrlServer`.

### WiFiManager fix for ESP8266 only
This fix only concerns ESP8266 builds. With the official WiFiManager 2.0.17, `getMenuOut()` directly compares `_menutokens[menuId]` (a string stored in flash memory) with a string, which caused the ESP8266 to reboot in a loop about one minute after startup. The value must be converted through `__FlashStringHelper`. In `WiFiManager.cpp`, function `WiFiManager::getMenuOut()` (around line 1442), replace:
```cpp
if((String)_menutokens[menuId] == "param" && _paramsCount == 0) continue;
if((String)_menutokens[menuId] == "custom" && _customMenuHTML!=NULL){
```
with:
```cpp
if((String)(__FlashStringHelper *)(_menutokens[menuId]) == "param" && _paramsCount == 0) continue;
if((String)(__FlashStringHelper *)(_menutokens[menuId]) == "custom" && _customMenuHTML!=NULL){
```
The fix is already applied in the project's `libs/WiFiManager` and in the installed library (`Documents\Arduino\libraries\WiFiManager`). If WiFiManager is reinstalled or updated, it must be applied again.
This change is not needed for the ESP32: it addresses the problem observed on ESP8266 only.

### RemoteDebug fix for ESP32
The official RemoteDebug 3.0.5 does not compile with the ESP32 core 3.x: `src/utility/WebSockets.cpp` includes `<hwcrypto/sha.h>`, which no longer exists. The only change is that line, replaced by:
```cpp
#include <esp32/sha.h> // <hwcrypto/sha.h>
```
The fix is already applied in the project's `libs/RemoteDebug` copy (MIT license kept). If you use the registry version instead, apply it yourself.

### OTA update
OTA update is possible through the VS Code task "OTA ESP8266 141" (see `.vscode/tasks.json`). It contains the IP address of the author's device and a masked password (`******`): adapt both to your setup.

## Code organization
| File | Role |
|---|---|
| `MHI-AC-CtrlServer.ino` | Main program, MQTT callbacks, publishing |
| `MHI-AC-Ctrl-core.cpp/.h` | SPI dialogue with the air conditioner, frame capture |
| `MHI-AC-Ctrl.h` | MQTT topic names and texts |
| `setup.cpp`, `loop.cpp` | Initialization and main loop (WiFi, MQTT, commands) |
| `webserver.cpp`, `aswebserver.cpp` | Web server (synchronous / asynchronous) |
| `mainHTML.h`, `paramHTML.h` | Embedded web pages |
| `support.cpp/.h`, `include.h` | Utility functions and build options |
| `params_logic.h` | Pure parameter logic (`/NewParam` parsing, `/param_data` building), testable on a PC |
| `test/test_params.cpp` | Automated tests for that logic |

### Tests
Tests run on a PC, no board needed: `g++ -I. test/test_params.cpp -o test_params && ./test_params`. CI runs them before building the firmwares.

## Versions
Versions are managed with Git tags, for example `v4.25-trace`.

### V4.32
The configuration page now lists the Wi-Fi networks in range in a drop-down next to the SSID field, each with its signal level in dBm. Picking one fills the SSID and, for the saved network, its password; typing an SSID by hand still works. The Wi-Fi list in `/param_data` is now made of triplets (SSID, password, RSSI) starting at index 10, and the response buffer grew to 4096 bytes. Automated PC tests of the parameter logic were added (`test/test_params.cpp`).

### V4.31
New "MQTT active" checkbox on the configuration page (`config.json` key `mqtt_enabled`, default on). Unchecked, the module runs standalone without any MQTT attempt (shown as `off`). The configuration page hides the MQTT IP and port fields when the box is unchecked, and shows the firmware version under the title. An empty server or `0.0.0.0` still disables MQTT. `/param_data` and `/NewParam` gain a 10th field (after `Thing`); the Wi-Fi list now starts at index 10.

### V4.30
Fixed buffer overflows: the Wi-Fi network list (`/param_data`) overflowed a 200-byte buffer when several networks were in range, the web setting route (`/SetValue`) used a 30-byte topic buffer, and `/NewParam` crashed on a malformed request. All string copies are now bounded, and `/NewParam` answers 400 on invalid input.

### V4.29
Cleanup: removed a debug trace that ran on every web request, and guarded the setpoint traces with `TELNET_DEBUG`.

### V4.28
Fixed: the "Last reboot" field showed 00/01/1900 0h00 without MQTT, because the date was only recorded on the first MQTT connection. It is now set as soon as the NTP time is known.

### V4.27
New: standalone mode. An empty MQTT server or `0.0.0.0` disables MQTT: no connection attempt, and no automatic restart after 30 failed attempts (with an unreachable broker the ESP restarted about every 2.5 minutes).

### V4.26
Fixed: the web page was extremely slow while the MQTT broker was unreachable. The main loop retried the MQTT connection on every pass (blocking on the TCP connect and a 1 s `delay`) and the web server was only polled every 100 ms after it.
- The web server is now polled on every loop pass, before the MQTT reconnect.
- MQTT reconnect attempts are spaced 5 seconds apart, with no blocking `delay` on failure.
- ESP8266: TCP connect timeout limited to 250 ms; MQTT socket timeout 1 s; Nagle disabled for the web server.

## Acknowledgments
Many thanks to **absalom-muc** for [MHI-AC-Ctrl](https://github.com/absalom-muc/MHI-AC-Ctrl): his work decoding the SPI protocol of Mitsubishi Heavy air conditioners, and releasing it as open source, is the origin of this project. Thanks also to **tzapu** for WiFiManager and to the authors of the libraries used.

## Credits and license
- This project is derived from [MHI-AC-Ctrl](https://github.com/absalom-muc/MHI-AC-Ctrl) by absalom-muc, released under the MIT license. The decoding of the MHI SPI protocol owes a lot to it.
- [WiFiManager](https://github.com/tzapu/WiFiManager) by tzapu (MIT license), copied into `libs/WiFiManager` with the ESP8266 fix described above.
- [RemoteDebug](https://github.com/JoaoLopesF/RemoteDebug) by Joao Lopes (MIT license), copied into `libs/RemoteDebug` with the ESP32 fix described above.
- Third-party libraries: ArduinoJson, PubSubClient, NTPClient, OneWire, DallasTemperature.
- This repository is released under the MIT license (see [LICENSE](LICENSE)), which keeps the copyright notice of the original project.
