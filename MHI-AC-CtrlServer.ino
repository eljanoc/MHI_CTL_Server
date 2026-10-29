// MHI-AC-Ctrl by absalom-muc
// read + write data via SPI controlled by MQTT
// for version see support.h

#include "MHI-AC-Ctrl-core.h"
#include "MHI-AC-Ctrl.h"
#include "support.h"

#define WEB_VERSION "V4.25"
MHI_AC_Ctrl_Core mhi_ac_ctrl_core;

POWER_STATUS power_status = unknown;

unsigned long room_temp_set_timeout_Millis = millis();
bool troom_was_set_by_MQTT = false;

char myPSTR_s[100];
char* myPSTR(char* s1, const char* s2) {
  sprintf(myPSTR_s, "%s%s", s1, s2);
  return myPSTR_s;
}
#if 0
bool dejaImprime = false;
void printTopic() {
  if (dejaImprime)
    return;
  dejaImprime = true;
#ifdef SERIAL_DEBUG
  Serial.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_POWER));
  Serial.printf("payload:%s\n", PSTR(PAYLOAD_POWER_ON));
  Serial.printf("payload:%s\n", PSTR(PAYLOAD_POWER_OFF));
  Serial.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_MODE));
  Serial.printf("payload:%s\n", PSTR(PAYLOAD_POWER_OFF));
  Serial.printf("payload:%s\n", PSTR(PAYLOAD_MODE_AUTO));
  Serial.printf("payload:%s\n", PSTR(PAYLOAD_MODE_DRY));
  Serial.printf("payload:%s\n", PSTR(PAYLOAD_MODE_COOL));
  Serial.printf("payload:%s\n", PSTR(PAYLOAD_MODE_FAN));
  Serial.printf("payload:%s\n", PSTR(PAYLOAD_MODE_HEAT));
  Serial.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_TSETPOINT));
  Serial.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_FAN));
  Serial.printf("payload:%s\n", PAYLOAD_FAN_AUTO);
  Serial.printf("payload:%s\n", "1");
  Serial.printf("payload:%s\n", "2");
  Serial.printf("payload:%s\n", "3");
  Serial.printf("payload:%s\n", "4");
  Serial.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_VANES));
  Serial.printf("payload:%s\n", PSTR(PAYLOAD_VANES_SWING));
  Serial.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_TROOM));
  Serial.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_REQUEST_ERROPDATA));
  Serial.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_REQUEST_RESET));
  Serial.printf("payload:%s\n", PSTR(PAYLOAD_REQUEST_RESET));
#endif
#ifdef TELNET_DEBUG
  Debug.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_POWER));
  Debug.printf("payload:%s\n", PSTR(PAYLOAD_POWER_ON));
  Debug.printf("payload:%s\n", PSTR(PAYLOAD_POWER_OFF));
  Debug.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_MODE));
  Debug.printf("payload:%s\n", PSTR(PAYLOAD_POWER_OFF));
  Debug.printf("payload:%s\n", PSTR(PAYLOAD_MODE_AUTO));
  Debug.printf("payload:%s\n", PSTR(PAYLOAD_MODE_DRY));
  Debug.printf("payload:%s\n", PSTR(PAYLOAD_MODE_COOL));
  Debug.printf("payload:%s\n", PSTR(PAYLOAD_MODE_FAN));
  Debug.printf("payload:%s\n", PSTR(PAYLOAD_MODE_HEAT));
  Debug.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_TSETPOINT));
  Debug.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_FAN));
  Debug.printf("payload:%s\n", PAYLOAD_FAN_AUTO);
  Debug.printf("payload:%s\n", "1");
  Debug.printf("payload:%s\n", "2");
  Debug.printf("payload:%s\n", "3");
  Debug.printf("payload:%s\n", "4");
  Debug.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_VANES));
  Debug.printf("payload:%s\n", PSTR(PAYLOAD_VANES_SWING));
  Debug.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_TROOM));
  Debug.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_REQUEST_ERROPDATA));
  Debug.printf("topic:%s\n", myPSTR(MQTT_SET_PREFIX, TOPIC_REQUEST_RESET));
  Debug.printf("payload:%s\n", PSTR(PAYLOAD_REQUEST_RESET));
#endif
}
#endif

void TopicService(byte* payload, unsigned int length);

