#pragma once
#include "AbstractPage.h"

class SecurityStatusGet : public AbstractPage {
public:
  explicit SecurityStatusGet(httpd_handle_t server) : AbstractPage(server, HTTP_GET, "/api/status/security") {}

protected:
  bool requiresAuth() override { return true; }

public:
  esp_err_t handler(httpd_req_t *req) override {
    bool must = false;
    if (auth) {
      must = auth->mustChangePassword();
    }
    httpd_resp_set_type(req, "application/json");
    String body = String("{\"mustChangePassword\":") + (must ? "true" : "false") + "}";
    httpd_resp_sendstr(req, body.c_str());
    return ESP_OK;
  }
};
