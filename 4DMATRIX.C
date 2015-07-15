/* ------------------------------------------------------------------------ */
#include <alloc.h>
                    /* conio.h used for testing lines */
#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <dir.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys\\stat.h>

#define my_version  "4DMatrix v1.17"
                    /*  maximum # bytes in a single message  */
#define MAX_MSG_SIZE    32000
                    /*  table size for echo areas  */
#define MAX_AREAS       150
                    /*  table size for equates of areas to BBS's  (50 x 50 = 2500)  */
#define MAX_FIX         2500
                    /*  table size for the SEEN-BY's  */
#define MAX_SEEN        250
                    /*  table size for the PATH's  */
#define MAX_PATH        100
                    /*  table size for disposition info  */
#define MAX_DISPOSITION 40
                    /*  table size for routing info  */
#define MAX_ROUTES      50
                    /*  table size for ARCmail info  */
#define MAX_ARCS        50
#define MAX_ARC_TYPES   5
                    /*  table size for passwords  */
#define MAX_PASSWORDS   50
                    /*  table size for message-limit info  */
#define MAX_LIMITS      20
                    /*  default outgoing message type if none specified
                        'H' - hold....  'C' - Crash....  'O' - Normal     */
#define DEFAULT_DISPOSITION 'H'



struct ffblk ffblk;


char *BINKpath = "";
int     outbound_file;
int     incoming_file;
FILE    *log_file;
int i;
int testing;
int verbose;
int logging;

int my_net;
int my_node;
int point_net;
int netmail_net;
int netmail_node;

int exit_status;

char    origin_handling;

struct  date    work_date;
struct  time    work_time;
struct  ffblk   ff;
long            work_long_secs;
/*  ----------------------------
 *  Misc global string variables
 *  ----------------------------  */

char    fido_title[255],            /* limited to 72 chars */
        fido_to_user[255],          /* limited to 36 chars */
        fido_from_user[255],        /* limited to 36 chars */
        fido_area_name[81],         /* area name for echomail */
        fido_origin[255],           /* Origin line for echomail */
        fido_EID_info[81],          /* Opus dupe-killer info */
        my_origin[81],              /* the " * Origin:" line text */
        my_tear[81],                /* the "---" tear-line text */
        via_line[81],               /* message for forwarded mail */
        arcmail_path[64],           /* the destination directory for ARCmail files */
        outbound_path[64],          /* the directory for outgoing packets */
        inbound_path[64],           /* the directory for incoming packets */
        frontdoor_path[64];         /* the directory for MSG file-attach messages */


int fido_from_point;                /* point # the message is from */
int fido_to_point;                  /* point # the message is to */



/*  --------------------------------
 *  definition of net/node structure
 *  --------------------------------  */
typedef struct {
    int net;
    int node;
} netnode;

netnode curr_out_netnode;       /* net/node of currently opened .PKT file */

netnode true_origin;

char    curr_out_disposition;   /* file-type "H"=.HUT "C"=.CUT, etc.  */


/*  ----------------------------------------------
 *  definition of beginning of message packet file
 *  ----------------------------------------------  */

typedef struct {
    int orig_node;
    int dest_node;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int baud;
    int packet_type;
    int orig_net;
    int dest_net;
    char product_code;
    char fill[33];
} packet_header_struct;

packet_header_struct   packet_header;


/*  ----------------------------
 *  definition of message header
 *  ----------------------------  */

typedef struct {
    int id;
    int orig_node;
    int dest_node;
    int orig_net;
    int dest_net;
    int attribute;
    int cost;
    char datetime[20];
} message_header_struct;

message_header_struct message_header;


/*  -----------------------------------------------------------------
 *  definition of configuration structure for other nodes in echo-net
 *  -----------------------------------------------------------------  */

typedef struct {
    netnode disposition_netnode;
    char    disposition_filetype;   /*  C=.CUT N=.OUT O=.OUT, H=.HUT, D=.DUT, etc.  */
} disposition_node_struct;

int curr_disposition_node;
int num_disposition_nodes;
disposition_node_struct     disposition[MAX_DISPOSITION];

/*  --------------------------------------------------------------------------  */
/*  Echomail control structures diagram.   Shows a sample AREA which is         */
/*  distributed to two other systems.                                           */
/*                                                                              */
/*       area[n]                                                                */
/*  -----------------                                                           */
/*  | type (E/G)    |                                                           */
/*  -----------------                                                           */
/*  | flags         |            areafix[x]                 areafix[y]          */
/*  -----------------       ---------------------       ---------------------   */
/*  |  pointer      |-----> | net/node | pointer|-----> | net/node | pointer|   */
/*  -----------------       ---------------------       ---------------------   */
/*  |  ECHO_NAME    |                                                   |       */
/*  -----------------                                                   |       */
/*                                                                     NULL     */
/*  --------------------------------------------------------------------------  */

/*  ----------------------------
 *  echomail distribution matrix
 *  ----------------------------  */
typedef struct areafix_type {
    netnode                 fixnode;
    struct areafix_type   * fixnext;
} areafix_struct;

areafix_struct  areafix[MAX_FIX];
int num_fixes;


/*  ----------------------------------------
 *  definition of configuration structure
 *  associating AREAs with the list of nodes
 *  who receive those areas.
 *  ----------------------------------------  */

typedef struct {
    char            areatype;
    int             areaflags;
    areafix_struct *arealist;
    char            areaname[81];
} area_struct;

int curr_area;
int num_areas;
area_struct     area[MAX_AREAS];
#define     areaflag_strip_SEEN       0x0001
#define     areaflag_strip_ORIGIN     0x0002
#define     areaflag_zonegate         0x0004
#define     areaflag_keep_pointnet    0x0008
#define     areaflag_manual_add       0x0010
#define     areaflag_manual_del       0x0020


/*  -----------------------
 *  echomail seen-by matrix
 *  -----------------------  */

netnode seen_by[MAX_SEEN];
netnode saw_by[MAX_SEEN];
int     num_seens;
int     num_saws;


/*  --------------------
 *  echomail PATH matrix
 *  --------------------  */

netnode path[MAX_PATH];
int     num_paths;


/*  -------------------
 *  Mail routing matrix
 *  -------------------  */

typedef struct {
    netnode route_from;
    netnode route_to;
} route_struct;

route_struct    route[MAX_ROUTES];
int             num_routes;


/*  ------------------
 *  ARCmail specifiers
 *  ------------------  */

typedef struct {
    netnode arc_to;
    char    arc_type;
    char    arc_flag;
} arcmail_struct;

arcmail_struct  arcmail[MAX_ARCS];
int     num_arcs;
int     num_arc_types;
char    arc_text[MAX_ARC_TYPES][80];

/*  ------------------------------
 *  message-count limit specifiers
 *  ------------------------------  */

typedef struct {
    netnode who;
    int     count;
    int     max;
    char    name;
} limit_struct;

limit_struct  limit[MAX_LIMITS];
int     num_limits;
int     curr_limit;         /*  -1 if no match  */

/*  ----------------
 *  Password storage
 *  ----------------  */

typedef struct {
    netnode         pass_who;
    unsigned char   pass_AR;
    char            pass_word[13];
} password_struct;

password_struct password[MAX_PASSWORDS];
int     num_passes;




