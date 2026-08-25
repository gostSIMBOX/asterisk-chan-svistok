#ifndef SVISTOK_CHARACTERIZATION_STAT_COMPAT_H
#define SVISTOK_CHARACTERIZATION_STAT_COMPAT_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct oracle_stats {
    long stat_call_sf;
    long stat_call_start;
    long stat_call_connected;
    long stat_call_fas;
    long stat_call_pddc;
    long stat_call_saved;
    long stat_call_response;
    long stat_call_process;
    long stat_call_end;
    int billing_pay;
    char billing_direction[32];
};

struct pvt {
    const char *id;
    char imsi[32];
    struct oracle_stats stat;
};

#define PVT_STAT(pvt, field) ((pvt)->stat.field)
#define PVT_ID(pvt) ((pvt)->id)

void putfilei(const char *, const char *, const char *, long);
void putfilel(const char *, const char *, const char *, long);
void putfiles(const char *, const char *, const char *, const char *);
void getfilel_def(const char *, const char *, const char *, long *, long);
void getfiles_def(const char *, const char *, const char *, char *, const char *);
void limits_temp(struct pvt *);
void limits_final(struct pvt *, int);
void timenow(char *);
void datenow(char *);
void ast_verb(int, const char *, ...);
extern char IAXME1[256];

void v_stat_call_start(struct pvt *);
void v_stat_call_response(struct pvt *);
void v_stat_call_pddc(struct pvt *);
void v_stat_call_fas(struct pvt *);
void v_stat_call_connected(struct pvt *);
void v_stat_call_process(struct pvt *);
void v_stat_call_end(struct pvt *, int);

#endif
