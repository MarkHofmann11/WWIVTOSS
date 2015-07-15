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

#define OK_LEVEL 0
#define NOK_LEVEL 1
#define QUIT_LEVEL 2
#define EVENT_LEVEL 4

#define DELIMS_NORMAL " ;.!:-?,\t\r\n"
#define DELIMS_WHITE  " \t\r\n"

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
      sex,            /* user's sex */
      womr[1],        /* Womr toggle */
      which_womr[1],  /* Which Womr to use? */
      need_qscfg[1],  /* Need to reconfigure Qscan? */
      use_list_plus[1], /* Use List_plus stuff */
      split_menu;       /* Use Split menus? */
    char
      res_char[73];   /* bytes for more strings */

    unsigned char
      age,            /* user's age */
      inact;          /* if deleted or inactive */
    char
      comp_type;      /* computer type */
    unsigned char
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
      cbv,            /* called back */
      new_sub_notify,
      timelock,
      clear_timelock,
      sds_requested,
      mods_requested;
    char
      res_byte[44];   /* reserved for byte values */

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
      expires,        /* numerical expiration date */
      datenscan;      /* numerical date of last new file scan */

    char
      res_long[44];   /* reserved for long values */

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
      res_gp[89];    /* reserved for whatever */

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
  unsigned int oldmail;
int list_config;

     unsigned char use_internal_tag : 1;
     unsigned char use_menusys : 1;

     unsigned char use_three : 1;
     unsigned char use_four : 1;
     unsigned char use_five : 1;
     unsigned char use_six : 1;
     unsigned char use_seven : 1;
     unsigned char use_eight : 1;

} userrec;


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


/* overlay information per instance */
typedef struct {
        unsigned char   com_ISR[9],
                        primaryport;
        unsigned short  com_base[9];
        char            modem_type[9],
                        tempdir[81],
                        batchdir[81];
        unsigned short  comflags;
        char            bootdrive;
        char            res[310];
} configoverrec;


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



/* MESSAGE BASE INFORMATION */
typedef struct {
	char		name[41],		/* board name */
			filename[9],		/* board database filename */
			key;			/* board special key */
	unsigned char	readsl,			/* sl required to read */
			postsl,			/* sl required to post */
			anony,			/* anonymous board? */
			age;			/* minimum age for sub */
	unsigned short	maxmsgs,		/* max # of msgs */
			ar,			/* AR for sub-board */
			storage_type,		/* how messages are stored */
			type;			/* 4 digit board type */
} subboardrec;



/* UPLOAD DIRECTORY INFORMATION */
typedef struct {
	char		name[41],		/* directory name */
			filename[9],		/* direct database filename */
			path[81];		/* filename path */
	unsigned char	dsl,			/* DSL for directory */
			age;			/* minimum age for directory */
	unsigned short	dar,			/* DAR for directory */
			maxfiles,		/* max files for directory */
			mask,			/* file type mask */
			type;			/* 4 digit directory type */
} directoryrec;



/* QUICK REFERNCE TO FIND USER NUMBER FROM NAME */
typedef struct {
  unsigned char name[31];
  unsigned short number;
} smalrec;



/* TYPE TO TELL WHERE A MESSAGE IS STORED */
typedef struct {
	unsigned char	storage_type;		/* how it is stored */
	unsigned long	stored_as;		/* where it is stored */
} messagerec;

/* MODIFIED MESSAGEREC FOR RIP MENUS */
typedef struct {
	unsigned char   storage_type;       /* how it is stored */
	unsigned long   stored_as;      /* where it is stored */
	unsigned int    menu_num;
} ripmsgrec;


/* DATA HELD FOR EVERY POST */
typedef struct {
	char		title[81];		/* title of post */
	unsigned char	anony,			/* anony-stat of message */
			status;			/* bit-mapped status */
	unsigned short	ownersys,owneruser;	/* who posted it */
	unsigned long	qscan,			/* qscan pointer */
			daten;			/* numerical date posted */
	messagerec	msg;			/* where to find it */
} postrec;



