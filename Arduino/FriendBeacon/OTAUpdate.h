#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <ArduinoOTA.h>
#include "Config.h"

class OTAUpdate {
private:
  bool enabled;
  bool updateInProgress;

public:
  OTAUpdate() : enabled(OTA_ENABLED), updateInProgress(false) {}

  void begin(String hostname = "FriendBeacon") {
    if (!enabled) return;

    ArduinoOTA.setHostname(hostname.c_str());

#ifdef OTA_PASSWORD
    ArduinoOTA.setPassword(OTA_PASSWORD);
#endif

    ArduinoOTA.onStart([this]() {
      updateInProgress = true;
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { // U_SPIFFS
        type = "filesystem";
      }
      LOG_INFO("OTA Update started: " + type);
    });

    ArduinoOTA.onEnd([this]() {
      updateInProgress = false;
      LOG_INFO("OTA Update completed");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      static unsigned int lastPercent = 0;
      unsigned int percent = (progress / (total / 100));

      if (percent != lastPercent && percent % 10 == 0) {
        LOG_INFO("OTA Progress: " + String(percent) + "%");
        lastPercent = percent;
      }
    });

    ArduinoOTA.onError([this](ota_error_t error) {
      updateInProgress = false;
      String errorMsg = "OTA Error[" + String(error) + "]: ";

      if (error == OTA_AUTH_ERROR) errorMsg += "Auth Failed";
      else if (error == OTA_BEGIN_ERROR) errorMsg += "Begin Failed";
      else if (error == OTA_CONNECT_ERROR) errorMsg += "Connect Failed";
      else if (error == OTA_RECEIVE_ERROR) errorMsg += "Receive Failed";
      else if (error == OTA_END_ERROR) errorMsg += "End Failed";

      LOG_ERROR(errorMsg);
    });

    ArduinoOTA.begin();
    LOG_INFO("OTA Update enabled. Hostname: " + hostname);
  }

  void handle() {
    if (enabled) {
      ArduinoOTA.handle();
    }
  }

  bool isUpdateInProgress() {
    return updateInProgress;
  }

  void setEnabled(bool enable) {
    enabled = enable;
  }
};

#endif // OTA_UPDATE_H
