int procmp(char * pvt_pro, char * cr_pro)
{
    // 0 - equal, PRO traffic
    // 1 - NOTE equal, NOT PRO traffic
    if (*cr_pro==0) return 1;
    if (*pvt_pro==0) return 1;
    if (strstr(cr_pro,pvt_pro)==cr_pro) return 0;
    return 1;
    //return strcmp(pvt_pro,cr_pro);
}

struct pvt * plain_dial(struct call_request * cr)
{
	struct pvt * pvt = NULL;
	struct pvt * found = NULL;
	struct pvt_select pvt_select;

	ast_debug (1, "Plain dial\n");

		AST_RWLIST_TRAVERSE(&cr->state->devices, pvt, entry)
		{
			if (!strcmp (PVT_ID(pvt), cr->resource))
			{
				//*exists = 1;
				if(can_dial(pvt, cr->opts, cr->requestor))
				{
						if (ast_mutex_trylock_pvt(pvt)!=EBUSY) 
						{
							if(can_dial(pvt, cr->opts, cr->requestor))
							{
								//PLAIN DIAL DONGLE
								pvt->selectbusy=1;
								ast_mutex_unlock_pvt (pvt);
								ast_debug (3, "  - PLAIN DIAL DONGLE %s\n", PVT_ID(pvt));
								pvt_select.pvt=pvt;
								pvt_select.IMB_type='N';
								pvt_select.IMB_pos=0;
								pvt_select_stat(cr,&pvt_select);

								found = pvt;
								//UPGRADE THIS ^^^^^^^^!!!
								//break;
								return found;
							} else {
								ast_debug (3, "  - Busy (ne uspel) %s\n",PVT_ID(pvt));
							}
							ast_mutex_unlock_pvt (pvt);
						} else {
							ast_verb (3, "  PLAIN DIAL CANNOT TRYLOCK 5 %s\n", PVT_ID(pvt));
						}
						
				}
			}
		}
	return NULL;
}


void im_count(char im, char* imsi)
{
	int count;

	if(im=='B') {	getfilei_def("sim/statistics", imsi,  "imb_count",&count,0); count++; putfilei("sim/statistics", imsi,  "imb_count",count);	}
	if(im=='C') {	getfilei_def("sim/statistics", imsi,  "imc_count",&count,0); count++; putfilei("sim/statistics", imsi,  "imc_count",count);	}
	if(im=='D') {	getfilei_def("sim/statistics", imsi,  "imd_count",&count,0); count++; putfilei("sim/statistics", imsi,  "imd_count",count);	}
	if(im=='E') {	getfilei_def("sim/statistics", imsi,  "ime_count",&count,0); count++; putfilei("sim/statistics", imsi,  "ime_count",count);	}
	if(im=='N') {	getfilei_def("sim/statistics", imsi,  "imn_count",&count,0); count++; putfilei("sim/statistics", imsi,  "imn_count",count);	}

}




void call_request_print(struct call_request * cr)
{
        int i;

	ast_verb (3, ">>> call_request\n");
	ast_verb (3, "--  resource=%s\n", cr->resource);
	ast_verb (3, "--  opts=%d\n", cr->opts);
//	ast_verb (3, "--  requestor=%s\n", cr->requestor);
	ast_verb (3, "--  dial_type=%d\n", cr->dial_type);
	ast_verb (3, "--  group=%d\n", cr->group);
	ast_verb (3, "--  limitnum=%d\n", cr->limitnum);
	ast_verb (3, "--  limittype='%c'\n", cr->limittype);
	ast_verb (3, "--  alg='%c'\n", cr->alg);
	ast_verb (3, "--  pro_str=%s\n",cr->pro_str);
	ast_verb (3, "--  numberdial=%s\n",cr->numberdial);
	ast_verb (3, "--  numberb=%s\n",cr->numberb);
	ast_verb (3, "--  numbera=%s\n",cr->numbera);

	ast_verb (3, "--  napr_str=%s\n",cr->napr_str);
	ast_verb (3, "--  billing_pay=%d\n",cr->billing_pay);
	ast_verb (3, "--  billing_direction=%s\n",cr->billing_direction);
	ast_verb (3, "--  IMB_str=%s\n",cr->IMB_str);
	ast_verb (3, "--  IMB_any=%d\n",cr->IMB_any);
	ast_verb (3, "--  IMB_count=%d\n",cr->IMB_count);
//	ast_verb (3, "--  IMB ");
	for(i=0;i<cr->IMB_count;i++)
	    ast_verb (3, "IMB [%d]='%s'\n",i,cr->IMB[i]);
//	ast_verb (3, "\n");


	ast_verb (3, "--  souimsito=%s\n",cr->souimsito);
	ast_verb (3, "--  fas=(%d)\n",cr->fas);
	ast_verb (3, "--  epdd=(%d)\n",cr->epdd);
	ast_verb (3, "--  fpdd=(%d)\n",cr->fpdd);
	ast_verb (3, "--  hem=(%d)\n",cr->hem);
	ast_verb (3, "--  qos_str=%s  (%d)\n",cr->qos_str,cr->qos);
	ast_verb (3, "--  cap_str=%s  (%d)\n",cr->cap_str, cr->cap);
	ast_verb (3, "--  spec_str=%s (%d)\n",cr->spec_str, cr->spec);
	ast_verb (3, "--  pre_str=%s  (%d)\n",cr->pre_str, cr->pre);
	ast_verb (3, "--  pos_str=%s  (%d)\n",cr->pos_str, cr->pos);
	ast_verb (3, "--  diff_penalty_str=%s  (%d)\n",cr->diff_penalty_str, cr->diff_penalty);
	ast_verb (3, "--  uid=%s\n\n",cr->uid);
}



void get_cr_qos(struct call_request * cr)
{
	cr->qos=0;
	cr->billing_pay=1;

	if ((cr->qos_str[0]=='N')&&(cr->qos_str[1]=='O')&&(cr->qos_str[2]=='S')) {cr->qos=0; cr->billing_pay=1;}
	if ((cr->qos_str[0]=='G')&&(cr->qos_str[1]=='O')&&(cr->qos_str[2]=='O')) {cr->qos=1; cr->billing_pay=1;}
	if ((cr->qos_str[0]=='B')&&(cr->qos_str[1]=='A')&&(cr->qos_str[2]=='D')) {cr->qos=2; cr->billing_pay=1;}
	if ((cr->qos_str[0]=='N')&&(cr->qos_str[1]=='O')&&(cr->qos_str[2]=='R')) {cr->qos=3; cr->billing_pay=1;}
	if ((cr->qos_str[0]=='N')&&(cr->qos_str[1]=='E')&&(cr->qos_str[2]=='W')) {cr->qos=4; cr->billing_pay=1;}
	if ((cr->qos_str[0]=='N')&&(cr->qos_str[1]=='E')&&(cr->qos_str[2]=='0')) {cr->qos=40; cr->billing_pay=1;}
	if ((cr->qos_str[0]=='N')&&(cr->qos_str[1]=='E')&&(cr->qos_str[2]=='C')) {cr->qos=41; cr->billing_pay=1;}
	if ((cr->qos_str[0]=='N')&&(cr->qos_str[1]=='E')&&(cr->qos_str[2]=='M')) {cr->qos=42; cr->billing_pay=1;}

	if ((cr->qos_str[0]=='S')&&(cr->qos_str[1]=='O')&&(cr->qos_str[2]=='U')) {cr->qos=5; cr->billing_pay=0;}
	if ((cr->qos_str[0]=='I')&&(cr->qos_str[1]=='M')&&(cr->qos_str[2]=='O')) {cr->qos=6; cr->billing_pay=1;}
	if ((cr->qos_str[0]=='S')&&(cr->qos_str[1]=='Y')&&(cr->qos_str[2]=='S')) {cr->qos=0; cr->billing_pay=0;}
	if ((cr->qos_str[0]=='V')&&(cr->qos_str[1]=='I')&&(cr->qos_str[2]=='P')) {cr->qos=7; cr->billing_pay=1;}
	if ((cr->qos_str[0]=='R')&&(cr->qos_str[1]=='O')&&(cr->qos_str[2]=='B')) {cr->qos=8; cr->billing_pay=1;}
	if ((cr->qos_str[0]=='B')&&(cr->qos_str[1]=='L')&&(cr->qos_str[2]=='O')) {cr->qos=9; cr->billing_pay=1;}

	if(cr->qos==0) // NOS, PROHODIT VSE !!!
	{
	    cr->IMB_any=1;
	}

}