/* DATA HELD FOR EVERY E-MAIL OR F-BACK */
typedef struct {
	char		title[81];		/* E-mail title */
	unsigned char	anony,			/* anonymous mail? */
			status;			/* status for e-mail */
	unsigned short	fromsys,fromuser,	/* originating system,user */
			tosys,touser;		/* destination system,user */
	unsigned long	daten;			/* date it was sent */
	messagerec	msg;			/* where to find it */
} mailrec;

/* USED IN READMAIL TO STORE EMAIL INFO */
typedef struct {
        short           index;                  /* index into email.dat */
        unsigned short  fromsys,fromuser;       /* originating system,user */
	unsigned long	daten;			/* date it was sent */
	messagerec	msg;			/* where to find it */
} tmpmailrec;


/* SHORT RESPONSE FOR USER, TELLING HIS MAIL HAS BEEN READ */
typedef struct {
	char		message[81];		/* short message to user */
	unsigned short	tosys,touser;		/* who it is to */
} shortmsgrec;



/* VOTING RESPONSE DATA */
typedef struct {
	char		response[81];		/* Voting question response */
	unsigned short	numresponses;		/* number of responses */
} voting_response;



/* VOTING DATA INFORMATION */
typedef struct {
	char		question[81];		/* Question */
	unsigned char	numanswers;		/* number of responses */
	voting_response	responses[20];		/* actual responses */
} votingrec;



/* DATA HELD FOR EVERY UPLOAD */
typedef struct {
	char		filename[13],		/* filename */
			description[59],	/* file description */
			date[9],		/* date u/l'ed */
                        upby[42];               /* name of upload user */
        unsigned long   filepoints;             /* file points */
        unsigned char   filetype;               /* file type for apples */
	unsigned short	numdloads,		/* number times d/l'ed */
			ownersys,ownerusr,	/* who uploaded it */
			mask;			/* file type mask */
	unsigned long	daten,			/* date uploaded */
			numbytes;		/* number bytes long file is */
} uploadsrec;


typedef struct {
        uploadsrec      u;                      /* file information */
        short           directory;              /* directory number */
        unsigned short  dir_mask;               /* directory mask */
} tagrec;


/* ZLOG INFORMATION FOR PAST SYSTEM USAGE */
typedef struct {
	char		date[9];		/* zlog for what date */
	unsigned short	active,			/* number minutes active */
			calls,			/* number calls */
			posts,			/* number posts */
			email,			/* number e-mail */
			fback,			/* number f-back */
			up;			/* number uploads */
} zlogrec;



/* DATA FOR OTHER PROGRAMS AVAILABLE */
typedef struct {
	char		filename[81],		/* filename for .chn file */
			description[81];	/* description of it */
	unsigned char	sl,			/* seclev restriction */
			ansir;			/* if ANSI required */
	unsigned short	ar;			/* AR restriction */
} chainfilerec;


typedef struct {
        int             regby[5],               /* who registered */
                        usage;                  /* number of runs */
        unsigned char   minage,                 /* minimum age necessary */
                        maxage;                 /* maximum age allowed */
        char            space[50];              /* reserved space */
} chainregrec;


/* DATA FOR EXTERNAL PROTOCOLS */
typedef struct {
	char		description[81],	/* protocol description */
			receivefn[81],		/* receive filename */
			sendfn[81];		/* send filename */
	unsigned short	ok1,ok2,		/* if sent */
			nok1,nok2;		/* if not sent */
} externalrec;

typedef struct {
	char		description[81],	/* protocol description */
			receivefn[81],		/* receive filename */
                        sendfn[81],             /* send filename */
                        receivebatchfn[81],     /* batch receive cmd */
                        sendbatchfn[81],        /* batch send cmd */
                        bibatchfn[81];          /* batch send/receive cmd */
        unsigned short  ok1;                    /* if sent */
        unsigned short  othr;                   /* other flags */
        char            pad[22];
} newexternalrec;


