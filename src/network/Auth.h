#pragma once
#include "../config/Config.h"
#include <Arduino.h>
#include <esp_system.h>

class Config;

class AuthManager {
private:
  Config *config;
  String currentToken;

  static String randomHex(size_t len = 32) {
    String out;
    out.reserve(len);
    for (size_t i = 0; i < len; i++) {
      uint8_t v = static_cast<uint8_t>(esp_random() & 0xFF);
      uint8_t nybble = (i % 2 == 0) ? (v >> 4) : (v & 0x0F);
      char c = nybble < 10 ? ('0' + nybble) : ('a' + (nybble - 10));
      out += c;
    }
    return out;
  }

public:
  explicit AuthManager(Config *cfg) : config(cfg) {}

  bool verifyPassword(const String &password) const {
    if (config == nullptr)
      return false;
    const String &stored = config->getAdminPassword();
    return stored.equalsConstantTime(password);
  }

  String issueSessionToken() {
    currentToken = randomHex(32);
    return currentToken;
  }

  void clearSession() { currentToken = ""; }

  bool isAuthorizedCookie(const String &cookieHeader) const {
    if (currentToken.isEmpty())
      return false;
    if (cookieHeader.length() == 0)
      return false;
    // simple parser for SID=<token>
    String target = String("SID=") + currentToken;
    return cookieHeader.indexOf(target) >= 0;
  }

  bool isAuthorizedHeader(const String &authHeader) const {
    if (currentToken.isEmpty())
      return false;
    if (!authHeader.startsWith("Bearer "))
      return false;
    String token = authHeader.substring(7);
    token.trim();
    return token == currentToken;
  }

  bool mustChangePassword() const {
    if (config == nullptr)
      return false;
    // Default password requires user to change it
    return config->getAdminPassword() == String("KNOMI");
  }
};