void get_cr_group(struct call_request * cr)
{
    int su1,su3;
    cr->dial_type=0;
    *(cr->billing_direction)=0;
    cr->group=0;
    cr->limitnum=0;
    cr->alg='D';

    su1= ((cr->resource[0] == 'l') || (cr->resource[0] == 'L')) && ((cr->resource[1] >= '0') && (cr->resource[1] <= '9')) && ((cr->resource[2] == '^') || (cr->resource[2] == '*') || (cr->resource[2] == 'd') || (cr->resource[2] == 'D') || (cr->resource[2] == '>') || (cr->resource[2] == '<'))  && ((cr->resource[3] == '-') || (cr->resource[3] == '=') || (cr->resource[3] == '_'));
    su3= ((cr->resource[6] >= '0') && (cr->resource[6] <= '9')) && (
   								     ((cr->resource[4] >= 'A') && (cr->resource[4] <= 'Z')) &&
   								     ((cr->resource[5] >= 'A') && (cr->resource[5] <= 'Z'))
    							    );

    if(su1&&su3)
    {
	cr->dial_type=1;

	cr->billing_direction[0]=cr->resource[4];
	cr->billing_direction[1]=cr->resource[5];
	cr->billing_direction[2]=0;
	cr->group = (int) strtol (&(cr->resource[6]), (char**) NULL, 10);

	cr->limitnum=cr->resource[1]-'0';
	cr->alg=cr->resource[2];
	cr->limittype=cr->resource[3];
    }

}

void parse_cr_im(struct call_request * cr)
{
	char *pch;

	cr->IMB_count=0;
	cr->IMB_any=0;
	
	if (cr->IMB_buf!=NULL) if (cr->IMB_buf[0]!=0)
	{
	    pch = strtok (cr->IMB_buf,";");
	    while ((pch != NULL)&&(*pch!=0)&&(cr->IMB_count<59))
	    {
		    if (strcmp(pch,"999999999999999")!=0)
		    {
			    cr->IMB[cr->IMB_count]=pch;
			    cr->IMB_count++;
		    } else {
			    cr->IMB_any=1;
		    }
		    pch = strtok (NULL, ";");
	     }
	}
}


void call_request_create(struct call_request * cr, struct public_state * state,const char * resource, int opts, const struct ast_channel * requestor)
{
    char tmp[256];

    ast_verb (3, ">>> call_request_create start\n");
    cr->resource=resource;
    cr->requestor=requestor;
    cr->state=state;
    cr->opts=opts;

    *cr->IMB_str=0;
    *cr->IMB_buf=0;
    *cr->qos_str=0;
    *cr->cap_str=0;
    *cr->spec_str=0;
    *cr->napr_str=0;
    *cr->uid=0;
    *cr->pro_str=0;
    *cr->pre_str=0;
    *cr->pos_str=0;
    *cr->diff_penalty_str=0;

    *cr->numbera=0;
    *cr->numberb=0;
    *cr->numberdial=0;
    *cr->souimsito=0;

    cr->spec=0;
    cr->pre=0;
    cr->pos=0;
    cr->cap=0;
    cr->diff_penalty=0;

    cr->fas=0;
    cr->epdd=0;
    cr->fpdd=0;
    cr->hem=0;

    cr->limittype='=';
    cr->limitnum=0;

    strcpy(cr->billing_direction,"HZ");

    if(cr->requestor!=NULL)
    {
	    //ast_channel_show_vars(requestor);
	    ast_channel_get_var(requestor,"_IMB2",cr->IMB_str);
	    strcpy(cr->IMB_buf,cr->IMB_str);

	    ast_channel_get_var(requestor,"fas",tmp); cr->fas=atoi(tmp);
	    ast_channel_get_var(requestor,"epdd",tmp); cr->epdd=atoi(tmp);
	    ast_channel_get_var(requestor,"fpdd",tmp); cr->fpdd=atoi(tmp);
	    ast_channel_get_var(requestor,"hem",tmp); cr->hem=atoi(tmp);

	    ast_channel_get_var(requestor,"qos",cr->qos_str);
	    ast_channel_get_var(requestor,"cap",cr->cap_str);
	    ast_channel_get_var(requestor,"spec",cr->spec_str);
	    ast_channel_get_var(requestor,"naprstr",cr->napr_str);
	    ast_channel_get_var(requestor,"uid",cr->uid);
	    ast_channel_get_var(requestor,"pro",cr->pro_str);
	    ast_channel_get_var(requestor,"pre",cr->pre_str);
	    ast_channel_get_var(requestor,"pos",cr->pos_str);
	    ast_channel_get_var(requestor,"diff_penalty",cr->diff_penalty_str);
	    if(*cr->diff_penalty_str!=0)
	    {
		cr->diff_penalty=atoi(cr->diff_penalty_str);
	    }

	    ast_channel_get_var(requestor,"numbera",cr->numbera);
	    ast_channel_get_var(requestor,"numberb",cr->numberb);

	    ast_channel_get_var(requestor,"souimsito",cr->souimsito);



	    if (strcmp(cr->spec_str,"MAY")==0) cr->spec=20;
	    if (strcmp(cr->spec_str,"NAV")==0) cr->spec=30;
	    if (strcmp(cr->spec_str,"SOU")==0) cr->spec=50;
	    if (strcmp(cr->spec_str,"SOU2")==0) cr->spec=51;
	    if (strcmp(cr->spec_str,"MAG")==0) cr->spec=200;


	    if (strcmp(cr->pre_str,"PRE")==0) cr->pre=1; // spec=2
	    if (strcmp(cr->pos_str,"POS")==0) cr->pos=1; //spec=3;

	    if(strcmp(cr->cap_str,"FAIL")==0) 	cr->cap=1;
	    if(strcmp(cr->cap_str,"OK")==0) 	cr->cap=2;

        } else 
	{
	    ast_verb (3, "!!!! BAD !!! requestor=NULL \n");
	}

	parse_cr_im(cr);
	ast_verb (3, "--  parse_cr_im OK\n");
	get_cr_qos(cr);
	get_cr_group(cr);
	ast_verb (3, "<<< call_request_create\n");
}

void pvt_select_remove(struct pvt_select ** pvt_select,int* num, int * pvt_select_count)
{
			ast_verb (3, "!!! remove %d/%d\n", *num, *pvt_select_count);
			ast_free(pvt_select[*num]);

			pvt_select[*num]=pvt_select[(*pvt_select_count)-1];
			(*pvt_select_count)--;
			(*num)--;
}

void pvt_select_create(struct call_request * cr, struct pvt_select ** pvt_select, int * pvt_select_count)
//void pvt_select_create(struct call_request * cr, struct pvt_select * pvt_select, int * pvt_select_count)
{
	//Проверка только на group, candial и лимит

	int pvt_count=0;
	int new_count=0;

	struct pvt * pvt;
	//struct pvt_select * ps;

	*pvt_select_count=0;

	int uu_total;
	int uu_group, uu_candial, uu_limit;


	ast_verb (3, ">>> pvt_select_create start\n");
	ast_verb (3, "--  pvt-----------  group uu tot=gro && can && lim (LNT = sec      )\n");

	// Generate a list of all availible devices //

	AST_RWLIST_TRAVERSE(&cr->state->devices, pvt, entry)
	{
		if(pvt!=NULL)
		{
			pvt_count++;
			uu_group=(CONF_SHARED(pvt, group) == cr->group);
			uu_candial=(can_dial(pvt, cr->opts, cr->requestor));
			uu_limit=((PVT_STAT(pvt,limit[cr->limitnum])>0)||((cr->limittype != '-')&&(cr->limittype != '_')));

			uu_total= (uu_group && uu_candial && uu_limit);

			if(uu_total)
			{
				ast_verb (3, "++ %15s %5d  uu %2d =%2d    %3d    %3d  (L%d%c   %d ) \n", PVT_ID(pvt),CONF_SHARED(pvt,group),uu_total,uu_group,uu_candial, uu_limit, cr->limitnum,cr->limittype, PVT_STAT(pvt,limit[cr->limitnum]));
				pvt_select[new_count]=(struct pvt_select*)ast_malloc(sizeof(struct pvt_select));
				    if(pvt_select[new_count]==NULL)
				    {
					ast_verb(1,"BIG MALLOC PROBLEM!!! FATAL!!!");
					return;
				    }
				pvt_select[new_count]->pvt=pvt;
				new_count++;
			} else {
				ast_verb (3, "-- %15s %5d  uu %2d =%2d    %3d    %3d  (L%d%c   %d ) \n", PVT_ID(pvt),CONF_SHARED(pvt,group),uu_total,uu_group,uu_candial, uu_limit, cr->limitnum,cr->limittype, PVT_STAT(pvt,limit[cr->limitnum]));
			}
		}
	}

	ast_verb (3, "<<< pvt_select_create count=%d/%d\n", new_count, pvt_count);
	*pvt_select_count=new_count;
}