/* DATA FOR EXTERNAL EDITORS */
typedef struct {
	char		description[81],	/* description of editor */
			filename[81];		/* how to run the editor */
	unsigned long	config;			/* configuration for editor */
        char		filenamecon[81];	/* how to run locally */
	char		res[119];
} editorrec;


typedef struct {                            /* ADD */
    unsigned char note1[81],                /* ADD */
    note2[81];                              /* ADD */
    unsigned int  color;                    /* ADD */
} noterec;                                  /* ADD */



/* DATA FOR CONVERSION OF MAIN MENU KEYS TO SUB-BOARD NUMBERS */
typedef struct {
        char            keys[5];
        short           subnum;
} usersubrec;


typedef struct {
        short           confnum;
} userconfrec;


typedef struct {
        char            sending;
        char            filename[13];
        short           dir;
        float           time;
        long            len;
        long            filepoints;
        short           status;
} batchrec;


typedef enum {
  xf_up, xf_down, xf_up_temp, xf_down_temp, xf_up_batch, xf_down_batch, xf_bi, xf_none
} xfertype;

/* userrec.inact */
#define inact_deleted  0x01
#define inact_inactive 0x02

/* userrec.exempt */
#define exempt_ratio 0x01
#define exempt_time  0x02
#define exempt_all   0x04
#define exempt_post  0x08

/* userrec.restrict */
#define restrict_logon           0x0001
#define restrict_chat            0x0002
#define restrict_validate        0x0004
#define restrict_automessage     0x0008
#define restrict_anony           0x0010
#define restrict_post            0x0020
#define restrict_email           0x0040
#define restrict_vote            0x0080
#define restrict_iichat          0x0100
#define restrict_net             0x0200
#define restrict_upload          0x0400

#define restrict_string "LCMA*PEVKNU     "

/* userrec.sysstatus */
#define sysstatus_ansi              0x00000001
#define sysstatus_color             0x00000002
#define sysstatus_music             0x00000004
#define sysstatus_pause_on_page     0x00000008
#define sysstatus_expert            0x00000010
#define sysstatus_smw               0x00000020
#define sysstatus_full_screen       0x00000040
#define sysstatus_nscan_file_system 0x00000080
#define sysstatus_extra_color       0x00000100
#define sysstatus_clr_scrn          0x00000200
#define sysstatus_upper_ASCII       0x00000400
#define sysstatus_no_tag            0x00001000
#define sysstatus_conference        0x00002000
#define sysstatus_nochat            0x00004000
#define sysstatus_rip               0x00008000
#define sysstatus_no_msgs           0x00010000
#define sysstatus_menusys           0x00020000
#define sysstatus_listplus          0x00040000
#define sysstatus_disable_rip       0x00080000
#define sysstatus_readplus          0x00100000


/* slrec.ability */
#define ability_post_anony       0x0001
#define ability_email_anony      0x0002
#define ability_read_post_anony  0x0004
#define ability_read_email_anony 0x0008
#define ability_limited_cosysop  0x0010
#define ability_cosysop          0x0020
#define ability_val_net          0x0040

/* subboardrec.anony */
#define anony_enable_anony     0x01
#define anony_enable_dear_abby 0x02
#define anony_force_anony      0x04
#define anony_real_name        0x08
#define anony_val_net          0x10
#define anony_ansi_only        0x20
#define anony_no_tag           0x40
#define anony_require_sv       0x80

/* postrec.anony, mailrec.anony */
#define anony_sender      0x01
#define anony_sender_da   0x02
#define anony_sender_pp   0x03
#define anony_receiver    0x10
#define anony_receiver_da 0x20
#define anony_receiver_pp 0x30

/* directoryrec.mask */
#define mask_PD            0x0001
#define mask_DIZ           0x0002
#define mask_no_uploads    0x0004
#define mask_archive       0x0008
#define mask_pending_batch 0x0010
#define mask_no_ratio      0x0020
#define mask_cdrom         0x0040
#define mask_offline       0x0080
#define mask_B4DIZ         0x0100
#define mask_guest         0x4000
#define mask_extended      0x8000

