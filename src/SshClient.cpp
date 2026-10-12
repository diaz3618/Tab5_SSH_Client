#include "SshClient.hpp"
#include "SshKeys.hpp"

#include <LittleFS.h>

#if ENABLE_SSH
#include <libssh_esp32.h>
#include <libssh/libssh.h>
#endif

extern void tab5SetCrashStage(const char* stage);
extern void tab5SshProgress(const char* stage);

void reportSshStage(const char* stage)
{
    tab5SetCrashStage(stage);
    tab5SshProgress(stage);
}

#if ENABLE_SSH
namespace {
void ensureLibsshStarted()
{
    static bool libsshStarted = false;
    if (!libsshStarted) {
        libssh_begin();
        libsshStarted = true;
    }
}

constexpr const char* kKnownHostsPath = "/known_hosts";

// Trust on first use: the first key seen for host:port is stored; a different key later is refused.
bool verifyHostKey(ssh_session session, const SshProfile& profile, String& error)
{
    ssh_key key = nullptr;
    if (ssh_get_server_publickey(session, &key) != SSH_OK || !key) {
        error = "server sent no host key";
        return false;
    }
    unsigned char* hash = nullptr;
    size_t hashLen = 0;
    const int rc = ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA256, &hash, &hashLen);
    ssh_key_free(key);
    if (rc != 0 || !hash) {
        error = "cannot hash host key";
        return false;
    }
    char* fp = ssh_get_fingerprint_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hashLen);
    ssh_clean_pubkey_hash(&hash);
    if (!fp) {
        error = "cannot format host key";
        return false;
    }
    const String fingerprint(fp);
    ssh_string_free_char(fp);
    const String entry = profile.host + ":" + profile.port;

    String stored;
    File in = LittleFS.open(kKnownHostsPath, "r");
    if (in) {
        while (in.available()) {
            String line = in.readStringUntil('\n');
            line.trim();
            if (line.startsWith(entry + " ")) {
                stored = line.substring(entry.length() + 1);
                break;
            }
        }
        in.close();
    }
    if (stored.length()) {
        if (stored == fingerprint) {
            return true;
        }
        error = "HOST KEY CHANGED for " + entry + " (was " + stored + ", now " + fingerprint +
                "). If this is expected run: sshhost forget " + entry;
        return false;
    }
    File out = LittleFS.open(kKnownHostsPath, "a");
    if (out) {
        out.println(entry + " " + fingerprint);
        out.close();
    }
    reportSshStage((String("new host key trusted ") + fingerprint).c_str());
    return true;
}

bool authenticate(ssh_session session, const SshProfile& profile, String& error)
{
    String keyError;
    if (profile.keyName.length()) {
        String pem;
        if (!SshKeys::read(profile.keyName, pem)) {
            keyError = "key '" + profile.keyName + "' not found";
        } else {
            ssh_key key = nullptr;
            if (ssh_pki_import_privkey_base64(pem.c_str(),
                                              profile.keyPassphrase.length() ? profile.keyPassphrase.c_str() : nullptr,
                                              nullptr, nullptr, &key) != SSH_OK || !key) {
                keyError = "cannot load key '" + profile.keyName + "' (passphrase or format)";
            } else {
                const int rc = ssh_userauth_publickey(session, nullptr, key);
                ssh_key_free(key);
                if (rc == SSH_AUTH_SUCCESS) {
                    return true;
                }
                keyError = String("key rejected: ") + ssh_get_error(session);
            }
        }
        if (!profile.password.length()) {
            error = keyError;
            return false;
        }
    }
    if (ssh_userauth_password(session, nullptr, profile.password.c_str()) == SSH_AUTH_SUCCESS) {
        return true;
    }
    error = keyError.length() ? keyError + "; password: " + ssh_get_error(session) : String(ssh_get_error(session));
    return false;
}

ssh_session openSession(const SshProfile& profile, String& error)
{
    ensureLibsshStarted();
    ssh_session session = ssh_new();
    if (!session) {
        error = "ssh_new failed";
        return nullptr;
    }
    const int verbosity = SSH_LOG_NOLOG;
    const int port = profile.port;
    ssh_options_set(session, SSH_OPTIONS_HOST, profile.host.c_str());
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(session, SSH_OPTIONS_USER, profile.user.c_str());
    ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    if (ssh_connect(session) != SSH_OK) {
        error = ssh_get_error(session);
        ssh_free(session);
        return nullptr;
    }
    if (!verifyHostKey(session, profile, error) || !authenticate(session, profile, error)) {
        ssh_disconnect(session);
        ssh_free(session);
        return nullptr;
    }
    ssh_set_blocking(session, 1);
    return session;
}

String basenameOf(const String& path)
{
    int slash = path.lastIndexOf('/');
    if (slash < 0 || slash + 1 >= static_cast<int>(path.length())) {
        return path.length() ? path : "upload.bin";
    }
    return path.substring(slash + 1);
}

String dirnameOf(const String& path)
{
    int slash = path.lastIndexOf('/');
    if (slash <= 0) {
        return slash == 0 ? "/" : ".";
    }
    return path.substring(0, slash);
}
}
#endif

