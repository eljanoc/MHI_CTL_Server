using namespace std;
#include "include.h"
#ifdef WEB_SERVER
#include "mainHTML.h"
#include "paramHTML.h"
#ifdef ESP32
#include <WebServer.h>
WebServer webServer(80);
#else
#include <ESP8266WebServer.h>
ESP8266WebServer webServer(80);
#endif

#ifdef TELNET_DEBUG
extern RemoteDebug Debug;
#endif

void WiFiDisconnectAndRestart();
void MQTT_subscribe_callback(const char *topic, uint8_t *payload, unsigned int length);
extern char MQTT_SET_PREFIX[];
void setTrace(bool enable);
size_t getTraceData(char *output, size_t capacity);

void newValue() {
  int nb = webServer.args();
#ifdef TELNET_DEBUG
  Debug.printf("*******************SetValue nb=%d !!!!!!!!!!!!!!!!!!\n",nb);
  Debug.println(MQTT_SET_PREFIX);
  Debug.println("Received from webClient");
#endif
#ifdef SERIAL_DEBUG
  Serial.printf("*******************SetValue nb=%d !!!!!!!!!!!!!!!!!!!\n",nb);
  Serial.println(MQTT_SET_PREFIX);
  Serial.println("Received from webClient");
#endif
  for (int i = 0; i < nb; i++) {
#ifdef TELNET_DEBUG
    Debug.println("Name:" + webServer.argName(i));
    Debug.println("Value:" + webServer.arg(i));
#endif
#ifdef SERIAL_DEBUG
    Serial.println("Name:" + webServer.argName(i));
    Serial.println("Value:" + webServer.arg(i));
#endif
    char topic[30];
    String tmpName = webServer.argName(i);
    String payload = webServer.arg(i);

    sprintf(topic, "%s%s", MQTT_SET_PREFIX, tmpName.c_str());
#ifdef TELNET_DEBUG
    Debug.printf("%s:%s\n", topic, payload.c_str());
#endif
#ifdef SERIAL_DEBUG
    Serial.printf("%s:%s\n", topic, payload.c_str());
#endif
    MQTT_subscribe_callback(topic, (uint8_t *)payload.c_str(), strlen(payload.c_str()));
  }
  webServer.send(200, "text/plain", String("OK"));
}

static char login[] = "John";
static char psw[] = "Pass";
bool verifAuthor() {  // If a POST request is made to URI /login
  Debug.println("verifAuthor");
  if (!webServer.hasArg("username") || !webServer.hasArg("password")
      || webServer.arg("username") == NULL || webServer.arg("password") == NULL) {  // If the POST request doesn't have username and password data
    webServer.send(400, "text/plain", "400: Invalid Request");                      // The request is invalid, so send HTTP status 400
    Debug.println("Return false");
    return false;
  }
  if (!strcmp(webServer.arg("username").c_str(),login) && !strcmp(webServer.arg("password").c_str(),psw)) {  // If both the username and the password are correct
    //    webServer.send(200, "text/html", "<h1>Welcome, " + webServer.arg("username") + "!</h1><p>Login successful</p>");
    Debug.println("Return true");
    return true;
  } else {  // Username and password don't match
    Debug.println("Return Unauthorized false");
    webServer.send(401, "text/plain", "401: Unauthorized");
    return false;
  }
}