/* postrec.status */
#define status_unvalidated          0x01
#define status_delete               0x02
#define status_no_delete            0x04
#define status_pending_net          0x08
#define status_post_source_verified 0x10
#define status_post_new_net         0x20

/* mailrec.status */
#define status_multimail       0x01
#define status_source_verified 0x02
#define status_new_net         0x04
#define status_seen            0x08
#define status_replied         0x10
#define status_file            0x80

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

/* configoverrec.comflags */
#define comflags_buffered_uart  0x0001

/* editorrec.config */
#define config_80_25 0x0001

#define ansir_ansi       0x01
#define ansir_no_DOS     0x02
#define ansir_no_300     0x04
#define ansir_shrink     0x08
#define ansir_no_pause   0x10
#define ansir_local_only 0x20
#define ansir_multi_user 0x40
#define ansir_rip_only   0x80

/* newexternalrec.othr */
#define othr_error_correct     0x0001
#define othr_bimodem           0x0002
#define othr_override_internal 0x0004

/* statusrec.filechange [0..6] */
#define filechange_names  0
#define filechange_upload 1
#define filechange_posts  2
#define filechange_email  3
#define filechange_net    4

/* g_flags */
#define g_flag_disable_conf   0x00000001
#define g_flag_disable_pause  0x00000002
#define g_flag_scanned_files  0x00000004
#define g_flag_made_find_str  0x00000008
#define g_flag_pipe_colors    0x00000010
#define g_flag_allow_extended 0x00000020
#define g_flag_disable_mci    0x00000040

#define PREV 1
#define NEXT 2
#define DONE 4
#define ABORTED 8
#define NUM_ONLY 1
#define UPPER_ONLY 2
#define ALL 4
#define SET 8

#define XFER_TIME(b) (modem_speed?\
                      (((double)(((b)+127)/128))*1280.0/((double)modem_speed))\
                      :0.0)

struct line {
	char		text[160];
	struct line 	*prev,*next;
};


typedef struct {
  short x1,y1,topline1,curatr1;
  char *scrn1;
} screentype;

typedef struct{
  char name[13];
  short len;
} ext_desc_type;

typedef struct {
	char		name[41],		/* g-file section name */
			filename[9];		/* g-file database filename */
	unsigned char	sl,			/* sl required to read */
			age;			/* minimum age for section */
	unsigned short	maxfiles,		/* max # of files */
			ar;			/* AR for g-file section */
} gfiledirrec;

typedef struct {
	char		description[81],	/* description of file */
			filename[13];		/* filename of file */
        long		daten;			/* date added */
} gfilerec;

typedef struct {
        char            curspeed[23];           /* description of speed */
        char            return_code[23];        /* modem result code */
        unsigned short  modem_speed,            /* speed modems talk at */
                        com_speed;              /* speed com port runs at */
} resultrec;


typedef struct {
        char            name[20];               /* language "name" */
        unsigned char   num;                    /* language "number" */
        char            dir[79];                /* directory info stored in */
} languagerec;

typedef struct {
   char filename[9];
   long daten;
 } newsubsrec;


#define MAX_BUF 1024

#define INT_REAL_DOS 0x21

/****************************************************************************/
/* modem info structure */

#define mode_norm     1   /* normal status */
#define mode_ring     2   /* phone is ringing */
#define mode_dis      3   /* disconnected (no connection) */
#define mode_err      4   /* error encountered */
#define mode_ringing  5   /* remote phone is ringing */
#define mode_con      6   /* connection established */
#define mode_ndt      7   /* no dial tone */
#define mode_fax      8   /* fax connection */
#define mode_cid_num  9   /* caller-id info, phone # */
#define mode_cid_name 10  /* caller-id info, caller's name */

