/* 
   Copyright (C) 2009 - 2010

   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org

   Dmitry Vagin <dmitry2004@yandex.ru>

   Copyright (C) 2010 - 2011
   bg <bg_one@mail.ru>
*/
#ifdef HAVE_CONFIG_H
#include <svistok_config.h>
#endif /* HAVE_CONFIG_H */

#include <asterisk.h>
#include <asterisk/utils.h>		/* ast_free() */

#include <asterisk-chan-dongle/at_queue.h>
#include "chan_dongle.h"		/* struct pvt */

void at_log(struct pvt* pvt, const char* buf, size_t count);

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


EXPORT_DEF int at_write (struct pvt* pvt, const char* buf, size_t count)
{
	size_t wrote;
	char dn[256];
	timenow(dn);


	at_log(pvt,dn,strlen(dn));
	at_log(pvt," >> ",4);
	at_log(pvt,buf,count);

	ast_debug (5, "[%s] [%.*s]\n", PVT_ID(pvt), (int) count, buf);

	wrote = write_all(pvt->data_fd, buf, count);
	PVT_STAT(pvt, d_write_bytes) += wrote;
	if(wrote != count)
	{
		ast_debug (1, "[%s] write() error: %d\n", PVT_ID(pvt), errno);
	}

	return wrote != count;
}

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
                                                       
 
                     
                                                     

        
  
                      
   
                                                         
   
  

                   
 
