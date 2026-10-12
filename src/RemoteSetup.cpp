#include "RemoteSetup.hpp"

#include <WebServer.h>
#include <WiFi.h>
#include <esp_random.h>

namespace {
constexpr uint16_t kPort = 8080;
constexpr uint32_t kTimeoutMs = 5UL * 60UL * 1000UL;
constexpr uint8_t kMaxBadPins = 5;

const char kPage[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Tab5 setup</title>
<style>body{font:16px system-ui;max-width:640px;margin:16px auto;padding:0 12px;background:#14161c;color:#dde}
h2{margin-top:28px}input,textarea,button{width:100%;box-sizing:border-box;margin:6px 0;padding:10px;font:inherit;
background:#1d2029;color:#dde;border:1px solid #444b5c;border-radius:6px}textarea{height:200px;font-family:monospace;font-size:13px}
button{background:#2b5fd9;border:0}small{color:#99a}</style></head><body>
<h1>Tab5 setup</h1><small>Plain HTTP: use only on a network you trust. This page closes after one upload.</small>
<label>PIN shown on the Tab5<input id="pin" inputmode="numeric" autocomplete="off"></label>
<h2>WireGuard</h2>
<form method="post" action="/wg" onsubmit="this.elements['pin'].value=document.getElementById('pin').value">
<input type="hidden" name="pin"><input type="file" accept=".conf,text/plain" onchange="load(this,'wgconf')">
<textarea id="wgconf" name="conf" placeholder="[Interface]&#10;PrivateKey = ...&#10;Address = 10.0.0.2/32&#10;&#10;[Peer]&#10;PublicKey = ...&#10;Endpoint = host:51820"></textarea>
<button>Save WireGuard config</button></form>
<h2>SSH private key</h2>
<form method="post" action="/key" onsubmit="this.elements['pin'].value=document.getElementById('pin').value">
<input type="hidden" name="pin"><input name="name" placeholder="key name, e.g. laptop" maxlength="32">
<input type="file" onchange="load(this,'keypem')">
<textarea id="keypem" name="pem" placeholder="-----BEGIN OPENSSH PRIVATE KEY-----"></textarea>
<button>Save SSH key</button></form>
<p><a href="#" onclick="location='/pub?pin='+encodeURIComponent(document.getElementById('pin').value);return false">Download public keys (authorized_keys lines)</a></p>
<script>function load(i,t){var f=i.files[0];if(!f)return;var r=new FileReader();r.onload=function(){document.getElementById(t).value=r.result};r.readAsText(f)}</script>
</body></html>)HTML";
}

RemoteSetup::RemoteSetup() = default;
RemoteSetup::~RemoteSetup() = default;

bool RemoteSetup::start(String& error)
{
    if (_server) return true;
    if (WiFi.status() != WL_CONNECTED) {
        error = "connect to Wi-Fi first";
        return false;
    }
    _pin = String(100000 + esp_random() % 900000);
    _badPins = 0;
    _stopPending = false;
    _lastResult = "";
    _server.reset(new WebServer(kPort));
    _server->on("/", HTTP_GET, [this]() { handleRoot(); });
    _server->on("/wg", HTTP_POST, [this]() { handleWireGuard(); });
    _server->on("/key", HTTP_POST, [this]() { handleSshKey(); });
    _server->on("/pub", HTTP_GET, [this]() { handlePublicKeys(); });
    _server->onNotFound([this]() { reply(404, "not found"); });
    _server->begin();
    _url = String("http://") + WiFi.localIP().toString() + ":" + kPort;
    _startedAt = millis();
    _changed = true;
    return true;
}

void RemoteSetup::stop()
{
    if (_server) {
        _server->stop();
        _server.reset();
    }
    _pin = "";
    _changed = true;
}

void RemoteSetup::poll()
{
    if (!_server) return;
    _server->handleClient();
    if (_stopPending) { // handlers must not destroy the server they run inside
        _stopPending = false;
        stop();
        return;
    }
    if (WiFi.status() != WL_CONNECTED || millis() - _startedAt > kTimeoutMs) {
        _lastResult = "remote setup timed out";
        stop();
    }
}

bool RemoteSetup::takeChanged()
{
    const bool changed = _changed;
    _changed = false;
    return changed;
}

void RemoteSetup::reply(int code, const String& message)
{
    _server->send(code, "text/plain; charset=utf-8", message + "\n");
}

bool RemoteSetup::checkPin()
{
    if (_server->arg("pin") == _pin) return true;
    ++_badPins;
    if (_badPins >= kMaxBadPins) {
        _lastResult = "remote setup stopped: too many wrong PINs";
        reply(403, "wrong PIN; remote setup closed");
        _stopPending = true;
    } else {
        reply(403, "wrong PIN");
    }
    return false;
}

void RemoteSetup::handleRoot()
{
    _startedAt = millis();
    _server->send_P(200, "text/html; charset=utf-8", kPage);
}

void RemoteSetup::handleWireGuard()
{
    if (!checkPin()) return;
    String result;
    const bool ok = onWireGuard && onWireGuard(_server->arg("conf"), result);
    _lastResult = result;
    reply(ok ? 200 : 400, ok ? "OK: " + result : "ERROR: " + result);
    if (ok) _stopPending = true;
    _changed = true;
}

void RemoteSetup::handlePublicKeys()
{
    if (_server->arg("pin") != _pin) {
        checkPin();
        return;
    }
    _startedAt = millis();
    reply(200, onPublicKeys ? onPublicKeys() : String(""));
}

void RemoteSetup::handleSshKey()
{
    if (!checkPin()) return;
    String result;
    const bool ok = onSshKey && onSshKey(_server->arg("name"), _server->arg("pem"), result);
    _lastResult = result;
    reply(ok ? 200 : 400, ok ? "OK: " + result : "ERROR: " + result);
    if (ok) _stopPending = true;
    _changed = true;
}
