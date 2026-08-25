/*
   Copyright (C) 2010,2011 bg <bg_one@mail.ru>
*/
#ifdef HAVE_CONFIG_H
#include <svistok_config.h>
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <fcntl.h>

#include <asterisk.h>
#include <asterisk/utils.h>

#include "cpvt.h"
#include "chan_dongle.h"			/* struct pvt */
#include <asterisk-chan-dongle/at_queue.h>	/* struct at_queue_task */
#include <asterisk-chan-dongle/mutils.h>	/* ITEMS_OF() */

#/* return 0 on success */
// TODO: move to activation time, save resouces
static int init_pipe(int filedes[2])
{
	int x;
	int rv;
	int flags;

	rv = pipe(filedes);
	if(rv == 0) {
		for(x = 0; x < 2; ++x) {
			rv = fcntl(filedes[x], F_GETFL);
			flags = fcntl(filedes[x], F_GETFD);
			if(rv == -1 || flags == -1 || (rv = fcntl(filedes[x], F_SETFL, O_NONBLOCK | rv)) == -1 || (rv = fcntl(filedes[x], F_SETFD, flags | FD_CLOEXEC)) == -1)
				goto bad;
			}
		return 0;
bad:
		close(filedes[0]);
		close(filedes[1]);
	}
	return rv;
}

#/* */
                                                                                                     
 
                
                           

                            
  
                                        
          
   
                   
                             
                       
                   
                                 
                                 

                                                                                

                                                  
                        
                                   
                             
                                             


                                                                                                                             
               
   
                    
                    
  

             
 

#/* */
                                            
 
                         
                     
                             

                         
                         

                                                                                                                                                                                                             
                                                          
                   
   
                                  
                                             
                              
         
   
  
                            

                              
                                                 
                        
   
                               
   
  
                                    
                                  
                               

                        
                                  
                       
   

                
 

#/* */
                                                                      
 
                    
                                              
                                
               
  

          
 

#/* */
                                                            
 
                               
           
             
             
        
   

               
                    
                                              
                                    
                
      
                
  

                    
 
