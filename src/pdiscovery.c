/*1
   Copyright (C) 2011 bg <bg_one@mail.ru>
*/
#ifdef HAVE_CONFIG_H
#include <svistok_config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>			/* u_int16_t u_int8_t */
#include <dirent.h>			/* DIR */
#include <stdio.h>			/* NULL */
#include <string.h>			/* strlen() */
#include <sys/stat.h>			/* stat() */

#include "pdiscovery.h"			/* pdiscovery_lookup()  */
#include <asterisk-chan-dongle/mutils.h>	/* ITEMS_OF() */
#include "ringbuffer.h"			/* struct ringbuffer */
#include <asterisk-chan-dongle/at_queue.h>	/* write_all() */
#include <asterisk-chan-dongle/at_read.h>	/* at_wait() at_read() at_read_result_iov() at_read_result_classification() */
#include "chan_dongle.h"		/* opentty() closetty() */
#include <asterisk-chan-dongle/manager.h>	/* manager_event_message_raw() */

/*
static const char sys_bus_usb_drivers_usb[] = "/sys/bus/usb/drivers/usb"; 
*/
extern const char svistok_bridge_upstream_sys_bus_usb_devices[21];
;


/* timeout for port readering milliseconds */
#define PDISCOVERY_TIMEOUT		1000


struct pdiscovery_device {
	u_int16_t	vendor_id;
	u_int16_t	product_id;
	u_int8_t	interfaces[INTERFACE_TYPE_NUMBERS];
};

struct pdiscovery_request {
	const char	* name;
	const char	* imei;
	const char	* imsi;
	const char	* serial;
};

struct pdiscovery_cache_item {
	AST_LIST_ENTRY (pdiscovery_cache_item)	entry;
	struct timeval			validtill;
	int				status;
	struct pdiscovery_result	res;
};

struct discovery_cache {
	AST_RWLIST_HEAD (, pdiscovery_cache_item)  items;
};


#define BUILD_NAME(d1, d2, d1len, d2len, out)		\
		d2len = strlen(d2);			\
		out = alloca(d1len + 1 + d2len + 1);	\
		memcpy(out, d1, d1len);			\
		out[d1len] = '/';			\
		memcpy(out + d1len + 1, d2, d2len);	\
		d2len += d1len + 1;			\
		out[d2len] = 0;


static const struct pdiscovery_device device_ids[] = {
	{ 0x12d1, 0x1001, { 2, 1, /* 0 */ } },		/* E1550 and generic */
//	{ 0x12d1, 0x1465, { 2, 1, /* 0 */ } },		/* K3520 */
	{ 0x12d1, 0x140c, { 3, 2, /* 0 */ } },		/* E17xx */
};

extern struct discovery_cache svistok_bridge_upstream_cache;
;

#/* return non-0 if all ports matched */
                                                                                              
 
            
                                           
                                                                                
            
  
          
 

#/* */
                                                                                         
 
            
                                          
                             
                                             
                            
             
   
           
 

#/* */
                                                       
 
            
                                            
                               
                             
                          
   
 

#/* */
static void info_free(struct pdiscovery_result * res)
{
	if(res->imsi) {
		ast_free(res->imsi);
		res->imsi = NULL;
	}	

	if(res->imei) {
		ast_free(res->imei);
		res->imei = NULL;
	}		

	if(res->serial) {
		ast_free(res->serial);
		res->serial = NULL;
	}		

}

#/* */
static void info_copy(struct pdiscovery_result * dst, const struct pdiscovery_result * src)
{
	if(src->imei)
		dst->imei = ast_strdup(src->imei);
	if(src->imsi)
		dst->imsi = ast_strdup(src->imsi);
	if(src->serial)
		dst->serial = ast_strdup(src->serial);

}

#/* */
extern void svistok_bridge_upstream_result_free(struct pdiscovery_result * res);
  
                
 

#/* */
                                                                
 
           
                          
                 
  
 

#/* */
                                                                                                                    
 
                       
                            
 
                       

                               
                                                           
 

#/* */
                                                                                                         
 
                                                                    
           
                                                 
                                        
          
                         
               
   
  
 
             
 

