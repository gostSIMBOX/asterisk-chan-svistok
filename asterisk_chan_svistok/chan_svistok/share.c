#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

//include "share.h"

#define PIMSI	1
#define PDONGLE	2
#define PBS	3



/*
void ast_mutex_lock_pvt(struct pvt* pvt)
{

    mutex_lock_pvt_e(pvt, __FILE__, __LINE__);
}
*/


void ast_mutex_unlock_pvt(struct pvt* pvt)
{

    mutex_unlock_pvt_e(pvt, __FILE__, __LINE__);
}

/*
int ast_mutex_trylock_pvt(struct pvt* pvt)
{

    mutex_trylock_pvt_e(pvt, __FILE__, __LINE__);
}
*/



int mutex_lock_pvt_e(struct pvt* pvt, const char* filename, int lineno)
{
    int res;

    pvt->lock_start=(long)time(NULL);
    putfilel("dongles/state",PVT_ID(pvt),"lock_start",pvt->lock_start);
    putfiles("dongles/state",PVT_ID(pvt),"lock_filename",filename);
    putfilei("dongles/state",PVT_ID(pvt),"lock_lineno",lineno);

    res=ast_mutex_lock(&pvt->lock);
    return res;
}


int mutex_unlock_pvt_e(struct pvt* pvt, const char* filename, int lineno)
{
    int res;
    pvt->lock_start=0;
    putfilel("dongles/state",PVT_ID(pvt),"lock_start",pvt->lock_start);

    res=ast_mutex_unlock(&pvt->lock);
    return res;
}

int mutex_trylock_pvt_e(struct pvt* pvt, const char* filename, int lineno)
{
    int res;
    if(pvt->lock_start==0) 
    {
        putfiles("dongles/state",PVT_ID(pvt),"lock_filename",filename);
	putfilei("dongles/state",PVT_ID(pvt),"lock_lineno",lineno);

	pvt->lock_start=(long)time(NULL);
    }
    putfilel("dongles/state",PVT_ID(pvt),"lock_start",pvt->lock_start);

    res=ast_mutex_trylock(&pvt->lock);
    return res;
}




char IAXME1[256]="server";

void readpvtsettings(struct pvt* pvt)
{
if (!getfilei("sim/limits",pvt->imsi,"limit_soft.0",&PVT_STAT(pvt, limit_soft[0]))) {PVT_STAT(pvt, limit_soft[0])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit_soft.1",&PVT_STAT(pvt, limit_soft[1]))) {PVT_STAT(pvt, limit_soft[1])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit_soft.2",&PVT_STAT(pvt, limit_soft[2]))) {PVT_STAT(pvt, limit_soft[2])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit_soft.3",&PVT_STAT(pvt, limit_soft[3]))) {PVT_STAT(pvt, limit_soft[3])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit_soft.4",&PVT_STAT(pvt, limit_soft[4]))) {PVT_STAT(pvt, limit_soft[4])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit_soft.5",&PVT_STAT(pvt, limit_soft[5]))) {PVT_STAT(pvt, limit_soft[5])=0;}


if (!getfilei("sim/limits",pvt->imsi,"limit_hard.0",&PVT_STAT(pvt, limit_hard[0]))) {PVT_STAT(pvt, limit_hard[0])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit_hard.1",&PVT_STAT(pvt, limit_hard[1]))) {PVT_STAT(pvt, limit_hard[1])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit_hard.2",&PVT_STAT(pvt, limit_hard[2]))) {PVT_STAT(pvt, limit_hard[2])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit_hard.3",&PVT_STAT(pvt, limit_hard[3]))) {PVT_STAT(pvt, limit_hard[3])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit_hard.4",&PVT_STAT(pvt, limit_hard[4]))) {PVT_STAT(pvt, limit_hard[4])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit_hard.5",&PVT_STAT(pvt, limit_hard[5]))) {PVT_STAT(pvt, limit_hard[5])=0;}

if (!getfilei("sim/settings",pvt->imsi,"alg.0",&PVT_STAT(pvt, alg[0]))) {PVT_STAT(pvt, alg[0])=0;}
if (!getfilei("sim/settings",pvt->imsi,"alg.1",&PVT_STAT(pvt, alg[1]))) {PVT_STAT(pvt, alg[1])=0;}
if (!getfilei("sim/settings",pvt->imsi,"alg.2",&PVT_STAT(pvt, alg[2]))) {PVT_STAT(pvt, alg[2])=0;}
if (!getfilei("sim/settings",pvt->imsi,"alg.3",&PVT_STAT(pvt, alg[3]))) {PVT_STAT(pvt, alg[3])=0;}
if (!getfilei("sim/settings",pvt->imsi,"alg.4",&PVT_STAT(pvt, alg[4]))) {PVT_STAT(pvt, alg[4])=0;}
if (!getfilei("sim/settings",pvt->imsi,"alg.5",&PVT_STAT(pvt, alg[5]))) {PVT_STAT(pvt, alg[5])=0;}

if (!getfilei("sim/settings",pvt->imsi,"nodiff.0",&PVT_STAT(pvt, nodiff[0]))) {PVT_STAT(pvt, nodiff[0])=0;}
if (!getfilei("sim/settings",pvt->imsi,"nodiff.1",&PVT_STAT(pvt, nodiff[1]))) {PVT_STAT(pvt, nodiff[1])=0;}
if (!getfilei("sim/settings",pvt->imsi,"nodiff.2",&PVT_STAT(pvt, nodiff[2]))) {PVT_STAT(pvt, nodiff[2])=0;}
if (!getfilei("sim/settings",pvt->imsi,"nodiff.3",&PVT_STAT(pvt, nodiff[3]))) {PVT_STAT(pvt, nodiff[3])=0;}
if (!getfilei("sim/settings",pvt->imsi,"nodiff.4",&PVT_STAT(pvt, nodiff[4]))) {PVT_STAT(pvt, nodiff[4])=0;}
if (!getfilei("sim/settings",pvt->imsi,"nodiff.5",&PVT_STAT(pvt, nodiff[5]))) {PVT_STAT(pvt, nodiff[5])=0;}

}


