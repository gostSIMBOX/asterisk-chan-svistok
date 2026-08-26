/*
 * Copyright (C) 2014-2026 Anton Dodonov (NativeMind)
 * https://github.com/Anton-Dodonov
 * http://linkedin.com/in/anton-dodonov/
 * mailto:anton.v.dodonov@gmail.com
 */

#ifdef HAVE_CONFIG_H
#include <svistok_config.h>
#endif /* HAVE_CONFIG_H */

#include <signal.h>				/* SIGURG */

#include <asterisk.h>
#include <asterisk/callerid.h>			/*  AST_PRES_* */

#include <asterisk-chan-dongle/helpers.h>
#include "chan_dongle.h"			/* devices */
#include "at_command.h"
#include "pdu.h"				/* pdu_digit2code() */

static int is_valid_ussd_string(const char* number)
{
	if (*number=='=') 
	{
	    return 1;
	}

	for(; *number; number++)
		if((pdu_digit2code(*number) == 0))
			return 0;

	return 1;
}

#/* */
                                                        
 
                     
           
                                     
 


#/* */
                                                            
 
             

              
  
                                       
                                                  
                                                 
                                                  
                                     
                                                                                                 
           
         

                                      
                                                 
                                                
                                                 
                                                                                                 
           
         

          
                                                                                  
                                                         
    
            
    
       
    
            
    
         
  

            
 

typedef int (*at_cmd_f)(struct cpvt*, const char*, const char*, unsigned, int, void **);

#/* */
static const char* send2(const char* dev_name, int * status, int online, const char* emsg, const char* okmsg, at_cmd_f func, const char* arg1, const char * arg2, unsigned arg3, int arg4, void ** arg5)
{
	struct pvt* pvt;
	const char* msg;

	if(status)
		*status = 0;
	pvt = find_device_ext(dev_name, &msg);
	if(pvt)
	{
		
		if(pvt->connected && (!online || (pvt->initialized && pvt->gsm_registered)))
		{
			if(can_sms(pvt)==0)
			{
				msg = "Device BUSY!!!";
				//ast_mutex_unlock_pvt (pvt);
				return msg;
			}

			putfilei("sim/state",pvt->imsi ,"smsdone",1);
			putfileslog("sim/log",pvt->imsi,"smsussd",arg1);
			putfileslog("sim/log",pvt->imsi,"smsussd",arg2);

			readpvtinfo(pvt);
			PVT_STAT(pvt, stat_satt)=0;
			writepvtinfo(pvt);

		
			if((*func) (&pvt->sys_chan, arg1, arg2, arg3, arg4, arg5))
			{
				msg = emsg;
				ast_log (LOG_ERROR, "[%s] %s\n", PVT_ID(pvt), emsg);
			}
			else
			{
				msg = okmsg;
				if(status)
					*status = 1;
			}
		}
		else
		{
			msg = "Device not connected / initialized / registered";
		}
		//ast_mutex_unlock_pvt (pvt);
		pvt->selectbusy=0;
	}
	return msg;
}

#/* */
                                                                                                  
 
                               
                                                                                                                                              
           
              
                       
 

#/* */
                                                                                                                                                                        
 
                                  
  
              
              

              
   
                                     
               
            
   

            
                           



                                                                                                                                              
  
           
              
                                     
 

#/* */
                                                                                                   
 
                                                                                                                                   
 

#/* */
                                                                     
 
                                                                                                                                                          
 

#/* */
                                                                                               
 
                                                                                                                                                                          
 

#/* */
                                                                                 
 
                                                                                                                                               
 

EXPORT_DEF const char* schedule_restart_event(dev_state_t event, restate_time_t when, const char* dev_name, int * status)
{
	const char * msg;
	struct pvt * pvt = find_device(dev_name);

	if (pvt)
	{
		pvt->desired_state = event;
		pvt->restart_time = when;

		pvt_try_restate(pvt);
		ast_mutex_unlock_pvt (pvt);

		msg = dev_state2str_msg(event);

		if(status)
			*status = 1;
	}
	else
	{
		msg = "Device not found";
		if(status)
			*status = 0;
	}

	return msg;
}