void pvt_select_im(struct call_request * cr, struct pvt_select ** pvt_select, int * pvt_select_count)
{
	//Проверяем IM

	int uu_im;
	int uu_imb, uu_imc, uu_imd, uu_ime, uu_imn;

	int i,p;
	int IMB_pos;
	char IMB_type;
	char alg;

	int old_count=*pvt_select_count;

	struct pvt * pvt;

	ast_verb (3, ">>> pvt_select_im start\n");

	
    if(cr->qos==5)
    {
	ast_verb (3, "--  pvt-----------  SPECIAL FOR SAMSEBE\n");
	for(i=0;i<*pvt_select_count;i++)
	{
		pvt=pvt_select[i]->pvt;
		pvt_select[i]->IMB_pos=cr->IMB_count;
		pvt_select[i]->IMB_type='N';

		if((strcmp(pvt->imsi,cr->souimsito)==0))
		{
			ast_verb (3, "-- %15s souimsito=%s CAN NOT CALL SAMSEBE\n", PVT_ID(pvt), cr->souimsito);
			pvt_select_remove(pvt_select,&i,pvt_select_count);
		} else {
			ast_verb (3, "++ %15s OK\n", PVT_ID(pvt));
		}
	}
    } else
    {
	ast_verb (3, "--  pvt-----------  typ pos uu im = N|B|C|D|E\n");
	for(i=0;i<*pvt_select_count;i++)
	{
		pvt=pvt_select[i]->pvt;

		//IM
		IMB_pos=cr->IMB_count;
		IMB_type='N';

		alg=PVT_STAT(pvt,alg[cr->limitnum]);

		for(p=0;p<cr->IMB_count;p++)
		    if(strcmp(cr->IMB[p],pvt->imsi)==0)
		    {
			IMB_pos=p;
			if(p==0) IMB_type='B'; else  IMB_type='C';
			break;
		    }
		if(IMB_type=='N')
		{
		    if (cr->IMB_any==0) IMB_type='E';
		    else
		    {
			if (cr->IMB_count==0) IMB_type='N';
			if (cr->IMB_count>0) IMB_type='D';
		    }
		}

		pvt_select[i]->IMB_pos=IMB_pos;
		pvt_select[i]->IMB_type=IMB_type;

		uu_imn=((PVT_STAT(pvt,imn)==1)&&(IMB_type=='N'));
		uu_imb=((PVT_STAT(pvt,imb)==1)&&(IMB_type=='B'));
		uu_imc=((PVT_STAT(pvt,imc)==1)&&(IMB_type=='C'));
		uu_imd=((PVT_STAT(pvt,imd)==1)&&(IMB_type=='D'));
		uu_ime=((PVT_STAT(pvt,ime)==1)&&(IMB_type=='E'));

		if (alg=='A') uu_ime=(IMB_type=='E');
		if (alg=='B') uu_ime=(IMB_type=='E');
		if (alg=='a') uu_ime=(IMB_type=='E');
		if (alg=='b') uu_ime=(IMB_type=='E');


		uu_im=uu_imn||uu_imb||uu_imc||uu_imd||uu_ime;


		//pvt_select[i]->uu_im=uu_im;
		if(uu_im)
			ast_verb (3, "++ %15s  '%c' %3d  %3d  = %1d %1d %1d %1d %1d\n", PVT_ID(pvt),IMB_type,IMB_pos,uu_im,       uu_imn, uu_imb, uu_imc, uu_imd, uu_ime);
		else
		{
			ast_verb (3, "-- %15s  '%c' %3d  %3d  = %1d %1d %1d %1d %1d\n", PVT_ID(pvt),IMB_type,IMB_pos,uu_im,       uu_imn, uu_imb, uu_imc, uu_imd, uu_ime);
			pvt_select_remove(pvt_select,&i,pvt_select_count);
		}
	}
    }


	ast_verb (3, "<<< pvt_select_im count=%d/%d\n", *pvt_select_count, old_count);
}


void pvt_select_qos(struct call_request * cr, struct pvt_select ** pvt_select, int * pvt_select_count)
{
	//Проверяем QOS

	int uu_qos;
	int uu_nos, uu_goo, uu_bad, uu_nor, uu_rob,uu_blo, uu_alq, uu_ald, uu_sou, uu_pro;

	int uu_new, uu_ne0, uu_nec, uu_nem;
	int uu_IMBC, uu_nerob;

	int i;
	char alg;

	int old_count=*pvt_select_count;

	struct pvt * pvt;

	ast_verb (3, ">>> pvt_select_qos start\n");
	ast_verb (3, "    cr->qos_str=%s (%d)\n", cr->qos_str, cr->qos);
	ast_verb (3, "--  pvt----------- uu qos=nos|new|ne0|nec|nem|goo|bad|nor|rob|blo|alq|ald|sou\n");

	for(i=0;i<*pvt_select_count;i++)
	{
	    pvt=pvt_select[i]->pvt;
	    uu_IMBC=((pvt_select[i]->IMB_type=='B')||(pvt_select[i]->IMB_type=='C')); // Звонили с них
	    uu_nerob=((cr->qos!=9) && (cr->qos!=8)); //не прозвонные
	    //QOS
	    if(uu_IMBC&&uu_nerob)
	    {
		    ast_verb (3, "++ %15s   IMBC + NEBOBBLO\n", PVT_ID(pvt));
	    } else
	    {
		alg=PVT_STAT(pvt,alg[cr->limitnum]);


		uu_new=((cr->qos==4)&&(PVT_STAT(pvt,inew)==1));
		uu_ne0=((cr->qos==40)&&(PVT_STAT(pvt,ine0)==1));
		uu_nec=((cr->qos==41)&&(PVT_STAT(pvt,inec)==1));
		uu_nem=((cr->qos==42)&&(PVT_STAT(pvt,inem)==1));

		uu_nos=((cr->qos==0)&&(PVT_STAT(pvt,inos)==1));
		uu_goo=((cr->qos==1)&&(PVT_STAT(pvt,igoo)==1));
		uu_bad=((cr->qos==2)&&(PVT_STAT(pvt,ibad)==1));
		uu_nor=((cr->qos==3)&&(PVT_STAT(pvt,inor)==1));
		uu_rob=((cr->qos==8)&&(PVT_STAT(pvt,irob)==1));
		uu_blo=((cr->qos==9)&&(PVT_STAT(pvt,iblo)==1));

		//uu_pro=((alg=='p')&&((strcmp(pvt->pro,cr->pro_str)==0))); // alg 'p' i PRO modema sovpalo s zvonkom

		uu_pro=(((alg=='p')||(alg=='v'))&&((procmp(pvt->pro,cr->pro_str)==0))); // alg 'p' i PRO modema sovpalo s zvonkom


		uu_alq=((alg == 'Q') && (cr->qos!=9) && (cr->qos!=8));
		uu_ald=((alg == 'd') && (cr->qos!=9) && (cr->qos!=8));
		uu_sou=(cr->qos==5)&&(PVT_STAT(pvt,can_sout)==1) ;
		uu_qos= uu_nos || uu_goo || uu_bad || uu_nor || uu_new || uu_ne0 || uu_nec || uu_nem || uu_rob || uu_blo || uu_alq || uu_ald || uu_sou || uu_pro;

		if(uu_qos)
		{
		    ast_verb (3, "++ %15s   %3d = %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d\n", PVT_ID(pvt),uu_qos, uu_nos, uu_new, uu_ne0, uu_nec, uu_nem, uu_goo, uu_bad, uu_nor, uu_rob, uu_blo, uu_alq, uu_ald, uu_sou, uu_pro );
		} else {
		    ast_verb (3, "-- %15s   %3d = %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d   %1d\n", PVT_ID(pvt),uu_qos, uu_nos, uu_new, uu_ne0, uu_nec, uu_nem, uu_goo, uu_bad, uu_nor, uu_rob, uu_blo, uu_alq, uu_ald, uu_sou, uu_pro );
		    pvt_select_remove(pvt_select,&i,pvt_select_count);
		}
	    }
	}

	ast_verb (3, "<<< pvt_select_qos count=%d/%d\n", *pvt_select_count, old_count);
}

