/* Svistok hook composition fragment. */

static void svistok_hook_persist_manufacturer(struct pvt *pvt) { putfiles("dongles/state",PVT_ID(pvt),"manufacturer",pvt->manufacturer); }
static void svistok_hook_persist_firmware(struct pvt *pvt) { putfiles("dongles/state",PVT_ID(pvt),"firmware",pvt->firmware); }
static void svistok_hook_persist_imei(struct pvt *pvt) { putfiles("dongles/state",PVT_ID(pvt),"imei",pvt->imei); }
static void svistok_hook_load_imsi_state(struct pvt *pvt) { readpvtinfo(pvt); writepvtinfo(pvt); readpvtlimits(pvt); writepvtlimits(pvt); }
static void svistok_hook_persist_provider(struct pvt *pvt) { putfiles("sim/state",pvt->imsi,"provider_name",pvt->provider_name); putfiles("dongles/state",PVT_ID(pvt),"operator",pvt->provider_name); }
static void svistok_hook_persist_csq(struct pvt *pvt) { putfilei("dongles/state",PVT_ID(pvt),"rssi",pvt->rssi); }
static void svistok_hook_persist_mode(struct pvt *pvt, int rv) { if(!rv) { putfilei("dongles/state",PVT_ID(pvt),"mode",pvt->linkmode); putfilei("dongles/state",PVT_ID(pvt),"submode",pvt->linksubmode); } at_enque_sysinfo(&pvt->sys_chan); }
static void svistok_hook_persist_rssi(struct pvt *pvt) { putfilei("dongles/state",PVT_ID(pvt),"rssi",pvt->rssi); }
