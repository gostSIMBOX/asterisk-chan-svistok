/* Dongle proxy-only composition fragment. */

extern int svistok_dongle_impl_at_response_cgmi(struct pvt *, const char *);
extern int svistok_dongle_impl_at_response_cgmr(struct pvt *, const char *);
extern int svistok_dongle_impl_at_response_cgsn(struct pvt *, const char *);
extern int svistok_dongle_impl_at_response_cimi(struct pvt *, const char *);
extern int svistok_dongle_impl_at_response_cops(struct pvt *, char *);
extern int svistok_dongle_impl_at_response_csq(struct pvt *, const char *);
extern int svistok_dongle_impl_at_response_mode(struct pvt *, char *, size_t);
extern int svistok_dongle_impl_at_response_rssi(struct pvt *, const char *);
static int at_response_cgmi(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_cgmi(pvt,str); svistok_hook_persist_manufacturer(pvt); return rv; }
static int at_response_cgmr(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_cgmr(pvt,str); svistok_hook_persist_firmware(pvt); return rv; }
static int at_response_cgsn(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_cgsn(pvt,str); svistok_hook_persist_imei(pvt); return rv; }
static int at_response_cimi(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_cimi(pvt,str); svistok_hook_load_imsi_state(pvt); return rv; }
static int at_response_cops(struct pvt *pvt, char *str) { int rv=svistok_dongle_impl_at_response_cops(pvt,str); svistok_hook_persist_provider(pvt); return rv; }
static int at_response_csq(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_csq(pvt,str); if(!rv) svistok_hook_persist_csq(pvt); return rv; }
static int at_response_mode(struct pvt *pvt, char *str, size_t len) { int rv=svistok_dongle_impl_at_response_mode(pvt,str,len); svistok_hook_persist_mode(pvt,rv); return rv; }
static int at_response_rssi(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_rssi(pvt,str); if(!rv) svistok_hook_persist_rssi(pvt); return rv; }