void pvt_select_cappro(struct call_request * cr, struct pvt_select ** pvt_select, int * pvt_select_count)
{
	int i;

	int old_count=*pvt_select_count;

	if (cr->qos==5)
	{
		ast_verb (3, ">>> NO pvt_select_cappro FOR SOUT\n");
		return;
	}

	int uu_pro1, uu_pro2, uu_pro, uu_cap;
	int uu_cap0, uu_cap1,uu_cap2;
	int uu_total;

	struct pvt * pvt;


	ast_verb (3, ">>> pvt_select_cappro start\n");
	ast_verb (3, "    cr->pro_str=%s\n", cr->pro_str);
	ast_verb (3, "    cr->cap_str=%s (%d)\n", cr->cap_str, cr->cap);
	ast_verb (3, "--  pvt----------- tot 'pro' uu    cap(0|1|2) pro\n");

	for(i=0;i<*pvt_select_count;i++)
	{
		pvt=pvt_select[i]->pvt;
		if ((pvt_select[i]->IMB_type=='B')||(pvt_select[i]->IMB_type=='C'))
		{
				ast_verb (3, "++ %15s IM%c PRIORITY\n", PVT_ID(pvt),pvt_select[i]->IMB_type);
		} else 
		{

			// 0 - NEW, 1 - FAIL, 2 - OK
			uu_cap0=((cr->cap==0)&&(PVT_STAT(pvt,capnew)==1));
			uu_cap1=((cr->cap==1)&&(PVT_STAT(pvt,capfail)==1));
			uu_cap2=((cr->cap==2)&&(PVT_STAT(pvt,capok)==1));

			uu_cap=(uu_cap0||uu_cap1||uu_cap2);

			//uu_pro1=(*(pvt->pro)==0); //  PRO ne ustanovleno na modem
			//uu_pro2=(strcmp(pvt->pro,cr->pro_str)==0); // PRO modema sovpalo s PRO ZVONKA
			//uu_pro=(uu_pro1||uu_pro2);

//			uu_pro1=(PVT_STAT(pvt,alg[cr->limitnum])!='P'); // alg P ne ustanovlen na modem
//			uu_pro2=(PVT_STAT(pvt,alg[cr->limitnum])=='P')&&(strcmp(pvt->pro,cr->pro_str)==0); // alg P ustanovlen, no PRO modema sovpalo s PRO ZVONKA


			uu_pro1=((PVT_STAT(pvt,alg[cr->limitnum])!='P')&&(PVT_STAT(pvt,alg[cr->limitnum])!='v')); // alg P ne ustanovlen na modem
			uu_pro2=((PVT_STAT(pvt,alg[cr->limitnum])=='P')||(PVT_STAT(pvt,alg[cr->limitnum])=='v'))&&(procmp(pvt->pro,cr->pro_str)==0); // alg P ustanovlen, no PRO modema sovpalo s PRO ZVONKA


			uu_pro=(uu_pro1||uu_pro2);

			uu_total=uu_cap && uu_pro;

			if(uu_total)
			{
				ast_verb (3, "++ %15s %2d  '%3s'      %3d %1d %1d %1d %3d %3d %3d \n", PVT_ID(pvt),uu_total,pvt->pro, uu_cap,uu_cap0,uu_cap1,uu_cap2, uu_pro, uu_pro1, uu_pro2);
			} else
			{
				ast_verb (3, "-- %15s %2d  '%3s'      %3d %1d %1d %1d %3d %3d %3d\n", PVT_ID(pvt),uu_total,pvt->pro, uu_cap,uu_cap0,uu_cap1,uu_cap2, uu_pro, uu_pro1, uu_pro2);
				pvt_select_remove(pvt_select,&i,pvt_select_count);
			}
		}
	}

	ast_verb (3, "<<< pvt_select_diff count=%d/%d\n", *pvt_select_count, old_count);
}

void pvt_select_work(struct call_request * cr, struct pvt_select ** pvt_select, int * pvt_select_count)
{
	int i;

	int old_count=*pvt_select_count;

	/*
	if (cr->qos==5)
	{
		ast_verb (3, ">>> NO pvt_select_work FOR SOUT\n");
		return;
	}*/


	int w_w,w_s,h_w,h_s,m_w,m_s,t_w,t_s;
	int imsi3;
	int pause,dayholiday;

	int date_n, date_h, date_z,date_m;
	long tim;
	struct pvt * pvt;


	ast_verb (3, ">>> pvt_select_work start\n");

	tim=(long)time(NULL)+14400; // +4 GMT

	date_z=floor(tim/86400); // den
	date_n=floor((date_z+3)%7)+1; // den nedeli 1 - ponedelnik
	date_m=floor((tim%3600)/60); // minuta
	date_h=(int)(floor(tim/3600))%24; //chas

	ast_verb (3, "    now=%2d:%2d date_n=%d date_z=%d\n", date_h,date_m, date_n, date_z);

	ast_verb (3, "--  pvt----------- pause w_w w_s h_w h_s m_w m_s\n");

	for(i=0;i<*pvt_select_count;i++)
	{
		pvt=pvt_select[i]->pvt;
		if ((pvt_select[i]->IMB_type=='B')||(pvt_select[i]->IMB_type=='C'))
		{
				ast_verb (3, "++ %15s IM%c PRIORITY\n", PVT_ID(pvt),pvt_select[i]->IMB_type);
		} else 
		{
			w_w=pvt->time_work_wake;
			w_s=pvt->time_work_sleep;

			h_w=pvt->time_holiday_wake;
			h_s=pvt->time_holiday_sleep;

			imsi3=((pvt->imsi[12]-'0')*100)+((pvt->imsi[13]-'0')*10)+((pvt->imsi[14]-'0'));

			m_w=(((imsi3*17)+(date_z*19))*17)%30*2;
			m_s=(((imsi3*17)+(date_z*29))*17)%30*2;

			if(date_n>5)
			{
			    t_w=h_w;
			    t_s=h_s;
			    dayholiday=2;
			} else {
			    t_w=w_w;
			    t_s=w_s;
			    dayholiday=1;
			}
			
			pause=0;
			if(t_w>=0)
			{
				if(date_h<t_w) pause=dayholiday;
				if((date_h==t_w)&&(date_m<m_w)) pause=dayholiday+10;
			}

			if(t_s>=0)
			{
				if((date_h>t_s)) pause=dayholiday;
				if((date_h==t_s)&&(date_m<m_s)) pause=dayholiday+20;
			}


			if(pause==0)
			{
				ast_verb (3, "++ %15s %2d   %3d %3d %3d %3d %3d %3d %3d %3d\n", PVT_ID(pvt), pause, t_w,t_s,w_w,w_s,h_w,h_s,m_w,m_s );
			} else
			{
				ast_verb (3, "-- %15s %2d   %3d %3d %3d %3d %3d %3d %3d %3d\n", PVT_ID(pvt), pause, t_w,t_s,w_w,w_s,h_w,h_s,m_w,m_s );
				pvt_select_remove(pvt_select,&i,pvt_select_count);
			}
		}
	}

	ast_verb (3, "<<< pvt_select_diff count=%d/%d\n", *pvt_select_count, old_count);
}




