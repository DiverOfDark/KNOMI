#pragma once
#include "AbstractPage.h"

class ApiUploadFileDelete : public AbstractPage {
private:
  UpdateProgress *updateProgress;

public:
  explicit ApiUploadFileDelete(httpd_handle_t server, UpdateProgress *progress)
      : AbstractPage(server, HTTP_DELETE, "/api/uploadFile") {
    this->updateProgress = progress;
  }

protected:
  bool requiresAuth() override { return true; }

  static bool isValidFilename(const String &name) {
    if (name.length() == 0)
      return false;
    if (name.startsWith("/"))
      return false;
    if (name.indexOf("..") >= 0)
      return false;
    if (name.indexOf('\n') >= 0 || name.indexOf('\r') >= 0 || name.indexOf('\0') >= 0)
      return false;
    return true;
  }

public:
  esp_err_t handler(httpd_req_t *req) override {
    String filename;

    if (!streamReadMultipart(req, [&](const String &formData, const String &fn) {
          if (formData.equals("filename")) {
            return readString(&filename);
          }
          return static_cast<ReadCallback>(nullptr);
        })) {
      return ESP_OK;
    }

    if (!isValidFilename(filename)) {
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "INVALID_FILENAME");
      return ESP_OK;
    }

    if (isProtectedFsPath(filename)) {
      httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "FORBIDDEN");
      return ESP_OK;
    }

    updateProgress->total = 1;
    updateProgress->current = 0;
    updateProgress->isInProgress = true;
    updateProgress->waitForCanWrite();
    bool result = LittleFS.remove("/" + filename);
    updateProgress->isInProgress = false;
    updateProgress->total = 0;
    updateProgress->current = 0;

    httpd_resp_set_type(req, "text/plain");
    if (result)
      httpd_resp_sendstr(req, "OK");
    else
      httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "NOT_FOUND");

    return ESP_OK;
  }
};