bool SshClient::connect(const SshProfile& profile, String& error, int columns, int rows)
{
#if ENABLE_SSH
    reportSshStage("ssh.libssh_begin");
    ensureLibsshStarted();

    reportSshStage("ssh.disconnect");
    disconnect();

    reportSshStage("ssh_new");
    ssh_session session = ssh_new();
    if (!session) {
        error = "ssh_new failed";
        return false;
    }

    reportSshStage("ssh_options");
    const int verbosity = SSH_LOG_NOLOG;
    const long connectTimeoutSeconds = 8; // bound the blocking connect so the UI cannot hang for minutes
    ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &connectTimeoutSeconds);
    ssh_options_set(session, SSH_OPTIONS_HOST, profile.host.c_str());
    const int port = profile.port;
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    ssh_options_set(session, SSH_OPTIONS_USER, profile.user.c_str());
    ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

    reportSshStage("ssh_connect");
    if (ssh_connect(session) != SSH_OK) {
        error = ssh_get_error(session);
        ssh_free(session);
        return false;
    }

    reportSshStage("ssh_hostkey");
    if (!verifyHostKey(session, profile, error)) {
        ssh_disconnect(session);
        ssh_free(session);
        return false;
    }

    reportSshStage(profile.keyName.length() ? "ssh_auth_key" : "ssh_auth_password");
    if (!authenticate(session, profile, error)) {
        ssh_disconnect(session);
        ssh_free(session);
        return false;
    }

    reportSshStage("ssh_channel_new");
    ssh_channel channel = ssh_channel_new(session);
    if (!channel) {
        error = "ssh_channel_new failed";
        ssh_disconnect(session);
        ssh_free(session);
        return false;
    }

    reportSshStage("ssh_pty_shell");
    if (ssh_channel_open_session(channel) != SSH_OK ||
        ssh_channel_request_pty_size(channel, profile.terminal.c_str(), columns, rows) != SSH_OK ||
        ssh_channel_request_shell(channel) != SSH_OK) {
        error = ssh_get_error(session);
        ssh_channel_free(channel);
        ssh_disconnect(session);
        ssh_free(session);
        return false;
    }

    ssh_set_blocking(session, 0);
    _session = session;
    _channel = channel;
    _lastKeepalive = millis();
    reportSshStage("ssh_ready");
    return true;
#else
    (void)profile;
    (void)columns;
    (void)rows;
    error = "ENABLE_SSH is disabled";
    return false;
#endif
}

void SshClient::disconnect()
{
#if ENABLE_SSH
    if (_channel) {
        ssh_channel channel = static_cast<ssh_channel>(_channel);
        ssh_channel_send_eof(channel);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        _channel = nullptr;
    }
    if (_session) {
        ssh_session session = static_cast<ssh_session>(_session);
        ssh_disconnect(session);
        ssh_free(session);
        _session = nullptr;
    }
#endif
}

bool SshClient::connected() const
{
#if ENABLE_SSH
    return _session && _channel && !ssh_channel_is_closed(static_cast<ssh_channel>(_channel));
#else
    return false;
#endif
}

int SshClient::read(char* buffer, size_t len)
{
#if ENABLE_SSH
    if (!connected()) {
        return -1;
    }
    int n = ssh_channel_read_nonblocking(static_cast<ssh_channel>(_channel), buffer, len, 0);
    if (n == SSH_AGAIN) {
        return 0;
    }
    return n == SSH_ERROR ? -1 : n;
#else
    (void)buffer;
    (void)len;
    return -1;
#endif
}