void readpvtlimits(struct pvt* pvt)
{
if (!getfilei("sim/limits",pvt->imsi,"limit.0",&PVT_STAT(pvt, limit[0]))) {PVT_STAT(pvt, limit[0])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit.1",&PVT_STAT(pvt, limit[1]))) {PVT_STAT(pvt, limit[1])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit.2",&PVT_STAT(pvt, limit[2]))) {PVT_STAT(pvt, limit[2])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit.3",&PVT_STAT(pvt, limit[3]))) {PVT_STAT(pvt, limit[3])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit.4",&PVT_STAT(pvt, limit[4]))) {PVT_STAT(pvt, limit[4])=0;}
if (!getfilei("sim/limits",pvt->imsi,"limit.5",&PVT_STAT(pvt, limit[5]))) {PVT_STAT(pvt, limit[5])=0;}
}

void writepvtlimits(struct pvt* pvt)
{
putfilei("sim/limits",pvt->imsi,"limit.0",PVT_STAT(pvt, limit[0]));
putfilei("sim/limits",pvt->imsi,"limit.1",PVT_STAT(pvt, limit[1]));
putfilei("sim/limits",pvt->imsi,"limit.2",PVT_STAT(pvt, limit[2]));
putfilei("sim/limits",pvt->imsi,"limit.3",PVT_STAT(pvt, limit[3]));
putfilei("sim/limits",pvt->imsi,"limit.4",PVT_STAT(pvt, limit[4]));
putfilei("sim/limits",pvt->imsi,"limit.5",PVT_STAT(pvt, limit[5]));
}

void writepvtstate(struct pvt* pvt)
{
putfiles("dongles/state",PVT_ID(pvt),"imsi",pvt->imsi);
putfiles("dongles/state",PVT_ID(pvt),"imei",pvt->imei);
putfiles("dongles/state",PVT_ID(pvt),"serial",pvt->serial);
putfiles("dongles/state",PVT_ID(pvt),"iccid",pvt->iccid);

putfiles("dongles/state",PVT_ID(pvt),"model",pvt->model);
putfiles("dongles/state",PVT_ID(pvt),"manufacturer",pvt->manufacturer);
putfiles("dongles/state",PVT_ID(pvt),"firmware",pvt->firmware);
putfiles("dongles/state",PVT_ID(pvt),"audio",PVT_STATE(pvt,audio_tty));
putfiles("dongles/state",PVT_ID(pvt),"data",PVT_STATE(pvt,data_tty));
putfiles("dongles/state",PVT_ID(pvt),"net",PVT_STATE(pvt,net));
putfiles("dongles/state",PVT_ID(pvt),"dev",PVT_STATE(pvt,dev));

putfiles("net",PVT_STATE(pvt,net),"dongle",PVT_ID(pvt));

putfilei("dongles/state",PVT_ID(pvt),"pinrequired",pvt->pinrequired);
putfilei("dongles/state",PVT_ID(pvt),"srvst",pvt->srvst);
putfilei("dongles/state",PVT_ID(pvt),"simst",pvt->simst);
putfilei("dongles/state",PVT_ID(pvt),"cfun",pvt->cfun);
}