extern char service[40];

void MQTT_subscribe_callback(const char* topic, byte* payload, unsigned int length) {
  payload[length] = 0;  // we need a string
#ifdef SERIAL_DEBUG
  Serial.printf_P(PSTR("MQTT_subscribe_callback, topic=%s payload=%s payload_length=%i\n"), topic, (char*)payload, length);
#endif
#ifdef TELNET_DEBUG
  Debug.printf_P(PSTR("MQTT_subscribe_callback, topic=%s payload=%s payload_length=%i\n"), topic, (char*)payload, length);
//  printTopic();
#endif
  if (!strcmp((const char*)topic, (const char*)service)) {
#ifdef SERIAL_DEBUG
    Serial.printf("received service\n");
#endif
#ifdef TELNET_DEBUG
    Debug.printf("received service\n");
#endif
    TopicService(payload, length);
    return;
  }

#ifndef POWERON_WHEN_CHANGING_MODE
  if (strcmp_P(topic, myPSTR(MQTT_SET_PREFIX, TOPIC_POWER)) == 0) {
    if (strcmp_P((char*)payload, PSTR(PAYLOAD_POWER_ON)) == 0) {
      mhi_ac_ctrl_core.set_power(power_on);
      publish_cmd_ok();
    } else if (strcmp_P((char*)payload, PSTR(PAYLOAD_POWER_OFF)) == 0) {
      mhi_ac_ctrl_core.set_power(power_off);
      publish_cmd_ok();
    } else
      publish_cmd_invalidparameter();
  } else
#endif
    if (strcmp_P(topic, myPSTR(MQTT_SET_PREFIX, TOPIC_MODE)) == 0) {
#ifdef POWERON_WHEN_CHANGING_MODE
    if (strcmp_P((char*)payload, PSTR(PAYLOAD_POWER_OFF)) == 0) {
      mhi_ac_ctrl_core.set_power(power_off);
      publish_cmd_ok();
    } else
#endif
      if (strcmp_P((char*)payload, PSTR(PAYLOAD_MODE_AUTO)) == 0) {
      mhi_ac_ctrl_core.set_mode(mode_auto);
#ifdef POWERON_WHEN_CHANGING_MODE
      mhi_ac_ctrl_core.set_power(power_on);
#endif
      publish_cmd_ok();
    } else if (strcmp_P((char*)payload, PSTR(PAYLOAD_MODE_DRY)) == 0) {
      mhi_ac_ctrl_core.set_mode(mode_dry);
#ifdef POWERON_WHEN_CHANGING_MODE
      mhi_ac_ctrl_core.set_power(power_on);
#endif
      publish_cmd_ok();
    } else if (strcmp_P((char*)payload, PSTR(PAYLOAD_MODE_COOL)) == 0) {
      mhi_ac_ctrl_core.set_mode(mode_cool);
#ifdef POWERON_WHEN_CHANGING_MODE
      mhi_ac_ctrl_core.set_power(power_on);
#endif
      publish_cmd_ok();
    } else if (strcmp_P((char*)payload, PSTR(PAYLOAD_MODE_FAN)) == 0) {
      mhi_ac_ctrl_core.set_mode(mode_fan);
#ifdef POWERON_WHEN_CHANGING_MODE
      mhi_ac_ctrl_core.set_power(power_on);
#endif
      publish_cmd_ok();
    } else if (strcmp_P((char*)payload, PSTR(PAYLOAD_MODE_HEAT)) == 0) {
      mhi_ac_ctrl_core.set_mode(mode_heat);
#ifdef POWERON_WHEN_CHANGING_MODE
      mhi_ac_ctrl_core.set_power(power_on);
#endif
      publish_cmd_ok();
    } else
      publish_cmd_invalidparameter();
  } else if (strcmp_P(topic, myPSTR(MQTT_SET_PREFIX, TOPIC_TSETPOINT)) == 0) {
    float f = atof((char*)payload);
    if ((f >= 18) & (f <= 30)) {
      mhi_ac_ctrl_core.set_tsetpoint((byte)(2 * f));
      publish_cmd_ok();
    } else
      publish_cmd_invalidparameter();
  } else if (strcmp_P(topic, myPSTR(MQTT_SET_PREFIX, TOPIC_FAN)) == 0) {
    if (strcmp_P((char*)payload, PAYLOAD_FAN_AUTO) == 0) {
      mhi_ac_ctrl_core.set_fan(7);
      publish_cmd_ok();
    } else if (strcmp_P((char*)payload, "1") == 0) {
      mhi_ac_ctrl_core.set_fan(0);
      publish_cmd_ok();
    } else if (strcmp_P((char*)payload, "2") == 0) {
      mhi_ac_ctrl_core.set_fan(1);
      publish_cmd_ok();
    } else if (strcmp_P((char*)payload, "3") == 0) {
      mhi_ac_ctrl_core.set_fan(2);
      publish_cmd_ok();
    } else if (strcmp_P((char*)payload, "4") == 0) {
      mhi_ac_ctrl_core.set_fan(6);
      publish_cmd_ok();
    } else
      publish_cmd_invalidparameter();
  } else if (strcmp_P(topic, myPSTR(MQTT_SET_PREFIX, TOPIC_VANES)) == 0) {
    if (strcmp_P((char*)payload, PSTR(PAYLOAD_VANES_SWING)) == 0) {
      mhi_ac_ctrl_core.set_vanes(vanes_swing);
      publish_cmd_ok();
    } else {
      if ((atoi((char*)payload) >= 1) & (atoi((char*)payload) <= 5)) {
        mhi_ac_ctrl_core.set_vanes(atoi((char*)payload));
        publish_cmd_ok();
      } else
        publish_cmd_invalidparameter();
    }
  } else if (strcmp_P(topic, myPSTR(MQTT_SET_PREFIX, TOPIC_TROOM)) == 0) {
    float f = atof((char*)payload);
#ifdef ENHANCED_RESOLUTION
    f = f + mhi_ac_ctrl_core.get_troom_offset();  // increase Troom with current offset to compensate higher setpoint
#endif
    if ((f > -10) & (f < 48)) {
      room_temp_set_timeout_Millis = millis();  // reset timeout
      troom_was_set_by_MQTT = true;
      byte tmp = f * 4 + 61;
      mhi_ac_ctrl_core.set_troom(f * 4 + 61);
#ifdef SERIAL_DEBUG
      Serial.printf("ROOM_TEMP_MQTT: %f %i %i\n", f, (byte)(f * 4 + 61), (byte)tmp);
#endif
#ifdef TELNET_DEBUG
      Debug.printf("ROOM_TEMP_MQTT: %f %i %i\n", f, (byte)(f * 4 + 61), (byte)tmp);
#endif
      publish_cmd_ok();
    } else
      publish_cmd_invalidparameter();
  } else if (strcmp_P(topic, myPSTR(MQTT_SET_PREFIX, TOPIC_REQUEST_ERROPDATA)) == 0) {
    mhi_ac_ctrl_core.request_ErrOpData();
    publish_cmd_ok();
  } else if (strcmp_P(topic, myPSTR(MQTT_SET_PREFIX, TOPIC_REQUEST_RESET)) == 0) {
    if (strcmp_P((char*)payload, PSTR(PAYLOAD_REQUEST_RESET)) == 0) {
      publish_cmd_ok();
      delay(500);
      ESP.restart();
    } else
      publish_cmd_invalidparameter();
  } else
    publish_cmd_unknown();
}

