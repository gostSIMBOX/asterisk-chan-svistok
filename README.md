# chan_svistok - Asterisk Channel Driver for Huawei UMTS Dongles

[🇷🇺 Русский](README_ru.md) | [🇬🇧 English](README.md)

## Overview

**chan_svistok** is a fork of the chan_dongle Asterisk PBX channel driver, enhanced by **Anton Dodonov** and the **Native Mind** team. It enables Huawei UMTS 3G dongles to function as telephony channels, supporting voice calls, SMS, and USSD operations.

### What's New in chan_svistok

Compared to the original chan_dongle, chan_svistok includes the following enhancements:

| Feature | chan_dongle | chan_svistok |
|---------|-------------|--------------|
| State persistence | Limited | Extended file-based storage |
| CLI commands | Basic | 23 commands with tab completion |
| Firmware flashing | External tools | Built-in Qualcomm DIAG programmer |
| Device limits | No | Per-device call/balance limits |
| Balance tracking | Manual | Automatic with ballast support |
| Group management | Basic | IMSI-based group assignment |
| Device binding | By IMSI only | AT^SN serial number binding |
| IMEI management | Static | Change IMEI on the fly via CLI |
| **Modem stability** | Basic reconnect | Enhanced disconnect detection & recovery |
| **Restart mechanism** | Simple restart | Graceful/convenient restart with state machine |
| Documentation | Minimal | Full SDD flows + ADRs |
| Call statistics | Basic | Extended with ACD calculation |
| Error handling | Standard | Enhanced with persistence |
| Logging | Standard | Extended with per-device logs |

#### Modem Stability Improvements

**Fixed modem disconnections:**
- Enhanced `port_status()` detection for USB device availability
- Automatic device state recovery on communication failure
- Improved monitor thread restart on device reconnect
- Better handling of device disappearance (USB unplug)

** Redesigned restart mechanism:**
- **3-stage restart states**: `stop` → `restart` → `remove` → `start`
- **Graceful restart options**:
  - `restart now` - Immediate restart
  - `restart gracefully` - Wait for active calls to complete
  - `restart when convenient` - Wait until no active channels
- **State machine**: `desired_state` vs `current_state` tracking
- **Automatic cleanup**: Proper resource release before restart
- **Config-triggered restart**: Auto-restart on critical config changes (TTY, IMEI, IMSI)

### Original Project

This project is based on **chan_dongle** by Artem Makhutov, Dmitry Vagin, and bg <bg_one@mail.ru>.

**Original Project Home**: http://code.google.com/p/asterisk-chan-dongle/

## Features

- 📞 **Voice Calls** - Place and receive calls via 3G dongles
- 📨 **SMS** - Send and receive text messages (PDU mode, UCS-2 encoding)
- 📟 **USSD** - Execute USSD codes for balance checks and service menus
- 🔄 **Multi-Device** - Support for multiple dongles with grouping and load balancing
- 🔧 **Firmware Flashing** - Update device firmware via Qualcomm DIAG protocol
- 💻 **CLI Management** - 23 Asterisk CLI commands for device control

## Supported Devices

| Device | Model | Status |
|--------|-------|--------|
| Huawei K3715 | ✓ | Supported |
| Huawei E169 / K3520 | ✓ | Supported |
| Huawei E155X | ✓ | Supported |
| Huawei E175X | ✓ | Supported |
| Huawei K3765 | ✓ | Supported |

## Requirements

- **OS**: Linux 2.6.33+ or FreeBSD 8.0+
- **Asterisk**: Compatible version with channel driver support
- **Hardware**: Huawei UMTS dongle with USB interface
- **SIM**: PIN code disabled

## Source ownership and build

This tree is composed from two immutable references rather than a flattened
fork:

- `asterisk-chan-dongle/` owns every implementation proven unchanged.
- `src/` contains the 12 Svistok-only and 28 modified legacy paths, plus small
  target-owned ABI/state integration files.
- `manifests/module-files.json` is the exhaustive 52-path module closure and
  109-path `DO NOT COPY` list. The 12 identical paths are not duplicated in
  `src`.
- `manifests/symbol-ownership.json` assigns functions, file data, macros,
  declarations, and directly included `.c` definitions to one owner.

The build generates two build-only slices for each of the 16 modified root
translation units. Unchanged bodies compile from `asterisk-chan-dongle/`;
modified/new bodies compile from the copied code in `src/`. Hidden
`svistok_bridge_*` aliases/wrappers connect cross-slice static functions and
state. Generated files are written only below `build/`.

Build against installed Asterisk headers (the include directory must contain
`asterisk.h` and `asterisk/`):

