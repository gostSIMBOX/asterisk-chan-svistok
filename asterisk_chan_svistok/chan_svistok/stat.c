//void v_stat_call_start(struct pvt * pvt);
//void v_stat_call_connected(struct pvt * pvt);
//void v_stat_call_process(struct pvt * pvt);
//void v_stat_call_end(struct pvt * pvt);




void v_stat_save(struct pvt * pvt)
{
    putfilei("sim",pvt->imsi,"stat_call_saved",PVT_STAT(pvt,stat_call_start));
    putfilei("sim",pvt->imsi,"stat_call_start",PVT_STAT(pvt,stat_call_start));
    putfilei("sim",pvt->imsi,"stat_call_connected",PVT_STAT(pvt,stat_call_connected));
    putfilei("sim",pvt->imsi,"stat_call_saved",PVT_STAT(pvt,stat_call_saved));

    putfilei("sim",pvt->imsi,"stat_call_fas",PVT_STAT(pvt,stat_call_fas));
    putfilei("sim",pvt->imsi,"stat_call_response",PVT_STAT(pvt,stat_call_response));
}

void v_stat_call_start(struct pvt * pvt)
{
    PVT_STAT(pvt,stat_call_sf)=0;

    PVT_STAT(pvt,stat_call_start)=(long)time(NULL);
    PVT_STAT(pvt,stat_call_connected)=0;
    PVT_STAT(pvt,stat_call_fas)=0;
    PVT_STAT(pvt,stat_call_pddc)=0;
    PVT_STAT(pvt,stat_call_saved)=0;
    PVT_STAT(pvt,stat_call_response)=0;
    PVT_STAT(pvt,stat_call_process)=0;

    v_stat_save(pvt);

    
    putfilei("sim/state",pvt->imsi,"busy",1);
    ast_verb(3,"TEST L-START %s \n",PVT_ID(pvt));
}

void v_stat_call_response(struct pvt * pvt) // видимо когда соединиля по CONF
{
    if(PVT_STAT(pvt,stat_call_response)==0)
        PVT_STAT(pvt,stat_call_response)=(long)time(NULL);
}

void v_stat_call_pddc(struct pvt * pvt)
{
    PVT_STAT(pvt,stat_call_pddc)=(long)time(NULL);
}

void v_stat_call_fas(struct pvt * pvt)
{
    PVT_STAT(pvt,stat_call_fas)=(long)time(NULL);
}

void v_stat_call_connected(struct pvt * pvt) // answer
{
    PVT_STAT(pvt,stat_call_sf)=1;
    
    PVT_STAT(pvt,stat_call_connected)=(long)time(NULL);
    PVT_STAT(pvt,stat_call_saved)=(long)time(NULL);

    if(PVT_STAT(pvt,stat_call_fas)==0) PVT_STAT(pvt,stat_call_fas)=(long)time(NULL);
    if(PVT_STAT(pvt,stat_call_pddc)==0) PVT_STAT(pvt,stat_call_pddc)=(long)time(NULL);


    putfilei("sim",pvt->imsi,"stat_call_connected",PVT_STAT(pvt,stat_call_connected));
    putfilei("sim",pvt->imsi,"stat_call_saved",PVT_STAT(pvt,stat_call_saved));

    putfilei("sim",pvt->imsi,"state_busy",1);
    ast_verb(3,"TEST L-CONNECTED %s \n",PVT_ID(pvt));
}

void v_stat_call_process(struct pvt * pvt)
{
//    if(PVT_STAT(pvt,stat_call_process)==0)
//        PVT_STAT(pvt,stat_call_process)=(long)time(NULL);

//    ast_verb(3,"TEST L4 %s \n",PVT_ID(pvt));


    if(PVT_STAT(pvt,stat_call_sf)==1)
    //промежуточная запись лимитов
    if(  
         (PVT_STAT(pvt,stat_call_saved)>0) && 
         (((long)time(NULL)-PVT_STAT(pvt,stat_call_saved))>5)
      )
    {
//    ast_verb(3,"TEST L3");

	//промежуточная запись лимитов
	limits_temp(pvt);
    }
}

