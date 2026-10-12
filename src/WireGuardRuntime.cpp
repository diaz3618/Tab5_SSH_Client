#include "WireGuardRuntime.hpp"

#include <WiFi.h>
#include <time.h>

namespace {
bool validKey(const String& key)
{
    if (key.length() != 44 || key[43] != '=') return false;
    for (size_t i = 0; i < 43; ++i) {
        const char c = key[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '+' || c == '/')) return false;
    }
    return true;
}
}

bool WireGuardRuntime::parseConf(const String& text, WireGuardConfig& out, String& notes, String& error)
{
    WireGuardConfig cfg;
    cfg.enabled = false;
    enum { None, Interface, Peer } section = None;
    int peers = 0;
    bool firstPeer = false;
    bool sawAddress = false;
    notes = "";
    size_t pos = 0;
    while (pos <= text.length()) {
        int nl = text.indexOf('\n', pos);
        if (nl < 0) nl = text.length();
        String line = text.substring(pos, nl);
        pos = nl + 1;
        const int hash = line.indexOf('#');
        if (hash >= 0) line = line.substring(0, hash);
        line.trim();
        if (!line.length()) continue;
        if (line[0] == '[') {
            String name = line;
            name.toLowerCase();
            if (name.startsWith("[interface")) {
                section = Interface;
            } else if (name.startsWith("[peer")) {
                section = Peer;
                ++peers;
                firstPeer = peers == 1;
            } else {
                section = None;
            }
            continue;
        }
        const int eq = line.indexOf('=');
        if (eq <= 0) continue;
        String key = line.substring(0, eq);
        String value = line.substring(eq + 1);
        key.trim();
        value.trim();
        key.toLowerCase();
        if (section == Interface) {
            if (key == "privatekey") {
                cfg.privateKey = value;
            } else if (key == "address" && !sawAddress) {
                // Take the first IPv4 entry of a comma separated list; drop /mask.
                size_t start = 0;
                while (start < value.length() && !sawAddress) {
                    int comma = value.indexOf(',', start);
                    if (comma < 0) comma = value.length();
                    String item = value.substring(start, comma);
                    item.trim();
                    const int slash = item.indexOf('/');
                    if (slash >= 0) item = item.substring(0, slash);
                    IPAddress probe;
                    if (probe.fromString(item) && probe.type() == IPv4) {
                        cfg.address = item;
                        sawAddress = true;
                    }
                    start = comma + 1;
                }
            } else if (key == "dns" || key == "mtu" || key == "listenport" || key == "postup" || key == "postdown") {
                notes += key + " ignored; ";
            }
        } else if (section == Peer) {
            if (!firstPeer) {
                continue;
            }
            if (key == "publickey") {
                cfg.publicKey = value;
            } else if (key == "presharedkey") {
                cfg.presharedKey = value;
            } else if (key == "endpoint") {
                const int colon = value.lastIndexOf(':');
                if (colon <= 0 || value[0] == '[') {
                    error = "Endpoint must be host:port (IPv6 endpoints are not supported)";
                    return false;
                }
                cfg.endpoint = value.substring(0, colon);
                cfg.port = static_cast<uint16_t>(value.substring(colon + 1).toInt());
            } else if (key == "persistentkeepalive") {
                cfg.keepalive = static_cast<uint16_t>(constrain(value.toInt(), 0, 65534));
            } else if (key == "allowedips") {
                if (value.indexOf("0.0.0.0/0") < 0) {
                    notes += "AllowedIPs: all traffic is routed through the tunnel; ";
                }
            }
        }
    }
    if (peers > 1) notes += "only the first [Peer] is used; ";
    if (!validate(cfg, error)) {
        return false;
    }
    out = cfg;
    error = "";
    return true;
}

bool WireGuardRuntime::validate(const WireGuardConfig& config, String& error)
{
    IPAddress address;
    if (!address.fromString(config.address) || address.type() != IPv4) {
        error = "invalid IPv4 tunnel address";
    } else if (!validKey(config.privateKey)) {
        error = "invalid private key (expected 44 base64 characters)";
    } else if (!validKey(config.publicKey)) {
        error = "invalid peer public key (expected 44 base64 characters)";
    } else if (!config.endpoint.length() || config.endpoint.indexOf(' ') >= 0 || config.endpoint.indexOf(':') >= 0) {
        error = "invalid peer endpoint; use a DNS name or IPv4 address";
    } else if (config.presharedKey.length() && !validKey(config.presharedKey)) {
        error = "invalid preshared key (expected 44 base64 characters)";
    } else if (!config.port) {
        error = "invalid peer port";
    } else {
        error = "";
    }
    return !error.length();
}

void WireGuardRuntime::stop()
{
    if (_beginRunning) {
        _stopAfterBegin = true; // the start-up task owns the tunnel until it finishes
        return;
    }
    if (_tunnel.is_initialized()) _tunnel.end();
    _lastAttempt = 0;
    _lastError = "";
}

// The endpoint lookup retries for up to ~10 s, so it must never run on the UI thread.
void WireGuardRuntime::beginTask(void* arg)
{
    auto* self = static_cast<WireGuardRuntime*>(arg);
    IPAddress address;
    address.fromString(self->_pending.address);
    self->_beginOk = self->_tunnel.begin(address, self->_pending.privateKey.c_str(), self->_pending.endpoint.c_str(),
                                         self->_pending.publicKey.c_str(), self->_pending.port,
                                         self->_pending.presharedKey.length() ? self->_pending.presharedKey.c_str() : nullptr,
                                         self->_pending.keepalive);
    self->_beginDone = true;
    self->_beginRunning = false;
    vTaskDelete(nullptr);
}

void WireGuardRuntime::poll(const WireGuardConfig& config)
{
    if (_beginRunning) {
        return;
    }
    if (_beginDone) {
        _beginDone = false;
        if (_stopAfterBegin) {
            _stopAfterBegin = false;
            if (_tunnel.is_initialized()) _tunnel.end();
            return;
        }
        if (_beginOk) {
            _lastError = "";
            Serial.println("WireGuard interface started");
        } else {
            _lastError = "interface start failed (DNS or network)";
            Serial.println("WireGuard interface start failed");
        }
        return;
    }
    if (!config.enabled || WiFi.status() != WL_CONNECTED) {
        if (_tunnel.is_initialized()) _tunnel.end();
        return;
    }
    if (_tunnel.is_initialized()) return;
    // WireGuard handshakes use wall-clock timestamps to reject replays.
    if (time(nullptr) < 1700000000) return;
    const uint32_t now = millis();
    if (_lastAttempt && now - _lastAttempt < 30000) return;
    _lastAttempt = now ? now : 1;
    if (!validate(config, _lastError)) return;
    _pending = config;
    _beginRunning = true;
    if (xTaskCreate(beginTask, "wg-begin", 12288, this, 1, nullptr) != pdPASS) {
        _beginRunning = false;
        _lastError = "cannot start WireGuard task";
    }
}

String WireGuardRuntime::status(const WireGuardConfig& config) const
{
    if (!config.enabled) return "off";
    if (_beginRunning) return "starting (resolving endpoint)";
    if (_tunnel.is_initialized()) return _tunnel.peer_is_up() ? "connected (handshake ok)" : "interface up; waiting for handshake";
    if (_lastError.length()) return String("error: ") + _lastError;
    if (WiFi.status() != WL_CONNECTED) return "waiting for Wi-Fi";
    if (time(nullptr) < 1700000000) return "waiting for NTP";
    return "starting";
}