void pvt_select_diff(struct call_request * cr, struct pvt_select ** pvt_select, int * pvt_select_count)
{
	int i;

	int old_count=*pvt_select_count;

	int diff_end, diff;
	//int diff_start;
	float penalty_koef;
	float alg_koef;
	int alg_penalty;

	struct pvt * pvt;
	char IMB_type;
	char alg;

	ast_verb (3, ">>> pvt_select_diff start\n");
	penalty_koef=1  - ((float)cr->diff_penalty/(float)(100+cr->diff_penalty));
//	ast_verb (3, "    limit_penalty=%d\n", limit_penalty);
	ast_verb (3, "    cr->diff_penalty_str=%s (%d) penalty_koef=%f\n", cr->diff_penalty_str, cr->diff_penalty,penalty_koef);
	ast_verb (3, "--  pvt----------- diff\n");

	for(i=0;i<*pvt_select_count;i++)
	{
		pvt=pvt_select[i]->pvt;
		IMB_type=pvt_select[i]->IMB_type;
		alg=PVT_STAT(pvt,alg[cr->limitnum]);
		alg_penalty=0;
		if (alg=='A') alg_penalty=-10;
		if (alg=='B') alg_penalty=-20;
		if (alg=='a') alg_penalty=-10;
		if (alg=='b') alg_penalty=-20;

//		if (alg=='C') alg_penalty=-20;
		if (alg=='X') alg_penalty=-10;
		if (alg=='Z') alg_penalty=-20;
		alg_koef=1  - ((float)alg_penalty/(float)(100+alg_penalty));

		//diff_start=(long)time(NULL)-PVT_STAT(pvt,stat_call_start);
		diff_end=(long)time(NULL)-PVT_STAT(pvt,stat_call_end);
		if (diff_end>86400) diff_end=86400;
		diff_end=((float)diff_end)*penalty_koef*alg_koef;

		//ast_verb (3, "Selecting %d, %s, limit=%d, acd=, diff_end=%d alg= %c\n", (int)i,PVT_ID(pvt),(int)(PVT_STAT(pvt,limit[limitnum])),(int)diff_end,(PVT_STAT(pvt,alg[limitnum])));

		diff=diff_end-PVT_STAT(pvt,diff_min); // IZMENIT DIFF_PENALTY
		if(cr->qos==5) // SOUT
		{
			diff=diff_end-PVT_STAT(pvt,diff_min_sout);
		} else {
			//if((qos==1)||((qos==7)) diff=diff_end-PVT_STAT(pvt,diff_min_out) // VIP? depretiated???

			if (cr->qos==1) diff=diff_end-PVT_STAT(pvt,diff_min_goo); // Bystriy dla IMB, IMC, a NE qos==1
			if (cr->qos==3) diff=diff_end-PVT_STAT(pvt,diff_min_nor); // Bystriy dla IMB, IMC, a NE qos==1

			if(IMB_type=='B')	diff=diff_end-PVT_STAT(pvt,diff_min_imode);
			if(IMB_type=='C')	diff=diff_end-PVT_STAT(pvt,diff_min_imode);
			if((PVT_STAT(pvt,nodiff[cr->limitnum])==1)&&(cr->limitnum!=0)) diff=diff_end-PVT_STAT(pvt,diff_min_goo); //Nodiff по лимиту = min_goo
		}

		pvt_select[i]->diff=diff;
		if(diff>=0)
		{
			ast_verb (3, "++ %15s   %5d\n", PVT_ID(pvt),diff );
		} else {
			ast_verb (3, "-- %15s   %5d\n", PVT_ID(pvt),diff );
			pvt_select_remove(pvt_select,&i,pvt_select_count);
		}

		if (alg_penalty!=0)
			ast_verb (3, "!! alg_penalty=%5d alg_koef=%5f\n", alg_penalty,alg_koef);

	}

	ast_verb (3, "<<< pvt_select_diff count=%d/%d\n", *pvt_select_count, old_count);
}


void pvt_select_shuffle(struct pvt_select ** pvt_select, int c)
{
	struct pvt_select * tmp;
	int i, rnum;

	ast_verb (3, ">>> Shuffle count=%d\n", (int)c);
			if (c>0)
			for (i=0;i<c-1;i++)
			{
			    rnum=i+(rand()%(c-i));
			    tmp=pvt_select[i];
			    pvt_select[i]=pvt_select[rnum];
			    pvt_select[rnum]=tmp;
//			    ast_verb (9, "%d<>%d\n",i,rnum );
			}
	ast_verb (3, "<<< Shuffle ok count=%d\n", (int)c);
}



void pvt_select_sort(struct call_request * cr, struct pvt_select ** pvt_select, int c)
{
	struct pvt_select * tmp;
	struct pvt * pvt;

	int i,j;

	int us1b, us1r, us2b, us2r, us3b, usa;
	int alg;

	int sou_diff_start;

	ast_verb (3, ">>> pvt_select_sort start\n");

	if (c<=0) return;



	if (cr->qos==5)
	{
		ast_verb (3, ">>> SOUT\n");
		//cr->souimsito
		for (i=0;i<c;i++)
		{
			pvt=pvt_select[i]->pvt;
			sou_diff_start=0;		// (long)time(NULL);
			for (j=0;j<pvt->soupri_count;j++)
			    if (strcmp(pvt->soupri[j].imsi,cr->souimsito)==0) sou_diff_start=pvt->soupri[j].sou_diff_start;
			pvt_select[i]->sou_diff_start=sou_diff_start;
		}
		for (i=0;i<c-1;i++)
		{

			for (j=i+1;j<c;j++)
			{
			    us1b=(pvt_select[i]->sou_diff_start>pvt_select[j]->sou_diff_start)||(pvt_select[i]->sou_diff_start==0);

			    if(us1b)
			    {
				    tmp=pvt_select[i];
				    pvt_select[i]=pvt_select[j];
				    pvt_select[j]=tmp;
			    }
			}
		}
		ast_verb (3, ">>> after\n");
		ast_verb (3, "--  pvt----------- pos pri diff  \n");
		for (i=0;i<c;i++)
			ast_verb (3, "++ %15s   %3d\n",PVT_ID(pvt_select[i]->pvt),pvt_select[i]->sou_diff_start );


	} else 
	{
		ast_verb (3, ">>> before\n");
		ast_verb (3, "--  pvt----------- pos pri diff  \n");
		for (i=0;i<c;i++)
			ast_verb (3, "++ %15s   %3d %2d %4d\n",PVT_ID(pvt_select[i]->pvt),pvt_select[i]->IMB_pos,PVT_STAT(pvt_select[i]->pvt,priority), pvt_select[i]->diff );

		for (i=0;i<c-1;i++)
		{
			alg=PVT_STAT(pvt_select[i]->pvt,alg[cr->limitnum]);
			usa=0;
			if (alg=='<') usa=1;
			if (alg=='a') usa=1;
			if (alg=='b') usa=1;

			if (alg=='A') usa=2;
			if (alg=='B') usa=2;
			if (alg=='v') usa=2;
			if (alg=='P') usa=2;
			if (alg=='p') usa=2;

			//PVT_STAT(pvt,limit[cr->limitnum]

			for (j=i+1;j<c;j++)
			{
			    us1b=(pvt_select[i]->IMB_pos>pvt_select[j]->IMB_pos);  // IMB po vozrastaniyu
			    us1r=(pvt_select[i]->IMB_pos==pvt_select[j]->IMB_pos);
			    // ALGORITM !!!???  esli diff v obr poradke
			    us2b=(PVT_STAT(pvt_select[i]->pvt,priority)<PVT_STAT(pvt_select[j]->pvt,priority));
			    us2r=(PVT_STAT(pvt_select[i]->pvt,priority)==PVT_STAT(pvt_select[j]->pvt,priority));

			    us3b=(pvt_select[i]->diff<pvt_select[j]->diff);		//diff po ubyvaniyu
			    if (usa==1)
			    {
				us3b=(PVT_STAT(pvt_select[i]->pvt,stat_out_calls[2]))<(PVT_STAT(pvt_select[j]->pvt,stat_out_calls[2]));		//diff po vozr
			    }
			    if (usa==2)
			    {
				//(PVT_STAT(pvt,limit[cr->limitnum])
				us3b=(PVT_STAT(pvt_select[i]->pvt,limit[cr->limitnum]))<(PVT_STAT(pvt_select[j]->pvt,limit[cr->limitnum]));		//diff po vozr limita
			    }




			    if(us1b||(us1r&&us2b)||(us1r&&us2r&&us3b))
			    {
				    tmp=pvt_select[i];
				    pvt_select[i]=pvt_select[j];
				    pvt_select[j]=tmp;
			    }
			}
		}
		ast_verb (3, ">>> after\n");
		ast_verb (3, "--  pvt----------- pos pri diff  \n");
		for (i=0;i<c;i++)
			ast_verb (3, "++ %15s   %3d %2d %4d\n",PVT_ID(pvt_select[i]->pvt),pvt_select[i]->IMB_pos,PVT_STAT(pvt_select[i]->pvt,priority), pvt_select[i]->diff );
	}


	ast_verb (3, ">>> pvt_select_sort end\n");
}




struct pvt * pvt_select_found(struct call_request * cr, struct pvt_select ** pvt_select,int pvt_select_count)
{
	int i;
	struct pvt * pvt;
	struct pvt * found=NULL;