#define flag_as       1   /* asymmetrical baud rates */
#define flag_ec       2   /* error correction in use */
#define flag_dc       4   /* data compression in use */
#define flag_fc       8   /* flow control should be used */
#define flag_append   16  /* description string should be appended */
/****************************************************************************/

#define SECONDS_PER_HOUR  3600L
#define SECONDS_PER_DAY   86400L

typedef struct {
  char result[41];
  char description[31];
  unsigned short main_mode;
  unsigned short flag_mask;
  unsigned short flag_value;
  unsigned short com_speed;
  unsigned short modem_speed;
} result_info;

typedef struct {
  unsigned short ver;
  char name[81];
  char init[161];
  char setu[161];
  char ansr[81];
  char pick[81];
  char hang[81];
  char dial[81];

  char sepr[10];

  result_info defl;
  unsigned short num_resl;
  result_info resl[1];
} modem_info;

/****************************************************************************/
/* Dropfile stuff */

typedef struct {
  char
    display[2],
    printer[2],
    page_bell[2],
    alarm[2],
    sysop_next,
    errcheck[2],
    graphics,
    nodechat,
    openbps[5],
    connectbps[5];
  int
    usernum;
  char
    firstname[15],
    password[12];
  int
    time_on,
    prev_used;
  char
    time_logged[5];
  int
    time_limit,
    down_limit;
  char
    curconf,
    bitmap1[5],
    bitmap2[5];
  int
    time_added,
    time_credit;
  char
    slanguage[4],
    name[25];
  int
    sminsleft;
  char
    snodenum,
    seventtime[5],
    seventactive[2],
    sslide[2],
	  smemmsg[4],
    scomport,
    packflag,
    bpsflag;

  /* PCB 14.5 extra stuff */
  char
    ansi,
    lastevent[8];
  int
    lasteventmin;
  char
    exittodos,
    eventupcoming;
  int
    lastconfarea;
  char
    hconfbitmap;
  /* end PCB 14.5 additions */
} pcboard_sys_rec;
#define AR_A 1
#define AR_B 2
#define AR_C 4
#define AR_D 8
#define AR_E 16
#define AR_F 32
#define AR_G 64
#define AR_H 128
#define AR_I 256
#define AR_J 512
#define AR_K 1024
#define AR_L 2048
#define AR_M 4096
#define AR_N 8192
#define AR_O 16384
#define AR_P 32768

/****************************************************************************/
/* conferencing stuff */

#define conf_status_ansi       0x0001  /* ANSI required */
#define conf_status_wwivreg    0x0002  /* WWIV regnum required */
#define conf_status_offline    0x0004  /* conference is "offline" */

#define CONF_UPDATE_INSERT     1
#define CONF_UPDATE_DELETE     2
#define CONF_UPDATE_SWAP       3

#define SUBCONF_TYPE unsigned short
#define MAX_CONFERENCES 26

typedef struct {
  unsigned char designator,       /* A to Z? */
                name[61],         /* Name of conference */
                minsl,            /* Minimum SL needed for access */
                maxsl,            /* Maximum SL allowed for access */
                mindsl,           /* Minimum DSL needed for access */
                maxdsl,           /* Maximum DSL allowed for access */
                minage,           /* Minimum age needed for access */
                maxage,           /* Maximum age allowed for access */
                sex;              /* Gender: 0=male, 1=female, 2=all */
  SUBCONF_TYPE  status,           /* Bit-mapped stuff */
                minbps,           /* Minimum bps rate for access */
                ar,               /* ARs necessary for access */
                dar,              /* DARs necessary for access */
                num,              /* Num "subs" in this conference */
                maxnum,           /* max num subs allocated in 'subs' */
                *subs;            /* "Sub" numbers in this conference */
} confrec;
typedef struct {
  int             user;
  char            filename[13];
  long            id;
  unsigned long   numbytes;
} filestatusrec;

/****************************************************************************/