```sh
make module ASTERISK_INCLUDE=/usr/include BUILD_DIR=build/module
```

The result is `build/module/chan_svistok.so`. A host-only maximum compilation
using compatibility headers is available for deterministic CI checks:

```sh
make compatibility-module BUILD_DIR=build/compatibility
```

That compatibility artifact proves slicing, linking, symbol ownership, and ABI
contracts; it is not loadable into a real Asterisk process. Full module
load/unload and modem discovery/call/SMS/USSD/programming scenarios require a
compatible Linux/Asterisk installation and supported hardware. The exact T3/T4
gaps are recorded in `manifests/effective-legacy-config.json`.

Both `legacy/asterisk-chan-svistok-v2014` and
`asterisk-chan-dongle/` are read-only inputs. The build and tests enforce their
commits and clean status with `tools/check_source_guards.py`.

## Project Structure

```
chan_svistok/
├── flows/                           # Development documentation
│   ├── legacy/                      # Reverse engineering analysis
│   │   ├── understanding/           # Domain understanding tree
│   │   ├── mapping.md               # Node to flow mapping
│   │   └── review.md                # Review items
│   ├── sdd-at-command-protocol/     # AT command protocol spec
│   ├── sdd-call-management/         # Call management spec
│   ├── sdd-device-communication/    # Serial communication spec
│   ├── sdd-sms-ussd-handling/       # SMS/USSD handling spec
│   ├── sdd-cli-management/          # CLI management spec
│   ├── sdd-device-programming/      # Firmware flashing spec
│   ├── sdd-state-persistence/       # State storage spec
│   ├── adr-001-at-command-queue/    # ADR: Queue architecture
│   ├── adr-002-pdu-mode-sms/        # ADR: PDU mode decision
│   ├── adr-003-file-state-persistence/ # ADR: File storage
│   ├── adr-004-qualcomm-diag/       # ADR: DIAG protocol
│   ├── adr-005-115200-baud/         # ADR: Serial configuration
│   └── adr-006-8-state-call-machine/ # ADR: Call state machine
├── chan_svistok/                    # Main source code
│   ├── at_command.*                 # AT command layer
│   ├── at_response.*                # Response parsing
│   ├── at_queue.*                   # Command queue
│   ├── channel.*                    # Asterisk channel driver
│   ├── cpvt.*                       # Call private data
│   ├── tty_v2.*                     # Serial communication
│   ├── pdu.*                        # SMS PDU handling
│   ├── char_conv.*                  # Character encoding
│   ├── cli.*                        # CLI commands
│   ├── ttyprog_*.*                  # Firmware flashing
│   └── share.*                      # State persistence
└── README.md                        # This file
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Asterisk PBX Core                        │
├─────────────────────────────────────────────────────────────┤
│ chan_dongle Channel Driver                                  │
│  ┌─────────────┐ ┌──────────────┐ ┌──────────────┐         │
│  │ Channel Mgmt│ │ CLI Interface│ │ Manager API  │         │
│  │ (channel.c) │ │   (cli.c)    │ │ (manager.c)  │         │
│  └─────────────┘ └──────────────┘ └──────────────┘         │
│  ┌──────────────────────────────────────────────┐           │
│  │           AT Command Protocol Layer          │           │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────────┐ │           │
│  │  │ Commands │ │ Responses│ │    Queue     │ │           │
│  │  │(at_cmd.c)│ │(at_resp.c)│ │ (at_queue.c) │ │           │
│  │  └──────────┘ └──────────┘ └──────────────┘ │           │
│  └──────────────────────────────────────────────┘           │
│  ┌──────────────────┐ ┌────────────────────────────┐        │
│  │ Device Comm (TTY)│ │ State Persistence (Files)  │        │
│  │  (tty_v2.c)      │ │      (share.c)             │        │
│  └──────────────────┘ └────────────────────────────┘        │
├─────────────────────────────────────────────────────────────┤
│  Firmware Flasher (ttyprog_*.c) - Qualcomm DIAG protocol    │
└─────────────────────────────────────────────────────────────┘
                          │
                    ┌─────┴─────┐
                    │ Huawei    │
                    │ UMTS      │
                    │ Dongle    │
                    └───────────┘
```

## Documentation

### SDD Flows (Spec-Driven Development)

