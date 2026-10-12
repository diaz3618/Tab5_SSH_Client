/*
 * WireGuard implementation for ESP32 Arduino by Kenta Ida (fuga@fugafuga.org)
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once
#include <IPAddress.h>

class WireGuard
{
private:
    bool _is_initialized = false;
public:
    bool begin(const IPAddress& localIP, const char* privateKey, const char* remotePeerAddress, const char* remotePeerPublicKey, uint16_t remotePeerPort,
               const char* presharedKey = nullptr, uint16_t persistentKeepalive = 0);
    void end();
    bool peer_is_up() const; // true once a handshake produced a valid keypair
    bool is_initialized() const { return this->_is_initialized; }
};
