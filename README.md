# asterisk_chan_simbox

[🇷🇺 Русский](README_ru.md) | [🇬🇧 English](README.md)

## Overview

**asterisk_chan_simbox** is an industrial-grade fork of the [chan_dongle](http://code.google.com/p/asterisk-chan-dongle/) Asterisk channel driver, developed by **Anton Dodonov** and the **[Native Mind](https://nativemind.net)** team. Originally known as **chan_svistok**, the project evolved far beyond the original chan_dongle scope during 2+ years of production operation managing **500+ Huawei UMTS 3G modems** simultaneously.

This repository consolidates:
- **`asterisk_chan_svistok/`** — the production-hardened driver (read-only reference)
- **`asterisk_chan_dongle/`** — upstream community forks for Asterisk 20+ compatibility reference (read-only)
- **`adapters/`** + **`src/`** — new adapter layer (Strangler Fig architecture) for standalone operation without Asterisk

---

## Major Enhancements vs. Original chan_dongle

### 🔍 Node Discovery — 3 Generations of Device Auto-Detection

The original chan_dongle has a single, primitive device discovery mechanism (`pdiscovery.c`). chan_svistok evolved through **three generations** of discovery, each solving real operational problems at scale:

| Generation | File(s) | Architecture | Use Case |
|------------|---------|-------------|----------|
| **Gen 1** — Legacy | `pdiscovery.c` | In-process, synchronous | Original chan_dongle approach. Blocks Asterisk during USB enumeration. Works for 1–5 modems. |
| **Gen 2** — In-Process Async | `simnode/adiscovery_core.c` | In-process, async with threading | Currently used by the live Asterisk module. Non-blocking discovery for dozens of modems. |
| **Gen 3** — Standalone Daemon | `simnode/adiscovery_core_new.c` + `adiscovery_simnode.c` | External daemon, IPC | Fully decoupled from Asterisk (`#ifdef IN_SIMBOX` macro-gates Asterisk includes). Designed for 100+ modem nodes, can run independently on headless devices. |

**What the discovery system actually does:**
- Enumerates all `/dev/ttyUSB*` devices on the system
- Sends AT identification commands (`ATI`, `AT+CIMI`, `AT^SN`) to each port
- Matches discovered modems against configuration by **Serial Number (S/N)** — not IMEI
- Handles USB hub topology for multi-port gateway boards
- Caches discovered devices to speed up subsequent scans

### 🔧 Programmator — Built-in Qualcomm DIAG Firmware Flasher

The original chan_dongle has **no firmware management** capabilities whatsoever. chan_svistok includes a complete, built-in **Qualcomm DIAG protocol** programmer for Huawei modems (E1550, E173, E171):

```
chan_svistok/programmator/
├── ttyprog_programmator.c    # Main programmer entry point
├── ttyprog_core.c            # DIAG protocol implementation
├── tty_v2.c                  # Low-level serial I/O for DIAG mode
└── addons.c                  # State persistence & progress tracking
```

**Key capabilities:**
- **Flash custom firmware** via DIAG mode — modem must be switched to DIAG mode first
- **Track flashing progress** with state machine: `init` → `diag` → `wait` → `flash` → `done`
- **Per-device state persistence** during multi-step firmware operations
- **Custom firmware features** implemented:
  - Correct **S/N (Serial Number)** reporting via `AT^SN` — for reliable device identification when no SIM card is inserted
  - Single AT command **IMEI change** — without reflashing (see below)

#### 🛡️ Modem Recovery & Reliability Statistics

> **Production reliability over 2 years of continuous operation with 500+ modems:**
>
> - 🟢 **Only 1 modem died** out of 500 — and likely from age/hardware failure, not from flashing
> - 🟢 **Successfully recovered all but 2** out of 10+ bricked modems brought in for repair
> - 🟢 The 2 unrecoverable modems had **hardware-level damage** beyond DIAG mode repair

**Recovery process for bricked modems:**
1. Force modem into **DIAG mode** (Qualcomm diagnostic mode via USB)
2. Use the built-in programmator to reflash custom firmware
3. Modem reboots with correct S/N, default AT configuration, and working state
4. Auto-discovery picks it up and brings it online

**Auto-recovery of "stuck" modems:**
- Enhanced `port_status()` detection for USB device availability
- Automatic device state recovery on communication failure
- Improved monitor thread restart on device reconnect
- Auto-reset to default state when modem becomes unresponsive

### 📖 Reader — SIM Card APDU Reader (No Radio Module)

The original chan_dongle has **no SIM reader support**. chan_svistok includes a standalone `reader/` module for working with **ttyUSB SIM card readers** using APDU (Application Protocol Data Units):

```
chan_svistok/reader/
├── reader_core.c    # APDU command implementation, ATR parsing
└── reader_core.h    # Reader API definitions
```

**What the reader does:**
- Communicates with USB SIM card readers (no GSM radio module needed)
- Sends/receives APDU commands to SIM cards
- Parses ATR (Answer to Reset) responses
- Controls RTS/DTR lines for reader hardware reset
- Completely independent from the modem/call surface — no calls, no SMS, no network registration

### 🔑 IMEI Change via Single AT Command (No Reflashing!)

In the original chan_dongle, IMEI is static and read-only.

In chan_svistok with **custom firmware**, IMEI can be changed **on the fly** with a single AT command via the Asterisk CLI:
```
dongle cmd <device> AT+EGMR=1,7,"<new_IMEI>"
```

- No reflashing required
- Takes effect immediately
- Auto-triggers device restart when IMEI config changes (critical config change detection)
- Persisted across reboots in the modem's NVRAM

### 🆔 Device Identification by S/N (Not IMEI)

This is a **critical architectural difference** from the original chan_dongle:

| Aspect | chan_dongle | chan_svistok |
|--------|-----------|-------------|
| **Device identification** | By IMEI | By **Serial Number (S/N)** via `AT^SN` |
| **Without SIM card** | Cannot reliably identify device | ✅ S/N always available regardless of SIM |
| **After IMEI change** | Device identity lost | ✅ S/N stays constant, device tracked correctly |
| **Binding in config** | `imei=` field | `sn=` field — matches hardware S/N |

**Why S/N matters for simbox operations:**
- When managing 500+ modems, SIM cards are frequently swapped between devices
- IMEI may be changed for operational reasons
- The **hardware serial number** is the only truly persistent identifier
- Without a SIM card inserted, IMEI may not be readable — but S/N always is
- Custom firmware ensures correct S/N reporting via `AT^SN` response

### 🔄 Enhanced Modem Stability & Auto-Recovery

| Feature | chan_dongle | chan_svistok |
|---------|-----------|-------------|
| Disconnect detection | Basic | Enhanced `port_status()` USB monitoring |
| Recovery on failure | Simple reconnect | Full state machine recovery |
| Restart options | Single restart | 3 modes: `now`, `gracefully`, `when convenient` |
| Restart state machine | None | `stop` → `restart` → `remove` → `start` |
| Config change restart | Manual | Automatic on critical changes (TTY, IMEI, IMSI) |
| State tracking | None | `desired_state` vs `current_state` |
| Resource cleanup | Basic | Full resource release before restart |

---

## Asterisk 20+ Compatibility

chan_svistok was originally built against an **older Asterisk base** (pre-opaque `ast_channel` era, roughly Asterisk 1.8–11). The Asterisk API underwent breaking changes in versions 12–20+.

Two community forks document and solve these compatibility issues. Both are included in this repository as **read-only references**:

### [pulpoff/asterisk-chan-dongle](https://github.com/pulpoff/asterisk-chan-dongle)

This fork adds **Asterisk 20 support** and documents the exact API migration needed:

| Breaking Change | Old API (Asterisk 1.8) | New API (Asterisk 20+) |
|----------------|----------------------|----------------------|
| Channel structure | Direct `ast_channel` field access | Opaque structure with accessor functions |
| Format capabilities | Legacy format API | `ast_format_cap` / `ast_format_slin` |
| Channel allocation | `ast_channel_alloc(...)` | `ast_channel_alloc(...)` with `assignedids`/`requestor` params |
| Bridge peer | `ast_bridged_channel()` | `ast_channel_bridge_peer()` |
| Channel request | Old callback signature | Updated `channel_request` callback |
| Module registration | Basic `AST_MODULE_INFO` | Extended with `load_pri`/`support_level` |
| Version macro | `ASTERISK_FILE_VERSION` | Removed (deprecated) |

Tested with **Asterisk 20.6.0** and Huawei E1762. Also provides Docker deployment (docker-compose) for quick start on Armbian/ARM boards.

### [wdoekes/asterisk-chan-dongle](https://github.com/wdoekes/asterisk-chan-dongle)

This fork (the most popular community fork, originally by bg111) provides:

- **`ast_compat.h` / `ast_config.h`** — a version-compatibility shim that adapts to different Asterisk versions at compile time. This is the closest prior art to the adapter shim being built in this project.
- **`smsdb.c/h`** — SMS persistence database (chan_svistok uses file-based persistence instead)
- **`gsm7_luts.h`** — GSM 7-bit encoding lookup tables
- **`error.c/h`** — structured error handling
- Supports Asterisk 14+ with automatic version detection during build
- Includes **Jitter buffer** and **AGC (Automatic Gain Control)** configuration guidance

Both forks are used as **pattern and migration knowledge references only** — their code is not merged into chan_svistok. The adapter layer in `adapters/`/`src/` builds compatibility against chan_svistok's specific API shape.

---

## Full Feature Comparison

| Feature | chan_dongle (original) | chan_svistok |
|---------|----------------------|-------------|
| **Device Discovery** | Single-gen pdiscovery | 3 generations (sync → async → daemon) |
| **Firmware Flashing** | ❌ None | ✅ Built-in Qualcomm DIAG programmer |
| **Bricked Modem Recovery** | ❌ None | ✅ DIAG mode recovery (98% success rate) |
| **SIM Card Reader** | ❌ None | ✅ APDU reader module |
| **IMEI Change** | ❌ Static, read-only | ✅ Single AT command, no reflash |
| **Device ID** | By IMEI | By S/N (`AT^SN`) |
| **ID without SIM** | ❌ Unreliable | ✅ Always works via S/N |
| **State Persistence** | Limited | Extended file-based storage |
| **CLI Commands** | ~10 basic | 23 commands with tab completion |
| **Device Limits** | ❌ None | ✅ Per-device call/balance limits |
| **Balance Tracking** | Manual | Automatic with ballast support |
| **Group Management** | Basic | IMSI-based group assignment |
| **Modem Stability** | Basic reconnect | Enhanced disconnect detection & recovery |
| **Restart Mechanism** | Simple | 3-stage state machine (gracefully/convenient) |
| **Call Statistics** | Basic | Extended with ACD calculation |
| **Per-device Logging** | ❌ None | ✅ Extended per-device logs |
| **Asterisk 20+** | ❌ | ✅ (via pulpoff/wdoekes compatibility) |
| **Scale (tested)** | 1–5 modems | **500+ modems** in production |
| **Documentation** | Minimal | Full SDD flows + 6 ADRs |

---

## Repository Structure

```
asterisk_chan_simbox/
├── asterisk_chan_svistok/              # READ-ONLY. Production-hardened driver
│   └── chan_svistok/                   #   Source code, programmator, reader, simnode
│       ├── simnode/                    #   Discovery generations (Gen 2 & 3)
│       ├── programmator/              #   Qualcomm DIAG firmware flasher
│       ├── reader/                    #   SIM card APDU reader
│       └── tools/                     #   Standalone utilities (discovery tool)
├── asterisk_chan_dongle/               # READ-ONLY. Community reference forks
│   ├── asterisk-chan-dongle-by-wdoekes/  # ast_compat.h, smsdb, Asterisk 14+
│   └── asterisk-chan-dongle-by-pulpoff/  # Asterisk 20 support, Docker
├── adapters/                           # NEW: Asterisk-API-compatible shim
├── src/                                # NEW: Standalone bridge code
└── flows/                              # SDD development flows
    └── sdd-asterisk-chan-simbox/        # Active spec-driven development
```

> ⚠️ **Hard rule**: `asterisk_chan_svistok/` and `asterisk_chan_dongle/` are **permanently read-only**. All new code goes in `adapters/` and `src/`.

## Supported Devices

| Device | Model | Status |
|--------|-------|--------|
| Huawei E1550 | ✓ | Primary target, custom firmware available |
| Huawei E173 | ✓ | Custom firmware available |
| Huawei E171 | ✓ | Custom firmware available |
| Huawei K3715 | ✓ | Supported |
| Huawei E169 / K3520 | ✓ | Supported |
| Huawei E155X | ✓ | Supported |
| Huawei E175X | ✓ | Supported |
| Huawei K3765 | ✓ | Supported |

## Requirements

- **OS**: Linux 2.6.33+ (production tested on Armbian, OpenWRT)
- **Asterisk**: 1.8–20+ (with compatibility shim)
- **Hardware**: Huawei UMTS dongle with USB interface
- **SIM**: PIN code must be disabled

## License

GNU General Public License Version 2. See [LICENSE](asterisk_chan_svistok/LICENSE) for details.

## Credits

### chan_svistok / chan_simbox Development
- **Lead Developer**: Anton Dodonov
- **Company**: [Native Mind](https://nativemind.net)

### Original chan_dongle Authors
- **Original Authors**: Artem Makhutov, Dmitry Vagin
- **Maintainer**: bg <bg_one@mail.ru>
- **Project Home**: http://code.google.com/p/asterisk-chan-dongle/

### Community Forks (Asterisk 20+ Compatibility)
- **pulpoff**: [github.com/pulpoff/asterisk-chan-dongle](https://github.com/pulpoff/asterisk-chan-dongle)
- **wdoekes**: [github.com/wdoekes/asterisk-chan-dongle](https://github.com/wdoekes/asterisk-chan-dongle)