bool SshClient::write(const uint8_t* data, size_t len)
{
#if ENABLE_SSH
    if (!connected()) {
        return false;
    }
    ssh_session session = static_cast<ssh_session>(_session);
    ssh_channel channel = static_cast<ssh_channel>(_channel);
    ssh_set_blocking(session, 0);
    size_t sent = 0;
    uint32_t start = millis();
    while (sent < len && millis() - start < 30) {
        int n = ssh_channel_write(channel, data + sent, len - sent);
        if (n == SSH_AGAIN) {
            delay(1);
            continue;
        }
        if (n <= 0) {
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return sent == len;
#else
    (void)data;
    (void)len;
    return false;
#endif
}

bool SshClient::resizePty(int columns, int rows)
{
#if ENABLE_SSH
    if (!connected()) {
        return false;
    }
    return ssh_channel_change_pty_size(static_cast<ssh_channel>(_channel), columns, rows) == SSH_OK;
#else
    (void)columns;
    (void)rows;
    return false;
#endif
}

bool SshClient::scpDownload(const SshProfile& profile, const String& remotePath, fs::FS& fs, const String& localPath, String& error)
{
#if ENABLE_SSH
    reportSshStage("scp.download.connect");
    ssh_session session = openSession(profile, error);
    if (!session) {
        return false;
    }
    ssh_scp scp = ssh_scp_new(session, SSH_SCP_READ, remotePath.c_str());
    if (!scp) {
        error = ssh_get_error(session);
        ssh_disconnect(session);
        ssh_free(session);
        return false;
    }
    bool ok = false;
    if (ssh_scp_init(scp) != SSH_OK) {
        error = ssh_get_error(session);
    } else {
        int request = ssh_scp_pull_request(scp);
        if (request == SSH_SCP_REQUEST_WARNING) {
            error = ssh_scp_request_get_warning(scp);
        } else if (request != SSH_SCP_REQUEST_NEWFILE) {
            error = "remote path is not a file";
        } else {
            uint64_t remaining = ssh_scp_request_get_size64(scp);
            File out = fs.open(localPath, FILE_WRITE);
            if (!out) {
                error = String("cannot open local file: ") + localPath;
                ssh_scp_deny_request(scp, "local file open failed");
            } else if (ssh_scp_accept_request(scp) != SSH_OK) {
                error = ssh_get_error(session);
                out.close();
            } else {
                uint8_t buffer[1024];
                ok = true;
                while (remaining > 0) {
                    size_t want = remaining > sizeof(buffer) ? sizeof(buffer) : static_cast<size_t>(remaining);
                    int n = ssh_scp_read(scp, buffer, want);
                    if (n <= 0) {
                        error = ssh_get_error(session);
                        ok = false;
                        break;
                    }
                    if (out.write(buffer, static_cast<size_t>(n)) != static_cast<size_t>(n)) {
                        error = "local write failed";
                        ok = false;
                        break;
                    }
                    remaining -= static_cast<uint64_t>(n);
                    delay(1);
                }
                out.close();
            }
        }
    }
    ssh_scp_close(scp);
    ssh_scp_free(scp);
    ssh_disconnect(session);
    ssh_free(session);
    reportSshStage(ok ? "scp.download.done" : "scp.download.failed");
    return ok;
#else
    (void)profile;
    (void)remotePath;
    (void)fs;
    (void)localPath;
    error = "ENABLE_SSH is disabled";
    return false;
#endif
}

bool SshClient::scpUpload(const SshProfile& profile, fs::FS& fs, const String& localPath, const String& remotePath, String& error)
{
#if ENABLE_SSH
    File in = fs.open(localPath, FILE_READ);
    if (!in || in.isDirectory()) {
        error = String("cannot open local file: ") + localPath;
        return false;
    }
    const size_t fileSize = in.size();
    String remoteDir = dirnameOf(remotePath);
    String remoteName = basenameOf(remotePath);

    reportSshStage("scp.upload.connect");
    ssh_session session = openSession(profile, error);
    if (!session) {
        in.close();
        return false;
    }
    ssh_scp scp = ssh_scp_new(session, SSH_SCP_WRITE, remoteDir.c_str());
    if (!scp) {
        error = ssh_get_error(session);
        in.close();
        ssh_disconnect(session);
        ssh_free(session);
        return false;
    }
    bool ok = false;
    if (ssh_scp_init(scp) != SSH_OK) {
        error = ssh_get_error(session);
    } else if (ssh_scp_push_file(scp, remoteName.c_str(), fileSize, 0644) != SSH_OK) {
        error = ssh_get_error(session);
    } else {
        uint8_t buffer[1024];
        ok = true;
        while (in.available()) {
            size_t n = in.read(buffer, sizeof(buffer));
            if (!n) {
                break;
            }
            if (ssh_scp_write(scp, buffer, n) != SSH_OK) {
                error = ssh_get_error(session);
                ok = false;
                break;
            }
            delay(1);
        }
    }
    in.close();
    ssh_scp_close(scp);
    ssh_scp_free(scp);
    ssh_disconnect(session);
    ssh_free(session);
    reportSshStage(ok ? "scp.upload.done" : "scp.upload.failed");
    return ok;
#else
    (void)profile;
    (void)fs;
    (void)localPath;
    (void)remotePath;
    error = "ENABLE_SSH is disabled";
    return false;
#endif
}

void SshClient::keepAlive()
{
#if ENABLE_SSH
    if (!_session || millis() - _lastKeepalive < 30000) {
        return;
    }
    _lastKeepalive = millis();
    ssh_send_ignore(static_cast<ssh_session>(_session), "keepalive");
#endif
}

String SshClient::knownHostsList()
{
    File in = LittleFS.open("/known_hosts", "r");
    return in ? in.readString() : String("");
}

bool SshClient::forgetHost(const String& hostSpec)
{
    String spec = hostSpec;
    spec.trim();
    if (spec.indexOf(':') < 0) {
        spec += ":22";
    }
    File in = LittleFS.open("/known_hosts", "r");
    if (!in) {
        return false;
    }
    String kept;
    bool removed = false;
    while (in.available()) {
        String line = in.readStringUntil('\n');
        line.trim();
        if (!line.length()) continue;
        if (line.startsWith(spec + " ")) {
            removed = true;
        } else {
            kept += line + "\n";
        }
    }
    in.close();
    if (removed) {
        File out = LittleFS.open("/known_hosts", "w");
        if (out) {
            out.print(kept);
        }
    }
    return removed;
}