extern int stateFlag;
/********************************** Core 0 ***********************************/
char *getWebValuesString(char *);
char *getParametersString(char *);
void newParams(void);
static void webServerTaskSetup() {

#ifdef MULTICORE
  esp_task_wdt_init(30, false);
  esp_task_wdt_add(NULL);  //add current thread to WDT watch
#endif

  webServer.on("/NewParam", newParams);

  webServer.on("/restart", []() {
    stateFlag=GO_RESTART;;
    webServer.send(200, "text/html", "restarting...");
  });

  webServer.on("/change", []() {
    stateFlag=GO_CHANGE;
    webServer.send(200, "text/plain", "changing...");
  });

  webServer.on("/wifimanager", []() {
    stateFlag=GO_WIFIMANAGER;
    webServer.send(200, "text/plain", "starting WiFi Manager...");
  });

  webServer.on("/reinit", []() {
    stateFlag=GO_REINIT;
    webServer.send_P(200, "text/html", "ok");
  });


  webServer.on("/", []() {
    webServer.send_P(200, "text/html", MainHtml);
  });

  webServer.on("/param.html", []() {
#ifdef SERIAL_DEBUG
    Serial.println("request ParamHtml");
#endif
    webServer.send_P(200, "text/html", ParamHtml);
  });

  webServer.on("/MainJS", []() {
#ifdef SERIAL_DEBUG
    Serial.println("request MainJS");
#endif
#ifdef TELNET_DEBUG
    Debug.println("request MainJS");
#endif
    webServer.send_P(200, "text/html", MainJS);
  });

  webServer.on("/ParamJS", []() {
#ifdef SERIAL_DEBUG
    Serial.println("request ParamJS");
#endif
#ifdef TELNET_DEBUG
    Debug.println("request ParamJS");
#endif
    webServer.send_P(200, "text/html", ParamJS);
  });

  webServer.on("/SetValue", newValue);

  webServer.on("/ajax_data",[]() {
    char tmp[200];
    webServer.send(200, "text/html", getWebValuesString(tmp));
  });

  webServer.on("/param_data",[]() {
    char tmp[200];
    webServer.send(200, "text/html", getParametersString(tmp));
  });

  webServer.on("/trace", HTTP_POST, []() {
    if (!webServer.hasArg("enable")) {
      webServer.send(400, "text/plain", "Missing enable parameter");
      return;
    }
    String value = webServer.arg("enable");
    if (value != "0" && value != "1") {
      webServer.send(400, "text/plain", "enable must be 0 or 1");
      return;
    }
    setTrace(value == "1");
    webServer.send(200, "text/plain", "OK");
  });

  webServer.on("/trace_data", []() {
    const size_t capacity = 2 * 64 * 43 + 100;  // 64 frames x2 (AC and board) of 40 hex chars + quotes and comma
    char *data = (char *)malloc(capacity);
    if (!data) {
      webServer.send(500, "text/plain", "Out of memory");
      return;
    }
    if (!getTraceData(data, capacity)) {
      free(data);
      webServer.send(500, "text/plain", "Unable to build trace data");
      return;
    }
    webServer.send(200, "application/json", data);
    free(data);
  });

  webServer.begin();
}

int lastTimeServerMillisCore0 = 0;
void loopWebServer(void) {
  unsigned long int curMillis = millis();
  ;
  if ((curMillis - lastTimeServerMillisCore0) > 100) {
    lastTimeServerMillisCore0 = curMillis;
    webServer.handleClient();
  }
}

#ifdef MULTICORE
static TaskHandle_t webServerTaskHandle;

static void webServerTask(void *pvParameters) {
  unsigned long int nbLoop = 0, lastPrintMillis = 0;
  /***************** setup Core 0 ****************/
  esp_task_wdt_add(NULL);  //add current thread to WDT watch
  esp_task_wdt_reset();

  webServerTaskSetup();

  while (true) { /********************* loop0 Core 0 ******************/
    unsigned long int curMillis = millis();
    vTaskDelay(10);
    esp_task_wdt_reset();  //Reset du Watchdog toutes  les 10s
    int core = xPortGetCoreID();

    loopWebServer();
    if (nbLoop && (curMillis - lastPrintMillis) > 4000) {
      lastPrintMillis = curMillis;
//      moyLoop/=nbLoop;
#ifdef SERIAL_DEBUG
      Serial.printf("Core:%d, nbLoop:%d\n", core, nbLoop);
#endif
#ifdef TELNET_DEBUG
      Debug.printf("Core:%d, nbLoop:%d\n", core, nbLoop);
#endif
      nbLoop = 0;
      //    minLoop=100000;
      //    nbLoop=maxLoop=moyLoop=nbLoop=0;
    } else
      nbLoop++;
  }
}
#endif

void loadWebServer() {
#ifdef MULTICORE
  esp_task_wdt_init(WDT_TIMEOUT, true);  //enable panic so ESP32 restarts
  xTaskCreatePinnedToCore(               //Préparation Tâche Multi Coeur
    webServerTask,                       /* Task function. */
    "webServerTask",                     /* name of task. */
    10000,                               /* Stack size of task */
    NULL,                                /* parameter of the task */
    10,                                  /* priority of the task */
    &webServerTaskHandle,                /* Task handle to keep track of created task */
    0);                                  /* pin task to core 0 */
#else
  webServerTaskSetup();
#endif  // MULTICORE
}  //setup

#endif