# Tab5 SSH Client

English | [日本語](README.ja.md)

M5Stack Tab5 firmware that turns the Tab5 into a portable SSH terminal. It is
built with PlatformIO and targets the Tab5 with the Tab5 Keyboard, microSD, and
Wi-Fi.

## Features

- PlatformIO project for M5Stack Tab5 / ESP32-P4.
- On-device Wi-Fi and SSH profile management persisted to flash.
- Single-peer WireGuard IPv4 tunnel for SSH over a VPN.
- Direct SSH command: `ssh user@host[:port] [password]`.
- Interactive SSH shell using `LibSSH-ESP32`.
- ANSI/VT-style terminal handling for shells, `vim`, `nano`, `sl`, and similar apps.
- Scrollback buffer and keyboard-driven command editing/history.
- Tab5 Keyboard, USB keyboard, and BLE keyboard configuration paths.
- US/JP keyboard layout mapping on the Tab5 side.
- SCP-style file transfer between SSH hosts and the Tab5 microSD card.
- Linux-like local CLI for SD files, Wi-Fi, SSH/SCP, diagnostics, and Python.
- Embedded MicroPython runner for REPL, `python -c`, and SD-card scripts.
- MicroPython graphics API backed by the firmware M5GFX sprite.

## Documentation

- [Command list](docs/COMMANDS.md)
- [コマンド一覧](docs/COMMANDS.ja.md)
- [Python and graphics](docs/PYTHON.md)
- [Third-party licenses](THIRD_PARTY_LICENSES.md)

## Hardware

- M5Stack Tab5
- Tab5 Keyboard
- microSD card
- USB cable for flashing and serial diagnostics
- Wi-Fi network reachable by the Tab5

## Build

Install PlatformIO, open this folder, then build the `tab5` environment.

```powershell
pio run -e tab5
```

On Windows consoles, use UTF-8 if PlatformIO output fails with encoding errors.

```powershell
$env:PYTHONUTF8='1'; pio run -e tab5
```

Upload firmware:

```powershell
pio run -e tab5 -t upload
```

For a complete device flash, including bootloader, partition table, firmware,
and LittleFS profiles, use:

```powershell
.\tools\flash_tab5.ps1 -Port COM4
```

If `data/profiles.local.json` exists, this command temporarily uses that ignored
local profile file for the LittleFS image, then restores the public
`data/profiles.json`. For a clean public image for M5Burner export:

```powershell
.\tools\flash_tab5.ps1 -Port COM4 -UseLocalProfiles:$false -EraseFirst
```

## Configuration

Profiles can be edited on the Tab5 UI and are persisted to flash.

- `WIFI`: saved Wi-Fi profiles, scan, add, edit, delete, connect, Wi-Fi on/off.
- `BT`: Bluetooth keyboard on/off, scan, pair, forget, choose the active device.
- `WG`: WireGuard status, on/off, import a `.conf` from the SD card, set up from another device, edit fields.
- `SSH`: saved SSH profiles (add, edit, delete, connect) and the `KEYS` page for SSH key login.
- `THEME`: 12 color schemes, terminal font size, interface text size (Normal/Large/X-Large), and line
  spacing. Pick what you want, then tap `APPLY`; nothing changes (or redraws) until you do.
- `SSH` > `SNIPPETS`: saved commands; `RUN` sends the command plus Enter to the open session.
- `CONF`: device name, region, UTC offset, NTP, keymap, Ctrl/Caps swap, navigation (`top` tab strip or
  `side` rail), and session logging (SSH output saved to `/logs` on the SD card). Fields with a `v`
  button have a dropdown list; most also accept typed values.

Do not commit real Wi-Fi passwords or SSH credentials.

## Usage

1. Upload the firmware.
2. Reboot the Tab5.
3. Configure Wi-Fi from the `WIFI` screen.
4. Configure SSH from the `SSH` screen.
5. Select a profile and press `CONNECT`.

You can also connect from the local CLI:

```text
ssh list
ssh connect 0
ssh demo@192.0.2.10:22
```

If a direct SSH command omits the password, the firmware tries to reuse
credentials from a saved profile with the same host/user or host/user/port.

## On-Device Controls

- `Esc`: switch focus between the terminal/content area and the top menu bar.
- `Tab`: move focus in menus, lists, and edit fields.
- Arrow keys: move focus in menus/settings, or send cursor movement to terminal apps.
- `Ctrl+Up` / `Ctrl+Down`: scroll the terminal buffer.

When an SSH session is active on the terminal screen, `Esc` is sent to the
remote application so `vim` can leave insert mode.

## Built-In CLI

The built-in CLI is Linux-like, not a full POSIX shell. There are no pipelines,
redirection, shell expansion, or background jobs.

```text
help
man <command>
status
wifi status
wifi off
wifi on
ssh list
ssh connect 0
ssh user@host[:port] [password]
ls /
ls -lah /
df
mkdir /scripts
rmdir /scripts
scp get /home/demo/test.py /test.py 0
scp put /test.py /home/demo/test.py 0
python -c print('hello')
```

Normal `ls` uses multi-column output; `ls -l` uses one file per line.

## MicroPython And Graphics

The local CLI can start the embedded REPL or run `.py` files from microSD:

```text
python
python -c print('hello')
python -c print('hello')
```

