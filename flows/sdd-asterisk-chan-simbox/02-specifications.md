# Specifications: asterisk-chan-simbox

> Version: 1.1
> Status: REOPENED — §9 amendment pending approval, rest still valid
> Last Updated: 2026-08-22

**Everything below through §8 describes the Asterisk-compatibility
shim (`adapters/`) — confirmed still accurate; chan_svistok's 20 files
really do compile against it.** §9 is new: it specifies how `src/`'s
public API must actually *drive* that shimmed chan_svistok, which the
delivered implementation didn't do — see
`01-requirements.md`'s "Amendment 2026-08-22" for the full finding.

## 1. Overview

This document specifies the Asterisk-API-compatible shim and adapter layer
that allows `chan_svistok`'s unmodified source code to compile, link, and run
**without a real Asterisk installation**. The shim lives in `adapters/` and
`src/`, built around the read-only `asterisk_chan_svistok/` tree using a
Strangler Fig approach.

### Scope

- Shim exactly the `ast_*` subset chan_svistok actually calls (not all of Asterisk)
- Expose a clean C API (no Asterisk types) for `flutter_gsm` FFI binding
- Separate adapter surfaces for modem driver, programmator, reader, and discovery
- Do **not** modify any read-only reference trees

### Out of Scope

- Porting wdoekes/pulpoff features (smsdb, GSM7 LUTs) into the shim
- Real Windows/macOS driver (stub-first)
- Asterisk module compatibility (the shim replaces Asterisk, not coexists)

---

## 2. ast_* Symbol Inventory

### 2.1 Function Inventory (~115 unique functions)

Exhaustive grep of `chan_svistok/` (excluding `simnode/`, `programmator/`,
`reader/` which are already decoupled or Asterisk-free).

#### Category: Memory & String Management

| Function | Files | Calls | Shim Behavior |
|----------|-------|-------|---------------|
| `ast_free` | cli.c, dsp.c, at_command.c, cpvt.c, select.c, chan_dongle.c, pdiscovery.c, manager.c, at_queue.c | ~50 | **Trivial**: `#define ast_free(p) free(p)` |
| `ast_malloc` | at_command.c, select.c | ~5 | **Trivial**: `#define ast_malloc(s) malloc(s)` |
| `ast_strdup` | at_command.c, chan_dongle.c, pdiscovery.c, pdu.c | ~15 | **Trivial**: `#define ast_strdup(s) strdup(s)` |
| `ast_strdupa` | at_response.c, manager.c | 2 | **Trivial**: stack alloca + strcpy |
| `ast_str_buffer` | cli.c | 3 | **Trivial**: return internal char* from `ast_str` |
| `ast_str_strlen` | chan_dongle.c | 1 | **Trivial**: return stored length |
| `ast_strlen_zero` | manager.c | 2 | **Trivial**: `#define ast_strlen_zero(s) (!(s) || *(s)=='\0')` |
| `ast_alloca` | dsp.c | 2 | **Trivial**: `#define ast_alloca(s) alloca(s)` |

> **Verdict**: All 8 functions → trivial `#define` / inline wrappers around
> standard C. Zero real logic needed.

#### Category: Logging & Debugging

| Function | Files | Calls | Shim Behavior |
|----------|-------|-------|---------------|
| `ast_verb` | dserial.c, cli.c, dsp.c, at_parse.c, at_response.c, share.c, at_command.c | ~150 | **Pass-through**: route to `fprintf(stderr, ...)` or callback |
| `ast_debug` | at_read.c, dsp.c, channel.c, at_response.c | ~50 | **Pass-through**: route to debug callback (level-gated) |
| `ast_log` | dsp.c, at_response.c | ~25 | **Pass-through**: route to `fprintf(stderr, ...)` with level |

> **Verdict**: 3 functions → pass-through to configurable log callback.
> The shim provides a `simbox_set_log_callback()` so consumers can
> redirect logging. Default: stderr.

#### Category: Format & Codec Management

| Function | Files | Calls | Shim Behavior |
|----------|-------|-------|---------------|
| `ast_format_cap_alloc` | chan_dongle.c | 1 | **Stub**: return opaque struct (always SLINEAR) |
| `ast_format_cap_add` | chan_dongle.c, channel.c | 2 | **No-op**: capability is always SLINEAR |
| `ast_format_cap_destroy` | chan_dongle.c | 2 | **Trivial**: free the opaque struct |
| `ast_format_set` | chan_dongle.c | 1 | **No-op**: always SLINEAR |
| `ast_format_copy` | channel.c | 3 | **Trivial**: memcpy of format struct |
| `ast_format_is_slinear` | dsp.c | 3 | **Stub**: always return true |
| `ast_getformatname` | dsp.c | 2 | **Stub**: return `"slin"` |
| `ast_format_cap_iscompatible` | channel.c | 1 | **Stub**: always return true (single format) |

> **Verdict**: 8 functions → mostly no-ops/stubs. chan_svistok only ever uses
> 8kHz Signed Linear (SLINEAR) audio. The shim provides a minimal
> `struct ast_format_cap` that hardcodes this single format.

#### Category: Channel Operations

| Function | Files | Calls | Shim Behavior |
|----------|-------|-------|---------------|
| `ast_channel_nativeformats` | channel.c | 1 | **Accessor**: return format_cap from shim channel |
| `ast_channel_writeformat` | channel.c | 1 | **Accessor**: return SLINEAR format |
| `ast_channel_readformat` | channel.c | 1 | **Accessor**: return SLINEAR format |
| `ast_queue_frame` | dsp.c, channel.c | ~4 | **Real logic**: enqueue frame to consumer callback |
| `ast_waitfor_n_fd` | at_read.c | 1 | **Real logic**: `poll()`/`select()` wrapper |
| `ast_frfree` | dsp.c | ~5 | **Trivial**: free frame + payload |
| `ast_frisolate` | dsp.c | 2 | **Real logic**: copy frame if shared |

