using namespace std;
#include "include.h"

#ifdef AS_WEB_SERVER
#include "mainHTML.h"
#include <ESPAsyncWebServer.h>
AsyncWebServer  webServer(80);

#ifdef TELNET_DEBUG
extern RemoteDebug Debug;
#endif

void MQTT_subscribe_callback(const char *topic, uint8_t *payload, unsigned int length);
extern char MQTT_SET_PREFIX[];

void newValue(AsyncWebServerRequest *request) {
  int nb = request->args();
#ifdef TELNET_DEBUG
  Debug.println("*******************SetValue !!!!!!!!!!!!!!!!!!!");
  Debug.println(MQTT_SET_PREFIX);
  Debug.println("Received from webClient");
#endif
  for (int i = 0; i < nb; i++) {
#ifdef TELNET_DEBUG
    Debug.println("Name:" + request->argName(i));
    Debug.println("Value:" + request->arg(i));
#endif
    char topic[30];
    String tmpName = request->argName(i);
    String payload = request->arg(i);

    sprintf(topic, "%s%s", MQTT_SET_PREFIX, tmpName.c_str());
#ifdef TELNET_DEBUG
    Debug.printf("%s:%s\n", topic, payload.c_str());
#endif
    MQTT_subscribe_callback(topic, (uint8_t *)payload.c_str(), strlen(payload.c_str()));
  }
  request->send(200, "text/plain", String("OK"));
}
static char login[] = "John";
static char psw[] = "Pass";
bool verifAuthor(AsyncWebServerRequest *request) {  // If a POST request is made to URI /login
  Debug.println("verifAuthor");
  if (!request->hasArg("username") || !request->hasArg("password")
      || request->arg("username") == NULL || request->arg("password") == NULL) {  // If the POST request doesn't have username and password data
    request->send(400, "text/plain", "400: Invalid Request");                      // The request is invalid, so send HTTP status 400
    Debug.println("Return false");
    return false;
  }
  if (!strcmp(request->arg("username").c_str(),login) && !strcmp(request->arg("password").c_str(),psw)) {  // If both the username and the password are correct
    //    request->send(200, "text/html", "<h1>Welcome, " + request->arg("username") + "!</h1><p>Login successful</p>");
    Debug.println("Return true");
    return true;
  } else {  // Username and password don't match
    Debug.println("Return Unauthorized false");
    request->send(401, "text/plain", "401: Unauthorized");
    return false;
  }
}

extern int stateFlag;
extern char *getWebValuesString();
/********************************** Core 0 ***********************************/
static void webServerTaskSetup() {

#ifdef MULTICORE
  esp_task_wdt_init(30, false);
  esp_task_wdt_add(NULL);  //add current thread to WDT watch
#endif

  webServer.on("/restart", [](AsyncWebServerRequest *request) {
    stateFlag=GO_RESTART;
    request->send(200, "text/html", "restarting...");
  });

  webServer.on("/wifimanager", [](AsyncWebServerRequest *request) {
    stateFlag=GO_WIFIMANAGER;
    request->send(200, "text/html", "Go WiFiManager...");
  });

  webServer.on("/reinit", [](AsyncWebServerRequest *request) {
    stateFlag=GO_REINIT;;
    request->send_P(200, "text/html", "Go Reinit...");
  });

  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
//  webServer.on("/", [](AsyncWebServerRequest *request){
//    request->send_P(200, "text/html", index_html, processor);
#ifdef SERIAL_DEBUG
      Serial.printf("webServer.on\n");
#endif
#ifdef TELNET_DEBUG
      Debug.printf("webServer.on\n");
#endif
    request->send_P(200, "text/html", MainHtml);
  });
#if 0
  webServer.on("/", [](AsyncWebServerRequest *request) {  // Call the 'handleLogin' function when a POST request is made   webServer.on("/", []() {
                            //    webServer.send(200, "text/html", String(MainHtml));
                            /*  if (verifAuthor())
    request->send(200, "text/html", String(MainHtml));
  else*/
    webServer.send(200, "text/html", "<form style=\"text-align:center;width:290px;margin:auto;margin-top:0px; color: white; font-size:2rem;\" action=\"/login\" method=\"POST\"><input type=\"text\" name=\"username\" placeholder=\"Username\"></br><input type=\"password\" name=\"password\" placeholder=\"Password\"></br><input type=\"submit\" value=\"Login\"></form>");
  });
#endif

  webServer.on("/MainJS", [](AsyncWebServerRequest *request) {
#ifdef SERIAL_DEBUG
    Serial.println("request MainJS");
#endif
#ifdef TELNET_DEBUG
    Debug.println("request MainJS");
#endif
    request->send_P(200, "text/html", MainJS);
  });
  webServer.on("/SetValue", newValue);

  webServer.on("/ajax_data", [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getWebValuesString());
  });

  webServer.begin();
}

#ifdef MULTICORE
static TaskHandle_t webServerTaskHandle;

static void webServerTask(void *pvParameters) {
  unsigned long int lastResetWDT=0;
  /***************** setup Core 0 ****************/
  esp_task_wdt_add(NULL);  //add current thread to WDT watch
  esp_task_wdt_reset();

  webServerTaskSetup();

  while (true) { /********************* loop0 Core 0 ******************/
    unsigned long int curMillis = millis();
    if ((curMillis - lastResetWDT)>10000)
    {
      vTaskDelay(10);
      esp_task_wdt_reset();  //Reset du Watchdog toutes  les 1s
      lastResetWDT=curMillis;
#ifdef SERIAL_DEBUG
      Serial.printf("Core:%d aSyncWebServer\n", xPortGetCoreID());
#endif
#ifdef TELNET_DEBUG
      Debug.printf("Core:%d aSyncWebServer\n", xPortGetCoreID());
#endif
    }
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

#endif    //AS_WEB_SERVER