void billing_save(struct pvt * pvt, long int duration)
{
    long int seconds;
    char dn[256];
    
    datenow(dn);
    //char date[256];
    //filename=billing_date.billing_direction

if ((duration>0)&&(duration<2000))
{
    if (PVT_STAT(pvt,billing_pay)==1)
    {
        getfilel_def("sim/statistics/billing",dn,PVT_STAT(pvt,billing_direction),&seconds,(long int)0);
        seconds+=duration;
        putfilel("sim/statistics/billing",dn,PVT_STAT(pvt,billing_direction),seconds);
    } else {
        getfilel_def("sim/statistics/billing",dn,"system",&seconds,(long int)0);
        seconds+=duration;
        putfilel("sim/statistics/billing",dn,"system",seconds);
    }
}

}

void v_stat_call_end(struct pvt * pvt, int duration)
{
    char tmp[256];
    char dn[256];

    if(duration>60)
    {
	timenow(dn);
        getfiles_def("sim/statistics",    pvt->imsi,  "date_1call",tmp,dn);
        putfiles("sim/statistics",    pvt->imsi,  "date_lcall",dn);
    }



    PVT_STAT(pvt,stat_call_sf)=0;
    
    PVT_STAT(pvt,stat_call_end)=(long)time(NULL);
    putfilei("sim",pvt->imsi,"stat_call_end",PVT_STAT(pvt,stat_call_end));
    
    //финальная запись лимитов
    limits_final(pvt,duration);

    
//    PVT_STAT(pvt,stat_call_start)=0;
//    PVT_STAT(pvt,stat_call_connected)=0;
//    PVT_STAT(pvt,stat_call_saved)=0;

//    putfilei("sim",pvt->imsi,"stat_call_start",PVT_STAT(pvt,stat_call_start));
//    putfilei("sim",pvt->imsi,"stat_call_connected",PVT_STAT(pvt,stat_call_connected));
//    putfilei("sim",pvt->imsi,"stat_call_saved",PVT_STAT(pvt,stat_call_saved));

    putfilei("sim/state",pvt->imsi,"busy",0);
    
    billing_save(pvt, duration);
}

void calltry(char * imsi, char* numbera, char*numberb, char*spec)
{
    char calltry_file[256]="/usr/simbox/system/svistok/calltry.sh ";

    strcat(calltry_file,"\""); strcat(calltry_file,imsi); strcat(calltry_file,"\" "); 
    strcat(calltry_file,"\""); strcat(calltry_file,numbera); strcat(calltry_file,"\" "); 
    strcat(calltry_file,"\""); strcat(calltry_file,numberb); strcat(calltry_file,"\" "); 
    strcat(calltry_file,"\""); strcat(calltry_file,spec); strcat(calltry_file,"\" "); 
    strcat(calltry_file," > /dev/null &");

    ast_verb (3, "  -system(%s)\n", calltry_file);
    system(calltry_file);
}

void callendout(
char* IMSI,
char* NUMBERA,
char* NUMBERB,
char* NUMBERMY,
char* DONGLES,
char* DONGLENAME,
long int EPOCH_i,
long int ANSWERED_i,
long int TOTALSEC_i,
long int BILLSEC_i,
long int FASSEC_i,
long int PDDCSEC_i,
char* DONGLEIMEI,
char* DONGLEIMSI,
char* LAC,
char* CELL,
int END_STATUS_i,
int CC_CAUSE_i,
char* spec,
char* qos,
char* vip,
long int pdd_i,
long int pdds_i,
char* naprstr,
char* im,
char* uid,
char* pro,
char* cap,

long int fas_i,
long int epdd_i,
long int fpdd_i,
long int hem_i,
long int hoa_i,
int em_type_i
);


