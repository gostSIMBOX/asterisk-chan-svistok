/*
 * Simbox Native SDK - Master Public API
 * Unified C Interface for Modem Driver, Discovery, Qualcomm DIAG Programmator, and APDU SIM Reader.
 */
#ifndef SIMBOX_API_H
#define SIMBOX_API_H

#include "simbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* 1. Global Lifecycle & Configuration                                       */
/* ========================================================================= */

simbox_handle_t simbox_init(const simbox_config_t *config);
void            simbox_shutdown(simbox_handle_t handle);
const char     *simbox_version(void);

void            simbox_set_event_callback(simbox_handle_t handle,
                                          simbox_event_cb cb, void *userdata);

/* ========================================================================= */
/* 2. Modem Device Management (Indexed by Serial Number)                    */
/* ========================================================================= */

int             simbox_device_count(simbox_handle_t handle);
simbox_device_t simbox_device_get_by_index(simbox_handle_t handle, int index);
simbox_device_t simbox_device_get_by_sn(simbox_handle_t handle, const char *sn);

int             simbox_device_get_info(simbox_device_t dev, simbox_device_info_t *info);
const char     *simbox_device_sn(simbox_device_t dev);
const char     *simbox_device_imei(simbox_device_t dev);
const char     *simbox_device_imsi(simbox_device_t dev);
simbox_device_state_t simbox_device_state(simbox_device_t dev);
int             simbox_device_rssi(simbox_device_t dev);

/* ========================================================================= */
/* 3. Voice Calls (SLINEAR 8kHz PCM)                                         */
/* ========================================================================= */

int             simbox_call_originate(simbox_device_t dev, const char *number);
int             simbox_call_hangup(simbox_device_t dev);
int             simbox_call_answer(simbox_device_t dev);
int             simbox_call_send_dtmf(simbox_device_t dev, char digit);

int             simbox_call_write_audio(simbox_device_t dev, const int16_t *pcm_samples, size_t count);
int             simbox_call_read_audio(simbox_device_t dev, int16_t *pcm_samples, size_t max_count);

/* ========================================================================= */
/* 4. SMS & USSD Operations                                                  */
/* ========================================================================= */

int             simbox_sms_send(simbox_device_t dev, const char *number, const char *message);
int             simbox_ussd_send(simbox_device_t dev, const char *code);

/* ========================================================================= */
/* 5. Raw AT Commands & Diagnostics                                          */
/* ========================================================================= */

int             simbox_at_command(simbox_device_t dev, const char *cmd,
                                  char *response_buf, size_t buf_len);
int             simbox_change_imei(simbox_device_t dev, const char *new_imei);

/* ========================================================================= */
/* 6. Multi-Generation Node Discovery                                        */
/* ========================================================================= */

simbox_discovery_t simbox_discovery_start(const char *config_path);
void               simbox_discovery_stop(simbox_discovery_t handle);
int                simbox_discovery_scan(simbox_discovery_t handle);
int                simbox_discovery_device_count(simbox_discovery_t handle);
int                simbox_discovery_device_get(simbox_discovery_t handle, int index,
                                               simbox_discovered_device_t *device);

/* ========================================================================= */
/* 7. Qualcomm DIAG Programmator & Firmware Flasher                          */
/* ========================================================================= */

simbox_prog_t   simbox_prog_open(const char *tty_port);
void            simbox_prog_close(simbox_prog_t handle);
int             simbox_prog_set_diagmode(simbox_prog_t handle);
int             simbox_prog_change_imei(simbox_prog_t handle, const char *new_imei);
int             simbox_prog_flash(simbox_prog_t handle,
                                   const char *usb_device,
                                   const char *firmware_path,
                                   simbox_prog_progress_cb cb,
                                   void *userdata);
int             simbox_prog_get_progress(simbox_prog_t handle);
const char     *simbox_prog_get_state(simbox_prog_t handle);

/* ========================================================================= */
/* 8. APDU SIM Reader Interface                                              */
/* ========================================================================= */

simbox_reader_t simbox_reader_open(const char *tty_port);
void            simbox_reader_close(simbox_reader_t handle);
int             simbox_reader_send_apdu(simbox_reader_t handle,
                                         const uint8_t *apdu, size_t len,
                                         uint8_t *response, size_t *resp_len);
int             simbox_reader_get_atr(simbox_reader_t handle,
                                       char *atr_hex, size_t atr_size);
int             simbox_reader_reset(simbox_reader_t handle);

#ifdef __cplusplus
}
#endif

#endif /* SIMBOX_API_H */