void readpvterrors(struct pvt* pvt);

void readpvterrors(struct pvt* pvt)
{

if (!getfilei("dongles/errors",PVT_ID(pvt),"errors.0",&PVT_STAT(pvt, stat_errors[0]))) {PVT_STAT(pvt, stat_errors[0])=0;}
if (!getfilei("dongles/errors",PVT_ID(pvt),"errors.1",&PVT_STAT(pvt, stat_errors[1]))) {PVT_STAT(pvt, stat_errors[1])=0;}
if (!getfilei("dongles/errors",PVT_ID(pvt),"errors.2",&PVT_STAT(pvt, stat_errors[2]))) {PVT_STAT(pvt, stat_errors[2])=0;}
}


void writepvterrors(struct pvt* pvt);

void writepvterrors(struct pvt* pvt)
{
putfilei("dongles/errors",PVT_ID(pvt),"errors.0",PVT_STAT(pvt, stat_errors[0]));
putfilei("dongles/errors",PVT_ID(pvt),"errors.1",PVT_STAT(pvt, stat_errors[1]));
putfilei("dongles/errors",PVT_ID(pvt),"errors.2",PVT_STAT(pvt, stat_errors[2]));
}


void timenow(char* dn)
{
    long ltime;
    time(&ltime);

    struct tm *newtime;
    newtime=localtime(&ltime);
    sprintf(dn,"%d-%02d-%02d %02d:%02d:%02d",newtime->tm_year+1900,newtime->tm_mon+1,newtime->tm_mday,newtime->tm_hour,newtime->tm_min,newtime->tm_sec);
    //return asctime(newtime);
}

void datenow(char* dn)
{
    long ltime;
    time(&ltime);

    struct tm *newtime;
    newtime=localtime(&ltime);
    
    sprintf(dn,"%d-%02d-%02d",newtime->tm_year+1900,newtime->tm_mon+1,newtime->tm_mday);

//    strcpy(dn,asctime(newtime));
}

void readglsettings()
{
    getfilei_def("dongles", "global",  "nosim2offline",&nosim2offline,1);
}


