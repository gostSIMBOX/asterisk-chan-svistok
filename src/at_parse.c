/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>

   bg <bg_one@mail.ru>
*/
#ifdef HAVE_CONFIG_H
#include <svistok_config.h>
#endif /* HAVE_CONFIG_H */

#include <asterisk-chan-dongle/memmem.h>

#include <stdio.h>			/* NULL */
#include <errno.h>			/* errno */
#include <stdlib.h>			/* strtol */

#include "at_parse.h"
#include <asterisk-chan-dongle/mutils.h>	/* ITEMS_OF() */
#include "chan_dongle.h"
#include "pdu.h"			/* pdu_parse() */

#/* */
extern unsigned int svistok_bridge_upstream_mark_line(char * line, const char * delimiters, char ** pointers);
                                         
  
                                  
   
                          
           
   
  
              
 

/*!
 * \brief Parse a CNUM response
 * \param str -- string to parse (null terminated)
 * @note str will be modified when the CNUM message is parsed
 * \return NULL on error (parse error) or a pointer to the subscriber number
 */

                                           
 
   
                                                
                                 
             
                                                   
                                       
                                     
    

                           
                                  

                          
                                                         
  
             
                        
              
                         
              
                  
                  
  

             
 

/*!
 * \brief Parse a COPS response
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * @note str will be modified when the COPS message is parsed
 * \return NULL on error (parse error) or a pointer to the provider name
 */

                                          
 
   
                                                
                                       
   
            
                         
    

                            
                                  

                          
                                                         
  
             
                        
              
                         
              
                  
                  
  

             
 


EXPORT_DEF char* at_parse_spn (char* str)
{
	/*
	 * parse COPS response in the following format:
	 * +COPS: <mode>[,<format>,<oper>,<?>]
	 *
	 * example 
	 *  ^SPN:1,0,SIM-1
	 *  +COPS: 0,0,"TELE2",0
	 */

	char delimiters[] = ":,,";
	char * marks[STRLEN(delimiters)];

	/* parse URC only here */
	if(mark_line(str, delimiters, marks) == ITEMS_OF(marks))
	{
		marks[2]++;
		/*quotes if(marks[2][0] == '"')
			marks[2]++;
		if(marks[3][-1] == '"')
			marks[3]--;
		marks[3][0] = 0;*/
		return marks[2];
	}

	return NULL;
}



/*!
 * \brief Parse a CREG response
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \param gsm_reg -- a pointer to a int
 * \param gsm_reg_status -- a pointer to a int
 * \param lac -- a pointer to a char pointer which will store the location area code in hex format
 * \param ci  -- a pointer to a char pointer which will store the cell id in hex format
 * @note str will be modified when the CREG message is parsed
 * \retval  0 success
 * \retval -1 parse error
 */

                                                                                                                
 
            
           
                 
                 
                 
                 

              
                      
             
             

   
                                                
                                  
    

                                                  
  
                
   
          
                      
     
             
     
          

          
                      
     
                  
             
     
                      

          
                      
     
                   
             
     
          

          
                      
     
                  
             
     
                      
          
                      
     
                   
             
     
          

          
                      
     
                  
             
     
                      

          
                      
     
                   
             
     
          

          
                      
     
                  
             
     
          
   
  

               
  
            
  

                                            
  
          
  

        
  
            
                                                         
                                              
   
                        
             
   

                                                   
   
                
   
  

                     
  
            
            
  
                   
  
            
            
  

          
 

/*!
 * \brief Parse a CMTI notification
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * @note str will be modified when the CMTI message is parsed
 * \return -1 on error (parse error) or the index of the new sms message
 */

                                              
 
           

   
                                            
                         
    

                                                                   
 


                                                                                                                                                      
 
   
                                                 
                                                         
                                  
              
      
                                                       
                                  
              
    

                             
                                  
               
 
                                                     
                             
  
                      
             
                        
              
                         
              
                                   
                     
                                               
                                                               
                  
                               

                      
                               
                                                       
              
  
                   
                          

                                          
 

                                                                                                                                                                
 
   
                                               
                                                            
                                        
              
   
          
                
                                                                                                  
              
    

                            
                                  
            
                    

                                                          
  
                                               
                                        
                                                        
                      
                                                                       
  

                                     
 

