#pragma once

#include <Arduino.h>
#include <functional>
#include <memory>

class WebServer;

// Temporary LAN web page for configuring the Tab5 from a phone or laptop:
// paste/upload a WireGuard .conf or an SSH private key. It is only running
// while the user has started it on the device, requires the on-screen PIN,
// and stops itself after a successful upload, too many bad PINs, or 5 minutes.
// Traffic is plain HTTP, so use it on a network you trust.
class RemoteSetup {
public:
    using WireGuardHandler = std::function<bool(const String& conf, String& result)>;
    using SshKeyHandler = std::function<bool(const String& name, const String& pem, String& result)>;

    RemoteSetup();
    ~RemoteSetup();
    bool start(String& error);
    void stop();
    void poll();
    bool active() const { return _server != nullptr; }
    String url() const { return _url; }
    String pin() const { return _pin; }
    const String& lastResult() const { return _lastResult; }
    bool takeChanged();

    WireGuardHandler onWireGuard;
    SshKeyHandler onSshKey;
    std::function<String()> onPublicKeys; // newline separated authorized_keys lines

private:
    void handleRoot();
    void handleWireGuard();
    void handleSshKey();
    void handlePublicKeys();
    bool checkPin();
    void reply(int code, const String& message);

    std::unique_ptr<WebServer> _server;
    String _url;
    String _pin;
    String _lastResult;
    uint32_t _startedAt{0};
    uint8_t _badPins{0};
    bool _changed{false};
    bool _stopPending{false};
};