> **Verdict**: 7 functions. `ast_queue_frame` and `ast_waitfor_n_fd` need
> real implementations. Channel accessors are trivial.

#### Category: CLI (Command Line Interface)

| Function | Files | Calls | Shim Behavior |
|----------|-------|-------|---------------|
| `ast_cli` | cli.c | ~40 | **Pass-through**: `fprintf()` to fd or callback |
| `ast_cli_complete` | cli.c | ~10 | **Stub/Optional**: tab-completion (not needed for FFI) |

> **Verdict**: 2 functions. CLI output routed to callback. Tab-completion
> can be stubbed initially (FFI consumers won't use Asterisk CLI).

#### Category: Threading & Locking

| Function | Files | Calls | Shim Behavior |
|----------|-------|-------|---------------|
| `ast_mutex_lock` | share.c, select.c, cli.c | ~30 | **Trivial**: `pthread_mutex_lock` |
| `ast_mutex_unlock` | share.c, select.c, cli.c | ~30 | **Trivial**: `pthread_mutex_unlock` |
| `ast_mutex_trylock` | share.c, select.c, chan_dongle.c | ~10 | **Trivial**: `pthread_mutex_trylock` |
| `ast_pthread_create_background` | chan_dongle.c | 1 | **Trivial**: `pthread_create` with detached attr |

> **Verdict**: 4 functions → trivial `pthread_*` wrappers.

#### Category: Timing & Timers

| Function | Files | Calls | Shim Behavior |
|----------|-------|-------|---------------|
| `ast_timer_open` | chan_dongle.c | 1 | **Real logic**: `timerfd_create()` on Linux |
| `ast_timer_close` | chan_dongle.c | 1 | **Trivial**: `close(fd)` |
| `ast_timer_fd` | chan_dongle.c, channel.c | ~2 | **Trivial**: return fd |
| `ast_timer_set_rate` | chan_dongle.c | 1 | **Real logic**: `timerfd_settime()` |
| `ast_timer_ack` | channel.c | 1 | **Trivial**: `read(fd, ...)` to consume event |
| `ast_tvnow` | channel.c, pdiscovery.c | ~5 | **Trivial**: `gettimeofday()` wrapper |
| `ast_tvcmp` | pdiscovery.c | 1 | **Trivial**: compare two timevals |
| `ast_tvdiff_ms` | channel.c | 3 | **Trivial**: `(a-b)` in milliseconds |
| `ast_tvzero` | channel.c | 2 | **Trivial**: check tv_sec==0 && tv_usec==0 |

> **Verdict**: 9 functions. `ast_timer_open`/`ast_timer_set_rate` need
> real `timerfd` implementations (Linux). Rest trivial.

#### Category: DSP & Tone Detection

| Function | Files | Calls | Shim Behavior |
|----------|-------|-------|---------------|
| `ast_tone_detect_init` | dsp.c | ~2 | **Real logic**: initialize Goertzel filter |
| `ast_dtmf_detect_init` | dsp.c | ~1 | **Real logic**: DTMF detector state |
| `ast_mf_detect_init` | dsp.c | ~1 | **Real logic**: MF detector state |
| `ast_fax_detect_init` | dsp.c | ~1 | **Real logic**: fax tone detector |
| `ast_dsp_busydetect` | dsp.c | 1 | **Real logic**: busy tone pattern matching |
| `ast_dsp_silence` | dsp.c | 1 | **Real logic**: silence/energy detection |
| `ast_dsp_silence_with_energy` | dsp.c | 1 | **Real logic**: silence + energy value |

> **Verdict**: 7 functions. **NOTE**: chan_svistok ships its own `dsp.c`
> which is a modified copy of Asterisk's DSP module. This file is
> self-contained — it implements these functions internally, it does not
> call them from an external library. The shim only needs to provide the
> types and headers these functions reference, not re-implement DSP logic.

#### Category: PBX & Dialplan

| Function | Files | Calls | Shim Behavior |
|----------|-------|-------|---------------|
| `ast_pbx_start` | at_response.c | 1 | **Callback**: notify consumer of incoming call |
| `ast_variable_browse` | dsp.c | 1 | **Stub**: return NULL (no config file) |
| `pbx_builtin_setvar_helper` | at_response.c, channel.c | ~10 | **Real logic**: store channel variables (key-value) |

> **Verdict**: 3 functions. `ast_pbx_start` becomes the "incoming call"
> event callback. `pbx_builtin_setvar_helper` stores variables the FFI
> consumer can read (DONGLEIMEI, DONGLEIMSI, DONGLESN, etc.).

#### Category: Module & Registration

| Function | Files | Calls | Shim Behavior |
|----------|-------|-------|---------------|
| `ast_channel_register` | chan_dongle.c | 1 | **No-op**: return success |
| `ast_channel_unregister` | chan_dongle.c | 1 | **No-op** |
| `ast_cli_register_multiple` | chan_dongle.c | 1 | **No-op** or register callbacks |
| `ast_cli_unregister_multiple` | chan_dongle.c | 1 | **No-op** |
| `ast_manager_register2` | chan_dongle.c | ~3 | **No-op**: AMI not needed standalone |
| `ast_manager_unregister` | chan_dongle.c | ~3 | **No-op** |

> **Verdict**: 6 functions → all no-ops. Module lifecycle is managed
> by the shim's own init/shutdown, not Asterisk's module loader.

#### Category: Linked Lists (Macros)

| Macro | Files | Shim Behavior |
|-------|-------|---------------|
| `AST_LIST_ENTRY` | mixbuffer.h, at_queue.h | **Real logic**: define linked list node struct |
| `AST_LIST_HEAD_NOLOCK` | mixbuffer.h, at_queue.h | **Real logic**: define list head struct |
| `AST_LIST_HEAD_INIT_NOLOCK` | various | **Trivial**: zero-initialize head |
| `AST_LIST_TRAVERSE` | various | **Real logic**: for-loop over list |
| `AST_LIST_FIRST` | various | **Trivial**: return head->first |
| `AST_LIST_INSERT_TAIL` | various | **Real logic**: append to tail |
| `AST_RWLIST_RDLOCK` | chan_dongle.c | **Trivial**: `pthread_rwlock_rdlock` |
| `AST_RWLIST_TRAVERSE` | chan_dongle.c | **Real logic**: same as LIST_TRAVERSE |
| `AST_RWLIST_UNLOCK` | chan_dongle.c | **Trivial**: `pthread_rwlock_unlock` |

> **Verdict**: These are all **macros** from `asterisk/linkedlists.h`.
> The shim must provide a compatible header with equivalent macro
> definitions. Asterisk's linked list macros are simple intrusive
> list operations — can be reimplemented in ~80 lines.

### 2.2 Summary: Shim Complexity by Category

| Category | Functions | Trivial/No-op | Pass-through | Real Logic |
|----------|-----------|---------------|-------------|------------|
| Memory/String | 8 | **8** | 0 | 0 |
| Logging | 3 | 0 | **3** | 0 |
| Format/Codec | 8 | **8** | 0 | 0 |
| Channel Ops | 7 | 4 | 0 | **3** |
| CLI | 2 | 0 | **2** | 0 |
| Threading | 4 | **4** | 0 | 0 |
| Timing | 9 | 7 | 0 | **2** |
| DSP | 7 | **7** *(self-contained)* | 0 | 0 |
| PBX/Dialplan | 3 | 1 | 0 | **2** |
| Module/Registration | 6 | **6** | 0 | 0 |
| Linked Lists (macros) | 9 | 3 | 0 | **6** |
| **TOTAL** | **66** | **48 (73%)** | **5 (7%)** | **13 (20%)** |

> **Key finding**: 73% of the Asterisk API surface is trivial (no-ops,
> `#define` to stdlib). Only 13 items need real logic, and 6 of those
> are linked-list macros (straightforward C).

### 2.3 Struct Inventory (~8 types)

| Struct | Files | Shim Design |
|--------|-------|-------------|
| `struct ast_channel` | channel.h, channel.c, chan_dongle.c | **Opaque pointer**. Shim defines internal fields; chan_svistok accesses only via accessors. |
| `struct ast_channel_tech` | channel.h, channel.c, chan_dongle.c | **Full struct with callbacks**. chan_svistok populates `.requester`, `.call`, `.hangup`, `.read`, `.write`, `.indicate`, `.fixup`, `.devicestate`. Shim invokes these callbacks. |
| `struct ast_frame` | dsp.c, channel.c, chan_dongle.c | **Full struct definition needed**. Fields: `frametype`, `subclass`, `datalen`, `samples`, `data.ptr`, `src`, `mallocd`. |
| `struct ast_dsp` | dsp.c | **Self-contained** in chan_svistok's own dsp.c. No shim needed. |
| `struct ast_dsp_busy_pattern` | dsp.c | **Self-contained** in dsp.c. |
| `struct ast_cli_entry` | cli.c | **Struct definition needed** for CLI command registration array. |
| `struct ast_cli_args` | cli.c | **Struct definition needed** for CLI command callback parameter. |
| `struct ast_str` | cli.c | **Simple struct**: `{ char *str; size_t len; size_t used; }` |

### 2.4 Include Headers Required

The shim must provide these headers (in `adapters/include/asterisk/`):

| Header | Content |
|--------|---------|
| `asterisk.h` | Master include, version defines |
| `channel.h` | `ast_channel`, `ast_channel_tech`, channel accessors |
| `frame.h` | `struct ast_frame`, frame types, subclasses |
| `cli.h` | `struct ast_cli_entry`, `struct ast_cli_args`, `AST_CLI_DEFINE` |
| `logger.h` | `ast_log`, `ast_verb`, `ast_debug`, LOG_* levels |
| `linkedlists.h` | `AST_LIST_*`, `AST_RWLIST_*` macros |
| `module.h` | `AST_MODULE_INFO`, `AST_MODFLAG_DEFAULT` |
| `pbx.h` | `ast_pbx_start`, `pbx_builtin_setvar_helper` |
| `utils.h` | `ast_strlen_zero`, `ast_strdupa`, `ast_pthread_create_background` |
| `strings.h` | `ast_str_*` functions |
| `timing.h` | `ast_timer_*` functions |
| `callerid.h` | CallerID constants |
| `causes.h` | Hangup cause codes (`AST_CAUSE_*`) |
| `musiconhold.h` | `ast_moh_start`/`ast_moh_stop` stubs |
| `manager.h` | `ast_manager_register2` stub |
| `stringfields.h` | `AST_DECLARE_STRING_FIELDS` macros |
| `config.h` | `ast_variable_browse` stub |
| `dsp.h` | DSP types (used by chan_svistok's own dsp.c) |
| `format_cap.h` | `ast_format_cap_*` stubs |
| `ast_version.h` | `ast_get_version()` stub |
| `options.h` | Global option flags |
| `ulaw.h` / `alaw.h` | µ-law/A-law tables (used by dsp.c) |

---

## 3. Prior Art: wdoekes `ast_compat.h` Analysis

### 3.1 What wdoekes Solves (and How)

| Aspect | wdoekes approach | Our shim difference |
|--------|-----------------|-------------------|
| **Problem** | Compile against multiple Asterisk versions | Compile against **no** Asterisk |
| **Mechanism** | `#if ASTERISK_VERSION_NUM < 110000` with inline accessors | Full header replacement — no Asterisk SDK needed |
| **Channel accessors** | Backports opaque `ast_channel_name()` etc. to pre-11 Asterisk | Provides all accessors pointing to our own `struct ast_channel` |
| **Format caps** | Shims old format fields to new API | Hardcodes SLINEAR-only caps — no negotiation needed |
| **Build system** | `configure.ac` + `--with-astversion` | CMake or plain Makefile, no Asterisk version detection |
| **Header collision** | `ast_config.h` `#undef`s PACKAGE_* macros | Not applicable — no Asterisk autoconf |

### 3.2 What pulpoff Solves

| Aspect | pulpoff approach | Our shim relevance |
|--------|-----------------|-------------------|
| **Problem** | Port to Asterisk 20 specifically | Shows exact API shapes chan_dongle needs |
| **Format caps** | Uses `ast_format_cap_alloc()` + `ast_format_slin` global | Our shim provides minimal `ast_format_cap` + hardcoded SLINEAR |
| **Module info** | `AST_MODULE_INFO` with `load_pri`/`support_level` | Our shim: `AST_MODULE_INFO` becomes a no-op macro |
| **Docker** | Provides docker-compose deployment | Not relevant to shim, but useful as deployment reference |

### 3.3 Actionable Takeaways

1. **Accessor pattern works**: wdoekes proved that wrapping `ast_channel`
   fields in inline accessors is sufficient. Our shim does the same but
   with our own struct instead of Asterisk's.
2. **Format caps can be minimal**: chan_svistok only uses SLINEAR.
   A 20-line stub is sufficient.
3. **No SQLite dependency**: chan_svistok uses file-based persistence
   (`share.c`), not smsdb. No need to port wdoekes' SQLite code.
4. **No autoconf needed**: Since we control the entire build, we skip
   the header-collision dance that both forks struggle with.

---

## 4. Shim Architecture

### 4.1 Directory Layout

```
adapters/
├── include/
│   └── asterisk/           # Drop-in replacement headers
│       ├── asterisk.h
│       ├── channel.h
│       ├── frame.h
│       ├── cli.h
│       ├── logger.h
│       ├── linkedlists.h
│       ├── module.h
│       ├── pbx.h
│       ├── utils.h
│       ├── strings.h
│       ├── timing.h
│       ├── callerid.h
│       ├── causes.h
│       ├── musiconhold.h
│       ├── manager.h
│       ├── stringfields.h
│       ├── config.h
│       ├── dsp.h
│       ├── format_cap.h
│       ├── ast_version.h
│       ├── options.h
│       ├── ulaw.h
│       └── alaw.h
├── src/
│   ├── shim_channel.c      # ast_channel impl + callbacks
│   ├── shim_frame.c        # ast_frame alloc/free/queue
│   ├── shim_timer.c        # timerfd-based timing
│   ├── shim_logging.c      # configurable log routing
│   ├── shim_linkedlists.c  # (header-only, no .c needed)
│   └── shim_pbx.c          # channel variable storage, call events
└── Makefile

src/
├── simbox_api.h             # PUBLIC C API for FFI consumers
├── simbox_modem.c           # Modem driver adapter (calls, SMS, USSD)
├── simbox_discovery.c       # Discovery adapter
├── simbox_programmator.c    # Firmware flasher adapter
├── simbox_reader.c          # SIM reader adapter
└── simbox_types.h           # Public types (no Asterisk deps)
```

### 4.2 Build Modes

| Mode | Include Path | Links Against | Result |
|------|-------------|--------------|--------|
| **Asterisk module** (existing) | `/usr/include/asterisk/` | Real Asterisk | `chan_dongle.so` — unchanged |
| **Standalone shim** (new) | `adapters/include/` | Shim `.a` / `.so` | `libsimbox.so` — Asterisk-free |

Same unmodified `asterisk_chan_svistok/` source, different `-I` path.

### 4.3 Compilation Strategy

```
# Standalone build (new):
gcc -I adapters/include/ \
    -I asterisk_chan_svistok/chan_svistok/ \
    asterisk_chan_svistok/chan_svistok/*.c \
    adapters/src/*.c \
    src/*.c \
    -o libsimbox.so -shared -lpthread
```

The key insight: by providing `adapters/include/asterisk/` with the same
header names as real Asterisk, chan_svistok's `#include <asterisk/channel.h>`
resolves to our shim without any source modification.

---

## 5. Public C API (`simbox_api.h`)

This is the FFI seam `flutter_gsm` binds to. No Asterisk types exposed.

### 5.1 Modem Driver API

```c
// Lifecycle
simbox_handle_t simbox_init(const simbox_config_t *config);
void            simbox_shutdown(simbox_handle_t handle);

// Device enumeration
int             simbox_device_count(simbox_handle_t handle);
simbox_device_t simbox_device_get(simbox_handle_t handle, int index);
const char*     simbox_device_sn(simbox_device_t dev);    // S/N identifier
const char*     simbox_device_imei(simbox_device_t dev);
const char*     simbox_device_imsi(simbox_device_t dev);
int             simbox_device_state(simbox_device_t dev);  // enum

// Voice calls
int             simbox_call_originate(simbox_device_t dev, const char *number);
int             simbox_call_hangup(simbox_device_t dev);
int             simbox_call_answer(simbox_device_t dev);

// SMS
int             simbox_sms_send(simbox_device_t dev, const char *number,
                                const char *message);

// USSD
int             simbox_ussd_send(simbox_device_t dev, const char *code);

// AT commands (raw)
int             simbox_at_command(simbox_device_t dev, const char *cmd,
                                  char *response, size_t response_size);

// Events (callback-based)
typedef void (*simbox_event_cb)(simbox_event_t *event, void *userdata);
void            simbox_set_event_callback(simbox_handle_t handle,
                                           simbox_event_cb cb, void *userdata);
```

### 5.2 Discovery API

```c
simbox_discovery_t simbox_discovery_start(const char *config_path);
void               simbox_discovery_stop(simbox_discovery_t handle);
int                simbox_discovery_scan(simbox_discovery_t handle);
int                simbox_discovery_device_count(simbox_discovery_t handle);
simbox_discovered_device_t simbox_discovery_device_get(
                               simbox_discovery_t handle, int index);
```

### 5.3 Programmator API

```c
simbox_prog_t   simbox_prog_open(const char *tty_port);
void            simbox_prog_close(simbox_prog_t handle);
int             simbox_prog_set_diagmode(simbox_prog_t handle);
int             simbox_prog_flash(simbox_prog_t handle,
                                   const char *usb_device,
                                   const char *firmware_path,
                                   simbox_prog_progress_cb cb,
                                   void *userdata);
int             simbox_prog_get_progress(simbox_prog_t handle);
const char*     simbox_prog_get_state(simbox_prog_t handle);
```

### 5.4 Reader API

```c
simbox_reader_t simbox_reader_open(const char *tty_port);
void            simbox_reader_close(simbox_reader_t handle);
int             simbox_reader_send_apdu(simbox_reader_t handle,
                                         const uint8_t *apdu, size_t len,
                                         uint8_t *response, size_t *resp_len);
int             simbox_reader_get_atr(simbox_reader_t handle,
                                       char *atr_hex, size_t atr_size);
int             simbox_reader_reset(simbox_reader_t handle);
```

---

## 6. Event Types

```c
typedef enum {
    SIMBOX_EVENT_DEVICE_CONNECTED,
    SIMBOX_EVENT_DEVICE_DISCONNECTED,
    SIMBOX_EVENT_INCOMING_CALL,
    SIMBOX_EVENT_CALL_STATE_CHANGED,
    SIMBOX_EVENT_INCOMING_SMS,
    SIMBOX_EVENT_USSD_RESPONSE,
    SIMBOX_EVENT_BALANCE_UPDATE,
    SIMBOX_EVENT_DEVICE_ERROR,
    SIMBOX_EVENT_PROG_PROGRESS,
} simbox_event_type_t;

typedef struct {
    simbox_event_type_t type;
    const char         *device_sn;
    union {
        struct { const char *caller; }                  incoming_call;
        struct { int old_state; int new_state; }        call_state;
        struct { const char *sender; const char *text; } incoming_sms;
        struct { const char *response; }                 ussd;
        struct { const char *balance; }                  balance;
        struct { int code; const char *message; }        error;
        struct { int percent; const char *state; }       prog_progress;
    } data;
} simbox_event_t;
```

---

## 7. Edge Cases & Design Decisions

### 7.1 DSP Module

chan_svistok includes its own `dsp.c` — a modified copy of Asterisk's DSP
code. This file **defines** the DSP functions (tone detection, silence
detection, DTMF), it doesn't call them from an external Asterisk library.

**Decision**: The shim headers (`asterisk/dsp.h`) provide type definitions
only. The DSP implementations come from chan_svistok's own `dsp.c`, which
compiles as part of the standalone build.

`dsp.c` internally uses `ulaw.h`/`alaw.h` tables — the shim must provide
these lookup tables (they're public domain, ~256 entries each).

### 7.2 CallerID

chan_svistok uses `ast_callerid_parse` and `AST_PRES_*` constants from
`asterisk/callerid.h`. The shim must provide these constants and a minimal
`ast_callerid_parse` implementation (string splitting, ~20 lines).

### 7.3 Music on Hold

chan_svistok calls `ast_moh_start`/`ast_moh_stop` in channel.c for
call hold scenarios. In standalone mode, these are **no-ops** (no audio
mixing available without Asterisk's MOH module).

### 7.4 Manager API (AMI)

chan_svistok registers several AMI actions via `ast_manager_register2`.
In standalone mode, all manager functions are **no-ops** — the simbox
C API replaces AMI for external control.

### 7.5 String Fields (`AST_DECLARE_STRING_FIELDS`)

chan_svistok uses Asterisk's string-field memory pool macros for
`pvt_config`. The shim must provide these macros — they're complex
but self-contained (pool allocator with realloc). Alternative: convert
to plain `char[]` fields in a future iteration, but that would modify
the read-only tree. **Decision**: implement the macros (~100 lines).

---

## 8. Platform-Specific Notes

### 8.1 Linux (primary target)

- `timerfd_create` / `timerfd_settime` for `ast_timer_*`
- `poll()` for `ast_waitfor_n_fd`
- `pthread_*` for all threading
- Full functionality

### 8.2 OpenWRT (cross-compile target)

- Same as Linux (musl libc compatible)
- No special handling needed per requirements decision
- Build with appropriate cross-compiler toolchain

### 8.3 macOS / Windows (stub targets)

- Timer: `kqueue` (macOS) or stub
- Serial: stub (no ttyUSB)
- Per requirements: stub-first, no real driver

---

## 9. Amendment 2026-08-22 — Real Adapter Wiring

Specifies how `src/simbox_*.c` must drive chan_svistok's real logic
through the shim, per requirements' new Must-Haves #8-#10. Grounded in
direct reads of `channel.c`/`chan_dongle.c`/`at_command.c` (function
names/signatures below are real, not inferred) — anything not yet
traced is marked explicitly rather than guessed.

**Platform split (correction 2026-08-22, later same day)**: everything
in §9.1-9.5 below — the whole point of calling into chan_svistok's real
`load_module`/`channel_tech`/`at_enque_*`/`gpublic` machinery — is
**Linux-only**, because chan_svistok itself is Linux-only by design
(confirmed empirically during Task 5.2: its background discovery thread
calls real `/sys/bus/usb/devices` scanning code that has no
non-Linux equivalent and isn't meant to). Every `src/simbox_*.c`
function this section describes must be written as:

```c
#ifdef __linux__
    /* real chan_svistok-driving implementation, per §9.1-9.5 below */
#else
    /* macOS/Windows/other: chan_svistok's internals never run here.
     * Stub-first, per the existing Should-Have default — same
     * "not implemented on this platform" shape ModemRepositoryImpl
     * already wraps into a typed exception on the Dart side, not a
     * fabricated simulation. */
#endif
```

This isn't new scope — it directly implements this document's own
pre-existing secondary user story ("OS-specific code added via
preprocessor macros... inside the new adapters/src/ layer... Linux
behavior needs minimal changes... other platforms get their own code
paths without touching proven logic"), just applied consistently from
here on, including to Task 5.2's already-built module-capture code
(the capture mechanism itself — `adapters/`'s macro change — is
harmless to compile on any platform; it's *calling*
`simbox_module_bridge_load()` from `src/` that must be
`#ifdef __linux__`-gated, starting with Task 5.3).

**Corrected 2026-08-22 (same day, per Anton's direct guidance)**: the
first draft below proposed adding `EXPORT_DEF` to five functions inside
`asterisk_chan_svistok/` as a "visibility-only" exception to the
read-only rule. Rejected — the rule has no exceptions. Every design
below now reaches chan_svistok exclusively through symbols it **already**
exports, or through adapter-side capture of the `AST_MODULE_INFO` macro
(which lives in `adapters/`, not the read-only tree). **Zero
modification of `asterisk_chan_svistok/` anywhere in this section.**

**Governing rule for `src/`**: every function in `src/simbox_*.c` must
be a thin marshaller — unpack `simbox_api.h`'s primitive parameters,
call straight into an already-exported chan_svistok symbol (or an
adapter-side wrapper that itself does the same), pack the result back.
No independent state, no parallel logic. If a `src/` function needs more
than a few lines, that logic belongs in `adapters/` instead.

### 9.1 Device population: the real, already-exported `gpublic` list

`simbox_device_t` currently wraps a hand-rolled
`struct simbox_device_internal` (see `simbox_modem.c`) with no
connection to chan_svistok. It must instead wrap a real `struct pvt *`
from chan_dongle's own live device list — which, it turns out,
chan_svistok **already exports** in full:

```c
// chan_dongle.h — already EXPORT_DECL, no changes needed:
typedef struct public_state {
    AST_RWLIST_HEAD(devices, pvt) devices;   // real, walkable device list
    ...
} public_state_t;
EXPORT_DECL public_state_t * gpublic;         // the live global instance

EXPORT_DECL struct pvt * find_device_ex(struct public_state * state, const char * name);
EXPORT_DECL struct pvt * find_device_ext(const char * name, const char ** reason);
```

`gpublic->devices` only gets populated once `load_module()` runs
(`load_module` → `public_state_init(gpublic)` → per-device
`pvt_create()`, all internal to `chan_dongle.c`). `load_module` itself
is `static` — chan_svistok has no exported way to call it directly. But
it doesn't need one: `chan_dongle.c`'s own unmodified

```c
AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, MODULE_DESCRIPTION,
    .load = load_module, .unload = unload_module, .reload = reload_module);
```

expands against the **shim's** `AST_MODULE_INFO` macro
(`adapters/include/asterisk/module.h`, part of the adapter, freely
editable). The current shim stores the resulting `ast_module_info`
in a function-local `static` — invisible outside `chan_dongle.c`'s own
translation unit. Changing *only the macro's expansion* (not
chan_svistok's invocation of it, which stays byte-identical) to also
register that `ast_module_info` pointer in an adapter-owned registry
(e.g. `simbox_module_registry_register(&__mod_info)`, a new function in
a new `adapters/src/shim_module_registry.c`) gives `src/simbox_api.c` a
legitimate way to retrieve `.load`/`.unload` and call them —
`simbox_init()` calls the captured `load()` once, which runs
chan_dongle's real, unmodified config-file-driven device population,
after which `gpublic->devices` is real.

**This reframes discovery** (per requirements' Open Question, resolved
this way): a "discovered" device becomes a **configured** device
(data_tty/audio_tty/imei from a config file, chan_dongle-style), not
something found by scanning `/sys/bus/usb/devices`.
`simbox_discovery.c`'s USB-scan stub can stay a stub — `simbox_api.h`'s
public `simbox_device_count`/`get_by_index`/`get_by_sn` should walk
`gpublic->devices` directly (via the already-shimmed `AST_RWLIST_TRAVERSE`
macro) instead. `simbox_config_t.config_dir` (already a public field,
currently unused) is the natural place to point at the dongle-style
config file consumed by `dc_config.c` during `load_module()`.

### 9.2 Calls: the already-exported `channel_tech` struct

```c
// channel.c — already EXPORT_DEF, no changes needed:
EXPORT_DEF struct ast_channel_tech channel_tech = {
    .requester = channel_request,   // static, but reachable via this pointer
    .call      = channel_call,      // static, but reachable via this pointer
    .hangup    = channel_hangup,    // static, but reachable via this pointer
    .answer    = channel_answer,    // static, but reachable via this pointer
    ...
};
```

`channel_request`/`channel_call`/`channel_hangup`/`channel_answer` are
individually `static`, but the struct holding pointers to all four is
already `EXPORT_DEF` — external code reaches them through the struct,
never needing the individual symbols to be non-static. No
read-only-tree change of any kind needed here.

- `simbox_call_originate(dev, number)` → build a dial string
  (`"<donglename>/<number>"`, matching `channel_request`'s
  `parse_dial_string`+`find_device_by_resource` parsing — confirmed by
  reading `channel_request`'s body) → `channel_tech.requester(type, cap,
  NULL, dial_string, &cause)` to allocate an `ast_channel` bound to this
  `pvt` → `channel_tech.call(channel, dest, 0)` to actually dial. The
  returned `ast_channel*` must be kept (cached on the
  `simbox_device_t`/`pvt` wrapper) for the matching hangup/answer calls.
- `simbox_call_hangup(dev)` → `channel_tech.hangup(channel)` on the
  cached channel.
- `simbox_call_answer(dev)` → `channel_tech.answer(channel)` on the
  cached channel (this path is for *incoming* calls the shim's PBX hook
  — §9.5 — already allocated a channel for).

### 9.3 SMS / USSD / raw AT: `at_enque_*`

```c
// at_command.c, real signatures:
EXPORT_DEF int at_enque_sms(struct cpvt* cpvt, const char* destination, const char* msg, unsigned validity_minutes, int report_req, void ** id);
EXPORT_DEF int at_enque_ussd(struct cpvt * cpvt, const char * code, const char *u1, unsigned u2, int u3, void ** id);
EXPORT_DEF int at_enque_cmd_proc(struct cpvt* cpvt, const char * cmd);
```

Already `EXPORT_DEF` — no visibility change needed, these are already
part of chan_svistok's intended public seam.

- `simbox_sms_send(dev, number, message)` → `at_enque_sms(cpvt, number,
  message, /*validity*/ 0, /*report_req*/ 0, &id)`. Needs a `struct
  cpvt*` (channel-pvt pairing), not just the bare `pvt` — `cpvt.c`
  (already compiling) is chan_svistok's own management of this; exact
  cpvt-acquisition call to use for a non-call SMS/USSD context (i.e., no
  active `ast_channel`) needs verification at implementation time — flag
  as an implementation-time investigation, not guessed here.
- `simbox_ussd_send(dev, code)` → `at_enque_ussd(cpvt, code, NULL, 0, 0,
  &id)`. Result (`SIMBOX_EVENT_USSD_RESPONSE`) arrives asynchronously via
  the AT-response parser (`at_response.c`) — must be wired to fire
  through §9.5's event path, not returned synchronously (this was
  already correctly specified as event-based in `sdd-flutter_gsm-ffi`'s
  own specs, just never had a real firing source before now).
- `simbox_at_command(dev, cmd, response_buf, buf_len)` → `at_enque_cmd_proc(cpvt,
  cmd)`. Response capture: `at_enque_cmd_proc` enqueues async, response
  arrives via `at_response.c`'s parser — the adapter needs a
  synchronous-looking wrapper (block on a condition variable until the
  matching response arrives, or timeout) since `simbox_at_command`'s
  existing signature is synchronous (`response_buf` filled by return
  time) — implementation-time detail, not a signature change.

### 9.4 IMEI change

Not yet traced to a specific chan_svistok entry point — `at_command.c`
has no obvious `at_enque_imei_change`. Investigate at implementation
time (likely a raw AT command sequence, e.g. Qualcomm DIAG-mode via the
existing `programmator/` subsystem — `simbox_prog_change_imei` already
exists in the public API and may be the *actual* right home for this,
with `simbox_change_imei` becoming a thin wrapper around
`simbox_prog_*` rather than an AT-command path). Flag as an open
implementation question, not resolved here.

### 9.5 Events: wire through `shim_pbx.c`, not ad-hoc `simbox_api.c` calls

Per this document's original §2.1 PBX category note: `ast_pbx_start` was
always meant to become "the incoming call event callback" — that's
chan_svistok's own real hook for "a call arrived, hand it to the PBX
layer." `shim_pbx.c` (Phase 2, already implemented per
`04-implementation.md`) should be the *only* place that invokes
`simbox_api.c`'s registered `simbox_event_cb` for call/registration
events — not new ad-hoc `cb(...)` call sites scattered through
`simbox_modem.c`. SMS/USSD-response events likely need a second real
hook into `at_response.c`'s completion path (where an enqueued
`at_enque_sms`/`at_enque_ussd`'s response actually arrives) — verify
whether `shim_pbx.c` already covers this or whether `at_response.c`
needs its own event-dispatch hook, at implementation time.

**Ownership/lifetime rule carries over unchanged**: every event fired
through any of these real hooks must be heap-allocated before invoking
the callback (see `simbox_types.h`'s `simbox_event_cb` doc comment,
added by `sdd-flutter_gsm-ffi` after finding a stack-lifetime bug in the
one event-firing site that existed before this amendment) — applies to
every new firing site this amendment adds, not just the one already
fixed.

### 9.6 What stays exactly as-is

- `simbox_prog_*` (DIAG programmator) and `simbox_reader_*` (APDU SIM
  reader) — not touched by this amendment; requirements' Must-Have #6
  already scoped them as separate surfaces, and nothing in the "adapter
  doesn't drive real logic" finding was about these two (not
  independently re-verified here — worth a follow-up grep before
  assuming they're fine, but out of this amendment's immediate scope).
- The public `simbox_api.h` C signatures — per requirements' Amendment,
  this rework should be purely internal.
- The Asterisk-compatibility shim itself (`adapters/`) — confirmed
  working, no changes needed.
- **`asterisk_chan_svistok/` and `asterisk_chan_dongle/` — literally
  every byte, forever.** Restated once more here because it's the
  organizing constraint of this entire section: nothing in §9.1-9.5
  touches either tree. All new code lives in `adapters/` (macro/registry
  additions) or `src/` (thin marshalling calls).

### 9.7 `EXPORT_DEF`/`EXPORT_DECL` symbol audit (requirements' Must-Have #9, Task 5.1, done 2026-08-22)

327 total `EXPORT_DEF`/`EXPORT_DECL` hits across chan_svistok's `.c`/`.h`
files — most are CLI commands, AMI manager actions, and internal
helpers irrelevant to an FFI driver with no dialplan/CLI/AMI consumer.
Full grep dump not reproduced here (not useful as prose); the table
below is the curated subset actually relevant to Phase 5's remaining
tasks, pulled from `chan_dongle.h`/`.c`, `channel.h`/`.c`,
`at_command.h`, `cpvt.h`.

| Symbol | File | Signature (abridged) | Relevance |
|---|---|---|---|
| `gpublic` | `chan_dongle.h` | `public_state_t *` | The live global device list (§9.1) |
| `find_device_ex`/`find_device_ext` | `chan_dongle.h` | `(state, name)` / `(name, &reason)` | Device lookup by name |
| `pvt_str_state`/`pvt_str_state_ex` | `chan_dongle.h` | `(const pvt*) -> const char*` | Human-readable device state, for diagnostics |
| `pvt_try_restate` | `chan_dongle.h` | `(pvt*)` | Device state-machine driver |
| `is_dial_possible`/`ready4voice_call` | `chan_dongle.h` | `(const pvt*, ...) -> int` | Pre-flight checks before `simbox_call_originate` |
| `channel_tech` | `channel.h` | `struct ast_channel_tech` | `.requester`/`.call`/`.hangup`/`.answer` — full `ast_channel`-based call control (§9.2's original design) |
| `new_channel` | `channel.h` | `(pvt, ast_state, cid_num, call_idx, dir, state, exten, requestor) -> ast_channel*` | Needed if using the `channel_tech` path |
| `queue_hangup` | `channel.h` | `(ast_channel*, cause) -> int` | Alternative hangup path via channel, not `cpvt` |
| `change_channel_state` | `channel.h` | `(cpvt*, newstate, cause)` | Direct `cpvt`-level state transition — lower-level than `channel_tech` |
| **`cpvt_alloc`** | `cpvt.h` | `(pvt*, call_idx, dir, call_state_t) -> cpvt*` | **New finding**: acquires a `cpvt` directly, no `ast_channel` needed — resolves §9.3's flagged-open "cpvt outside a call" question |
| `cpvt_free`/`pvt_find_cpvt` | `cpvt.h` | | Release / look up a `cpvt` |
| **`at_enque_dial`** | `at_command.h` | `(cpvt*, number, clir) -> int` | **New finding**: direct `cpvt`-level dial, no `channel_request`/`channel_tech.call` dance needed |
| **`at_enque_answer`** | `at_command.h` | `(cpvt*) -> int` | Direct `cpvt`-level answer |
| **`at_enque_hangup`** | `at_command.h` | `(cpvt*, call_idx) -> int` | Direct `cpvt`-level hangup |
| `at_enque_sms`/`at_enque_ussd`/`at_enque_cmd_proc` | `at_command.h` | (unchanged from earlier draft) | SMS/USSD/raw AT |
| `at_response` | `at_response.h` | `(pvt*, iovec*, iovcnt, at_res_t) -> int` | Where AT responses actually land — the real hook for event/response wiring (§9.5) |
| `dc_config_fill`/`dc_sconfig_fill`/`dc_gconfig_fill` | `dc_config.h` | take `struct ast_config *` | Config-file parsing — needs the shim's `ast_config` (already built: `shim_config.c`, per Phase 2) |
| `self_module` | `chan_dongle.h` | `() -> struct ast_module *` | Possibly relevant to the `AST_MODULE_INFO` capture design (§9.1) — investigate at Task 5.2 |

**Design refinement from this audit**: `cpvt_alloc` +
`at_enque_dial`/`at_enque_answer`/`at_enque_hangup` are a **simpler,
lower-level alternative** to §9.2's original `channel_tech`-based call
design — operating directly on a `cpvt` without needing to fabricate a
full `ast_channel` via `channel_request` (which does dial-string
parsing, format-capability checks, and other Asterisk-dialplan-oriented
work the FFI driver doesn't need). **Preferred for Task 5.5/5.6**: use
`cpvt_alloc`+`at_enque_*` directly; keep `channel_tech` as a documented
fallback if the lower-level path turns out to skip state-tracking the
adapter needs (e.g. if audio routing or hangup-cause propagation
genuinely requires the full channel machinery — verify empirically at
implementation time, don't assume either way in advance).

### 9.8 Test sequencing (requirements' Must-Have #10)

- **First** (before any `adapters/`/`src/` code for this amendment):
  build and run chan_svistok's own existing test files
  (`chan_svistok/test/test1.c`, `chan_svistok/simnode/adiscovery_test.c`,
  `chan_svistok/programmator/ttyprog_test.c`,
  `chan_svistok/reader/old/test.c`) against chan_svistok directly, to
  establish a verified, known-good behavioral baseline. These files are
  read-only like the rest of chan_svistok — build a harness *around*
  them (a new `tests/`-side Makefile target), don't modify them to run.
- **Last** (once all of §9.1-9.5's wiring is implemented and the
  regular `tests/test_simbox.c` suite passes): copy those exact same
  four files **verbatim** into `libsCpp/asterisk_chan_simbox/tests/`
  and build/run them against the new adapter/shim path. Passing without
  modification is the concrete, mechanical proof that the adapter
  forwards to real behavior rather than reimplementing it — the
  strongest test this flow can offer against its own original failure
  mode recurring.

---

## Approval

- [ ] Reviewed by: Anton Dodonov
- [ ] Approved on: (original §1-8 content — never formally checked off
      despite `_status.md` claiming "Specifications approved
      2026-08-21"; carrying that inconsistency forward rather than
      silently fixing it)
- [x] Reviewed by: Anton Dodonov — corrected §9's first draft directly
      (rejected read-only-tree modification, redirected to
      already-exported-symbol + `AST_MODULE_INFO`-capture design; added
      §9.7/9.8)
- [x] Approved on: 2026-08-22 (§9 amendment, corrected version)
- [ ] Notes:
