#include "SettingsStore.hpp"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace {
constexpr const char* kConfigPath = "/profiles.json";
}

bool SettingsStore::begin()
{
    if (LittleFS.begin(true, "/littlefs", 10, "littlefs")) {
        return true;
    }
    // Started from a launcher with its own partition table: use whatever data partition it provides.
    if (LittleFS.begin(true, "/littlefs", 10, "spiffs")) {
        return true;
    }
    _lastError = "LittleFS mount failed";
    return false;
}

bool SettingsStore::load(AppConfig& config)
{
    File file = LittleFS.open(kConfigPath, "r");
    if (!file) {
        _lastError = "profiles.json not found. Upload data filesystem first.";
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    if (err) {
        _lastError = String("JSON parse failed: ") + err.c_str();
        return false;
    }

    config = AppConfig{};
    for (JsonObject item : doc["wifi"].as<JsonArray>()) {
        WifiProfile profile;
        profile.name = item["name"] | "";
        profile.ssid = item["ssid"] | "";
        profile.password = item["password"] | "";
        if (profile.name.length() && profile.ssid.length()) {
            config.wifi.push_back(profile);
        }
    }

    for (JsonObject item : doc["ssh"].as<JsonArray>()) {
        SshProfile profile;
        profile.name = item["name"] | "";
        profile.host = item["host"] | "";
        profile.port = item["port"] | 22;
        profile.user = item["user"] | "";
        profile.password = item["password"] | "";
        profile.terminal = item["terminal"] | "xterm-256color";
        profile.keyName = item["keyName"] | "";
        profile.keyPassphrase = item["keyPassphrase"] | "";
        if (profile.name.length() && profile.host.length() && profile.user.length()) {
            config.ssh.push_back(profile);
        }
    }

    config.keyboard.layout = doc["keyboard"]["layout"] | "us";
    config.keyboard.terminalFont = doc["keyboard"]["terminalFont"] | "mono12";
    config.keyboard.terminalLineStep = doc["keyboard"]["terminalLineStep"] | 15;
    config.keyboard.swapCtrlCaps = doc["keyboard"]["swapCtrlCaps"] | false;
    config.keyboard.bleKeyboardEnabled = doc["keyboard"]["bleKeyboardEnabled"] | false;
    JsonArrayConst savedBleDevices = doc["keyboard"]["bleDevices"].as<JsonArrayConst>();
    for (JsonObjectConst item : savedBleDevices) {
        BleHidProfile profile;
        profile.name = item["name"] | "";
        profile.address = item["address"] | "";
        profile.kind = item["kind"] | "keyboard";
        profile.addressType = item["addressType"] | 1;
        profile.enabled = item["enabled"] | true;
        if (profile.address.length()) {
            config.keyboard.bleDevices.push_back(profile);
        }
    }
    const String legacyBleAddress = doc["keyboard"]["bleKeyboardAddress"] | "";
    if (!config.keyboard.bleDevices.size() && legacyBleAddress.length()) {
        BleHidProfile profile;
        profile.name = doc["keyboard"]["bleKeyboardName"] | "BLE keyboard";
        profile.address = legacyBleAddress;
        profile.kind = "keyboard";
        profile.addressType = doc["keyboard"]["bleKeyboardAddressType"] | 1;
        profile.enabled = true;
        config.keyboard.bleDevices.push_back(profile);
    }
    config.keyboard.activeBle = doc["keyboard"]["activeBle"] | 0;
    if (config.keyboard.activeBle >= config.keyboard.bleDevices.size()) {
        config.keyboard.activeBle = 0;
    }
    if (config.keyboard.bleDevices.size()) {
        const auto& active = config.keyboard.bleDevices[config.keyboard.activeBle];
        config.keyboard.bleKeyboardName = active.name;
        config.keyboard.bleKeyboardAddress = active.address;
    }
    config.system.deviceName = doc["system"]["deviceName"] | "tab5";
    config.system.region = doc["system"]["region"] | "Asia/Tokyo";
    config.system.utcOffsetMinutes = doc["system"]["utcOffsetMinutes"] | 540;
    config.system.ntpServer = doc["system"]["ntpServer"] | "pool.ntp.org";
    config.wireguard.autoStart = doc["wireguard"]["autoStart"] | false;
    config.wireguard.enabled = config.wireguard.autoStart; // an old saved "enabled" flag is ignored
    config.wireguard.address = doc["wireguard"]["address"] | "";
    config.wireguard.privateKey = doc["wireguard"]["privateKey"] | "";
    config.wireguard.endpoint = doc["wireguard"]["endpoint"] | "";
    config.wireguard.port = doc["wireguard"]["port"] | 51820;
    config.wireguard.publicKey = doc["wireguard"]["publicKey"] | "";
    config.wireguard.presharedKey = doc["wireguard"]["presharedKey"] | "";
    config.wireguard.keepalive = doc["wireguard"]["keepalive"] | 0;
    config.system.logSessions = doc["system"]["logSessions"] | false;
    for (JsonObject item : doc["snippets"].as<JsonArray>()) {
        Snippet snippet;
        snippet.name = item["name"] | "";
        snippet.text = item["text"] | "";
        if (snippet.name.length() && snippet.text.length()) {
            config.snippets.push_back(snippet);
        }
    }
    config.ui.theme = doc["ui"]["theme"] | "tokyo-night";
    config.ui.nav = doc["ui"]["nav"] | "top";
    config.ui.scale = doc["ui"]["scale"] | 100;
    if (config.ui.scale != 100 && config.ui.scale != 125 && config.ui.scale != 150) config.ui.scale = 100;
    config.activeWifi = doc["activeWifi"] | 0;
    config.activeSsh = doc["activeSsh"] | 0;
    if (config.activeWifi >= config.wifi.size()) {
        config.activeWifi = 0;
    }
    if (config.activeSsh >= config.ssh.size()) {
        config.activeSsh = 0;
    }
    return true;
}

bool SettingsStore::save(const AppConfig& config)
{
    JsonDocument doc;
    JsonArray wifi = doc["wifi"].to<JsonArray>();
    for (const auto& profile : config.wifi) {
        JsonObject item = wifi.add<JsonObject>();
        item["name"] = profile.name;
        item["ssid"] = profile.ssid;
        item["password"] = profile.password;
    }

    JsonArray ssh = doc["ssh"].to<JsonArray>();
    for (const auto& profile : config.ssh) {
        JsonObject item = ssh.add<JsonObject>();
        item["name"] = profile.name;
        item["host"] = profile.host;
        item["port"] = profile.port;
        item["user"] = profile.user;
        item["password"] = profile.password;
        item["terminal"] = profile.terminal;
        item["keyName"] = profile.keyName;
        item["keyPassphrase"] = profile.keyPassphrase;
    }

    doc["keyboard"]["layout"] = config.keyboard.layout;
    doc["keyboard"]["terminalFont"] = config.keyboard.terminalFont;
    doc["keyboard"]["terminalLineStep"] = config.keyboard.terminalLineStep;
    doc["keyboard"]["swapCtrlCaps"] = config.keyboard.swapCtrlCaps;
    doc["keyboard"]["bleKeyboardEnabled"] = config.keyboard.bleKeyboardEnabled;
    JsonArray bleDevices = doc["keyboard"]["bleDevices"].to<JsonArray>();
    for (const auto& profile : config.keyboard.bleDevices) {
        JsonObject item = bleDevices.add<JsonObject>();
        item["name"] = profile.name;
        item["address"] = profile.address;
        item["kind"] = profile.kind;
        item["addressType"] = profile.addressType;
        item["enabled"] = profile.enabled;
    }
    doc["keyboard"]["activeBle"] = config.keyboard.activeBle;
    doc["system"]["deviceName"] = config.system.deviceName;
    doc["system"]["region"] = config.system.region;
    doc["system"]["utcOffsetMinutes"] = config.system.utcOffsetMinutes;
    doc["system"]["ntpServer"] = config.system.ntpServer;
    doc["wireguard"]["autoStart"] = config.wireguard.autoStart;
    doc["wireguard"]["address"] = config.wireguard.address;
    doc["wireguard"]["privateKey"] = config.wireguard.privateKey;
    doc["wireguard"]["endpoint"] = config.wireguard.endpoint;
    doc["wireguard"]["port"] = config.wireguard.port;
    doc["wireguard"]["publicKey"] = config.wireguard.publicKey;
    doc["wireguard"]["presharedKey"] = config.wireguard.presharedKey;
    doc["wireguard"]["keepalive"] = config.wireguard.keepalive;
    doc["system"]["logSessions"] = config.system.logSessions;
    JsonArray snippets = doc["snippets"].to<JsonArray>();
    for (const auto& snippet : config.snippets) {
        JsonObject item = snippets.add<JsonObject>();
        item["name"] = snippet.name;
        item["text"] = snippet.text;
    }
    doc["ui"]["theme"] = config.ui.theme;
    doc["ui"]["nav"] = config.ui.nav;
    doc["ui"]["scale"] = config.ui.scale;
    doc["activeWifi"] = config.activeWifi;
    doc["activeSsh"] = config.activeSsh;

    // Serialize to memory first and write once: fewer flash erase cycles than many small writes.
    String text;
    serializeJsonPretty(doc, text);
    File file = LittleFS.open(kConfigPath, "w");
    if (!file) {
        _lastError = "profiles.json open for write failed";
        return false;
    }
    const size_t written = file.print(text);
    file.close();
    if (written != text.length()) {
        _lastError = "profiles.json write incomplete";
        return false;
    }
    return true;
}