/************************************ WEB Interface */  //////////////////////////////////////////

static const char* webModeTab[] = { PAYLOAD_MODE_STOP, PAYLOAD_MODE_AUTO, PAYLOAD_MODE_DRY, PAYLOAD_MODE_COOL, PAYLOAD_MODE_FAN, PAYLOAD_MODE_HEAT, "undef" };
static int webMode = 6;
static const char* webFanTab[] = { "1", "2", "3", "?3?", "?4?", "?5?", "4", PAYLOAD_FAN_AUTO, "undef" };
static int webFan = 8;
static const char* webVanesTab[] = { PAYLOAD_VANES_UNKNOWN, "1", "2", "3", "4", PAYLOAD_VANES_SWING, "undef" };
static int webVanes = 6;
static float webRoomTemp = 200.0;
static float webRoomTempExt = 200.0;
static int webTsetpoint = 200;
static bool webPower = false;
static float TauxDerreurs = 0;
extern struct tm tReboot;

extern char webPuissance[6];
extern char webIpSalon[40];
extern char webIpChNO[40];
extern char webIpCuisine[40];
extern char Thing[];
extern char MQTTconnectedFlag[4];
char* getWebValuesString(char *S) {
#ifdef ESP32
  #ifdef MULTICORE
    #ifdef WEB_SERVER 
      char Version[] = "esp32 Sync Multicore";
    #else
      char Version[] = "esp32 ASync Multicore";
    #endif
  #else
    #ifdef WEB_SERVER 
      char Version[] = "esp32 Sync";
    #else
      char Version[] = "esp32 ASync";
    #endif
  #endif
#else
  #ifdef WEB_SERVER 
    char Version[] = "esp8266 Sync";
  #else
    char Version[] = "esp8266 ASync";
  #endif
#endif

  char cWebTsetpoint[8] = "undef";
  if (webTsetpoint < 200)
    sprintf(cWebTsetpoint, "%d°C", webTsetpoint);
Debug.printf("Server:cWebTsetpoint:%s, webTsetpoint:%d\n",cWebTsetpoint,webTsetpoint);
  char cWebRoomTemp[8] = "undef";
  if (webRoomTemp < 150.0)
    sprintf(cWebRoomTemp, "%.1f°C", webRoomTemp);
  char cWebRoomTempExt[8] = "undef";
  if (webRoomTempExt < 150.0)
    sprintf(cWebRoomTempExt, "%.1f°C", webRoomTempExt);
  sprintf(S, "Deb?%s?%s?%s?%s?%s?%s?%s?%d?Warning: Taux d'erreurs=%.1f%%?%s %s&emsp;Last reboot : %02d/%02d/%d %2dh%02d?%s?%s?%s?%s?%s?Fin", 
    Thing,webModeTab[webMode], cWebTsetpoint, webFanTab[webFan], webVanesTab[webVanes], cWebRoomTemp, cWebRoomTempExt, webPower,
          TauxDerreurs,// warning,
          WEB_VERSION,
          Version, tReboot.tm_mday, /* Day.     [1-31]      */
          tReboot.tm_mon + 1,       /* Month.   [0-11]      */
          tReboot.tm_year + 1900,
          tReboot.tm_hour, /* Hours.   [0-23]      */
          tReboot.tm_min,  /* Minutes. [0-59]      */
          webPuissance, webIpSalon, webIpChNO, webIpCuisine,MQTTconnectedFlag);
#ifdef SERIAL_DEBUG
//  Serial.printf ("webValueString len : %d\n",strlen(S));
#endif
  return S;
}

