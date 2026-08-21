/*
 * chan_dongle
 *
 * Copyright (C) 2011
 * bg <bg_one@mail.ru>
 * http://www.e1550.mobi
 *
 * chan_dongle is based on chan_datacard by
 *
 * Artem Makhutov <artem@makhutov.org>
 * http://www.makhutov.org
 *
 * Dmitry Vagin <dmitry2004@yandex.ru>
 *
 * Copyright (C) 2011
 * bg <bg_one@mail.ru>
 * http://www.e1550.mobi

 * chan_datacard is based on chan_mobile by Digium
 * (Mark Spencer <markster@digium.com>)
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \brief UMTS Voice Dongle channel driver
 *
 * \author Artem Makhutov <artem@makhutov.org>
 * \author Dave Bowerman <david.bowerman@gmail.com>
 * \author Dmitry Vagin <dmitry2004@yandex.ru>
 * \author bg <bg_one@mail.ru>
 *
 * \ingroup channel_drivers
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */


#include <asterisk.h>
ASTERISK_FILE_VERSION(__FILE__, "$Rev: " PACKAGE_REVISION " $")

#include <asterisk/ast_version.h>
#include <asterisk/stringfields.h>	/* AST_DECLARE_STRING_FIELDS for asterisk/manager.h */
#include <asterisk/manager.h>
#include <asterisk/dsp.h>
#include <asterisk/callerid.h>
#include <asterisk/module.h>		/* AST_MODULE_LOAD_DECLINE ... */
#include <asterisk/timing.h>		/* ast_timer_open() ast_timer_fd() */

#include <sys/stat.h>			/* S_IRUSR | S_IRGRP | S_IROTH */
#include <termios.h>			/* struct termios tcgetattr() tcsetattr()  */
#include <pthread.h>			/* pthread_t pthread_kill() pthread_join() */
#include <fcntl.h>			/* O_RDWR O_NOCTTY */
#include <signal.h>			/* SIGURG */

#include "chan_dongle.h"
#include "at_response.h"		/* at_res_t */
#include "at_queue.h"			/* struct at_queue_task_cmd at_queue_head_cmd() */
#include "at_command.h"			/* at_cmd2str() */
#include "mutils.h"			/* ITEMS_OF() */
#include "at_read.h"
#include "cli.h"
#include "app.h"
#include "manager.h"
#include "channel.h"			/* channel_queue_hangup() */
#include "dc_config.h"			/* dc_uconfig_fill() dc_gconfig_fill() dc_sconfig_fill()  */
#include "pdiscovery.h"			/* pdiscovery_lookup() pdiscovery_init() pdiscovery_fini() */

#include "simnode/adiscovery_svistok.c"
//include "dsp.c"


//include "share.c"

char svistok_version[64]="1.1200";

pvt_config_t def_settings;


EXPORT_DEF const char * const dev_state_strs[4] = { "stop", "restart", "remove", "start" };
EXPORT_DEF public_state_t * gpublic;
EXPORT_DEF struct ast_format chan_dongle_format;
EXPORT_DEF struct ast_format_cap * chan_dongle_format_cap;

static int public_state_init(struct public_state * state);


/*!
 * Get status of the dongle. It might happen that the device disappears
 * (e.g. due to a USB unplug).
 *
 * \return 0 if device seems ok, non-0 if it seems not available
 */

static int port_status (int fd)
{
	struct termios t;

	if (fd < 0)
	{
		return -1;
	}

	return tcgetattr (fd, &t);
}

#/* return length of lockname */
static int lock_build(const char * devname, char * buf, unsigned length)
{
	const char * basename;
	char resolved_path[PATH_MAX];

	/* follow symlinks */
	if(realpath(devname, resolved_path) != NULL)
		devname = resolved_path;

	basename = strrchr(devname, '/');
	if(basename)
		basename++;
	else
		basename = devname;

	/* NOTE: use system system wide lock directory */
	return snprintf(buf, length, "/var/lock/LOCK..%s", basename);
}

#/* return 0 on error */
static int lock_create(const char * lockfile)
{
	int fd;
	int len = 0;
	char pidb[21];

	fd = open(lockfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IROTH);
	if(fd >= 0)
	{
		/* NOTE: bg: i assume next open reuse same fd - not thread-safe */
		len = snprintf(pidb, sizeof(pidb), "%d %d", getpid(), fd);
		len = write(fd, pidb, len);
		close(fd);
	}
	return len;
}

#/* return pid of owner, 0 if free */
EXPORT_DEF int lock_try(const char * devname, char ** lockname)
{
	int fd;
	int len;
	int pid = 0;
	int assigned;
	int fd2;
	char name[1024];
	char buffer[65];

	lock_build(devname, name, sizeof(name));

	/* FIXME: rise conditions: some time between lock check and got lock */
	fd = open(name, O_RDONLY);
	if(fd >= 0)
	{
		len = read(fd, buffer, sizeof(buffer) - 1);
		if(len > 0)
		{
			buffer[len] = 0;
			assigned = sscanf(buffer, "%d %d", &len, &fd2);
			if(assigned > 0 && kill(len, 0) == 0)
			{
				if(len == getpid() && assigned > 1)
				{
					if(port_status(fd2) == 0)
						pid = len;
				}
				else
					pid = len;
			}
		}
		close(fd);
	}

	if(pid == 0)
	{
		unlink(name);
		lock_create(name);
		*lockname = ast_strdup(name);
	}
	return pid;
}

#/* */
EXPORT_DEF void closetty(int fd, char ** lockfname)
{
	close(fd);

	/* remove lock */
	unlink(*lockfname);
	ast_free(*lockfname);
	*lockfname = NULL;
}

