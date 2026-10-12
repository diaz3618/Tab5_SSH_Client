#pragma once

#include <Arduino.h>
#include <vector>

// SSH private keys live in the internal LittleFS under /keys (never on the
// removable SD card). Keys are OpenSSH/PEM text accepted by libssh.
namespace SshKeys {
bool validName(const String& name);
std::vector<String> list();
bool exists(const String& name);
bool read(const String& name, String& pem);
bool save(const String& name, const String& pem, String& error);
bool remove(const String& name);
bool importFromFile(const String& sdPath, const String& name, String& error);
bool generate(const String& name, const String& passphrase, String& error);
// Returns "ssh-ed25519 AAAA... comment" for the stored key, or "" with error set.
String publicLine(const String& name, const String& passphrase, String& error);
}