void readpvtinfo(struct pvt* pvt)
{

char tmp[256];
char dn[256];
if(pvt==NULL) return;
timenow(dn);
getfiles_def("sim/statistics",    pvt->imsi,  "date_activated",tmp,dn);

getfilei_def("sim/settings",    pvt->imsi,  "group",&CONF_SHARED(pvt, group),0);

getfilei_def("dongles/statistics",PVT_ID(pvt),"stat_out_calls",&PVT_STAT(pvt, stat_out_calls[1]),0);
getfilei_def("sim/statistics",    pvt->imsi,  "stat_out_calls",&PVT_STAT(pvt, stat_out_calls[2]),0);

getfilei_def("dongles/statistics",PVT_ID(pvt),"stat_calls_duration",&PVT_STAT(pvt, stat_calls_duration[1]),0);
getfilei_def("sim/statistics",    pvt->imsi,  "stat_calls_duration",&PVT_STAT(pvt, stat_calls_duration[2]),0);

getfilei_def("dongles/statistics",PVT_ID(pvt),"stat_wait_duration",&PVT_STAT(pvt, stat_wait_duration[1]),0);
getfilei_def("sim/statistics",    pvt->imsi,  "stat_wait_duration",&PVT_STAT(pvt, stat_wait_duration[2]),0);

getfilei_def("dongles/statistics",PVT_ID(pvt),"stat_calls_answered",&PVT_STAT(pvt, stat_calls_answered[1]),0);
getfilei_def("sim/statistics",    pvt->imsi,  "stat_calls_answered",&PVT_STAT(pvt, stat_calls_answered[2]),0);

getfilei_def("dongles/staisticst",PVT_ID(pvt),"stat_acdl",&PVT_STAT(pvt, stat_acdl[1]),ACDLINIT*1000);
getfilei_def("sim/statistics",    pvt->imsi,  "stat_acdl",&PVT_STAT(pvt, stat_acdl[2]),ACDLINIT*1000);

getfilei_def("dongles/statistics",PVT_ID(pvt),"stat_asrl",&PVT_STAT(pvt, stat_asrl[1]),ASRLINIT);
getfilei_def("sim/statistics",    pvt->imsi,  "stat_asrl",&PVT_STAT(pvt, stat_asrl[2]),ASRLINIT);

getfilei_def("dongles/statistics",PVT_ID(pvt),"stat_pddl0",&PVT_STAT(pvt, stat_pddl[0][1]),PDDLINIT);
getfilei_def("sim/statistics",    pvt->imsi,  "stat_pddl0",&PVT_STAT(pvt, stat_pddl[0][2]),PDDLINIT);

getfilei_def("dongles/statistics",PVT_ID(pvt),"stat_pddl1",&PVT_STAT(pvt, stat_pddl[1][1]),PDDLINIT);
getfilei_def("sim/statistics",    pvt->imsi,  "stat_pddl1",&PVT_STAT(pvt, stat_pddl[1][2]),PDDLINIT);

getfilei_def("dongles/statistics",PVT_ID(pvt),"stat_datt",&PVT_STAT(pvt, stat_datt[1]),0);
getfilei_def("sim/statistics",    pvt->imsi,  "stat_datt",&PVT_STAT(pvt, stat_datt[2]),0);

getfilei_def("sim/statistics",    pvt->imsi,  "stat_in_duration",&PVT_STAT(pvt, stat_in_duration),0);
getfilei_def("sim/statistics",    pvt->imsi,  "stat_in_answered",&PVT_STAT(pvt, stat_in_answered),0);

getfilei_def("sim/statistics",    pvt->imsi,  "stat_iatt",&PVT_STAT(pvt, stat_iatt),0);

getfilei_def("sim/statistics",    pvt->imsi,  "stat_satt",&PVT_STAT(pvt, stat_satt),0);


getfiles_def("sim/statistics",    pvt->imsi,  "number",&PVT_STAT(pvt, number),"");
getfiles_def("sim/statistics",    pvt->imsi,  "balance",&PVT_STAT(pvt, balance),"");
getfiles_def("sim/statistics",    pvt->imsi,  "bal_last",&PVT_STAT(pvt, ballast),"");
getfiles_def("sim/statistics",    pvt->imsi,  "tarif",&PVT_STAT(pvt, tarif),"");


getfilei_def("sim/settings",    pvt->imsi,  "priority",&PVT_STAT(pvt, priority),5);
getfiles_def2("sim/settings",    pvt->imsi,  "pro",pvt->pro,"");

getfilei_def("sim/settings",    pvt->imsi,  "diff_min",&PVT_STAT(pvt, diff_min),120);
getfilei_def("sim/settings",    pvt->imsi,  "diff_min_out",&PVT_STAT(pvt, diff_min_out),60);
getfilei_def("sim/settings",    pvt->imsi,  "diff_min_vip",&PVT_STAT(pvt, diff_min_vip),40);
getfilei_def("sim/settings",    pvt->imsi,  "diff_min_goo",&PVT_STAT(pvt, diff_min_goo),30);
getfilei_def("sim/settings",    pvt->imsi,  "diff_min_nor",&PVT_STAT(pvt, diff_min_nor),50);
getfilei_def("sim/settings",    pvt->imsi,  "diff_min_sout",&PVT_STAT(pvt, diff_min_sout),30);
getfilei_def("sim/settings",    pvt->imsi,  "diff_min_imode",&PVT_STAT(pvt, diff_min_imode),10);

getfilei_def("sim/settings",    pvt->imsi,  "active",&PVT_STAT(pvt, active),1);


getfilei_def("sim/settings",    pvt->imsi,  "can_in",&PVT_STAT(pvt, can_in),1);
getfilei_def("sim/settings",    pvt->imsi,  "can_out",&PVT_STAT(pvt, can_out),1);
getfilei_def("sim/settings",    pvt->imsi,  "can_sout",&PVT_STAT(pvt, can_sout),1);


getfilei_def("sim/settings",    pvt->imsi,  "igoo",&PVT_STAT(pvt, igoo),1);
getfilei_def("sim/settings",    pvt->imsi,  "ibad",&PVT_STAT(pvt, ibad),0);
getfilei_def("sim/settings",    pvt->imsi,  "inew",&PVT_STAT(pvt, inew),0);
getfilei_def("sim/settings",    pvt->imsi,  "ine0",&PVT_STAT(pvt, ine0),0);
getfilei_def("sim/settings",    pvt->imsi,  "inec",&PVT_STAT(pvt, inec),0);
getfilei_def("sim/settings",    pvt->imsi,  "inem",&PVT_STAT(pvt, inem),0);
getfilei_def("sim/settings",    pvt->imsi,  "inor",&PVT_STAT(pvt, inor),0);
getfilei_def("sim/settings",    pvt->imsi,  "inos",&PVT_STAT(pvt, inos),0);

getfilei_def("sim/settings",    pvt->imsi,  "iblo",&PVT_STAT(pvt, iblo),0);
getfilei_def("sim/settings",    pvt->imsi,  "irob",&PVT_STAT(pvt, irob),0);
getfilei_def("sim/settings",    pvt->imsi,  "ivip",&PVT_STAT(pvt, ivip),1);
getfilei_def("sim/settings",    pvt->imsi,  "notvip",&PVT_STAT(pvt, notvip),1);

getfilei_def("sim/settings",    pvt->imsi,  "imn",&PVT_STAT(pvt, imn),1);
getfilei_def("sim/settings",    pvt->imsi,  "ima",&PVT_STAT(pvt, ima),1);
getfilei_def("sim/settings",    pvt->imsi,  "imb",&PVT_STAT(pvt, imb),1);
getfilei_def("sim/settings",    pvt->imsi,  "imc",&PVT_STAT(pvt, imc),1);
getfilei_def("sim/settings",    pvt->imsi,  "imd",&PVT_STAT(pvt, imd),1);
getfilei_def("sim/settings",    pvt->imsi,  "ime",&PVT_STAT(pvt, ime),0);

getfilei_def("sim/settings",    pvt->imsi,  "capnew",&PVT_STAT(pvt, capnew),1);
getfilei_def("sim/settings",    pvt->imsi,  "capfail",&PVT_STAT(pvt, capfail),0);
getfilei_def("sim/settings",    pvt->imsi,  "capok",&PVT_STAT(pvt, capok),1);


getfilei_def("sim/settings",    pvt->imsi,  "imode",&PVT_STAT(pvt, imode),1);
getfilei_def("sim/state",    pvt->imsi,  "in_imode",&PVT_STAT(pvt, in_imode),0);

getfilei_def("sim/settings",    pvt->imsi,  "time_work_wake",&(pvt->time_work_wake),-1);
getfilei_def("sim/settings",    pvt->imsi,  "time_work_sleep",&(pvt->time_work_sleep),-1);

getfilei_def("sim/settings",    pvt->imsi,  "time_holiday_wake",&(pvt->time_holiday_wake),-1);
getfilei_def("sim/settings",    pvt->imsi,  "time_holiday_sleep",&(pvt->time_holiday_sleep),-1);



readpvtsettings(pvt);
}