/**************************************************************************************/


class StatusHandler : public CallbackInterface_Status {
public:
  void cbiStatusFunction(ACStatus status, int value) {
    char strtmp[10];
#ifdef POWERON_WHEN_CHANGING_MODE
    static int mode_tmp = 0xff;
#endif
#ifdef ENHANCED_RESOLUTION
    float offset = mhi_ac_ctrl_core.get_troom_offset();
    float tmp_value;
#endif
    static byte status_troom_old = 0xff;
#ifdef SERIAL_DEBUG
    //Serial.printf_P(PSTR("status=%i value=%i\n"), status, value);
#endif
    //Debug.printf_P(PSTR("status=%i value=%i\n"), status, value);
    switch (status) {
      case status_power:
        // After powerdown AC (230V), fan status is only showing 1, 2 or 3. 4 and Auto is not shown when changing with RC.
        // Only when setting fan to Auto one time after powerdown AC, it will show 4 and Auto.
        // Below will take care of this.
        if (power_status == unknown) {  // First time after startup esp
#ifdef SERIAL_DEBUG
          Serial.printf("power_status: unknown; received status_power: %i\n", value);
#endif
#ifdef TELNET_DEBUG
          Debug.printf("power_status: unknown; received status_power: %i\n", value);
#endif
          if (value == power_off) {  // Only when status is power off, set fan to Auto.
#ifdef SERIAL_DEBUG
            Serial.println("Set fan to Auto to fix fan status after powerdown (230V) AC");
#endif
#ifdef TELNET_DEBUG
            Debug.println("Set fan to Auto to fix fan status after powerdown (230V) AC");
#endif
            mhi_ac_ctrl_core.set_fan(7);
          }
        } else if (power_status == off) {
#ifdef SERIAL_DEBUG
          Serial.printf("power_status: off; received status_power: %i\n", value);
#endif
#ifdef TELNET_DEBUG
          Debug.printf("power_status: off; received status_power: %i\n", value);
#endif
        } else if (power_status == on) {
#ifdef SERIAL_DEBUG
          Serial.printf("power_status: on; received status_power: %i\n", value);
#endif
#ifdef TELNET_DEBUG
          Debug.printf("power_status: on; received status_power: %i\n", value);
#endif
        }
        if (value == power_on) {
          webPower = true;
          output_P(status, (TOPIC_POWER), PSTR(PAYLOAD_POWER_ON));
          power_status = on;
#ifdef POWERON_WHEN_CHANGING_MODE
          cbiStatusFunction(status_mode, mode_tmp);
#endif
        } else {
          webPower = false;
          output_P(status, (TOPIC_POWER), (PAYLOAD_POWER_OFF));
          power_status = off;
#ifdef POWERON_WHEN_CHANGING_MODE
          output_P(status, PSTR(TOPIC_MODE), PSTR(PAYLOAD_MODE_OFF));
#endif
        }
        break;
      case status_mode:
#ifdef POWERON_WHEN_CHANGING_MODE
        mode_tmp = value;
#endif
      case opdata_mode:
      case erropdata_mode:
        switch (value) {
          case mode_auto:
            if (status != erropdata_mode)
              webMode = 1;
            else
              webMode = 0;
            break;
          case mode_dry:
            webMode = 2;
            break;
          case mode_cool:
            webMode = 3;
            break;
          case mode_fan:
            webMode = 4;
            break;
          case mode_heat:
            webMode = 5;
            break;
        }
        if (webMode < 6)
          output_P(status, PSTR(TOPIC_MODE), webModeTab[webMode]);
        break;

      case opdata_kwh:
        dtostrf(highByte(value) * 64.0f + lowByte(value) * 0.25f, 0, 2, strtmp);
        output_P(status, PSTR(TOPIC_KWH), strtmp);
        break;
      case opdata_unknown:
        itoa(value, strtmp, 10);
        output_P(status, PSTR(TOPIC_UNKNOWN), strtmp);
        break;
      case status_fan:
        webFan = value;
        if (value < 8)
          output_P(status, TOPIC_FAN, webFanTab[webFan]);
        else
          output_P(status, TOPIC_FAN, "???");
#if 0
        if (value)
        switch (value) {
          case 0:
            output_P(status, TOPIC_FAN, "1");
            break;
          case 1:
            output_P(status, TOPIC_FAN, "2");
            break;
          case 2:
            output_P(status, TOPIC_FAN, "3");
            break;
          case 6:
            output_P(status, TOPIC_FAN, "4");
            break;
          case 7:
            output_P(status, TOPIC_FAN, PAYLOAD_FAN_AUTO);
            break;
          default:  // invalid values
            itoa(value, strtmp, 10);
            strcat(strtmp, "?");
            output_P(status, TOPIC_FAN, strtmp);
            break;
        }
#endif
        break;
      case status_vanes:
        webVanes = value;
#ifdef TELNET_DEBUG
        Debug.printf("webVanes:%d\n", webVanes);
#endif
        if (value < 6) {
#ifdef TELNET_DEBUG
          Debug.printf("webVanesTab[webVanes]:%s\n", webVanesTab[webVanes]);
#endif
          output_P(status, PSTR(TOPIC_VANES), webVanesTab[webVanes]);
        }

#if 0       
        switch (value) {
          case vanes_unknown:
            output_P(status, PSTR(TOPIC_VANES), PSTR(PAYLOAD_VANES_UNKNOWN));
            break;
          case vanes_swing:
            output_P(status, PSTR(TOPIC_VANES), PSTR(PAYLOAD_VANES_SWING));
            break;
          default:
            itoa(value, strtmp, 10);
            output_P(status, PSTR(TOPIC_VANES), strtmp);
        }
#endif
        break;
      case status_troom:
        {
          int8_t troom_diff = value - status_troom_old;        // avoid using other functions inside the brackets of abs, see https://www.arduino.cc/reference/en/language/functions/math/abs/
          if (abs(troom_diff) > TROOM_FILTER_LIMIT / 0.25f) {  // Room temperature delta > 0.25°C
            status_troom_old = value;
            webRoomTemp = float(value - 61) / 4.0;
            dtostrf(webRoomTemp, 0, 2, strtmp);
            output_P(status, PSTR(TOPIC_TROOM), strtmp);
          }
        }
        break;
      case status_tsetpoint:
        webTsetpoint = (value & 0x7f) / 2;
Debug.printf("case status_tsetpoint:");goto lab1;
#ifdef ENHANCED_RESOLUTION
        tmp_value = (value & 0x7f) / 2.0;
        offset = round(tmp_value) - tmp_value;  // Calculate offset when setpoint is changed
        mhi_ac_ctrl_core.set_troom_offset(offset);
#endif
      case opdata_tsetpoint:
Debug.printf("case opdata_tsetpoint:");goto lab1;
      case erropdata_tsetpoint:
Debug.printf("case erropdata_tsetpoint:");

lab1:Debug.printf(" webTsetpoint=%d\n",webTsetpoint);
        sprintf(strtmp, "%d", (value & 0x7f) / 2);
        //dtostrf((value & 0x7f) / 2.0, 0, 1, strtmp);
        output_P(status, PSTR(TOPIC_TSETPOINT), strtmp);
        break;
      case status_errorcode:
      case erropdata_errorcode:
        itoa(value, strtmp, 10);
        output_P(status, PSTR(TOPIC_ERRORCODE), strtmp);
        break;
      case opdata_return_air:
      case erropdata_return_air:
        dtostrf((value - 61) / 4.0, 0, 2, strtmp);
        output_P(status, PSTR(TOPIC_RETURNAIR), strtmp);
        break;
      case opdata_thi_r1:
      case erropdata_thi_r1:
        itoa(0.327f * value - 11.4f, strtmp, 10);  // only rough approximation
        output_P(status, PSTR(TOPIC_THI_R1), strtmp);
        break;
      case opdata_thi_r2:
      case erropdata_thi_r2:
        itoa(0.327f * value - 11.4f, strtmp, 10);  // formula for calculation not known
        output_P(status, PSTR(TOPIC_THI_R2), strtmp);
        break;
      case opdata_thi_r3:
      case erropdata_thi_r3:
        itoa(0.327f * value - 11.4f, strtmp, 10);  // only rough approximation
        output_P(status, PSTR(TOPIC_THI_R3), strtmp);
        break;
      case opdata_iu_fanspeed:
      case erropdata_iu_fanspeed:
        itoa(value, strtmp, 10);
        output_P(status, PSTR(TOPIC_IU_FANSPEED), strtmp);
        break;
      case opdata_total_iu_run:
      case erropdata_total_iu_run:
        itoa(value * 100, strtmp, 10);
        output_P(status, PSTR(TOPIC_TOTAL_IU_RUN), strtmp);
        break;
      case erropdata_outdoor:
      case opdata_outdoor:
        webRoomTempExt = (value - 94) * 0.25f;
        dtostrf(webRoomTempExt, 0, 2, strtmp);
        output_P(status, PSTR(TOPIC_OUTDOOR), strtmp);
        break;
      case opdata_tho_r1:
      case erropdata_tho_r1:
        itoa(0.327f * value - 11.4f, strtmp, 10);  // formula for calculation not known
        output_P(status, PSTR(TOPIC_THO_R1), strtmp);
        break;
      case opdata_comp:
      case erropdata_comp:
        dtostrf(highByte(value) * 25.6f + 0.1f * lowByte(value), 0, 2, strtmp);  // to be confirmed
        output_P(status, PSTR(TOPIC_COMP), strtmp);
        break;
      case erropdata_td:
      case opdata_td:
        if (value < 0x12)
          strcpy(strtmp, "<=30");
        else
          itoa(value / 2 + 32, strtmp, 10);
        output_P(status, PSTR(TOPIC_TD), strtmp);
        break;
      case opdata_ct:
      case erropdata_ct:
        dtostrf(value * 14 / 51.0f, 0, 2, strtmp);
        output_P(status, PSTR(TOPIC_CT), strtmp);
        break;
      case opdata_tdsh:
        itoa(value, strtmp, 10);  // formula for calculation not known
        output_P(status, PSTR(TOPIC_TDSH), strtmp);
        break;
      case opdata_protection_no:
        itoa(value, strtmp, 10);
        output_P(status, PSTR(TOPIC_PROTECTION_NO), strtmp);
        break;
      case opdata_ou_fanspeed:
      case erropdata_ou_fanspeed:
        itoa(value, strtmp, 10);
        output_P(status, PSTR(TOPIC_OU_FANSPEED), strtmp);
        break;
      case opdata_defrost:
        if (value)
          output_P(status, PSTR(TOPIC_DEFROST), PSTR(PAYLOAD_OP_DEFROST_ON));
        else
          output_P(status, PSTR(TOPIC_DEFROST), PSTR(PAYLOAD_OP_DEFROST_OFF));
        break;
      case opdata_total_comp_run:
      case erropdata_total_comp_run:
        itoa(value * 100, strtmp, 10);
        output_P(status, PSTR(TOPIC_TOTAL_COMP_RUN), strtmp);
        break;
      case opdata_ou_eev1:
      case erropdata_ou_eev1:
        itoa(value, strtmp, 10);
        output_P(status, PSTR(TOPIC_OU_EEV1), strtmp);
        break;
    }
  }
};


