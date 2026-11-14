#include "publishing_api.h"

#if defined(USE_WEBSERVER_PUBLISHING_API) && defined(USE_ESP_IDF)

#include "esphome/components/json/json_util.h"

#include <atomic>
#include <cstdlib>
#include <cstring>

namespace esphome::web_server {

static const char *const API_PREFIX = "/api/publishing/";
static const char *const CARD_URL = "/publishing/card.js";

void WebServerPublishingApi::setup() { this->base_->add_handler(this); }

void WebServerPublishingApi::run_on_loop_(std::function<void()> &&op) {
  TaskHandle_t waiter = xTaskGetCurrentTaskHandle();
  std::atomic<bool> done{false};
  this->defer([op = std::move(op), waiter, &done]() {
    op();
    done.store(true);
    xTaskNotifyGive(waiter);
  });
  while (!done.load())
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

bool WebServerPublishingApi::canHandle(AsyncWebServerRequest *request) const {
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  StringRef url = request->url_to(url_buf);
  const auto method = request->method();
  if (method == HTTP_GET && url == CARD_URL)
    return true;
  if (strncmp(url.c_str(), API_PREFIX, strlen(API_PREFIX)) != 0)
    return false;
  return method == HTTP_GET || method == HTTP_POST;
}

static std::string param(AsyncWebServerRequest *request, const char *name) {
  auto *p = request->getParam(name);
  return p != nullptr ? std::string(p->value()) : std::string();
}

static bool has_param(AsyncWebServerRequest *request, const char *name) { return request->getParam(name) != nullptr; }

static uint32_t entity_key(const EntityBase *entity) {
#ifdef USE_DEVICES
  return entity->get_object_id_hash() ^ entity->get_device_id();
#else
  return entity->get_object_id_hash();
#endif
}

static int group_filter(const std::string &text) { return text.empty() ? -1 : atoi(text.c_str()); }

static EntityBase *find_entity(const std::string &kind, uint32_t key) {
  EntitySettings &settings = global_entity_settings;
  const uint16_t count = settings.entity_count(kind.c_str());
  for (uint16_t i = 0; i < count; i++) {
    EntityBase *entity = settings.entity_at(i, nullptr, kind.c_str());
    if (entity != nullptr && entity_key(entity) == key)
      return entity;
  }
  return nullptr;
}

void WebServerPublishingApi::list_(AsyncWebServerRequest *request, std::string &json) {
  EntitySettings &settings = global_entity_settings;
  const std::string kind_text = param(request, "kind");
  const char *kind = kind_text.empty() ? nullptr : kind_text.c_str();
  const int group = group_filter(param(request, "group"));
  const uint16_t offset = static_cast<uint16_t>(strtoul(param(request, "offset").c_str(), nullptr, 10));
  json::JsonBuilder builder;
  JsonObject root = builder.root();
  const uint16_t count = settings.entity_count(kind, group);
  root["revision"] = settings.revision();
  root["count"] = count;
  root["offset"] = offset;
  root["page"] = PAGE;
  JsonObject ways = root["ways"].to<JsonObject>();
#ifdef USE_API
  ways["api"] = ENTITY_API_OFF;
#endif
#ifdef USE_MQTT
  ways["mqtt"] = ENTITY_MQTT_OFF;
#endif
  ways["web"] = ENTITY_WEB_OFF;
  JsonArray kinds = root["kinds"].to<JsonArray>();
  for (uint8_t i = 0; EntitySettings::kind_at(i) != nullptr; i++) {
    if (settings.entity_count(EntitySettings::kind_at(i)) > 0)
      kinds.add(EntitySettings::kind_at(i));
  }
  JsonArray groups = root["groups"].to<JsonArray>();
  for (uint8_t i = 0; i < settings.group_count(); i++) {
    JsonObject g = groups.add<JsonObject>();
    g["id"] = settings.group_id(i);
    g["name"] = settings.group_name(i);
  }
  JsonArray entities = root["entities"].to<JsonArray>();
  for (uint16_t i = offset; i < count && i < offset + PAGE; i++) {
    const char *entity_kind = nullptr;
    EntityBase *entity = settings.entity_at(i, &entity_kind, kind, group);
    if (entity == nullptr)
      break;
    JsonObject e = entities.add<JsonObject>();
    e["key"] = entity_key(entity);
    e["kind"] = entity_kind;
    e["name"] = entity->get_name().c_str();
    e["off"] = settings.off(entity);
    e["default"] = settings.default_off(entity);
    e["group"] = settings.web_group(entity);
  }
  auto buf = builder.serialize();
  json.assign(buf.data(), buf.size());
}

bool WebServerPublishingApi::command_(const char *op, AsyncWebServerRequest *request) {
  EntitySettings &settings = global_entity_settings;
  const std::string kind = param(request, "kind");
  if (strcmp(op, "set") == 0 || (strcmp(op, "reset") == 0 && has_param(request, "key"))) {
    EntityBase *entity = find_entity(kind, static_cast<uint32_t>(strtoul(param(request, "key").c_str(), nullptr, 10)));
    if (entity == nullptr)
      return false;
    if (strcmp(op, "reset") == 0) {
      settings.reset(entity);
      return true;
    }
    if (has_param(request, "off"))
      settings.set_off(entity, static_cast<uint8_t>(strtoul(param(request, "off").c_str(), nullptr, 10)) & 7, false);
    if (has_param(request, "group")) {
      const int group = atoi(param(request, "group").c_str());
      if (group != EntitySettings::GROUP_DEFAULT && settings.group_index(static_cast<uint8_t>(group)) < 0)
        return false;
      settings.set_web_group(entity, static_cast<uint8_t>(group), false);
    }
    settings.save();
    return true;
  }
  if (strcmp(op, "all") == 0 || strcmp(op, "reset") == 0) {
    const bool reset = strcmp(op, "reset") == 0;
    const uint8_t way = static_cast<uint8_t>(strtoul(param(request, "way").c_str(), nullptr, 10)) & 7;
    const bool on = param(request, "on") != "0";
    if (!reset && way == 0)
      return false;
    const char *filter = kind.empty() ? nullptr : kind.c_str();
    const int group = group_filter(param(request, "group"));
    const uint16_t count = settings.entity_count(filter, group);
    for (uint16_t i = count; i > 0; i--) {
      EntityBase *entity = settings.entity_at(i - 1, nullptr, filter, group);
      if (entity == nullptr)
        continue;
      if (reset) {
        settings.reset(entity, false);
      } else {
        const uint8_t off = settings.off(entity);
        settings.set_off(entity, on ? off & ~way : off | way, false);
      }
    }
    settings.save();
    return true;
  }
  if (strcmp(op, "group") == 0) {
    const std::string action = param(request, "op");
    const std::string name = param(request, "name");
    if (action == "add")
      return !name.empty() && settings.add_group(name.c_str());
    const int index = settings.group_index(static_cast<uint8_t>(atoi(param(request, "id").c_str())));
    if (index < 0)
      return false;
    if (action == "rename" && !name.empty()) {
      settings.rename_group(static_cast<uint8_t>(index), name.c_str());
      return true;
    }
    if (action == "remove") {
      settings.remove_group(static_cast<uint8_t>(index));
      return true;
    }
    if (action == "move") {
      settings.move_group(static_cast<uint8_t>(index), static_cast<int8_t>(atoi(param(request, "by").c_str())));
      return true;
    }
  }
  return false;
}

void WebServerPublishingApi::handleRequest(AsyncWebServerRequest *request) {
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  StringRef url = request->url_to(url_buf);
  const bool is_get = request->method() == HTTP_GET;

  if (is_get && url == CARD_URL) {
    auto *response = request->beginResponse(200, "text/javascript", ESPHOME_WEBSERVER_PUBLISHING_JS,
                                            ESPHOME_WEBSERVER_PUBLISHING_JS_SIZE);
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "public, max-age=3600");
    request->send(response);
    return;
  }

  const char *op = url.c_str() + strlen(API_PREFIX);
  std::string json;
  if (is_get) {
    if (strcmp(op, "list") != 0) {
      request->send(404, "application/json", "{\"error\":\"unknown request\"}");
      return;
    }
    this->run_on_loop_([this, request, &json]() { this->list_(request, json); });
    request->send(200, "application/json", json.c_str());
    return;
  }

  this->run_on_loop_([this, op, request, &json]() {
    const bool ok = this->command_(op, request);
    json::JsonBuilder builder;
    JsonObject root = builder.root();
    root["success"] = ok;
    root["error"] = ok ? "" : "not changed: unknown request, entity or group";
    auto buf = builder.serialize();
    json.assign(buf.data(), buf.size());
  });
  request->send(200, "application/json", json.c_str());
}

}

#endif