void callendout(
char* IMSI,
char* NUMBERA,
char* NUMBERB,
char* NUMBERMY,
char* DONGLES,
char* DONGLENAME,
long int EPOCH_i,
long int ANSWERED_i,
long int TOTALSEC_i,
long int BILLSEC_i,
long int FASSEC_i,
long int PDDCSEC_i,
char* DONGLEIMEI,
char* DONGLEIMSI,
char* LAC,
char* CELL,
int END_STATUS_i,
int CC_CAUSE_i,
char* spec,
char* qos,
char* vip,
long int pdd_i,
long int pdds_i,
char* naprstr,
char* im,
char* uid,
char* pro,
char* cap,
long int fas_i,
long int epdd_i,
long int fpdd_i,
long int hem_i,
long int hoa_i,
int em_type_i
)


{
char EPOCH[256];
char ANSWERED[256];
char TOTALSEC[256];
char BILLSEC[256];
char FASSEC[256];
char PDDCSEC[256];

char END_STATUS[256];
char CC_CAUSE[256];
    
char pdd[256];
char pdds[256];

char fas[256];
char epdd[256];
char fpdd[256];
char hem[256];
char hoa[256];
char em_type[256];


    char call_file[1024]="/usr/simbox/system/svistok/callendout.sh ";

    sprintf(EPOCH,"%ld",EPOCH_i);
    sprintf(ANSWERED,"%ld",ANSWERED_i);
    sprintf(TOTALSEC,"%ld",TOTALSEC_i);
    sprintf(BILLSEC,"%ld",BILLSEC_i);

    sprintf(FASSEC,"%ld",FASSEC_i);
    sprintf(PDDCSEC,"%ld",PDDCSEC_i);

    sprintf(END_STATUS,"%d",END_STATUS_i);
    sprintf(CC_CAUSE,"%d",CC_CAUSE_i);

    sprintf(pdd,"%ld",pdd_i);
    sprintf(pdds,"%ld",pdds_i);

    sprintf(fas,"%ld",fas_i);
    sprintf(epdd,"%ld",epdd_i);
    sprintf(fpdd,"%ld",fpdd_i);
    sprintf(hem,"%ld",hem_i);
    sprintf(hoa,"%ld",hoa_i);
    sprintf(em_type,"%d",em_type_i);


    strcat(call_file,"\""); 	strcat(call_file,IMSI);
    strcat(call_file,"\" \"");	strcat(call_file,NUMBERA);
    strcat(call_file,"\" \"");	strcat(call_file,NUMBERB);
    strcat(call_file,"\" \"");	strcat(call_file,NUMBERMY);
    strcat(call_file,"\" \"");	strcat(call_file,DONGLES);
    strcat(call_file,"\" \"");	strcat(call_file,DONGLENAME);
    strcat(call_file,"\" \"");	strcat(call_file,IAXME1);
    strcat(call_file,"\" \"");	strcat(call_file,EPOCH);
    strcat(call_file,"\" \"");	strcat(call_file,ANSWERED);
    strcat(call_file,"\" \"");	strcat(call_file,TOTALSEC);
    strcat(call_file,"\" \"");	strcat(call_file,BILLSEC);
    strcat(call_file,"\" \"");	strcat(call_file,FASSEC);
    strcat(call_file,"\" \"");	strcat(call_file,PDDCSEC);
    strcat(call_file,"\" \"");	strcat(call_file,DONGLEIMEI);
    strcat(call_file,"\" \"");	strcat(call_file,DONGLEIMSI);
    strcat(call_file,"\" \"");	strcat(call_file,LAC);
    strcat(call_file,"\" \"");	strcat(call_file,CELL);
    strcat(call_file,"\" \"");	strcat(call_file,END_STATUS);
    strcat(call_file,"\" \"");	strcat(call_file,CC_CAUSE);
    strcat(call_file,"\" \"");	strcat(call_file,spec);
    strcat(call_file,"\" \"");	strcat(call_file,qos);
    strcat(call_file,"\" \"");	strcat(call_file,vip);
    strcat(call_file,"\" \"");	strcat(call_file,pdd);
    strcat(call_file,"\" \"");	strcat(call_file,pdds);
    strcat(call_file,"\" \"");	strcat(call_file,naprstr);
    strcat(call_file,"\" \"");	strcat(call_file,im);
    strcat(call_file,"\" \"");	strcat(call_file,uid);
    strcat(call_file,"\" \"");	strcat(call_file,pro);
    strcat(call_file,"\" \"");	strcat(call_file,cap);

    strcat(call_file,"\" \"");	strcat(call_file,fas);
    strcat(call_file,"\" \"");	strcat(call_file,epdd);
    strcat(call_file,"\" \"");	strcat(call_file,fpdd);
    strcat(call_file,"\" \"");	strcat(call_file,hem);
    strcat(call_file,"\" \"");	strcat(call_file,hoa);
    strcat(call_file,"\" \"");	strcat(call_file,em_type);

    strcat(call_file,"\"");
    strcat(call_file," >> /tmp/callendout.log &");
    ast_verb (3, "  -system(%s)\n", call_file);
    system(call_file);
}