byte ledValue = 0;

unsigned long int myTime = 0;

/*********************************************** loop0 : tache core 1 ************************************/
StatusHandler mhiStatusHandler;
int mhi_ac_ctrl_core_error = 0;
int mhi_ac_ctrl_core_count = 0;
struct StabError {
  int err;
  int time;
} tabError[100];
unsigned long int timeGood = 0;

void setupMHI() {
#if TEMP_MEASURE_PERIOD > 0
  setup_ds18x20();
#endif
  MeasureFrequency();
  mhi_ac_ctrl_core.MHIAcCtrlStatus(&mhiStatusHandler);
  mhi_ac_ctrl_core.init();
  // mhi_ac_ctrl_core.set_fan(7); // set fan AUTO, see https://github.com/absalom-muc/MHI-AC-Ctrl/issues/99
}
extern char debug[60];

// publishes captured MOSI frames as 40 hex characters on <debug>/trame
static void publishTraceFrames() {
  byte frame[20];
  char hex[41];
  char topic[70];
  snprintf(topic, sizeof(topic), "%s/trame", debug);
  for (byte n = 0; n < 4 && mhi_ac_ctrl_core.pop_trace(frame); n++) {
    for (byte i = 0; i < 20; i++)
      sprintf(&hex[i * 2], "%02x", frame[i]);
    MQTTclient.publish(topic, hex, false);
  }
}

