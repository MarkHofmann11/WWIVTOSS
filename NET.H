#ifndef _NET_H_
#define _NET_H_
/* All network nodes except the destination will only look at:
 *   tosys - the destination system.  Is 0 if more than one system
 *   list_len - if (tosys==0) then list_len is the number of destination
 *     systems in a list immediately after the header.  NOTE this list will
 *     be 2*list_len bytes long, since each system entry is 2 bytes.
 *   length - is the overall length of the message, not including the header
 *     or the system ist length.  Thus the overall length of a message in
 *     the net will be (sizeof(net_header_rec) + 2*list_len + length)
 */


typedef struct {
	unsigned short	tosys,		/* destination system */
			touser,		/* destination user */
			fromsys,	/* originating system */
			fromuser;	/* originating user */
	unsigned short	main_type,	/* main message type */
			minor_type;	/* minor message type */
	unsigned short	list_len;	/* # of entries in system list */
	unsigned long	daten;		/* date/time sent */
	unsigned long	length;		/* # of bytes of msg after header */
	unsigned short	method;		/* method of compression */
} net_header_rec;


/*
 * Please note that not all of these are used yet, some will probably never
 * be used, but sounded like a good idea at the time.
 */

#define main_type_net_info        0x0001  /* normal network updates */
#define main_type_email           0x0002  /* email by user number */
#define main_type_post            0x0003  /* post from sub host */
#define main_type_file            0x0004  /* unused */
#define main_type_pre_post        0x0005  /* post to sub host */
#define main_type_external        0x0006  /* external message */
#define main_type_email_name      0x0007  /* email by user name */
#define main_type_net_edit        0x0008  /* network editor packet */
#define main_type_sub_list        0x0009  /* subs.lst update */
#define main_type_extra_data      0x000a  /* unused */
#define main_type_group_bbslist   0x000b  /* network update from GC */
#define main_type_group_connect   0x000c  /* network update from GC */
#define main_type_unsed_1         0x000d  /* unused */
#define main_type_group_info      0x000e  /* misc update from GC */
#define main_type_ssm             0x000f  /* so-and-so read your mail */
#define main_type_sub_add_req     0x0010  /* add me to your subboard */
#define main_type_sub_drop_req    0x0011  /* remove me from your subboard */
#define main_type_sub_add_resp    0x0012  /* status of add, 0=ok */
#define main_type_sub_drop_resp   0x0013  /* status of drop, 0=ok */
#define main_type_sub_list_info   0x0014  /* info for subs.lst file */

#define main_type_new_post        0x001a  /* post by sub name */
#define main_type_new_extern      0x001b  /* auto-processing external msgs */


/* these are in main_type_sub_*_resp, as the first byte of the text */
#define sub_adddrop_ok            0x00    /* you've been added/removed */
#define sub_adddrop_not_host      0x01    /* don't host that sub */
#define sub_adddrop_not_there     0x02    /* can't drop, you're not listed */
#define sub_adddrop_not_allowed   0x03    /* not allowed to add/drop you */
#define sub_adddrop_already_there 0x04    /* already in sub */
#define sub_adddrop_error         0xff    /* internal error */

typedef struct {
	unsigned short	systemnumber,	/* System number of the contact */
			numcontacts,	/* number of contacts with system */
			numfails;	/* number of consec failed calls out */
	unsigned long	firstcontact,	/* time of first contact with system */
			lastcontact,	/* time of most recent contact */
			lastcontactsent,/* time of last contact with data sent */
			lasttry,	/* time of last try to connect */
			bytes_received,	/* bytes received from system */
			bytes_sent,	/* bytes sent to system */
			bytes_waiting;	/* bytes waiting to be sent */
} net_contact_rec;

/* Each system will hold a file of these records.  Each record will hold the
 * data pertaining to all contacts with other systems.
 *
 * systemnumber tells what other system this data is for.
 * numcontacts tells how many times a connection has been made with
 *   that system.
 * numfails tells how many times this system has tried to call this other
 *   system but failed.  This is consecutive times, and is zeroed out
 *   when a successful connection is made by calling out.
 * firstcontact is time in unix format of the last time that system has been
 *   contacted on the net.
 * lastcontact is the time the last contact with that system was made.
 * lastcontactsent is the last time data was SENT to that system.
 * lasttry is the last time we tried to call out to that system.
 * bytes_received is number of bytes received from that system.
 * bytes_sent is number of bytes sent to that system.
 * bytes_waiting is the number of bytes waiting to be sent out to that
 *   system.
 *
 * This data is maintained so that the BBS can decide which other system to
 * attempt connection with next.
 */