EXPORT_DEF const char* at_parse_cds(char** str, attribute_unused size_t len, char* oa, size_t oa_len, str_encoding_t* oa_enc, char** msg, str_encoding_t* msg_enc)
{
	/* from +CDS: 25
	 * parse cmgr info in the following PDU format
	 * +CMGR: message_status,[address_text],TPDU_length<CR><LF>
	 * SMSC_number_and_TPDU<CR><LF><CR><LF>
	 * OK<CR><LF>
	 *
	 *	sample
	 * +CMGR: 1,,31
	 * 07911234567890F3040B911234556780F20008012150220040210C041F04400438043204350442<CR><LF><CR><LF>
	 * OK<CR><LF>
	 */

	char delimiters[] = ":\n";
	char * marks[STRLEN(delimiters)];
	char * end;
	size_t tpdu_length;

	if(mark_line(*str, delimiters, marks) == ITEMS_OF(marks))
	{
		tpdu_length = strtol(marks[0] + 2, &end, 10);
		if(tpdu_length <= 0 || end[0] != '\r')
			return "Invalid TPDU length in CDS PDU status line";

		ast_verb(3,"CDS length: %d, %s, %s",tpdu_length, end+2,marks[1] + 1);
		*str = marks[1] + 1;

FILE * fp;
fp=fopen("/var/log/cds.log","a");
if(fp)
{
    fprintf(fp,"%s\n",*str);
    fclose(fp);
}

		return pdu_parse_cds(str, tpdu_length+8, oa, oa_len, oa_enc, msg, msg_enc);
	}

	return "Can't parse +CDS response";
}


/*!
 * \brief Parse a CMGR message
 * \param str -- pointer to pointer of string to parse (null terminated)
 * \param len -- string lenght
 * \param number -- a pointer to a char pointer which will store the from number
 * \param text -- a pointer to a char pointer which will store the message text
 * @note str will be modified when the CMGR message is parsed
 * \retval  0 success
 * \retval -1 parse error
 */

                                                                                                                                                        
 
                                                    

                    
           
          

                          
                                   
  
           
        
  

            
  
                              
                                                                                                                                        
                                                             

                                                           
  

           
 

 /*!
 * \brief Parse a CUSD answer
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * @note str will be modified when the CUSD string is parsed
 * \retval  0 success
 * \retval -1 parse error
 */

                                                                            
 
   
                                               
                            
    
            
              
                                                                                                                    
    

                           
                                  
                

            
            
           

                                           
             
              
  
                                           
   
                
    
               
                          
                
                     

                   
                                     
                            
                 
                     
            
                             
                                           
                         
     
    
            
   
  
           
 

/*!
 * \brief Parse a CPIN notification
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \return  2 if PUK required
 * \return  1 if PIN required
 * \return  0 if no PIN required
 * \return -1 on error (parse error) or card lock
 */

EXPORT_DEF int at_parse_cpin (char* str, size_t len)
{
	static const struct {
		const char	* value;
		unsigned	length;
	} resp[] = {
		{ "READY", 5 },
		{ "SIM PIN", 7 },
		{ "SIM PUK", 7 },
	};

	ast_verb(3,"ATCPIN: %s",str);

	unsigned idx;
	for(idx = 0; idx < ITEMS_OF(resp); idx++)
	{
		if(memmem (str, len, resp[idx].value, resp[idx].length) != NULL)
			return idx;
	}
	return -1;
}

/*!
 * \brief Parse +CSQ response
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error

– received signal strength indication
0 – (-113) dBm or less
1 – (-111) dBm
2..30 – (-109)dBm..(-53)dBm / 2 dBm per step
31 – (-51)dBm or greater
99 – not known or not detectable
[-113 + Х * 2]

 */



                                                        
 
   
                                                
                      
    

                                                      
 

/*!
 * \brief Parse a ^RSSI notification
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \return -1 on error (parse error) or the rssi value
 */

                                              
 
               

   
                                            
                
    

                                 
             
 

/*!
 * \brief Parse a ^MODE notification (link mode)
 * \param str -- string to parse (null terminated)
 * \param len -- string lenght
 * \return -1 on error (parse error) or the the link mode value
 */

                                                                    
 
   
                                            
                          
    

                                                                 
 


EXPORT_DEF int at_parse_sysinfo (char * str, int * srvst, int * srvd, int * roamst, int * sysmode, int * simst)
{
	/*
	    ^SYSINFO:1,0,1,3,0,,3
	    srv_status >, < srv_domain >,< roam_status >, < sys_mode >,< sim_state 
	 */

	return sscanf (str, "^SYSINFO:%d,%d,%d,%d,%d", srvst, srvd, roamst, sysmode, simst) == 5 ? 0 : -1;
}


#/* */
                                                     
 
   
                                            
                        
                              
                  
    
                            
                                  

                                                         
  
                       
                  
           
  

           
 

#/* */
                                                                                                                                                                
 
   
                                                                                       
        
                                                                                       
             
                             
                                         
                                          
                                         
    
                               
                                  

               
          
            
           
           
              
          

                                                         
  
                                               
     
                                          
     
                                            
     
                                           
     
                                           
     
                                            
   
              
                         
               
                          
               
                      
                   

            
   
  

           
 

#/* */
                                                         
 
    
                        
                            
                 
                            
               
                                                     
                             
   
                           
                                                                                                
    
                           
                                  

                          
                                                         
  
                                            
            
  

           
 