void setTrace(bool enable) {
  mhi_ac_ctrl_core.set_trace(enable);
}

uint32_t traceDropped() {
  return mhi_ac_ctrl_core.get_trace_dropped();
}

size_t getTraceData(char *output, size_t capacity) {
  const byte maxFrames = 64;  // must match MHI_AC_Ctrl_Core::TRACE_HISTORY_SIZE
  static byte frames[maxFrames][20];  // static: too large for the ESP8266 stack
  static byte misoFrames[maxFrames][20];
  byte count = mhi_ac_ctrl_core.copy_trace_history(frames, maxFrames, misoFrames);
  int written = snprintf(output, capacity, "{\"enabled\":%s,\"dropped\":%u,\"frames\":[",
                         mhi_ac_ctrl_core.is_trace_enabled() ? "true" : "false",
                         (unsigned)mhi_ac_ctrl_core.get_trace_dropped());
  if (written < 0 || (size_t)written >= capacity) {
    output[0] = '\0';
    return 0;
  }

  size_t used = written;
  for (byte pass = 0; pass < 2; pass++) {  // pass 0: frames received from the AC, pass 1: frames sent by the board
    byte (*src)[20] = pass ? misoFrames : frames;
    if (pass) {
      written = snprintf(output + used, capacity - used, "],\"miso\":[");
      if (written < 0 || (size_t)written >= capacity - used) {
        output[0] = '\0';
        return 0;
      }
      used += written;
    }
    for (byte n = 0; n < count; n++) {
      written = snprintf(output + used, capacity - used, "%s\"", n ? "," : "");
      if (written < 0 || (size_t)written >= capacity - used) {
        output[0] = '\0';
        return 0;
      }
      used += written;
      for (byte i = 0; i < 20; i++) {
        written = snprintf(output + used, capacity - used, "%02x", src[n][i]);
        if (written < 0 || (size_t)written >= capacity - used) {
          output[0] = '\0';
          return 0;
        }
        used += written;
      }
      written = snprintf(output + used, capacity - used, "\"");
      if (written < 0 || (size_t)written >= capacity - used) {
        output[0] = '\0';
        return 0;
      }
      used += written;
    }
  }
  written = snprintf(output + used, capacity - used, "]}");
  if (written < 0 || (size_t)written >= capacity - used) {
    output[0] = '\0';
    return 0;
  }
  return used + written;
}

