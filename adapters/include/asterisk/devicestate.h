/*
 * Asterisk compatibility shim for chan_simbox
 * devicestate.h - Device state constants and functions
 */
#ifndef ASTERISK_DEVICESTATE_H
#define ASTERISK_DEVICESTATE_H

enum ast_device_state {
    AST_DEVICE_UNKNOWN = 0,
    AST_DEVICE_NOT_INUSE = 1,
    AST_DEVICE_INUSE = 2,
    AST_DEVICE_BUSY = 3,
    AST_DEVICE_INVALID = 4,
    AST_DEVICE_UNAVAILABLE = 5,
    AST_DEVICE_RINGING = 6,
    AST_DEVICE_RINGINUSE = 7,
    AST_DEVICE_ONHOLD = 8,
    AST_DEVICE_TOTAL = 9,
};

#define AST_DEVICE_UNKNOWN     0
#define AST_DEVICE_NOT_INUSE   1
#define AST_DEVICE_INUSE       2
#define AST_DEVICE_BUSY        3
#define AST_DEVICE_INVALID     4
#define AST_DEVICE_UNAVAILABLE 5
#define AST_DEVICE_RINGING     6
#define AST_DEVICE_RINGINUSE   7
#define AST_DEVICE_ONHOLD      8
#define AST_DEVICE_TOTAL       9

#endif /* ASTERISK_DEVICESTATE_H */
