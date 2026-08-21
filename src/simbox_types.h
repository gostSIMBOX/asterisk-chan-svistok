/*
 * Simbox Native SDK - Public Type Definitions
 * Clean C types for FFI consumers (Dart/Flutter, Python, Rust, Go)
 * NO Asterisk headers or dependencies exposed here.
 */
#ifndef SIMBOX_TYPES_H
#define SIMBOX_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *simbox_handle_t;
typedef void *simbox_device_t;
typedef void *simbox_discovery_t;
typedef void *simbox_prog_t;
typedef void *simbox_reader_t;

typedef enum {
    SIMBOX_STATE_DISCONNECTED = 0,
    SIMBOX_STATE_CONNECTING   = 1,
    SIMBOX_STATE_IDLE         = 2,
    SIMBOX_STATE_RINGING      = 3,
    SIMBOX_STATE_DIALING      = 4,
    SIMBOX_STATE_ACTIVE_CALL  = 5,
    SIMBOX_STATE_HOLD         = 6,
    SIMBOX_STATE_ERROR        = 7,
    SIMBOX_STATE_DIAGNOSTIC   = 8,
} simbox_device_state_t;

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
    char sn[64];
    char imei[32];
    char imsi[32];
    char name[64];
    char model[64];
    char firmware[64];
    char tty_data[128];
    char tty_audio[128];
    int  rssi;
    simbox_device_state_t state;
} simbox_device_info_t;

typedef struct {
    char dev_name[64];
    char serial_number[64];
    char imei[32];
    char data_port[128];
    char audio_port[128];
    char net_port[128];
    int  hub_port;
    int  vendor_id;
    int  product_id;
} simbox_discovered_device_t;

typedef struct {
    simbox_event_type_t type;
    const char         *device_sn;
    union {
        struct { const char *caller; }                  incoming_call;
        struct { int old_state; int new_state; }        call_state;
        struct { const char *sender; const char *text; } incoming_sms;
        struct { const char *response; }                 ussd;
        struct { const char *balance; }                  balance;
        struct { int error_code; const char *message; }  error;
        struct { int percent; const char *stage; }       prog_progress;
    } data;
} simbox_event_t;

typedef struct {
    const char *config_dir;
    const char *state_dir;
    int         log_level;
    bool        auto_discovery;
    bool        auto_recover_diag;
} simbox_config_t;

typedef void (*simbox_event_cb)(const simbox_event_t *event, void *userdata);
typedef void (*simbox_prog_progress_cb)(int percent, const char *stage, void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* SIMBOX_TYPES_H */
