/*
 * Asterisk compatibility shim for chan_simbox
 * config.h - Autoconf macro definitions and persistence function prototypes
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <asterisk/compat.h>
#include <asterisk/compiler.h>
#include <strings.h>
#include <string.h>
#include <math.h>

#define HAVE_CONFIG_H 1
#define HAVE_MEMMEM   1
#define HAVE_ICONV    1

#define MODULE_VERSION    "1.1.0"
#define PACKAGE_REVISION  "simbox"
#define MODULE_URL        "https://nativemind.net"
#define MODULE_BUGREPORT  "https://nativemind.net"

/* Enable chan_svistok's manager_event_* functions (in manager.c) which
 * are the real dispatch points for SMS/USSD/call-state events. Without
 * this, manager.h reduces them all to no-op macros. With this, they
 * compile with EXPORT_DEF and call manager_event() — our shim's hook
 * point for bridging all events to simbox_event_cb. */
#define BUILD_MANAGER 1

#ifndef ICONV_CONST
#define ICONV_CONST
#endif

#ifndef ICONV_T
#define ICONV_T iconv_t
#endif

struct pvt;
struct ast_cli_args;

#ifdef __cplusplus
extern "C" {
#endif

void timenow(char* dn);
int getfilei (char* devtype, char* fileitem, char* filetype, int *value);
void getfilei_def (char* devtype, char* fileitem, char* filetype, int *value, int def);
int getfilel (char* devtype, char* fileitem, char* filetype, long *value);
int getfiles (char* devtype, char* fileitem, char* filetype, char *value);
void getfiles_def (char* devtype, char* fileitem, char* filetype, char *value, char *def);
void getfiles_def2 (char* devtype, char* fileitem, char* filetype, char *value, char *def);

int putfilei (char* devtype, char* fileitem, char* filetype, int value);
int putfilel (char* devtype, char* fileitem, char* filetype, long value);
int putfiles (char* devtype, char* fileitem, char* filetype, char* value);
int putgetfilei (char putget, char* devtype, char* fileitem, char* filetype, int value, struct ast_cli_args* a);
int putfileslog (char* devtype, char* fileitem, char* filetype, char* value);
int putfileslog2 (char* devtype, char* fileitem, char* filetype, const char* valueformat, va_list va);

void readglsettings(void);
void readpvtlimits(struct pvt* pvt);
void writepvtlimits(struct pvt* pvt);
void writepvtstate(struct pvt* pvt);
void readpvterrors(struct pvt* pvt);
void writepvterrors(struct pvt* pvt);
void readpvtinfo(struct pvt* pvt);
void writepvtinfo(struct pvt* pvt);

void limits_temp(struct pvt* pvt);
void limits_final(struct pvt* pvt, int duration);
void calltry(char* imsi, char* numbera, char* numberb, char* spec);
int msleep(unsigned long milisec);

void v_stat_call_start(struct pvt* pvt);
void v_stat_call_process(struct pvt* pvt);
void v_stat_call_response(struct pvt* pvt);
void v_stat_call_end(struct pvt* pvt, int duration);
void v_stat_call_pddc(struct pvt* pvt);
void v_stat_call_connected(struct pvt* pvt);

void callendout(
    char* IMSI, char* NUMBERA, char* NUMBERB, char* NUMBERMY,
    char* DONGLES, char* DONGLENAME, long int EPOCH_i, long int ANSWERED_i,
    long int TOTALSEC_i, long int BILLSEC_i, long int FASSEC_i, long int PDDCSEC_i,
    char* DONGLEIMEI, char* DONGLEIMSI, char* LAC, char* CELL,
    int END_STATUS_i, int CC_CAUSE_i, char* spec, char* qos, char* vip,
    long int pdd_i, long int pdds_i, char* naprstr, char* im,
    char* uid, char* pro, char* cap, long int fas_i, long int epdd_i,
    long int fpdd_i, long int hem_i, long int hoa_i, int em_type_i
);

void callendin(
    char* IMSI, char* NUMBERB, char* NUMBERMY, char* DONGLES,
    char* DONGLENAME, long int TOTALSEC_i, long int BILLSEC_i,
    char* DONGLEIMEI, char* DONGLEIMSI, char* LAC, char* CELL,
    int END_STATUS_i, int CC_CAUSE_i, char* uid
);

void at_log(struct pvt* pvt, const char* buf, size_t count);

int can_sms(struct pvt* pvt);
void make_dongles_imsi_list(void);
void dserial_clearname(struct pvt *pvt);
char *dserial_getname(char *serial);

void ttyprog_set_diagmode(int fd);
void ttyprog_changeimei(int fd, char *newimei);

int get_file_x(const char *filename, unsigned int *value);
int sysdev_getport(char *device, unsigned int ifaceno, char *port);
int iface_getport(char *filename, char *port);

void dserial_init(void);
void clear_state(void);
void IAXME_get(void);

void ast_mutex_unlock_pvt(struct pvt* pvt);
int mutex_lock_pvt_e(struct pvt* pvt, const char* filename, int lineno);
int mutex_unlock_pvt_e(struct pvt* pvt, const char* filename, int lineno);
int mutex_trylock_pvt_e(struct pvt* pvt, const char* filename, int lineno);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