void callendin(
char* IMSI,
char* NUMBERB,
char* NUMBERMY,
char* DONGLES,
char* DONGLENAME,
long int TOTALSEC_i,
long int BILLSEC_i,
char* DONGLEIMEI,
char* DONGLEIMSI,
char* LAC,
char* CELL,
int END_STATUS_i,
int CC_CAUSE_i,
char* uid
);

void callendin(
char* IMSI,
char* NUMBERB,
char* NUMBERMY,
char* DONGLES,
char* DONGLENAME,
long int TOTALSEC_i,
long int BILLSEC_i,
char* DONGLEIMEI,
char* DONGLEIMSI,
char* LAC,
char* CELL,
int END_STATUS_i,
int CC_CAUSE_i,
char* uid
)


{
char TOTALSEC[256];
char BILLSEC[256];
char END_STATUS[256];
char CC_CAUSE[256];
    

    char call_file[256]="/usr/simbox/system/svistok/callendin.sh ";

    sprintf(TOTALSEC,"%ld",TOTALSEC_i);
    sprintf(BILLSEC,"%ld",BILLSEC_i);
    sprintf(END_STATUS,"%d",END_STATUS_i);
    sprintf(CC_CAUSE,"%d",CC_CAUSE_i);

    strcat(call_file,"\""); 	strcat(call_file,IMSI);
    strcat(call_file,"\" \"");	strcat(call_file,NUMBERB);
    strcat(call_file,"\" \"");	strcat(call_file,NUMBERMY);
    strcat(call_file,"\" \"");	strcat(call_file,DONGLES);
    strcat(call_file,"\" \"");	strcat(call_file,DONGLENAME);
    strcat(call_file,"\" \"");	strcat(call_file,IAXME1);
    strcat(call_file,"\" \"");	strcat(call_file,TOTALSEC);
    strcat(call_file,"\" \"");	strcat(call_file,BILLSEC);
    strcat(call_file,"\" \"");	strcat(call_file,DONGLEIMEI);
    strcat(call_file,"\" \"");	strcat(call_file,DONGLEIMSI);
    strcat(call_file,"\" \"");	strcat(call_file,LAC);
    strcat(call_file,"\" \"");	strcat(call_file,CELL);
    strcat(call_file,"\" \"");	strcat(call_file,END_STATUS);
    strcat(call_file,"\" \"");	strcat(call_file,CC_CAUSE);
    strcat(call_file,"\" \"");	strcat(call_file,uid);
    strcat(call_file,"\"");
    strcat(call_file," >> /tmp/callendin.log &");

    ast_verb (3, "  -system(%s)\n", call_file);
    system(call_file);
}