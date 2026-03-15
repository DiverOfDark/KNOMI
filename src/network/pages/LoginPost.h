#pragma once
#include "../Auth.h"
#include "AbstractPage.h"

class LoginPost : public AbstractPage {
public:
  explicit LoginPost(httpd_handle_t server) : AbstractPage(server, HTTP_POST, "/api/login") {}

protected:
  bool requiresAuth() override { return false; }

  esp_err_t handler(httpd_req_t *req) override {
    String password;
    // Accept either multipart field named "password" or a raw body (text/plain or application/json with {password})
    String contentType = getHeader(req, "Content-Type");
    if (contentType.startsWith("multipart/form-data")) {
      bool ok = readForm(req, "password", [&password](const char *buf, uint size, uint offset) {
        // Append exactly 'size' bytes starting at 'offset' without temporary String allocations
        password.concat(buf + offset, size);
      });
      if (!ok) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        httpd_resp_sendstr(req, "{\"error\":\"invalid form\"}");
        return ESP_OK;
      }
    } else {
      // read entire body
      String body;
      char buf[256];
      int r;
      while ((r = httpd_req_recv(req, buf, sizeof(buf))) > 0) {
        body.concat(String(buf).substring(0, r));
        if (r < (int)sizeof(buf))
          break;
      }
      // naive parse: if JSON, look for "password":"..." else treat as raw password
      int p = body.indexOf("\"password\"");
      if (p >= 0) {
        int c = body.indexOf('"', body.indexOf(':', p) + 1);
        int e = body.indexOf('"', c + 1);
        if (c >= 0 && e > c)
          password = body.substring(c + 1, e);
      } else {
        password = body;
        password.trim();
      }
    }

    if (!auth || !auth->verifyPassword(password)) {
      httpd_resp_set_status(req, "401 Unauthorized");
      httpd_resp_set_hdr(req, "Cache-Control", "no-store");
      httpd_resp_set_type(req, "application/json");
      httpd_resp_sendstr(req, "{\"error\":\"invalid_credentials\"}");
      return ESP_OK;
    }

    String token = auth->issueSessionToken();
    String cookie = String("SID=") + token + "; HttpOnly; SameSite=Strict; Path=/";
    // Add Secure when proxied over HTTPS (simple heuristic)
    String xfproto = getHeader(req, "X-Forwarded-Proto");
    if (xfproto.equalsIgnoreCase("https")) {
      cookie += "; Secure";
    }
    httpd_resp_set_hdr(req, "Set-Cookie", cookie.c_str());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
  }
};
