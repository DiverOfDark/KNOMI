#pragma once
#include <WiFi.h>
#include <esp_wifi.h>
#include <vector>

struct NetworkInfo {
  String networkName;
  bool isOpen;
  int rssiDbm; // RSSI in dBm
};

class WifiScanner {
private:
  bool scanInProgress = false;
  unsigned long lastScan = 0;
  std::vector<NetworkInfo> foundNetworks;
  static constexpr unsigned long intervalForRescanMs = 5000UL; // every 5 seconds

public:
  WifiScanner() = default;

  const std::vector<NetworkInfo> &scan() {
    LV_LOG_INFO("Scanning for networks");
    if (scanInProgress) {
      int foundNetworksCount = WiFi.scanComplete();
      LV_LOG_INFO("Scan already in progress, found count: %d", foundNetworksCount);
      if (foundNetworksCount >= 0) {
        scanInProgress = false;
        foundNetworks.clear();
        foundNetworks.reserve(foundNetworksCount);
        for (int i = 0; i < foundNetworksCount; i++) {
          String networkName = WiFi.SSID(i);
          int rssi = WiFi.RSSI(i);
          // clamp RSSI to a typical range [-100, -10] dBm for robustness
          if (rssi < -100)
            rssi = -100;
          if (rssi > -10)
            rssi = -10;
          bool isOpen = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
          LV_LOG_DEBUG("Pushed back %s / %d / %d", networkName.c_str(), rssi, static_cast<int>(isOpen));
          foundNetworks.push_back({networkName, isOpen, rssi});
        }
        WiFi.scanDelete();
        // Sort by RSSI (desc) and deduplicate by SSID (keep strongest)
        std::sort(foundNetworks.begin(), foundNetworks.end(),
                  [](const NetworkInfo &a, const NetworkInfo &b) { return a.rssiDbm > b.rssiDbm; });
        foundNetworks.erase(
            std::unique(foundNetworks.begin(), foundNetworks.end(),
                        [](const NetworkInfo &a, const NetworkInfo &b) { return a.networkName == b.networkName; }),
            foundNetworks.end());
      } else if (foundNetworksCount == WIFI_SCAN_FAILED) {
        LV_LOG_WARN("WiFi scan failed, resetting state");
        scanInProgress = false;
        WiFi.scanDelete();
      }
      return foundNetworks;
    }

    unsigned long now = millis();
    LV_LOG_INFO("Checking if we need to rescan, %lu > %lu", now, lastScan);
    if ((now - lastScan) > intervalForRescanMs) {
      lastScan = now;
      WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
      int16_t i = WiFi.scanNetworks(true, true, false, 150);
      LV_LOG_INFO("Started a scan, response %d", static_cast<int>(i));
      if (i == WIFI_SCAN_RUNNING) {
        scanInProgress = true;
      }
    }

    return foundNetworks;
  }

  bool isRunning() const { return scanInProgress; }
  unsigned long lastUpdatedMs() const { return lastScan; }
};