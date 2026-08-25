/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/
#ifndef CHAN_DONGLE_H_INCLUDED
#define CHAN_DONGLE_H_INCLUDED
#ifdef SVISTOK_COMPOSED_CHAN_DONGLE_H_HEADER
#include SVISTOK_COMPOSED_CHAN_DONGLE_H_HEADER
#else

#include <asterisk.h>
#include <asterisk/lock.h>
#include <asterisk/linkedlists.h>


//include "share.h"
#include <asterisk-chan-dongle/mixbuffer.h>	/* struct mixbuffer */
//#include "ringbuffer.h"				/* struct ringbuffer */
#include "cpvt.h"				/* struct cpvt */
#include <asterisk-chan-dongle/export.h>	/* EXPORT_DECL EXPORT_DEF */
#include "dc_config.h"				/* pvt_config_t */

#include "select.h"

/* SVISTOK_BASELINE_UNIT macro MODULE_DESCRIPTION */
/*define MAXDONGLEDEVICES	512*/
#define MAXDONGLEDEVICES	256

#define ACDL	10
#define ASRL	20
#define PDDL	20

#define ACDL_BS	50
#define ASRL_BS	50
#define PDDL_BS	50


#define ACDLINIT	120
#define ASRLINIT	50000
#define PDDLINIT	10000


#define STAT_SESSION	0
#define STAT_DONGLE	1
#define STAT_IMSI	2

extern int nosim2offline;


/* SVISTOK_BASELINE_UNIT function dev_state2str */

/* SVISTOK_BASELINE_UNIT function dev_state2str_msg */

/* Only linear is allowed */
EXPORT_DECL struct ast_format chan_dongle_format;
EXPORT_DECL struct ast_format_cap * chan_dongle_format_cap;
/* SVISTOK_BASELINE_UNIT typedef restate_time_t */

/* state */
typedef struct pvt_state
{
	char			audio_tty[DEVPATHLEN];		/*!< tty for audio connection */
	char			data_tty[DEVPATHLEN];		/*!< tty for AT commands */
	char			net[DEVPATHLEN];
	char			dev[DEVPATHLEN];

	uint32_t		at_tasks;			/*!< number of active tasks in at_queue */
	uint32_t		at_cmds;			/*!< number of active commands in at_queue */
	uint32_t		chansno;			/*!< number of channels in channels list */
	uint8_t			chan_count[CALL_STATES_NUMBER];	/*!< channel number grouped by state */
} pvt_state_t;

/* SVISTOK_BASELINE_UNIT macro PVT_STATE_T */