void loopMHI() {

  static byte ds18x20_value_old = 0;
  static unsigned long previousMillis = millis();
  unsigned long int myTmpTime = millis();

#if TEMP_MEASURE_PERIOD > 0
  byte ds18x20_value = getDs18x20Temperature(25);
#ifdef ENHANCED_RESOLUTION
  float offset = mhi_ac_ctrl_core.get_troom_offset();
  byte tmp = offset * 4;
  ds18x20_value = ds18x20_value + tmp;  // add offset
#endif

#ifdef ROOM_TEMP_DS18X20
  if (ds18x20_value != ds18x20_value_old) {
    if ((ds18x20_value > 21) & (ds18x20_value < 253)) {  // use only values -10°C < T < 48°C
      mhi_ac_ctrl_core.set_troom(ds18x20_value);
      ds18x20_value_old = ds18x20_value;
#ifdef SERIAL_DEBUG
      Serial.printf("update Troom based on DS18x20 value %i\n", ds18x20_value);
#endif
#ifdef TELNET_DEBUG
      Debug.printf("update Troom based on DS18x20 value %i\n", ds18x20_value);
#endif
    }
  }
#endif
#endif

  // fallback to AC internal Troom temperature sensor
  if (troom_was_set_by_MQTT & (millis() - room_temp_set_timeout_Millis >= ROOM_TEMP_MQTT_SET_TIMEOUT * 1000)) {
    mhi_ac_ctrl_core.set_troom(0xff);  // use IU temperature sensor
#ifdef SERIAL_DEBUG
    Serial.println(F("ROOM_TEMP_MQTT_SET_TIMEOUT exceeded, use IU temperature sensor value!"));
#endif
#ifdef TELNET_DEBUG
    Debug.println(F("ROOM_TEMP_MQTT_SET_TIMEOUT exceeded, use IU temperature sensor value!"));
#endif
    troom_was_set_by_MQTT = false;
  }
  int ret = mhi_ac_ctrl_core.loop(80);
  publishTraceFrames();
  if (ret < 0) {
    //    tabError[mhi_ac_ctrl_core_error].err=ret;
    mhi_ac_ctrl_core_error++;
  } else
    timeGood += millis() - myTmpTime;
  mhi_ac_ctrl_core_count++;

  if ((myTmpTime - myTime) > 10000) {
    int core = 1;
    myTime = myTmpTime;
    if (mhi_ac_ctrl_core_count - mhi_ac_ctrl_core_error)
      timeGood /= (mhi_ac_ctrl_core_count - mhi_ac_ctrl_core_error);
    else
      timeGood = -1;
    if (mhi_ac_ctrl_core_count)
      TauxDerreurs = ((float)mhi_ac_ctrl_core_error * 100.0) / (float)mhi_ac_ctrl_core_count;
#ifdef SERIAL_DEBUG
    Serial.printf_P(PSTR("core:%d,timeGood:%d,Taux erreurs:%.01f%%, mhi_ac_ctrl_core_error: %d / %d\n"), core, timeGood, TauxDerreurs,
                    mhi_ac_ctrl_core_error, mhi_ac_ctrl_core_count);
    Serial.println();
#endif
#ifdef TELNET_DEBUG
    Debug.printf_P(PSTR("core:%d,timeGood:%d,Taux erreurs:%.01f%%, mhi_ac_ctrl_core_error: %d / %d\n"), core, timeGood, TauxDerreurs,
                   mhi_ac_ctrl_core_error, mhi_ac_ctrl_core_count);
    Debug.println();
#endif
    mhi_ac_ctrl_core_error = 0;
    mhi_ac_ctrl_core_count = 0;
    timeGood = 0;
  }
}
