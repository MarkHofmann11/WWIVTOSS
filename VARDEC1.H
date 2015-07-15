/*****************************************************************************

				WWIV Version 4
                    Copyright (C) 1988-1993 by Wayne Bell

Distribution of the source code for WWIV, in any form, modified or unmodified,
without PRIOR, WRITTEN APPROVAL by the author, is expressly prohibited.
Distribution of compiled versions of WWIV is limited to copies compiled BY
THE AUTHOR.  Distribution of any copies of WWIV not compiled by the author
is expressly prohibited.


*****************************************************************************/


#ifndef _VARDEC_H_
#define _VARDEC_H_





/* DATA FOR EVERY USER */
typedef struct {

    unsigned char
      name[31],       /* user's name/handle */
      realname[21],   /* user's real name */
      callsign[7],    /* user's amateur callsign */
      phone[13],      /* user's phone number */
      dataphone[13],  /* user's data phone */
      street[31],     /* street address */
      city[31],       /* city */
      state[3],       /* state code [MO, CA, etc] */
      country[4],     /* country [USA, CAN, FRA, etc] */
      zipcode[11],    /* zipcode [#####-####] */
      pw[9],          /* user's password */
      laston[9],      /* last date on */
      firston[9],     /* first date on */
      note[61],       /* sysop's note about user */
      macros[3][81],  /* macro keys */
      sex;            /* user's sex */

    char
      res_char[78];   /* bytes for more strings */

    unsigned char
      age,            /* user's age */
      inact,          /* if deleted or inactive */
      comp_type,      /* computer type */
      defprot,        /* deflt transfer protocol */
      defed,          /* default editor */
      screenchars,    /* screen width */
      screenlines,    /* screen height */
      num_extended,   /* extended description lines */
      optional_val,   /* optional lines in msgs */
      sl,             /* security level */
      dsl,            /* transfer security level */
      exempt,         /* exempt from ratios, etc */
      colors[10],     /* user's colors */
      bwcolors[10],   /* user's b&w colors */
      votes[20],      /* user's votes */
      illegal,        /* illegal logons */
      waiting,        /* number mail waiting */
      ontoday,        /* num times on today */
      month,          /* birth month */
      day,            /* birth day */
      year,           /* birth year */
      language,       /* language to use */
      cbv;            /* called back */

    char
      res_byte[49];   /* reserved for byte values */

    unsigned short
      homeuser,       /* user number where user can be found */
      homesys,        /* system where user can be found */
      forwardusr,     /* mail forwarded to this user number */
      forwardsys,     /* mail forwarded to this system number */
      net_num,        /* net num for forwarding */
      msgpost,        /* number messages posted */
      emailsent,      /* number of email sent */
      feedbacksent,   /* number of f-back sent */
      fsenttoday1,    /* feedbacks today */
      posttoday,      /* number posts today */
      etoday,         /* number emails today */
      ar,             /* board access */
      dar,            /* directory access */
      restrict,       /* restrictions on account */
      ass_pts,        /* bad things the user did */
      uploaded,       /* number files uploaded */
      downloaded,     /* number files downloaded */
      lastrate,       /* last baud rate on */
      logons,         /* total number of logons */
      emailnet,       /* email sent via net */
      postnet,        /* posts sent thru net */
      deletedposts,   /* how many posts deleted */
      chainsrun,      /* how many "chains" run */
      gfilesread,     /* how many gfiles read */
      banktime,       /* how many mins in timebank */
      homenet;        /* home net number */

   char
     res_short[48];   /* reserved for short values */

    unsigned long
      msgread,        /* total num msgs read */
      uk,             /* number of k uploaded */
      dk,             /* number of k downloaded */
      daten,          /* numerical time last on */
      sysstatus,      /* status/defaults */
      wwiv_regnum,    /* user's WWIV reg number */
      filepoints,     /* points to spend for files */
      registered,     /* numerical registration date */
      expires;        /* numerical expiration date */

    char
      res_long[48];   /* reserved for long values */

    float
      timeontoday,    /* time on today */
      extratime,      /* time left today */
      timeon,         /* total time on system */
      pos_account,    /* $ credit */
      neg_account,    /* $ debit */
      gold;           /* game money */

    char
      res_float[32];  /* reserved for real values */

    char
      res_gp[94];    /* reserved for whatever */

    unsigned int qwk_max_msgs;
    unsigned int qwk_max_msgs_per_sub;

    unsigned int qwk_dont_scan_mail : 1;
    unsigned int qwk_delete_mail : 1;
    unsigned int qwk_dontsetnscan : 1;
    unsigned int qwk_remove_color : 1;
    unsigned int qwk_convert_color : 1;
    unsigned int qwk_archive : 3;

    unsigned int qwk_leave_bulletin : 1;
    unsigned int qwk_dontscanfiles : 1;
    unsigned int qwk_keep_routing : 1;
    unsigned int                  : 1;
    unsigned int qwk_protocol : 4;

} userrec;
/* DYNAMIC SYSTEM STATUS */
typedef struct {
	char		date1[9],		/* last date active */
			date2[9],		/* date before now */
			date3[9],		/* two days ago */
			log1[13],		/* yesterday's log */
			log2[13],		/* two days ago log */
			gfiledate[9],		/* date gfiles last updated */
                        filechange[7];          /* flags for files changing */
	unsigned short
                        localposts,             /* how many local posts today */
                        users,                  /* Current number of users */
			callernum,		/* Current caller number */
			callstoday,		/* Number of calls today */
                        msgposttoday,           /* Messages posted today*/
			emailtoday,		/* Email sent today */
			fbacktoday,		/* Feedback sent today */
			uptoday,		/* files uploaded today */
			activetoday;		/* Minutes active today */
	unsigned long	qscanptr;		/* Q-scan pointer value */
	char		amsganon;		/* auto-message anony stat */
	unsigned short	amsguser;		/* user who wrote a-msg */
	unsigned long	callernum1;		/* caller number */
        unsigned short  net_edit_stuff;         /* word for net editor */
        unsigned short  wwiv_version;           /* tell what version it is */
        unsigned short  net_version;            /* tell what version of net */
        float           net_bias;               /* network bias factor */
        long            last_connect,           /* date last connect.net */
                        last_bbslist;           /* date last bbslist.net */
        float           net_req_free;           /* net free factor def 3 */
        unsigned short  days;                   /* # days BBS active */
        char            res[29];                /* RESERVED */
} statusrec;


