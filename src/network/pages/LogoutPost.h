#pragma once
#include "../Auth.h"
#include "AbstractPage.h"

class LogoutPost : public AbstractPage {
public:
  explicit LogoutPost(httpd_handle_t server) : AbstractPage(server, HTTP_POST, "/api/logout") {}

protected:
  bool requiresAuth() override { return true; }

  esp_err_t handler(httpd_req_t *req) override {
    if (auth)
      auth->clearSession();
    httpd_resp_set_hdr(req, "Set-Cookie", "SID=; Max-Age=0; Path=/; HttpOnly; SameSite=Strict");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
  }
};