Scripts receive `argv` and a global `gfx` object. Drawing commands render into
the firmware sprite, and `gfx.present()` pushes that sprite to the display.
Graphics scripts can be interrupted at `gfx.present()` with `Ctrl-C` or `q`.

See [docs/PYTHON.md](docs/PYTHON.md) for the API.

## Tailscale Hosts

This firmware does not run a Tailscale node on the ESP32-P4. To connect to a
tailnet host, put the Tab5 on a network that has a Tailscale gateway, subnet
router, tethered Tailscale device, or SSH relay, then configure the SSH profile
with the reachable address and port.

## Host keys

The first key a server presents is saved to `/known_hosts` (trust on first use). If it later
changes, the connection is refused and the message says how to accept the new key:
`sshhost forget <host[:port]>`. `sshhost list` shows what is trusted. An idle session sends an
SSH keepalive every 30 seconds.

## SSH keys

Open `SSH` > `KEYS`. Keys are stored in the Tab5's internal flash (`/keys`), never on the SD card.

- `GENERATE` creates an ed25519 key; its public key is shown, printed in the `TERM` tab and serial
  log, and copied to `/ssh/<name>.pub` on the SD card.
- Existing keys can be imported from the SD card (`/ssh`, `/.ssh`, `/keys`, or `id_*` in the root)
  or uploaded through `REMOTE SETUP`; that page can also download the public keys.
- `USE FOR SSH` assigns the selected key to the active SSH profile (or pick it in the profile's
  `SSH key` dropdown). If the key is rejected, the profile's password is tried when one is set.
- Encrypted keys: enter the passphrase in the profile's `Key pass` field.

CLI: `sshkey list|gen [name]|pub <name>|rm <name>|import <sd-path> [name]|use <name> [profile]`.

## WireGuard

WireGuard is always off after boot and after a new config is saved or imported. Turn it on with
`WG ON` (or `wg on`), or set `CONF` > `WG at boot` to `Always on` to start it automatically.

The easiest way is the `WG` tab. Either copy a standard `wg-quick` `.conf` file to the SD card
(`/wg` is searched first; `RESCAN SD` searches every folder, and only files that really contain a WireGuard `[Interface]` are listed) and tap it in the list, or tap `REMOTE SETUP`: the Tab5 serves a
page on your Wi-Fi (`http://<tab5-ip>:8080`) where you paste or upload the `.conf` from a phone or
laptop. The page needs the PIN shown on the Tab5, closes after one upload, after five wrong PINs,
or after five minutes. It uses plain HTTP, so use it only on a network you trust. A `PresharedKey`
and `PersistentKeepalive` in the file are honoured; only the first `[Peer]` is used.

You can also configure a WireGuard peer through the local CLI or the 115200 baud serial API.
The server must have a peer entry for this Tab5's public key and tunnel IP.
Use the address from the client `[Interface] Address` line without its CIDR
suffix, and use the server `[Peer] Endpoint` host and port.

```text
wg set address 10.13.13.2
wg set private-key <client-private-key-base64>
wg set endpoint vpn.example.net
wg set port 51820
wg set public-key <server-public-key-base64>
wg on
wg status
```

`wg off` closes the tunnel; `wg reset` clears its saved settings. Settings persist in LittleFS, and an enabled tunnel
restarts after Wi-Fi reconnects and NTP has set the clock. `wg status` says
"interface up; waiting for handshake" once the network interface starts and
"connected (handshake ok)" after the peer has answered. Test access by connecting to an SSH
host reachable only through the tunnel.

This implementation supports one IPv4 peer, routes all outgoing IPv4 traffic
through it while active, and does not support preshared keys or custom
AllowedIPs. It uses the BSD-licensed WireGuard-ESP32-Arduino lwIP implementation
with an ESP-IDF 5.5 compatibility change. Keys are stored in the same
unencrypted LittleFS settings file as Wi-Fi and SSH credentials. Serial command
echo redacts `wg set private-key`, but avoid sharing serial logs containing
other credentials.

## Tab5 Hardware Revisions

M5Stack reports a change from ILI9881C display plus GT911 touch to integrated
ST7123 on October 14, 2025, then ST7121 on April 28, 2026. This project
uses M5Unified and M5GFX auto detection; M5Stack specifies M5Unified 0.2.23+
and M5GFX 0.2.30+ for stable reset on the newer panels. The build currently
resolves 0.2.25 and 0.2.32. See the [official Tab5 hardware notes](https://docs.m5stack.com/en/core/Tab5).

## Serial Diagnostics

The firmware exposes a serial API at `115200` baud for diagnostics:

```text
help
status
sd ls /
wifi status
ssh list
ssh connect [index]
ssh disconnect
term dump
python /life.py 0 5
```

When opening the serial port from host tools, avoid unnecessary DTR/RTS
transitions because they may reset the board.

## Repository Layout

```text
data/       LittleFS profile data
docs/       Documentation
include/    Headers
lib/        Embedded MicroPython and local libraries
src/        Firmware source
tools/      Helper scripts
```

## Status

This is experimental firmware for Tab5 hardware bring-up and mobile SSH use.
Expect to tune Wi-Fi behavior, terminal escape handling, performance, fonts,
and keyboard mappings for your own setup.

## License

Project-owned source code is distributed under the [MIT License](LICENSE).
Third-party components remain under their own licenses; see
[THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md). In particular, SSH/SCP
support uses `LibSSH-ESP32` / `libssh`, which is LGPL-2.1-or-later.