	ast_verb (3, ">>> pvt_select_found \n");
	for(i=0;i<pvt_select_count;i++)
	{
		pvt=pvt_select[i]->pvt;
		ast_verb (3, "--  %s ",PVT_ID(pvt));
//		if(pvt_select[i]->ok)
//		{
//			ast_verb (3, "	=> ok ");
			if (can_dial(pvt, cr->opts, cr->requestor))
			{
				ast_verb (3, "		=> ok can dial ");
				if (ast_mutex_trylock_pvt(pvt)!=EBUSY) 
				{
					ast_verb (3, "			=> ok locked ");
					if (can_dial(pvt, cr->opts, cr->requestor))
					{
						pvt->selectbusy=1;
						ast_mutex_unlock_pvt (pvt);

						ast_verb (3, "				=> ok found(+can dial) SELECTED\n");
						found = pvt;
						pvt_select_stat(cr,pvt_select[i]);
						break;
					} else {
						ast_verb (3, "				=> NEok - Busy (ne uspel)\n");
					}
					ast_mutex_unlock_pvt (pvt);
				} else {
					ast_verb (3, "			-> NEok canot trylock (ne uspel) %s\n", PVT_ID(pvt));
				}
			} else {
				ast_verb (3, "		=> NEok can dial \n");
			}
//		} else {
//			ast_verb (3, "	=> NEok \n");
//		}
	}

	for(i=0;i<pvt_select_count;i++)
	{
	    ast_free(pvt_select[i]);
	}

	if(found!=NULL)
	    	ast_verb (3, "<<< pvt_select_found OK FOUND\n");
	else
	    	ast_verb (3, "<<< pvt_select_found NOT FOUND\n");

	return found;
}

/*
struct call_stat
{
    int group;
    int limitnum;
    char limittype;
    char alg;

    char numberdial[32];
    char numberb[32];
    char numbera[32];


    char napr_str[16];
    int billing_pay;
    char billing_direction[3];

    char qos_str[16];
    char cap_str[16];
    char spec_str[16];

    char pre_str[16];
    char pos_str[16];
    char pro_str[16];

    int qos;
    int cap;
    int spec;
    int pre;
    int pos;

    char souimsito[64];

    char uid[64];

    numbera
} call_stat_t;
*/

void pvt_select_stat(struct call_request *cr, struct pvt_select * pvt_select)
{
	struct pvt * pvt=pvt_select->pvt;
	int j;

	ast_verb (3, ">>> pvt_select_stat \n");

	pvt->dsp_state=-1;

	pvt->fas=cr->fas;
	pvt->epdd=cr->epdd;
	pvt->fpdd=cr->fpdd;
	pvt->hem=cr->hem;

	pvt->em_type=0;

	PVT_STAT(pvt,limitnum)=cr->limitnum;
	PVT_STAT(pvt,limittype)=cr->limittype;
	PVT_STAT(pvt,billing_pay)=cr->billing_pay;
	strcpy(PVT_STAT(pvt,billing_direction),cr->billing_direction);

	pvt->im[1]=0;
	pvt->im[0]=pvt_select->IMB_type;


	strcpy(pvt->procur,cr->pro_str);
	strcpy(pvt->capcur,cr->cap_str);



	strcpy(pvt->spec,cr->spec_str);
	strcpy(pvt->qos,cr->qos_str);
	strcpy(pvt->uid,cr->uid);

	strcpy(pvt->numbera,cr->numbera);
	strcpy(pvt->numberb,cr->numberb);

	strcpy(pvt->naprstr,cr->napr_str);


	putfilei("sim/state",pvt->imsi,"fas",pvt->fas);
	putfilei("sim/state",pvt->imsi,"epdd",pvt->epdd);
	putfilei("sim/state",pvt->imsi,"fpdd",pvt->fpdd);
	putfilei("sim/state",pvt->imsi,"hem",pvt->hem);
	putfilei("sim/state",pvt->imsi,"em_type",pvt->em_type);

	putfiles("sim/state",pvt->imsi,"im",pvt->im);
	putfiles("sim/state",pvt->imsi,"pro",pvt->procur);
	putfiles("sim/state",pvt->imsi,"cap",pvt->capcur);

	putfiles("sim/state",pvt->imsi,"numbera",pvt->numbera);
	putfiles("sim/state",pvt->imsi,"numberb",pvt->numberb);

	putfilei("sim/state",pvt->imsi,"spec",cr->spec);
	putfilei("sim/state",pvt->imsi,"pre",cr->pre);
	putfilei("sim/state",pvt->imsi,"pos",cr->pos);

	putfilei("sim/state",pvt->imsi,"qos",cr->qos);
	putfiles("sim/state",pvt->imsi,"billing_direction",cr->billing_direction);

	putfiles("sim/state",pvt->imsi,"naprstr",pvt->naprstr);

	im_count(pvt_select->IMB_type,pvt->imsi);

	//postavit flag zvonka sout na etu sim so svoim imsi

	if(*(cr->souimsito)!=0)
	{  
		    putfiles("sim/state",cr->souimsito,"sout",pvt->imsi);
	}
	if(cr->qos==5)
	{
		for (j=0;j<pvt->soupri_count;j++)
			    if (strcmp(pvt->soupri[j].imsi,cr->souimsito)==0) break;
		if(!(j<pvt->soupri_count))
		{
		    if (pvt->soupri_count<MAXDONGLEDEVICES*2)
		    {
			pvt->soupri_count++;
			strcpy(pvt->soupri[j].imsi,cr->souimsito);
		    }
		}
		pvt->soupri[j].sou_diff_start=(long)time(NULL);
	}

	ast_verb (3, "<<< pvt_select_stat \n");
}

#/* like find_device but for resource spec; return locked! pvt or NULL */
EXPORT_DEF struct pvt * find_device_by_resource_ex(struct public_state * state, const char * res, int opts, const struct ast_channel * requestor, int * exists)
{
	/*
	    Алгоритм правильного выбора модема из группы. Группа известна и четко определена, лимиты известны.
	    0. Разгребаем передаваемые параметры
	    0. 1. Получаем параметры строки набора
	    0. 2. Строим таблицу IMB

	    1. 1. Выбираем незаблокированные модемы
	    1. 2. Выбираем те, у которых лимит больше 0
	    1. 3. Выбираем те, которые проходят по всем правилам
	    1. 4. Перемешиваем
	    1. 5. Сортируем по заданному алгоритму

	    2. 1. Проверяем можно ли звонить, проверяем не заблокирован ли
	    2. 2. Блокируем и звоним
	*/

    long t1,t2,t3,t4,t5;
    long t6,t7,t8,t9,t10;


	struct call_request cr;

	struct pvt * found = NULL;
////	struct pvt_select pvt_select[MAXDONGLEDEVICES];

	struct pvt_select * pvt_select[MAXDONGLEDEVICES];



	int pvt_select_count;



	srand(time(NULL));
	call_request_create(&cr,state,res,opts,requestor);
	call_request_print(&cr);

	*exists = 1; //0?

	//??? NADO ???	

        

//	AST_RWLIST_RDLOCK(&state->devices);
	AST_RWLIST_RDLOCK(&state->devices);

	if (cr.dial_type==1)
	{
	    ast_verb (3, "GroupL dial\n");
//	t1=clock();
	    pvt_select_create(&cr, pvt_select, &pvt_select_count);
//	t2=clock();
	    pvt_select_im(&cr, pvt_select, &pvt_select_count);
//	t3=clock();
	    pvt_select_qos(&cr, pvt_select, &pvt_select_count);
//	t4=clock();
	    pvt_select_cappro(&cr, pvt_select, &pvt_select_count);
//	t5=clock();
	    pvt_select_work(&cr, pvt_select, &pvt_select_count);
//	t6=clock();
	    pvt_select_diff(&cr, pvt_select, &pvt_select_count);
//	t7=clock();
	    pvt_select_shuffle(pvt_select, pvt_select_count);
//	t8=clock();
	    pvt_select_sort(&cr, pvt_select, pvt_select_count);
//	t9=clock();
	    found=pvt_select_found(&cr, pvt_select,pvt_select_count);
//	t10=clock();
	}
	else
	{
		found=plain_dial(&cr);
	}
	AST_RWLIST_UNLOCK(&state->devices);

//	ast_verb (3, "!!! timing find_device %ld,%ld,%ld,%ld\n", t2-t1,t3-t2,t4-t3,t5-t4);
//	ast_verb (3, "!!! timing find_device %ld,%ld,%ld,%ld,%ld\n", t6-t5,t7-t6,t8-t7,t9-t8,t10-t9);
	return found;


}






























