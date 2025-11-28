#pragma once
#include "../../config/Config.h"
#include "AbstractPage.h"

class ChangePasswordPost : public AbstractPage {
private:
  Config *config;

public:
  explicit ChangePasswordPost(httpd_handle_t server, Config *cfg)
      : AbstractPage(server, HTTP_POST, "/api/changePassword"), config(cfg) {}

protected:
  bool requiresAuth() override { return true; }

public:
  esp_err_t handler(httpd_req_t *req) override {
    String oldPassword;
    String newPassword;

    if (!streamReadMultipart(req, [&](const String &formData, const String &fn) {
          if (formData.equals("oldPassword")) {
            return readString(&oldPassword);
          }
          if (formData.equals("newPassword")) {
            return readString(&newPassword);
          }
          return (ReadCallback) nullptr;
        })) {
      return ESP_OK;
    }

    // Validate old password
    if (!auth || !auth->verifyPassword(oldPassword)) {
      httpd_resp_set_status(req, "401 Unauthorized");
      httpd_resp_set_type(req, "application/json");
      httpd_resp_sendstr(req, "{\"error\":\"invalid_old_password\"}");
      return ESP_OK;
    }

    // Policy: any ASCII allowed, but must not equal default
    if (newPassword.length() == 0 || newPassword == String("KNOMI")) {
      httpd_resp_set_status(req, "400 Bad Request");
      httpd_resp_set_type(req, "application/json");
      httpd_resp_sendstr(req, "{\"error\":\"invalid_new_password\"}");
      return ESP_OK;
    }

    // Save
    config->setAdminPassword(newPassword);
    config->save();

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
  }
};
