/*
 * Internal, Linux-only seam between simbox_api.c and simbox_modem.c -
 * NOT part of the public simbox_api.h surface (which must stay free of
 * chan_svistok/Asterisk types, per specifications' §9.6 "what stays
 * exactly as-is"). Only included under #ifdef __linux__.
 */
#ifndef SIMBOX_INTERNAL_LINUX_H
#define SIMBOX_INTERNAL_LINUX_H

#ifdef __linux__

#include "simbox_types.h"
#include <chan_dongle.h>

/* Wraps a real struct pvt* (from gpublic->devices) as a simbox_device_t
 * - see simbox_modem.c. */
simbox_device_t simbox_device_wrap_pvt(struct pvt *pvt);
void simbox_device_destroy(simbox_device_t dev);
void simbox_device_apply_call_state(simbox_device_t dev,
                                     simbox_device_state_t state);
void simbox_device_attach_call(simbox_device_t dev, struct cpvt *cpvt,
                               int adapter_owned);
void simbox_device_capture_at_response(simbox_device_t dev,
                                        const char *response_line);

/* Fires a real SIMBOX_EVENT_INCOMING_CALL through whichever
 * simbox_handle_t's event callback is currently registered - called
 * from adapters/src/shim_pbx.c's ast_pbx_start(), the real chan_svistok
 * hook for "an incoming call is ready to be handed to the PBX/dialplan
 * layer" (see specifications §9.5). Implemented in simbox_api.c, which
 * owns the private simbox_instance/event_cb state this needs. No-op if
 * no callback is registered. device_sn/caller may be NULL/empty; not
 * heap-allocating them here - simbox_api.c copies what it needs into
 * the heap-allocated simbox_event_t itself (see simbox_event_cb's
 * ownership-transfer contract in simbox_types.h). */
void simbox_event_bridge_fire_incoming_call(const char *device_sn,
                                             const char *caller,
                                             struct cpvt *cpvt);

/* Called by the adapter-owned AMI shim after chan_svistok's real
 * manager_event_* helpers format their payloads. */
void simbox_event_bridge_fire_call_state(const char *device_sn, const char *new_state);
void simbox_event_bridge_fire_incoming_sms(const char *device_sn,
                                            const char *sender,
                                            const char *text);
void simbox_event_bridge_fire_ussd_response(const char *device_sn,
                                             const char *response);

/* Called by the logging shim for the exact CMD_USER response log emitted
 * by at_response.c. The modem adapter owns correlation/wakeup state. */
void simbox_at_response_bridge_capture(const char *device_sn,
                                        const char *response_line);

#endif /* __linux__ */

#endif /* SIMBOX_INTERNAL_LINUX_H */