#define CHAINFILE_CHAIN       0
#define CHAINFILE_DORINFO     1
#define CHAINFILE_PCBOARD     2
#define CHAINFILE_CALLINFO    3
#define CHAINFILE_DOOR        4

/****************************************************************************/

#define EFLAG_ABORT           0x0001  /* check for a ^C to abort program */
#define EFLAG_INTERNAL        0x0002  /* make it look internal to BBS */
#define EFLAG_NOHUP           0x0004  /* don't check for hangup (UL event) */
#define EFLAG_COMIO           0x0008  /* redirect IO to com port */
#define EFLAG_SHRINK          0x0010  /* shrink for more free mem (slower) */
#define EFLAG_FILES           0x0020  /* create shrink files for extrn progs */
#define EFLAG_NOPAUSE         0x0040  /* disable pause in remote */
#define EFLAG_NETPROG         0x0080  /* try running out of net dir first */
#define EFLAG_TOPSCREEN       0x0100  /* leave topscreen as-is */

/****************************************************************************/

#define OP_FLAGS_FORCE_NEWUSER_FEEDBACK   0x00000001
#define OP_FLAGS_CHECK_DUPE_PHONENUM      0x00000002
#define OP_FLAGS_HANGUP_DUPE_PHONENUM     0x00000004
#define OP_FLAGS_SIMPLE_ASV               0x00000008
#define OP_FLAGS_POSTTIME_COMPENSATE      0x00000010
#define OP_FLAGS_SHOW_HIER                0x00000020
#define OP_FLAGS_IDZ_DESC                 0x00000040
#define OP_FLAGS_SETLDATE                 0x00000080
#define OP_FLAGS_NEW_CHATSOUND            0x00000100
#define OP_FLAGS_SLASH_SZ                 0x00000200
#define OP_FLAGS_READ_CD_IDZ              0x00000400
#define OP_FLAGS_FSED_EXT_DESC            0x00000800
#define OP_FLAGS_FAST_TAG_RELIST          0x00001000
#define OP_FLAGS_MAIL_PROMPT              0x00002000
#define OP_FLAGS_SHOW_CITY_ST             0x00004000
#define OP_FLAGS_NO_EASY_DL               0x00008000
#define OP_FLAGS_NEW_EXTRACT              0x00010000
#define OP_FLAGS_FAST_SEARCH              0x00020000
#define OP_FLAGS_NET_CALLOUT              0x00040000
#define OP_FLAGS_WFC_SCREEN               0x00080000
#define OP_FLAGS_FIDO_PROCESS             0x00100000
#define OP_FLAGS_USER_REGIST              0x00200000
#define OP_FLAGS_MSG_TAG                  0x00400000
#define OP_FLAGS_CHAIN_REG                0x00800000
#define OP_FLAGS_CAN_SAVE_SSM             0x01000000
#define OP_FLAGS_PACKSCAN_FREQ            0x02000000
#define OP_FLAGS_EXTRA_COLOR              0x04000000
#define OP_FLAGS_RIP_SUPPORT              0x08000000
#define OP_FLAGS_RIPDRIVE_ON              0x10000000

#define E_C (sysinfo.flags & OP_FLAGS_EXTRA_COLOR)

/****************************************************************************/

typedef struct {
  unsigned char
    sl,
    dsl,
    exempt;
  unsigned short
    ar,
    dar,
    restrict;
} asv_rec;

typedef struct {
  unsigned long
    flags,
    flags1;
  unsigned short
    spawn_opts[20];
  unsigned char
    newuser_colors[10],
    newuser_bwcolors[10],
    topscreen_color,
    f1_color,
    editline_color,
    chat_select_color,
    msg_color,
    ripdir[80],
    ripbbsid[30],
    ripmenudate[14];
  asv_rec
    asv;
  unsigned short
    mail_who_field_len,
    max_batch,
    max_extend_lines,
    max_chains,
    max_gfilesec,
    screen_saver_time;
  char
    rip_rm_adjust;
} system_operation_rec;


 /* RIP support constants */