void invent_pkt_name (char * string)
{
   struct tm *tp;
   time_t ltime;

   time (&ltime);
   tp = localtime (&ltime);
   sprintf (string, "%02d%02d%02d%02d.pkt",
            tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
}

static char *suffixes[8] = {
                            "SU", "MO", "TU", "WE", "TH", "FR", "SA", NULL
};

int is_arcmail (p, n)
char *p;
int n;
{
   int i;
   char c[128];

   if (!isdigit (p[n]))
      {
      return (0);
      }

   strcpy (c, p);
   strupr (c);

   for (i = n - 11; i < n - 3; i++)
      {
      if ((!isdigit (c[i])) && ((c[i] > 'F') || (c[i] < 'A')))
         return (0);
      }

   for (i = 0; i < 7; i++)
      {
      if (strnicmp (&c[n - 2], suffixes[i], 2) == 0)
         break;
      }

   if (i >= 7)
      {
      return (0);
      }
   return (1);
}




char upcase(char ch)
/* This converts a character to uppercase */
{
  if ((ch>96) && (ch<123))
    ch=ch-32;
  return(ch);
}





void outchr(unsigned char ch)
{
    write(outbound_file,(void *)&ch,1);
    if (testing) {
        _AL=ch;
        _AH=0x0e;
        _BL=0x07;
        geninterrupt(0x10);
    }
}






/*  -------------------------------------------------------------
 *  Process the configuration files.
 *  These files should reside in the main BBS subdirectory (the
 *  same subdirectory from which BBS.EXE is executed.)
 *  This is a plain ASCII file, and contains various lines which
 *  specify various options for program operation.   Each line must
 *  be terminated by a carriage-return character, and optionally
 *  a linefeed.   Note that this departs from UNIX/XENIX standards
 *  but follows MSDOS standards.   It is expected that other ports
 *  of this program will follow the conventions used on those
 *  systems.
 *  ------------------------------------------------------------- */
void get_areafix_config(void)
{
unsigned char    s[255];
unsigned char    wkstr[25];
int     areafix_config_file;
int     p,i;
char    area_type;
int     area_flags;
int     init_area;
int     config_linecount;
unsigned char    area_name[81];
unsigned char *  sline;
unsigned char *  wksp;
unsigned char *  temp_sp;
char    ch;
netnode user_netnode;
int     curr_net;
areafix_struct * last_fix;
areafix_struct * first_fix;

    config_linecount=0;
    num_fixes=0;

    my_net = -1;
    my_node = -1;
    strcpy(s,"AREAFIX.CFG");
    areafix_config_file=-1;
    if ((areafix_config_file=open(&s[0],O_RDONLY | O_BINARY))==NULL) {
        printf("error opening areafix_config_file file: %s. errno=%d\n",s,errno);
        exit_status=1;
        return;
    }
    p=0;
    sline=&s[0];
    while (read(areafix_config_file,(void *)&ch,1)) {
        if (ch!=0x0a){
            if (ch!=0x0d) {
                if (p < 255) {
                    s[p++]=ch;
                } else {
                    s[p]=0;
                    printf("Line %d too long in AREAFIX.CFG at %s\n",config_linecount,s);
                    exit_status=1;
                    return;
                }
            } else {
                ++config_linecount;
                s[p]=0;
                p=0;
                sline=&s[0];
                /*  ---------------------------
                 *  Skip any leading whitespace
                 *  ---------------------------  */
                while ((*sline == ' ')
                ||     (*sline == 0x09) )
                    ++sline;
                /*  -------------------------
                 *  get the name of this AREA
                 *  -------------------------  */
                if ((temp_sp=strchr(sline,' ')) == NULL) {
                    printf("error in format of areafix config file: Line %d\n", config_linecount);
                    exit_status=1;
                    return;
                }
                *temp_sp++=0;           /* make a terminator on the string */
                strcpy(area_name,sline);
                /*  ------------------------------------------------
                 *  check if it's a new area, or a continuation line
                 *  ------------------------------------------------  */
                init_area = -1;
                for (curr_area=0 ; curr_area < num_areas ; curr_area++) {
                    if (!strcmp(area[curr_area].areaname,area_name)) {
                        init_area = curr_area;
                        break;
                    }
                }
                /*  ---------------------------------------------------
                 *  if it's a new area, begin a fresh area[] entry
                 *  if it's an existing echo, initialize linkage info
                 *  to point to the last entry already present.
                 *  ---------------------------------------------------  */
                first_fix=NULL;
                if (init_area == -1) {
                    init_area = num_areas;
                    last_fix = NULL;
                } else {
                    first_fix = area[init_area].arealist;
                    last_fix = first_fix;
                    while ((last_fix -> fixnext) != NULL) {
                        last_fix = last_fix -> fixnext;
                    }
                }
                sline=temp_sp;           /* point to next char on line */
                while ((*sline == ' ')
                ||     (*sline == 0x09) )
                    ++sline;
                area_type=upcase(*sline++);
                if ((area_type != 'G')
                &&  (area_type != 'E')) {
                    printf("bad Echomail/Groupmail flag in AREAFIX.CFG Line %d at '%s'\n", config_linecount,sline);
                    exit_status=1;
                    return;
                }
                while ((*sline == ' ')
                ||     (*sline == 0x09) )
                    ++sline;
                /*  ---------------------------------
                 *  check for optional echomail flags
                 *  ---------------------------------  */
                area_flags=0;
                if (*sline == '^') {
                    sscanf(++sline,"%x",&area_flags);
                    if (testing) {
                        printf("Found area_flags = %x\n",area_flags);
                    }
                    while ((*sline != ' ')
                    &&     (*sline != 0x09))
                        ++sline;
                }

                while ((*sline == ' ')
                ||     (*sline == 0x09))
                    ++sline;
                /*  ----------------------------------------------------
                 *  build the linked list of nodes who receive this echo
                 *  ----------------------------------------------------  */
                curr_net=0;
                for(;;) {
                    /*  -----------------------
                     *  skip any leading blanks
                     *  -----------------------  */
                    while ((*sline == ' ')
                    ||     (*sline == 0x09))
                        ++sline;
                    /*  ------------------------
                     *  exit when at end of line
                     *  ------------------------  */
                    if ((*sline == 0)
                    ||  (*sline == 0x8d)
                    ||  (*sline == 0x0d)
                    ||  (*sline == 0x0a)) {
                        break;                  /* goes to comment "sline_exit" */
                    }
                    /*  -----------------------------------------------
                     *  copy next blank-delimited string into work-area
                     *  -----------------------------------------------  */
                    wksp=&wkstr[0];
                    i=0;
                    while ((*sline != ' ')
                    &&     (*sline != 0)
                    &&     (*sline != 0x8d)
                    &&     (*sline != 0x0d)
                    &&     (*sline != 0x0a)) {
                        *wksp++ = *sline++;
                        if ((++i) >= 20)        /*  enforce string length limit  */
                            --wksp;
                    }
                    *wksp=0;
                    if (testing) {
                        printf("Area %s, node: %s\n",area_name,wkstr);
                    }
                    /*  -------------------------------------------------
                     *  determine if this is a net/node, or just a node #
                     *  -------------------------------------------------  */
                    if ((wksp=strchr(&wkstr[0],'/')) != NULL) {
                        user_netnode.net=atoi(&wkstr[0]);
                        ++wksp;
                        if (!strnicmp(wksp,"ALL",3)) {
                            user_netnode.node=-1;
                        } else {
                            user_netnode.node=atoi(wksp);
                        }
                        curr_net=user_netnode.net;
                    } else {
                        user_netnode.net=curr_net;
                        user_netnode.node=atoi(&wkstr[0]);
                    }
                    /*  --------------------------------------
                     *  add it into the echo distribution list
                     *  --------------------------------------  */
                    if (num_fixes >= MAX_FIX){
                        printf("Too many area::net/node associations in AREAFIX.CFG - need to recompile\n");
                        exit_status=4;
                        return;
                    }
                    /* -------------------------------------------------------------------------------------- *\
                    |  if this is the first echo in the list, save the pointer to put in the "area" structure  |
                    \* -------------------------------------------------------------------------------------- */
                    if (first_fix == NULL) {
                        first_fix = &areafix[num_fixes];
                        if (testing) {
                            printf("First node this area\n");
                        }
                    }
                    /* --------------------------------------------------------------------------- *\
                    |  if there was a previous node in this list, go patch it to point to this one  |
                    \* --------------------------------------------------------------------------- */
                    if (last_fix != NULL) {
                        last_fix -> fixnext = &areafix[num_fixes];
                    }
                    areafix[num_fixes].fixnext  = NULL;
                    areafix[num_fixes].fixnode  = user_netnode;
                    last_fix=&areafix[num_fixes];
                    ++num_fixes;
                }
/* break above exits to here "sline_exit" */
                if (num_areas >= MAX_AREAS){
                    printf("Too many AREA lines in AREAFIX.CFG - need to recompile\n");
                    exit_status=4;
                    return;
                }
                if (first_fix == NULL){
                    printf("Warning - area %s is not sent anywhere - line %d\n",
                                            area_name,
                                            config_linecount);
                }

                area[init_area].areatype    = area_type;
                area[init_area].areaflags   = area_flags;
                strcpy(area[init_area].areaname,area_name);
                area[init_area].arealist    = first_fix;
                if (init_area == num_areas) {
                    num_areas++;
                }
            }
        }
    }
    close(areafix_config_file);
    if (testing) {
        printf("%d areas.\n",num_areas);
        for (i=0 ; i<num_areas ; i++) {
            printf("Area: %s type %c flags %04x sent to :", area[i].areaname, area[i].areatype, area[i].areaflags);
            if (area[i].arealist == NULL) {
                printf("*NOBODY*");
            } else {
                last_fix = area[i].arealist;
                while (last_fix != NULL) {
                    printf("%d/%d ",(last_fix -> fixnode.net), (last_fix -> fixnode.node));
                    last_fix = last_fix -> fixnext;
                }
            }
            printf("\n");
        }
    }
}





void get_routing(unsigned char * sline)
{
netnode curr_to;
int curr_net;
int net;
int node;
int i;
char    wkstr[20];
unsigned char * wksp;


    /*  ------------------------
     *  Get the node to route TO
     *  ------------------------  */
    while ((*sline == ' ')
    ||     (*sline == 0x09) )
        ++sline;
    curr_to.net=atoi(sline);
    if ((sline=strchr(sline,'/')) != NULL) {
        curr_to.node=atoi(++sline);
    } else {
        printf("Expecting slash at line %s\n",sline);
    }
    sline=strchr(sline,' ');
    if (testing) {
        printf("Routing to node %d/%d\n",curr_to.net,curr_to.node);
    }
    /*  -----------------------------------
     *  get the list of nodes to route FROM
     *  -----------------------------------  */
    curr_net=0;
    for(;;) {
        /*  -----------------------
         *  skip any leading blanks
         *  -----------------------  */
        while ((*sline == ' ')
        ||     (*sline == 0x09))
            ++sline;
        /*  ------------------------
         *  exit when at end of line
         *  ------------------------  */
        if ((*sline == 0)
        ||  (*sline == 0x8d)
        ||  (*sline == 0x0d)
        ||  (*sline == 0x0a))
            return;
        /*  -----------------------------------------------
         *  copy next blank-delimited string into work-area
         *  -----------------------------------------------  */
        wksp=&wkstr[0];
        i=0;
        while ((*sline != ' ')
        &&     (*sline != 0)
        &&     (*sline != 0x8d)
        &&     (*sline != 0x0d)
        &&     (*sline != 0x0a)) {
            *wksp++ = *sline++;
            if ((++i) >= 20)        /*  enforce string length limit  */
                --wksp;
        }
        *wksp=0;
        /*  -------------------------------------------------
         *  determine if this is a net/node, or just a node #
         *  -------------------------------------------------  */
        if ((wksp=strchr(&wkstr[0],'/')) != NULL) {
            net=atoi(&wkstr[0]);
            ++wksp;
            if (!strnicmp(wksp,"ALL",3)) {
                node=-1;
            } else {
                node=atoi(wksp);
            }
            curr_net=net;
        } else {
            net=curr_net;
            node=atoi(&wkstr[0]);
        }
        /*  --------------------------
         *  add it into the route list
         *  --------------------------  */
        if (num_routes > MAX_ROUTES) {
            printf("Too many routing entries - must recompile\n");
            --num_routes;
        }
        route[num_routes].route_from.net  = net;
        route[num_routes].route_from.node = node;
        route[num_routes].route_to.net    = curr_to.net;
        route[num_routes].route_to.node   = curr_to.node;
        ++num_routes;
    }
}







void add_disposition_list(char disp_type,unsigned char * sline)
{
int curr_net;
int net;
int node;
int i;
char    wkstr[20];
unsigned char * wksp;

    curr_net=0;
    for(;;) {
        /*  -----------------------
         *  skip any leading blanks
         *  -----------------------  */
        while ((*sline == ' ')
        ||     (*sline == 0x09))
            ++sline;
        /*  ------------------------
         *  exit when at end of line
         *  ------------------------  */
        if ((*sline == 0)
        ||  (*sline == 0x8d)
        ||  (*sline == 0x0d)
        ||  (*sline == 0x0a))
            return;
        /*  -----------------------------------------------
         *  copy next blank-delimited string into work-area
         *  -----------------------------------------------  */
        wksp=&wkstr[0];
        i=0;
        while ((*sline != ' ')
        &&     (*sline != 0)
        &&     (*sline != 0x8d)
        &&     (*sline != 0x0d)
        &&     (*sline != 0x0a)) {
            *wksp++ = *sline++;
            if ((++i) >= 20)        /*  enforce string length limit  */
                --wksp;
        }
        *wksp=0;
        /*  -------------------------------------------------
         *  determine if this is a net/node, or just a node #
         *  -------------------------------------------------  */
        if ((wksp=strchr(&wkstr[0],'/')) != NULL) {
            net=atoi(&wkstr[0]);
            ++wksp;
            if (!strnicmp(wksp,"ALL",3)) {
                node=-1;
            } else {
                node=atoi(wksp);
            }
            curr_net=net;
        } else {
            net=curr_net;
            node=atoi(&wkstr[0]);
        }
        /*  --------------------------------
         *  add it into the disposition list
         *  --------------------------------  */
        if (num_disposition_nodes > MAX_DISPOSITION) {
            printf("Too many disposition entries - must recompile\n");
            --num_disposition_nodes;
        }
        disposition[num_disposition_nodes].disposition_netnode.net=net;
        disposition[num_disposition_nodes].disposition_netnode.node=node;
        disposition[num_disposition_nodes].disposition_filetype=disp_type;
        ++num_disposition_nodes;
    }
}






void add_arcmail_list(unsigned char * sline)
{
int curr_net;
int net;
int node;
int i;
char    curr_type;
char    wkstr[20];
unsigned char * wksp;
unsigned char * save_sline;

    save_sline = sline;
    curr_net=0;
    curr_type=0;
    for(;;) {
        /*  -----------------------
         *  skip any leading blanks
         *  -----------------------  */
        while ((*sline == ' ')
        ||     (*sline == 0x09))
            ++sline;
        /*  ------------------------
         *  exit when at end of line
         *  ------------------------  */
        if ((*sline == 0)
        ||  (*sline == 0x8d)
        ||  (*sline == 0x0d)
        ||  (*sline == 0x0a))
            return;
        /*  -----------------------------------------------
         *  copy next blank-delimited string into work-area
         *  -----------------------------------------------  */
        wksp=&wkstr[0];
        i=0;
        while ((*sline != ' ')
        &&     (*sline != 0)
        &&     (*sline != 0x8d)
        &&     (*sline != 0x0d)
        &&     (*sline != 0x0a)) {
            *wksp++ = *sline++;
            if ((++i) >= 20)        /*  enforce string length limit  */
                --wksp;
        }
        *wksp=0;
        /*  -------------------------------------------------------------
         *  if this item begins with "^", then it's an ARC-type indicator
         *  -------------------------------------------------------------  */
        if (wkstr[0] == '^') {
            i = atoi(&wkstr[1]);
            if (i >= num_arc_types) {
                printf("Invalid arctype at line %s\n",save_sline);
            } else {
                if (testing)
                    printf("type %d\n",i);
                curr_type = i;
            }
        } else {
            /*  -------------------------------------------------
             *  determine if this is a net/node, or just a node #
             *  -------------------------------------------------  */
            if ((wksp=strchr(&wkstr[0],'/')) != NULL) {
                net=atoi(&wkstr[0]);
                ++wksp;
                node=atoi(wksp);
                curr_net=net;
            } else {
                net=curr_net;
                node=atoi(&wkstr[0]);
            }
            /*  --------------------------------
             *  add it into the disposition list
             *  --------------------------------  */
            if (num_arcs > MAX_ARCS) {
                printf("Too many ARCmail entries - must recompile\n");
                --num_arcs;
            }
            arcmail[num_arcs].arc_to.net  = net;
            arcmail[num_arcs].arc_to.node = node;
            arcmail[num_arcs].arc_type    = curr_type;
            arcmail[num_arcs].arc_flag    = 0;
            ++num_arcs;
        }
    }
}






/*  -------------------------------------------------------------
 *  Process the configuration files.
 *  These files should reside in the main BBS subdirectory (the
 *  same subdirectory from which BBS.EXE is executed.)
 *  This is a plain ASCII file, and contains various lines which
 *  specify various options for program operation.   Each line must
 *  be terminated by a carriage-return character, and optionally
 *  a linefeed.   Note that this departs from UNIX/XENIX standards
 *  but follows MSDOS standards.   It is expected that other ports
 *  of this program will follow the conventions used on those
 *  systems.
 *  ------------------------------------------------------------- */
void get_system_config(void)
{
char    s[255];
int     system_config_file;
int     p;
int     config_linecount;
unsigned int    wk_word;
char *  wksp;
char    ch;

    config_linecount=0;

    my_net = -1;
    my_node = -1;
    point_net = -1;
    num_routes = 0;
    num_arcs = 0;
    num_arc_types = 0;
    netmail_net = 0;
    num_passes = 0;
    num_limits = 0;
    curr_limit = -1;
    strcpy(s,"4DMATRIX.CFG");
    system_config_file=-1;
    if ((system_config_file=open(&s[0],O_RDONLY | O_BINARY))==NULL) {
        printf("error opening system_config_file file: %s. errno=%d\n",s,errno);
        exit_status=1;
        return;
    }
    p=0;
    while (read(system_config_file,(void *)&ch,1)) {
        if (ch!=0x0a){
            if (ch!=0x0d) {
                if (p < 255) {
                    s[p++]=ch;
                } else {
                    s[p]=0;
                    printf("Line %d too long in 4DMATRIX.CFG at %s\n",config_linecount,s);
                    exit_status=1;
                    return;
                }
            } else {
                ++config_linecount;
                s[p]=0;
                p=0;
                wksp=&s[0];
                /*  ---------------------------
                 *  Skip any leading whitespace
                 *  ---------------------------  */
                while ((*wksp == ' ')
                ||     (*wksp == 0x09) )
                    ++wksp;
                /*  ----------------------------------
                 *  get the node number of this system
                 *  ----------------------------------  */
                if (!strnicmp(wksp,"MYNODE",6)) {
                    wksp=&s[6];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    my_net=atoi(wksp);
                    if ((wksp=strchr(wksp,'/')) != NULL) {
                        my_node=atoi(++wksp);
                    } else {
                        printf("Expecting slash at line %s\n",s);
                    }
                    if (testing) {
                        printf("Mynode is %d/%d\n",my_net,my_node);
                    }
                }
                /*  ----------------------------------
                 *  get the password for another node
                 *  ----------------------------------  */
                if (!strnicmp(wksp,"PASSWORD",8)) {
                    wksp=&s[8];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    password[num_passes].pass_who.net=atoi(wksp);
                    if ((wksp=strchr(wksp,'/')) != NULL) {
                        password[num_passes].pass_who.node=atoi(++wksp);
                    } else {
                        printf("Expecting slash at line %s\n",s);
                    }
                    wksp=strchr(wksp,' ');
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    /*  ----------------------------------
                     *  check for optional password access
                     *  ----------------------------------  */
                    wk_word = 0;
                    if (*wksp == '^') {
                        sscanf(++wksp,"%x",&wk_word);
                        if (testing) {
                            printf("Found password AR = %x\n",wk_word);
                        }
                        while ((*wksp != ' ')
                        &&     (*wksp != 0x09))
                            ++wksp;
                    }
                    password[num_passes].pass_AR = wk_word;

                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09))
                        ++wksp;
                    *(wksp+12)=0;                   /*  force limit of 12 characters */
                    strcpy(password[num_passes].pass_word,wksp);
                    if (num_passes >= MAX_PASSWORDS) {
                        printf("At line %d -- too many passwords\n",config_linecount);
                        exit_status=1;
                        return;
                    }
                    ++num_passes;
                }
                /*  ---------------------------------------------
                 *  allow for sending netmail to alternate system
                 *  ---------------------------------------------  */
                if (!strnicmp(wksp,"NETMAIL",7)) {
                    wksp=&s[7];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    netmail_net=atoi(wksp);
                    if ((wksp=strchr(wksp,'/')) != NULL) {
                        netmail_node=atoi(++wksp);
                    } else {
                        printf("Expecting slash at line %s\n",s);
                    }
                    if (testing) {
                        printf("Netmail node is %d/%d\n",netmail_net,netmail_node);
                    }
                }
                /*  --------------------------------------
                 *  get the node number of the private net
                 *  --------------------------------------  */
                else if (!strnicmp(wksp,"POINTNET",8)) {
                    wksp=&s[8];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    point_net=atoi(wksp);
                    if (testing) {
                        printf("Point Net is %d\n",point_net);
                    }
                }
                /*  -----------------------------------------------
                 *  limits of how many messages per file for a node
                 *  -----------------------------------------------  */
                if (!strnicmp(wksp,"LIMIT",5)) {
                    wksp=&s[5];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    limit[num_limits].who.net = atoi(wksp);
                    if ((wksp=strchr(wksp,'/')) != NULL) {
                        limit[num_limits].who.node = atoi(++wksp);
                    } else {
                        printf("Expecting slash at line %s\n",s);
                    }
                    if ((wksp=strchr(wksp,' ')) != NULL) {
                        while ((*wksp == ' ')
                        ||     (*wksp == 0x09) )
                            ++wksp;
                        limit[num_limits].max = atoi(wksp);
                    } else {
                        printf("Expecting slash at line %s\n",s);
                    }
                    limit[num_limits].count = 0;
                    limit[num_limits].name  = '0';
                    ++num_limits;
                    if (num_limits >= MAX_LIMITS) {
                        printf("Max number of LIMIT lines exceeded\n");
                        --num_limits;
                    }
                }
                /*  -----------------------------------
                 *  get any special routing information
                 *  -----------------------------------  */
                else if (!strnicmp(wksp,"ROUTE",5)) {
                    wksp=&s[5];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    get_routing(wksp);
                }
                /*  ----------------------------
                 *  save list of any crash nodes
                 *  ----------------------------  */
                else if (!strnicmp(wksp,"CRASH",5)) {
                    wksp=&s[5];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    add_disposition_list('C',wksp);
                }
                /*  --------------------------------------
                 *  save list of any continuous-mail nodes
                 *  --------------------------------------  */
                else if (!strnicmp(wksp,"CONT",4)) {
                    wksp=&s[4];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    add_disposition_list('C',wksp);
                }
                /*  --------------------------------
                 *  save list of any hold-mail nodes
                 *  --------------------------------  */
                else if (!strnicmp(wksp,"HOLD",4)) {
                    wksp=&s[4];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    add_disposition_list('H',wksp);
                }
                /*  ----------------------------------
                 *  save list of any normal-mail nodes
                 *  ----------------------------------  */
                else if (!strnicmp(wksp,"NORM",4)) {
                    wksp=&s[4];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    add_disposition_list('O',wksp);
                }
                /*  -------------------------------
                 *  get the text for forwarded mail
                 *  -------------------------------  */
                else if (!strnicmp(wksp,"VIA",3)) {
                    wksp=&s[3];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    strcpy(via_line,wksp);
                    if (testing) {
                        printf("Via Line is %s\n",via_line);
                    }
                }
                /*  --------------------------------------------
                 *  get the text to execute the ARCmail archiver
                 *  --------------------------------------------  */
                else if (!strnicmp(wksp,"ARCTEXT",7)) {
                    wksp=&s[7];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    if (num_arc_types >= MAX_ARC_TYPES) {
                        --num_arc_types;
                        printf("Too many ARCTEXT lines\n");
                    }
                    strcpy(&arc_text[num_arc_types++][0],wksp);
                    if (testing) {
                        printf("ARC type %d is %s\n",num_arc_types,wksp);
                    }
                }
                /*  --------------------------------
                 *  get the nodes to send ARCmail to
                 *  --------------------------------  */
                else if (!strnicmp(wksp,"ARCMAIL",7)) {
                    wksp=&s[7];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    if (num_arcs >= MAX_ARCS) {
                        --num_arcs;
                        printf("Too many ARCMAIL lines\n");
                    }
                    add_arcmail_list(wksp);
                }
                /*  -------------------------------------
                 *  get the name of the inbound directory
                 *  -------------------------------------  */
                else if (!strnicmp(wksp,"INPATH",6)) {
                    wksp=&s[6];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    strcpy(inbound_path,wksp);
                    if (testing) {
                        printf("Inbound Path is %s\n",inbound_path);
                    }
                }
                /*  -------------------------------------
                 *  get the name of the outbound directory
                 *  -------------------------------------  */
                else if (!strnicmp(wksp,"OUTPATH",7)) {
                    wksp=&s[7];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    strcpy(outbound_path,wksp);
                    if (testing) {
                        printf("Outbound Path is %s\n",outbound_path);
                    }
                }
                /*  -------------------------------------
                 *  get the name of the arcmail directory
                 *  -------------------------------------  */
                else if (!strnicmp(wksp,"ARCPATH",7)) {
                    wksp=&s[7];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    strcpy(arcmail_path,wksp);
                    if (testing) {
                        printf("ARCmail Path is %s\n",arcmail_path);
                    }
                }
                /*  ------------------------------------------------
                 *  get the name of the Frontdoor MSG file directory
                 *  ------------------------------------------------  */
                else if (!strnicmp(wksp,"FRONTDOOR",9)) {
                    wksp=&s[9];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    strcpy(frontdoor_path,wksp);
                    if (testing) {
                        printf("Frontdoor Path is %s\n",frontdoor_path);
                    }
                }
                /*  ----------------------------------------
                 *  determine how to handle bad Origin lines
                 *  ----------------------------------------  */
                else if (!strnicmp(wksp,"BADORIGIN",9)) {
                    wksp=&s[9];
                    while ((*wksp == ' ')
                    ||     (*wksp == 0x09) )
                        ++wksp;
                    origin_handling=atoi(wksp);
                    if (testing) {
                        switch (origin_handling) {
                            case 0:     printf("Bad origin: no action\n");              break;
                            case 1:     printf("Bad origin: truncate from right\n");    break;
                            case 2:     printf("Bad origin: truncate from left\n");     break;
                            case 3:     printf("Bad origin: Make new origin line\n");   break;
                            default:    printf("Bad origin: -----invalid value-----\007\n");
                        }
                    }
                }
            }
        }
    }
    close(system_config_file);
    if (netmail_net == 0) {
        netmail_net  = my_net;
        netmail_node = my_node;
    }
    if (testing) {
        printf("%d dispositions.\n",num_disposition_nodes);
        for (i=0 ; i<num_disposition_nodes ; i++) {
            printf("Disposition of %d/%d is %c\n",
                    disposition[i].disposition_netnode.net,
                    disposition[i].disposition_netnode.node,
                    disposition[i].disposition_filetype);
        }
        printf("%d routes.\n",num_routes);
        for (i=0 ; i<num_routes ; i++) {
            printf("Routing %d/%d to %d/%d\n",
                route[i].route_from.net,
                route[i].route_from.node,
                route[i].route_to.net,
                route[i].route_to.node);
        }
        printf("%d ARCmail entries\n",num_arcs);
        for (i=0 ; i<num_arcs ; i++) {
            printf("Arcing mail to %d/%d by method %d\n",
                arcmail[i].arc_to.net,
                arcmail[i].arc_to.node,
                arcmail[i].arc_type);
        }
        printf("%d passwords\n",num_passes);
        for (i=0; i<num_passes ; i++) {
            printf("Password for %d/%d (access %02x) is %s\n",
                password[i].pass_who.net,
                password[i].pass_who.node,
                password[i].pass_AR,
                password[i].pass_word);
        }
        printf("%d limits\n",num_limits);
        for (i=0; i<num_limits ; i++) {
            printf("Limit node %d/%d to %d messages\n", limit[i].who.net,
                                                        limit[i].who.node,
                                                        limit[i].max);
        }
    }
}