/* statictics */
typedef struct pvt_stat
{
	uint32_t		at_tasks;			/*!< number of tasks added to queue */
	uint32_t		at_cmds;			/*!< number of commands added to queue */
	uint32_t		at_responces;			/*!< number of responses handled */

	uint32_t		d_read_bytes;			/*!< number of bytes of commands actually readed from device */
	uint32_t		d_write_bytes;			/*!< number of bytes of commands actually written to device */

	uint64_t		a_read_bytes;			/*!< number of bytes of audio readed from device */
	uint64_t		a_write_bytes;			/*!< number of bytes of audio written to device */

	uint32_t		read_frames;			/*!< number of frames readed from device */
	uint32_t		read_sframes;			/*!< number of truncated frames readed from device */

	uint32_t		write_frames;			/*!< number of tries to frame write */
	uint32_t		write_tframes;			/*!< number of truncated frames to write */
	uint32_t		write_sframes;			/*!< number of silence frames to write */

	uint64_t		write_rb_overflow_bytes;	/*!< number of overflow bytes */
	uint32_t		write_rb_overflow;		/*!< number of times when a_write_rb overflowed */

	uint32_t		in_calls;			/*!< number of incoming calls not including waiting */
	uint32_t		cw_calls;			/*!< number of waiting calls */
	uint32_t		out_calls;			/*!< number of all outgoing calls attempts */
	uint32_t		in_calls_handled;		/*!< number of ncoming/waiting calls passed to dialplan */
	uint32_t		in_pbx_fails;			/*!< number of start_pbx fails */

	uint32_t		calls_answered[2];		/*!< number of outgoing and incoming/waiting calls answered */
	uint32_t		calls_duration[2];		/*!< seconds of outgoing and incoming/waiting calls */

	uint32_t		stat_calls_answered[3];
	uint32_t		stat_calls_duration[3];


	uint32_t		stat_in_answered;
	uint32_t		stat_in_duration;
	
	uint32_t		stat_out_calls[3];
	uint32_t		stat_wait_duration[3];		/* Длительность ожиданий для PDD    */
	uint32_t		stat_acdl[3];			/* Последние ACD для ACDL звонков   */
	uint32_t		stat_asrl[3];			/* Последние ACD для ACDL звонков   */
	uint32_t		stat_pddl[2][3];			/* Последние PDD для PDDL звонков   */
						// 0 - bez otveta, 1 - s otvetom
	uint32_t		stat_datt[3];			/* Когда последний раз дозванивался */
	uint32_t		stat_iatt;			/* Количество подряд исходящих */
	uint32_t		stat_satt;			/* Количество подряд исходящих без смс */

	uint32_t		limitnum; //Tekushiy limit
	uint32_t		limittype; 
	
	char		billing_direction[3];
	int		billing_pay;  // 0 - sou 1-pay
	
	int		limit[10];

	int		limit_soft[10];
	int		limit_hard[10];

	int		alg[10];
	int		nodiff[10];

	long int stat_errors[3];
	
	char number[16];
	char balance[64];
	char ballast[64];
	
	char tarif[64];
	

	long int stat_call_start;     //dial
	long int stat_call_response;
	long int stat_call_process;   // когда записано

	long int stat_call_pddc;
	long int stat_call_fas;

	long int stat_call_connected; //answer
	long int stat_call_end;
	long int stat_call_saved;


/*	long int epoch_dial;
	long int epoch_connect;
	long int epoch_pddc;
	long int epoch_fas;
	long int epoch_answer;
	long int epoch_end;*/


	
	long int stat_call_sf;

	
	int priority;

	int diff_min;

	int diff_min_out;

	int diff_min_vip;
	int diff_min_goo;
	int diff_min_nor;

	int diff_min_sout;
	int diff_min_imode;

	int in_imode;
	
	int active;
	
	int igoo,ibad,inor,inew,inos,imode;
	int ine0,inec,inem;

	int iblo,irob,ivip;
	int notvip;
	int ima,imb,imc,imd,ime,imn;
	int can_in,can_out,can_sout;
	int capnew,capfail,capok;
	
} pvt_stat_t;

extern uint32_t		total_stat_acdl;			/* Последние ACD для ACDL звонков   */
extern uint32_t		total_stat_pddl[2];			/* Последние PDD для PDDL звонков   */
						// 0 - bez otveta, 1 - s otvetom
extern uint32_t		total_stat_datt;			/* Когда последний раз дозванивался */


/* SVISTOK_BASELINE_UNIT macro PVT_STAT_T */

/* SVISTOK_BASELINE_UNIT record at_queue_task */

typedef struct soupri
{
    char imsi[17];
    long sou_diff_start;
} soupri_t;

