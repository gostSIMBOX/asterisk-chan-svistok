#ifndef READER_CORE__H_INCLUDED
#define READER_CORE__H_INCLUDED

#define MAXREADERDEVICES	512
#define DEVPATHLEN		256

#define SIMTIMERESET	500
#define SIMTIMEPOST	200
#define SIMTIMEOUT	1000


/*
INLINE_DECL const char * dev_state2str(dev_state_t state)
{
	return enum2str(state, dev_state_strs, ITEMS_OF(dev_state_strs));
}

INLINE_DECL const char * dev_state2str_msg(dev_state_t state)
{
	static const char * const states[] = { "Stop scheduled", "Restart scheduled", "Removal scheduled", "Start scheduled" };
	return enum2str(state, states, ITEMS_OF(states));
}
*/


typedef struct rdr
{
	char			dev[DEVPATHLEN];
	char			data_tty[DEVPATHLEN];
	int			data_fd;
	char			* data_lock;


	int			timeout;			/*!< used to set the timeout for data */
	#define READER_DATA_READ_TIMEOUT	500				/* 500 msec */
	#define READER_INIT_TIME	10000

	unsigned int		connected;			/*!< do we have an connection to a device */
//	unsigned int		initialized;			/*!< whether a service level connection exists or not */

	char iccid[64];
	char imsi[64];
	char ki[64];
	char spn[64];

	int haspin;
} rdr_t;

#endif
