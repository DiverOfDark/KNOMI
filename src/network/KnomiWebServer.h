#pragma once
#include "../config/Config.h"
#include "../generated/images.h"
#include "Arduino.h"
#include "WifiManager.h"
#include "log.h"
#include <esp_http_server.h>
#include <esp_ota_ops.h>

#include "Auth.h"
#include "UpdateProgress.h"
#include "pages/ApiConfigWifiPost.h"
#include "pages/ApiCoreDumpGet.h"
#include "pages/ApiDumpHeapGet.h"
#include "pages/ApiListFilesGet.h"
#include "pages/ApiRestartPageGet.h"
#include "pages/ApiScanNetworks.h"
#include "pages/ApiStatusGet.h"
#include "pages/ApiThemeConfigPost.h"
#include "pages/ApiUploadFileDelete.h"
#include "pages/ApiUploadFilePost.h"
#include "pages/ChangePasswordPost.h"
#include "pages/LoginPost.h"
#include "pages/LogoutPost.h"
#include "pages/RootPage.h"
#include "pages/SecurityStatusGet.h"
#include "pages/StaticFileContentGet.h"
#include "pages/UpdatePost.h"
#include "pages/WebsocketLog.h"

class KnomiWebServer {
private:
  httpd_handle_t server = nullptr;
  WifiManager *manager = nullptr;

  Config *config = nullptr;

  UpdateProgress *progress = nullptr;

  AuthManager *auth = nullptr;

  RootPage *rootPage = nullptr;
  LoginPost *loginPost = nullptr;
  LogoutPost *logoutPost = nullptr;
  ApiRestartPageGet *apiRestartPageGet = nullptr;
  ApiScanNetworks *apiScanNetworks = nullptr;
  ApiDumpHeapGet *apiDumpHeapGet = nullptr;
  ApiListFilesGet *apiListFilesGet = nullptr;
  ApiStatusGet *apiStatusGet = nullptr;
  SecurityStatusGet *securityStatusGet = nullptr;
  ApiCoreDumpGet *apiCoreDumpGet = nullptr;
  ApiUploadFileDelete *apiUploadFileDelete = nullptr;
  ApiUploadFilePost *apiUploadFilePost = nullptr;
  ApiConfigWifiPost *apiConfigWifiPost = nullptr;
  ApiThemeConfigPost *apiThemeConfigPost = nullptr;
  ChangePasswordPost *changePasswordPost = nullptr;
  UpdatePost *updatePost = nullptr;
  StaticFileContentGet *staticFileContentGet = nullptr;

  WebsocketLog *websocketPage = nullptr;

public:
  bool started = false;

  KnomiWebServer(Config *config, WifiManager *manager, UpdateProgress *progress) {
    this->config = config;
    this->manager = manager;
    this->progress = progress;
  }

  ~KnomiWebServer() {
    delete loginPost;
    delete logoutPost;
    delete rootPage;
    delete apiRestartPageGet;
    delete apiScanNetworks;
    delete apiDumpHeapGet;
    delete apiListFilesGet;
    delete apiStatusGet;
    delete securityStatusGet;
    delete apiCoreDumpGet;
    delete apiUploadFileDelete;
    delete apiUploadFilePost;
    delete apiConfigWifiPost;
    delete apiThemeConfigPost;
    delete changePasswordPost;
    delete updatePost;
    delete staticFileContentGet;

    delete websocketPage;
    httpd_stop(server);
  }

  void registerNotFound() { httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, notFoundStaticCode); }
  static esp_err_t notFoundStaticCode(httpd_req_t *req, httpd_err_code_t error) {
    LV_LOG_INFO("Failed to find page for %s", req->uri);
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_sendstr(req, "");
    return ESP_OK;
  }

  void tick() {
    if (!this->started) {
      httpd_config_t httpdConfig = HTTPD_DEFAULT_CONFIG();
      httpdConfig.lru_purge_enable = true;
      httpdConfig.uri_match_fn = httpd_uri_match_wildcard;
      httpdConfig.stack_size = 8192;
      httpdConfig.max_uri_handlers = 32;

      LV_LOG_INFO("Starting server on port: '%d'", httpdConfig.server_port);
      if (httpd_start(&server, &httpdConfig) == ESP_OK) {
        this->auth = new AuthManager(config);
        // Set URI handlers
        LV_LOG_INFO("Registering URI handlers");
        this->rootPage = new RootPage(server);
        this->loginPost = new LoginPost(server);
        this->logoutPost = new LogoutPost(server);
        this->apiRestartPageGet = new ApiRestartPageGet(server);
        this->apiScanNetworks = new ApiScanNetworks(server, manager);
        this->apiDumpHeapGet = new ApiDumpHeapGet(server);
        this->apiListFilesGet = new ApiListFilesGet(server);
        this->apiStatusGet = new ApiStatusGet(config, server);
        this->securityStatusGet = new SecurityStatusGet(server);
        this->apiCoreDumpGet = new ApiCoreDumpGet(server);
        this->apiThemeConfigPost = new ApiThemeConfigPost(server, config);
        this->apiUploadFileDelete = new ApiUploadFileDelete(server, progress);
        this->apiUploadFilePost = new ApiUploadFilePost(server, progress);
        this->apiConfigWifiPost = new ApiConfigWifiPost(manager, server, config);
        this->changePasswordPost = new ChangePasswordPost(server, config);
        this->updatePost = new UpdatePost(server, progress);
        this->staticFileContentGet = new StaticFileContentGet(server);

        this->websocketPage = new WebsocketLog(server);
        // Set auth on relevant pages
        rootPage->setAuth(auth); // not required but harmless
        loginPost->setAuth(auth);
        logoutPost->setAuth(auth);
        apiRestartPageGet->setAuth(auth);
        apiScanNetworks->setAuth(auth);
        apiDumpHeapGet->setAuth(auth);
        apiListFilesGet->setAuth(auth);
        apiStatusGet->setAuth(auth);
        securityStatusGet->setAuth(auth);
        apiCoreDumpGet->setAuth(auth);
        apiThemeConfigPost->setAuth(auth);
        apiUploadFileDelete->setAuth(auth);
        apiUploadFilePost->setAuth(auth);
        apiConfigWifiPost->setAuth(auth);
        changePasswordPost->setAuth(auth);
        updatePost->setAuth(auth);
        staticFileContentGet->setAuth(auth);
        websocketPage->setAuth(auth);
        registerNotFound();
        this->started = true;
      }

      LV_LOG_INFO("WebServer started!");
    }
  }

  void websocketLog(const char *logString) {
    if (this->websocketPage != nullptr) {
      this->websocketPage->textAll(logString);
    }
  }
};