typedef struct pvt
{
	AST_LIST_ENTRY (pvt)	entry;				/*!< linked list pointers */

	ast_mutex_t		lock;				/*!< pvt lock */
	long			lock_start;

	AST_LIST_HEAD_NOLOCK (, at_queue_task) at_queue;	/*!< queue for commands to modem */

	AST_LIST_HEAD_NOLOCK (, cpvt)		chans;		/*!< list of channels */
	struct cpvt		sys_chan;			/*!< system channel */
	struct cpvt		*last_dialed_cpvt;		/*!< channel what last call successfully set ATDnum; leave until ^ORIG received; need because real call idx of dialing call unknown until ^ORIG */

	pthread_t		monitor_thread;			/*!< monitor (at commands reader) thread handle */

	int			audio_fd;			/*!< audio descriptor */
	int			data_fd;			/*!< data descriptor */
	char			* alock;			/*!< name of lockfile for audio */
	char			* dlock;			/*!< name of lockfile for data */

	struct ast_dsp*		dsp;				/*!< silence/DTMF detector */
	struct ast_timer*	a_timer;			/*!< audio write timer */

	char			a_write_buf[FRAME_SIZE * 5];	/*!< audio write buffer */
	struct mixbuffer	a_write_mixb;			/*!< audio mix buffer */
//	struct ringbuffer	a_write_rb;			/*!< audio ring buffer */

//	char			a_read_buf[FRAME_SIZE + AST_FRIENDLY_OFFSET];	/*!< audio read buffer */
//	struct ast_frame	a_read_frame;			/*!< readed frame buffer */

	
	char			dtmf_digit;			/*!< last DTMF digit */
	struct timeval		dtmf_begin_time;		/*!< time of begin of last DTMF digit */
	struct timeval		dtmf_end_time;			/*!< time of end of last DTMF digit */

	int			timeout;			/*!< used to set the timeout for data */
/* SVISTOK_BASELINE_UNIT macro DATA_READ_TIMEOUT */

	unsigned long		channel_instanse;		/*!< number of channels created on this device */
	unsigned int		rings;				/*!< ring/ccwa  number distributed to at_response_clcc() */

	/* device caps */
	unsigned int		use_ucs2_encoding:1;
	unsigned int		cusd_use_7bit_encoding:1;
	unsigned int		cusd_use_ucs2_decoding:1;

	/* device state */
	int			gsm_reg_status;
	int			rssi;
	int			srna;
	int			srnb;
	int			linkmode;
	int			linksubmode;
	char			provider_name[32];
	char			provider_name2[32];
	char			manufacturer[32];
	char			model[32];
	char			firmware[32];
	char			imei[17];
	char			imsi[17];
	char			iccid[64];
	int			freqlock;
	
	char			numbera[64];
	char			numberb[64];
	char			numberb_before[64];

	char			spec[64];
	char			qos[64];
	char			vip[64];
	char			naprstr[1024];
	char			im[1024];
	char			uid[1024];

	char			serial[SERIAL_SIZE+2];
	char			subscriber_number[128];
	char			location_area_code[8];
	char			cell_id[8];
	char			sms_scenter[20];

	unsigned int		pinrequired; // Vvod pina
	unsigned int		nosim; // NOSIM
	unsigned int		eerror; 
	unsigned int		cardlock; // cardlock
	unsigned int		novoice; //


	unsigned int		sim_ready;
	unsigned int		sim_start;




	int 			diagmode;
	int 			changeimei;
	char			newimei[64];

	int ima_count;
	int imb_count;

	char pro[256];
	char procur[256];
	char capcur[256];

	int cfun;
	int simst;
	int srvst;

	int em_type;



	int fas;
	int epdd;
	int fpdd;
	int hem;
	int hoa;

	int dsp_state;

	int time_work_wake;
	int time_work_sleep;

	int time_holiday_wake;
	int time_holiday_sleep;
	
	struct soupri soupri[MAXDONGLEDEVICES*2];
	int soupri_count;

	volatile unsigned int	connected:1;			/*!< do we have an connection to a device */
	unsigned int		initialized:1;			/*!< whether a service level connection exists or not */
	unsigned int		gsm_registered:1;		/*!< do we have an registration to a GSM */

	unsigned int		selectbusy; 			//true from select-found until old unlock

	unsigned int		dialing;			/*!< HW state; true from ATD response OK until CEND or CONN for this call idx */
	unsigned int		ring:1;				/*!< HW state; true if has incoming call from first RING until CEND or CONN */
	unsigned int		cwaiting:1;			/*!< HW state; true if has incoming call waiting from first CCWA until CEND or CONN for */
	unsigned int		outgoing_sms:1;			/*!< outgoing sms */
	unsigned int		outgoing_ussd:1;		/*!< outgoing sms */
	unsigned int		incoming_sms:1;			/*!< incoming sms */
	unsigned int		volume_sync_step:2;		/*!< volume synchronized stage */
/* SVISTOK_BASELINE_UNIT macro VOLUME_SYNC_BEGIN */
/* SVISTOK_BASELINE_UNIT macro VOLUME_SYNC_DONE */

	unsigned int		use_pdu:1;			/*!< PDU SMS mode in force */
	unsigned int		has_sms:1;			/*!< device has SMS support */
	unsigned int		has_voice:1;			/*!< device has voice call support */
	unsigned int		has_call_waiting:1;		/*!< call waiting enabled on device */

	unsigned int		group_last_used:1;		/*!< mark the last used device */
	unsigned int		prov_last_used:1;		/*!< mark the last used device */
	unsigned int		sim_last_used:1;		/*!< mark the last used device */

	unsigned int		terminate_monitor:1;		/*!< non-zero if we want terminate monitor thread i.e. restart, stop, remove */
//	unsigned int		off:1;				/*!< device not used */
//	unsigned int		prevent_new:1;			/*!< prevent new usage */

	unsigned int		has_subscriber_number:1;	/*!< subscriber_number field is valid */
//	unsigned int		monitor_running:1;		/*!< true if monitor thread is running */
	unsigned int		must_remove:1;			/*!< mean must removed from list: NOT FULLY THREADSAFE */


	unsigned int		must_cfun1;
	unsigned int		must_cfun5;

	volatile dev_state_t	desired_state;			/*!< desired state */
	volatile restate_time_t	restart_time;			/*!< time when change state */
	volatile dev_state_t	current_state;			/*!< current state */

	pvt_config_t		settings;			/*!< all device settings from config file */
	pvt_state_t		state;				/*!< state */
	pvt_stat_t		stat;				/*!< various statistics */
} pvt_t;

