/*
 * Copyright (C) 2014-2026 Anton Dodonov (NativeMind)
 * https://github.com/Anton-Dodonov
 * http://linkedin.com/in/anton-dodonov/
 * mailto:anton.v.dodonov@gmail.com
 */

#ifndef CHAN_DONGLE_PDISCOVERY_H_INCLUDED
#define CHAN_DONGLE_PDISCOVERY_H_INCLUDED
#ifdef SVISTOK_COMPOSED_PDISCOVERY_H_HEADER
#include SVISTOK_COMPOSED_PDISCOVERY_H_HEADER
#else

#include <asterisk-chan-dongle/export.h>	/* EXPORT_DECL EXPORT_DEF */

/* SVISTOK_BASELINE_UNIT enum INTERFACE_TYPE */

/* SVISTOK_BASELINE_UNIT record pdiscovery_ports */

struct pdiscovery_result {
	char			* imei;
	char			* imsi;
	char			* serial;
	struct pdiscovery_ports	ports;
};

/* SVISTOK_BASELINE_UNIT record pdiscovery_cache_item */

/* SVISTOK_BASELINE_UNIT declaration pdiscovery_init */
/* SVISTOK_BASELINE_UNIT declaration pdiscovery_fini */
/* return non-zero if found */
EXPORT_DECL int pdiscovery_lookup(const char * device, const char * imei, const char * imsi, const char * serial, char ** dport, char ** aport);
/* SVISTOK_BASELINE_UNIT declaration pdiscovery_list_begin */
/* SVISTOK_BASELINE_UNIT declaration pdiscovery_list_next */
/* SVISTOK_BASELINE_UNIT declaration pdiscovery_list_end */

#endif /* SVISTOK_COMPOSED_PDISCOVERY_H_HEADER */
#endif /* CHAN_DONGLE_PDISCOVERY_H_INCLUDED */
