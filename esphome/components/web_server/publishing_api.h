#pragma once

#include "esphome/core/defines.h"

#if defined(USE_WEBSERVER_PUBLISHING_API) && defined(USE_ESP_IDF)

#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/core/component.h"
#include "esphome/core/entity_settings.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <functional>
#include <string>

extern const uint8_t ESPHOME_WEBSERVER_PUBLISHING_JS[] PROGMEM;
extern const size_t ESPHOME_WEBSERVER_PUBLISHING_JS_SIZE;

namespace esphome::web_server {

class WebServerPublishingApi : public Component, public AsyncWebHandler {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  void set_web_server_base(web_server_base::WebServerBase *base) { this->base_ = base; }

  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;
  bool isRequestHandlerTrivial() const override { return false; }

 protected:
  static constexpr uint16_t PAGE = 32;

  void run_on_loop_(std::function<void()> &&op);
  void list_(AsyncWebServerRequest *request, std::string &json);
  bool command_(const char *op, AsyncWebServerRequest *request);

  web_server_base::WebServerBase *base_{nullptr};
};

}

#endif
