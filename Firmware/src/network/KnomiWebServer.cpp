//
// Created by g33k on 19.08.2023.
//

#include "KnomiWebServer.h"

extern void connectToWiFi();

KnomiWebServer::KnomiWebServer(WifiConfig* config) {
  server = new WebServer(webPort);
  wificonfig = config;

  server->on("/", HTTP_GET, [&](){ handleRoot(); });
  server->on("/configwifi", HTTP_POST, [&](){ handleConfigWifi(); });

  server->onNotFound([&](){ handleNotFound(); });
  String shortSha = Version::getGitCommitSha1().substring(0, 8);
  String timestamp = Version::getBuildTimestamp();
  String id = shortSha + " - " + timestamp;
  ElegantOTA.setID(id.c_str());
  ElegantOTA.begin(server);
  server->begin();

  LV_LOG_INFO("WebServer started!");
}

#define ROOT_HTML_OK                                                           \
  "<!DOCTYPE html><html><head><title>WIFI SET</title><meta name=\"viewport\" " \
  "content=\"width=device-width, initial-scale=1\"></head><style "             \
  "type=\"text/css\">.c,body {text-align: center}</style><body><form "         \
  "method=\"POST\" action=\"configwifi\"></label><p><span> Submission "        \
  "successful!</P><p><span> You may now close this page.</P> </form>"

#define ROOT_HTML                                                              \
  "<!DOCTYPE html><html><head><title>WIFI</title><meta name=\"viewport\" "     \
  "content=\"width=device-width, initial-scale=1\"></head><style "             \
  "type=\"text/css\">.input{display: block; margin-top: 10px;}.input "         \
  "span{width: 100px; float: left; float: left; height: 36px; line-height: "   \
  "36px;}.input input{height: 30px;width: 200px;}.btn{width: 110px; height: "  \
  "50px; background-color: #438EF0; border-radius:8px; "                       \
  "font-size:20px;color:#ffffff;border-color: #438EF0; margin-top:16px; "      \
  "margin-left:104px;}</style><body><form method=\"POST\" "                    \
  "action=\"configwifi\"><p><span> KNOMI :-)</P><label "                       \
  "class=\"input\"><span>WiFi SSID</span><input type=\"text\" name=\"ssid\" "  \
  "value=\"\"></label><label class=\"input\"><span>WiFi PASS</span> <input "   \
  "type=\"text\"  name=\"pass\"><label class=\"input\"><span>Klipper "         \
  "IP</span> <input type=\"text\"  name=\"klipper\"></label><input "           \
  "class=\"btn\" type=\"submit\" name=\"submit\" value=\"SUBMIT\"> </form>"

void KnomiWebServer::handleRoot() { server->send(200, "text/html", ROOT_HTML); }

void KnomiWebServer::handleConfigWifi() // 返回http状态
{
  if (server->hasArg("ssid")) // 判断是否有账号参数
  {
    LV_LOG_INFO("got ssid:");
    String wifi_ssid = server->arg("ssid"); // 获取html表单输入框name名为"ssid"的内容

    wificonfig->setSSID(wifi_ssid);
    LV_LOG_INFO(wifi_ssid.c_str());
  } else // 没有参数
  {
    LV_LOG_INFO("error, not found ssid");
    server->send(200, "text/html",
                "<meta charset='UTF-8'>error, not found ssid"); // 返回错误页面
    return;
  }
  // 密码与账号同理
  if (server->hasArg("pass")) {
    LV_LOG_INFO("got password:");
    String wifi_pass = server->arg("pass"); // 获取html表单输入框name名为"pwd"的内容

    wificonfig->setPassword(wifi_pass);
    LV_LOG_INFO(wifi_pass.c_str());
  } else {
    LV_LOG_INFO("error, not found password");
    server->send(200, "text/html",
                "<meta charset='UTF-8'>error, not found password");
    return;
  }
  // klipper ip
  if (server->hasArg("klipper")) {
    LV_LOG_INFO("got KlipperIP:");
    String klipper_ip = server->arg("klipper"); // 获取html表单输入框name名为"KlipperIP"的内容

    wificonfig->setKlipperIp(klipper_ip);
    wificonfig->setAccessPointMode('8');

    LV_LOG_INFO(klipper_ip.c_str());
  } else {
    LV_LOG_INFO("error, not found klipper ip");
    server->send(200, "text/html",
                "<meta charset='UTF-8'>error, not found klipper ip");
    return;
  }
  delay(200);

  // server->send(200, "text/html", "<meta charset='UTF-8'>SSID：" + wifi_ssid +
  // "<br />password:" + wifi_pass + "<br />Trying to connect Trying to connect,
  // please manually close this page."); //返回保存成功页面
  server->send(200, "text/html", ROOT_HTML_OK); // 返回保存成功页面
  delay(2000);
  WiFi.softAPdisconnect(
      true); // 参数设置为true，设备将直接关闭接入点模式，即关闭设备所建立的WiFi网络。
  server->close();          // 关闭web服务
  WiFi.softAPdisconnect(); // 在不输入参数的情况下调用该函数,将关闭接入点模式,并将当前配置的AP热点网络名和密码设置为空值.
  LV_LOG_INFO(("WiFi Connect SSID:" + wificonfig->getSSID() + "  PASS:" + wificonfig->getPassword()).c_str());

  if (WiFi.status() != WL_CONNECTED) // wifi没有连接成功
  {
    LV_LOG_INFO("开始调用连接函数connectToWiFi()..");
    connectToWiFi();
  } else {
    LV_LOG_INFO("提交的配置信息自动连接成功..");
  }
}