| Flow | Description |
|------|-------------|
| [AT Command Protocol](flows/sdd-at-command-protocol/) | 60+ AT commands, queue management |
| [Call Management](flows/sdd-call-management/) | 8-state call machine, channel driver |
| [Device Communication](flows/sdd-device-communication/) | 115200 baud serial, HDLC framing |
| [SMS/USSD Handling](flows/sdd-sms-ussd-handling/) | PDU encoding, UCS-2 conversion |
| [CLI Management](flows/sdd-cli-management/) | 23 CLI commands |
| [Device Programming](flows/sdd-device-programming/) | Qualcomm DIAG firmware flashing |
| [State Persistence](flows/sdd-state-persistence/) | File-based KV storage |

### ADRs (Architecture Decision Records)

| ADR | Title | Type |
|-----|-------|------|
| [ADR-001](flows/adr-001-at-command-queue/) | AT command queue architecture | enabling |
| [ADR-002](flows/adr-002-pdu-mode-sms/) | PDU mode vs text mode SMS | constraining |
| [ADR-003](flows/adr-003-file-state-persistence/) | File-based state persistence | enabling |
| [ADR-004](flows/adr-004-qualcomm-diag/) | Qualcomm DIAG protocol | enabling |
| [ADR-005](flows/adr-005-115200-baud/) | 115200 baud serial communication | constraining |
| [ADR-006](flows/adr-006-8-state-call-machine/) | 8-state call machine | enabling |

### Legacy Analysis

The [legacy analysis](flows/legacy/) contains reverse-engineered documentation from the original codebase:

- [Understanding Tree](flows/legacy/understanding/) - Domain knowledge
- [Mapping](flows/legacy/mapping.md) - Code to documentation mapping
- [Review](flows/legacy/review.md) - Items for human review

## CLI Commands

### Show Commands
```
dongle show devices              # List all devices
dongle show devicesl             # List with limits
dongle show device settings      # Show device settings
dongle show device state         # Show device state
dongle show device statistics    # Show statistics
dongle show version              # Show driver version
```

### Control Commands
```
dongle cmd <device> <at_command> # Execute raw AT command
dongle sms <device> <number> <msg> # Send SMS
dongle ussd <device> <code>      # Send USSD
dongle reset <device>            # Reset device
dongle start <device>            # Start device
dongle reload                    # Reload configuration
```

### Configuration Commands
```
dongle setgroup <device> <group> # Set device group
dongle setgroupimsi <imsi> <g>   # Set group by IMSI
dongle callwaiting <device>      # Toggle call waiting
```

## Technical Specifications

### Serial Communication
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: Hardware (CRTSCTS)

### SMS
- **Mode**: PDU (binary)
- **Encoding**: UCS-2 / 7-bit GSM
- **Max Length**: 140 octets (160 chars 7-bit, 70 chars UCS-2)

### Call States
1. ACTIVE
2. ONHOLD
3. DIALING
4. ALERTING
5. INCOMING
6. WAITING
7. RELEASED
8. INIT

## Installation

```bash
# Configure
./configure

# Build
make

# Install
make install
```

See [INSTALL](chan_svistok/INSTALL) for detailed instructions.

## Configuration

### dongle.conf Example
```ini
[general]
interval = 1
discoveryinterval = 300
u2diag = 0
initcmd = AT+CGMI

[device0]
device = /dev/ttyUSB0
imsi = 250000000000000
group = 1
```

## Dialplan Examples

```asterisk
[dongle-incoming]
exten => sms,1,Verbose(Incoming SMS from ${CALLERID(num)})
exten => sms,n,System(echo '${BASE64_DECODE(${SMS_BASE64})}' >> /var/log/sms.txt)
exten => sms,n,Hangup()

exten => ussd,1,Verbose(Incoming USSD: ${BASE64_DECODE(${USSD_BASE64})})
exten => ussd,n,Hangup()

exten => _X.,1,Dial(Dongle/r1/${EXTEN})
exten => _X.,n,Hangup()
```

## License

GNU General Public License Version 2. See [LICENSE.txt](chan_svistok/LICENSE.txt) for details.

## Credits

### chan_svistok Development
- **Lead Developer**: Anton Dodonov
- **Company**: [Native Mind](https://nativemind.net)

### Original chan_dongle Authors
- **Original Authors**: Artem Makhutov, Dmitry Vagin
- **Maintainer**: bg <bg_one@mail.ru>
- **Project Home**: http://code.google.com/p/asterisk-chan-dongle/
- **Wiki**: http://wiki.e1550.mobi

## Disclaimer

This software is provided "as is" without warranty. The authors are not responsible for any damages or unexpected charges on your SIM card. Use at your own risk.

## Links

- [🇷🇺 Русская версия](README_ru.md)
- [Legacy Analysis](flows/legacy/)
- [SDD Flows](flows/)
- [ADRs](flows/)