void writepvtinfo(struct pvt* pvt)
{
if(pvt==NULL) return;

putfiles("dongles/state",PVT_ID(pvt),"imsi",pvt->imsi);
putfiles("sim/state",pvt->imsi,"dongle",PVT_ID(pvt));


putfilei("dongles/statistics",PVT_ID(pvt),"stat_out_calls",PVT_STAT(pvt, stat_out_calls[1]));
putfilei("sim/statistics",   pvt->imsi,   "stat_out_calls",PVT_STAT(pvt, stat_out_calls[2]));


putfilei("dongles/statistics",PVT_ID(pvt),"stat_calls_answered",PVT_STAT(pvt, stat_calls_answered[1]));
putfilei("sim/statistics",pvt->imsi,      "stat_calls_answered",PVT_STAT(pvt, stat_calls_answered[2]));

putfilei("dongles/statistics",PVT_ID(pvt),"stat_calls_duration",PVT_STAT(pvt, stat_calls_duration[1]));
putfilei("sim/statistics",pvt->imsi,      "stat_calls_duration",PVT_STAT(pvt, stat_calls_duration[2]));

putfilei("dongles/statistics",PVT_ID(pvt),"stat_wait_duration",PVT_STAT(pvt, stat_wait_duration[1]));
putfilei("sim/statistics",pvt->imsi,      "stat_wait_duration",PVT_STAT(pvt, stat_wait_duration[2]));


putfilei("dongles/statistics",PVT_ID(pvt),"stat_acdl",PVT_STAT(pvt, stat_acdl[1]));
putfilei("sim/statistics",pvt->imsi,      "stat_acdl",PVT_STAT(pvt, stat_acdl[2]));

putfilei("dongles/statistics",PVT_ID(pvt),"stat_asrl",PVT_STAT(pvt, stat_asrl[1]));
putfilei("sim/statistics",pvt->imsi,      "stat_asrl",PVT_STAT(pvt, stat_asrl[2]));


putfilei("dongles/statistics",PVT_ID(pvt),"stat_pddl0",PVT_STAT(pvt, stat_pddl[0][1]));
putfilei("sim/statistics",pvt->imsi,      "stat_pddl0",PVT_STAT(pvt, stat_pddl[0][2]));
putfilei("dongles/statistics",PVT_ID(pvt),"stat_pddl1",PVT_STAT(pvt, stat_pddl[1][1]));
putfilei("sim/statistics",pvt->imsi,      "stat_pddl1",PVT_STAT(pvt, stat_pddl[1][2]));


putfilei("dongles/statistics",PVT_ID(pvt),"stat_datt",PVT_STAT(pvt, stat_datt[1]));
putfilei("sim/statistics",pvt->imsi,      "stat_datt",PVT_STAT(pvt, stat_datt[2]));

putfilei("sim/statistics",pvt->imsi,      "stat_iatt",PVT_STAT(pvt, stat_iatt));

putfilei("sim/statistics",pvt->imsi,      "stat_satt",PVT_STAT(pvt, stat_satt));

putfilei("sim/statistics",pvt->imsi,      "stat_in_answered",PVT_STAT(pvt, stat_in_answered));
putfilei("sim/statistics",pvt->imsi,      "stat_in_duration",PVT_STAT(pvt, stat_in_duration));

}