typedef struct {
        unsigned short  sysnum;         /* system number of the system */
	char		phone[13],	/* phone number of system */
                        name[40];       /* name of system */
        unsigned char   group;          /* group of the system */
        unsigned short  speed,          /* max baud rate of system */
			other,		/* other info about sys (bit-mapped)*/
			forsys;		/* how to get there */
        short           numhops;        /* how long to get there */
        union {
          unsigned short  rout_fact;    /* routing factor */
          float           cost;         /* cost factor */
          long            temp;         /* temporary variable */
        } xx;
} net_system_list_rec;

/* This data is all read in from a text file which holds info about all of
 * the systems in the network.  This text file doesn't hold connection info
 * between the systems.  The purpose of all records should be obvious.
 */

#define other_fido 0x0001           /* \ */

/* modem type designators */
#define other_Telebit_19200 0x0002  /* | */
#define other_USR_9600 0x0004       /* < */
#define other_Hayes_9600 0x0008     /* > */
#define other_V32 0x0020            /* ! */
#define other_V32bis 0x0040         /* $ */
#define other_compucom 0x0400       /* / */
#define other_FAX 0x1000            /* ? */
#define other_VFAST 0x4000          /* ~ */

/* system type designators */
#define other_coordinator 0x0010    /* ^ */
#define other_group_coord 0x0100    /* % */
#define other_net_coord   0x0200    /* & */

#define other_end_system 0x2000     /* _ */
#define other_net_server 0x0800     /* + */
#define other_PCP 0x0080            /* = */


typedef struct {
        unsigned short  sysnum,         /* outward calling system */
			numsys;		/* num systems it can call */
        unsigned short  *connect;       /* list of numsys systems */
	float		*cost;		/* list of numsys costs */
} net_interconnect_rec;

/* This data is also read in from a text file.  It tells how much it costs for
 * sysnum to call out to other systems.
 *
 * numsys - is the number of systems that sysnum can call out to
 * connect[] - points to an array of numsys integers that tell which
 *   other systems sysnum can connect to
 * cost[] - points to an array of numsys floating point numbers telling
 *   how much it costs to connect to that system, per minute.  ie, it would
 *   cost (cost[1]) dollars per minute for sysnum to call out to system
 *   number (connect[1]).
 */


typedef struct {
        unsigned short  sysnum;         /* system to contact */
	unsigned char	macnum;		/* macro/script to use */
        unsigned short  options;        /* bit mapped */
	unsigned char	call_anyway;	/* num days between calls */
        char            min_hr,
                        max_hr;         /* calling hours */
	char		password[20];	/* password for that system */
        unsigned char   times_per_day;  /* num times should call per day */
        unsigned char   call_x_days;    /* call only every x days */
        unsigned short  min_k;          /* minimum # k before callout */
        char *          opts;           /* options or NULL */
} net_call_out_rec;

/* This record holds info about other systems that the sysop has determined
 * that we can call out to.
 *
 * sysnum - system number that we can call
 * macnum - macro/script number to use to call that system.  This is set
 *   to zero if we just dial it directly.
 * options - bit mapped.  See #define's below.
 * call_anyway - if non-zero, then the minimum number of days between calls
 *   to that system, even if there is nothing waiting to be sent there.  This
 *   should only be non-zero if options_sendback is also set for this system.
 * password - is the password used for connection to this system.
 */


#define options_sendback      0x0001   /* & they can send data back */
#define options_ATT_night     0x0002   /* - callout only at AT&T nigh hours */
#define options_PCP_night     0x0004   /* = callout only at PCP night hours */
#define options_no_call       0x0008   /* + don't call that system, it will call */
#define options_receive_only  0x0010   /* ~ never send anything to the system */
#define options_once_per_day  0x0020   /* ! Only call once per day */
#define options_compress      0x0040   /* ; compress data */
#define options_hslink        0x0080   /* ^ use HSLINK */
#define options_force_ac      0x0100   /* $ force area code on dial */

typedef struct {
        unsigned short  sysnum;         /* system number we can call */
        short           nument;         /* number of entries in list */
        unsigned short  *list;          /* list of systems */
} sys_for_rec;


typedef struct {
        unsigned char   type;           /* type of network */
        char            name[16];       /* network name */
        char            dir[69];        /* directory for net data */
        unsigned short  sysnum;         /* system number */
        net_call_out_rec far *con;      /* ptr to callout data */
        net_contact_rec far *ncn;       /* ptr to contact info */
        short           num_con;        /* number in array */
        short           num_ncn;        /* number in array */
} net_networks_rec;

#define net_type_wwivnet 0
#define net_type_fidonet 1

#endif