/*  ----------------------------------------------------------------
 *  open a file which is to be used to pack messages into
 *  if the file doesn't exist, create one and write the file header.
 *  if there's already one, just extend it
 *  ----------------------------------------------------------------  */
void open_outbound_packet(netnode who_to, unsigned char out_disposition)
{
char fn[65],wk_filetype[12];
char    new_limit;

    strcpy(wk_filetype,"?UT");
    wk_filetype[0]=out_disposition;
    /*  ------------------------------------------------------
     *  check if the message limit has been exceeded
     *  ------------------------------------------------------  */
    new_limit = 'N';
    if (curr_limit >= 0) {
        if (limit[curr_limit].count >= limit[curr_limit].max) {
            limit[curr_limit].count = 0;
            limit[curr_limit].name += 1;
            if (limit[curr_limit].name == '9'+1) {
                limit[curr_limit].name = 'A';
            }
            new_limit = 'Y';
        }
    }
    /*  ---------------------------------------------------------
     *  if the file is already open, check if it's the right file
     *  ---------------------------------------------------------  */
    if (outbound_file > 0) {
        /*  ------------------------------------------------------
         *  it's open already - if it's the right file, do nothing
         *  ------------------------------------------------------  */
        if ((curr_out_netnode.net == who_to.net)
        &&  (curr_out_netnode.node == who_to.node)
        &&  (curr_out_disposition  == out_disposition)
        &&  (new_limit == 'N')) {
            if (curr_limit >= 0) {
                limit[curr_limit].count += 1;
            }
            return;
        }
        /*  ----------------------------------------------------------------
         *  it's not the right file, mark the end of the old file & close it
         *  ----------------------------------------------------------------  */
        outchr(00);
        outchr(00);             /* message-type == 0000 (hex) terminates packet */
        close(outbound_file);
    }
    /*  --------------------------------------------------------------
     *  look through ARCmail status and mark this node if it's present
     *  --------------------------------------------------------------  */
    for (i=0 ; i<num_arcs ; i++) {
        if ((arcmail[i].arc_to.net  == who_to.net)
        &&  (arcmail[i].arc_to.node == who_to.node)) {
            arcmail[i].arc_flag = 1;
        }
    }
    /*  -----------------------------------------------------------
     *  look through file-counts and see if it needs a special name
     *  -----------------------------------------------------------  */
    curr_limit = -1;
    for (i=0 ; i<num_limits ; i++) {
        if ((limit[i].who.net  == who_to.net)
        &&  (limit[i].who.node == who_to.node)) {
            /*  -------------------------------------
             *  the node is in the limit list
             *  set the filetype to that special type
             *  -------------------------------------  */
            strcpy(wk_filetype,"LMx");
            wk_filetype[2]=limit[i].name;
            limit[i].count += 1;
            curr_limit = i;
        }
    }
    /*  ----------------------------------------------------------
     *  at this point, the file is closed (or never opened)
     *  build the filename to be used so that it can be opened
     *  the filename is built from the net/node of the destination
     *  by converting its net and node numbers into 4-byte hex
     *  ASCII strings.
     *  ----------------------------------------------------------  */
    sprintf(fn,"%s%04x%04x.%s",  outbound_path,
                            who_to.net,
                            who_to.node,
                            wk_filetype);
    if (testing)
        printf("Opening outbound file: %s\n",fn);
    outbound_file=-1;
    outbound_file=open(fn,O_RDWR | O_BINARY);
    if (outbound_file<0) {
        /*  --------------------------------------------------------------
         *  The file doesn't already exist - make one and write the header
         *  --------------------------------------------------------------  */
        outbound_file=open(fn,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        if (outbound_file<0) {
            printf("error opening outbound file '%s'. errno=%d\n",fn,errno);
            exit_status=3;
            return;
        }
        getdate(&work_date);
        gettime(&work_time);
        packet_header.orig_node     =my_node;
        packet_header.dest_node     =who_to.node;
        packet_header.year          =work_date.da_year;
        packet_header.month         =work_date.da_mon;
        packet_header.day           =work_date.da_day;
        packet_header.hour          =work_time.ti_hour;
        packet_header.minute        =work_time.ti_min;
        packet_header.second        =work_time.ti_sec;
        packet_header.baud          =0;
        packet_header.packet_type   =0x0002;
        packet_header.orig_net      =my_net;
        packet_header.dest_net      =who_to.net;
        packet_header.product_code  =0x2c;  /* my very own product code! */
        write(outbound_file,(void *)&packet_header.orig_node,sizeof(packet_header_struct));
    } else {
        /*  ----------------------------------------------------------
         *  the file already exists
         *  back up over the previous end-of-packet marker (two nulls)
         *  ----------------------------------------------------------  */
        lseek(outbound_file,-2L,SEEK_END);
    }
    curr_out_netnode.net = who_to.net;
    curr_out_netnode.node = who_to.node;
    curr_out_disposition  = out_disposition;
}




netnode route_where(netnode who_to)
{
int     route_count;
int     wk_count;
char    found_route;
    /*  -------------------------------------------------------------------------
     *  Scan through routing matrix to see if this message should be re-addressed
     *  Apply routing recursively - continue loop until no further routing found.
     *  count passes through loop in case circular routing is established
     *  -------------------------------------------------------------------------  */
    route_count = 0;
    found_route = 'Y';
    while ((found_route == 'Y') && (route_count++ < MAX_ROUTES)) {
        found_route = 'N';
        for (wk_count = 0 ; wk_count < num_routes ; wk_count++) {
            if  (route[wk_count].route_from.net  == who_to.net ) {
                if ((route[wk_count].route_from.node == who_to.node)
                ||  (route[wk_count].route_from.node == -1         )) {
                    found_route = 'Y';
                    if (testing) {
                        printf("Re-routing from %d/%d to %d/%d\n",
                            who_to.net,
                            who_to.node,
                            route[wk_count].route_to.net,
                            route[wk_count].route_to.node);
                    }
                    who_to.net  = route[wk_count].route_to.net;
                    who_to.node = route[wk_count].route_to.node;
                }
            }
        }
    }
    return(who_to);
}



char    actual_disposition(netnode who_to)
{
disposition_node_struct * disp_ptr;
char    wk_disposition;
int     wk_count;
    /*  --------------------------------
     *  determine the output disposition
     *  --------------------------------  */
    wk_disposition = ' ';
    disp_ptr = &disposition[0];
    for (wk_count = 0;  ((wk_count < num_disposition_nodes) && (wk_disposition == ' '));  wk_count++,disp_ptr++) {
        if (((disp_ptr -> disposition_netnode.net)  == who_to.net)
        &&  ((disp_ptr -> disposition_netnode.node) == who_to.node)) {
            wk_disposition = (disp_ptr -> disposition_filetype);
        }
        /*  --------------------------------------------------
         *  match the net if the disposition says to match ALL
         *  --------------------------------------------------  */
        if (((disp_ptr -> disposition_netnode.net)  == who_to.net)
        &&  ((disp_ptr -> disposition_netnode.node) == -1)) {
            wk_disposition = (disp_ptr -> disposition_filetype);
        }
    }
    /*  ---------------------------------------------
     *  if no routing found, default to HOLD filetype
     *  ---------------------------------------------  */
    if (wk_disposition == ' ') {
        wk_disposition = DEFAULT_DISPOSITION;
    }
    return(wk_disposition);
}






/*  -------------------------------------------------------
 *  Send a message to the node whose address is in the
 *  message header.   The message text is passed to this
 *  function as a pointer, since it's dynamically
 *  allocated at run-time.   The message header and other
 *  control information is all passed in global data areas.
 *  -------------------------------------------------------  */
void send_message(char * msg_text, netnode to_who, char out_disposition)
{
char    wkstr[255];
char    seen_str[20];
int     wk_count;
int     last_net;
netnode who_to;
unsigned char    wk_disposition;
char    * wksp;

    who_to.net=to_who.net;
    who_to.node=to_who.node;
    if (testing) {
        printf("Sending message to %d/%d\n",who_to.net,who_to.node);
    }
    /*  -------------------------------------------
     *  the message header gets the address of the
     *  destination net/node, regardless of routing
     *  -------------------------------------------  */
    message_header.dest_net  = who_to.net;
    message_header.dest_node = who_to.node;
    who_to=route_where(who_to);
    if (out_disposition == ' ') {
        wk_disposition = actual_disposition(who_to);
    } else {
        wk_disposition = out_disposition;
    }
    open_outbound_packet(who_to,wk_disposition);
    /*  -------------------------------------------
     *  special kludge for Opus 19-byte date format
     *  and for Ben Mann's CHKLST program with NO date field
     *  -------------------------------------------  */
    write(outbound_file,(void *)&message_header.id,14);
    write(outbound_file,(void *)&message_header.datetime,(strlen(message_header.datetime)+1));
    write(outbound_file,(void *)&fido_to_user,(strlen(fido_to_user)+1));
    write(outbound_file,(void *)&fido_from_user,(strlen(fido_from_user)+1));
    write(outbound_file,(void *)&fido_title,(strlen(fido_title)+1));
    /*  ---------------------------------------------------------
     *  The "AREA" line  (Required for conference mail (Echomail)
     *  ---------------------------------------------------------  */
    if (fido_area_name[0]) {
        strcpy(wkstr,"AREA:");
        strcat(wkstr,fido_area_name);
        strcat(wkstr,"\015\012");        /* hard return */
        write(outbound_file,(void *)&wkstr,strlen(wkstr));
    }
    /*  ---------------------------------
     *  propogate any existing ^aEID info
     *  ---------------------------------  */
    if (fido_EID_info[0]) {
        strcpy(wkstr,"\001EID: ");
        strcat(wkstr,fido_EID_info);
        strcat(wkstr,"\015\012");        /* hard return */
        write(outbound_file,(void *)&wkstr,strlen(wkstr));
    }
    /*  ----------------------------------------------------------------
     *  If the message is addressed to a point, include the ^aTOPT: line
     *  ----------------------------------------------------------------  */
    if (fido_to_point != 0){
        strcpy(wkstr,"\001TOPT ");
        itoa(fido_to_point,&wkstr[6],10);
        strcat(wkstr,"\015\012");                   /* trailing CR/LF */
        write(outbound_file,(void *)wkstr,(strlen(wkstr)));
    }
    /*  ------------------------------------------------------------------
     *  If the message is addressed from a point, include the ^aFMPT: line
     *  ------------------------------------------------------------------  */
    if (fido_from_point != 0){
        strcpy(wkstr,"\001FMPT ");
        itoa(fido_from_point,&wkstr[6],10);
        strcat(wkstr,"\015\012");                   /* trailing CR/LF */
        write(outbound_file,(void *)wkstr,(strlen(wkstr)));
    }
    /*  -----------------------
     *  the message text itself
     *  -----------------------  */
    write(outbound_file,(void *)msg_text,(strlen(msg_text)));
    /*  ---------------
     *  The Origin line
     *  ---------------  */
    if (fido_origin[0]) {
        /*  ---------------
         *  the ORIGIN line
         *  ---------------  */
        strcpy(wkstr," * Origin: ");
        if (strlen(fido_origin) > 69) {
            /*  ---------------------------------------------------
             *  The origin line was made too long - determine which
             *  method should be used to handle it
             *  ---------------------------------------------------  */
            switch (origin_handling) {
            case 0:                 /*  do nothing (within reason)  */
                fido_origin[254]=0;
                break;
            case 1:                 /*  truncate  */
                fido_origin[69]=0;
                break;
            case 2:                 /*  shift left to valid length */
                fido_origin[254]=0;
                while (strlen(fido_origin) > 69) {
                    strcpy(&fido_origin[0], &fido_origin[1]);
                }
                break;
            case 3:                 /*  complain about it  */
                strcat(wkstr," ** INVALID ORIGIN LINE ** ");
                wksp = strrchr(fido_origin,'(');        /*  find last left paren */
                if ((wksp != NULL)
                &&  (strlen(wksp) < 25)) {
                    strcpy(fido_origin,wksp);
                } else {
                    fido_origin[0]=0;
                }
                break;
            }
        }
        strcat(wkstr,fido_origin);
        strcat(wkstr,"\015\012");                     /* hard return */
        write(outbound_file,(void *)&wkstr,strlen(wkstr));
    }
    /*  ----------------
     *  The SEEN-BY line
     *  ----------------  */
    wk_count=0;
    last_net=-1;
    wkstr[0]=0;
    for (wk_count=0 ; wk_count < num_seens ; wk_count++) {
        if (!wkstr[0])
            strcpy(wkstr,"SEEN-BY:");
        seen_str[0]=' ';
        /*  ---------------------------------------------------------
         *  Don't output local point nodes into the outgoing SEEN-BYs
         *  ---------------------------------------------------------  */
        if ((seen_by[wk_count].net != point_net)
        ||  ((area[curr_area].areaflags & areaflag_keep_pointnet) != 0)) {
            if (seen_by[wk_count].net == last_net) {
                itoa(seen_by[wk_count].node,&seen_str[1],10);
                strcat(wkstr,seen_str);
            } else {
                itoa(seen_by[wk_count].net,&seen_str[1],10);
                strcat(wkstr,seen_str);
                seen_str[0]='/';
                itoa(seen_by[wk_count].node,&seen_str[1],10);
                strcat(wkstr,seen_str);
            }
        }
        last_net=seen_by[wk_count].net;
        /*  -----------------------------------------------------------
         *  Write out the line if it gets too long, or if it's all done
         *  -----------------------------------------------------------  */
        if ((strlen(wkstr) > 70)
        ||  (wk_count == (num_seens - 1))) {
            strcat(wkstr,"\015\012");      /* hard return */
            write(outbound_file,(void *)&wkstr,strlen(wkstr));
            wkstr[0]=0;
            last_net=0;
        }
    }
    /*  ----------------
     *  The ^A PATH line
     *  ----------------  */
    wk_count=0;
    last_net=-1;
    wkstr[0]=0;
    for (wk_count=0 ; wk_count < num_paths ; wk_count++) {
        if (!wkstr[0])
            strcpy(wkstr,"\001PATH:");
        seen_str[0]=' ';
        if (path[wk_count].net == last_net) {
            itoa(path[wk_count].node,&seen_str[1],10);
        } else {
            itoa(path[wk_count].net,&seen_str[1],10);
            strcat(wkstr,seen_str);
            seen_str[0]='/';
            itoa(path[wk_count].node,&seen_str[1],10);
            last_net=path[wk_count].net;
        }
        strcat(wkstr,seen_str);
        /*  -----------------------------------------------------------
         *  Write out the line if it gets too long, or if it's all done
         *  -----------------------------------------------------------  */
        if ((strlen(wkstr) > 70)
        ||  (wk_count == (num_paths - 1))) {
            strcat(wkstr,"\015\012");      /* hard return */
            write(outbound_file,(void *)&wkstr,strlen(wkstr));
            wkstr[0]=0;
            last_net=0;
        }
    }
    outchr(0);                      /* a null to terminate the message area */
}










/*  -----------------------------------
 *  Update the AREAFIX.CFG file on disk
 *  -----------------------------------  */
void update_areafix_config(void)
{
unsigned char wkstr[81];
unsigned char * wksp;
area_struct     *af_ptr;
int             af_file;
int             curr_net;
areafix_struct  *fix_ptr;
     
    strcpy(wkstr,"AREAFIX.CFG");
    unlink(wkstr);
    af_file = open(wkstr,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    if (af_file<0) {
        printf("error opening areafix file. errno=%d\n",errno);
        exit_status=3;
        return;
    }
    for (curr_area = 0 ; curr_area < num_areas ; curr_area++) {
        af_ptr = &area[curr_area];
        fix_ptr = af_ptr -> arealist;
        curr_net = 0;
        wksp=&wkstr[0];
        wkstr[0]=0;
        while (fix_ptr != NULL) {
            if (strlen(wkstr) > 70) {
                    sprintf(wksp,"\015\012");
                    write(af_file, wkstr, strlen(wkstr));
                    wkstr[0]=0;
                    wksp=&wkstr[0];
                    curr_net=0;
            }
            if (curr_net == 0) {
                sprintf(wkstr, "%-12s %c ^%04x ",
                                    af_ptr -> areaname,
                                    af_ptr -> areatype,
                                    af_ptr -> areaflags);
                wksp=strchr(wksp,0);
            }
            if ((fix_ptr -> fixnode.net) == curr_net) {
                sprintf(wksp, " %d", fix_ptr -> fixnode.node);
            } else {
                sprintf(wksp, " %d/%d",
                                fix_ptr -> fixnode.net,
                                fix_ptr -> fixnode.node);
            }
            wksp=strchr(wksp,0);
            curr_net = fix_ptr -> fixnode.net;
            fix_ptr = fix_ptr -> fixnext;
        }
        if (curr_net != 0) {
                sprintf(wksp,"\015\012");
                write(af_file, wkstr, strlen(wkstr));
                wkstr[0]=0;
                wksp=&wkstr[0];
                curr_net=0;
        }
    }
    close(af_file);
}












/*  -------------------------------------------------------
 *  Process a request for AREAFIX.   Add or delete echos
 *  from the areafix list as directed by the message, then
 *  write out the areafix list into a new AREAFIX.CFG file.
 *  -------------------------------------------------------  */
void process_areafix_request(char * msg_text)
{
unsigned char   echo_name[81];
netnode         who_to;
char            made_change;
char            areafix_error;
char            found_area;
char            done;
char            list_requested;
char            password_valid;
unsigned char   *wksp;
unsigned char   *tosp;
areafix_struct  *fix_ptr;
areafix_struct  *prev_fix;
areafix_struct  *next_fix;
areafix_struct  *last_fix;
areafix_struct  *look_fix;
char            *reply;
char            *reply_ptr;
int             curr_pass_index;


    if ((reply=(char *)farmalloc(10000)) == NULL) {
        printf("Allocation error for areafix reply text\n");
        exit_status = 8;
        farfree((void *)reply);
        return;
    }
    reply_ptr=reply;
    sprintf(reply_ptr,"AreaFix Report for %d/%d\015\012",
                        true_origin.net,
                        true_origin.node);
    reply_ptr = strchr(reply_ptr,0);
    /*  ----------------
     *  get the password
     *  ----------------  */
    tosp=&echo_name[0];
    wksp=&fido_title[0];
    while ((*wksp != 0x0d)
    &&     (*wksp != 0x8d)
    &&     (*wksp != 0x0a)
    &&     (*wksp != 0x20)
    &&     (*wksp != 0x00)) {
        *tosp++ = *wksp++;
    }
    *tosp=0;
    /*  ------------------
     *  check the password
     *  ------------------ */
    password_valid = 'N';
    for (curr_pass_index=0 ; curr_pass_index<num_passes ; curr_pass_index++) {
        if ((password[curr_pass_index].pass_who.net  == true_origin.net)
        &&  (password[curr_pass_index].pass_who.node == true_origin.node)) {
            if (stricmp(password[curr_pass_index].pass_word,echo_name) == 0) {
                password_valid = 'Y';
            }
        }
    }
    if (password_valid != 'Y') {
        sprintf(reply_ptr,"Password error for %d/%d - no changes will be recorded\015\012",
                            true_origin.net,
                            true_origin.node);
        reply_ptr = strchr(reply_ptr,0);
        if (testing) {
            printf("reply='%s'\n\n",reply);
        }
    }
    /*  --------------------
     *  look for a -L switch
     *  --------------------  */
    list_requested = 'N';
    while (*wksp == 0x20) {
        ++wksp;
    }
    if (*wksp == '-') {
        ++wksp;
        if (*wksp == 'L') {
            list_requested = 'Y';
        }
    }
    /*  -------------------
     *  process the message
     *  -------------------  */
    wksp=&msg_text[0];
    if (testing) {
        printf("<<<<<<<<<<<<<  AREAFIX input  >>>>>>>>>>\n");
        printf("%s",wksp);
        printf("<<<<<<<<<<<<<  AREAFIX input  >>>>>>>>>>\n");
    }
    while (*wksp != 0) {
        while ((*wksp == 0x0d)
        ||     (*wksp == 0x8d)
        ||     (*wksp == 0x0a))
            ++wksp;
        /*  ----------------------------------------------
         *  see if it's an add request or a delete request
         *  ----------------------------------------------  */
        if (*wksp == '-') {
            ++wksp;                             /*  skip the (first) dash  */
            if (*wksp == '-') {                 /*  look for the tear line  */
                break;
            }
            /*  -------------------------------------------------
             *  it's a delete request.   Get the name of the echo
             *  -------------------------------------------------  */
            tosp = &echo_name[0];
            while ((*wksp != 0x0d)
            &&     (*wksp != 0x8d)
            &&     (*wksp != 0x00)) {
                *tosp++ = *wksp++;
            }
            *tosp=0;
            if (testing) {
                printf("Trying to delete echo '%s'\n",echo_name);
            }
            /*  -----------------------------------------
             *  search for the list for the matching area
             *  -----------------------------------------  */
            made_change = 'N';
            areafix_error = 'N';
            found_area = 'N';
            for (curr_area=0 ; ((curr_area < num_areas) && (found_area == 'N')) ; curr_area++ ) {
                /* -------------------------------------------------------------- *\
                |  in order to find a match, it must:                              |
                |  match the area-name, of course                                  |
                |  if the area has no AR bits set, that's all                      |
                |  if the area has any AR bits set, then the user's AR must match  |
                \* -------------------------------------------------------------- */
                if ((!strcmp(area[curr_area].areaname,echo_name))
                &&  ((((area[curr_area].areaflags>>8) & password[curr_pass_index].pass_AR) != 0)
                    ||((area[curr_area].areaflags>>8) == 0) ) ) {
                    found_area = 'Y';
                    /*  --------------------------------------------------------
                     *  found the AREA - look through distribution for this node
                     *  --------------------------------------------------------  */
                    fix_ptr=area[curr_area].arealist;
                    prev_fix = NULL;
                    while ((fix_ptr       != NULL)
                    &&     (made_change   == 'N')
                    &&     (areafix_error == 'N')) {
                        if (((fix_ptr -> fixnode.net)  == true_origin.net)
                        &&  ((fix_ptr -> fixnode.node) == true_origin.node)) {
                            /*  -------------------------------------------------------------------
                             *  he's the first one in the list - mark the change in the area_struct
                             *  -------------------------------------------------------------------  */
                            if (prev_fix == NULL) {
                                area[curr_area].arealist = (fix_ptr -> fixnext);
                            } else {
                                /*  -----------------------------------------------
                                 *  he's in the middle (or at the end) of the list.
                                 *  link his predecessor to his successor
                                 *  -----------------------------------------------  */
                                (prev_fix -> fixnext) = (fix_ptr -> fixnext);
                            }
                            if ((area[curr_area].areaflags & areaflag_manual_del) != 0) {
                                areafix_error = 'Y';
                                sprintf(reply_ptr,
                                            "Area: %s - this area is required: not deleted by %d/%d\015\012"
                                            ,echo_name
                                            ,true_origin.net
                                            ,true_origin.node);
                                reply_ptr = strchr(reply_ptr,0);
                                if (testing) {
                                    printf("reply='%s'\n\n",reply);
                                }
                            } else {
                                made_change='Y';
                                sprintf(reply_ptr,
                                            "Area: %s - deleted %d/%d from scan\015\012"
                                            ,echo_name
                                            ,true_origin.net
                                            ,true_origin.node);
                                reply_ptr = strchr(reply_ptr,0);
                                if (testing) {
                                    printf("reply='%s'\n\n",reply);
                                }
                            }
                        }
                        prev_fix = fix_ptr;
                        fix_ptr = (fix_ptr -> fixnext);
                    }                                           /*  end while  */
                }                                           /*  end if  */
            }                                           /*  end for */
            if (found_area != 'Y') {
                sprintf(reply_ptr,
                        "Can't delete echo '%s', node %d/%d not already receiving it\015\012"
                        ,echo_name
                        ,true_origin.net
                        ,true_origin.node);
                reply_ptr = strchr(reply_ptr,0);
                if (testing) {
                    printf("reply='%s'\n\n",reply);
                }
            }
        } else {
            /*  ----------------------------------------------
             *  it's an add request.  Get the name of the echo
             *  ----------------------------------------------  */
            tosp = &echo_name[0];
            while ((*wksp != 0x0d)
            &&     (*wksp != 0x8d)
            &&     (*wksp != 0x00)) {
                *tosp++ = *wksp++;
            }
            *tosp=0;
            if (testing) {
                printf("Trying to add echo '%s'\n",echo_name);
            }
            /*  -----------------------------------------
             *  search for the list for the matching area
             *  -----------------------------------------  */
            found_area = 'N';
            areafix_error = 'N';
            for (curr_area=0 ; ((curr_area < num_areas) && (found_area == 'N')) ; curr_area++ ) {
                /* -------------------------------------------------------------- *\
                |  in order to find a match, it must:                              |
                |  match the area-name, of course                                  |
                |  if the area has no AR bits set, that's all                      |
                |  if the area has any AR bits set, then the user's AR must match  |
                \* -------------------------------------------------------------- */
                if ((!strcmp(area[curr_area].areaname,echo_name))
                &&  ((((area[curr_area].areaflags>>8) & password[curr_pass_index].pass_AR) != 0)
                    ||((area[curr_area].areaflags>>8) == 0) ) ) {
                    if (testing) {
                        printf("Found area: %s\n",echo_name);
                    }
                    found_area = 'Y';
                    /*  --------------------------------------------------------
                     *  found the AREA - look through distribution for this node
                     *  --------------------------------------------------------  */
                    fix_ptr=area[curr_area].arealist;
                    prev_fix = NULL;
                    made_change = 'N';
                    while ((made_change   == 'N')
                    &&     (areafix_error == 'N')) {
                        if (fix_ptr != NULL) {
                            if (testing) {
                                printf("Looking at node: %d/%d\n",
                                        fix_ptr -> fixnode.net,
                                        fix_ptr -> fixnode.node);
                            }
                            if (((fix_ptr -> fixnode.net)  == true_origin.net)
                            &&  ((fix_ptr -> fixnode.node) == true_origin.node)) {
                                if (testing) {
                                    printf("Found node match\n");
                                }
                                /*  --------------------------------------------
                                 *  he's already in the list - don't do anything
                                 *  --------------------------------------------  */
                                areafix_error = 'Y';
                                sprintf(reply_ptr,
                                        "Can't add echo '%s', node %d/%d already receiving it\015\012"
                                        ,echo_name
                                        ,true_origin.net
                                        ,true_origin.node);
                                reply_ptr = strchr(reply_ptr,0);
                                if (testing) {
                                    printf("reply='%s'\n\n",reply);
                                }
                            }
                        } else {
                            /*  (fix_ptr == NULL)  */
                            /*  ----------------------------------------------
                             *  He's not already there - check if there's room
                             *  ----------------------------------------------  */
                            if (num_fixes >= MAX_FIX){
                                areafix_error = 'Y';
                                sprintf(reply_ptr,
                                        "Can't add echo '%s', for node %d/%d no room in internal tables\015\012"
                                        ,echo_name
                                        ,true_origin.net
                                        ,true_origin.node);
                                reply_ptr = strchr(reply_ptr,0);
                                if (testing) {
                                    printf("reply='%s'\n\n",reply);
                                }
                            }
                            if ((area[curr_area].areaflags & areaflag_manual_add) != 0) {
                                areafix_error = 'Y';
                                sprintf(reply_ptr,
                                            "Area: %s - this is a restricted area: not added by %d/%d\015\012"
                                            ,echo_name
                                            ,true_origin.net
                                            ,true_origin.node);
                                reply_ptr = strchr(reply_ptr,0);
                                if (testing) {
                                    printf("reply='%s'\n\n",reply);
                                }
                            } else {
                                if (areafix_error != 'Y') {
                                    /*  ------------------------------------
                                     *  allocate a new areafix entry for him
                                     *  ------------------------------------  */
                                    next_fix=&areafix[num_fixes];
                                    (next_fix -> fixnext)  = NULL;
                                    (next_fix -> fixnode.net)  = true_origin.net;
                                    (next_fix -> fixnode.node) = true_origin.node;
                                    ++num_fixes;
                                    /*  ------------------------------------
                                     *  link the new entry onto the old list
                                     *  ------------------------------------  */
                                    if (prev_fix == NULL) {
                                        /*  ----------------------------------------------------
                                         *  the list is currently empty - make him the 1st entry
                                         *  ----------------------------------------------------  */
                                        area[curr_area].arealist = next_fix;
                                    } else {
                                        /*  -----------------------------------------------
                                         *  link him into the list in order by net/node #'s
                                         *  -----------------------------------------------  */
                                        done=0;
                                        last_fix=NULL;
                                        look_fix=area[curr_area].arealist;
                                        while ((look_fix != NULL)
                                        &&     (!done)            ) {
                                            /* --------------------------------------------------------------------- *\
                                            |  it's not the end of the list.  Add him in before the first net/node    |
                                            |  which is larger than his own.                                          |
                                            \* --------------------------------------------------------------------- */
                                            if ( (  (look_fix -> fixnode.net)  > true_origin.net)
                                            ||   (  ((look_fix -> fixnode.net)  == true_origin.net)
                                            &&      ((look_fix -> fixnode.node) >  true_origin.node))   ) {
                                                if (last_fix == NULL) {
                                                    area[curr_area].arealist = next_fix;
                                                } else {
                                                    last_fix -> fixnext = next_fix;
                                                }
                                                next_fix -> fixnext = look_fix;
                                                done=1;
                                            }
                                            last_fix = look_fix;
                                            look_fix = (look_fix -> fixnext);
                                        }
                                        if (!done) {
                                            /* ----------------------------------- *\
                                            |  if the end of the list was reached,  |
                                            |  this is the highest net/node,        |
                                            |  so add him on the end                |
                                            \* ----------------------------------- */
                                            if (last_fix == NULL) {
                                                area[curr_area].arealist = next_fix;
                                            } else {
                                                last_fix -> fixnext = next_fix;
                                            }
                                        }
                                    }
                                    made_change='Y';
                                    sprintf(reply_ptr,
                                            "echo '%s' added for node %d/%d\015\012"
                                            ,echo_name
                                            ,true_origin.net
                                            ,true_origin.node);
                                    reply_ptr = strchr(reply_ptr,0);
                                    if (testing) {
                                        printf("reply='%s'\n\n",reply);
                                    }
                                }
                            }
                        }
                        prev_fix = fix_ptr;
                        fix_ptr = (fix_ptr -> fixnext);
                    }                                           /*  end while  */
                }                                           /*  end if  */
            }                                           /*  end for */
            if (found_area != 'Y') {
                areafix_error = 'Y';
                sprintf(reply_ptr,
                        "add request for echo '%s' denied:  Echo not available\015\012",
                        echo_name);
                reply_ptr = strchr(reply_ptr,0);
                if (testing) {
                    printf("reply='%s'\n\n",reply);
                }
            }
        }                                           /*  end if (*wksp == '-')  */
    }                                           /*  end while (*wksp != 0)  */
    /*  ---------------------------------------------
     *  Print a copy of the results into the log file
     *  ---------------------------------------------  */
    printf("<<<<<<<<<<<<<  AREAFIX  >>>>>>>>>>\n");
    printf("%s",reply);
    printf("**********************************\n");
    /*  ------------------------------------------
     *  if he requested a list, let's give him one
     *  ------------------------------------------  */
    if (list_requested == 'Y') {
        sprintf(reply_ptr,"\015\012Areaname (* indicates you are set to receive it)\015\012");
        reply_ptr = strchr(reply_ptr,0);
        sprintf(reply_ptr,"----------       -----\015\012");
        reply_ptr = strchr(reply_ptr,0);
        for (curr_area=0 ; (curr_area < num_areas) ; curr_area++ ) {
            if (  ((area[curr_area].areaflags>>8) == 0)
            ||   (((area[curr_area].areaflags>>8) & password[curr_pass_index].pass_AR) != 0) ) {
                sprintf(reply_ptr,"%-20s",area[curr_area].areaname);
                reply_ptr = strchr(reply_ptr,0);
                fix_ptr=area[curr_area].arealist;
                while (fix_ptr != NULL) {
                    if (((fix_ptr -> fixnode.net)  == true_origin.net)
                    &&  ((fix_ptr -> fixnode.node) == true_origin.node)) {
                        sprintf(reply_ptr,"*");
                        reply_ptr = strchr(reply_ptr,0);
                    }
                    fix_ptr = (fix_ptr -> fixnext);
                }                                           /*  end while  */
                sprintf(reply_ptr,"\015\012");
                reply_ptr = strchr(reply_ptr,0);
            }                                           /*  end for */
        }
    }
    /*  ----------------------------------------------------
     *  send a net-mail reply back to tell him what happened
     *  ----------------------------------------------------  */
    who_to.net  = true_origin.net;
    who_to.node = true_origin.node;
    message_header.orig_net  = my_net;
    message_header.orig_node = my_node;
    strcpy(fido_to_user,fido_from_user);
    strcpy(fido_from_user,my_version);
    strcat(fido_from_user," (Areafix)");
    strcpy(fido_title,"Echo change request");
    fido_to_point = fido_from_point;
    fido_from_point = 0;
    fido_origin[0]=0;
    num_seens=0;
    num_paths=0;
    send_message(reply, who_to, ' ');
    farfree((void *)reply);
    /*  ----------------------------
     *  record the changes for later
     *  ----------------------------  */
    if (password_valid == 'Y') {
        update_areafix_config();
    }
}













void add_text_seen_by(unsigned char * sline)
{
int curr_net;
int net;
int node;
int i;
char    wkstr[20];
unsigned char * wksp;

    curr_net=0;
    for(;;) {
        /*  -----------------------
         *  skip any leading blanks
         *  -----------------------  */
        while (*sline == ' ')
            ++sline;
        /*  ------------------------
         *  exit when at end of line
         *  ------------------------  */
        if ((*sline == 0)
        ||  (*sline == 0x8d)
        ||  (*sline == 0x0d)
        ||  (*sline == 0x0a))
            return;
        /*  -----------------------------------------------
         *  copy next blank-delimited string into work-area
         *  -----------------------------------------------  */
        wksp=&wkstr[0];
        i=0;
        while ((*sline != ' ')
        &&     (*sline != 0)
        &&     (*sline != 0x8d)
        &&     (*sline != 0x0d)
        &&     (*sline != 0x0a)) {
            *wksp++ = *sline++;
            if ((++i) >= 20)        /*  enforce string length limit  */
                --wksp;
        }
        *wksp=0;
        /*  -------------------------------------------------
         *  determine if this is a net/node, or just a node #
         *  -------------------------------------------------  */
        if ((wksp=strchr(&wkstr[0],'/')) != NULL) {
            net=atoi(&wkstr[0]);
            node=atoi(++wksp);
            curr_net=net;
        } else {
            net=curr_net;
            node=atoi(&wkstr[0]);
        }
        /*  ----------------------------
         *  add it into the seen-by list
         *  ----------------------------  */
        if (num_seens > MAX_SEEN) {
            printf("Too many SEEN-BY entries - must recompile\n");
            --num_seens;
        }
        seen_by[num_seens].net=net;
        seen_by[num_seens].node=node;
        ++num_seens;
    }
}







void add_text_path(unsigned char * sline)
{
int curr_net;
int net;
int node;
int i;
unsigned char    wkstr[20];
unsigned char * wksp;

    curr_net=0;
    for(;;) {
        /*  -----------------------
         *  skip any leading blanks
         *  -----------------------  */
        while (*sline == ' ')
            ++sline;
        /*  ------------------------
         *  exit when at end of line
         *  ------------------------  */
        if ((*sline == 0)
        ||  (*sline == 0x8d)
        ||  (*sline == 0x0d)
        ||  (*sline == 0x0a))
            return;
        /*  -----------------------------------------------
         *  copy next blank-delimited string into work-area
         *  -----------------------------------------------  */
        wksp=&wkstr[0];
        i=0;
        while ((*sline != ' ')
        &&     (*sline != 0)
        &&     (*sline != 0x8d)
        &&     (*sline != 0x0d)
        &&     (*sline != 0x0a)) {
            *wksp++ = *sline++;
            if ((++i) >= 20)        /*  enforce string length limit  */
                --wksp;
        }
        *wksp=0;
        /*  -------------------------------------------------
         *  determine if this is a net/node, or just a node #
         *  -------------------------------------------------  */
        if ((wksp=strchr(&wkstr[0],'/')) != NULL) {
            net=atoi(&wkstr[0]);
            node=atoi(++wksp);
            curr_net=net;
        } else {
            net=curr_net;
            node=atoi(&wkstr[0]);
        }
        /*  ----------------------------
         *  add it into the seen-by list
         *  ----------------------------  */
        if (num_paths > MAX_PATH) {
            printf("Too many PATH entries - must recompile\n",num_paths);
            --num_paths;
        }
        path[num_paths].net=net;
        path[num_paths].node=node;
        ++num_paths;
    }
}







void get_AREA(unsigned char * msg)
{
int wk_area;
unsigned char * fan_ptr;
    if (testing) {
        printf("in get_AREA\n");
    }
    while (*msg == ' ')
        ++msg;
    curr_area = -1;
    fan_ptr=&fido_area_name[0];
    while ((*msg != ' ')
    &&     (*msg != 0x09)
    &&     (*msg != 0x0d)
    &&     (*msg != 0x8d))
        *fan_ptr++=*msg++;
    *fan_ptr = 0;
    for (wk_area=0;wk_area<num_areas;wk_area++) {
        if (!stricmp(area[wk_area].areaname,fido_area_name)) {
            curr_area=wk_area;
            break;
        }
    }
    if (testing)
        printf("curr_area=%d, areaflags=%04x\n",
                        curr_area,
                        area[curr_area].areaflags);
}





int get_point(char * msg)
{
    while (*msg == ' ')
        ++msg;
    return(atoi(msg));
}







void copyline(unsigned char * dest, unsigned char * src)
{
    while ((*src != 0x0d)
    &&     (*src != 0x8d)
    &&     (*src != 0x0a)
    &&     (*src != 0x00))
        *dest++=*src++;
    --src;
    if ((*src == 0x0d)
    ||  (*src == 0x8d))
        if (*++src == 0x0a)
            *dest++=*src++;
    *dest=0;
}











void delete_msg_line(unsigned char * mptr)
{
unsigned char * svptr;

    /*  ----------------------------
     *  save beginning point of line
     *  ----------------------------  */
    svptr=mptr;
    /*  -------------------
     *  Skip to end of line
     *  -------------------  */
    while ((*mptr != 0)
    &&     (*mptr != 0x8d)
    &&     (*mptr != 0x0d)
    &&     (*mptr != 0x0a))
        ++mptr;
    /*  ------------------------------
     *  Skip over CR/LF at end of line
     *  ------------------------------  */
    if ((*mptr == 0x0d)
    ||  (*mptr == 0x8d)) {
        ++mptr;
        if (*mptr == 0x0a)
            ++mptr;
    }
    strcpy(svptr,mptr);
}




void dissect_message(char * msg)
{
unsigned char * wksp;
char * start_this_line;
char    had_origin, had_seen;

    /*  ----------------------------------------------
     *  Strip the control information from the message
     *  ---------------------------------------------- */
    num_seens=0;
    num_paths=0;
    fido_to_point=0;
    fido_from_point=0;
    curr_area=-1;
    fido_EID_info[0]=0;
    fido_origin[0]=0;
    fido_area_name[0]=0;
    had_origin='N';
    had_seen='N';

    wksp=&msg[0];
    while (*wksp != 0) {
        start_this_line=wksp;
        /*  -----------------------
         *  SEEN-BY: and ^aSEEN-BY:
         *  -----------------------  */
        if (strncmp(wksp,"SEEN-BY:",8) == 0) {
            add_text_seen_by(wksp+8);
            wksp=start_this_line;
            delete_msg_line(wksp);
            had_seen='Y';
        } else if (strncmp(wksp,"\001SEEN-BY:",9) == 0) {
            add_text_seen_by(wksp+9);
            wksp=start_this_line;
            delete_msg_line(wksp);
            had_seen='Y';
        /*  -----------------
         *  AREA: and ^aAREA:
         *  -----------------  */
        } else if ((strncmp(wksp,"AREA:",5) == 0)
               &&  (fido_area_name[0]       == 0) ) {
            if (testing) {
                printf("calling get_AREA(+5)\n");
            }
            get_AREA(wksp+5);
            wksp=start_this_line;
            delete_msg_line(wksp);
        } else if ((strncmp(wksp,"\001AREA:",6) == 0)
               &&  (fido_area_name[0]           == 0) ) {
            if (testing) {
                printf("calling get_AREA(+6)\n");
            }
            get_AREA(wksp+6);
            wksp=start_this_line;
            delete_msg_line(wksp);
        /*  -------
         *  ^aPATH:
         *  -------  */
        } else if (strncmp(wksp,"\001PATH:",6) == 0) {
            add_text_path(wksp+6);
            wksp=start_this_line;
            delete_msg_line(wksp);
        /*  -----------
         *  ^aTOPT line
         *  -----------  */
        } else if (strncmp(wksp,"\001TOPT:",6) == 0) {
            fido_to_point = get_point(wksp+6);
            wksp=start_this_line;
            delete_msg_line(wksp);
        /*  -------------------------------------
         *  ^aTOPT line from SEAdog - not to spec
         *  -------------------------------------  */
        } else if (strncmp(wksp,"\001TOPT",5) == 0) {
            fido_to_point = get_point(wksp+5);
            wksp=start_this_line;
            delete_msg_line(wksp);
        /*  -----------
         *  ^aFMPT line
         *  -----------  */
        } else if (strncmp(wksp,"\001FMPT:",6) == 0) {
            fido_from_point = get_point(wksp+6);
            wksp=start_this_line;
            delete_msg_line(wksp);
        /*  -------------------------------------
         *  ^aFMPT line from SEAdog - not to spec
         *  -------------------------------------  */
        } else if (strncmp(wksp,"\001FMPT",5) == 0) {
            fido_from_point = get_point(wksp+5);
            wksp=start_this_line;
            delete_msg_line(wksp);
        /*  ----------
         *  ^aEID line
         *  ----------  */
        } else if (strncmp(wksp,"\001EID:",5) == 0) {
            copyline(fido_EID_info,wksp+5);
            wksp=start_this_line;
            delete_msg_line(wksp);
        /*  -----------
         *  Origin line
         *  -----------  */
        } else if (strncmp(wksp," * Origin:",10) == 0) {
            wksp=wksp+10;
            while (*wksp == ' ') {
                ++wksp;
            }
            copyline(fido_origin,wksp);
            wksp=start_this_line;
            delete_msg_line(wksp);
            had_origin='Y';
        } else {
            if ( (had_seen == 'Y')
            &&   (had_origin == 'Y')
            &&   (strncmp(wksp,"\015\012",2) == 0) ) {
                /*  ----------------------------------------------------------
                 *  it's a null line after the Origin & Seen-by's - discard it
                 *  ----------------------------------------------------------  */
                wksp=start_this_line;
                delete_msg_line(wksp);
            } else {
                /*  --------------------------------------
                 *  unrecognised line - must be plain text
                 *  skip to end of line and skip CR/LF
                 *  --------------------------------------  */
                while ((*wksp != 0x0d)
                &&     (*wksp != 0x8d)
                &&     (*wksp != 0x00))
                    ++wksp;
                while ((*wksp == 0x0d)
                ||     (*wksp == 0x8d)
                ||     (*wksp == 0x0a))
                    ++wksp;
            }
        }
    }
}





void    forward_group_mail(unsigned char * b)
{
    return;
}




void    forward_echo_mail(unsigned char * b)
{
areafix_struct  *fix_ptr;
char            found_node;
int             wk_count;

    if (testing) {
        printf("Old SEEN-BY list:\n");
        for (wk_count = 0 ; wk_count < num_saws ; wk_count++) {
            printf("%d/%d ",saw_by[wk_count].net, saw_by[wk_count].node);
        }
        printf("\n\nNew SEEN-BY list:\n");
        for (wk_count = 0 ; wk_count < num_seens ; wk_count++) {
            printf("%d/%d ",seen_by[wk_count].net, seen_by[wk_count].node);
        }
        printf("\n\n");
    }
    /*  ---------------------------------------------------
     *  echomail compatibility kludge  (Former "-s" switch)
     *  apparently ConfMail in secure mode needs this
     *  ---------------------------------------------------  */
    message_header.orig_net  = my_net;
    message_header.orig_node = my_node;
    /*  --------------------------------------
     *  check SEEN-BY's to see if already seen
     *  --------------------------------------  */
    fix_ptr=area[curr_area].arealist;
    while (fix_ptr != NULL) {
        if (testing) {
            printf("Examining %d/%d whether to send echomail\n",(fix_ptr -> fixnode.net),(fix_ptr -> fixnode.node));
        }
        /*  -----------------------------------------------------------------------
         *  look through original SEEN-BY's to see if this node's already seen this
         *  -----------------------------------------------------------------------  */
        wk_count=0;
        found_node = 'N';
        for (wk_count=0 ; wk_count < num_saws ; wk_count++) {
            if ((saw_by[wk_count].net  == (fix_ptr -> fixnode.net))
            &&  (saw_by[wk_count].node == (fix_ptr -> fixnode.node))) {
                /*  ----------------------------------------------------------------
                 *  distribute mail to myself since SEEN-BY's already have me listed
                 *  ----------------------------------------------------------------  */
                if (((fix_ptr -> fixnode.net)  != my_net)
                ||  ((fix_ptr -> fixnode.node) != my_node)) {
                    found_node = 'Y';
                    if (testing) {
                        printf("Not sent due to SEEN-BY already there\n");
                    }
                }
                break;
            }
        }
        /*  ----------------------------------------------------------------
         *  but don't send back a message I generated
         *  Nor send back any message to its source
         *  This "should" never happen, but just to be safe....
         *  ----------------------------------------------------------------  */
        if (((fix_ptr -> fixnode.net)  == true_origin.net)
        &&  ((fix_ptr -> fixnode.node) == true_origin.node)) {
            found_node = 'Y';
            if (testing) {
                printf("Not sent due to origin = dest\n");
            }
        }
        /*  --------------------------------------------
         *  if he hasn't already seen it, send it to him
         *  --------------------------------------------  */
        if (found_node == 'N') {
            if (testing) {
                printf("Sending message\n");
            }
            send_message(b,(fix_ptr ->fixnode),' ');    /*  use default routing  */
        }
        fix_ptr = (fix_ptr -> fixnext);
    }
}





void    sort_seen_by_list(void)
{
netnode low_seen;       /* the lowest found this pass */
netnode wk_seen;        /* current working entry */
netnode base_seen;      /* save area for first entry of each sub-list */
int base_index;         /* index to base of this sub-list */
int wk_index;           /* index to current working entry */
int low_index;          /* index to lowest found this pass */

    /*  --------------------------------------------------
     *  Scan through seen-by's for the lowest numbered one
     *  As each lowest one is found, move it to the front,
     *  and repeat the sort for the remeinder of the list
     *  --------------------------------------------------  */
    for (base_index=0 ; base_index<num_seens ; base_index++) {
        low_seen    = seen_by[base_index];
        base_seen   = low_seen;
        low_index   = base_index;
        /*  ---------------------------------------------------
         *  find the lowest one in this subset of the seen-by's
         *  ---------------------------------------------------  */
        for (wk_index = base_index+1 ; wk_index < num_seens ; wk_index++) {
            wk_seen   = seen_by[wk_index];
            if (wk_seen.net < low_seen.net) {
                low_index   = wk_index;
                low_seen    = wk_seen;
            } else if ((wk_seen.net == low_seen.net)
                   &&  (wk_seen.node < low_seen.node)) {
                low_index   = wk_index;
                low_seen    = wk_seen;
            }
        }
        /*  ---------------------------------------------------
         *  the lowest valued one has been found
         *  Put it in the right place if it's not there already
         *  ---------------------------------------------------  */
        if (low_index != base_index) {
            seen_by[base_index] = low_seen;
            seen_by[low_index]  = base_seen;
        }
    }
    /*  -----------------------------------------------------
     *  make a final pass through to eliminate any duplicates
     *  -----------------------------------------------------  */
    wk_seen.node = -999;
    wk_seen.net  = -999;
    for (base_index=0 ; base_index<num_seens ; base_index++) {
        base_seen = seen_by[base_index];
        if ((wk_seen.net  == base_seen.net)
        &&  (wk_seen.node == base_seen.node)) {
            /*  ---------------------------------------------------
             *  the current entry is the same as the previous entry
             *  wipe out the current entry and shift all above down
             *  ---------------------------------------------------  */
            for (wk_index = base_index ; wk_index < (num_seens-1) ; wk_index++) {
                seen_by[wk_index] = seen_by[wk_index+1];
            }
            /*  -------------------------------------------
             *  decrement to start over with last one found
             *  in case there were more than two
             *  Also, decrease counter to reflect new count
             *  -------------------------------------------  */
            if (base_index > 0) {
                base_index--;
            }
            num_seens--;
        }
        wk_seen  = base_seen;
    }

}





/*  ----------------------------------------------
 *  update seen-by list to include the nodes which
 *  I'm sending any echos to.   Don't include any
 *  point nodes.  Sort the resulting list.
 *  ----------------------------------------------  */
void    update_seen_by_list(void)
{
areafix_struct *fix_ptr;

    fix_ptr=area[curr_area].arealist;
    while (fix_ptr != NULL) {
        if ((fix_ptr -> fixnode.net) != point_net) {
            if (num_seens > MAX_SEEN) {
                printf("Can't update SEEN-BY list - must recompile\n");
                --num_seens;
            }
            seen_by[num_seens].net = (fix_ptr -> fixnode.net);
            seen_by[num_seens].node = (fix_ptr -> fixnode.node);
            ++num_seens;
        }
        fix_ptr = (fix_ptr -> fixnext);
    }
    sort_seen_by_list();
}





void    forward_attached_file(netnode who_to, char * file_path, char * file_name)
{
char wkstr[255];
char outstr[64];
char req_filename[64];
char * wksp;
char * req_sp;
netnode file_who_to;
int req_file;
int wk_handle;
int a;
char wkbyte;
long    wk_long;

    file_who_to = route_where(who_to);
    wkbyte = actual_disposition(who_to);
    sprintf(req_filename,"%s%04x%04x.%cLO",  outbound_path,
                file_who_to.net,
                file_who_to.node,
                wkbyte);
    req_file = -1;
    req_file = open(req_filename,O_RDWR | O_BINARY);
    if (req_file < 0) {
        req_file = open(req_filename,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        if (req_file < 0) {
            printf("error opening outbound file '%s'. errno=%d\n",req_filename,errno);
            exit_status=3;
            return;
        }
    }
    /* ----------------------------------- *\
    |  position to end of file-attach file  |
    \* ----------------------------------- */
    lseek(req_file,0L,SEEK_END);
    /* ------------------------------------------------------- *\
    |  prepare for the possibility of multiple files per title  |
    \* ------------------------------------------------------- */
    memset(req_filename,0,64);      /* clear string to all zeros */
    strcpy(req_filename,file_name);
    req_sp = &req_filename[0];
    while (*req_sp != 0) {
        if (testing) {
            printf("Forwarding title: '%s'\n",req_sp);
        }
        /* -------------------------- *\
        |  find the end of a filename  |
        \* -------------------------- */
        wksp = &wkstr[0];
        while ((*req_sp != ' ')
        &&     (*req_sp != 0  )) {
            *wksp++ = *req_sp++;
        }
        *wksp = 0;
        /* ------------------------------------------------ *\
        |  add this name to the end of the file-attach file  |
        \* ------------------------------------------------ */
        sprintf(outstr,"%s%s\015\012",file_path,wkstr);
        write(req_file, outstr, strlen(outstr));
        /* -------------------------------------------------- *\
        |  recopy the string to process any further filenames  |
        \* -------------------------------------------------- */
        while (*req_sp == ' ') {
            ++req_sp;
        }
        /* --------------------------------------- *\
        |  show the filename & filesize in the log  |
        \* --------------------------------------- */
        if (logging) {
            wk_handle = -1;
            wk_long = 0;
            outstr[strlen(outstr)-2] = 0;       /* remove CR/LF */
            wk_handle = open(outstr,O_RDONLY | O_BINARY);
            if (wk_handle < 0) {
                printf("Error opening attached file:'%s', errno = %d\n",outstr, errno);
            } else {
                wk_long = filelength(wk_handle);
            }
            close(wk_handle);
            a=fprintf(log_file, "FILE: %-20s %12ld %d/%d\n",wkstr, wk_long, who_to.net, who_to.node);
            if (a==EOF) {
                exit_status=3;
            }
        }
        /* ------------------------------------- *\
        |  display info for information purposes  |
        \* ------------------------------------- */
        printf("\n");
        printf("~~~~~~~~~~~~~\n");
        printf("Forwarded file '%s' to %d/%d\n",
                wkstr,
                who_to.net,
                who_to.node);
        printf("~~~~~~~~~~~~~\n");
        printf("\n");
    }
    close(req_file);
}








void process_incoming_message(void)
{
int             p1;
long            file_pos_save;
char wkstr[255];
int dm,a,wk_area;
netnode who_to;
netnode true_dest;
int     true_dest_point;
unsigned char * wksp;

    long message_length, buff_size;
    char *b,s[169];     /* enough for 161 + line termination */
    unsigned char   ch;


    /*  -----------------------------------------------------------
     *  the header ID word has already been read, so read past that
     *  read the orig-node, dest-node, orig-net, dest-net,
     *  attribute, and cost
     *  -----------------------------------------------------------  */
    read(incoming_file,(void *)&message_header.orig_node,12);
    /*  ----------------
     *  read in the date
     *  ----------------  */
    p1=0;
    do {
        ch=0;
        read(incoming_file,(void *)&ch,1);
        if (p1<20)
            message_header.datetime[p1++]=ch;
    } while (ch);
    /*  ------------------
     *  get the "TO:" user
     *  ------------------  */
    p1=0;
    do {
        ch=0;
        read(incoming_file,(void *)&ch,1);
        if (p1<80)
            fido_to_user[p1++]=ch;
    } while (ch);
    /*  --------------------
     *  get the "FROM:" user
     *  --------------------  */
    p1=0;
    do {
        ch=0;
        read(incoming_file,(void *)&ch,1);
        if (p1<80)
            fido_from_user[p1++]=ch;
    } while (ch);
    /*  ----------------
     *  get the "Title:"
     *  ----------------  */
    p1=0;
    do {
        ch=0;
        read(incoming_file,(void *)&ch,1);
        if (p1<60)                          /* WWIV title limitation 50 chars */
            fido_title[p1++]=ch;
    } while (ch);
    if (testing){
        printf("ID=%X\n",               message_header.id);
        printf("orig =(%d/%d)",         message_header.orig_net,
                                        message_header.orig_node);
        printf("  dest =(%d/%d)",       message_header.dest_net,
                                        message_header.dest_node);
        printf("Attr=%X",               message_header.attribute);
        printf("Cost=%d",               message_header.cost);
        printf("datetime=%s\n",         message_header.datetime);
        printf("To:%s\n",               fido_to_user);
        printf("From:%s\n",             fido_from_user);
        printf("Title:%s\n",            fido_title);
    }
    /*  -------------------------------------
     *  get the full message text into memory
     *  -------------------------------------  */
    buff_size=filelength(incoming_file);
    if ( buff_size > MAX_MSG_SIZE )
        buff_size=MAX_MSG_SIZE;
    if ((b=(char *)farmalloc(buff_size+1024)) == NULL) {
        printf("Allocation error for message text\n");
    }
    b[0]=0;
    message_length=0;
    while (ch=0,                                /* init to zero in case EOF */
           read(incoming_file,(void *)&ch,1),
           (ch!=0)&&(exit_status==0)) {
        if (message_length < MAX_MSG_SIZE)
            b[message_length++]=ch;
    }
    b[message_length++]=0;
    true_origin.net  = message_header.orig_net;
    true_origin.node = message_header.orig_node;
    /*  --------------------------------------------
     *  dissect the message into its component parts
     *  --------------------------------------------  */
    dissect_message(b);
    /*  -----------------------------------------------------
     *  Special processing for messages arriving FROM a point
     *  -----------------------------------------------------  */
    if (message_header.orig_net == point_net) {
        /*  ---------------------------------------------------
         *  Patch up Origin line from point systems in echomail
         *  ---------------------------------------------------  */
        if (curr_area >= 0) {
            wksp = strrchr(fido_origin,'(');        /*  find last left paren */
            if (wksp == NULL) {
                wksp = strchr(fido_origin,0);       /*  if none, find end of message */
            }
            *wksp++ = '(';                          /*  store lpar in case none there  */
            itoa(my_net,wksp,10);
            wksp = strchr(wksp,0);
            *wksp++ = '/';
            itoa(my_node,wksp,10);
            wksp = strchr(wksp,0);
            *wksp++ = '.';
            itoa(message_header.orig_node,wksp,10);
            wksp = strchr(wksp,0);
            *wksp++ = ')';
            *wksp   = 0;
        }
        /*  --------------------------------------------------------------
         *  fake a ^aFMPT if it's from a point and there's not already one
         *  --------------------------------------------------------------  */
        if (fido_from_point == 0) {
            fido_from_point = message_header.orig_node;
        }
        /*  ----------------------------------------
         *  re-address the message as coming from me
         *  unless it's netmail to someone else in the
         *  same net or the same point-net
         *  ----------------------------------------  */
/*      if ((curr_area >= 0)                                    */
/*      || (   (message_header.dest_net != point_net)           */
/*         &&  (message_header.dest_net != my_net)   )) {       */
/*          message_header.orig_net = my_net;                   */
/*          message_header.orig_node = my_node;                 */
/*      }                                                       */
        /* ----------------------------------------------------- *\
        |  deleted the above in an attempt to be compatible       |
        |  with MSGED - now 4Dmatrix will use "TOPT" and "FMPT"   |
        |                         instead of "TOPT:" and "FMPT:"  |
        \* ----------------------------------------------------- */
        message_header.orig_net = my_net;
        message_header.orig_node = my_node;
    }
    /*  -----------------------------------------------------
     *  Special processing for messages arriving TO a point
     *  Re-address net-mail which may be addressed to a point
     *  -----------------------------------------------------  */
    true_dest.net  = message_header.dest_net;
    true_dest.node = message_header.dest_node;
    true_dest_point = fido_to_point;
    if ((message_header.dest_net == my_net)
    &&  (message_header.dest_node == my_node)) {
        if (fido_to_point != 0) {
            message_header.dest_net = point_net;
            message_header.dest_node = fido_to_point;
            fido_to_point = 0;          /* delete to prevent ^aTOPT in outbound message */
        }
    }
    /*  -------------------------------------
     *  determine what to do with the message
     *  -------------------------------------  */
    if (curr_area < 0) {
        /*  ------------------------------------------------------
         *  There is not a recognized AREA, so it must be net-mail
         *  send it on to its destination
         *  ------------------------------------------------------  */
        who_to.net  = message_header.dest_net;
        who_to.node = message_header.dest_node;
        if ((!strnicmp(fido_to_user,"AREAFIX",7))
        &&  (who_to.net  == my_net)
        &&  (who_to.node == my_node) ){
            process_areafix_request(b);
        } else {
            if ((via_line[0] != 0)
            &&  (  (message_header.orig_net  != my_net)
                || (message_header.orig_node != my_node) ) ) {
                strcat(b,"\001Via: ");
                strcat(b,via_line);                         /*  say "Via <boardname>"  */
                strcat(b,"\015\012");
            }
            /*  ---------------------------------------------------------
             *  Special routing for net-mail addressed to the host system
             *  ---------------------------------------------------------  */
            if ((who_to.net  == my_net)
            &&  (who_to.node == my_node)) {
                sprintf(wkstr, " * Routed from: %d/%d.%d\015\012",   true_dest.net, true_dest.node, true_dest_point);
                strcat(b,wkstr);
                who_to.net  = netmail_net;
                who_to.node = netmail_node;
            }
            /*  --------------------------
             *  forward any attached files
             *  --------------------------  */
            if (message_header.attribute & 0x0010) {
                forward_attached_file(who_to, inbound_path, fido_title);
            }
            send_message(b,who_to,' ');                     /*  use default routing  */
            strcpy(s,"*");
            strcat(s,fido_title);
            strcat(s," Sent to ");
            sprintf(s,"%d/%d", who_to.net, who_to.node);
            printf("%s\n",s);
        }
    } else {
        /*  ---------------------------------------------------------
         *  The AREA name is recognized - if it's GroupMail, send a
         *  copy to myself only.   If it's EchoMail, send a copy to
         *  everyone receiving this conference
         *  ---------------------------------------------------------  */
        if (area[curr_area].areatype == 'G') {
            forward_group_mail(b);
        }
        if (area[curr_area].areatype == 'E') {
            /*  ----------------------------------------------------------
             *  save old SEEN-BY array to determine who to send message to
             *  ----------------------------------------------------------  */
            memcpy(saw_by, seen_by, sizeof(seen_by));
            num_saws = num_seens;
            /*  -------------------------------------------------------------
             *  add nodes to new SEEN-BY array who are being sent the message
             *  -------------------------------------------------------------  */
            update_seen_by_list();
            /*  ---------------------------
             *  add myself to the PATH line
             *  ---------------------------  */
            path[num_paths].net=my_net;
            path[num_paths].node=my_node;
            ++num_paths;
            /*  -----------------------
             *  distribute the echomail
             *  -----------------------  */
            forward_echo_mail(b);
            if (verbose) {
                strcpy(s,"*");
                strcat(s,fido_title);
                strcat(s," posted on ");
                strcat(s,fido_area_name);
                printf("%s\n",s);
            }
            if (logging) {
                a=fprintf(log_file, "ECHO: %-20s %12ld\n",fido_area_name, message_length);
                if (a==EOF) {
                    exit_status=3;
                }
            }
        }
    }
    farfree((void *)b);
    if (exit_status != 0) {
        return;
    }
}   /* end of process_incoming_message */



















int do_remote(char *prog_to_execute, int shell_flag)
{
int i,i1,l;
char s[160];
char *ss[30];
char cdir[81];
char *xenviron[50];

    i=0;
    while (environ[i]!=NULL) {
      xenviron[i]=environ[i];
      ++i;
    }
    xenviron[i++]="FOURDOG=TRUE";
    xenviron[i]=NULL;
    strcpy(cdir,"X:\\");
    cdir[0]='A'+getdisk();
    getcurdir(0,&(cdir[3]));
    if (shell_flag) {
        strcpy(s,getenv("COMSPEC"));
        strcat(s," /C ");
        strcat(s,prog_to_execute);
    } else {
        strcpy(s,prog_to_execute);
    }
    ss[0]=s;
    i=1;
    l=strlen(s);
    for (i1=1; i1<l; i1++)
        if (s[i1]==32) {
            s[i1]=0;
            ss[i++]=&(s[i1+1]);
        }
    ss[i]=NULL;
    i=(spawnvpe(P_WAIT,ss[0],ss,xenviron) & 0x00ff);
    chdir(cdir);
    setdisk(cdir[0]-'A');
    return(i);
}



void    process_ARCmail(void)
{
char    s[255], new_path[255], arc_command[255], new_name[15], fn[81], arcmail_file[81], wk_filetype[5];
char    ch;
char    out_disposition;
int     ARC_find;
int     MSG_file;
int     i,j,f1,wk_count;
char    * wksp;
char    * arcsp;
disposition_node_struct * disp_ptr;
netnode who_to;
FILE    *attach_file;
struct  ffblk   ARC_ff;
long            MSG_secs;

    strcpy(wk_filetype, "?UT");
    /*  ---------------------------------------------------------------
     *  ARC up any ARCmail bundles which need it.
     *  Look through arcmail array for any nodes which have been marked
     *  ---------------------------------------------------------------  */
    for (i=0 ; i<num_arcs ; i++) {
        if (arcmail[i].arc_flag != 0) {
            who_to.net  = arcmail[i].arc_to.net;
            who_to.node = arcmail[i].arc_to.node;
            /*  -----------------------------------------------------------
             *  look through file-counts and see if it needs a special name
             *  -----------------------------------------------------------  */
            for (j=0 ; j<num_limits ; j++) {
                if ((limit[j].who.net  == who_to.net)
                &&  (limit[j].who.node == who_to.node)) {
                    strcpy(wk_filetype,"LM?");
                }
            }
            /*  -----------------------------------------------------------------------
             *  for each node marked, add any files addressed to that node into the
             *  ARCmail archive.   (including OUT, CUT, and HUT).
             *  -----------------------------------------------------------------------  */
            sprintf(s,"%s%04x%04x.%s",      outbound_path,
                                            who_to.net,
                                            who_to.node,
                                            wk_filetype);
            if (testing) {
                printf("ready to search for *.?UT files with pattern:%s\n",s);
            }
            f1=findfirst(s,&ff,0);
            while ((f1==0) && (exit_status==0)) {
                /*  -------------------------------------------------
                 *  found a *.?UT file - rename it to a .PKT filename
                 *  -------------------------------------------------  */
                strcpy(s,outbound_path);
                strcat(s,(ff.ff_name));
                invent_pkt_name(new_name);
                strcpy(new_path,outbound_path);
                strcat(new_path,new_name);
                if (testing)
                    printf("-------Renaming %s to %s--------\n",s,new_path);
                rename(s,new_path);
                /*  --------------------------------------
                 *  now build up the archiver command line
                 *  replacing %1 with the archive name,
                 *  and %2 with the PKT-file name.
                 *  --------------------------------------  */
                wksp = &arc_text[arcmail[i].arc_type][0];
                arcsp = &arc_command[0];
                if (testing) {
                    printf("\nARChiver pattern is:\n'%s'\n",wksp);
                }
                sprintf(arcmail_file,"%s%04x%04x.MO%1d",  arcmail_path,
                                                        (my_net - who_to.net),
                                                        (my_node - who_to.node),
                                                        arcmail[i].arc_type);

                ARC_find = findfirst(arcmail_file, &ARC_ff,0);
                if (testing) {
                    printf("-----------> ARC_find == %d\n",ARC_find);
                }
                while (*wksp != 0) {
                    if (*wksp == '%') {
                        ++wksp;
                        if (*wksp == '1') {
                            *arcsp=0;
                            strcat(arcsp,arcmail_file);
                            arcsp=strchr(arcsp,0);
                            ++wksp;
                        } else if (*wksp == '2') {
                            *arcsp=0;
                            strcat(arcsp,new_path);
                            arcsp=strchr(arcsp,0);
                            ++wksp;
                        } else {
                            /*  ----------------------------------------
                             *  it's not %1 or %2, so just copy verbatim
                             *  ----------------------------------------  */
                            *arcsp++ = '%';
                            *arcsp++ = *wksp++;
                        }
                    } else {
                        *arcsp++ = *wksp++;
                    }
                }
                *arcsp = 0;
                if (testing) {
                    printf("\nrunning ARChiver with\n'%s'\n",arc_command);
                }
                do_remote(arc_command,1);
                /*  ---------------------------------
                 *  determine how to send the ARCmail
                 *  ---------------------------------  */
                out_disposition = ' ';
                disp_ptr = &disposition[0];
                for (wk_count = 0;  ((wk_count < num_disposition_nodes) && (out_disposition == ' '));  wk_count++,disp_ptr++) {
                    if (((disp_ptr -> disposition_netnode.net)  == who_to.net)
                    &&  ((disp_ptr -> disposition_netnode.node) == who_to.node)) {
                        out_disposition = (disp_ptr -> disposition_filetype);
                    }
                    /*  --------------------------------------------------
                     *  match the net if the disposition says to match ALL
                     *  --------------------------------------------------  */
                    if (((disp_ptr -> disposition_netnode.net)  == who_to.net)
                    &&  ((disp_ptr -> disposition_netnode.node) == -1)) {
                        out_disposition = (disp_ptr -> disposition_filetype);
                    }
                }
                /*  ---------------------------------------------
                 *  if no routing found, default to HOLD filetype
                 *  Normal is 'FLO', so change "O" to "F"
                 *  ---------------------------------------------  */
                switch (out_disposition) {
                    case ' ':   out_disposition = 'H';  break;
                    case 'O':   out_disposition = 'F';
                }
                if (frontdoor_path[0] == 0) {
                    if (testing) {
                        printf("-----------> Binkley-Compatible\n");
                    }
                    /*  -----------------------------------------
                     *  now create a file-attach to send the file
                     *  -----------------------------------------  */
                    sprintf(fn,"%s%04x%04x.%cLO",   outbound_path,
                                                    who_to.net,
                                                    who_to.node,
                                                    out_disposition);
                    /*  ---------------------------------------
                     *  get string to put into file-attach file
                     *  ---------------------------------------  */
                    sprintf(arc_command,"^%s",arcmail_file);
                    /*  --------------------------------------------------
                     *  if there's an attach-file already, look through it
                     *  to see if the .MOx file is already listed there
                     *  --------------------------------------------------  */
                    ch = 'N';
                    attach_file=fopen(fn,"rt");
                    if (attach_file != NULL) {
                        while (i=fscanf(attach_file, "%s", s),   ((i!= EOF) && (i!=0)) ) {
                            if (testing) {
                                printf("Comparing attach: '%s' :: '%s'\n",s,arc_command);
                            }
                            if (!strcmp(s,arc_command)) {
                                ch = 'Y';
                            }
                        }
                        fclose(attach_file);
                    }
                    /*  -------------------------------------------
                     *  add a new line to the attach file if needed
                     *  -------------------------------------------  */
                    if (ch == 'N') {
                        if (testing)
                            printf("Opening file-attach file: %s\n",fn);
                        attach_file=fopen(fn,"a+t");
                        if (attach_file == NULL) {
                            printf("error opening attach file '%s'. errno=%d\n",fn,errno);
                            exit_status=3;
                            return;
                        }
                        /*  --------------------------------------------------------------------
                         *  send it with an "^" in front of name to delete file after sending it
                         *  --------------------------------------------------------------------  */
                        fprintf(attach_file,"%s\n",arc_command);
                        fclose(attach_file);
                    }
                } else {
                    if (testing) {
                        printf("-----------> FrontDoor-Compatible\n");
                    }
                    /* --------------------------------------------- *\
                    |  We're running in FrontDoor compatibility mode  |
                    |  Instead, create a .MSG file with the ARCmail   |
                    |  file attached.  Rather than come up with some  |
                    |  elaborate way to determine if an attach has    |
                    |  already been generated, just assume it has if  |
                    |  the ARCmail packet already exists.             |
                    \* --------------------------------------------- */
                    if (ARC_find != 0) {    /* if the file was NOT found */
                        if (testing) {
                            printf("-----------> Making new MSG file\n");
                        }
                        /* ------------------------------------------------------------------- *\
                        |  now create a MSG file with the file-attach bit set to send the file  |
                        \* ------------------------------------------------------------------  */
                        ARC_find = 0;
                        i=1;
                        while (ARC_find == 0) {
                            sprintf(fn,"%s%d.MSG",   frontdoor_path, ++i);
                            ARC_find = findfirst(fn, &ARC_ff,0);
                        }
                        if (testing) {
                            printf("-----------> MSG file = %s\n",fn);
                        }
                        MSG_file=-1;
                        MSG_file=open(fn,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
                        if (MSG_file <= 0) {
                            printf("error opening %s. errno=%d\n",fn,errno);
                            exit_status=1;
                        } else {
                            /* -- the FromUserName -- */
                            memset(s,0,36);
                            strcpy(s,my_version);
                            write(MSG_file, s, 36);
                            /* -- the ToUserName -- */
                            memset(s,0,36);
                            strcpy(s,"Sysop");
                            write(MSG_file, s, 36);
                            /* -- the Subject (the name of the attached file) -- */
                            memset(s,0,72);
                            strcpy(s,arcmail_file);
                            write(MSG_file, s, 72);
                            /* -- the Date -- */
                            memset(s,0,20);
                            time(&MSG_secs);
                            strcpy(arc_command,ctime(&MSG_secs));
                            strncpy(&s[0],  &arc_command[8],  3);     /* the day of the month & blank */
                            strncpy(&s[3],  &arc_command[4],  4);     /* the month name & blank */
                            strncpy(&s[7],  &arc_command[22], 2);     /* the year */
                            strncpy(&s[9],  &arc_command[3],  1);     /* a blank */
                            strncpy(&s[10], &arc_command[3],  1);     /* a blank */
                            strncpy(&s[11], &arc_command[11], 8);     /* the time */
                            s[19]=0;
                            write(MSG_file, s, 20);
                            /* -- the times read -- */
                            i=0;
                            write(MSG_file, &i, 2);
                            /* -- destination node -- */
                            write(MSG_file, &who_to.node, 2);
                            /* -- from node -- */
                            write(MSG_file, &my_node, 2);
                            /* -- the cost -- */
                            i=0;
                            write(MSG_file, &i, 2);
                            /* -- from net -- */
                            write(MSG_file, &my_net, 2);
                            /* -- destination net -- */
                            write(MSG_file, &who_to.net, 2);
                            /* -- fill (8 bytes) -- */
                            memset(s,0,8);
                            write(MSG_file, s, 8);
                            /* -- the reply-to ID -- */
                            i=0;
                            write(MSG_file, &i, 2);
                            /* -- the Attribute -- */
                            i=1             /* private       */
                             +16            /* file attached */
                             +128           /* KillSent      */
                            ;
                            switch (out_disposition) {
                                case 'H':   i+=512;     break;
                                case 'C':   i+=2;       break;
                            }
                            write(MSG_file, &i, 2);
                            /* -- the next reply ID -- */
                            i=0;
                            write(MSG_file, &i, 2);
                            /* -- the message text (null) -- */
                            i=0;
                            write(MSG_file, &i, 1);
                            close(MSG_file);
                        }
                    }
                }
                f1=findnext(&ff);
                /*  --------------------------------------------
                 *  since the PKT name is derived from the time,
                 *  if there's more files for this node, just
                 *  sleep for a second to insure that the next
                 *  file gets a unique name
                 *  --------------------------------------------  */
                if (f1 == 0)
                    sleep(1);
            }
        }
    }
}







void main (int argc, char *argv[])
{
char s[161];
char ch;
int f1,more_msgs;

    exit_status = 0;
    testing = 0;
    verbose = 0;
    logging = 0;
    log_file = NULL;
    for (i=1; i<argc; i++) {
        strcpy(s,argv[i]);
        if ((s[0]=='-') || (s[0]=='/')) {
            ch=upcase(s[1]);
            switch(ch) {
                case 'T':
                    testing=1;
                    printf("test mode enabled\n");
                    break;
                case 'V':
                    verbose=1;
                    break;
                case 'L':
                    logging=1;
                    break;
            }
        }
    }



    tzset();
    outbound_file = -1;
    frontdoor_path[0] = 0;
    get_areafix_config();            /* # nodes, paths, etc. */
    get_system_config();            /* directories, etc.  */
    if (exit_status != 0){
        exit(exit_status);
    }
    if (logging) {
        log_file = fopen("Fourdog.log", "at");
        if (log_file == NULL) {
            printf("Error opening log file\n");
            exit(1);
        }
    }
    /*  -------------------------------------------------
     *  look in the inbound directory for any .PKT files
     *  (ARCmail bundles will have already been unARC'ed)
     *  -------------------------------------------------  */
    strcpy(s,inbound_path);
    strcat(s,"*.PKT");
    if (testing) {
        printf("ready to search for PKT files with pattern:%s\n",s);
    }
    f1=findfirst(s,&ff,0);
    while ((f1==0) && (exit_status==0)) {
        strcpy(s,inbound_path);
        strcat(s,(ff.ff_name));
        if (testing)
            printf("-------Opening PKT file '%s'--------\n",s);
        incoming_file=-1;
        incoming_file=open(s,O_RDWR | O_BINARY);
        if (incoming_file<0) {
            printf("error opening incoming file: %s. errno=%d\n",s,errno);
            exit_status=6;
            break;
            }
        read(incoming_file,(void *)&packet_header.orig_node,sizeof(packet_header_struct));
        if (testing){
            printf("orig_node=%d/%d\n", packet_header.orig_net,
                                        packet_header.orig_node);
            printf("dest_node=%d/%d\n", packet_header.dest_net,
                                        packet_header.dest_node);
            printf("packet date=%02d/%02d/%02d\n",  packet_header.year,
                                                    packet_header.month,
                                                    packet_header.day);
            printf("packet time=%02d:%02d:%02d\n",  packet_header.hour,
                                                    packet_header.minute,
                                                    packet_header.second);
            printf("baud=%d\n",packet_header.baud);
            printf("packet_type=%d\n",packet_header.packet_type);
            printf("product_code=%d\n",packet_header.product_code);
        }
        more_msgs=1;
        while ((more_msgs) && (exit_status ==0)) {
            message_header.id = 0;                  /* init in case it's EOF */
            read(incoming_file,(void *)&message_header.id,2);
            if (message_header.id == 0x0002) {
                process_incoming_message();
            }else{
                if (message_header.id == 0) {
                    if (testing)
                        printf("<<<end of message packet>>>\n");
                    more_msgs=0;
                }else{
                    printf("invalid message header ID = %X\n",message_header.id);
                    exit_status=7;
                    break;
                }
            }

        }
        close(incoming_file);
        if (exit_status != 0) {
            /*  -----------------------------------------------
             *  Had an error in this packet - rename it to .BAD
             *  This is so that the file can stay around to be
             *  examined for the cause of the error, and/or
             *  re-processed after the error condition has been
             *  corrected.
             *  -----------------------------------------------  */
            char newname[81];
            char *wksp;
            int errval;

            strcpy(newname,s);
            wksp = strrchr(newname,'.');
            wksp++;
            strcpy(wksp,"BAD");
            errval=rename(s,newname);
            if (errval<0)
                printf("Error renaming file to .BAD, errno=%d",errno);
        } else
            if (!testing)
                unlink(s);  /* delete file after processing it */
        if (testing)
            printf("just unlinked '%s'\n",s);
        /*  ------------------------------------------------------
         *  save the new-message pointers so we don't re-send msgs
         *  this is done even on the packets in error since they
         *  may have been partially processed.
         *  ------------------------------------------------------  */
        f1=findnext(&ff);
    }
    if (outbound_file > 0) {
        outchr(00);
        outchr(00);             /* message-type == 0000 (hex) terminates packet */
        close(outbound_file);
    }
    /*  ----------------------------------------------
     *  process ARCmail for any nodes which receive it
     *  ----------------------------------------------  */
    process_ARCmail();
    if (testing)
        printf("exit_status=%d\n",exit_status);
    if (logging) {
        fclose(log_file);
    }
    exit (exit_status);
}