#/* */
extern struct pdiscovery_cache_item * svistok_bridge_upstream_cache_search(struct discovery_cache * cache, const struct pdiscovery_result * res);
                      
                                     
                                  

                                  
                                                           
                                           
                                                   
                 
          
    
          
                         
                                  
                         
   
  
                            
                                  

              
 


#/* */
static int cache_lookup(struct discovery_cache * cache, const struct pdiscovery_request * req, struct pdiscovery_result * res, int * failed)
{
	int found = 0;
	struct pdiscovery_cache_item * item = cache_search(cache, res);
	if(item) {
		res->imei = item->res.imei ? ast_strdup(item->res.imei) : NULL;
		res->imsi = item->res.imsi ? ast_strdup(item->res.imsi) : NULL;
		res->serial = item->res.serial ? ast_strdup(item->res.serial) : NULL;
		found = item->status || ((req->imei || item->res.imei) && (req->imsi || item->res.imsi) && (req->serial || item->res.serial));
		if(found) {
			*failed = item->status;
		}
	}
	return found;
}

#/* */
                                                                                                          
 
                                                                
           
                                       
         
                                        
                                                   
  
 

#/* */
                                                      
 
                                                          

                                     
 

#/* */
                                                      
 
                                     

                                  
                                                           
                                 
                        
  
                            
                                  

                                        
 

#/* */
extern const struct pdiscovery_cache_item * svistok_bridge_upstream_cache_first_readlock(struct discovery_cache * cache);
           
                                        
 

#/* */
                                                        
 
                                  
 

#/* */
                                                                                                   
 
          
              
                

                                              
                                 
           
                                       
               
  

               
 

#/* */
                                                         
 
          
              
                   

                                                   
                                                           
 


#/* */
                                                                              
 
                
                       
                   
                    

                                            

                                                                                            
                                                                            
                                             
                           
  
             
 


#/* */
                                                              
 
                    
                        
                           
          
                                          
                                                                              
                                                      
            
           
    
   
                
  
             
 

#/* */
                                                                                    
 
                    
                                                                       
                                                                                
                                         
  
             
 

#/* */
                                                                                                         
 
          
              
                   
                    

                                            

                                                         
                                                      
  
             
 

#/* */
                                                                                                                                                            
 
                    
              
               
                        
             

                           
          
                                          
                                    
                                                                       
              
                                                                                                   
                                                                   
                                                
                                      
                                 
                                             
               
               
                                                                                                                                                 
                
        
       
      
     
    
   
                
  
              
 





#/* */
                                                                                                                     
 
              
              
              

                                                                                                                  
                                                                                                     
                                                   
                                                                              
                            
     
   
  
             
 

#/* 0D 0A IMEI: <15 digits> 0D 0A */
extern char * svistok_bridge_upstream_pdiscovery_handle_ati(const char * devname, char * str);
                 
                                 

           
                       
                       
          
             

                                       
         
                                                                     
              
                           
                 
                                                                 
               
   
  

             
 

static char * pdiscovery_handle_sn(const char * devname, char * str)
{
	static const char SERIAL[] = "\r\n^SN:";
	char * serial = strstr(str, SERIAL);

	if(serial) {
		serial += STRLEN(SERIAL);
		while(serial[0] == ' ')
			serial++;
		str = serial;

		while((str[0] >= '0' && str[0] <= '9')||(str[0] >= 'A' && str[0] <= 'Z'))
			str++;
		if((str - serial) == SERIAL_SIZE && str[0] == '\r' && str[1] == '\n') {
			str[0] = 0;
			serial = ast_strdup(serial);
			str[0] = '\r';
			ast_debug(4, "[%s discovery] found S %s\n", devname, serial);
			return serial;
		}
	}

	return NULL;
}


#/* 0D 0A 15 digits 0D 0A */
extern char * svistok_bridge_upstream_pdiscovery_handle_cimi(const char * devname, char * str);
            
              
            
            
               
            
         

                                        
                 
                    
                    
             
          
                  
                    
             
        
                         
          
                  
                                    
             
                
                           
                       
        
                         
          
                     
                                  
      
                           
                                  
              
         
                        
          
                         
          
                  
                      
                 
                             
                    
                                                                   
                 
     
               
           
                        
   
  

             
 