#define M_DEFAULT   '3'         /* Default menu set */
#define MAXMENU '9'             /* # of possible PD menus */

#define submenu_code   11

/* Constants for cleared flag */
#define WASCLEARED  -1
#define NEEDCLEAR   -2
#define HYPCLEAR    -3

/****************************************************************************/

typedef struct
{
  int pos, curlen, maxlen;

  int first;

  int xpos, ypos, amount_to_show;
  int curxpos, curypos;

  int insert, curmode, bs, char_case;
  char *text;
  char *show;

} inputeditrec;


typedef struct
{
  int xpos, ypos, amount_to_show;
  int curxpos, curypos;

  int justify;
  int bg, cur_color;

  char *text;
  char *show;
} showtextrec;

struct individuals
{
  unsigned one : 1;
  unsigned two : 1;
  unsigned three : 1;
  unsigned four : 1;
  unsigned five : 1;
  unsigned six : 1;
  unsigned seven : 1;
  unsigned eight : 1;
  unsigned nine : 1;
  unsigned ten : 1;
  unsigned eleven : 1;
  unsigned twelve : 1;
  unsigned thirteen : 1;
  unsigned fourteen : 1;
  unsigned fifteen : 1;
  unsigned sixteen : 1;
};

struct nonindividual
{
  unsigned int ni;
};



union bitinfo
{
  struct individuals i;
  struct nonindividual n;
};

typedef struct
{
  union bitinfo bi;
  int xpos, ypos;
  int amount; // Amount of active items
  int pos;

  int check_char, fill_char;
} checkboxrec;



typedef struct
{
  int xpos, ypos;
  int amount; // Amount of active items
  int pos, last;

  int radio_char, fill_char;
}radiobuttonrec;



typedef union
{
    showtextrec    st;
    inputeditrec   ie;
    radiobuttonrec rb;
    checkboxrec    cb;
} VARI_TYPES;


struct VariMenuRec
{
  int amount; // Only used in the first record

  int type;

  VARI_TYPES *types;

  int xpos, ypos;  // copied from each type for easy access

  unsigned hotkey;
  int active;
  int returnvalue;

  struct VariMenuRec *next;
};

typedef struct VariMenuRec varimenurec;

typedef struct
{
  // mi=menu item, ci=current item
  unsigned mi_normal, mi_hi, ci_normal, ci_hi;
  unsigned inactive_color;
  int redraw; // COMMON_NONE, COMMON_PARTIAL (pos and last), COMMON_FULL
  int pos, last;

  int xpos, ypos;  // Where the cursor is right now

  unsigned event;
  int status;
  int returnvalue;
} varimenuinfo;


typedef struct
{
  int width;
  int amount_per_square;

  char square_list[10];

  int empty_space, side_char1, side_char2;
  int surround_color, box_color;

  long total_items, current_item;
  long last_maj_pos, last_min_pos;
} statusbarrec;


typedef struct
{
  int pos, curlen, maxlen;

  int first;

  int xpos, ypos, amount_to_show;
  int curxpos, curypos;

  int insert, curmode, bs, char_case;
  char *text;
  char *show;

  char *picture[256];
} inputpicrec;


/****************************************************************************/


/****************************************************************************/
#ifndef bbsmalloc
#ifdef __OS2__
#define bbsmalloc(x) malloc(x)
#define bbsfree(x) free(x)
#define bbscoreleft() coreleft()
#else
#define bbsmalloc(x) farmalloc(x)
#define bbsfree(x) farfree(x)
#define bbscoreleft() farcoreleft()
#if __TURBOC__ >= 0x0450
#define _chmod _rtl_chmod
#endif
#define ISR_VECT(n) ((n>7)?(0x68+n):(0x08+n))
#define ISR_CTRLR(n) ((n>7)?(0xa1):(0x21))
#define ISR_RESET(n) {outportb(0x20, 0x20); if (n>7) outportb(0xa0,0x20);}

#endif
#endif

#endif
