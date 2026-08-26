/*
 * Copyright (C) 2014-2026 Anton Dodonov (NativeMind)
 * https://github.com/Anton-Dodonov
 * http://linkedin.com/in/anton-dodonov/
 * mailto:anton.v.dodonov@gmail.com
 */

#ifdef HAVE_CONFIG_H
#include <svistok_config.h>
#endif /* HAVE_CONFIG_H */

#include <asterisk-chan-dongle/memmem.h>
#include <string.h>			/* memchr() */

#include "../ringbuffer.h"

                                                                                   
 
            

                                                
  
                                  
   
                             
                                                     
    
               
               

                                           
     
              
     
    
   
       
   
                                                     
    
             
    
   

           
  

           
 

/* ============================ READ ============================= */
                                                                                 
 
                  
  
                                       
   
                                           
                                         
                                
                                               
            
   
      
   
                                           
                              
                       
            
   
  

          
 

                                                                                           
 
                    
  
           
  

             
  
                                  
   
                                           
                                         
                                
                                          
            
   
      
   
                                           
                         
                       
            
   
  

          
 

                                                                                                
 
         

                  
  
                                       
   
                                           
                                         
                                                                 
    
                                         
                       
             
    
  
                                                                       
    
                                 
                                    
             
    
   
       
   
                                           
                              
                                                                 
    
                                         
                       
             
    
   
  

          
 





                                                                                                                    
 
          
         

              
  
                                                          
  

                                                
  
                                       
   
                                           
                                         
                             
    
                                                                         
     
                                          
                        
              
     

          
                                                                 
    
       
    
                             
                                      
    


                  
    
                                                                                            
     
                                                        
                        
              
     

                                       
     
              
     

                      
        
    

                                        
    
                                                                               
     
                         
      
                         
               
      
    
                                  
                                     
              
     
    
   
       
   
                                           
                              
                                                                        
    
                                         
                       

             
    
   
  

          
 

                                                                 
 
          

                    
  
                 
  

             
  
                  

                    
   
                 
                 
   
      
   
                      

                     
    
                            
    
       
    
                 
    
   
  

            
 

/* unused
static size_t rb_read (struct ringbuffer* rb, char* buf, size_t len)
{
	size_t s;

	if (rb->used < len)
	{
		len = rb->used;
	}

	if (len > 0)
	{
		s = rb->read + len;
		if (s > rb->size)
		{
			memmove (buf, rb->buffer + rb->read, rb->size - rb->read);
			memmove (buf + rb->size - rb->read, rb->buffer, s - rb->size);
			rb->read = s - rb->size;
		}
		else
		{
			memmove (buf, rb->buffer + rb->read, len);
			if (s == rb->size)
			{
				rb->read = 0;
			}
			else
			{
				rb->read = s;
			}
		}

		rb->used -= len;

		if (rb->used == 0)
		{
			rb->read  = 0;
			rb->write = 0;
		}
	}

	return len;
}
*/

/* ============================ WRITE ============================ */

/* Svistok-only composition fragment. */

EXPORT_DEF int rb_read_until_char_after_iov (const struct ringbuffer* rb, struct iovec iov[2], char c, int after)
{
	void* p;

	if (rb->used > 0)
	{
		if ((rb->read + rb->used) > rb->size)
		{
			iov[0].iov_base = rb->buffer + rb->read;
			iov[0].iov_len  = rb->size - rb->read;
			if ((p = memchr (iov[0].iov_base+after, c, iov[0].iov_len-after)) != NULL)
			{
				iov[0].iov_len = p - iov[0].iov_base;
				iov[1].iov_len = 0;
				return 1;
			}
		
			if ((p = memchr (rb->buffer+after, c, rb->used - iov[0].iov_len-after)) != NULL)
			{
				iov[1].iov_base = rb->buffer;
				iov[1].iov_len = p - rb->buffer;
				return 2;
			}
		}
		else 
		{
			iov[0].iov_base = rb->buffer + rb->read;
			iov[0].iov_len  = rb->used;
			if ((p = memchr (iov[0].iov_base+after, c, iov[0].iov_len-after)) != NULL)
			{
				iov[0].iov_len = p - iov[0].iov_base;
				iov[1].iov_len = 0;
				return 1;
			}
		}
	}

	return 0;
}
