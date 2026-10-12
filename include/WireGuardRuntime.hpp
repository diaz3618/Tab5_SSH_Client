#pragma once

#include "AppConfig.hpp"
#include <WireGuard-ESP32.h>

class WireGuardRuntime {
public:
    void poll(const WireGuardConfig& config);
    void stop();
    bool active() const { return _tunnel.is_initialized(); }
    String status(const WireGuardConfig& config) const;
    bool handshakeUp() const { return _tunnel.peer_is_up(); }
    static bool validate(const WireGuardConfig& config, String& error);
    // Parses a standard wg-quick style .conf. `notes` receives non-fatal warnings.
    static bool parseConf(const String& text, WireGuardConfig& out, String& notes, String& error);

private:
    WireGuard _tunnel;
    static void beginTask(void* arg);
    WireGuardConfig _pending;           // copy used by the background start-up task
    volatile bool _beginRunning{false}; // DNS lookup + interface start can take many seconds
    volatile bool _beginDone{false};
    volatile bool _beginOk{false};
    volatile bool _stopAfterBegin{false};
    uint32_t _lastAttempt{0};
    String _lastError;
};