void getfilei_def (char* devtype, char* fileitem, char* filetype, int *value, int def)
{
    if (!getfilei(devtype,fileitem,filetype,value))
    {
	*value=def;
	putfilei(devtype,fileitem,filetype,def);
    }
}

void getfilel_def (char* devtype, char* fileitem, char* filetype, long int *value, long int def)
{
    if (!getfilel(devtype,fileitem,filetype,value))
    {
	*value=def;
	putfilel(devtype,fileitem,filetype,def);
    }
}


void getfiles_def (char* devtype, char* fileitem, char* filetype, char *value, char *def)
{
    if (!getfiles(devtype,fileitem,filetype,value))
    {
	strcpy(value,def);
	putfiles(devtype,fileitem,filetype,def);
    }
}

void getfiles_def2 (char* devtype, char* fileitem, char* filetype, char *value, char *def)
{
    if (!getfiles(devtype,fileitem,filetype,value))
    {
	strcpy(value,def);
	//putfiles(devtype,fileitem,filetype,def);
    }
}


int pvt_getfilei (int item, char* dir,struct pvt* pvt, char* filetype, int *value)
{
	char* fileitem;
	if(pvt==NULL) return;
	if(item==PIMSI) fileitem=pvt->imsi;
	if(item==PDONGLE) fileitem=PVT_ID(pvt);
	if(item==PBS) fileitem=pvt->cell_id;

	return getfilei (dir, fileitem, filetype, value);
	
}

int getfilei (char* devtype, char* fileitem, char* filetype, int *value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
  

  *value=0;
  pFile=fopen(filename,"r");
  if (pFile!=NULL)
  {
    if(fscanf(pFile,"%d",value)<=0) return 0;
    fclose(pFile);
    return 1;
  }
  return 0;

}

int getfilel (char* devtype, char* fileitem, char* filetype, long int *value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
  
  *value=0;
  pFile=fopen(filename,"r");
  if (pFile!=NULL)
  {
    if(fscanf(pFile,"%ld",value)<=0) return 0;
    fclose(pFile);
    return 1;
  }
  return 0;

}