/* SECLEV DATA FOR 1 SL */
typedef struct {
	unsigned short	time_per_day,		/* time allowed on per day */
			time_per_logon,		/* time allowed on per logon */
			messages_read,		/* messages allowed to read */
			emails,			/* number emails allowed */
			posts;			/* number posts allowed */
	unsigned long	ability;		/* bit mapped abilities */
} slrec;


/* AUTO-VALIDATION DATA */
typedef struct {
	unsigned char	sl,			/* SL */
			dsl;			/* DSL */
	unsigned short	ar,			/* AR */
			dar,			/* DAR */
			restrict;		/* restrictions */
} valrec;

typedef struct {
        char            extension[4],           /* extension for archive */
                        arca[32],
                        arce[32],
                        arcl[32];
} arcrec;

/* STATIC SYSTEM INFORMATION */
typedef struct {
	char		newuserpw[21],		/* new user password */
			systempw[21],		/* system password */
			msgsdir[81],		/* path for msgs directory */
			gfilesdir[81],		/* path for gfiles dir */
			datadir[81],		/* path for data directory */
			dloadsdir[81],		/* path for dloads dir */
			ramdrive,		/* drive for ramdisk */
			tempdir[81],		/* path for temporary directory */
                        xmark,                  /* 0xff */
                        regcode[83],            /* registration code */
			bbs_init_modem[51],	/* modem initialization cmd */
			answer[21],		/* modem answer cmd */
			connect_300[21],	/* modem responses for */
			connect_1200[21],	/* connections made at */
			connect_2400[21],	/* various speeds */
			connect_9600[21],
			connect_19200[21],
			no_carrier[21],		/* modem disconnect */
			ring[21],		/* modem ring */
			terminal[21],		/* DOS cmd for run term prg */
			systemname[51],		/* BBS system name */
			systemphone[13],	/* BBS system phone number */
			sysopname[51],		/* sysop's name */
			executestr[51];		/* mail route path name */
	unsigned char	newusersl,		/* new user SL */
			newuserdsl,		/* new user DSL */
			maxwaiting,		/* max mail waiting */
			comport[5],		/* what connected to comm */
			com_ISR[5],		/* Com Interrupts */
			primaryport,		/* primary comm port */
			newuploads,		/* file dir new uploads go */
			closedsystem;		/* if system is closed */
	unsigned short	systemnumber,		/* BBS system number */
			baudrate[5],		/* Baud rate for com ports */
			com_base[5],		/* Com base addresses */
			maxusers,		/* max users on system */
			newuser_restrict,	/* new user restrictions */
			sysconfig,		/* System configuration */
			sysoplowtime,		/* Chat time on */
			sysophightime,		/* Chat time off */
			executetime;		/* time to run mail router */
	float		req_ratio,		/* required up/down ratio */
			newusergold;		/* new user gold */
	slrec		sl[256];		/* security level data */
	valrec		autoval[10];		/* sysop quik-validation data*/
	char		hangupphone[21],	/* string to hang up phone */
			pickupphone[21];	/* string to pick up phone */
        unsigned short  netlowtime,             /* net time on */
			nethightime;		/* net time off */
	char            connect_300_a[21],	/* alternate connect str's */
			connect_1200_a[21],
			connect_2400_a[21],
			connect_9600_a[21],
			connect_19200_a[21];
        arcrec          arcs[4];
        char            beginday_c[51],
                        logon_c[51];
        short           userreclen,
                        waitingoffset,
                        inactoffset;
        char            newuser_c[51];
        unsigned long   wwiv_reg_number;
        char            dial_prefix[21];
        float           post_call_ratio;
        char            upload_c[51];
        char            dszbatchdl[81];
        char            modem_type[9];
        char            batchdir[81];
        short           sysstatusoffset;
        char            network_type;
        short           fuoffset,
                        fsoffset,
                        fnoffset;
        unsigned short  max_subs,
                        max_dirs,
                        qscn_len;
        unsigned char   email_storage_type;
        unsigned long   sysconfig1;
        char            res[19];               /* RESERVED */
} configrec;


/* userrec.inact */
#define inact_deleted  0x01
#define inact_inactive 0x02

/* configrec.sysconfig */
#define sysconfig_no_local      0x0001
#define sysconfig_no_beep       0x0002
#define sysconfig_high_speed    0x0004
#define sysconfig_off_hook      0x0008
#define sysconfig_two_color     0x0010
#define sysconfig_flow_control  0x0020
#define sysconfig_printer       0x0040
#define sysconfig_list          0x0080
#define sysconfig_no_xfer       0x0100
#define sysconfig_2_way         0x0200
#define sysconfig_no_alias      0x0400
#define sysconfig_all_sysop     0x0800
#define sysconfig_shrink_term   0x1000
#define sysconfig_free_phone    0x2000
#define sysconfig_log_dl        0x4000
#define sysconfig_extended_info 0x8000


#endif
