#include "math.h"

void limits_final(struct pvt * pvt, int duration);
void limits_temp(struct pvt * pvt);

void limits_temp(struct pvt * pvt)
{

	struct cpvt * cpvt;

	//if(PVT_STAT(pvt,stat_call_saved)==(long)time(NULL)) return;
	
	//writepvtinfo(pvt);

	//Временная запись
	// Округлять до минут в большую сторону ?
	// Взаимодействовать с датой лимита ?

	readpvtlimits(pvt);
	PVT_STAT(pvt,limit[PVT_STAT(pvt,limitnum)])-=((long)time(NULL))-(PVT_STAT(pvt,stat_call_saved));
	PVT_STAT(pvt,stat_call_saved)=(long)time(NULL);
	writepvtlimits(pvt);
	
	PVT_STAT(pvt,stat_call_saved)=(long)time(NULL);
	putfilei("sim",pvt->imsi,"stat_call_saved",PVT_STAT(pvt,stat_call_saved));

	ast_verb(3,"TEST L2 (limits_temp ...)\n");

	//proverka na limit_hard
	if ((PVT_STAT(pvt,limittype)=='_')||(PVT_STAT(pvt,limittype)=='-'))
{

	    ast_verb(3,"TEST L1 %s %d<=%d (%d-%d)\n",PVT_ID(pvt),PVT_STAT(pvt,limit[PVT_STAT(pvt,limitnum)]),(PVT_STAT(pvt,limit_soft[PVT_STAT(pvt,limitnum)]) - PVT_STAT(pvt,limit_hard[PVT_STAT(pvt,limitnum)])),PVT_STAT(pvt,limit_soft[PVT_STAT(pvt,limitnum)]),PVT_STAT(pvt,limit_hard[PVT_STAT(pvt,limitnum)]));

	if(PVT_STAT(pvt,limit[PVT_STAT(pvt,limitnum)])<=0)
	//if(PVT_STAT(pvt,limit[PVT_STAT(pvt,limitnum)])<=(PVT_STAT(pvt,limit_soft[PVT_STAT(pvt,limitnum)]) - PVT_STAT(pvt,limit_hard[PVT_STAT(pvt,limitnum)])) )
	{
	    ast_verb(3,"PEREBOR PO LIMITU %s %d<=%d\n",PVT_ID(pvt),PVT_STAT(pvt,limit[PVT_STAT(pvt,limitnum)]),(PVT_STAT(pvt,limit_soft[PVT_STAT(pvt,limitnum)]) - PVT_STAT(pvt,limit_hard[PVT_STAT(pvt,limitnum)])));

	        AST_LIST_TRAVERSE(&pvt->chans, cpvt, entry)
		{
	                if(cpvt->dir == CALL_DIR_OUTGOING)
			{
				queue_hangup(cpvt->channel,0);
			}
		}
	}
}

}

void limits_final(struct pvt * pvt, int duration)
{
	readpvtlimits(pvt);

	//S okrugleniem
	//PVT_STAT(pvt,limit[PVT_STAT(pvt,limitnum)])-=ceil(((float)duration)/60)*60;

	// ?? Vozvrashaem bez tmp
	PVT_STAT(pvt,limit[PVT_STAT(pvt,limitnum)])+=PVT_STAT(pvt,stat_call_saved)-PVT_STAT(pvt,stat_call_connected);
	if (PVT_STAT(pvt,limitnum)!='_')
	{
	    PVT_STAT(pvt,limit[PVT_STAT(pvt,limitnum)])-=duration;
	} else {
	    //Okruglaem
	    PVT_STAT(pvt,limit[PVT_STAT(pvt,limitnum)])-=ceil(((float)duration+5)/60)*60;
	}
	PVT_STAT(pvt,limitnum)=0;
	//уменьшить лимиты
	writepvtlimits(pvt);
	
	PVT_STAT(pvt,stat_call_saved)=PVT_STAT(pvt,stat_call_connected)+duration;
	putfilei("sim",pvt->imsi,"stat_call_saved",PVT_STAT(pvt,stat_call_saved));

}