int getfiles (char* devtype, char* fileitem, char* filetype, char *value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
  
  *value=0;
  pFile=fopen(filename,"r");
  if (pFile!=NULL)
  {
    if(fscanf(pFile,"%s",value)<=0) *value=0;
    fclose(pFile);
    return 1;
  }
  return 0;

}


int putfilei (char* devtype, char* fileitem, char* filetype, int value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"w");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%d",value);
		fclose(pFile);
		return 1;
	}
	return 0;
	

//    chmod(filename,511);
}


int putfilel (char* devtype, char* fileitem, char* filetype, long int value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"w");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%ld",value);
		fclose(pFile);
		return 1;
	}
	return 0;
	

//    chmod(filename,511);
}


int putfileilog (char* devtype, char* fileitem, char* filetype, int value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;

char dn[256];
timenow(dn);

  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"a");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%s %d\n",dn,value);
		fclose(pFile);
		return 1;
	}
	return 0;
	

//    chmod(filename,511);
}


int putfiles (char* devtype, char* fileitem, char* filetype, char* value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"w");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%s",value);
		fclose(pFile);
		return 1;
	}
	return 0;
	
//    chmod(filename,511);
}


int putfileslog (char* devtype, char* fileitem, char* filetype, char* value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
char dn[256];
timenow(dn);

  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"a");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%s %s\n",dn,value);
		fclose(pFile);
		return 1;
	}
	return 0;
	
//    chmod(filename,511);
}

int putfileslog2 (char* devtype, char* fileitem, char* filetype, const char* valueformat, va_list va)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
char dn[256];
timenow(dn);

  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"a");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%s",dn);
		fprintf(pFile,valueformat,va);
		fclose(pFile);
		return 1;
	}
	return 0;
	
//    chmod(filename,511);
}





int putgetfilei (char putget, char* devtype, char* fileitem, char* filetype, int value,  struct ast_cli_args* a)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
  /*ast_cli (a->fd, "!!!%s!!!\n", filename);*/
  /* return 0;*/
  
  if (putget=='r')
  {
  ast_cli (a->fd, "!!!r!!!\n", filename);

  pFile=fopen(filename,"r");
  if (pFile!=NULL)
  {
    fscanf(pFile,"%d",&value);
    fclose(pFile);
    return 1;
  }
  return 0;
  }

  if (putget=='w')
  {
  ast_cli (a->fd, "!!!w!!!\n", filename);

	pFile=fopen(filename,"w");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%d",value);
		fclose(pFile);
		return 1;
	}
	return 0;
  }
  return 0;
}