/* old
	int e;

	size_t i=0,rnum;
	size_t j=0;
	size_t c=0;
	size_t last_used;
	struct pvt * pvt;

	struct pvt * tmp;

	long diff_start;
	long diff_end;
	int u0,u1, u2, u3, u4_1,u4_2,u4_3,u4_4, u5;
	int du1,du2,du3,du4, du7,du8,du9, dupro, ducap;

	int alg;
	

	char res_copy[2096];
	char *resource;

	int uu1,uu2,uu3,uu4,uu5, uu_vip, uu_notvip, uu_other, uu_im, uu_qos, uu_inew;


	char res_tmp[256];
	char imsi[20]="";



	int IMApn;
	int IMBpn;

	int IMAp[MAXDONGLEDEVICES];
	int IMBp[MAXDONGLEDEVICES];

	int du5,du6;

	int tmpe;
	int cap;


*/

/* DEPRECEATED 
	if ((resource[0]=='N')&&(resource[1]=='I')&&(resource[2]=='M')&&(strlen(resource)>19))
	{

	    nim=1;
	    resource+=1;

	}


	if ((resource[0]=='I')&&(resource[1]=='M')&&(strlen(resource)>18))
	{

	    memcpy(imsi,resource+3,15);
	    imsi[15]=0;
	    im[0]=resource[2];
	    
	    resource+=18;

	}


	xm1=strchr(resource,':');
	if (xm1!=NULL)
	{
		xm2=strchr(xm1+1,':');
		*xm1=0;
		xm1=xm1+1;

		if (xm2!=NULL)
		{
			*xm2=0;
			xm2=xm2+1;
			ast_debug (1, "xm2=%s\n",xm2);
		}
		ast_debug (1, "xm1=%s\n",xm1);
	}
	//parsim xm1


//    ast_verb (3, "try requestor=%s \n",requestor);
*/


/* IM OLD VERSION START 
//        >> IM



	if(im[0]!='-')
	{
		ast_verb (3, "  IM ONLY %c %s\n",im[0],res);
		AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
		{
			if (!strcmp(pvt->imsi, imsi))
			{
				ast_verb (3, "  IM ONLY FOUND %c %s %s\n",im[0],res,PVT_ID(pvt));

				*exists = 1;
				if(can_dial(pvt, opts, requestor))
				{
					ast_verb (3, "  IM ONLY CANDIAL\n");

					uu1=(CONF_SHARED(pvt, group) == group);
					
					if ((resource[3] == '-')||(resource[3] == '_'))
					{
        					ast_verb (3, "  CHECK LIMIT ln = %d\n",limitnum);
						ast_verb (3, "  CHECK LIMIT lim= %d\n",(PVT_STAT(pvt,limit[limitnum])));
					}
					else
					{
        					ast_verb (3, "  CHECK LIMIT ln = %d\n",limitnum);
						ast_verb (3, "  CHECK UNLIMIT =%c\n",resource[3]);
					}
						ast_verb (3, "  CHECK GROUP group=%d uu1= %d\n",CONF_SHARED(pvt, group),uu1);
	
					
					uu1=(CONF_SHARED(pvt, group) == group);
					uu2=((PVT_STAT(pvt,limit[limitnum])>0)||((resource[3] != '-')&&(resource[3] != '_')));
					
					if(!uu1)
					{
						ast_verb (3, "  IM ONLY DIFFERENT GROUP. VERY STRANGE (MAY BE BLOCKED)!!! \n");
					}
					else if(!uu2)
					{

						ast_verb (3, "  IM ONLY LOW LIMIT\n");
					}
					else
					{
						ast_verb (3, "  CHECK LIMIT GROUP OK\n");

						if (ast_mutex_trylock_pvt(pvt)!=EBUSY) 
						{
	//						ast_mutex_lock_pvt (pvt);
							found = pvt;

							PVT_STAT(pvt,limitnum)=limitnum;
							PVT_STAT(pvt,billing_pay)=billing_pay;
							strcpy(PVT_STAT(pvt,billing_direction),billing_direction);
	
							ast_verb (3, "  - IMSELECTED by IMSI %s\n", PVT_ID(pvt));
	
							putfiles("sim/state",pvt->imsi,"im",im);
							putfilei("sim/state",pvt->imsi,"qos",qos);
							putfiles("sim/state",pvt->imsi,"billing_direction",billing_direction);

//							putfiles("sim/state",pvt->imsi,"naprstr",pvt->naprstr);

						
							if(im[0]!='A')
							{
								getfilei_def("sim/statistics", pvt->imsi,  "ima_count",&(pvt->ima_count),0);
								pvt->ima_count++;
								putfilei("sim/statistics",pvt->imsi,  "ima_count",pvt->ima_count);
							}
							if(im[0]!='B')
							{
								getfilei_def("sim/statistics", pvt->imsi,  "imb_count",&(pvt->imb_count),0);
								pvt->imb_count++;
								putfilei("sim/statistics",pvt->imsi,  "imb_count",pvt->imb_count);
							}
						    
						
							ast_verb (3, "  IM RETURN %s\n",PVT_ID(found));
							AST_RWLIST_UNLOCK(&state->devices);
							return found;
						} else ast_verb (3, "  CANNOT TRYLOCK 1 %s\n", PVT_ID(pvt));
						
						
					}
				} else {
					ast_verb (3, "  IM ONLY CANNOTDIAL\n");
				}
			} else
			{
			    //ast_verb (3, "  IM ONLY NOT FOUND\n");
			}
			
		}

		ast_verb (3, "  IM ONLY END\n");
		
		if (nim==0)
		{
			AST_RWLIST_UNLOCK(&state->devices);
				return found;
		}
		ast_verb (3, "  NIM CONTINUE\n");
		im[0]="N";
	}
	
	
//             <<IM
IM OLD VERSION END */




