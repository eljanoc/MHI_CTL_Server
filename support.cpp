#include "support.h"
#include <Arduino.h>

extern WiFiClient espClient;
extern PubSubClient MQTTclient;

struct rising_edge_cnt_struct {
  volatile uint32_t SCK = 0;
  volatile uint32_t MOSI = 0;
  volatile uint32_t MISO = 0;
} rising_edge_cnt;

IRAM_ATTR void handleInterrupt_SCK() {
  rising_edge_cnt.SCK++;
}

IRAM_ATTR void handleInterrupt_MOSI() {
  rising_edge_cnt.MOSI++;
}

IRAM_ATTR void handleInterrupt_MISO() {
  rising_edge_cnt.MISO++;
}

void MeasureFrequency() {  // measure the frequency on the pins
  pinMode(SCK_PIN, INPUT);
  pinMode(MOSI_PIN, INPUT);
  pinMode(MISO_PIN, INPUT);
  DBG_PRINTLN(F("Measure frequency for SCK, MOSI and MISO pin"));
  attachInterrupt(digitalPinToInterrupt(SCK_PIN), handleInterrupt_SCK, RISING);
  attachInterrupt(digitalPinToInterrupt(MOSI_PIN), handleInterrupt_MOSI, RISING);
  attachInterrupt(digitalPinToInterrupt(MISO_PIN), handleInterrupt_MISO, RISING);
  unsigned long starttimeMicros = micros();
  while (micros() - starttimeMicros < 1000000)
    ;
  detachInterrupt(SCK_PIN);
  detachInterrupt(MOSI_PIN);
  detachInterrupt(MISO_PIN);

  DBG_PRINTF_P(PSTR("SCK frequency=%iHz (expected: >3000Hz) "), rising_edge_cnt.SCK);
  if (rising_edge_cnt.SCK > 3000) {
    DBG_PRINTLN(F("o.k."));
  } else {
    DBG_PRINTLN(F("out of range!"));
  }

  DBG_PRINTF("MOSI frequency=%iHz (expected: <SCK frequency) ", rising_edge_cnt.MOSI);
  if ((rising_edge_cnt.MOSI > 30) & (rising_edge_cnt.MOSI < rising_edge_cnt.SCK)) {
    DBG_PRINTLN(F("o.k."));
  } else {
    DBG_PRINTLN(F("out of range!"));
  }

  DBG_PRINTF("MISO frequency=%iHz (expected: ~0Hz) ", rising_edge_cnt.MISO);
  if (rising_edge_cnt.MISO <= 10) {
    DBG_PRINTLN(F("o.k."));
  } else {
    DBG_PRINTLN(F("out of range!"));
    //    while (1);
  }
}

void printInfo() {
  char strtmp[50];
  DBG_PRINTLN(F(" connected"));
  DBG_PRINTF("MQTTclient.connected=%i\n", MQTTclient.connected());
  output_P((ACStatus)type_status, PSTR(TOPIC_CONNECTED), PSTR(PAYLOAD_CONNECTED_TRUE));
  output_P((ACStatus)type_status, PSTR(TOPIC_VERSION), PSTR(VERSION));
  itoa(WiFi.RSSI(), strtmp, 10);
  output_P((ACStatus)type_status, PSTR(TOPIC_RSSI), strtmp);
  WiFi.BSSIDstr().toCharArray(strtmp, 20);
  output_P((ACStatus)type_status, PSTR(TOPIC_WIFI_BSSID), strtmp);
}

void publish_cmd_ok() {
  output_P((ACStatus)type_status, PSTR(TOPIC_CMD_RECEIVED), PSTR(PAYLOAD_CMD_OK));
}
void publish_cmd_unknown() {
  output_P((ACStatus)type_status, PSTR(TOPIC_CMD_RECEIVED), PSTR(PAYLOAD_CMD_UNKNOWN));
}
void publish_cmd_invalidparameter() {
  output_P((ACStatus)type_status, PSTR(TOPIC_CMD_RECEIVED), PSTR(PAYLOAD_CMD_INVALID_PARAMETER));
}