#/* return non-zero on done with command */
static int pdiscovery_handle_response(const struct pdiscovery_request * req, const struct iovec iov[2], int iovcnt, struct pdiscovery_result * res)
{
	int done = 0;
	char * str;
	char sym;
	size_t len = iov[0].iov_len + iov[1].iov_len;
	if(len > 0) {
		len--;
		if(iovcnt == 2) {
			str = alloca(len + 1);
			if(!str) {
				return 1;
			memcpy(str                 , iov[0].iov_base, iov[0].iov_len);
			memcpy(str + iov[0].iov_len, iov[1].iov_base, iov[1].iov_len);
			}
		} else {
			str = iov[0].iov_base;
		}
		sym = str[len];
		str[len] = 0;

		ast_debug(4, "[%s discovery] < %s\n", req->name, str);
		done = strstr(str, "OK") != NULL || strstr(str, "ERROR") != NULL;
		if(req->imei && res->imei == NULL)
			res->imei = pdiscovery_handle_ati(req->name, str);
		if(req->imsi && res->imsi == NULL)
			res->imsi = pdiscovery_handle_cimi(req->name, str);
		if(req->serial && res->serial == NULL)
			res->serial = pdiscovery_handle_sn(req->name, str);
			

		/* restore tail of string for collect data in buffer */
		str[len] = sym;
	}
	return done;
}


#/* return zero on sucess */
static int pdiscovery_do_cmd(const struct pdiscovery_request * req, int fd, const char * name, const char * cmd, unsigned length, struct pdiscovery_result * res)
{
	int timeout;
	char buf[1024 + 1];
	struct ringbuffer rb;
	struct iovec iov[2];
	int iovcnt;
	size_t wrote;

	ast_debug(4, "[%s discovery] use %s for IMEI/IMSI discovery\n", req->name, name);

	clean_read_data(req->name, fd);
	wrote = write_all(fd, cmd, length);
	if(wrote == length) {
		timeout = PDISCOVERY_TIMEOUT;
		rb_init(&rb, buf, sizeof(buf) - 1);
		while(timeout > 0 && at_wait(fd, &timeout) != 0) {
			iovcnt = at_read(fd, name, &rb);
			if(iovcnt > 0) {
				iovcnt = rb_read_all_iov(&rb, iov);
				if(pdiscovery_handle_response(req, iov, iovcnt, res))
					return 0;
			} else {
				snprintf(buf, sizeof(buf), "Read Failed\r\nErrorCode: %d", errno);
				manager_event_message_raw("DonglePortFail", name, buf);
				ast_log (LOG_ERROR, "[%s discovery] read from %s failed: %s\n", req->name, name, strerror(errno));
				return -1;
			}
		}
		manager_event_message_raw("DonglePortFail", name, "Response Failed");
		ast_log (LOG_ERROR, "[%s discovery] failed to get valid response from %s in %d msec\n", req->name, name, PDISCOVERY_TIMEOUT);
	} else {
		snprintf(buf, sizeof(buf), "Write Failed\r\nErrorCode: %d", errno);
		manager_event_message_raw("DonglePortFail", name, buf);
		ast_log (LOG_ERROR, "[%s discovery] write to %s failed: %s\n", req->name, name, strerror(errno));
	}
	return 1;
}

#/* return non-zero on fail */
static int pdiscovery_get_info(const char * port, const struct pdiscovery_request * req, struct pdiscovery_result * res)
{
	static const struct {
		const char	* cmd;
		unsigned	length;
	} cmds[] = {
		{ "AT+CIMI\r", 8 },		/* IMSI */
		{ "ATI\r", 4 },			/* IMEI */
		{ "ATI; +CIMI\r" , 11 },	/* IMSI + IMEI */
		{ "AT^SN\r" , 6 },	/* serial */
	};

	static const int want_map[2][2][2] = {
		{ {2,3}, {0,3},  },	// want_imei = 0
		{ {1,3}, {2,3},  }	// want_imei = 1
	};

	int fail = 1;
	char * lock_file;

	int fd = opentty(port, &lock_file);
	if(fd >= 0) {
		unsigned want_imei = req->imei && res->imei == NULL;		// 1 && 0
		unsigned want_imsi = req->imsi && res->imsi == NULL;		// 1 && 1
		unsigned want_serial = req->serial && res->serial == NULL;		// 1 && 1
		
		unsigned cmd = want_map[want_imei][want_imsi][want_serial];
		
		/* clean queue first ? */
		fail = pdiscovery_do_cmd(req, fd, port, cmds[cmd].cmd, cmds[cmd].length, res);
		closetty(fd, &lock_file);
	}

	return fail;
}