EXPORT_DEF int opentty (const char* dev, char ** lockfile)
{
	int		flags;
	int		pid;
	int		fd;
	struct termios	term_attr;
	char		buf[40];


	pid = lock_try(dev, lockfile);
	if(pid != 0)
	{
		ast_log (LOG_WARNING, "%s already used by process %d\n", dev, pid);
		return -1;
	}

	ast_log (LOG_WARNING, "AAA2Trying to open %s\n", dev);
	// AAA 
	//fd = open (dev, O_RDWR | O_NOCTTY);
	fd = open (dev, O_RDWR | O_NOCTTY | O_NDELAY);
	ast_log (LOG_WARNING, "AAA 1 Trying to open %s\n", dev);
	if (fd < 0)
	{
		flags = errno;
		closetty(fd, lockfile);
		snprintf(buf, sizeof(buf), "Open Failed\r\nErrorCode: %d", flags);
		manager_event_message_raw("DonglePortFail", dev, buf);
		ast_log (LOG_WARNING, "unable to open %s: %s\n", dev, strerror(flags));
		return -1;
	}
	ast_log (LOG_WARNING, "AAA 2 Trying to open %s\n", dev);

	flags = fcntl(fd, F_GETFD);
	if(flags == -1 || fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
	{
		flags = errno;
		closetty(fd, lockfile);
		ast_log (LOG_WARNING, "fcntl(F_GETFD/F_SETFD) failed for %s: %s\n", dev, strerror(flags));
		return -1;
	}
	ast_log (LOG_WARNING, "AAA 3 Trying to open %s\n", dev);

	if (tcgetattr (fd, &term_attr) != 0)
	{
		flags = errno;
		closetty(fd, lockfile);
		ast_log (LOG_WARNING, "tcgetattr() failed for %s: %s\n", dev, strerror(flags));
		return -1;
	}

	term_attr.c_cflag = B115200 | CS8 | CREAD | CRTSCTS;
	term_attr.c_iflag = 0;
	term_attr.c_oflag = 0;
	term_attr.c_lflag = 0;
	term_attr.c_cc[VMIN] = 1;
	term_attr.c_cc[VTIME] = 0;

	if (tcsetattr (fd, TCSAFLUSH, &term_attr) != 0)
	{
		ast_log (LOG_WARNING, "tcsetattr(TCSAFLUSH) failed for %s: %s\n", dev, strerror(errno));
	}

	return fd;
}

#/* phone monitor thread pvt cleanup */
static void disconnect_dongle (struct pvt* pvt);

static void disconnect_dongle (struct pvt* pvt)
{
	struct cpvt * cpvt, * next;

	if (!PVT_NO_CHANS(pvt))
	{
		ast_debug (1, "[%s] Dongle disconnecting, hanging up channels\n", PVT_ID(pvt));

		for(cpvt = pvt->chans.first; cpvt; cpvt = next)
		{
			next = cpvt->entry.next;
			at_hangup_immediality(cpvt);
			CPVT_RESET_FLAGS(cpvt, CALL_FLAG_NEED_HANGUP);
			change_channel_state(cpvt, CALL_STATE_RELEASED, 0);
		}
	}
	at_queue_flush(pvt);
	pvt->last_dialed_cpvt = NULL;

	if(pvt->audio_fd>0)
	closetty (pvt->audio_fd, &pvt->alock);
	closetty (pvt->data_fd, &pvt->dlock);

	pvt->data_fd = -1;
	pvt->audio_fd = -1;

	if(pvt->dsp)
		ast_dsp_digitreset(pvt->dsp);
	pvt_on_remove_last_channel(pvt);

/*	pvt->a_write_rb */

	pvt->dtmf_digit = 0;
	pvt->rings = 0;

//	else
	//{
		/* unaffected in case of restart */
		pvt->use_ucs2_encoding = 0;
		pvt->cusd_use_7bit_encoding = 0;
		pvt->cusd_use_ucs2_decoding = 1;
		pvt->gsm_reg_status = -1;
		pvt->rssi = 0;
		pvt->linkmode = 0;
		pvt->linksubmode = 0;
		ast_copy_string (pvt->provider_name, "NONE", sizeof (pvt->provider_name));
		ast_copy_string (pvt->provider_name2, "", sizeof (pvt->provider_name2));
		pvt->manufacturer[0] = '\0';
		pvt->model[0] = '\0';
		pvt->firmware[0] = '\0';
		pvt->imei[0] = '\0';
		pvt->imsi[0] = '\0';
		pvt->serial[0] = '\0';
		pvt->has_subscriber_number = 0;
		ast_copy_string (pvt->subscriber_number, "Unknown", sizeof (pvt->subscriber_number));
		pvt->location_area_code[0] = '\0';
		pvt->cell_id[0] = '\0';
		pvt->sms_scenter[0] = '\0';

		pvt->gsm_registered	= 0;
		pvt->selectbusy	= 0;
		pvt->has_sms = 0;
		pvt->has_voice = 0;
		pvt->has_call_waiting = 0;
		pvt->use_pdu = 0;
	//}

	pvt->connected		= 0;
	pvt->initialized	= 0;
	pvt->use_pdu		= 0;
	pvt->has_call_waiting	= 0;


	pvt->pinrequired = 0;
	pvt->nosim = 0;
	pvt->sim_ready = 0;
	pvt->sim_start = 0;

	pvt->cfun		= -1;
	pvt->simst		= -1;
	pvt->srvst		= -1;


	writepvtstate(pvt);



	/* FIXME: LOST real device state */
	pvt->dialing = 0;
	pvt->ring = 0;
	pvt->cwaiting = 0;
	pvt->outgoing_sms = 0;
	pvt->outgoing_ussd = 0;
	putfilei("sim/state",pvt->imsi,"outgoing_sms",pvt->outgoing_sms);
	putfilei("sim/state",pvt->imsi,"outgoing_ussd",pvt->outgoing_ussd);

	pvt->incoming_sms = 0;
	pvt->volume_sync_step = VOLUME_SYNC_BEGIN;

	pvt->current_state = DEV_STATE_STOPPED;

	/* clear statictics */
	memset(&pvt->stat, 0, sizeof(pvt->stat));

	ast_copy_string (PVT_STATE(pvt, data_tty),  CONF_UNIQ(pvt, data_tty), sizeof (PVT_STATE(pvt, data_tty)));
	ast_copy_string (PVT_STATE(pvt, audio_tty), CONF_UNIQ(pvt, audio_tty), sizeof (PVT_STATE(pvt, audio_tty)));
	*PVT_STATE(pvt, dev)=0;

	ast_verb (3, "[%s] Dongle has disconnected\n", PVT_ID(pvt));
//	dserial_clearname(pvt);
	ast_verb (3, "[%s] Renamed dongle has disconnected\n", PVT_ID(pvt));
	ast_mutex_unlock_pvt (pvt);
	ast_verb (3, "[%s] ???UNLOCK \n", PVT_ID(pvt));
	manager_event_device_status(PVT_ID(pvt), "Disconnect");
}


/* anybody wrote some to device before me, and not read results, clean pending results here */
#/* */
EXPORT_DEF void clean_read_data(const char * devname, int fd)
{
	char buf[2*1024*16];
	struct ringbuffer rb;
	int iovcnt;
	int t;
	
	rb_init (&rb, buf, sizeof (buf));
	for (t = 0; at_wait(fd, &t); t = 0)
	{
		iovcnt = at_read (fd, devname, &rb);
		ast_debug (4, "[%s] drop %u bytes of pending data before initialization\n", devname, (unsigned)rb_used(&rb));
		/* drop readed */
		rb_init (&rb, buf, sizeof (buf));
		if (iovcnt == 0)
			break;
	}
}


/*!
 * \brief Check if the module is unloading.
 * \retval 0 not unloading
 * \retval 1 unloading
 */

static void* do_monitor_phone (void* data)
{
	struct pvt*	pvt = (struct pvt*) data;
	at_res_t	at_res;
	const struct at_queue_cmd * ecmd;
	int		t;
	char buf[2*1024*16];
	struct ringbuffer rb;
	struct iovec	iov[2];
	int		iovcnt;
	char		dev[sizeof(PVT_ID(pvt))];
	int 		fd;
	int		read_result = 0;
	int atr;
	int locked;

	pvt->timeout = DATA_READ_TIMEOUT;
	rb_init (&rb, buf, sizeof (buf));

	// FOR MULTISIMS - WAIT
	//sleep(30);



	pvt->cfun		= -1;
	pvt->simst		= -1;
	pvt->srvst		= -1;
	pvt->pinrequired	= 0;

	writepvtstate(pvt);

		pvt->sim_ready=0;
		pvt->sim_start=0;


	ast_mutex_lock_pvt (pvt);

	/* 4 reduce locking time make copy of this readonly fields */
	fd = pvt->data_fd;
	ast_copy_string(dev, PVT_ID(pvt), sizeof(dev));

	clean_read_data(dev, fd);
	
	/* schedule dongle initilization  */
	if (at_enque_initialization (&pvt->sys_chan, CMD_AT))
	{
		ast_log (LOG_ERROR, "[%s] Error adding initialization commands to queue\n", dev);
		goto e_cleanup;
	}

	ast_mutex_unlock_pvt (pvt);


	while (1)
	{
		//ast_mutex_lock_pvt (pvt);
		//EXP locked=(ast_mutex_trylock_pvt(pvt)!=EBUSY);

		if (port_status (pvt->data_fd) || (port_status (pvt->audio_fd) && pvt->audio_fd>0 ))
		{
			ast_log (LOG_ERROR, "[%s] Lost connection to Dongle\n", dev);
		//EXP 	ast_mutex_unlock_pvt (pvt);

			readpvterrors(pvt);
			PVT_STAT(pvt,stat_errors[0])++;
			//ast_log (LOG_ERROR, "!!! %s %f\n", PVT_ID(pvt),(float)PVT_STAT(pvt,stat_errors[0]));
			writepvterrors(pvt);
			
			dserial_clearname(pvt);

			goto e_cleanup;
		}

		if(pvt->terminate_monitor)
		{
			ast_log (LOG_NOTICE, "[%s] stopping by %s request\n", dev, dev_state2str(pvt->desired_state));
			goto e_restart;
		}

		t = at_queue_timeout(pvt);
		if(t < 0)
			t = pvt->timeout;

		//EXP if (locked) ast_mutex_unlock_pvt (pvt);

		if (!at_wait (fd, &t))
		{
			locked=(ast_mutex_trylock_pvt(pvt)!=EBUSY);
			//ast_mutex_lock_pvt (pvt);
			if (locked)
			{
				ecmd = at_queue_head_cmd (pvt);
				if(ecmd)
				{
					ast_log (LOG_ERROR, "[%s] timedout while waiting '%s' in response to '%s'\n", dev, at_res2str (ecmd->res), at_cmd2str (ecmd->cmd));
					goto e_cleanup;
				}
				at_enque_ping(&pvt->sys_chan);
				ast_mutex_unlock_pvt (pvt);
			} else {
				ast_verb (3, "[%s] FIX THIS!!! 1 \n", dev);
			}
			continue;
		}

		/* FIXME: access to device not locked */
		iovcnt = at_read (fd, dev, &rb);
		if (iovcnt < 0)
		{
			break;
		}

		PVT_STAT(pvt, d_read_bytes) += iovcnt;
		while ((iovcnt = at_read_result_iov (dev, &read_result, &rb, iov)) > 0)
		{
			at_res = at_read_result_classification (&rb, iov[0].iov_len + iov[1].iov_len);
			PVT_STAT(pvt, at_responces) ++;

			atr=at_response (pvt, iov, iovcnt, at_res);

			if (atr==50) atr=0;
			if (atr==51) atr=0;
			if (atr==401) atr=0;

			if (atr || at_queue_run(pvt))
			{
				ast_verb(3,"[%s] sleep 120 before cleanup\n",PVT_ID(pvt));
				sleep(120);
				ast_verb(3,"[%s] sleep 120 before cleanup ok\n",PVT_ID(pvt));
			        	goto e_cleanup;
			}


/* OLD


				{

				} else
				if (atr==51)  //wait for pin
				{

				} else
				if (atr==401)  //wait for sim
				{

				} else
				    if((!(pvt->nosim==1))&&(!(pvt->cardlock==1))&&(!(pvt->novoice==1)))
				    {
				        if (atr || at_queue_run(pvt)) // atr uzhe kak by 0
				        {
				        	//EXP2
				        	// EXP3 ast_mutex_unlock_pvt (pvt);
				        	goto e_cleanup;
					}
				    }

				}

			// EXP3 	ast_mutex_unlock_pvt (pvt);
			// EXP3 } else {
			// EXP3 	ast_verb (3, "[%s] FIX THIS 2!!! \n", dev);
			// EXP3 }
*/

			
		}
	}

	ast_verb (3, "[%s] FIX THIS!!! 3 HREN`, SUDA VOOBSHE NE DOLZHNO POPADAT\n", dev);
	//ast_mutex_lock_pvt (pvt);

e_cleanup:
	if (!pvt->initialized)
	{
		// TODO: send monitor event
		ast_verb (3, "[%s] Error initializing Dongle\n", dev);
	}
	/* it real, unsolicited disconnect */
	pvt->terminate_monitor = 0;

e_restart:
	disconnect_dongle (pvt);
//	pvt->monitor_running = 0;
	ast_mutex_unlock_pvt (pvt);

	/* TODO: wakeup discovery thread after some delay */
	return NULL;


}

static inline int start_monitor (struct pvt * pvt)
{
	if (ast_pthread_create_background (&pvt->monitor_thread, NULL, do_monitor_phone, pvt) < 0)
	{
		pvt->monitor_thread = AST_PTHREADT_NULL;
		return 0;
	}

	return 1;
}

#/* */
static void pvt_stop(struct pvt * pvt)
{
	pthread_t id;

	if(pvt->monitor_thread != AST_PTHREADT_NULL)
	{
		pvt->terminate_monitor = 1;
		pthread_kill (pvt->monitor_thread, SIGURG);
		id = pvt->monitor_thread;

		ast_mutex_unlock_pvt (pvt);
		pthread_join (id, NULL);
		ast_mutex_lock_pvt (pvt);

		pvt->terminate_monitor = 0;
		pvt->monitor_thread = AST_PTHREADT_NULL;
	}
//	pvt->current_state = DEV_STATE_STOPPED;
}

#/* called with pvt lock hold */
static int pvt_discovery(struct pvt * pvt)
{
	char devname[DEVNAMELEN];
	char imei[IMEI_SIZE+1];
	char imsi[IMSI_SIZE+1];
	char serial[SERIAL_SIZE+1];

	int resolved;
	if(CONF_UNIQ(pvt, data_tty)[0] == 0 && CONF_UNIQ(pvt, audio_tty)[0] == 0) {
		char * data_tty;
		char * audio_tty;

		ast_copy_string(devname, PVT_ID(pvt), sizeof(devname));
		ast_copy_string(imei, CONF_UNIQ(pvt, imei), sizeof(imei));
		ast_copy_string(imsi, CONF_UNIQ(pvt, imsi), sizeof(imsi));
		ast_copy_string(serial, CONF_UNIQ(pvt, serial), sizeof(serial));

		ast_debug(3, "[%s] Trying ports discovery for%s%s%s%s%s%s\n", 
			PVT_ID(pvt), 
			imei[0] == 0 ? "" : " IMEI ", 
			imei, 
			imsi[0] == 0 ? "" : " IMSI ", 
			imsi,
			serial[0] == 0 ? "" : " SERIAL ", 
			serial

			);
		ast_mutex_unlock_pvt (pvt);
//sleep(10); // test
		resolved = pdiscovery_lookup(devname, imei, imsi, serial, &data_tty, &audio_tty);
		ast_mutex_lock_pvt (pvt);
		
		if(resolved) {
			ast_copy_string (PVT_STATE(pvt, data_tty),  data_tty,  sizeof (PVT_STATE(pvt, data_tty)));
			ast_copy_string (PVT_STATE(pvt, audio_tty), audio_tty, sizeof (PVT_STATE(pvt, audio_tty)));

			ast_free(audio_tty);
			ast_free(data_tty);
			ast_verb (3, "[%s]%s%s%s%s%s%s found on data_tty=%s audio_tty=%s\n", 
				PVT_ID(pvt), 
				imei[0] == 0 ? "" : " IMEI ", 
				imei, 
				imsi[0] == 0 ? "" : " IMSI ", 
				imsi,
				serial[0] == 0 ? "" : " SERIAL ", 
				serial,
				PVT_STATE(pvt, data_tty), 
				PVT_STATE(pvt, audio_tty)
				);
		} else {
			ast_debug(3, "[%s] Not found ports for%s%s%s%s%s%s\n", 
				PVT_ID(pvt), 
				imei[0] == 0 ? "" : " IMEI ", 
				imei, 
				imsi[0] == 0 ? "" : " IMSI ", 
				imsi,
				serial[0] == 0 ? "" : " SERIAL ", 
				serial
				
				);
		}
	} else {
		ast_copy_string (PVT_STATE(pvt, data_tty),  CONF_UNIQ(pvt, data_tty), sizeof (PVT_STATE(pvt, data_tty)));
		ast_copy_string (PVT_STATE(pvt, audio_tty), CONF_UNIQ(pvt, audio_tty), sizeof (PVT_STATE(pvt, audio_tty)));
		resolved = 1;
	}
	return ! resolved;
}

#/* */
static void pvt_start(struct pvt * pvt)
{
int r;
	/* prevent start_monitor() multiple times and on turned off devices */
	if (!pvt->connected && pvt->desired_state == DEV_STATE_STARTED)
//	&& (pvt->monitor_thread == AST_PTHREADT_NULL || (pthread_kill(pvt->monitor_thread, 0) != 0 && errno == ESRCH)))
	{
		pvt_stop(pvt);


r=pvt_adiscovery(pvt);
//ast_verb(3,"r=%d",r);
//		if(pvt_adiscovery(pvt))
//			return;

		if (!r) return;
		ast_verb (3, "[%s] Trying to connect on date=%s audio=%s...\n", PVT_ID(pvt), PVT_STATE(pvt, data_tty), PVT_STATE(pvt, audio_tty));

		pvt->data_fd = opentty(PVT_STATE(pvt, data_tty), &pvt->dlock);
		if (pvt->data_fd >= 0)
		{
			*pvt->imei=0;
			*pvt->serial=0;
			*pvt->imsi=0;


			//
			if (pvt->diagmode==1)
			{
			    pvt->diagmode=0;
			    ast_verb (3, "[%s] Sending Diag Mode Command fd=%d\n", PVT_ID(pvt),pvt->data_fd);
			    ttyprog_set_diagmode(pvt->data_fd);
			    ast_verb (3, "[%s] Sending Diag Mode Command OK\n", PVT_ID(pvt));
			    disconnect_dongle(pvt);
			    ast_mutex_unlock_pvt(pvt);

			    //closetty(pvt->data_fd, &pvt->dlock);

			    return;
			}

			if (pvt->changeimei==1)
			{
			    ast_verb (3, "[%s] Trying to connect on audio=%s...\n", PVT_ID(pvt), PVT_STATE(pvt, audio_tty));
			    pvt->audio_fd = opentty(PVT_STATE(pvt, audio_tty), &pvt->alock);
			    pvt->changeimei=0;
			    ast_verb (3, "[%s] Changing imei on fd=%d\n", PVT_ID(pvt),pvt->audio_fd);
			    ttyprog_changeimei(pvt->audio_fd,pvt->newimei);
			    ast_verb (3, "[%s] Changing imei OK\n", PVT_ID(pvt));
			    closetty(pvt->audio_fd, &pvt->alock);

			    disconnect_dongle(pvt);
			    ast_mutex_unlock_pvt(pvt);


			    return;
			}


			// TODO: delay until device activate voice call or at pvt_on_create_1st_channel()

			//AAA
			if(*PVT_STATE(pvt, audio_tty)!=0)
			    pvt->audio_fd = opentty(PVT_STATE(pvt, audio_tty), &pvt->alock);
			else
			    pvt->audio_fd=-1;

			if (pvt->audio_fd >= 0 || *PVT_STATE(pvt, audio_tty)==0)
//			if (pvt->audio_fd >= 0)
			{
				if (start_monitor (pvt))
				{
					pvt->connected = 1;
					pvt->current_state = DEV_STATE_STARTED;
					manager_event_device_status(PVT_ID(pvt), "Connect");
					ast_verb (3, "[%s] Dongle has connected, initializing...\n", PVT_ID(pvt));
					pvt->selectbusy = 0;
					return;
				}
				if(pvt->audio_fd>0)
				closetty(pvt->audio_fd, &pvt->alock);
			}
			closetty(pvt->data_fd, &pvt->dlock);
		}


	}
}

#/* */
static void pvt_free(struct pvt * pvt)
{
	at_queue_flush(pvt);
	if(pvt->dsp)
		ast_dsp_free(pvt->dsp);

	ast_mutex_unlock_pvt(pvt);

	ast_free(pvt);
}

#/* */
static void pvt_destroy(struct pvt * pvt)
{
	ast_mutex_lock_pvt(pvt);
	pvt_stop(pvt);
	pvt_free(pvt);

}

static void * do_discovery(void * arg)
{
	struct public_state * state = (struct public_state *) arg;
	struct pvt * pvt;


/*
	int *crash;



    crash=253134/0;
    *crash=2134/0;

    crash=45134/0;
    *crash=2134/0;
*/

	while(state->unloading_flag == 0)
	{

		sysdevs_find();
		usbdevs_find();

		sysdevs_log("/var/svistok/lists/sysdevs.list");
		usbdevs_log("/var/svistok/lists/usbdevs_all.list",-1);
		usbdevs_log("/var/svistok/lists/usbdevs_active.list",1);
		usbdevs_log("/var/svistok/lists/usbdevs_diag.list",2);
		usbdevs_log("/var/svistok/lists/usbdevs_unknown.list",0);

		usbdevs_filelist("/var/svistok/lists/devices_diag.list",2);
		usbdevs_filelist("/var/svistok/lists/readers.list",1002);

		usbdevs_filelist_2("/usr/simbox/programmator/fuall.sh",2);
		usbdevs_filelist_3("/usr/simbox/programmator/u2diag_all.sh",3);
		usbdevs_filelist_4("/usr/simbox/programmator/unlock_all.sh",1);

	

		/* read lock for avoid deadlock when IMEI/IMSI discovery */
//		AST_RWLIST_RDLOCK(&state->devices);
		
		AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
		{
		if (ast_mutex_trylock_pvt(pvt)!=EBUSY) 
		{

			//ast_mutex_lock_pvt (pvt);
			pvt->must_remove = 0;

			if(pvt->restart_time == RESTATE_TIME_NOW && pvt->desired_state != pvt->current_state)
			{
				switch(pvt->desired_state)
				{
					case DEV_STATE_RESTARTED:
						pvt_stop(pvt);
						/* passthru */
						pvt->desired_state = DEV_STATE_STARTED;
					case DEV_STATE_STARTED:
						pvt_start(pvt);
						break;
					case DEV_STATE_REMOVED:
						pvt_stop(pvt);
						pvt->must_remove = 1;
						break;
					case DEV_STATE_STOPPED:
						pvt_stop(pvt);
				}
			}

			ast_mutex_unlock_pvt (pvt);
			} else ast_verb (3, "  CANNOT TRYLOCK DODISCOVERY %s\n", PVT_ID(pvt));
		}
//		AST_RWLIST_UNLOCK (&state->devices);

		/* actual device removal here for avoid long (discovery) time write lock on device list in loop above */
//		AST_RWLIST_WRLOCK(&state->devices);
		AST_RWLIST_TRAVERSE_SAFE_BEGIN(&state->devices, pvt, entry)
		{
			// expif (ast_mutex_trylock_pvt(pvt)!=EBUSY) 
			// exp{
	
//				ast_mutex_lock_pvt(pvt);
				if(pvt->must_remove)
				{
					AST_RWLIST_REMOVE_CURRENT(entry);
					pvt_free(pvt);
				}
			// exp	} else
			// exp		ast_mutex_unlock_pvt(pvt);
			//} exp else ast_verb (3, "  CANNOT TRYLOCK DODISCOVERY 2 %s\n", PVT_ID(pvt));

		}
		AST_RWLIST_TRAVERSE_SAFE_END;
//		AST_RWLIST_UNLOCK(&state->devices);

		/* Go to sleep (only if we are not unloading) */
		
		make_dongles_imsi_list();
		
		if (state->unloading_flag == 0)
		{
			sleep(SCONF_GLOBAL(state, discovery_interval));
			ast_verb (3, " DISCOVERY SLEEP %d\n", SCONF_GLOBAL(state, discovery_interval));
		} else {
			ast_verb (3, " WARNING! DISCOVERY NOT SLEEP \n");
		}
	}

	return NULL;
}

#/* */
static int discovery_restart(public_state_t * state)
{
	if(state->discovery_thread == AST_PTHREADT_STOP)
		return 0;

	ast_mutex_lock(&state->discovery_lock);
	if (state->discovery_thread == pthread_self()) {
		ast_mutex_unlock(&state->discovery_lock);
		ast_log(LOG_WARNING, "Cannot kill myself\n");
		return -1;
	}
	if (state->discovery_thread != AST_PTHREADT_NULL) {
		/* Wake up the thread */
		pthread_kill(state->discovery_thread, SIGURG);
	} else {
		/* Start a new monitor */
		if (ast_pthread_create_background(&state->discovery_thread, NULL, do_discovery, state) < 0) {
			ast_mutex_unlock(&state->discovery_lock);
			ast_log(LOG_ERROR, "Unable to start discovery thread\n");
			return -1;
		}
	}
	ast_mutex_unlock(&state->discovery_lock);
	return 0;
}

#/* */
static void discovery_stop(public_state_t * state)
{
	/* signal for discovery unloading */
	state->unloading_flag = 1;

	ast_mutex_lock(&state->discovery_lock);
	if (state->discovery_thread && (state->discovery_thread != AST_PTHREADT_STOP) && (state->discovery_thread != AST_PTHREADT_NULL)) {
//		pthread_cancel(state->discovery_thread);
		pthread_kill(state->discovery_thread, SIGURG);
		pthread_join(state->discovery_thread, NULL);
	}

	state->discovery_thread = AST_PTHREADT_STOP;
	ast_mutex_unlock(&state->discovery_lock);
}

#/* */
EXPORT_DEF void pvt_on_create_1st_channel(struct pvt* pvt)
{
	mixb_init (&pvt->a_write_mixb, pvt->a_write_buf, sizeof (pvt->a_write_buf));
//	rb_init (&pvt->a_write_rb, pvt->a_write_buf, sizeof (pvt->a_write_buf));

	if(!pvt->a_timer)
		pvt->a_timer = ast_timer_open ();

/* FIXME: do on each channel switch */
	if(pvt->dsp)
		ast_dsp_digitreset (pvt->dsp);
	pvt->dtmf_digit = 0;
	pvt->dtmf_begin_time.tv_sec = 0;
	pvt->dtmf_begin_time.tv_usec = 0;
	pvt->dtmf_end_time.tv_sec = 0;
	pvt->dtmf_end_time.tv_usec = 0;

	manager_event_device_status(PVT_ID(pvt), "Used");
}

#/* */
EXPORT_DEF void pvt_on_remove_last_channel(struct pvt* pvt)
{
	if (pvt->a_timer)
	{
		ast_timer_close(pvt->a_timer);
		pvt->a_timer = NULL;
	}
	manager_event_device_status(PVT_ID(pvt), "Free");
}

#define SET_BIT(dw_array,bitno)		do { (dw_array)[(bitno) >> 5] |= 1 << ((bitno) & 31) ; } while(0)
#define TEST_BIT(dw_array,bitno)	((dw_array)[(bitno) >> 5] & 1 << ((bitno) & 31))
#/* */
EXPORT_DEF int pvt_get_pseudo_call_idx(const struct pvt * pvt)
{
	struct cpvt * cpvt;
	int * bits;
	int dwords = ((MAX_CALL_IDX + sizeof(*bits) - 1) / sizeof(*bits));

	bits = alloca(dwords * sizeof(*bits));
	memset(bits, 0, dwords * sizeof(*bits));

	AST_LIST_TRAVERSE(&pvt->chans, cpvt, entry) {
		SET_BIT(bits, cpvt->call_idx);
	}

	for(dwords = 1; dwords <= MAX_CALL_IDX; dwords++)
	{
		if(!TEST_BIT(bits, dwords))
			return dwords;
	}
	return 0;
}

#undef TEST_BIT
#undef SET_BIT

#/* */
static int is_dial_possible2(const struct pvt * pvt, int opts, const struct cpvt * ignore_cpvt)
{
	struct cpvt * cpvt;
	int hold = 0;
	int active = 0;
	// FIXME: allow HOLD states for CONFERENCE
	int use_call_waiting = opts & CALL_FLAG_HOLD_OTHER;

	if(pvt->ring || pvt->cwaiting || pvt->dialing)
		return 0;

	AST_LIST_TRAVERSE(&pvt->chans, cpvt, entry) {
		switch(cpvt->state)
		{
			case CALL_STATE_INIT:
				if(cpvt != ignore_cpvt)
					return 0;
				break;

			case CALL_STATE_DIALING:
			case CALL_STATE_ALERTING:
			case CALL_STATE_INCOMING:
			case CALL_STATE_WAITING:
				return 0;

			case CALL_STATE_ACTIVE:
				if(hold || !use_call_waiting)
					return 0;
				active++;
				break;

			case CALL_STATE_ONHOLD:
				if(active || !use_call_waiting)
					return 0;
				hold++;
				break;

			case CALL_STATE_RELEASED:
				;
		}
	}
	return 1;
}

#/* */
EXPORT_DEF int is_dial_possible(const struct pvt * pvt, int opts)
{
	return is_dial_possible2(pvt, opts, NULL);
}

#/* */
EXPORT_DECL int pvt_enabled(const struct pvt * pvt)
{
	return pvt->current_state == DEV_STATE_STARTED && (pvt->desired_state == pvt->current_state || pvt->restart_time == RESTATE_TIME_CONVENIENT);
}

#/* */
EXPORT_DEF int ready4voice_call(const struct pvt* pvt, const struct cpvt * current_cpvt, int opts)
{
	if(!pvt->connected 
		|| 
	   !pvt->initialized
		|| 
	   !pvt->has_voice 
		|| 
	   !pvt->gsm_registered 
		|| 
	   !pvt_enabled(pvt)
		||
	    pvt->outgoing_sms!=0
		||
	    pvt->outgoing_ussd!=0
		||
	    pvt->selectbusy!=0
	)
		return 0;

	return is_dial_possible2(pvt, opts, current_cpvt);
}


#/* */
static int can_dial(struct pvt* pvt, int opts, const struct ast_channel * requestor)
{
	/* not allow hold requester channel :) */
	/* FIXME: requestor may be just proxy/masquerade for real channel */
	//	use ast_bridged_channel(chan) ?
	//	use requestor->tech->get_base_channel() ?

	if((opts & CALL_FLAG_HOLD_OTHER) == CALL_FLAG_HOLD_OTHER && channels_loop(pvt, requestor))
		return 0;
	return ready4voice_call(pvt, NULL, opts);
}


EXPORT_DEF int can_sms(struct pvt* pvt);
EXPORT_DEF int can_sms(struct pvt* pvt)
{

	if(pvt->ring || PVT_STATE(pvt, chan_count[CALL_STATE_INCOMING])) return 0; //state = "Ring";
	if(pvt->cwaiting || PVT_STATE(pvt, chan_count[CALL_STATE_WAITING])) return 0; //state = "Waiting";
	if(pvt->dialing ||
			(PVT_STATE(pvt, chan_count[CALL_STATE_INIT])
				+
				PVT_STATE(pvt, chan_count[CALL_STATE_DIALING])
				+
				PVT_STATE(pvt, chan_count[CALL_STATE_ALERTING])) > 0)
			return 0; //state = "Dialing";

	if(PVT_STATE(pvt, chan_count[CALL_STATE_ACTIVE]) > 0) return 0; //state = "Active";
	if(PVT_STATE(pvt, chan_count[CALL_STATE_ONHOLD]) > 0) return 0; //state = "Held";

	return 1;
}


#/* return locked pvt or NULL */
EXPORT_DEF struct pvt * find_device_ex(struct public_state * state, const char * name)
{
	struct pvt * pvt;

	//AST_RWLIST_RDLOCK(&state->devices);
	AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
	{
		if (!strcmp (PVT_ID(pvt), name))
		{
		    if(pvt->connected!=0)
		    {
			//ast_mutex_lock_pvt (pvt);
			break;
		    }
		}
//		ast_mutex_unlock_pvt (pvt);
	}
	//AST_RWLIST_UNLOCK(&state->devices);

	return pvt;
}

#/* return locked pvt or NULL */
EXPORT_DEF struct pvt * find_device_ext (const char * name, const char ** reason)
{
	int locked;
	char * res = "";
	struct pvt * pvt = find_device(name);


	if(pvt)
	{

		if(!pvt_enabled(pvt))
		{
//			ast_mutex_unlock_pvt (pvt);
			res = "device disabled";
			pvt = NULL;
		} else {
			locked=(ast_mutex_trylock_pvt(pvt)!=EBUSY);
			//ast_mutex_lock_pvt (pvt);
			if(!locked)
			{
				res = "device locked! Please try again later";
				pvt = NULL;
			} else {
			    pvt->selectbusy=1;
			    ast_mutex_unlock_pvt (pvt);
			}
		}
	}
	else
		res = "no such device";
	if(reason)
		*reason = res;
	return pvt;
}









void ast_channel_show_vars(const struct ast_channel *parent)
{
        struct ast_var_t *current;
        const char *varname;

        AST_LIST_TRAVERSE(ast_channel_varshead(parent), current, entries) {
//                int vartype = 0;

                varname = ast_var_full_name(current);
                if (!varname)
                        continue;

	    ast_verb(3, "%s=%s\n",varname, ast_var_value(current));
        }
}

void ast_channel_get_var(const struct ast_channel *parent, char *varname1, char *value)
{
        struct ast_var_t *current;
        const char *varname;
	strcpy(value,"");
        AST_LIST_TRAVERSE(ast_channel_varshead(parent), current, entries) {
//                int vartype = 0;

                varname = ast_var_full_name(current);
                if (!varname)
                        continue;
		if(strcmp(varname,varname1)==0) {strcpy(value, ast_var_value(current));}
//	    ast_verb(3, "%s=%s\n",varname, ast_var_value(current));
        }
}

#include "select.c"







#/* */
static const char * pvt_state_base(const struct pvt * pvt)
{
	const char * state = NULL;
// length is "AAAAAAAAAA"
	if(pvt->current_state == DEV_STATE_STOPPED && pvt->desired_state == DEV_STATE_STOPPED)
		state = "Stopped";
	else if(pvt->eerror)
		state = "ERROR";

	else if((pvt->cfun==5)&&(pvt->pinrequired))
		state = "Ofline,PIN";

	else if(pvt->cfun==5)
		state = "Offline";

	else if(pvt->cfun==4)
		state = "SIM removed";

	else if(pvt->cfun==6)
		state = "Restarting";

	else if(pvt->nosim)
		state = "NO SIM";
	else if(pvt->cardlock)
		state = "CARDLOCK";
	else if(pvt->pinrequired)
		state = "PIN required";
	else if(pvt->novoice)
		state = "NOVOICE";
	else if(!pvt->connected)
		state = "Not connected";

	else if(pvt->sim_ready==0)
		state = "SIM not ready";

//	else if((!pvt->initialized)&&(pvt->sim_ready==0))
//		state = "SIM not ready";
	else if(!pvt->initialized) // => sim_ready==1
		state = "Not initialized";
	else if(!pvt->gsm_registered)
		state = "GSM not registered";
	return state;
}


#/* */
EXPORT_DEF const char* pvt_str_state(const struct pvt* pvt)
{
	const char * state = pvt_state_base(pvt);
	if(!state) {
		if(pvt->ring || PVT_STATE(pvt, chan_count[CALL_STATE_INCOMING]))
			state = "Ring";
		else if(pvt->cwaiting || PVT_STATE(pvt, chan_count[CALL_STATE_WAITING]))
			state = "Waiting";
		else if(pvt->dialing ||
			(PVT_STATE(pvt, chan_count[CALL_STATE_INIT])
				+
				PVT_STATE(pvt, chan_count[CALL_STATE_DIALING])
				+
				PVT_STATE(pvt, chan_count[CALL_STATE_ALERTING])) > 0)
			state = "Dialing";

		else if(PVT_STATE(pvt, chan_count[CALL_STATE_ACTIVE]) > 0)
//			state = "Active";
			state = pvt_call_dir(pvt);
		else if(PVT_STATE(pvt, chan_count[CALL_STATE_ONHOLD]) > 0)
			state = "Held";
		else if(pvt->outgoing_sms)
			state = "SMS_OUT";
		else if(pvt->outgoing_sms)
			state = "USSD_OUT";
		else if(pvt->incoming_sms)
			state = "SMS_IN";

		else
			state = "Free";
	}
	return state;
}

#/* */
EXPORT_DEF struct ast_str* pvt_str_state_ex(const struct pvt* pvt)
{
	struct ast_str* buf = ast_str_create (256);
	const char * state = pvt_state_base(pvt);

	if(state)
		ast_str_append (&buf, 0, "%s", state);
	else
	{
		if(pvt->ring || PVT_STATE(pvt, chan_count[CALL_STATE_INCOMING]))
			ast_str_append (&buf, 0, "Ring ");

		if(pvt->dialing ||
			(PVT_STATE(pvt, chan_count[CALL_STATE_INIT])
				+
			PVT_STATE(pvt, chan_count[CALL_STATE_DIALING])
				+
			PVT_STATE(pvt, chan_count[CALL_STATE_ALERTING])) > 0)
			ast_str_append (&buf, 0, "Dialing ");

		if(pvt->cwaiting || PVT_STATE(pvt, chan_count[CALL_STATE_WAITING]))
			ast_str_append (&buf, 0, "Waiting ");

		if(PVT_STATE(pvt, chan_count[CALL_STATE_ACTIVE]) > 0)
			ast_str_append (&buf, 0, "Active %u ", PVT_STATE(pvt, chan_count[CALL_STATE_ACTIVE]));

		if(PVT_STATE(pvt, chan_count[CALL_STATE_ONHOLD]) > 0)
			ast_str_append (&buf, 0, "Held %u ", PVT_STATE(pvt, chan_count[CALL_STATE_ONHOLD]));

		if(pvt->incoming_sms)
			ast_str_append (&buf, 0, "Incoming SMS ");

		if(pvt->outgoing_sms)
			ast_str_append (&buf, 0, "Outgoing SMS");

		if(pvt->outgoing_ussd)
			ast_str_append (&buf, 0, "Outgoing USSD");

		if(ast_str_strlen(buf) == 0)
		{
			ast_str_append (&buf, 0, "%s", "Free");
		}
	}

	if(pvt->desired_state != pvt->current_state)
		ast_str_append (&buf, 0, " %s", dev_state2str_msg(pvt->desired_state));

	return buf;
}

#/* */
EXPORT_DEF const char* GSM_regstate2str(int gsm_reg_status)
{
	static const char * const gsm_states[] = {
		"Not registered, not searching",
		"Registered, home network",
		"Not registered, but searching",
		"Registration denied",
		"Unknown",
		"Registered, roaming",
		};
	return enum2str_def(gsm_reg_status, gsm_states, ITEMS_OF (gsm_states), "Unknown");
}

#/* */
EXPORT_DEF const char* sys_mode2str(int sys_mode)
{
	static const char * const sys_modes[] = {
		"No Service",
		"AMPS",
		"CDMA",
		"GSM/GPRS",
		"HDR",
		"WCDMA",
		"GPS",
		};

	return enum2str_def(sys_mode, sys_modes, ITEMS_OF (sys_modes), "Unknown");
}

#/* */
EXPORT_DEF const char * sys_submode2str(int sys_submode)
{
	static const char * const sys_submodes[] = {
		"No service",
		"GSM",
		"GPRS",
		"EDGE",
		"WCDMA",
		"HSDPA",
		"HSUPA",
		"HSDPA and HSUPA",
		};

	return enum2str_def(sys_submode, sys_submodes, ITEMS_OF (sys_submodes), "Unknown");
}

#/* */
EXPORT_DEF char* rssi2dBm(int rssi, char * buf, unsigned len)
{
	if(rssi <= 0)
	{
		snprintf(buf, len, "<= -125 dBm");
	}
	else if(rssi <= 30)
	{
		snprintf(buf, len, "%d dBm", 31 * rssi / 50 - 125);
	}
	else if(rssi == 31)
	{
		snprintf(buf, len, ">= -75 dBm");
	}
	else
	{
		snprintf(buf, len, "unknown");
	}
	return buf;
}


/* Module */

#/* */
static void pvt_dsp_setup(struct pvt * pvt, const pvt_config_t * settings)
{
	/* first remove dsp if off or changed */
	if(SCONFIG(settings, dtmf) != CONF_SHARED(pvt, dtmf))
	{
		if(pvt->dsp)
		{
			ast_dsp_free(pvt->dsp);
			pvt->dsp = NULL;
		}
	}

	/* wake up and initialize dsp */
	if(SCONFIG(settings, dtmf) != DC_DTMF_SETTING_OFF)
	{
		pvt->dsp = ast_dsp_new();
		if(pvt->dsp)
		{
			int digitmode = DSP_DIGITMODE_DTMF;
			if(SCONFIG(settings, dtmf) == DC_DTMF_SETTING_RELAX)
				digitmode |= DSP_DIGITMODE_RELAXDTMF;

			//ast_dsp_set_features(pvt->dsp, DSP_FEATURE_DIGIT_DETECT); //!! original
			//ast_dsp_set_features(pvt->dsp, DSP_FEATURE_DIGIT_DETECT | DSP_FEATURE_SILENCE_SUPPRESS | DSP_FEATURE_BUSY_DETECT | DSP_FEATURE_FAX_DETECT | DSP_FEATURE_CALL_PROGRESS); //!! was only DSP_FEATURE_DIGIT_DETECT
			// DSP_FEATURE_SILENCE_SUPPRESS |
			//ast_dsp_set_features(pvt->dsp, DSP_FEATURE_DIGIT_DETECT | DSP_FEATURE_CALL_PROGRESS | DSP_PROGRESS_TALK | DSP_PROGRESS_RINGING ); //!! test
			ast_dsp_set_features(pvt->dsp, DSP_FEATURE_DIGIT_DETECT | DSP_PROGRESS_TALK | DSP_PROGRESS_RINGING ); //!! test

			ast_dsp_set_digitmode(pvt->dsp, digitmode);
			//ast_dsp_set_faxmode(pvt->dsp, DSP_FAXMODE_DETECT_ALL);

                        ast_dsp_set_call_progress_zone(pvt->dsp, "cr");
                        //ast_dsp_set_busy_count(pvt->dsp, 3);

		}
		else
		{
			ast_log(LOG_ERROR, "[%s] Can't setup dsp for dtmf detection, ignored\n", UCONFIG(settings, id));
		}
	}
}

static struct pvt * pvt_create(const pvt_config_t * settings)
{
	struct pvt * pvt = ast_calloc (1, sizeof (*pvt));
	if(pvt)
	{
		ast_mutex_init (&pvt->lock);

		AST_LIST_HEAD_INIT_NOLOCK (&pvt->at_queue);
		AST_LIST_HEAD_INIT_NOLOCK (&pvt->chans);
		pvt->sys_chan.pvt = pvt;
		pvt->sys_chan.state = CALL_STATE_RELEASED;

		pvt->monitor_thread		= AST_PTHREADT_NULL;
		pvt->audio_fd			= -1;
		pvt->data_fd			= -1;
		pvt->timeout			= DATA_READ_TIMEOUT;
		pvt->cusd_use_ucs2_decoding	=  1;
		pvt->gsm_reg_status		= -1;


		pvt->cfun		= -1;
		pvt->simst		= -1;
		pvt->srvst		= -1;
		pvt->pinrequired	= 0;

		writepvtstate(pvt);

		pvt->sim_ready=0;
		pvt->sim_start=0;


		ast_copy_string (pvt->provider_name, "NONE", sizeof (pvt->provider_name));
		ast_copy_string (pvt->provider_name2, "", sizeof (pvt->provider_name2));
		ast_copy_string (pvt->subscriber_number, "Unknown", sizeof (pvt->subscriber_number));
		pvt->has_subscriber_number = 0;

		pvt->desired_state = SCONFIG(settings, initstate);

		pvt->soupri_count=0;

		pvt_dsp_setup(pvt, settings);

		/* and copy settings */
		memcpy(&pvt->settings, settings, sizeof(pvt->settings));
		return pvt;
	}
	else
	{
		ast_log (LOG_ERROR, "[%s] Skipping device: Error allocating memory\n", UCONFIG(settings, id));
	}
	return NULL;
}

/*
static struct pvt * pvt_create_new(char* donglename, char* data_tty, char* audio_tty)
{
	struct pvt * pvt = ast_calloc (1, sizeof (*pvt));
	if(pvt)
	{
		ast_mutex_init (&pvt->lock);

		AST_LIST_HEAD_INIT_NOLOCK (&pvt->at_queue);
		AST_LIST_HEAD_INIT_NOLOCK (&pvt->chans);
		pvt->sys_chan.pvt = pvt;
		pvt->sys_chan.state = CALL_STATE_RELEASED;

		pvt->monitor_thread		= AST_PTHREADT_NULL;
		pvt->audio_fd			= -1;
		pvt->data_fd			= -1;
		pvt->timeout			= DATA_READ_TIMEOUT;
		pvt->cusd_use_ucs2_decoding	=  1;
		pvt->gsm_reg_status		= -1;

		pvt->cfun		= -1;
		pvt->simst		= -1;
		pvt->srvst		= -1;

		ast_copy_string (pvt->provider_name, "NONE", sizeof (pvt->provider_name));
		ast_copy_string (pvt->provider_name2, "", sizeof (pvt->provider_name2));
		ast_copy_string (pvt->subscriber_number, "Unknown", sizeof (pvt->subscriber_number));
		pvt->has_subscriber_number = 0;

		//!!
		pvt->desired_state = DEV_STATE_STARTED;//SCONFIG(settings, initstate);

		pvt_dsp_setup(pvt, settings);

		//memcpy(&pvt->settings, settings, sizeof(pvt->settings));
		return pvt;
	}
	else
	{
		ast_log (LOG_ERROR, "[%s] Skipping device: Error allocating memory\n", UCONFIG(settings, id));
	}
	return NULL;
}
*/

#/* */
static int pvt_time4restate(const struct pvt * pvt)
{
	if(pvt->desired_state != pvt->current_state)
	{
		if(pvt->restart_time == RESTATE_TIME_NOW || (PVT_NO_CHANS(pvt) && !pvt->outgoing_sms && !pvt->incoming_sms && !pvt->outgoing_ussd))
			return 1;
	}
	return 0;
}

#/* */
EXPORT_DEF void pvt_try_restate(struct pvt * pvt)
{
	if(pvt_time4restate(pvt))
	{
		pvt->restart_time = RESTATE_TIME_NOW;
		discovery_restart(gpublic);
	}
}

#/* assume caller hold lock */
static int pvt_reconfigure(struct pvt * pvt, const pvt_config_t * settings, restate_time_t when)
{
	int rv = 0;

	if(SCONFIG(settings, initstate) == DEV_STATE_REMOVED)
	{
		/* handle later, in one place */
		pvt->must_remove = 1;
	}
	else
	{
		/* check what changes require starting or stopping */
		if(pvt->desired_state != SCONFIG(settings, initstate)) {
			pvt->desired_state = SCONFIG(settings, initstate);
			
			rv = pvt_time4restate(pvt);
			pvt->restart_time = rv ? RESTATE_TIME_NOW : when;
		}

		/* check what config changes require restaring */
		else if(
		   strcmp(UCONFIG(settings, audio_tty), CONF_UNIQ(pvt, audio_tty))
			||
		   strcmp(UCONFIG(settings, data_tty), CONF_UNIQ(pvt, data_tty))
			||
		   strcmp(UCONFIG(settings, imei), CONF_UNIQ(pvt, imei))
			||
		   strcmp(UCONFIG(settings, imsi), CONF_UNIQ(pvt, imsi))
			||
		   strcmp(UCONFIG(settings, serial), CONF_UNIQ(pvt, serial))
			||
		   SCONFIG(settings, u2diag) != CONF_SHARED(pvt, u2diag)
			||
		   SCONFIG(settings, resetdongle) != CONF_SHARED(pvt, resetdongle)
			||
		   SCONFIG(settings, smsaspdu) != CONF_SHARED(pvt, smsaspdu)
			||
		   SCONFIG(settings, callwaiting) != CONF_SHARED(pvt, callwaiting)
		   )
		{
			/* TODO: schedule restart */
			pvt->desired_state = DEV_STATE_RESTARTED;

			rv = pvt_time4restate(pvt);
			pvt->restart_time = rv ? RESTATE_TIME_NOW : when;
		}

		pvt_dsp_setup(pvt, settings);

		/* and copy settings */
		memcpy(&pvt->settings, settings, sizeof(pvt->settings));
	}
	return rv;
}

#/* */
static int reload_config(public_state_t * state, int recofigure, restate_time_t when, unsigned * reload_immediality)
{
	struct ast_config * cfg;
	const char * cat;
	struct ast_flags config_flags = { 0 };
	struct dc_sconfig config_defaults;
	pvt_config_t settings;
	int err;
	struct pvt * pvt;
	unsigned reload_now = 0;

	if ((cfg = ast_config_load (CONFIG_FILE, config_flags)) == NULL)
	{
		return -1;
	}

	/* read global config */
	dc_gconfig_fill(cfg, "general", &state->global_settings);

	/* read defaults */
	dc_sconfig_fill_defaults(&config_defaults);
	dc_sconfig_fill(cfg, "defaults", &config_defaults);

	/* FIXME: deadlock avoid ? */
	AST_RWLIST_RDLOCK(&state->devices);
	AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
	{
		ast_mutex_lock_pvt(pvt);
		pvt->must_remove = 1;
		ast_mutex_unlock_pvt(pvt);
	}
	AST_RWLIST_UNLOCK(&state->devices);

	/* now load devices */

	/* dinamicheskiy zapusk

	for (cat = ast_category_browse (cfg, NULL); cat; cat = ast_category_browse (cfg, cat))
	{
		if (strcasecmp (cat, "general") && strcasecmp (cat, "defaults"))
		{
			err = dc_config_fill(cfg, cat, &config_defaults, &settings);
			err = dc_config_fill(cfg, cat, &config_defaults, &def_settings);
			if(!err)
			{
				pvt = find_device(UCONFIG(&settings, id));
				if(pvt)
				{
					if(!recofigure)
					{
						ast_log (LOG_ERROR, "device %s already exists, duplicate in config file\n", cat);
					}
					else
					{
						pvt->must_remove = 0;
						reload_now += pvt_reconfigure(pvt, &settings, when);
					}
					ast_mutex_unlock_pvt(pvt);
				}
				else
				{
					// new devices
					if(SCONFIG(&settings, initstate) == DEV_STATE_REMOVED)
					{
						ast_log (LOG_NOTICE, "Skipping device %s as disabled\n", cat);
					}
					else
					{
						pvt = pvt_create(&settings);
						if(pvt)
						{
							// FIXME: deadlock avoid
							AST_RWLIST_WRLOCK(&state->devices);
							AST_RWLIST_INSERT_TAIL(&state->devices, pvt, entry);
							AST_RWLIST_UNLOCK(&state->devices);
							reload_now++;

							ast_log (LOG_NOTICE, "[%s] Loaded device\n", PVT_ID(pvt));
						}
					}
				}
			}
		}
	}
	*/
	ast_config_destroy (cfg);

	/* FIXME: deadlock avoid ? */
	/* schedule removal of devices not listed in config file or disabled */
	AST_RWLIST_RDLOCK(&state->devices);
	AST_RWLIST_TRAVERSE(&state->devices, pvt, entry)
	{
		ast_mutex_lock_pvt(pvt);
		if(pvt->must_remove)
		{
			pvt->desired_state = DEV_STATE_REMOVED;
			if(pvt_time4restate(pvt))
			{
				pvt->restart_time = RESTATE_TIME_NOW;
				reload_now++;
			}
			else
				pvt->restart_time = when;
		}
		ast_mutex_unlock_pvt(pvt);
	}
	AST_RWLIST_UNLOCK (&state->devices);

	if(reload_immediality)
		*reload_immediality = reload_now;
	return 0;
}


#/* */
static void devices_destroy(public_state_t * state)
{
	struct pvt * pvt;

	/* Destroy the device list */
	AST_RWLIST_WRLOCK(&state->devices);
	while((pvt = AST_RWLIST_REMOVE_HEAD(&state->devices, entry)))
	{
		pvt_destroy(pvt);
	}
	AST_RWLIST_UNLOCK(&state->devices);
}


static int load_module()
{
	int rv;
	dserial_init();
	clear_state();
	putfiles("","svistok","version",svistok_version);
	IAXME_get();

	gpublic = ast_calloc(1, sizeof(*gpublic));
	if(gpublic)
	{
		pdiscovery_init();
		rv = public_state_init(gpublic);
		if(rv != AST_MODULE_LOAD_SUCCESS)
			ast_free(gpublic);
	}
	else
	{
		ast_log (LOG_ERROR, "Unable to allocate global state structure\n");
		rv = AST_MODULE_LOAD_DECLINE;
	}
	return rv;
}

#/* */
static int public_state_init(struct public_state * state)
{
	int rv = AST_MODULE_LOAD_DECLINE;
	
	AST_RWLIST_HEAD_INIT(&state->devices);
	ast_mutex_init(&state->discovery_lock);

	state->discovery_thread = AST_PTHREADT_NULL;
	ast_mutex_init(&state->round_robin_mtx);

	if(reload_config(state, 0, RESTATE_TIME_NOW, NULL) == 0)
	{
		rv = AST_MODULE_LOAD_FAILURE;
		if(discovery_restart(state) == 0)
		{
		        ast_format_set(&chan_dongle_format, AST_FORMAT_SLINEAR, 0);
		        if (!(channel_tech.capabilities = ast_format_cap_alloc())) {
                		return AST_MODULE_LOAD_FAILURE;
		        }
		        ast_format_cap_add(channel_tech.capabilities, &chan_dongle_format);
			chan_dongle_format_cap = channel_tech.capabilities;
			/* register our channel type */
			if(ast_channel_register(&channel_tech) == 0)
			{
				cli_register();

				app_register();
				manager_register();

				readglsettings();

				return AST_MODULE_LOAD_SUCCESS;
			}
			else
			{
				channel_tech.capabilities = ast_format_cap_destroy(channel_tech.capabilities);
				ast_log (LOG_ERROR, "Unable to register channel class %s\n", channel_tech.type);
			}
			discovery_stop(state);
		}
		else
		{
			ast_log (LOG_ERROR, "Unable to create discovery thread\n");
		}
		devices_destroy(state);
	}
	else
	{
		ast_log (LOG_ERROR, "Errors reading config file " CONFIG_FILE ", Not loading module\n");
	}

	ast_mutex_destroy(&state->round_robin_mtx);
	ast_mutex_destroy(&state->discovery_lock);
	AST_RWLIST_HEAD_DESTROY(&state->devices);

	return rv;
}

#/* */
static void public_state_fini(struct public_state * state)
{
	/* First, take us out of the channel loop */
	ast_channel_unregister (&channel_tech);
	channel_tech.capabilities = ast_format_cap_destroy(channel_tech.capabilities);

	/* Unregister the CLI & APP & MANAGER */

	manager_unregister();

	app_unregister();

	cli_unregister();

	discovery_stop(state);
	devices_destroy(state);
	
	ast_mutex_destroy(&state->round_robin_mtx);
	ast_mutex_destroy(&state->discovery_lock);
	AST_RWLIST_HEAD_DESTROY(&state->devices);
}

static int unload_module()
{

	public_state_fini(gpublic);
	pdiscovery_fini();
	
	ast_free(gpublic);
	gpublic = NULL;
	return 0;
}


#/* */
EXPORT_DEF void pvt_reload(restate_time_t when)
{
	unsigned dev_reload = 0;
	reload_config(gpublic, 1, when, &dev_reload);
	if(dev_reload > 0)
		discovery_restart(gpublic);
}

#/* */
static int reload_module()
{
	pvt_reload(RESTATE_TIME_GRACEFULLY);
	return 0;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, MODULE_DESCRIPTION,
		.load = load_module,
		.unload = unload_module,
		.reload = reload_module,
	       );

//AST_MODULE_INFO_STANDARD (ASTERISK_GPL_KEY, MODULE_DESCRIPTION);

EXPORT_DEF struct ast_module* self_module()
{
	return ast_module_info->self;
}







