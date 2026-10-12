#pragma once

#include <Arduino.h>
#include <vector>

struct WifiProfile {
    String name;
    String ssid;
    String password;
};

struct SshProfile {
    String name;
    String host;
    uint16_t port{22};
    String user;
    String password;
    String terminal{"xterm-256color"};
    String keyName;       // file under /keys on LittleFS; empty = password auth
    String keyPassphrase; // optional passphrase for an encrypted private key
};

struct BleHidProfile {
    String name;
    String address;
    String kind{"keyboard"};
    uint8_t addressType{1};
    bool enabled{true};
};

struct KeyboardConfig {
    String layout{"us"};
    String terminalFont{"mono12"};
    uint8_t terminalLineStep{15};
    bool swapCtrlCaps{false};
    bool bleKeyboardEnabled{false};
    std::vector<BleHidProfile> bleDevices;
    size_t activeBle{0};
    String bleKeyboardName;
    String bleKeyboardAddress;
};

struct Snippet {
    String name;
    String text;
};

struct SystemConfig {
    String deviceName{"tab5"};
    String region{"Asia/Tokyo"};
    int16_t utcOffsetMinutes{540};
    String ntpServer{"pool.ntp.org"};
    bool logSessions{false}; // append SSH output to /logs on the SD card
};

struct UiConfig {
    String theme{"tokyo-night"};
    uint8_t scale{100}; // interface text size in percent: 100, 125 or 150
    String nav{"top"}; // "top" tab strip or "side" rail
};

struct WireGuardConfig {
    bool enabled{false};   // runtime only: never saved, so the tunnel is off after every boot...
    bool autoStart{false}; // ...unless the user turned on CONF > WG at boot
    String address;
    String privateKey;
    String endpoint;
    uint16_t port{51820};
    String publicKey;
    String presharedKey;
    uint16_t keepalive{0}; // seconds; 0 = library default
};

struct AppConfig {
    std::vector<WifiProfile> wifi;
    std::vector<SshProfile> ssh;
    KeyboardConfig keyboard;
    SystemConfig system;
    WireGuardConfig wireguard;
    UiConfig ui;
    std::vector<Snippet> snippets;
    size_t activeWifi{0};
    size_t activeSsh{0};
};