/* OLD DONGLE
	if (((resource[0] == 'g') || (resource[0] == 'G')) && ((resource[1] >= '0') && (resource[1] <= '9')))
	{
		errno = 0;
		group = (int) strtol (&resource[1], (char**) NULL, 10);
		if (errno != EINVAL)
		{
			AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
			{
				ast_mutex_lock_pvt (pvt);

				if (CONF_SHARED(pvt, group) == group)
				{
					*exists = 1;
					if(can_dial(pvt, opts, requestor))
					{
						found = pvt;
						break;
					}
				}
				ast_mutex_unlock_pvt (pvt);
			}
		}
	}

	else if (((resource[0] == 'a') || (resource[0] == 'A')) && ((resource[1] >= '0') && (resource[1] <= '9')))
	{ //RANDOM
		errno = 0;
		group = (int) strtol (&resource[1], (char**) NULL, 10);
		if (errno != EINVAL)
		{
			ast_mutex_lock(&state->round_robin_mtx);

			// Generate a list of all availible devices //
			c = 0;
			AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
			{
				ast_mutex_lock_pvt (pvt);
				if (CONF_SHARED(pvt, group) == group)
				{
					state->random_select[c] = pvt;
					c++;
				}
				ast_mutex_unlock_pvt (pvt);
			}

			//Peremeshaem
			for (i=0;i<c-1;i++)
			for (j=i+1;j<c;j++)
			{
			    if(random()<.5)
			    {
				tmp=state->random_select[i];
				state->random_select[i]=state->random_select[j];
				state->random_select[j]=tmp;
			    }
			}
			Search for a availible device starting at the last used device 
			for (i = 0; i < c; i++)
			{
				pvt = state->random_select[j];

				ast_mutex_lock_pvt (pvt);
				if (can_dial(pvt, opts, requestor))
				{
					found = pvt;
					break;
				}
				ast_mutex_unlock_pvt (pvt);
			}

			ast_mutex_unlock(&state->round_robin_mtx);
		}
	}

	else if (((resource[0] == 'r') || (resource[0] == 'R')) && ((resource[1] >= '0') && (resource[1] <= '9')))
	{ //Round Robbin
		errno = 0;
		group = (int) strtol (&resource[1], (char**) NULL, 10);
		if (errno != EINVAL)
		{
			ast_mutex_lock(&state->round_robin_mtx);

			// Generate a list of all availible devices //
			j = ITEMS_OF (state->round_robin);
			c = 0; last_used = 0;
			AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
			{
				ast_mutex_lock_pvt (pvt);
				if (CONF_SHARED(pvt, group) == group)
				{
					state->round_robin[c] = pvt;
					if (pvt->group_last_used == 1)
					{
						pvt->group_last_used = 0;
						last_used = c;
					}

					c++;

					if (c == j)
					{
						ast_mutex_unlock_pvt (pvt);
						break;
					}
				}
				ast_mutex_unlock_pvt (pvt);
			}

			// Search for a availible device starting at the last used device //
			for (i = 0, j = last_used + 1; i < c; i++, j++)
			{
				if (j == c)
				{
					j = 0;
				}

				pvt = state->round_robin[j];
				*exists = 1;

				ast_mutex_lock_pvt (pvt);
				if (can_dial(pvt, opts, requestor))
				{
					pvt->group_last_used = 1;
					found = pvt;
					break;
				}
				ast_mutex_unlock_pvt (pvt);
			}

			ast_mutex_unlock(&state->round_robin_mtx);
		}
	}
	else if (((resource[0] == 'p') || (resource[0] == 'P')) && resource[1] == ':')
	{
		ast_mutex_lock(&state->round_robin_mtx);

		// Generate a list of all availible devices //
		j = ITEMS_OF(state->round_robin);
		c = 0; last_used = 0;
		AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
		{
			ast_mutex_lock_pvt (pvt);
			if (!strcmp (pvt->provider_name, &resource[2]))
			{
				state->round_robin[c] = pvt;
				if (pvt->prov_last_used == 1)
				{
					pvt->prov_last_used = 0;
					last_used = c;
				}

				c++;

				if (c == j)
				{
					ast_mutex_unlock_pvt (pvt);
					break;
				}
			}
			ast_mutex_unlock_pvt (pvt);
		}

		// Search for a availible device starting at the last used device //
		for (i = 0, j = last_used + 1; i < c; i++, j++)
		{
			if (j == c)
			{
				j = 0;
			}

			pvt = state->round_robin[j];
			*exists = 1;

			ast_mutex_lock_pvt(pvt);
			if (can_dial(pvt, opts, requestor))
			{
				pvt->prov_last_used = 1;
				found = pvt;
				break;
			}
			ast_mutex_unlock_pvt (pvt);
		}

		ast_mutex_unlock(&state->round_robin_mtx);
	}
	else if (((resource[0] == 's') || (resource[0] == 'S')) && resource[1] == ':')
	{
		ast_mutex_lock(&state->round_robin_mtx);

		// Generate a list of all availible devices //
		j = ITEMS_OF(state->round_robin);
		c = 0; last_used = 0;
		i = strlen (&resource[2]);

		AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
		{
			ast_mutex_lock_pvt (pvt);
			if (!strncmp (pvt->imsi, &resource[2], i))
			{
				state->round_robin[c] = pvt;
				if (pvt->sim_last_used == 1)
				{
					pvt->sim_last_used = 0;
					last_used = c;
				}

				c++;

				if (c == j)
				{
					ast_mutex_unlock_pvt (pvt);
					break;
				}
			}
			ast_mutex_unlock_pvt (pvt);
		}

		/ Search for a availible device starting at the last used device //
		for (i = 0, j = last_used + 1; i < c; i++, j++)
		{
			if (j == c)
			{
				j = 0;
			}

			pvt = state->round_robin[j];
			*exists = 1;

			ast_mutex_lock_pvt (pvt);
			if (can_dial(pvt, opts, requestor))
			{
				pvt->sim_last_used = 1;
				found = pvt;
				break;
			}
			ast_mutex_unlock_pvt (pvt);
		}

		ast_mutex_unlock(&state->round_robin_mtx);
	}
	else if (((resource[0] == 'i') || (resource[0] == 'I')) && resource[1] == ':')
	{
		AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
		{
			ast_mutex_lock_pvt (pvt);
			if (!strcmp(pvt->imei, &resource[2]))
			{
				*exists = 1;
				if(can_dial(pvt, opts, requestor))
				{
					found = pvt;
					break;
				}
			}
			ast_mutex_unlock_pvt (pvt);
		}
	}


*/



/*
			    alg=PVT_STAT(limit_select[j],alg[limitnum]);
//			    ast_verb (3, "check %s %d %d alg=%c\n",PVT_ID(limit_select[j]), i,j,alg);

			    u0=((qos!=0)&&(qos!=4));
			    u1=(PVT_STAT(limit_select[i],priority)<PVT_STAT(limit_select[j],priority));
			    u2=(PVT_STAT(limit_select[i],priority)==PVT_STAT(limit_select[j],priority));
			    u3=((alg == '>')&&(PVT_STAT(limit_select[i],limit[limitnum])<PVT_STAT(limit_select[j],limit[limitnum]))) ||						 
				((alg == 'D')&&(PVT_STAT(limit_select[i],stat_call_end)>PVT_STAT(limit_select[j],stat_call_end))) ||						 
				((alg == 'd')&&(PVT_STAT(limit_select[i],limit[limitnum])<PVT_STAT(limit_select[j],limit[limitnum]))) ||						 
				((alg == 'Q')&&(PVT_STAT(limit_select[i],limit[limitnum])<PVT_STAT(limit_select[j],limit[limitnum]))) ||						 
			        ((alg == '<')&&(PVT_STAT(limit_select[i],limit[limitnum])>PVT_STAT(limit_select[j],limit[limitnum]))) ||						
			        ((alg == '^')&&(PVT_STAT(limit_select[i],stat_calls_duration[2])<PVT_STAT(limit_select[j],stat_calls_duration[2]))) ||
			        ((alg == '_')&&(PVT_STAT(limit_select[i],stat_calls_duration[2])>PVT_STAT(limit_select[j],stat_calls_duration[2])));
;						
			    u4_1=(IMBp[i]>IMBp[j]);
			    u4_2=(IMBp[i]==IMBp[j]);

			    u4_3=(IMAp[i]>IMAp[j]);
			    u4_4=(IMAp[i]==IMAp[j]);


			    
//			    ast_verb (3, "u1 %d %d %d %d %d %d \n", u1,u2,u3,u4,(resource[2] == '*'),(((rand())%2)==0));
			    u5=((u1&&u0) || (u2 && u3)); //vsya bayda
			    
			    if (u4_1 || (u4_2 && (u4_3 || (u4_4 && u5))))
			    //if ((u1&&u0) || (u2 && u3))
			    {
			    	//ast_verb (3, "Swap %d %d\n", i,j);
				tmp=limit_select[i];
				limit_select[i]=limit_select[j];
				limit_select[j]=tmp;

			    tmpe=IMBp[i];
			    IMBp[i]=IMBp[j];
			    IMBp[j]=tmpe;

			    tmpe=IMAp[i];
			    IMAp[i]=IMAp[j];
			    IMAp[j]=tmpe;

			    }
			}

*/


/*

void pvt_select_ok(struct call_request * cr, struct pvt ** pvt_select, , int pvt_select_count)
{

			ast_debug (3, ">>> pvt_select_ok \n");
			for (i = 0; i < pvt_select_count; i++)
			{


				pvt = limit_select[i];



				if((du1||du2||du4||du6||du7||du8||du9)&&du5&&dupro&&ducap)
					limit_select_ok[i]=0;
				else
					limit_select_ok[i]=1;

				ast_verb (3, "  %s, diff-nos-du1=%d diff-sout-du2=%d diff-goo-du3=%d nodiff-du4=%d diff-imbc-du6=%d can-im-du5=%d im=%c\n",PVT_ID(pvt),du1,du2,du3,du4,du6,du5,im[0]);
				ast_verb (3, "      du7-vip=%d du8-goo=%d du9-nor=%d spec=%d vip=%d \n",du7,du8,du9,spec,vip);
				ast_verb (3, "      dupro=%d tmppro=%s pro=%s \n", dupro, tmppro, pvt->pro );
				ast_verb (3, "      ducap=%d tmpcap=%s cap=%d capnew=%d capfail=%d capok=%d \n", ducap, tmpcap, cap, PVT_STAT(pvt,capnew), PVT_STAT(pvt,capfail),  PVT_STAT(pvt,capok));
				ast_verb (3, "      diff=%d dimff_min=%d nodiff=%d \n", (int)diff_end, (int)(PVT_STAT(pvt,diff_min)),PVT_STAT(pvt,nodiff[limitnum]));
				ast_verb (3, "      imn=%d ima=%d imb=%d imc=%d imd=%d ime=%d \n", PVT_STAT(pvt,imn) , PVT_STAT(pvt,ima) , PVT_STAT(pvt,imb) , PVT_STAT(pvt,imc) , PVT_STAT(pvt,imd) , PVT_STAT(pvt,ime) );
				ast_verb (3, "      => limit_select_ok=%d", limit_select_ok[i]);

}
*/