#/* return non-zero on fail */
                                                                                                                               
 
              
                                  
                                                   
             
                                             
                                  
         
                                                                                                                                            
  
  
             
 

#/* return zero on success */
extern int svistok_bridge_upstream_pdiscovery_read_info(const struct pdiscovery_request * req, struct pdiscovery_result * res);
       

                                                             
                                                            

                                           
                                    
                
                                                      
                        
          
                                                                                                  
                                                                                                  
   
           
                                                       
    
             
 

#/* */
static int pdiscovery_check_req(const struct pdiscovery_request * req, struct pdiscovery_result * res)
{
	int match = 0;
	if(pdiscovery_read_info(req, res) == 0) {

		match = ((req->imei == 0) || (res->imei && strcmp(req->imei, res->imei) == 0))
			&&
			((req->imsi == 0) || (res->imsi && strcmp(req->imsi, res->imsi) == 0))
			&&
			((req->serial == 0) || (res->serial && strcmp(req->serial, res->serial) == 0));

		ast_debug(4, "[%s discovery] %smatched IMEI=%s/%s IMSI=%s/%s S=%s/%s\n",
			req->name,
			match ? "" : "un" ,
			S_OR(req->imei, "") , S_OR(res->imei, ""),
			S_OR(req->imsi, ""),  S_OR(res->imsi, ""),
			S_OR(req->serial, "") , S_OR(res->serial, "")
			);
	}

	return match;
}

#/* */
                                                                                                                                                          
 
          
              
                                         
               

                                            

                                                        
             
                                                                                                                   
                                                                                                      
             
                      
                       
                                            
                                            
                                          
     
                                                                     

                             
                                                                                       
                                          
   
  

           
                   
              
 

#/* */
extern int svistok_bridge_upstream_pdiscovery_request_do(const char * name, int len, const struct pdiscovery_request * req, struct pdiscovery_result * res);
                 
                           
          
                                          
                                                                                                                                 
                                                                                     
                                                                         
             
           
    
   
                
  
              
 


#/* */
                                 
 
                    
 

#/* */
                                 
 
                    
 

#/* */
EXPORT_DEF int pdiscovery_lookup(const char * devname, const char * imei, const char * imsi, const char * serial, char ** dport, char ** aport)
{
	int found;
	struct pdiscovery_result res;
	const struct pdiscovery_request req = {
		devname, 
		((imei && imei[0]) ? imei : NULL),
		((imsi && imsi[0]) ? imsi : NULL),
		((serial && serial[0]) ? serial : NULL),
		};

	memset(&res, 0, sizeof(res));
	found = pdiscovery_request_do(sys_bus_usb_devices, STRLEN(sys_bus_usb_devices), &req, &res);
	if(found) {
		*dport = ast_strdup(res.ports.ports[INTERFACE_TYPE_DATA]);
		*aport = ast_strdup(res.ports.ports[INTERFACE_TYPE_VOICE]);
	}
	result_free(&res);
	return found;
}

#/* */
EXPORT_DEF const struct pdiscovery_result * pdiscovery_list_begin(const struct pdiscovery_cache_item ** opaque)
{
	const struct pdiscovery_cache_item * item;
	struct pdiscovery_result res;
	const struct pdiscovery_request req = {
		"list", 
		"ANY", 
		"ANY", 
		};

	memset(&res, 0, sizeof(res));
	pdiscovery_request_do(sys_bus_usb_devices, STRLEN(sys_bus_usb_devices), &req, &res);
	result_free(&res);

	*opaque = item = cache_first_readlock(&cache);
	return item != NULL ? &item->res : NULL;
}

#/* */
                                                                                                               
 
                                                                             
                
                                         
 

#/* */
                                      
 
                      
 