void make_bs_list()
{
    struct pvt* pvt;
    int i;
    int bs_count=0;
    int bs_max=256;
    char bs[bs_max][10];
    int  bs_online[bs_max];
    int  bs_connected[bs_max];
    int  bs_rssisum[bs_max];
    
    long int bs_acdl_sum[bs_max];
    long int bs_acd_sum[bs_max];

    long int bs_asrl_sum[bs_max];
    long int bs_asr_sum[bs_max];

    char bs_provider[bs_max][64];
    
    //Получим список и количество Cell

	AST_RWLIST_RDLOCK (&gpublic->devices);
	AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
	{
	if (pvt!=NULL)
	{
        if(*pvt->cell_id!=0)
	    for(i=0;i<bs_count;i++)
	    {
		if(strcmp(bs[i],pvt->cell_id)==0)
		{
			bs_online[i]++;
			bs_rssisum[i]+=pvt->rssi;

			bs_acdl_sum[i]+=PVT_STAT(pvt,stat_acdl[2]);
			bs_acd_sum[i]+=PVT_STAT(pvt,stat_acdl[2]);
			bs_asrl_sum[i]+=PVT_STAT(pvt,stat_asrl[2]);
			bs_asr_sum[i]+=PVT_STAT(pvt,stat_asrl[2]);

			break;
		}
	    }
	    if(!(i<bs_count))
	    {
		bs_count++;
		bs_online[bs_count-1]=1;
		bs_connected[bs_count-1]=0;
		bs_rssisum[bs_count-1]=pvt->rssi;
		
		bs_acdl_sum[i]=PVT_STAT(pvt,stat_acdl[2]);
		bs_acd_sum[i]=PVT_STAT(pvt,stat_acdl[2]);
		bs_asrl_sum[i]=PVT_STAT(pvt,stat_asrl[2]);
		bs_asr_sum[i]=PVT_STAT(pvt,stat_asrl[2]);

		strcpy(bs[bs_count-1],pvt->cell_id);
		strcpy(bs_provider[bs_count-1],pvt->provider_name);
	    }
	}
	}
	AST_RWLIST_UNLOCK (&gpublic->devices);


    char filename_bs[128]="/var/svistok/lists/bs.list";
    //char filename_bs_item[128]="";

    FILE* pFile_bs;
    
    pFile_bs=fopen(filename_bs,"w");
    if (pFile_bs!=NULL)
	{
	    for(i=0;i<bs_count;i++)
	    {
		fprintf(pFile_bs,"%s\n",bs[i]);
		putfilei("bs/state",bs[i],"connected",bs_connected[i]);
		putfilei("bs/state",bs[i],"online",bs_online[i]);
		putfiles("bs/state",bs[i],"provider_name",bs_provider[i]);
		putfilei("bs/state",bs[i],"rssi",bs_rssisum[i]/bs_online[i]);

//		putfilei("bs/state",bs[i],"acdl",bs_acdl_sum[i]/bs_online[i]);
		putfilei("bs/state",bs[i],"acd",bs_acd_sum[i]/bs_online[i]);
//		putfilei("bs/state",bs[i],"asrl",bs_asrl_sum[i]/bs_online[i]);
		putfilei("bs/state",bs[i],"asr",bs_asr_sum[i]/bs_online[i]);

	    }
	    fclose(pFile_bs);
	}
ast_verb(3,"%s","--bsok\n");
    
    
}
void make_dongles_imsi_list()
{
    struct pvt* pvt;
    int bs_count=0;

    char filename_dongles[128]="/var/svistok/lists/dongles.list";
    char filename_imsi[128]="/var/svistok/lists/imsi.list";

//return;

    FILE* pFile_dongles;
    FILE* pFile_imsi;

ast_verb(3,"%s","--writedongles\n");

	pFile_dongles=fopen(filename_dongles,"w");
	if (pFile_dongles!=NULL)
	{
//		AST_RWLIST_RDLOCK (&gpublic->devices);
		AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
		{
//			if (pvt->connected!=0)
//			{
//			if (pvt->nosim!=0)
//			if (*pvt->imei!=0)
//			{
			    fprintf(pFile_dongles,"%s\n",PVT_ID(pvt));
			    putfiles("dongles/state",PVT_ID(pvt),"str_state",pvt_str_state(pvt));
			    putfiles("sim/state",pvt->imsi,"str_state",pvt_str_state(pvt));
//			} else 
//			{
//			    fprintf(pFile_dongles,"%s\n",PVT_ID(pvt));
//			    putfiles("dongles/state",PVT_ID(pvt),"str_state",pvt_str_state(pvt));
//			}

//			}
		}
//	        AST_RWLIST_UNLOCK (&gpublic->devices);

		
		fclose(pFile_dongles);
	}
ast_verb(3,"%s","--writeimsi\n");

        pFile_imsi=fopen(filename_imsi,"w");
	if (pFile_imsi!=NULL)
	{
//		AST_RWLIST_RDLOCK (&gpublic->devices);
		AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
		{
			if (pvt->connected!=0)
			if (*pvt->imsi!=0)
			{
				fprintf(pFile_imsi,"%s\n",pvt->imsi);
				putfiles("sim/state",pvt->imsi,"dongle",PVT_ID(pvt));
			}
		}
//	        AST_RWLIST_UNLOCK (&gpublic->devices);
		fclose(pFile_imsi);
	}

ast_verb(3,"%s","--writedongles_ok\n");

//    make_bs_list();
}

void clear_state()
{
    system("rm /var/svistok/sim/state/*");
    system("rm /var/svistok/lists/*");
    system("rm /var/svistok/lists/dongles.list");
    system("rm /var/svistok/lists/imsi.list");
    system("rm /var/svistok/lists/bs.list");
    return;
}

void IAXME_get()
{
    //char buf
    ast_verb(3,"get IAXME\n");
    //strcpy(IAXME1,pbx_builtin_getvar_helper(NULL,"IAXME"));
    ast_verb(3,"get IAXME OK %s\n",IAXME1);
}