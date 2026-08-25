#ifdef HAVE_CONFIG_H
#include <svistok_config.h>
#endif /* HAVE_CONFIG_H */

#ifdef BUILD_MANAGER /* no manager, no copyright */
/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>

   bg <bg_one@mail.ru>
*/

#include <asterisk.h>
#include <asterisk/stringfields.h>		/* AST_DECLARE_STRING_FIELDS for asterisk/manager.h */
#include <asterisk/manager.h>			/* struct mansession, struct message ... */
#include <asterisk/strings.h>			/* ast_strlen_zero() */
#include <asterisk/callerid.h>			/* ast_describe_caller_presentation */

#include <asterisk-chan-dongle/manager.h>
#include "chan_dongle.h"			/* devices */
#include <asterisk-chan-dongle/helpers.h>	/* ITEMS_OF() send_ccwa_set() send_reset() send_sms() send_ussd() */

                                                ;

static int manager_show_devices (struct mansession* s, const struct message* m)
{
	const char * id = astman_get_header (m, "ActionID");
	const char * device = astman_get_header (m, "Device");
	struct pvt * pvt;
	size_t count = 0;
	char buf[40];

	astman_send_listack (s, m, "Device status list will follow", "start");

	AST_RWLIST_RDLOCK (&gpublic->devices);
	AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
	{
		ast_mutex_lock (&pvt->lock);
		if(ast_strlen_zero(device) || strcmp(device, PVT_ID(pvt)) == 0)
		{
			astman_append (s, "Event: DongleDeviceEntry\r\n");
			if(!ast_strlen_zero (id))
				astman_append (s, "ActionID: %s\r\n", id);
			astman_append (s, "Device: %s\r\n", PVT_ID(pvt));
/* settings */
			astman_append (s, "AudioSetting: %s\r\n", CONF_UNIQ(pvt, audio_tty));
			astman_append (s, "DataSetting: %s\r\n", CONF_UNIQ(pvt, data_tty));
			astman_append (s, "IMEISetting: %s\r\n", CONF_UNIQ(pvt, imei));
			astman_append (s, "IMSISetting: %s\r\n", CONF_UNIQ(pvt, imsi));
			astman_append (s, "ChannelLanguage: %s\r\n", CONF_SHARED(pvt, language));
			astman_append (s, "Context: %s\r\n", CONF_SHARED(pvt, context));
			astman_append (s, "Exten: %s\r\n", CONF_SHARED(pvt, exten));
			astman_append (s, "Group: %d\r\n", CONF_SHARED(pvt, group));
			astman_append (s, "RXGain: %d\r\n", CONF_SHARED(pvt, rxgain));
			astman_append (s, "TXGain: %d\r\n", CONF_SHARED(pvt, txgain));
			astman_append (s, "U2DIAG: %d\r\n", CONF_SHARED(pvt, u2diag));
			astman_append (s, "UseCallingPres: %s\r\n", CONF_SHARED(pvt, usecallingpres) ? "Yes" : "No");
			astman_append (s, "DefaultCallingPres: %s\r\n", CONF_SHARED(pvt, callingpres) < 0 ? "<Not set>" : ast_describe_caller_presentation (CONF_SHARED(pvt, callingpres)));
			astman_append (s, "AutoDeleteSMS: %s\r\n", CONF_SHARED(pvt, autodeletesms) ? "Yes" : "No");
			astman_append (s, "DisableSMS: %s\r\n", CONF_SHARED(pvt, disablesms) ? "Yes" : "No");
			astman_append (s, "ResetDongle: %s\r\n", CONF_SHARED(pvt, resetdongle) ? "Yes" : "No");
			astman_append (s, "SMSPDU: %s\r\n", CONF_SHARED(pvt, smsaspdu) ? "Yes" : "No");
			astman_append (s, "CallWaitingSetting: %s\r\n", dc_cw_setting2str(CONF_SHARED(pvt, callwaiting)));
			astman_append (s, "DTMF: %s\r\n", dc_dtmf_setting2str(CONF_SHARED(pvt, dtmf)));
			astman_append (s, "MinimalDTMFGap: %d\r\n", CONF_SHARED(pvt, mindtmfgap));
			astman_append (s, "MinimalDTMFDuration: %d\r\n", CONF_SHARED(pvt, mindtmfduration));
			astman_append (s, "MinimalDTMFInterval: %d\r\n", CONF_SHARED(pvt, mindtmfinterval));
			astman_append (s, "S: %s\r\n", CONF_UNIQ(pvt, serial));
/* state */
			astman_append (s, "State: %s\r\n", pvt_str_state(pvt));
			astman_append (s, "AudioState: %s\r\n", PVT_STATE(pvt, audio_tty));
			astman_append (s, "DataState: %s\r\n", PVT_STATE(pvt, data_tty));
			astman_append (s, "Voice: %s\r\n", (pvt->has_voice) ? "Yes" : "No");
			astman_append (s, "SMS: %s\r\n", (pvt->has_sms) ? "Yes" : "No");
			astman_append (s, "Manufacturer: %s\r\n", pvt->manufacturer);
			astman_append (s, "Model: %s\r\n", pvt->model);
			astman_append (s, "Firmware: %s\r\n", pvt->firmware);
			astman_append (s, "IMEIState: %s\r\n", pvt->imei);
			astman_append (s, "IMSIState: %s\r\n", pvt->imsi);
			astman_append (s, "GSMRegistrationStatus: %s\r\n", GSM_regstate2str(pvt->gsm_reg_status));
			astman_append (s, "RSSI: %d, %s\r\n", pvt->rssi, rssi2dBm(pvt->rssi, buf, sizeof(buf)));
			astman_append (s, "Mode: %s\r\n", sys_mode2str(pvt->linkmode));
			astman_append (s, "Submode: %s\r\n", sys_submode2str(pvt->linksubmode));
			astman_append (s, "ProviderName: %s\r\n", pvt->provider_name);
			astman_append (s, "LocationAreaCode: %s\r\n", pvt->location_area_code);
			astman_append (s, "CellID: %s\r\n", pvt->cell_id);
			astman_append (s, "SubscriberNumber: %s\r\n", pvt->subscriber_number);
			astman_append (s, "SMSServiceCenter: %s\r\n", pvt->sms_scenter);
			astman_append (s, "UseUCS2Encoding: %s\r\n", pvt->use_ucs2_encoding ? "Yes" : "No");
			astman_append (s, "USSDUse7BitEncoding: %s\r\n", pvt->cusd_use_7bit_encoding ? "Yes" : "No");
			astman_append (s, "USSDUseUCS2Decoding: %s\r\n", pvt->cusd_use_ucs2_decoding ? "Yes" : "No");
			astman_append (s, "TasksInQueue: %u\r\n", PVT_STATE(pvt, at_tasks));
			astman_append (s, "CommandsInQueue: %u\r\n", PVT_STATE(pvt, at_cmds));
			astman_append (s, "CallWaitingState: %s\r\n", pvt->has_call_waiting ? "Enabled" : "Disabled");
			astman_append (s, "CurrentDeviceState: %s\r\n", dev_state2str(pvt->current_state));
			astman_append (s, "DesiredDeviceState: %s\r\n", dev_state2str(pvt->desired_state));
			astman_append (s, "CallsChannels: %u\r\n", PVT_STATE(pvt, chansno));
			astman_append (s, "Active: %u\r\n", PVT_STATE(pvt, chan_count[CALL_STATE_ACTIVE]));
			astman_append (s, "Held: %u\r\n", PVT_STATE(pvt, chan_count[CALL_STATE_ONHOLD]));
			astman_append (s, "Dialing: %u\r\n", PVT_STATE(pvt, chan_count[CALL_STATE_DIALING]));
			astman_append (s, "Alerting: %u\r\n", PVT_STATE(pvt, chan_count[CALL_STATE_ALERTING]));
			astman_append (s, "Incoming: %u\r\n", PVT_STATE(pvt, chan_count[CALL_STATE_INCOMING]));
			astman_append (s, "Waiting: %u\r\n", PVT_STATE(pvt, chan_count[CALL_STATE_WAITING]));
			astman_append (s, "Releasing: %u\r\n", PVT_STATE(pvt, chan_count[CALL_STATE_RELEASED]));
			astman_append (s, "Initializing: %u\r\n", PVT_STATE(pvt, chan_count[CALL_STATE_INIT]));
/* TODO: stats */

			astman_append (s, "\r\n");
			count++;
		}
		ast_mutex_unlock (&pvt->lock);
	}
	AST_RWLIST_UNLOCK (&gpublic->devices);

	astman_append (s, "Event: DongleShowDevicesComplete\r\n");
	if(!ast_strlen_zero (id))
		astman_append (s, "ActionID: %s\r\n", id);
	astman_append (s, 
		"EventList: Complete\r\n"
		"ListItems: %zu\r\n"
		"\r\n",
		count
	);

	return 0;
}

                                                                            
 
                                                      
                                                  

                
                 
             
                     

                              
  
                                                   
           
  

                            
  
                                                 
           
  

                                                
                                                                      
           
  
                             
  
     
  
                               
  

          
 

                                                                           
 
                                                      
                                                      
                                                        
                                                         
                                                      

                
                 
             
              
 
                              
  
                                                   
           
  

                              
  
                                                   
           
  

                               
  
                                                    
           
  

                                                                            
                                                                       
           
  
                             
  
     
  
                               
  

          
 

                                                                           
 
                                                      
                                                

                
                 
             
              
 
                              
  
                                                   
           
  

                           
  
                                                
           
  

                                              
                                                                       
           
  
                             
  
     
  
                               
  

          
 

