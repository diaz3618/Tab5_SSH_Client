#include "SshKeys.hpp"

#include <LittleFS.h>
#include <SD.h>

#if ENABLE_SSH
#include <libssh_esp32.h>
#include <libssh/libssh.h>
#endif

namespace {
constexpr const char* kDir = "/keys";
constexpr size_t kMaxKeyBytes = 8192;

String pathFor(const String& name)
{
    return String(kDir) + "/" + name;
}

void ensureDir()
{
    if (!LittleFS.exists(kDir)) {
        LittleFS.mkdir(kDir);
    }
}
}

namespace SshKeys {

bool validName(const String& name)
{
    if (!name.length() || name.length() > 32) return false;
    for (size_t i = 0; i < name.length(); ++i) {
        const char c = name[i];
        if (!(isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' || c == '-')) return false;
    }
    return name[0] != '.';
}

std::vector<String> list()
{
    std::vector<String> names;
    ensureDir();
    File dir = LittleFS.open(kDir);
    if (!dir || !dir.isDirectory()) return names;
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        String n = f.name();
        const int slash = n.lastIndexOf('/');
        if (slash >= 0) n = n.substring(slash + 1);
        if (!f.isDirectory() && validName(n)) names.push_back(n);
    }
    return names;
}

bool exists(const String& name)
{
    return validName(name) && LittleFS.exists(pathFor(name));
}

bool read(const String& name, String& pem)
{
    if (!validName(name)) return false;
    File f = LittleFS.open(pathFor(name), "r");
    if (!f) return false;
    pem = f.readString();
    return pem.length() > 0;
}

bool save(const String& name, const String& pem, String& error)
{
    if (!validName(name)) {
        error = "invalid key name (use A-Z a-z 0-9 . _ -)";
        return false;
    }
    if (pem.length() > kMaxKeyBytes || pem.indexOf("PRIVATE KEY") < 0) {
        error = "not a private key (expected a PEM/OpenSSH private key)";
        return false;
    }
    ensureDir();
    File f = LittleFS.open(pathFor(name), "w");
    if (!f) {
        error = "cannot write key file";
        return false;
    }
    String body = pem;
    body.replace("\r\n", "\n");
    body.trim();
    body += "\n";
    const size_t written = f.print(body);
    f.close();
    if (written != body.length()) {
        LittleFS.remove(pathFor(name));
        error = "key write incomplete (flash full?)";
        return false;
    }
    return true;
}

bool remove(const String& name)
{
    return validName(name) && LittleFS.remove(pathFor(name));
}

bool importFromFile(const String& sdPath, const String& name, String& error)
{
    File f = SD.open(sdPath, "r");
    if (!f || f.isDirectory()) {
        error = "cannot open " + sdPath;
        return false;
    }
    if (f.size() > kMaxKeyBytes) {
        error = "file too large for a private key";
        return false;
    }
    String pem = f.readString();
    f.close();
    return save(name, pem, error);
}

#if ENABLE_SSH
bool generate(const String& name, const String& passphrase, String& error)
{
    if (!validName(name)) {
        error = "invalid key name";
        return false;
    }
    if (exists(name)) {
        error = "a key with that name already exists";
        return false;
    }
    libssh_begin();
    ssh_key key = nullptr;
    if (ssh_pki_generate(SSH_KEYTYPE_ED25519, 256, &key) != SSH_OK || !key) {
        error = "ed25519 generation failed";
        return false;
    }
    char* b64 = nullptr;
    const int rc = ssh_pki_export_privkey_base64(key, passphrase.length() ? passphrase.c_str() : nullptr,
                                                 nullptr, nullptr, &b64);
    ssh_key_free(key);
    if (rc != SSH_OK || !b64) {
        error = "private key export failed";
        return false;
    }
    const bool ok = save(name, String(b64), error);
    ssh_string_free_char(b64);
    return ok;
}

String publicLine(const String& name, const String& passphrase, String& error)
{
    String pem;
    if (!read(name, pem)) {
        error = "key not found";
        return "";
    }
    libssh_begin();
    ssh_key priv = nullptr;
    if (ssh_pki_import_privkey_base64(pem.c_str(), passphrase.length() ? passphrase.c_str() : nullptr,
                                      nullptr, nullptr, &priv) != SSH_OK || !priv) {
        error = "cannot read key (wrong passphrase or unsupported format)";
        return "";
    }
    ssh_key pub = nullptr;
    String line;
    if (ssh_pki_export_privkey_to_pubkey(priv, &pub) == SSH_OK && pub) {
        char* b64 = nullptr;
        if (ssh_pki_export_pubkey_base64(pub, &b64) == SSH_OK && b64) {
            line = String(ssh_key_type_to_char(ssh_key_type(pub))) + " " + b64 + " tab5:" + name;
            ssh_string_free_char(b64);
        }
        ssh_key_free(pub);
    }
    ssh_key_free(priv);
    if (!line.length()) error = "public key export failed";
    return line;
}
#else
bool generate(const String&, const String&, String& error) { error = "SSH disabled"; return false; }
String publicLine(const String&, const String&, String& error) { error = "SSH disabled"; return ""; }
#endif
}