/* SVISTOK_BASELINE_UNIT macro CONF_GLOBAL */
/* SVISTOK_BASELINE_UNIT macro SCONF_GLOBAL */

/* SVISTOK_BASELINE_UNIT macro CONF_SHARED */
/* SVISTOK_BASELINE_UNIT macro CONF_UNIQ */
/* SVISTOK_BASELINE_UNIT macro PVT_ID */

/* SVISTOK_BASELINE_UNIT macro PVT_STATE */
/* SVISTOK_BASELINE_UNIT macro PVT_STAT */

typedef struct public_state
{
	AST_RWLIST_HEAD(devices, pvt)	devices;
	ast_mutex_t			discovery_lock;
	pthread_t			discovery_thread;		/* The discovery thread handler */
	volatile int			unloading_flag;			/* no need mutex or other locking for protect this variable because no concurent r/w and set non-0 atomically */
	ast_mutex_t			round_robin_mtx;
	struct pvt			* round_robin[MAXDONGLEDEVICES];
	struct pvt			* random_select[MAXDONGLEDEVICES];
	struct pvt			* limit_select[MAXDONGLEDEVICES];
	struct dc_gconfig		global_settings;
} public_state_t;

EXPORT_DECL public_state_t * gpublic;

/* SVISTOK_BASELINE_UNIT declaration clean_read_data */
/* SVISTOK_BASELINE_UNIT declaration pvt_get_pseudo_call_idx */
/* SVISTOK_BASELINE_UNIT declaration ready4voice_call */
/* SVISTOK_BASELINE_UNIT declaration is_dial_possible */

/* SVISTOK_BASELINE_UNIT declaration pvt_str_state */
/* SVISTOK_BASELINE_UNIT declaration pvt_str_state_ex */
/* SVISTOK_BASELINE_UNIT declaration GSM_regstate2str */
/* SVISTOK_BASELINE_UNIT declaration sys_mode2str */
/* SVISTOK_BASELINE_UNIT declaration sys_submode2str */
/* SVISTOK_BASELINE_UNIT declaration rssi2dBm */

/* SVISTOK_BASELINE_UNIT declaration pvt_on_create_1st_channel */
/* SVISTOK_BASELINE_UNIT declaration pvt_on_remove_last_channel */
/* SVISTOK_BASELINE_UNIT declaration pvt_reload */
/* SVISTOK_BASELINE_UNIT declaration pvt_enabled */
/* SVISTOK_BASELINE_UNIT declaration pvt_try_restate */

/* SVISTOK_BASELINE_UNIT declaration opentty */
/* SVISTOK_BASELINE_UNIT declaration closetty */
/* SVISTOK_BASELINE_UNIT declaration lock_try */
/* SVISTOK_BASELINE_UNIT declaration find_device_ex */

/* SVISTOK_BASELINE_UNIT function find_device */

/* SVISTOK_BASELINE_UNIT declaration find_device_ext */
/* SVISTOK_BASELINE_UNIT declaration find_device_by_resource_ex */

/* SVISTOK_BASELINE_UNIT function find_device_by_resource */

/* SVISTOK_BASELINE_UNIT declaration self_module */

static struct pvt * pvt_create(const pvt_config_t * settings);
static void pvt_destroy(struct pvt * pvt);

/* SVISTOK_BASELINE_UNIT macro PVT_NO_CHANS */

#endif /* SVISTOK_COMPOSED_CHAN_DONGLE_H_HEADER */
#endif /* CHAN_DONGLE_H_INCLUDED */
