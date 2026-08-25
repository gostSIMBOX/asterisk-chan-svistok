#ifdef HAVE_CONFIG_H
#include <svistok_config.h>
#endif /* HAVE_CONFIG_H */

#ifdef BUILD_APPLICATIONS
/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>

   bg <bg_one@mail.ru>
*/

#include <asterisk.h>
#include <asterisk/app.h>	/* AST_DECLARE_APP_ARGS() ... */
#include <asterisk/pbx.h>	/* pbx_builtin_setvar_helper() */
#include <asterisk/module.h>	/* ast_register_application2() ast_unregister_application() */
#include <asterisk/ast_version.h>	/* ASTERISK_VERSION_NUM */

#include "app.h"		/* app_register() app_unregister() */
#include "chan_dongle.h"	/* struct pvt */
#include <asterisk-chan-dongle/helpers.h>	/* send_sms() ITEMS_OF() */
//include "share.c"

struct ast_channel;

static int app_status_exec (struct ast_channel* channel, const char* data)
{
	struct pvt * pvt;
	char * parse;
	int stat;
	char status[2];
	int exists = 0;

	AST_DECLARE_APP_ARGS (args,
		AST_APP_ARG (resource);
		AST_APP_ARG (variable);
	);

	if (ast_strlen_zero (data))
	{
		return -1;
	}

	parse = ast_strdupa (data);

	AST_STANDARD_APP_ARGS (args, parse);

	if (ast_strlen_zero (args.resource) || ast_strlen_zero (args.variable))
	{
		return -1;
	}

	/* TODO: including options number */
	pvt = find_device_by_resource(args.resource, 0, NULL, &exists);
	if(pvt)
	{
		/* ready for outgoing call */
		pvt->selectbusy=0;
		//ast_mutex_unlock (&pvt->lock);
		stat = 2;
	}
	else
	{
		stat = exists ? 3 : 1;
	}

	snprintf (status, sizeof (status), "%d", stat);
	pbx_builtin_setvar_helper (channel, args.variable, status);

	return 0;
}

                                                                                             
 
             
                 
            
              

                            
                       
                       
                        
                         
                       
   

                            
  
            
  

                            

                                     

                                   
  
                                                                           
            
  

                                   
  
                                                                                
            
  

                                                                                                     
            
                                                                       
                
 



                                      
 
                  

                                                             
                      
                  
          
 
  
                 
                  
                                    
                                     
                                                
                                                           
                                                                              
   
  
                  
                    
                                                        
                                                        
                                                
                              
                                      
                                             
                                                  
  
 ;

#if ASTERISK_VERSION_NUM >= 10800
typedef int		(*app_func_t)(struct ast_channel* channel, const char * data);
#else
typedef int		(*app_func_t)(struct ast_channel* channel, void * data);
#endif

#/* */
                              
 
            
                                   
  
                                                                                                                  
  
 

#/* */
                                
 
       
                                      
  
                                          
  
 

#endif /* BUILD_APPLICATIONS */