#/* */
                                                                                                                        
 
              
                                                    

                                     
                  
              
                   
          
     
        
   
 

/*!
 * \brief Send a DongleNewUSSD event to the manager
 * This function splits the message in multiple lines, so multi-line
 * USSD messages can be send over the manager API.
 * \param pvt a pvt structure
 * \param message a null terminated buffer containing the message
 */

                                                                            
 
                     
                    
           
                       

                            

                                   
  
                  
   
                                                                     
               
   
  

                                                 
                  
                      
                                 
             
       
                                          
   

                
 


#/* */
                                                                                                     
 
                                           
              
                                                     
                    
  
 

#/* */
                                                                                                         
 
                                       
                  
                    
          
         
   
 

#/* */
                                                
 
                
          
                                    
                                        
       
  
                             
              
                                
                        
                        
                       
    
                             
                        
                       
           
                           
     
   
                 
  
 
                
 

#/* */
                                                                                                                    
 
                                              
                  
                   
                    
                     
                    
          
             
           
             
          
    
 

#/* */
                                                                                                            
 
                                                        
                  
                   
                     
          
             
          
    
 

#/* */
                                                                                        
 
                                               
                  
                   
          
          
    
 


/*!
 * \brief Send a DongleNewSMS event to the manager
 * This function splits the message in multiple lines, so multi-line
 * SMS messages can be send over the manager API.
 * \param pvt a pvt structure
 * \param number a null terminated buffer containing the from number
 * \param message a null terminated buffer containing the message
 */

