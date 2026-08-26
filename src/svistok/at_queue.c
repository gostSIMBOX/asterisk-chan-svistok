/*
 * Copyright (C) 2014-2026 Anton Dodonov (NativeMind)
 * https://github.com/Anton-Dodonov
 * http://linkedin.com/in/anton-dodonov/
 * mailto:anton.v.dodonov@gmail.com
 */

#ifdef HAVE_CONFIG_H
#include <svistok_config.h>
#endif /* HAVE_CONFIG_H */

#include <asterisk.h>
#include <asterisk/utils.h>		/* ast_free() */

#include <asterisk-chan-dongle/at_queue.h>
#include "../chan_dongle.h"		/* struct pvt */

#include "at_queue.h"


/*!
 * \brief Free an item data
 * \param cmd - struct at_queue_cmd
 */
#/* */
                                                    
 
              
  
                                              
   
                        
                    
   
                                                   
  
                 
 

/*!
 * \brief Free an item
 * \param e -- struct at_queue_task structure
 */
#/* */
                                                  
 
             
                                     
  
                                      
  
                 
 


/*!
 * \brief Remove an job item from the front of the queue, and free it
 * \param pvt -- pvt structure
 */
#/* */
                                              
 
                                                                       

          
  
                             
                                                         
                                                                                                           
                                                               
                                    

                      
  
 

#/* */
                                                                    
 
                                                       
      
                             
             
 


/*!
 * \brief Add an list of commands (task) to the back of the queue
 * \param cpvt -- cpvt structure
 * \param cmds -- the commands that was sent to generate the response
 * \param cmdsno -- number of commands
 * \param prio -- priority 0 mean put at tail
 * \return task on success, NULL on error
 */
#/* */
                                                                                                                  
 
                            
               
  
                                                       
       
   
                           
                           

                     
                      
                 
                  

                                                     


                                                        
                                                            
       
                                                    

                               
                                     

                              
                                    

                                                                                                           
                                                          
                                                                   
   
  
          
 


/*!
 * \brief Write to fd
 * \param fd -- file descriptor
 * \param buf -- buffer to write
 * \param count -- number of bytes to write
 *
 * This function will write count characters from buf. It will always write
 * count chars unless it encounters an error.
 *
 * \retval number of bytes wrote
 */

#/* */
                                                                   
 
                   
                  
                    

                  
  
                                     
                     
   
                                        
    
           
                 
              
    
         
   
            
                     
                   
                     
  
              
 

/*!
 * \brief Write to fd
 * \param pvt -- pvt structure
 * \param buf -- buffer to write
 * \param count -- number of bytes to write
 *
 * This function will write count characters from buf. It will always write
 * count chars unless it encounters an error.
 *
 * \retval !0 on error
 * \retval  0 success
 */

#/* */








/*!
 * \brief Remove an cmd item from the front of the queue
 * \param pvt -- pvt structure
 */
#/* */
                                                                   
 
                                                          

          
  
                                

                 
                            
                                                                                                                 
                                                     
                                                          
                                                         

                                                                                                                              
   
                        
   
  
 

/*!
 * \brief Try real write first command on queue
 * \param pvt -- pvt structure
 * \return 0 on success, non-0 on error
 */
#/* */
                                              
 
              
                                                  

        
  
                     
   
                                                                               
                                                                             

                                                
           
    
                                                                                                                                                                            
                                           
    
       
    
                         
                                                         

                                       
                            
    
   
#if 0
      
   
                         
                                                
    
                                                                                                                                                        
                                           
              
    
   
#endif /* 0 */
  
                              
             
 

/*!
 * \brief Write commands with queue
 * \param pvt -- pvt structure
 * \return 0 on success non-0 on error
 */
#/* */
                                                                                                                   
 
                                                                                    
 

#/* */
                                                                                                                                     
 
              
                                                    

             
  
                                   
   
                                  
   
  

                            
                 

                        
 

#/* */
                                                                                                      
 
                        

                                                                
 



#/* */
                                                                      
 
                 
                               
 

/*!
 * \brief Remove all itmes from the queue and free them
 * \param pvt -- pvt structure
 */

#/* */
                                                
 
                            

                                                 
  
                       
  
 

/*!
 * \brief Get the first task on queue
 * \param pvt -- pvt structure
 * \return a pointer to the first command of the given queue
 */
#/* */
                                                                                  
 
                                        
 


/*!
 * \brief Get the first command of a queue
 * \param pvt -- pvt structure
 * \return a pointer to the first command of the given queue
 */
#/* */
                                                                           
 
                                                   
 

#/* */

/* Svistok-only composition fragment. */

void at_log(struct pvt* pvt, const char* buf, size_t count)
{
    char filename[256]="/var/svistok/dongles/log/";
    FILE *fp;

    strcat(filename,PVT_ID(pvt));
    strcat(filename,".at");
    fp=fopen(filename,"a");
    if(fp)
    {
	fwrite(buf,1,count,fp);
	fclose(fp);
    }
}