void output_P(const ACStatus status, PGM_P topic, PGM_P payload) {
  const int mqtt_topic_size = 100;
  char mqtt_topic[mqtt_topic_size];
  DBG_PRINTF_P(PSTR("output_P status=%i\n"), status);

  if ((status & 0xc0) == type_status)
    strncpy_P(mqtt_topic, topic_prefix, mqtt_topic_size);
  else if ((status & 0xc0) == type_opdata)
    strncpy_P(mqtt_topic, MQTT_OP_PREFIX, mqtt_topic_size);
  else if ((status & 0xc0) == type_erropdata)
    strncpy_P(mqtt_topic, MQTT_ERR_OP_PREFIX, mqtt_topic_size);
  strncat_P(mqtt_topic, topic, mqtt_topic_size - strlen(mqtt_topic));
  // publish_P(char*) uses strnlen on a flash pointer: crashes on ESP8266
  bool b = MQTTclient.publish_P(mqtt_topic, (const uint8_t *)payload, strlen_P(payload), true);
  DBG_PRINTF_P(PSTR("publish :retour:%d status=%i topic=%s\n"), b, status, mqtt_topic);
}

#if TEMP_MEASURE_PERIOD > 0
OneWire oneWire(ONE_WIRE_BUS);        // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature.
DeviceAddress insideThermometer;      // arrays to hold device address

byte getDs18x20Temperature(int temp_hysterese) {
  static unsigned long DS1820Millis = millis();
  static int16_t tempR_old = 0xffff;

  if (millis() - DS1820Millis > TEMP_MEASURE_PERIOD * 1000) {
    int16_t tempR = sensors.getTemp(insideThermometer);
    tempR += ROOM_TEMP_DS18X20_OFFSET * 128;
    if (tempR > (48 * 128) || tempR < (-10 * 128)) {  // skip onrealistic values
      tempR = tempR_old;                              // use previous value
    }
    int16_t tempR_diff = tempR - tempR_old;  // avoid using other functions inside the brackets of abs, see https://www.arduino.cc/reference/en/language/functions/math/abs/
    if (abs(tempR_diff) > temp_hysterese) {
      tempR_old = tempR;
      char strtmp[10];
      dtostrf(sensors.rawToCelsius(tempR), 0, 2, strtmp);
#ifdef SERIAL_DEBUG
      {  //Serial.printf_P(PSTR("new DS18x20 temperature=%s°C\n"), strtmp);
#endif
        //Debug.printf_P(PSTR("new DS18x20 temperature=%s°C\n"), strtmp);}
        output_P((ACStatus)type_status, PSTR(TOPIC_TDS1820), strtmp);
      }
      DS1820Millis = millis();
      sensors.requestTemperatures();
    }
#ifdef SERIAL_DEBUG
    {  //Serial.printf_P(PSTR("temp DS18x20 tempR_old=%i %i\n"), tempR_old, (byte)(tempR_old/32 + 61));
#endif
      //Debug.printf_P(PSTR("temp DS18x20 tempR_old=%i %i\n"), tempR_old, (byte)(tempR_old/32 + 61));}
      if (tempR_old < 0)
        return 0;
      return tempR_old / 32 + 61;
    }

    void printAddress(DeviceAddress deviceAddress) {
      for (uint8_t i = 0; i < 8; i++) {
        if (deviceAddress[i] < 16) {
          DBG_PRINT(F("0"));
        }
        DBG_PRINT(deviceAddress[i], HEX);
      }
    }


    void setup_ds18x20() {
      sensors.begin();
      DBG_PRINTF_P(PSTR("Found %i DS18xxx family devices.\n"), sensors.getDS18Count());
      if (!sensors.getAddress(insideThermometer, 0)) {
        DBG_PRINTLN(F("Unable to find address for Device 0"));
      } else {
        DBG_PRINTF_P(PSTR("Device 0 Address: 0x%02x\n"), insideThermometer);
      }
      sensors.setResolution(insideThermometer, 9);  // set the resolution to 9 bit
      sensors.setWaitForConversion(false);
      sensors.requestTemperatures();  // Send the command to get temperatures
    }
#endif