// TODO: use espace_newlines() and join with manager_event_new_sms_base64()
                                                                                         
 
                     
                      
                   
          

                            

                                   
  
                  
   
                                                                     
               
   
  

                                                
                  
                
                      
           
                                                  
   
                
 

/*!
 * \brief Send a DongleNewSMSBase64 event to the manager
 * \param pvt a pvt structure
 * \param number a null terminated buffer containing the from number
 * \param message_base64 a null terminated buffer containing the base64 encoded message
 */

                                                                                                         
 
                                                      
                  
                
                    
                                 
   
 

                                                                           
 
                                                      
                                                    
                                                      

                
                 
             
                       

                              
  
                                                   
           
  

                                  
                                
                                        
                                   
     
  
                                            
           
  

                                              
                                                      
                                                           

                           
                                              

          
 

                                                                        
 
                                                      
                                                      

                
                 
             

                              
  
                                                   
           
  

                                   
                                                      
                                                           

                           
                                              

          
 

                                                                                  ;
#/* */
                                                                                                     
 

                                                       
                                                   
                                                       

               
                  
            
            

                              
  
                                                   
           
  

                                         
  
                                                                       
   
                                                           
                                                        
                                                             
                             
                                                
            
   
  

                                                   
                           
                                              
          
 

#/* */
                                                                           
 
                                                          
 

#/* */
                                                                        
 
                                                        
 

#/* */
                                                                         
 
                                                        
 

#/* */
                                                                          
 
                                                        
 

#/* */
                                                                          
 
                                                   
                                                       

            

                                         
  
                                         
   
                 
                                             
                             
                                                
            
   
  

                                                   
                           
                                              
          
 

extern const struct dongle_manager
{
	int		(*func)(struct mansession* s, const struct message* m);
	int		authority;
	const char*	name;
	const char*	brief;
	const char*	desc;
} svistok_bridge_upstream_dcm[11];
    
                                          
                      
                        
                                                                                       
                               
               
                                                                       
                                               
   
  
                    
                 
                   
                                      
                                                    
                                                  
                                                                       
                                                                        
                                                                     
    
  
                   
                 
                  
                        
                                                     
                                                  
                                                                       
                                                             
                                                                        
                                                            
   
  
                   
                 
                  
                           
                                                        
                                                  
                                                                       
                                                             
                                      
   
  
                   
                   
                  
                                             
                                                             
                                                  
                                                                       
                                                                 
                                                      
    
  
               
                                       
                
                    
                                   
                                                  
                                                                       
                                                          
   
  
                 
                                       
                  
                      
                                     
                                                  
                                                                       
                                                            
                                                                                  
   
  
              
                                       
               
                   
                                  
                                                  
                                                                       
                                                            
                                                                                
   
  
               
                                       
                
                    
                                   
                                                  
                                                                       
                                                            
   
  
                
                                       
                 
                     
                                    
                                                  
                                                                       
                                                            
                                                                                
   
  
                
                                       
                 
                                   
                                                    
                                                  
                                                                       
                                                                                      
   
 ;

EXPORT_DEF void manager_register()
{
	unsigned i;
	struct ast_module* module = self_module();

	for(i = 0; i < ITEMS_OF(dcm); i++)
	{
		ast_manager_register2 (dcm[i].name, dcm[i].authority, dcm[i].func, module, dcm[i].brief, dcm[i].desc);
	}
}

                                    
 
       
                                      
  
                                             
  
 

#endif /* BUILD_MANAGER */
