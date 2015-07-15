/*--------------------------------------------------------------------------*/
/*                                                                          */
/*                   WWIVTOSS!  Fido/WWIV Message Tosser!                   */
/*                                                                          */
/*  A WWIV <-> Fidonet Message Tosser and Translator                        */
/*                                                                          */
/*  WWIVTOSS greatly simplifies the necessity for a seperate mail tosser    */
/*  and translation program to exchange messages between a WWIV BBS and     */
/*  Fidonet.                                                                */
/*                                                                          */
/*  WWIVTOSS relies heavily on code from Shay Walter's 4dog WWIV/Fido       */
/*  interface, as well as WWIV Source code, and features availble in the    */
/*  WFIDO software owned by WWIV Software Services and written by Kevin     */
/*  Carpenter.                                                              */
/*                                                                          */
/*  WWIVTOSS is NOT public domain or freeware.  It is a shareware product   */
/*  and all rights are reserved by the author, Craig Dooley.                */
/*                                                                          */
/*  File    : WWIVTOSS.C                                                    */
/*                                                                          */
/*  Purpose : Handles Importing of Fido Messages to WWIV Packets            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#define _DEFINE_GLOBALS_
#include "vardec.h"             // WWIV Structures
#include "vars.h"               // Global Variables
#include "net.h"                // WWIV Network Structures
#include "fcns.h"               // Function List

net_system_list_rec huge *csn;  // Network Connection Records
struct ffblk ffblk;             // For findnext(), findfirst();
struct  ffblk   ff;             // Ditto
int month[13] = {               // Days Of The Month
   0,                           // Place Holder
   31,                          // January
   28,                          // February
   31,                          // March
   30,                          // April
   31,                          // May
   30,                          // June
   31,                          // July
   31,                          // August
   30,                          // September
   31,                          // October
   30,                          // November
   31                           // December
};

/* --------------------------------------------------- */
/* Checks for STR1 in STR2.  Returns NULL if not there */
/* --------------------------------------------------- */

char *striinc(char *str1,char *str2)
{
    register int max;
    char *p;

    max=strlen(str1);

    for(p=str2;*p;p++) {
        if(!strnicmp(str1,p,max)) return(p);
    }
    return(NULL);                       /* string1 not found in string2 */
}

/* ----------------------------------------------- */
/* Reads AREAS.DAT and returns the number of areas */
/* ----------------------------------------------- */

int get_number_of_areas(void)
{
    int f,count;
    f=open("AREAS.DAT",O_RDONLY | O_BINARY | O_CREAT);
    if (f<0)
    {
        return(0);
    }

    count=((int) (filelength(f)/sizeof(arearec)));
    close(f);
    return(count);
}

/* ------------------------------------------------ */
/* Reads configuration options from WWIVTOSS.DAT    */
/* ------------------------------------------------ */

void read_wwivtoss_config(void)
{
    int configfile;

    configfile=sh_open1("WWIVTOSS.DAT",O_RDWR | O_BINARY);
    if (configfile<0)
    {
        printf("Error opening WWIVTOSS.DAT!\r\n");
        exit(1);
    }
    sh_read(configfile,(void *) (&cfg), sizeof (tosser_config));
    sh_close(configfile);
    /* registered=cfg.registered; MMH removed */
    registered=1;
    cfg.registered=1;
    cfg.add_hard_cr=cfg.add_line_feed=1;
    cfg.soft_to_hard=cfg.hard_to_soft=cfg.add_soft_cr=0;
    num_areas=cfg.num_areas=get_number_of_areas();

}

/* ------------------------------------------------ */
/* Writes configuration options to WWIVTOSS.DAT     */
/* ------------------------------------------------ */

void write_config(void)
{
    int configfile;

    configfile=sh_open1("WWIVTOSS.DAT",O_RDWR | O_BINARY | O_CREAT);
    if (configfile<0)
    {
        printf("Error opening WWIVTOSS.DAT!\r\n");
        exit(1);
    }
    sh_write(configfile,(void *) (&cfg), sizeof (tosser_config));
    sh_close(configfile);

}

/* ------------------------------------------------ */
/* Reads node configurations from NODES.DAT         */
/* ------------------------------------------------ */

void read_nodes_config(void)
{
    int handle;

    handle=sh_open1("NODES.DAT",O_RDONLY | O_BINARY);
    if (handle<0)
    {
        printf("Error opening NODES.DAT!\r\n");
        exit(1);
    }

    sh_read(handle,(void *) (&nodes), sizeof (nodes));
    sh_close(handle);
    num_nodes=cfg.num_nodes;
}

/* ------------------------------------------------ */
/* Reads uplink configurations from UPLINKS.DAT     */
/* ------------------------------------------------ */

void read_uplink_config(void)
{
    int handle;

    handle=sh_open1("UPLINKS.DAT",O_RDONLY | O_BINARY);
    if (handle<0)
    {
        printf("Error opening UPLINKS.DAT!\r\n");
        uplinkable=0;
        return;
    }

    sh_read(handle,(void *) (&uplink), sizeof (uplink));
    sh_close(handle);
    num_uplinks=cfg.num_uplinks+1;
    uplinkable=1;
}

/* ------------------------------------------------ */
/*  Write Area Configuration To AREAS.DAT           */
/* ------------------------------------------------ */

void write_area(unsigned int un, arearec *a)
{
    char s[81];
    long pos;
    int f,j;

    for (j=0;j<MAX_NODES;j++)
    {
        if (thisarea.feed[j].zone==10)
            thisarea.feed[j].zone=32765;
        if (thisarea.feed[j].net==10)
            thisarea.feed[j].net=32765;
        if (thisarea.feed[j].node==10)
            thisarea.feed[j].node=32765;
        if (thisarea.feed[j].point==10)
            thisarea.feed[j].point=32765;
    }
    strcpy(s,"AREAS.DAT");
    f=sh_open(s,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    if (f>=0)
    {
        pos=((long) sizeof(arearec)) * ((long) un);
        sh_lseek(f,pos,SEEK_SET);
        sh_write(f, (void *)a, sizeof(arearec));
        sh_close(f);
    }
}

/* ------------------------------------------------ */
/*  Read area configuration from AREAS.DAT          */
/* ------------------------------------------------ */

void read_area(unsigned int un, arearec *a)
{
    char s[81];
    long pos;
    int f, nu,j;

    strcpy(s,"AREAS.DAT");
    f=sh_open1(s,O_RDONLY | O_BINARY);
    if (f<0)
    {
        return;
    }

    nu=((int) (filelength(f)/sizeof(arearec))-1);

    if (un>nu)
    {
        sh_close(f);
        return;
    }
    pos=((long) sizeof(arearec)) * ((long) un);
    sh_lseek(f,pos,SEEK_SET);
    sh_read(f, (void *)a, sizeof(arearec));
    sh_close(f);
    for (j=0;j<MAX_NODES;j++)
    {
        if (thisarea.feed[j].zone==32765)
            thisarea.feed[j].zone=10;
        if (thisarea.feed[j].net==32765)
            thisarea.feed[j].net=10;
        if (thisarea.feed[j].node==32765)
            thisarea.feed[j].node=10;
        if (thisarea.feed[j].point==32765)
            thisarea.feed[j].point=10;
    }
}

/* -------------------------------------------------------- */
/* Read in Translation Tables from XLAT.DAT                 */
/* -------------------------------------------------------- */

void load_xlat(void)
{
    int handle;

    handle=sh_open1("XLAT.DAT",O_RDWR | O_BINARY);

    if (handle<0)
    {
        printf("Error Opening XLAT.DAT!\r\n");
        exit(1);
    }
    sh_read(handle,(void *) (&xlat), sizeof (xlat));
    sh_close(handle);
}

/* ----------------------------------------------------------------- */
/* Function to actually write incoming message to WWIV Data Bases    */
/* ----------------------------------------------------------------- */

void send_mail2(void)
{
    int i,dm,j;
    char s[80],source[300],*b,scratch[181],source1[300];
    xtrasubsnetrec *xnp;
    messagerec m;
    long len,len2;
    unsigned long dat;
    postrec p;
    struct date dt;
    struct time tm;
    char *month[] = {"Bug","Jan","Feb","Mar","Apr","May","Jun",
                     "Jul","Aug","Sep","Oct","Nov","Dec"};

    source[0]=0;
    get_status(0,1);
    i=read_subs();
    curlsub=-1;
    if ((msgtype==3) || (msgtype==5))
        sub_type=atoi(thisarea.subtype);
    else
        sub_type=0;

    for(i=0;i<num_subs;i++)
    {
        if (xsubs[i].num_nets)
        {
            for (j=0;j<xsubs[i].num_nets;j++)
            {
                xnp=&xsubs[i].nets[j];
                if (sub_type)
                {
                    if ((xnp->type==sub_type) && (xnp->net_num==thisarea.net_num))
                    {
                        curlsub=i;
                        j=xsubs[i].num_nets;
                        i=num_subs;
                    }
                } else
                {
                    if ((stricmp(xnp->stype,thisarea.subtype)==0) && (xnp->net_num==thisarea.net_num))
                    {
                        curlsub=i;
                        j=xsubs[i].num_nets;
                        i=num_subs;
                    }
                }
            }
        }
    }

    if (curlsub<0)
    {
        sprintf(s,"Can't find the sub to post %s (Subtype %s) on!!!!\n",thisarea.name,thisarea.subtype);
        bad_post++;
        badd_2++;
        valid_post--;
        write_log(s);
        check_mail();
        return;
    }
    if (debug)
    {
        sprintf(scratch,"þ Message Going to sub # %d\n   Data File %s\n    Sub Name %s\n",curlsub,subboards[curlsub].filename,subboards[curlsub].name);
        write_log(scratch);
    }
    set_net_num(thisarea.net_num);
    net_num=thisarea.net_num;
    len= (unsigned) strlen(buffer);
    len2=len;

    b=malloc(45*1024);

    len=0;
//    strcpy(source1,fido_from_user);
//    strcat(source1," (");
//    strcat(source1,origi_node);
//    strcat(source1,")");
    sprintf(source1,"%s (%s)",fido_from_user,origi_node);
    strcpy(source, source1);
    addline(b,source,&len);
    /* Get date/time of Email creation. */
    /* Ditto the \015 for the time/date string. */

    strncpy(scratch,fido_date_line,20);

    scratch[20]='\0';

    /* Build the date - cumbersome, but it needs to be done */
	/* Build tblock as we go along so we can have a pretty date */
	scratch[2] = '\0';	/* make day a string */
	dt.da_day = atoi(scratch);
    for (i=1;i<13;i++)
        if (strncmpi(month[i],&scratch[3],3) == 0)
        {
            dt.da_mon = i;
            break;
        }
    scratch[9] = '\0';  /* make year a string */
    dt.da_year = atoi(&scratch[7]);
    if (dt.da_year>90)
        dt.da_year += 1900;
    else
        dt.da_year += 2000;
    scratch[13] = '\0'; /* make hour a string */
	tm.ti_hour = atoi(&scratch[11]);
    scratch[16] = '\0'; /* make minute a string */
    tm.ti_min = atoi(&scratch[14]);
    scratch[19] = '\0'; /* make second a string */
	tm.ti_sec = atoi(&scratch[17]);
    tm.ti_hund = 0;

    dat = dostounix(&dt,&tm);
    strncpy(scratch,ctime(&(time_t)dat),24);
    scratch[24]='\0';
    strcat(scratch,"\r\n");
    strcpy(s,scratch);

    addline(b,s,&len);
    strcat(b,buffer);
    len+=len2;
    if (b[len-1]!=26)
        b[len++]=26;

    len=(unsigned) strlen(buffer);
    iscan(curlsub);
    set_net_num(thisarea.net_num);
    m.stored_as=0L;
    m.storage_type=2;
    savefile(b,len,&m,subboards[curlsub].filename);
    strcpy(p.title, fido_title);
    p.title[80]=thisarea.net_num;
    p.anony=0;
    p.msg=m;
    p.ownersys=atoi(wwiv_node);
    p.owneruser=0;
    p.qscan=status.qscanptr++;
    save_status();

    time((long *)(&p.daten));
    p.status=0;
    p.status|=status_post_new_net;
    if ((subboards[curlsub].anony & anony_val_net) && (thisarea.val_incoming))
        p.status |=status_pending_net;
    if (nummsgs>=subboards[curlsub].maxmsgs)
    {
        i=1;
        dm=0;
        while ((dm==0) && (i<=nummsgs))
        {
            if ((get_post(i)->status & status_no_delete)==0)
                dm=i;
            ++i;
        }
        if (dm==0)
            dm=1;
        deletemsg(dm);
    }
    sub_dates[curlsub]=p.qscan;
    add_post(&p);
    ++status.msgposttoday;
    close_sub();
    if ((!(p.status & status_pending_net)) && (xsubs[curlsub].num_nets))
    {
        send_net_post(&p, subboards[curlsub].filename, curlsub);
    }
    free(buffer);
}

/* -------------------------------------------------------- */
/* Function to actually write incoming message to LOCAL.NET */
/* -------------------------------------------------------- */

void send_mail(void)
{
    char netfile[81], source[81], mailtime[81],title[81],
    message[80],scratch[80];
    int p0file,length,i;
    unsigned int *list;
    net_header_rec nh;
    struct date dt;
    struct time tm;
    char *month[] = {"Bug","Jan","Feb","Mar","Apr","May","Jun",
                     "Jul","Aug","Sep","Oct","Nov","Dec"};

    /* If not a valid message (2,3,5,7,26), then go back */

    if ((msgtype==1) || (msgtype==6))
        return;

    /* Set WWIV Message Header Variables */

    nh.touser = user_to_num;
    nh.tosys = net_networks[net_num].sysnum;
    nh.fromsys=atoi(wwiv_node);
    nh.fromuser=0;
    nh.list_len=0;
    nh.main_type = msgtype;
    if ((nh.main_type==3) || (nh.main_type==5))
        sub_type=atoi(thisarea.subtype);
    else
        sub_type=0;

    nh.minor_type = sub_type;
    nh.method = 0;
    time((long *)&(nh.daten));
    strcpy(title,fido_title);

    if ((debug>0) && (debug<3))
    {
        sprintf(message,"Sending Message #%4d in area : %s",count,curr_conf_name);
        length=strlen(message);
        printf(message);
        printf("     ");
        length+=5;
        for (i=0;i<length;i++)
            printf("\b");
        for (i=0;i<length;i++)
            printf(" ");
        for (i=0;i<length;i++)
            printf("\b");
    }

    count++;

    sprintf(source,"%s (%s)\r\n",fido_from_user,origi_node);
    if (debug==2)
    {
        printf(source);
    }

    /* Get size of message and check for size... */
    size=(unsigned) strlen(buffer);

    if (size>30000L)
    {
        printf("þ Error!  Message exceeds WWIV 32k Limit.  Tossing to bad!\r\n");
        write_log("þ Error!  Message exceeds WWIV 32k Limit.  Tossing to bad!\r\n");
        toss_to_bad(message_header, buffer,0);
        free(buffer);
        return;
    }

    strcpy(netfile,net_data);
    strcat(netfile,"LOCAL.$$$");

    if (debug)
    {
        sprintf(scratch,"þ Sending type %d/%u message to %s on %s\n",nh.main_type,nh.minor_type,netfile,net_networks[net_num].name);
        write_log(scratch);
    }

    p0file = sh_open(netfile, O_RDWR | O_APPEND | O_BINARY, S_IREAD | S_IWRITE);
    if (p0file == -1)
        p0file = sh_open(netfile, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);

    if (p0file == -1)
    {
        perror("Could not open LOCAL.NET file for append or write! Tossing to Bad");
        write_log("Could Not Open LOCAL.NET file for append or write!\n");
        toss_to_bad(message_header, buffer,0);
        free(buffer);
        return;
    }

    sh_lseek(p0file,0L,SEEK_END);

    /* Get date/time of Email creation. */
    /* Ditto the \015 for the time/date string. */

    strncpy(scratch,fido_date_line,20);
    scratch[20]='\0';

    /* Build the date - cumbersome, but it needs to be done */
	/* Build tblock as we go along so we can have a pretty date */
	scratch[2] = '\0';	/* make day a string */
	dt.da_day = atoi(scratch);
	for (i=1;i<13;i++) 
        if (strncmpi(month[i],&scratch[3],3) == 0)
        {
			dt.da_mon = i;
			break; 
		}
	scratch[9] = '\0';	/* make year a string */
    dt.da_year = atoi(&scratch[7]);
    if (dt.da_year>90)
        dt.da_year += 1900;
    else
        dt.da_year += 2000;
    scratch[13] = '\0'; /* make hour a string */
	tm.ti_hour = atoi(&scratch[11]);
	scratch[16] = '\0'; /* make minute a string */
	tm.ti_min = atoi(&scratch[14]);
	scratch[19] = '\0'; /* make second a string */
	tm.ti_sec = atoi(&scratch[17]);
	tm.ti_hund = 0;
    nh.daten = dostounix(&dt,&tm);
    strncpy(scratch,ctime(&(time_t)nh.daten),24);
    scratch[24]='\0';
    strcat(scratch,"\r\n");
    strcpy(mailtime,scratch);

/*
* Compute the actual length of network packet. The extra 1 is
* to allow for the NULL at then end of the title string.
* If message is type 7 or 26, add in size of appropriate field,
* adding 1 to each to allow for the trailing NULL.
*/

    nh.length = (unsigned long) size +
                (unsigned long) strlen(source) +
                (unsigned long) strlen(title) + 1 +
                (unsigned long) strlen(mailtime);
    if (msgtype==7)
        nh.length+=strlen(fido_to_user)+1;
    if (msgtype==26)
        nh.length+=strlen(thisarea.subtype)+1;

/*
* Write out the netpacket in chunks;
* Let DOS buffer them together.
*/

/* Write Net Header */

    if (sh_write(p0file, &nh, sizeof(nh)) == -1)
    {
        perror("Error writing Net Header to LOCAL.NET");
        toss_to_bad(message_header, buffer,0);
        sh_close(p0file);
        free(buffer);
        return;
    }

/* Write list_len (which should always be 0!) */

    if (sh_write(p0file, (void *)list, 2*(nh.list_len)) == -1)
    {
        perror("Error writing Net Header to LOCAL.NET");
        toss_to_bad(message_header, buffer,0);
        sh_close(p0file);
        free(buffer);
        return;
    }

/* Write Mail-To-User Name if message is type 7 email */

    if (msgtype==7)
    {
        if (sh_write(p0file, fido_to_user, strlen(fido_to_user) + 1) == -1)
        {
            perror("Error writing dest user to LOCAL.NET");
            toss_to_bad(message_header, buffer,0);
            sh_close(p0file);
            free(buffer);
            return;
        }
    }

/* Write Subtype if message is type 26 */

    if (msgtype==26)
    {
        if (sh_write(p0file, thisarea.subtype, strlen(thisarea.subtype)+1) == -1)
        {
            perror("Error writing Subtype to LOCAL.NET");
            toss_to_bad(message_header, buffer,0);
            sh_close(p0file);
            free(buffer);
            return;
        }
    }

/* Title needs to be stored with its terminating NULL */

    if (sh_write(p0file, title, strlen(title) + 1) == -1)
    {
        perror("Error writing title to LOCAL.NET");
        toss_to_bad(message_header, buffer,0);
        sh_close(p0file);
        free(buffer);
        return;
    }

/* Write source address */

    if (sh_write(p0file, source, strlen(source)) == -1)
    {
        perror("Error writing source address to LOCAL.NET");
        toss_to_bad(message_header, buffer,0);
        sh_close(p0file);
        free(buffer);
        return;
    }

/* Write time of message */

    if (sh_write(p0file, mailtime, strlen(mailtime)) == -1)
    {
        perror("Error writing time string to LOCAL.NET");
        toss_to_bad(message_header, buffer,0);
        sh_close(p0file);
        free(buffer);
        return;
    }

/* Write the message itself */

    if (sh_write(p0file, buffer, size) == -1)
    {
        perror("Error writing MailFile to LOCAL.NET");
        toss_to_bad(message_header, buffer,0);
        free(buffer);
        sh_close(p0file);
        return;
    }

    sh_close(p0file);
    free(buffer);
    return;
}
/* -------------------------------------------------------------------- */
/* Function to actually write incoming Unknown Messages To CHECKME.NET  */
/* -------------------------------------------------------------------- */

void check_mail(void)
{
    char netfile[81], source[81], mailtime[81],title[81],scratch[80];
    int p0file,i;
    unsigned int *list;
    net_header_rec nh;
    struct date dt;
    struct time tm;
    char *month[] = {"Bug","Jan","Feb","Mar","Apr","May","Jun",
                     "Jul","Aug","Sep","Oct","Nov","Dec"};

    /* If not a valid message (2,3,5,7,26), then go back */

    if ((msgtype==1) || (msgtype==6))
        return;

    /* Set WWIV Message Header Variables */

    nh.touser = user_to_num;
    nh.tosys = net_networks[net_num].sysnum;
    nh.fromsys=atoi(wwiv_node);
    nh.fromuser=0;
    nh.list_len=0;
    nh.main_type = msgtype;
    if ((nh.main_type==3) || (nh.main_type==5))
        sub_type=atoi(thisarea.subtype);
    else
        sub_type=0;

    nh.minor_type = sub_type;
    nh.method = 0;
    time((long *)&(nh.daten));
    strcpy(title,fido_title);

    count++;

    sprintf(source,"%s (%s)\r\n",fido_from_user,origi_node);

    /* Get size of message and check for size... */
    size=(unsigned) strlen(buffer);

    strcpy(netfile,net_data);
    strcat(netfile,"CHECKME.NET");

    p0file = sh_open(netfile, O_RDWR | O_APPEND | O_BINARY, S_IREAD | S_IWRITE);
    if (p0file == -1)
        p0file = sh_open(netfile, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);

    if (p0file == -1)
    {
        perror("Could not open CHECKME.NET file for append or write! Tossing to Bad");
        write_log("Could Not Open CHECKME.NET file for append or write!\n");
        toss_to_bad(message_header, buffer,0);
        free(buffer);
        return;
    }

    sh_lseek(p0file,0L,SEEK_END);

    /* Get date/time of Email creation. */
    /* Ditto the \015 for the time/date string. */

    strncpy(scratch,fido_date_line,20);
    scratch[20]='\0';

    /* Build the date - cumbersome, but it needs to be done */
	/* Build tblock as we go along so we can have a pretty date */
	scratch[2] = '\0';	/* make day a string */
	dt.da_day = atoi(scratch);
	for (i=1;i<13;i++) 
        if (strncmpi(month[i],&scratch[3],3) == 0)
        {
			dt.da_mon = i;
			break; 
		}
	scratch[9] = '\0';	/* make year a string */
    dt.da_year = atoi(&scratch[7]);
    if (dt.da_year>90)
        dt.da_year += 1900;
    else
        dt.da_year += 2000;
    scratch[13] = '\0'; /* make hour a string */
	tm.ti_hour = atoi(&scratch[11]);
	scratch[16] = '\0'; /* make minute a string */
	tm.ti_min = atoi(&scratch[14]);
	scratch[19] = '\0'; /* make second a string */
	tm.ti_sec = atoi(&scratch[17]);
	tm.ti_hund = 0;
    nh.daten = dostounix(&dt,&tm);
    strncpy(scratch,ctime(&(time_t)nh.daten),24);
    scratch[24]='\0';
    strcat(scratch,"\r\n");
    strcpy(mailtime,scratch);

/*
* Compute the actual length of network packet. The extra 1 is
* to allow for the NULL at then end of the title string.
* If message is type 7 or 26, add in size of appropriate field,
* adding 1 to each to allow for the trailing NULL.
*/

    nh.length = (unsigned long) size +
                (unsigned long) strlen(source) +
                (unsigned long) strlen(title) + 1 +
                (unsigned long) strlen(mailtime);
    if (msgtype==7)
        nh.length+=strlen(fido_to_user)+1;
    if (msgtype==26)
        nh.length+=strlen(thisarea.subtype)+1;

/*
* Write out the netpacket in chunks;
* Let DOS buffer them together.
*/

/* Write Net Header */

    if (sh_write(p0file, &nh, sizeof(nh)) == -1)
    {
        perror("Error writing Net Header to CHECKME.NET");
        toss_to_bad(message_header, buffer,0);
        sh_close(p0file);
        free(buffer);
        return;
    }

/* Write list_len (which should always be 0! */

    if (sh_write(p0file, (void *)list, 2*(nh.list_len)) == -1)
    {
        perror("Error writing Net Header to CHECKME.NET");
        toss_to_bad(message_header, buffer,0);
        sh_close(p0file);
        free(buffer);
        return;
    }

/* Write Mail-To-User Name if message is type 7 email */

    if (msgtype==7)
    {
        if (sh_write(p0file, fido_to_user, strlen(fido_to_user) + 1) == -1)
        {
            perror("Error writing dest user to CHECKME.NET");
            toss_to_bad(message_header, buffer,0);
            sh_close(p0file);
            free(buffer);
            return;
        }
    }

/* Write Subtype if message is type 26 */

    if (msgtype==26)
    {
        if (sh_write(p0file, thisarea.subtype, strlen(thisarea.subtype)+1) == -1)
        {
            perror("Error writing Subtype to CHECKME.NET");
            toss_to_bad(message_header, buffer,0);
            sh_close(p0file);
            free(buffer);
            return;
        }
    }

/* Title needs to be stored with its terminating NULL */

    if (sh_write(p0file, title, strlen(title) + 1) == -1)
    {
        perror("Error writing title to CHECKME.NET");
        toss_to_bad(message_header, buffer,0);
        sh_close(p0file);
        free(buffer);
        return;
    }

/* Write source address */

    if (sh_write(p0file, source, strlen(source)) == -1)
    {
        perror("Error writing source address to CHECKME.NET");
        toss_to_bad(message_header, buffer,0);
        sh_close(p0file);
        free(buffer);
        return;
    }

/* Write time of message */

    if (sh_write(p0file, mailtime, strlen(mailtime)) == -1)
    {
        perror("Error writing time string to CHECKME.NET");
        toss_to_bad(message_header, buffer,0);
        sh_close(p0file);
        free(buffer);
        return;
    }

/* Write the message itself */

    if (sh_write(p0file, buffer, size) == -1)
    {
        perror("Error writing MailFile to CHECKME.NET");
        toss_to_bad(message_header, buffer,0);
        free(buffer);
        sh_close(p0file);
        return;
    }

    sh_close(p0file);
    free(buffer);
    return;
}


/* -------------------------------------------------------- */
/* Function to read in message and translate                */
/* -------------------------------------------------------- */

void load_workspace(void)
{
    int k,j,char_count,line_count,flag;
    char temp1[10],*temp,*temp2,t1[181];
    unsigned char   ch;
    delete_me=0;
    /* Allocate Main Message Buffer to 40k to allow for plenty of room */

    buffer = malloc(40*1024);

    /* Allocate Temporary Working buffers to 8k to allow for plenty of room */

    temp=malloc(18*1024);
    temp2=malloc(18*1024);

    /* Clear variables */

    strcpy(buffer,"");
    char_count=line_count=0;
    strcpy(temp,"");
    strcpy(temp1,"");
    strcpy(temp2,"");
    line_num=0;

    if ((msgtype!=2) && (msgtype!=7))
        if (cfg.color_to)
            sprintf(buffer,"1To9.......... 2%s\r\n\r\n",fido_to_user);
        else
            sprintf(buffer,"TO: %s\r\n\r\n",fido_to_user);

    while (ch=0, sh_read(incoming_file,(void *)&ch,1), (ch!=0)&&(exit_status==0))
    {

        if (debug==2)
        {
            printf("%c",ch);
            if (ch=='\r')
                printf("\n");

        }
        switch (ch)
        {
            case ' ':   /* End of word  */
                sprintf(temp1,"%c",ch);
                strcat(temp,temp1);
                strcat(temp2,temp1);
                char_count++;
                if (strstr(temp,"Origin:")!=NULL)
                    flag=1;
                if (flag)
                    break;

                /* If line getting too long, let's break it up */

                if ((line_count+char_count)>77)
                {
                    strcat(buffer,"\r\n");
                    line_num++;
                    strcat(buffer,temp);
                    line_count=char_count;
                    char_count=0;
                    strcpy(temp,"");
                    strcpy(temp2,"");
                } else
                {
                    /* Let's hide the SEEN-BY lines with WWIV Control Codes */
                    if (line_count==0)
                        if (!strnicmp(temp,"SEEN-BY:",8))
                            strcat(buffer,"0");

                    strcat(buffer,temp);
                    line_count+=char_count;
                    char_count=0;
                    strcpy(temp,"");
                }
                break;

            case '\n':  /* Ignore Linefeeds */
                break;

            case '\r':  /* End of line */
                if ((cfg.add_hard_cr) && (!cfg.hard_to_soft))
                {
                    sprintf(temp1,"%c",ch);
                    strcat(temp,temp1);
                }
                if (cfg.hard_to_soft)
                {
                    sprintf(temp1,"%c",0x8d);
                    strcat(temp,temp1);
                }
                if (cfg.add_line_feed)
                {
                    strcpy(temp1,"\n");
                    strcat(temp,temp1);
                    char_count+=2;
                }
                /*  If we've found the origin line, we'll use that
                    to get the authors node number! */
                if ((strstr(temp,"Origin:")!=NULL) && (!msgid))
                {
                    k=strlen(temp);
                    for (j=k;(temp[j]!='(') && (j>0);j--);

                    strcpy(origi_node,&temp[j+1]);
                    k=strlen(origi_node);
                    while((origi_node[k]!=')') && (k>0))
                    {
                        origi_node[k]=0;
                        k--;
                    }
                    if (origi_node[k]==')')
                        origi_node[k]=0;
                    strcpy(dupeadr,origi_node);
                }
                if (flag)
                {
                    /* Let's show the via lines after all */

                    if (!strnicmp(temp,"0VIA",5))
                    {
                        strcpy(&temp[0],&temp[2]);
                    }
                    if (!strnicmp(temp,"0FLAGS",7))
                    {
                        if (strstr(temp,"KFS")!=NULL)
                        {
                            delete_me=1;
                            msgtype=1;
                        }
                    }

                    flag=0;
                }

                /* Let's try to get address from MSGID */
                if (!strnicmp(temp,"0MSGID: ",9))
                {
                    strcpy(origi_node,&temp[9]);
                    sscanf(origi_node,"%s %s",t1,msgidd);
                    strcpy(dupeadr,t1);
                    msgid2=1;
                }

                if (msgid2)
                {
                    if (dupeadr[0]!=0)
                    {
                        while (dupeadr[0] == ' ')
                            strcpy(&dupeadr[0],&dupeadr[1]);
                        for (k=0;k<strlen(dupeadr);k++)
                        {
                            if (dupeadr[k]==' ')
                                dupeadr[k]=0;
                            if (dupeadr[k]=='@')
                                dupeadr[k]=0;
                        }
                    }
                    if (check_for_dupe())
                    {
                        sprintf(t1,"þ Duplicate Message (MSGID: %s) Arrived!  Message Killed!\n",msgidd);
                        dupe_msg++;
                        valid_post--;
                        write_log(t1);
                        msgtype=1;
                        msgid2=0;
                    }
                    msgidd[0]=0;
                    dupeadr[0]=0;
                    msgid2=0;
                    for (k=0;k<10;k++)
                        msgidd[k]=0;
                    for (k=0;k<40;k++)
                        dupeadr[k]=0;
                }
                /* Clean up origi_node line */

                if (origi_node[0]!=0)
                {
                    while (origi_node[0] == ' ')
                        strcpy(&origi_node[0],&origi_node[1]);
                    for (k=0;k<strlen(origi_node);k++)
                    {
                        if (origi_node[k]==' ')
                            origi_node[k]=0;
                        if (origi_node[k]=='@')
                            origi_node[k]=0;
                    }
                    strcpy(dupeadr,origi_node);
                }


                /*  In case the SEEN-BY line is short, we'll go ahead
                    and hide it here, if the lines starts with SEEN-BY */

                if (!strnicmp(temp,"SEEN-BY:",8))
                {
                    strcat(buffer,"0");
                }

                /*  If there's a physical TO: line in the message
                    that means it's probably gated using FSC-0035 */

                if (!strnicmp(temp2,"TO: ",4))
                {
                    if (msgtype==7)
                    {
                        strcpy(&temp2[0],&temp2[4]);
                        strcpy(fido_to_user,temp2);
                        sprintf(temp2,"þ Gateing email from %s to %s\n",fido_from_user, fido_to_user);
                        write_log(temp2);
                        gated_mail++;
                    }
                }
                strcpy(temp2,"");
                strcat(buffer,temp);
                strcpy(temp,"");
                line_num++;
                char_count=0;
                line_count=0;
                break;
            case '\001':    /* If ^A, change to WWIV Control Code to hide line */
                strcpy(temp,"0");
                flag=1;
                break;
            case 141:
                break;
            case '\215':    /* Not quite sure why this is here, but it is...*/
                if (cfg.soft_to_hard)
                {
                    strcpy(buffer,"\r");
                    char_count+=2;
                }
                else
                if (cfg.add_soft_cr)
                {
                    sprintf(buffer,"%c",'\215');
                    char_count+=2;
                }

                if (cfg.add_line_feed)
                {
                    strcat(buffer,"\n");
                    char_count+=2;
                }
                break;
            default:        /* Otherwise, process as normal... */
                if ((msgtype==26) || (msgtype==3) || (msgtype==5))
                   /* if ((thisarea.translate==1) && (ch>32))
                        ch=xlat[ch-33].in;     MMH mod */
                ch==ch;  /* MMH mod */
                sprintf(temp1,"%c",ch);
                strcat(temp,temp1);
                strcat(temp2,temp1);
                char_count++;
                break;
        }
    }
    /* Free the temporary buffers */
    free(temp);
    free(temp2);


    if (twit_me(fido_from_user,origi_node))
        msgtype=1;
    if (incoming_invalid)
        if (cfg.bounce_mail)
            bounce_mail();
        else
        {
            toss_to_bad(message_header,buffer,0);
            send_nag("4WWIVTOSS: Bad Email Arrived!  Check BADECHO Directory!");
        }
    else
    if ((msgtype!=1) && (msgtype!=6))
    {
        if (testing_new)
        {
            if ((msgtype!=2))
            {
                if (msgtype==7)
                    send_mail();
                else
                {
                    send_mail2();
                }
            }
            else
                send_email2();
        }
        else
        {
            send_mail();
        }
    }
    else
    {
        free(buffer);
    }
}

/*  ------------------------------------------------
 *  This function is called for each message-packet
 *  inside each incoming message bundle.   It is
 *  responsible for converting the message from
 *  FIDO format into WWIV format
 *  ------------------------------------------------  */

void process_incoming_message(void)
{
    unsigned char   ch;
    int             p1,counter;
    long            file_pos_save;
    char s[81],*badmessage,log[81],t1[80];

    int i,k,ok,ok1,temp_out_node,temp_out_net;
    int cycle;
    long this_pos,message_length;
    FIDOADR fadr;

    user_to_num=0;
    msgid=0;
    msgid2=0;
    msgidd[0]=0;
    /*  -----------------------------------------------------------
     *  the header ID word has already been read, so read past that
     *  read the orig-node, dest-node, orig-net, dest-net,
     *  attribute, and cost
     *  -----------------------------------------------------------  */
    sh_read(incoming_file,(void *)&message_header.orig_node,2);
    sh_read(incoming_file,(void *)&message_header.dest_node,2);
    sh_read(incoming_file,(void *)&message_header.orig_net,2);

    sh_read(incoming_file,(void *)&message_header.dest_net,2);
    sh_read(incoming_file,(void *)&message_header.attribute,2);
    sh_read(incoming_file,(void *)&message_header.cost,2);
    if (debug)
    {

        if (message_header.attribute)
        {
            printf("¯ Attr  : %X\r\n",message_header.attribute);
            printf("        : ");
            if (message_header.attribute & MSGPRIVATE)
                printf("PRIVATE ");
            if (message_header.attribute & MSGCRASH)
                printf("CRASH ");
            if (message_header.attribute & MSGREAD)
                printf("READ ");
            if (message_header.attribute & MSGSENT)
                printf("SENT ");
            if (message_header.attribute & MSGFILE)
                printf("FILE_ATTACH ");
            if (message_header.attribute & MSGTRANSIT)
                printf("IN-TRANSIT ");
            if (message_header.attribute & MSGORPHAN)
                printf("ORPHAN ");
            if (message_header.attribute & MSGKILL)
                printf("KILL ");
            if (message_header.attribute & MSGLOCAL)
                printf("LOCAL ");
            if (message_header.attribute & MSGHOLD)
                printf("HOLD ");
            printf("\r\n");
        }
    }
    out_node=message_header.orig_node;
    out_net=message_header.orig_net;
    my_net=message_header.dest_net;
    my_node=message_header.dest_node;
    my_zone=packet_header.DestZone;
    for (cycle=0;cycle<11;cycle++)
    {
        if ((cfg.aka_list[cycle].net==my_net) && (cfg.aka_list[cycle].node==my_node));
        {
            my_zone=cfg.aka_list[cycle].zone;;
            my_point=cfg.aka_list[cycle].point;
            cycle=11;
        }
    }
    cycle=0;
    giveup_timeslice();
    this_pos=tell(incoming_file);
    buffer2=malloc(40*1024);

    /*  ----------------
     *  read in the date
     *  ----------------  */
    p1=0;
    do {
        ch=0;
        sh_read(incoming_file,(void *)&ch,1);
        if (p1<20)
	    message_header.datetime[p1++]=ch;
    } while (ch);
    strcpy(fido_date_line, message_header.datetime);
    /*  ------------------
     *  get the "TO:" user
     *  ------------------  */
    p1=0;
    do {
        ch=0;
        sh_read(incoming_file,(void *)&ch,1);
        if (p1<80)
	    fido_to_user[p1++]=ch;
    } while (ch);
    /*  --------------------
     *  get the "FROM:" user
     *  --------------------  */
    p1=0;
    do {
        ch=0;
        sh_read(incoming_file,(void *)&ch,1);
        if (p1<80)
            fido_from_user[p1++]=ch;
    } while (ch);
    /*  ----------------
     *  get the "Title:"
     *  ----------------  */
    p1=0;
    do {
        ch=0;
        sh_read(incoming_file,(void *)&ch,1);
        if (p1<80)                          /* WWIV title limitation 50 chars */
            fido_title[p1++]=ch;
    } while (ch);
    this_pos=tell(incoming_file);
    message_length=0;
    while (ch=0,                                /* init to zero in case EOF */
       sh_read(incoming_file,(void *)&ch,1),
	   (ch!=0)&&(exit_status==0)) {
        if (message_length < MAX_MSG_SIZE)
            buffer2[message_length++]=ch;
    }

    buffer2[message_length++]=0;
    sh_lseek(incoming_file,this_pos,SEEK_SET);
    /*  --------------------------------------------------------
     *  look for "AREA" by reading the first line of the message
     *  --------------------------------------------------------  */
    /*  ----------------------------------------------------------
     *  save position of beginning of message in case needed later
     *  ----------------------------------------------------------  */
    file_pos_save=tell(incoming_file);              /* Turbo C */
    /*  --------------------------------------------------
     *  read the first line (terminated by carriage-return
     *  --------------------------------------------------  */
    p1=0;
    do {
        ch=0;
        sh_read(incoming_file,(void *)&ch,1);
        s[p1++]=ch;
    } while ((ch!=0)&&(ch!=0x0d)&&(p1<=80));
    s[--p1]=0;                      /* wipe out the carriage return */
		
    /*  -------------------------------------------------
     *  Attempt to determine the appropriate message area
     *  in which to place the message
     *  -------------------------------------------------  */
    curr_conf_name[0]=0;
    /*  ----------------------------
     *  allow ^AAREA as well as AREA
     *  ----------------------------  */
    if (!strnicmp(s,"\001AREA:",6))
        strcpy(&s[0], &s[1]);
    if (!strnicmp(s,"AREA:",5))
    {
        strcpy(curr_conf_name,&s[5]);
        /*  ---------------------------------------------------------
         *  remove any blanks between "AREA:" and the conference name
         *  ---------------------------------------------------------  */
        while (curr_conf_name[0] == ' ')
            strcpy(&curr_conf_name[0],&curr_conf_name[1]);
        ok1=current_area=0;
        for (counter=1;counter<=num_areas;counter++)
        {
            read_area(counter,&thisarea);

            if (stricmp(curr_conf_name,thisarea.name)==0)
            {
                current_area=counter;
                counter=num_areas;
                net_num=thisarea.net_num;
                if (net_num>net_num_max)
                {
                    printf("þ Error!  Network Number %d Listed in AREAS.DAT Not defined in INIT!\r\n",net_num);
                    sprintf(log,"þ Error!  Network Number %d Listed in AREAS.DAT Not defined in INIT!\r\n",net_num);
                    write_log(log);
                    bad_post++;
                    badd_1++;
                    cur_post++;
                    msgtype=1;
                    sh_lseek(incoming_file,file_pos_save,SEEK_SET);
                    badmessage=malloc(32*1024);
                    sprintf(badmessage,"");
                    while (ch=0,sh_read(incoming_file,(void *)&ch,1),ch!=0)
                    {
                        sprintf(s,"%c",ch);
                        strcat(badmessage,s);
                    }
                    toss_to_bad(message_header, badmessage,0);

                    free(badmessage);
                    free(buffer2);
                    return;

                }
                if (thisarea.type==2)
                {
                    sh_lseek(incoming_file,file_pos_save,SEEK_SET);
                    badmessage=malloc(32*1024);
                    sprintf(badmessage,"");
                    while (ch=0,sh_read(incoming_file,(void *)&ch,1),ch!=0)
                    {
                        sprintf(s,"%c",ch);
                        strcat(badmessage,s);
                    }
                    toss_to_bad(message_header, badmessage,1);
                    free(badmessage);
                    free(buffer2);
                    thisarea.count++;
                    write_area(current_area,&thisarea);
                    return;
                }

                thisarea.count++;
                write_area(current_area,&thisarea);
                ok1=1;
                strcpy(net_data,net_networks[net_num].dir);
                sprintf(log,"Area %s goes to Network # %d (%s) in %s\r\n",thisarea.name,thisarea.net_num,net_networks[net_num].name,net_networks[net_num].dir);
                if (debug)
                {
                    write_log(log);
                    printf("%s",log);
                }
            }
        }
        if (!ok1)
        {
            if (debug)
            {
                printf("þ Error!  Area: %s Not defined in AREAS.DAT!\r\n",curr_conf_name);
            }
            sprintf(log,"þ Error!  Area: %s Not defined in AREAS.DAT!\r\n",curr_conf_name);
            write_log(log);
            bad_post++;
            badd_1++;
            cur_post++;
            msgtype=1;
            sh_lseek(incoming_file,file_pos_save,SEEK_SET);
            badmessage=malloc(32*1024);
            sprintf(badmessage,"");
            while (ch=0,sh_read(incoming_file,(void *)&ch,1),ch!=0)
            {
                sprintf(s,"%c",ch);
                strcat(badmessage,s);
            }
            toss_to_bad(message_header, badmessage,0);
            if (cfg.auto_add)
            {
                write_log("Adding to AREAS.DAT");
                sprintf(s,"1WWIVTOSS:Auto-Added %s - Please Setup in WSETUP!",curr_conf_name);
                send_nag(s);
                num_areas=cfg.num_areas;
                i=cfg.num_areas;
                strcpy(thisarea.name,curr_conf_name);
                strcpy(thisarea.comment,"Auto-Added Area");
                strcpy(thisarea.subtype,"-1");
                strcpy(thisarea.origin,cfg.origin_line);
                thisarea.alias_ok=0;
                thisarea.pass_color=0;
                thisarea.high_ascii=0;
                thisarea.net_num=0;
                thisarea.group=0;
                thisarea.count=0;
                thisarea.keep_tag=0;
                thisarea.def_origin=1;
                thisarea.translate=0;
                thisarea.feed[0].zone=my_zone;
                thisarea.feed[0].net=packet_header.orig_net;
                thisarea.feed[0].node=packet_header.orig_node;
                thisarea.feed[0].point=packet_header.OrigPoint;
                write_area(i,&thisarea);
                cfg.num_areas++;
                write_config();
            }
            free(badmessage);
            free(buffer2);
            return;


    } else
    {
        if (thisarea.subtype[0]>57)
            msgtype=26;
        else
            msgtype=3;
        if (stricmp(thisarea.subtype,"-1")==0)
        {
            printf("þ Error!  Area %s Not Set Up Yet!\r\n",curr_conf_name);
            sprintf(log,"þ Error!  Areas %s Not Set Up Yet!\r\n",curr_conf_name);
            write_log(log);
            bad_post++;
            badd_1++;
            cur_post++;
            msgtype=1;
            sh_lseek(incoming_file,file_pos_save,SEEK_SET);
            badmessage=malloc(32*1024);
            sprintf(badmessage,"");
            while (ch=0,sh_read(incoming_file,(void *)&ch,1),ch!=0)
            {
                sprintf(s,"%c",ch);
                strcat(badmessage,s);
            }
            toss_to_bad(message_header, badmessage,0);

            free(badmessage);
            free(buffer2);
            return;

        }
        if ((thisarea.subtype[0]==48) || (thisarea.type==0))
        {
            msgtype=1;
            passthru++;
        }
        else
            valid_post++;
        cur_post++;
        temp_out_node=out_node;
        temp_out_net=out_net;
        for (cycle=0;cycle<11;cycle++)
        {
            if (cfg.aka_list[cycle].wwiv_netnum==thisarea.net_num);
            {
                my_zone=cfg.aka_list[cycle].zone;
                my_net=cfg.aka_list[cycle].net;
                my_node=cfg.aka_list[cycle].node;
                my_point=cfg.aka_list[cycle].point;
                cycle=11;
            }
        }
        cycle=0;
        for (cycle=0;cycle<MAX_NODES;cycle++)
        {
            if ((thisarea.feed[cycle].zone!=0) && (thisarea.feed[cycle].zone!=999) && ((thisarea.feed[cycle].node!=temp_out_node)||(thisarea.feed[cycle].net!=temp_out_net)))
            {
                out_zone=who_to.zone=thisarea.feed[cycle].zone;
                out_net=who_to.net=thisarea.feed[cycle].net;
                out_node=who_to.node=thisarea.feed[cycle].node;
                out_point=who_to.point=thisarea.feed[cycle].point;
                for (i=0;i<MAX_NODES;i++)
                {
                    fidoadr_split(nodes[i].address,&fadr);
                    if ((fadr.zone==out_zone) && (fadr.net==out_net) && (fadr.node==out_node) && (fadr.point==out_point))
                    {
                        attribute=0;
                        attribute=attribute | MSGLOCAL;
                        switch (nodes[i].archive_status)
                        {
                            case 0:
                                break;
                            case 1:
                                attribute = attribute | (MSGHOLD);
                                break;
                            case 2:
                                break;
                            case 3:
                                attribute = attribute | (MSGCRASH);
                                break;

                        }
                    }
                }
                send_message(1,buffer2);
                passthru++;
                if (cfg.log_feeds)
                {
                    sprintf(log,"þ Sending Message on %s to %d:%d/%d.%d\n",curr_conf_name,out_zone,out_net,out_node,out_point);
                    write_log(log);
                }
                outchr(00);
                outchr(00);             /* message-type == 0000 (hex) terminates packet */
                sh_close(outbound_file);
            }
        }
        out_node=temp_out_node;
        out_net=temp_out_net;
        }
    }

    /*  -----------------------------------------------
     *  if first line wasn't AREA:, reset file position
     *  -----------------------------------------------  */
    if (curr_conf_name[0] == 0)
    {
        sh_lseek(incoming_file,file_pos_save,SEEK_SET);
        if ((stricmp(fido_to_user,"AREAFIX")==0) || (stricmp(fido_to_user,"ALLFIX")==0))
        {
            msgtype=4;
//            process_areafix_message();
        }
        else
        {
            sprintf(log,"þ Incoming mail for %s from %s\n",fido_to_user,fido_from_user);
            write_log(log);
            if ((fido_to_user[0]=='`') && (fido_to_user[1]=='`'))
            {
                msgtype=7;
                user_to_num=0;
                gated_mail++;
                printf("Gated Mail\r\n");
            }
            else
            {
                if (!strnicmp(fido_to_user,"WWIVTOSS GATE",13))
                {
                    user_to_num=0;
                    msgtype=7;
                }
                else
                {
                    incoming_mail();
                    msgtype=2;
                    if ((user_to_num==0) || (cfg.skip_mail))
                    {
                        msgtype=1;
                        my_node=message_header.orig_node;
                        my_net=message_header.orig_net;
                        out_net=message_header.dest_net;
                        out_node=message_header.dest_node;
                        my_zone=packet_header.DestZone;
                        out_zone=packet_header.OrigZone;
                        my_point=packet_header.DestPoint;
                        out_point=packet_header.OrigPoint;
                        send_message(5,buffer2);
                        cur_mail++;

                    }
                }
            }
            temp_out_node=my_node;
            temp_out_net=my_net;
            sprintf(log,"þ Incoming mail from %d/%d to %d/%d\r\n",out_net,out_node,my_net,my_node);
            write_log(log);
            if (debug)
                printf("%s",log);
            for (cycle=0;cycle<11;cycle++)
            {
                if ((cfg.aka_list[cycle].net==temp_out_net) &&(cfg.aka_list[cycle].node==temp_out_node))
                {
                    my_zone=cfg.aka_list[cycle].zone;
                    my_net=cfg.aka_list[cycle].net;
                    my_node=cfg.aka_list[cycle].node;
                    my_point=cfg.aka_list[cycle].point;
                    net_num=cfg.aka_list[cycle].wwiv_netnum;
                    strcpy(net_data,net_networks[net_num].dir);
                    sprintf(wwiv_node,"%d",cfg.aka_list[cycle].wwiv_node);
                    cycle=11;
                }
            }
            if (net_num>net_num_max)
            {
                printf("þ Error!  Network Number %d Not defined in INIT!\r\n",net_num);
                sprintf(log,"þ Error!  Network Number %d Not defined in INIT!\r\n",net_num);
                write_log(log);
                bad_mail++;
                cur_mail++;
                msgtype=1;
                sh_lseek(incoming_file,file_pos_save,SEEK_SET);
                badmessage=malloc(32*1024);
                sprintf(badmessage,"");
                while (ch=0,sh_read(incoming_file,(void *)&ch,1),ch!=0)
                {
                    sprintf(s,"%c",ch);
                    strcat(badmessage,s);
                }
                toss_to_bad(message_header, badmessage,0);

                free(badmessage);
                free(buffer2);
                return;

            }
            sub_type=0;
            if (!cfg.import_recd)
                if ((message_header.attribute & 0x0004) || (message_header.attribute & 0x0008))
                {
                    msgtype=1;  /* Already received! */
                    if (!debug)
                        printf("þ Already Received!\r\n");
                }
            }
            if (msgtype==4)
            {
                process_areafix_message();
                free(buffer2);
                return;
            }
        } else
            file_pos_save=tell(incoming_file);              /* Turbo C */
        ok=0;
        do
        {
            p1=0;
            do
            {
                ch=0;
                sh_read(incoming_file,(void *)&ch,1);
                s[p1++]=ch;
            } while ((ch!=0)&&(ch!=0x0d)&&(p1<=80));
            s[--p1]=0;                      /* wipe out the carriage return */

        /*  -------------------------------------------------
         *  Attempt to determine the appropriate MSGID
         *  -------------------------------------------------  */
        origi_node[0]=0;

        /*  ----------------------------
         *  allow ^AMSGID as well as MSGID
         *  ----------------------------  */
        if (!strnicmp(s,"\001MSGID: ",8))
            strcpy(&s[0], &s[1]);
        if (!strnicmp(s,"MSGID: ",7))
        {
            msgid=1;
            msgid2=1;
            strcpy(origi_node,&s[7]);
            sscanf(origi_node,"%s %s",t1,msgidd);
            strcpy(dupeadr,t1);
            if (origi_node[0]!=0)
            {
                while (origi_node[0] == ' ')
                    strcpy(&origi_node[0],&origi_node[1]);
                for (k=0;k<strlen(origi_node);k++)
                {
                    if (origi_node[k]==' ')
                        origi_node[k]=0;
                    if (origi_node[k]=='@')
                        origi_node[k]=0;
                }
                ok=1;
            }
        }
		
        /*  ----------------------------
         *  allow ^AINTL as well as INTL
         *  ----------------------------  */
		
		   if (!strnicmp(s,"\001INTL ",6))
            strcpy(&s[0], &s[1]);
        if (!strnicmp(s,"INTL ",5))
        {
            msgid=1;
            msgid2=1;
            strcpy(origi_node,&s[5]);
            sscanf(origi_node,"%s %s",t1,msgidd);
           /* strcpy(dupeadr,msgidd); */
			strcpy(origi_node,msgidd);
            if (origi_node[0]!=0)
            {
                while (origi_node[0] == ' ')
                    strcpy(&origi_node[0],&origi_node[1]);
                for (k=0;k<strlen(origi_node);k++)
                {
                    if (origi_node[k]==' ')
                        origi_node[k]=0;
                    if (origi_node[k]=='@')
                        origi_node[k]=0;
                }
                ok=1;
            }
        }
		
		
         if (origi_node[0]==0)
        {
            sprintf(origi_node,"%d:%d/%d",my_zone,temp_out_net,temp_out_node);  
            ok=1;
        }  
    } while (!ok);

    /*  -----------------------------------------------
     *  get the full message text into a temporary file
     *  it's treated as though it were entered with the
     *  full-screen editor.
     *  -----------------------------------------------  */
    load_workspace();
    free(buffer2);
    if (exit_status != 0)
    {
        return;
    }
}

/*  ------------------------------------------------------------------  */
/*  Go through and process .BAD messages                                */
/*  ------------------------------------------------------------------  */

int process_bad_message(void)
{
    unsigned char   ch;
    int             p1,counter;
    char s[81],log[81];
    int ok1;
    user_to_num=0;

    /*  -----------------------------------------------------------
     *  the header ID word has already been read, so read past that
     *  read the orig-node, dest-node, orig-net, dest-net,
     *  attribute, and cost
     *  -----------------------------------------------------------  */

    out_node=fdmsg.OrigNode;
    out_net=fdmsg.OrigNet;

    strcpy(fido_date_line, fdmsg.Date);
    strcpy(fido_from_user,fdmsg.FromUser);
    strcpy(fido_to_user,fdmsg.ToUser);
    strcpy(fido_title,fdmsg.Subject);
    /*  --------------------------------------------------
     *  read the first line (terminated by carriage-return
     *  --------------------------------------------------  */
    p1=0;
    do {
        ch=0;
        sh_read(incoming_file,(void *)&ch,1);
        s[p1++]=ch;
    } while ((ch!=0)&&(ch!=0x0d)&&(p1<=80));
    s[--p1]=0;                      /* wipe out the carriage return */
    /*  -------------------------------------------------
     *  Attempt to determine the appropriate message area
     *  in which to place the message
     *  -------------------------------------------------  */
    curr_conf_name[0]=0;
    /*  ----------------------------
     *  allow ^AAREA as well as AREA
     *  ----------------------------  */
    if (!strnicmp(s,"\001AREA:",6))
        strcpy(&s[0], &s[1]);
    if (!strnicmp(s,"AREA:",5))
    {
        strcpy(curr_conf_name,&s[5]);
        /*  ---------------------------------------------------------
         *  remove any blanks between "AREA:" and the conference name
         *  ---------------------------------------------------------  */
        while (curr_conf_name[0] == ' ')
            strcpy(&curr_conf_name[0],&curr_conf_name[1]);
        ok1=current_area=0;
        for (counter=1;counter<=num_areas;counter++)
        {
            read_area(counter,&thisarea);
            if (stricmp(curr_conf_name,thisarea.name)==0)
            {
                current_area=counter;
                counter=num_areas;
                net_num=thisarea.net_num;
                if (net_num>net_num_max)
                {
                    printf("þ Error!  Network Number %d Listed in AREAS.DAT Not defined in INIT!\r\n",net_num);
                    sprintf(log,"þ Error!  Network Number %d Listed in AREAS.DAT Not defined in INI!\r\n",net_num);
                    write_log(log);
                    bad_post++;
                    badd_1++;
                    cur_post++;
                    msgtype=1;
                    return 0;
                }
                ok1=1;
            }
        }
        if (!ok1)
        {
            if (debug)
            {
                printf("þ Error!  Area: %s Not defined in AREAS.DAT!\r\n",curr_conf_name);
            }
            sprintf(log,"þ Error!  Area: %s Not defined in AREAS.DAT!\r\n",curr_conf_name);
            write_log(log);
            badd_1++;
            bad_post++;
            cur_post++;
            msgtype=1;
            return 0;
        } else
        {
            msgtype=3;
            valid_post++;
            cur_post++;
        }
        if (stricmp(thisarea.subtype,"-1")==0)
        {
            printf("þ Error!  Area %s Not Set Up Yet!\r\n",curr_conf_name);
            sprintf(log,"þ Error!  Areas %s Not Set Up Yet!\r\n",curr_conf_name);
            write_log(log);
            bad_post++;
            badd_1++;
            valid_post--;
            msgtype=1;
            return 0;
        }
    }
    else
    {
        incoming_mail();
        msgtype=2;
        if (user_to_num==0)
        {
            msgtype=1;
            bad_mail++;
            return 0;
        }
    }

    /*  -----------------------------------------------
     *  get the full message text into a temporary file
     *  it's treated as though it were entered with the
     *  full-screen editor.
     *  -----------------------------------------------  */
    strcpy(net_data,net_networks[net_num].dir);
    ok1=0;
    for (counter=0;counter<12;counter++)
    {
        if ((cfg.aka_list[counter].net==fdmsg.DestNet) && (cfg.aka_list[counter].node==fdmsg.DestNode))
        {
            ok1=1;
		    net_num=cfg.aka_list[counter].wwiv_netnum;
            sprintf(wwiv_node,"%d",cfg.aka_list[counter].wwiv_node);
        }
    }
    if (!ok1)
        return(0);
    load_workspace();
    return(1);
}

/* ---------------------------- */
/* Help Function for WWIVTOSS   */
/* ---------------------------- */
void help(void)
{
    header();
    textcolor(LIGHTMAGENTA);
    printf("\r\n");
    cprintf("   Usage: WWIVTOSS /I /O /B /P /D \r\n");
    cprintf("             /I  - Toss Incoming Packet\r\n");
    cprintf("             /O  - Toss Outgoing Packet\r\n");
    cprintf("             /B  - Scan and Toss Bad Packets\r\n");
    cprintf("             /P  - Pack Outgoing Packets Only\r\n");
    cprintf("             /T  - Process Messages To LOCAL.NET\r\n");
    cprintf("             /D  - Verbose Scanning\r\n");
    cprintf("             /D1 - VERY Verbose Scanning (Displays messages)\r\n\n\n");
    cprintf("    /I, /O, and /D can be used together.\r\n");
    cprintf("    /B can only be used with the /D switch,\r\n");
}

/* ---------------------------- */
/*  Main Function For WWIVToss  */
/* ---------------------------- */

void main (int argc, char *argv[])
{
    int             i,ok,more_msgs,f1,scanbad,incoming,outgoing,length,j,pack;
    int             valid_command;
    unsigned char   ch,ch2;
    char            s[181],s1[81],log[81],tempyear[10];
    int             dotcount,dotlength,counter,ok2,success,skipped,retry,skip_local;
    double          total;
    long            this_pos,pkt_size;
    time_t          start,finish,t;
    struct          tm *systime;
    struct          date d;
    FSC39_header_struct *p39;
    FSC45_header_struct *p45;
    FSC48_header_struct *p48;

    /* Initialize Variables */
    start = time(NULL);
    t= time(NULL);
    systime=localtime(&start);
    getdate(&d);
    exit_status = 0;
    testing=0;
    count=0;
    debug=0;
    scanbad=0;
    pack=0;
    incoming=0;
    outgoing=0;
    show_seen_by=0;
    retry=0;
    pass_colors=0;
    pass_ascii=0;
    skip_local=1;
    pause=0;
    badd_1=badd_2=0;
    valid_command=0;
    testing_new=1;
    valid_mail=bad_mail=valid_post=bad_post=export_mail=export_post=passthru=gated_mail=dupe_msg=0;
 //   blaster = getenv("BLASTER");
    detect_multitask();


    /* If no arguments, print "help" */
    if (argc <= 1)
    {
        help();
        exit(1);
    }

    /* Determine switches and operating parameters */
    for (i=1; i<argc; i++) {
        strcpy(s,argv[i]);
	if ((s[0]=='-') || (s[0]=='/')) {
            ch=upcase(s[1]);
            ch2=upcase(s[2]);
            switch(ch) {
                case 'B':           // Scan Bad Messages
                    scanbad=1;
                    valid_command=1;
                    break;
                case 'D':           // Debug Information
                    debug=1;
                    if (ch2=='1')
                        debug=2;    // Verbose Debug Information
                    if (ch2=='2')
                        debug=3;    // Verbose Debug Information
                    break;
                case 'I':           // Process Incoming
                    incoming=1;
                    valid_command=1;
                    break;
                case 'P':           // Pack Outgoing
                    valid_command=1;
                    pack=1;
                    break;
                case 'O':           // Process Outgoing
                    valid_command=1;
                    outgoing=1;
                    break;
                case 'L':           // Skip Local Message Processing
                    skip_local=0;
                    break;
                case 'S':           // Pause After Message
                    pause=1;        // Not Implemented
                    break;
                case '?':           // Display Help
                    help();
                    exit(1);
                    break;
                case 'T':
                    printf("\7Now Processing Messages By Writing To LOCAL.NET!\r\n\7");
                    testing_new=0;
                    break;
            }
        }
    }
    if (!valid_command)
    {
        help();
        exit(1);
    }
    if ( (scanbad) && ( (incoming) || (outgoing) ) )
    {
        printf("\n\nþ /B switch cannot be used with /I or /O\n");
        printf("\n\n     WWIVTOSS will now only scan bad packets\n");
        incoming=0;
        outgoing=0;
    }

    tzset();
    read_config_dat();
    read_wwivtoss_config();
    load_xlat();
    for (i=0;i<MAX_NODES;i++)
    {
        track[i].wwiv_node=0;
        track[i].out_used=0;
        track[i].used=0;
        track[i].in_used=0;
        track[i].in_k=0;
        track[i].out_k=0;
        strcpy(track[i].net_name,"");
    }
    days_used = ((t-cfg.date_installed)/86400);
    if (days_used>1092)
        (cfg.date_installed=t-1);
    for (i=0;i<strlen(syscfg.systemname);i++)
    {
	if (syscfg.systemname[i]=='£')
	    syscfg.systemname[i]='u';
    }
    /* if (strcmp(cfg.bbs_name,syscfg.systemname)!=0)
        cfg.registered=0; MMH mod removed */
  /*  if (!cfg.registered)
    {
        cfg.pass_origin=0;
        header();
        printf("\r\n");  MMH Mod */
/* This section (1) gets uncommented in the release!    */
/* #ifndef BETA
        textcolor(BLINK+LIGHTRED);
        center(" Running Unregistered!  5 Second Delay!");
        for (i=0;i<38;i++)
            printf(" ");
        for (i=0;i<5;i++)
        {
            textcolor(i+10);
            cprintf("%d\7",i+1);
            sleep(1);
            printf("\b");
	}   MMH Mod */

/* End Section 1 */
/* #else   MMH Mod */
/* Don't forget to change the section down in void send_notification!!! */


/* This following section (2) gets removed in the release!  */
  /*      printf("Illegal Copy of WWIVTOSS!\r\n\7\7\7\7");
        printf("This is a BETA version, and is only to be used\r\n");
        printf("By WWIVTOSS BETA TEAM MEMBERS!\r\n\7\7\7");
        printf("If you are a Beta tester and get this message\r\n");
        printf("please contact Morgul IMMEDIATELY!\r\n");
        exit(1);   MMH Mod */
/* End Section 2 */
/* #endif 
    }  MMH Mod */
    read_nodes_config();
    read_uplink_config();
    /* if (!registered)
    {
        if ( (cfg.notify==0) || (cfg.notify==10) || (cfg.notify==20) || (cfg.notify==30) || (cfg.notify==40))
        {
            sprintf(s,"6Don't Forget To Register WWIVToss! (Run for %d Days!)",days_used);
            send_nag(s);
            if (days_used>45)
            {
                strcpy(s,"6You've run WWIVTOSS for over 45 days!  Don't you think you should register?\7\7\7\7");
                send_nag(s);
            }
        }
    }  MMH Mod */
    if (cfg.installed==0)
    {
        /* send_notification();   MMH Mod */
        cfg.installed=1;
        cfg.notify++;
        notify.date_notified=t;
    }
    if (cfg.notify==0)
    {
        /* send_notification(); MMH Mod */
        notify.date_notified=t;
    }
    Read_Hidden_Data_File();
    cfg.notify++;
    if (cfg.notify>50)
        cfg.notify=0;
    sprintf(tempyear,"%d",d.da_year);

    sprintf(now_time,"%-2.2d:%-2.2d:%-2.2d",systime->tm_hour,systime->tm_min,systime->tm_sec);
    sprintf(now_date,"%-2.2d/%-2.2d/%c%c",d.da_mon, d.da_day,tempyear[2],tempyear[3]);
    sprintf(s,"þ WWIVToss Processing Begun At %-2.2d:%-2.2d:%-2.2d on %-2.2d/%-2.2d/%c%c\n\r",systime->tm_hour,systime->tm_min,systime->tm_sec,d.da_mon,d.da_day,tempyear[2],tempyear[3]);
    write_log(s);
    header();
    if (cfg.registered)
    {
      textcolor(LIGHTGREEN);
        center("Registered!  Thank You!");
    }
    else
    {
        textcolor(BLINK+LIGHTRED);
        center("Running Unregistered!");
        printf("\7");
    }

    textcolor(WHITE);
    if (testing_new)
    {
        sub_f=-1;
        currsub=-1;
        read_status();
        sub_dates=(unsigned long *) mallocx(max_subs*sizeof(long),"sub_dates");
        read_subs();
        gat = (short *)farmalloc(2048 * sizeof(short));
        if (gat == NULL)
        {
            printf("Not enough memory.\n");
            exit(1);
        }
    }
    if (incoming)
    {
        pack=1;
        printf("\r\n");
        unarchive_packets();
        header();
        if (cfg.registered)
        {
            textcolor(LIGHTGREEN);
            center("Registered!  Thank You!");
        }
        else
        {
            textcolor(BLINK+LIGHTRED);
            center("Running Unregistered!");
        }
	textcolor(WHITE);
    }
    write_config();
    printf("\r\n");
    if ( (incoming) || (scanbad) )
    {
        /*  ------------------------------------------------
         *  look in the inbound directory for any .PKT files
         *  ------------------------------------------------  */
        strcpy(s,cfg.inbound_path);
        if (scanbad)
            strcat(s,"*.BAD");
        else
            strcat(s,"*.PKT");
        printf("þ Searching for incoming packets '%s'\n",s);
        f1=findfirst(s,&ff,0);
        ok=1;
        start_mem=coreleft();
        sprintf(log,"þ Starting memory = %ld\r\n",start_mem);
        write_log(log);

        if (debug)
            printf("%s",log);


        while ((f1==0) && (ok) && (exit_status==0))
        {
            strcpy(s,cfg.inbound_path);
            strcat(s,(ff.ff_name));
            printf("\r\nþ Opening PKT file '%s'  \n",s);
            sprintf(log,"þ Opening PKT file '%s'  \n",s);
            write_log(log);
            cur_mail=cur_post=0;
            incoming_file=-1;
            incoming_file=sh_open1(s,O_RDWR | O_BINARY);
            if (incoming_file<0)
            {
                printf("\7þ Error opening incoming file: %s. errno=%d\n",s,errno);
                sprintf(log,"þ Error opening incoming file: %s. errno=%d\n",s,errno);
                write_log(log);
                exit_status=6;
                break;
            }

            sh_read(incoming_file,(void *)&packet_header,sizeof(packet_header_struct));
            if (packet_header.packet_type!=2)
            {
                printf("þ Unknown Packet Type!\r\n");
            }
            p45 = (FSC45_header_struct *)&packet_header;
            p48 = (FSC48_header_struct *)&packet_header;
            p39 = (FSC39_header_struct *)&packet_header;

            if (packet_header.baud==2)
            {
                strcpy(pkt_type,"FSC-045 Type 2.2");
                packet_header.orig_node=p45->onode;
                packet_header.dest_node=p45->dnode;
                packet_header.orig_net=p45->onet;
                packet_header.dest_net=p45->dnet;
                strcpy(packet_header.Password,p45->password);
                packet_header.OrigZone=p45->ozone;
                packet_header.DestZone=p45->dzone;
                packet_header.OrigPoint=p45->opoint;
                packet_header.DestPoint=p45->dpoint;
                pktt_type=2;
            }
            else
            {
                if (p39->capword && p39->capword == (~p39->capword2))
                {
                    packet_header.orig_node=p39->orig_node;
                    packet_header.dest_node=p39->dest_node;
                    packet_header.orig_net=p39->orig_net;
                    packet_header.dest_net=p39->dest_net;
                    strcpy(packet_header.Password,p39->password);
                    packet_header.OrigZone=p39->orig_zone;
                    packet_header.DestZone=p39->dest_zone;
                    packet_header.OrigPoint=p39->orig_point;
                    packet_header.DestPoint=p39->dest_point;
                    strcpy(pkt_type,"FSC-0039 Type 2+");
                    pktt_type=1;
                }
                else
                if (p48->capword && p48->capword == p48->capword2)
                {
                    packet_header.orig_node=p48->orig_node;
                    packet_header.dest_node=p48->dest_node;
                    packet_header.orig_net=p48->orig_net;
                    packet_header.dest_net=p48->dest_net;
                    strcpy(packet_header.Password,p48->password);
                    packet_header.OrigZone=p48->orig_zone;
                    packet_header.DestZone=p48->dest_zone;
                    packet_header.OrigPoint=p48->orig_point;
                    packet_header.DestPoint=p48->dest_point;
                    strcpy(pkt_type,"FSC-0048 Type 2.N");
                    pktt_type=3;
                }
                else
                {
                    pktt_type=0;
                    strcpy(pkt_type,"Stone Age");
                }
            }
            printf("Packet Type is %s\r\n",pkt_type);
            if (packet_header.OrigZone==0)
                packet_header.OrigZone=packet_header.QMOrigZone;
            if (packet_header.DestZone==0)
                packet_header.DestZone=packet_header.QMDestZone;

            sprintf(temp_dest,"%d/%d",packet_header.dest_net,packet_header.dest_node);
            ok2=0;
            for(counter=0;counter<12;counter++)
            {
                if ((cfg.aka_list[counter].net==packet_header.dest_net) && (cfg.aka_list[counter].node==packet_header.dest_node))
                {
                    ok2=1;
                    net_num=cfg.aka_list[counter].wwiv_netnum;
                    sprintf(wwiv_node,"%d",cfg.aka_list[counter].wwiv_node);
                    track[counter].wwiv_node=cfg.aka_list[counter].wwiv_node;
                    track[counter].used=1;
                    track[counter].in_used=1;
                    pkt_size=filelength(incoming_file)/1000;
                    track[counter].in_k+=filelength(incoming_file)/1000;
                    strcpy(track[counter].net_name,net_networks[net_num].name);
                    counter=12;
                }
            }
            if (!ok2)
            {
                printf("\r\n\7þ Error: No Matching NETWORK entry in WWIVTOSS.DAT!\r\n");
                sprintf(log,"þ Error: No Matching NETWORK entry in WWIVTOSS.DAT!\r\n");
                write_log(log);
                exit_status=5;
            }

            if (exit_status==0)
            {
                strcpy(net_data,net_networks[net_num].dir);
                printf("þ Incoming Packet for %s from %d:%d/%d.%d (%ldk)\r\n",net_networks[net_num].name,packet_header.OrigZone,
                                                                packet_header.orig_net,
                                                                packet_header.orig_node,
                                                                packet_header.OrigPoint,
                                                                pkt_size);
                printf("þ Packet Type is %s\r\n",pkt_type);
                sprintf(log,"þ Incoming Packet for %s from %d:%d/%d.%d (%ldk)\n",net_networks[net_num].name,packet_header.OrigZone,
                                                                packet_header.orig_net,
                                                                packet_header.orig_node,
                                                                packet_header.OrigPoint,
                                                                pkt_size);
                write_log(log);
                sprintf(log,"þ Packet Type is %s\n",pkt_type);
                write_log(log);

                if (debug)
                {
                    printf("     orig_node=%d:%d/%d.%d\n",      packet_header.OrigZone,
                                                                packet_header.orig_net,
                                                                packet_header.orig_node,
                                                                packet_header.OrigPoint);
                    printf("     dest_node=%d:%d/%d.%d\n",      packet_header.DestZone,
                                                                packet_header.dest_net,
                                                                packet_header.dest_node,
                                                                packet_header.DestPoint);
                    printf("     packet date=%02d/%02d/%02d\n", packet_header.year,
                                                                packet_header.month+1,
                                                                packet_header.day);
                    printf("     packet time=%02d:%02d:%02d\n", packet_header.hour,
                                                                packet_header.minute,
                                                                packet_header.second);
                    printf("     baud=%d\n",                    packet_header.baud);
                    printf("     packet_type=%d\n",             packet_header.packet_type);
                    printf("     password=%s\n",                packet_header.Password);
                    printf("     Product Code (Low)  =%c\n",    packet_header.PCodeLo);
//                    printf("     Product Code (High) =%c\n",    packet_header.PCodeHi);

                }
                more_msgs=1;
                printf("     þ Processing Packet\r\n");
                dotcount=dotlength=0;
                while ((more_msgs) && (exit_status ==0))
                {
                    message_header.id = 0;                  /* init in case it's EOF */
                    this_pos=tell(incoming_file);
                    retry=0;
                    do
                    {
                        if (debug)
                            printf("Reading Message (Attempt # %d)\r\n",retry+1);
                        sh_read(incoming_file,(void *)&message_header.id,2);
                        if ((message_header.id==0x0002) || (message_header.id==0))
                            retry=3;
                        else
                        {
                            sprintf(log,"þ this_pos=%ld\n",this_pos);
                            write_log(log);
                            retry++;
                            sprintf(log,"þ Error: Message Header = %d!\n",message_header.id);
                            write_log(log);
                            if (retry<3)
                                sh_lseek(incoming_file,this_pos,SEEK_SET);
                        }

                    } while (retry<3);

                    if (!debug)
                    {
                        printf(".");
                        dotcount++;
                        if (dotcount==75)
                        {
                            printf("\r");
                            for (dotcount=0;dotcount<75;dotcount++)
                                printf(" ");
                            printf("\r");
                            dotcount=0;
                        }
                    }
                    if (message_header.id == 0x0002)
                    {
                        process_incoming_message();

                    } else
                    {
                        if (message_header.id == 0)
                        {
                            if (debug)
                                printf("\r\n<<< End of message packet >>>\n");
                            more_msgs=0;
                        } else
                        {
                            printf("invalid message header ID = %X\n",message_header.id);
                            exit_status=7;
                            break;
                        }
                    }
                }
                if (!debug)
                {
                    dotlength=dotcount;
                    printf("\r");
                    for (dotcount=0;dotcount<dotlength;dotcount++)
                        printf(" ");
                    printf("\r");
                    dotcount=0;
                }
            }
            sh_close(incoming_file);
            printf("\r");
            printf("     þ Processing Finished - %4d Mail - %4d Posts\r\n",cur_mail,cur_post);
            sprintf(log,"     þ Processing Finished - %4d Mail - %4d Posts\n\r",cur_mail,cur_post);
            write_log(log);
            if (outbound_file)
            {
                outchr(00);
                outchr(00);
                sh_close(outbound_file);
            }

            if (exit_status != 0)
            {
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
                {
                    printf("þ Error renaming file to .BAD, errno=%d",errno);
                    sprintf(log,"þ Error renaming file to .BAD, errno=%d",errno);
                    write_log(log);
                }
            } else
            {
                printf("þ Removing %s\r\n",s);
                unlink(s);
            }
            exit_status=0;
            count=0;
            f1=findnext(&ff);
        }
        write_log("=======================================================================\n");
        for (dotcount=0;dotcount<=num_areas;dotcount++)
        {
            read_area(dotcount,&thisarea);
            if (thisarea.count)
            {
                sprintf(log,"     Area [%-35s] Imported %-4d Messages\n",thisarea.name,thisarea.count);
                if (debug)
                    printf("%s\r",log);
                write_log(log);
                thisarea.count=0;
                write_area(dotcount,&thisarea);
            }
        }
        write_log("=======================================================================\r\n");
        printf("\r\n");
        strcpy(s,cfg.netmail_path);
        strcat(s,"*.MSG");
        printf("þ Searching for incoming netmail '%s'\n",s);
        sprintf(log,"þ Searching for incoming netmail '%s'\n",s);
        write_log(log);
        f1=findfirst(s,&ff,0);
        ok=1;
        if (cfg.skip_mail)
        {
            printf("þ Skip Mail toggle on.  Ignoring email\n");
            write_log("þ Skip Mail toggle on.  Ignoring email\n");
        }
        while ((f1==0) && (ok) && (exit_status==0) && (!cfg.skip_mail))
        {
            strcpy(s,cfg.netmail_path);
            strcat(s,(ff.ff_name));
            sprintf(s1,"þ Opening MSG file '%s'  ",s);
            length=strlen(s1);
            printf(s1);
            incoming_file=-1;
            incoming_file=sh_open1(s,O_RDWR | O_BINARY);
            if (incoming_file<0)
            {
                printf("\7þ Error opening incoming file: %s. errno=%d\n",s,errno);
                exit_status=6;
                break;
            }
            more_msgs=1;
            sh_read(incoming_file,&fdmsg,sizeof(fdmsg));
            if (debug)
            {
                printf("\r\n--------------------------\r\n");
                printf("¯ To    : %s (%d/%d)\n",    fdmsg.ToUser,
                                                    fdmsg.DestNet,
                                                    fdmsg.DestNode);
                printf("¯ From  : %s (%d/%d)\n",    fdmsg.FromUser,
                                                    fdmsg.OrigNet,
                                                    fdmsg.OrigNode);
                printf("¯ Title : %s\r\n",          fdmsg.Subject);
                printf("¯ Sent  : %s\r\n",          fdmsg.Date);
                printf("¯ Attr  : %X\r\n",          fdmsg.Attr);
                printf("        : ");
                if (fdmsg.Attr & MSGPRIVATE)
                    printf("PRIVATE ");
                if (fdmsg.Attr & MSGCRASH)
                    printf("CRASH ");
                if (fdmsg.Attr & MSGREAD)
                    printf("READ ");
                if (fdmsg.Attr & MSGSENT)
                    printf("SENT ");
                if (fdmsg.Attr & MSGFILE)
                    printf("FILE_ATTACH ");
                if (fdmsg.Attr & MSGTRANSIT)
                    printf("IN-TRANSIT ");
                if (fdmsg.Attr & MSGORPHAN)
                    printf("ORPHAN ");
                if (fdmsg.Attr & MSGKILL)
                    printf("KILL ");
                if (fdmsg.Attr & MSGLOCAL)
                    printf("LOCAL ");
                if (fdmsg.Attr & MSGHOLD)
                    printf("HOLD ");
                printf("\r\n");
                printf("--------------------------\r\n");
            }
            skipit=0;
            if ((fdmsg.Attr & MSGREAD) && (cfg.import_recd))
                skipit=0;
            if (cfg.skip_mail)
                skipit=1;
            if ((!strnicmp(fdmsg.ToUser,"AREAFIX",7)) && (cfg.use_areafix))
                skipit=0;
            if ((fdmsg.Attr & MSGLOCAL) && (!skip_local))
                skipit=1;

            if (!skipit)
            {
                process_incoming_netmail();
                if (!skipit)
                {
                    sh_lseek(incoming_file,0,SEEK_SET);
                    fdmsg.Attr |= MSGSENT;
                    fdmsg.Attr |= MSGREAD;
                    sh_write(incoming_file,(void *)&fdmsg,sizeof(FIDOMSG));
                    skipped=0;
                }
                else
                    skipped=1;
            }
            else
                skipped=1;
            if (skipped)
                if (debug)
                    printf("þ Skipping Message\r\n");
            sh_close(incoming_file);
            if ((fdmsg.Attr & MSGFILE) && !(fdmsg.Attr & MSGLOCAL))
                skipped=0;
            if ((fdmsg.Attr & MSGSENT) && (fdmsg.Attr & MSGLOCAL) && (cfg.delete_sent))
                skipped=0;
            if ((fdmsg.Attr & MSGSENT) && (fdmsg.Attr & MSGKILL))
                skipped=0;
            if ((fdmsg.Attr & MSGKILL) && !(fdmsg.Attr & MSGLOCAL))
                skipped=0;
            if (delete_me)
                skipped=0;
            if (!skipped)
            {
                if (!(fdmsg.Attr & MSGLOCAL))
                    unlink(s);
                if ((fdmsg.Attr & MSGSENT) && (cfg.delete_sent))
                    unlink(s);
                if ((fdmsg.Attr & MSGKILL))
                    unlink(s);
                if (delete_me)
                {
                    unlink(s);
                    delete_me=0;
                }
                skipped=0;
            }

            if (!debug)
            {
                for (j=0;j<length;j++)
                    printf("\b");
            } else
                printf("\r\n");

            skipit=0;
            f1=findnext(&ff);
        }
        if (debug)
            printf("--------------------------\r\n");
    }
    for (i=0;i<net_num_max;i++)
    {
        sprintf(s,"%sLOCAL.$$$",net_networks[i].dir);
        sprintf(s1,"%sLOCAL.NET",net_networks[i].dir);
        if (exist(s))
        {
            if (exist(s1))
                sprintf(s1,"%sP-WTOSS.NET",net_networks[i].dir);
            rename(s,s1);
            unlink(s);
        }
    }

    if (outgoing)
    {
        process_outgoing();
        pack=1;
    }
    if (scanbad)
    {
        strcpy(s,cfg.badecho_path);
        strcat(s,"*.MSG");
        printf("þ Re-Scanning Bad Messages '%s'\n",s);
        sprintf(log,"þ Re-Scanning Bad Messages '%s'\n",s);
        write_log(log);
        f1=findfirst(s,&ff,0);
        ok=1;
        while ((f1==0) && (ok) && (exit_status==0))
        {
            strcpy(s,cfg.badecho_path);
            strcat(s,(ff.ff_name));
            sprintf(s1,"þ Opening MSG file '%s'  ",s);
            length=strlen(s1);
            printf(s1);
            incoming_file=-1;
            incoming_file=sh_open1(s,O_RDWR | O_BINARY);
            if (incoming_file<0)
            {
                printf("\7þ Error opening incoming file: %s. errno=%d\n",s,errno);
                exit_status=6;
                break;
            }
            more_msgs=1;
            sh_read(incoming_file,&fdmsg,sizeof(fdmsg));
            if (debug)
            {
                printf("--------------------------\r\n");
                printf("¯ To    : %s (%d/%d)\n",    fdmsg.ToUser,
                                                    fdmsg.DestNet,
                                                    fdmsg.DestNode);
                printf("¯ From  : %s (%d/%d)\n",    fdmsg.FromUser,
                                                    fdmsg.OrigNet,
                                                    fdmsg.OrigNode);
                printf("¯ Title : %s\r\n",          fdmsg.Subject);
                printf("¯ Sent  : %s\r\n",          fdmsg.Date);
                printf("¯ Attr  : %X\r\n",          fdmsg.Attr);
            }

            success=process_bad_message();
            sh_close(incoming_file);
            if (success)
                unlink(s);
            if (!debug)
            {
                for (j=0;j<5;j++)
                    printf(" ");
                for (j=0;j<length+5;j++)
                    printf("\b");
            } else
                printf("\r\n");

            f1=findnext(&ff);
        }
        if (debug)
            printf("--------------------------\r\n");
    }
    if (pack)
    {
        printf("þ Now Archiving Outgoing Packets...           \r\n\r\n");
        pack_outbound_packets();
        printf("þ Archiving Packets Finished!\r\n");
    }
    pack_outbound_packets();
    printf("\r\n\r\n");
    if ((incoming) || (scanbad))
        printf("þ Imported Mail = %3d   Imported Posts = %4d\r\n",valid_mail,valid_post);
    if ((outgoing) || (export_mail))
        printf("þ Exported Mail = %3d   Exported Posts = %4d\r\n",export_mail,export_post);
    if (gated_mail)
        printf("þ Gated E-Mail  = %3d\r\n",gated_mail);
    if (passthru)
        printf("þ Passthru Posts= %3d\r\n",passthru);
    if ((bad_mail) || (bad_post))
        printf("þ Bad Mail      = %3d   Bad Posts      = %4d\r\n",bad_mail,bad_post);
    if (dupe_msg)
        printf("þ Dupe Messages = %3d\r\n",dupe_msg);
    sprintf(s,"þ Imported Mail = %3d   Imported Posts = %4d\n",valid_mail,valid_post);
    write_log("\r\n");
    write_log("=================================================================\r\n");
    write_log(s);
    sprintf(s,"þ Exported Mail = %3d   Exported Posts = %4d\n",export_mail,export_post);
    write_log(s);
    sprintf(s,"þ Gated E-Mail  = %3d   Passthru Posts = %4d\n",gated_mail,passthru);
    write_log(s);
    sprintf(s,"þ Bad Mail      = %3d   Bad Posts      = %4d\n",bad_mail,bad_post);
    write_log(s);
    sprintf(s,"þ Dupe Messages = %3d\n",dupe_msg);
    write_log(s);
    printf("\r\nþ WWIVToss %s Processing Complete\r\n\r\n",VERSION);
    write_log("=================================================================\r\n");
    save_status();
    // Yo!
    if ((bad_mail) || (bad_post))
        if (badd_1)
            send_nag("4WWIVTOSS: Check your Bad Echo directory.  Bad messages received");
        if (badd_2)
            send_nag("4WWIVTOSS: Check for CHECKME.NET in network directories!");
    finish=time(NULL);
    total = difftime(finish,start);
    write_net_log(total);
    printf("þ Elapsed Time - %s\r\n",ctim2(total));
    sprintf(s,"þ Elapsed Time - %s\r\n",ctim2(total));
    write_log(s);
    semaphore();
    if (cfg.cleanup)
    {
        printf("\r\nþ Now cleaning up networks\r\n");
        for(counter=0;counter<12;counter++)
        {
            sprintf(s,"%sLOCAL.NET",net_networks[cfg.aka_list[counter].wwiv_netnum].dir);
            if (exist(s))
            {
                sprintf(log,"Current Memory=%ld\n",coreleft());
                write_log(log);
                sprintf(s1,"þ Processing %s...",s);
                write_log(s1);
                sprintf(s," .%d",cfg.aka_list[counter].wwiv_netnum);
                spawnl(P_WAIT,"NETWORK2.EXE","NETWORK2.EXE",s,NULL);
                write_log("Done!\n");
            }
        }
    }
    cleanup_zero_byte_msgs();
    systime=localtime(&finish);
    getdate(&d);
    sprintf(tempyear,"%d",d.da_year);
    sprintf(now_date,"%-2.2d/%-2.2d/%c%c",d.da_mon, d.da_day,tempyear[2],tempyear[3]);
    sprintf(s,"þ WWIVToss Processing Ended At %-2.2d:%-2.2d:%-2.2d on %s\r\n\r\n",systime->tm_hour,systime->tm_min,systime->tm_sec,now_date);
    write_log(s);
    printf("\r\n%s\r\n",s);
/*
    sound(8*32.7);
    delay(125);
    sound(8*36.7);
    delay(125);
    sound(8*41.2);
    delay(125);
    sound(8*61.73);
    delay(300);
    sound(8*51.9);
    delay(120);
    sound(8*61.73);
    delay(500);
    nosound();
*/
    if (cfg.log_days)
        purge_log();
    if (testing_new)
    {
        free(gat);
    }
    exit (exit_status);
}

/*  --------------------------------------------------------------- */
/*  Read WWIV Data Files                                            */
/*  --------------------------------------------------------------- */

void read_config_dat(void)
{
    char s[80];
    int i,f;
    char *ss;


    f=sh_open1("CONFIG.DAT",O_RDONLY | O_BINARY);
    if (f<0)
    {
        printf("\r\n\r\n\7Error:  WWIVTOSS Must be run from the main WWIV directory!\r\n\r\n");
        exit(1);
    }
    sh_read(f,(void *) (&syscfg), sizeof(configrec));
    sh_close(f);

    if(net_networks)
        farfree(net_networks);
    net_networks=NULL;
    sprintf(s,"%sNETWORKS.DAT", syscfg.datadir);
    f=sh_open1(s,O_RDONLY|O_BINARY);
    if (f>0)
    {
        net_num_max=filelength(f)/sizeof(net_networks_rec);
        if (net_num_max)
        {
            net_networks=mallocx(net_num_max*sizeof(net_networks_rec),"networks.dat");
            sh_read(f, net_networks, net_num_max*sizeof(net_networks_rec));
        }
        else
        {
            printf("þ\7\7 Error!  No networks defined in INIT!  Cannot Continue!\r\n");
            write_log("þ Error!  No Networks Defined in INIT!  Cannot Continue!\n");
            sh_close(f);
            exit(1);
        }
        sh_close(f);
        filenet=notify_net=0;

        for (i=0; i<net_num_max; i++)
        {
            ss=strchr(net_networks[i].name, ' ');
            if (ss)
                *ss=0;
            if (stricmp(net_networks[i].name,"WWIVNET")==0)
            {
                filenet=1;
                notify_net=i;
                i=net_num_max;
                continue;
            }
            else
                if (((stricmp(net_networks[i].name,"ICENET")==0) ||
                     (stricmp(net_networks[i].name,"TERRANET")==0) ||
                     (stricmp(net_networks[i].name,"SIERRALINK")==0) ||
                     (stricmp(net_networks[i].name,"GLOBALNET")==0) ||
                     (stricmp(net_networks[i].name,"LABNET")==0) ||
                     (stricmp(net_networks[i].name,"SIERRALINK")==0)) && (!filenet))
                {
                    filenet=2;
                    notify_net=i;
                    i=net_num_max;
                    continue;
                }
            else
                if ((stricmp(net_networks[i].name,"WWIVLINK")==0) && (!filenet))
                {
                    filenet=3;
                    notify_net=i;
                    i=net_num_max;
                    continue;
                }
            else
                if ((stricmp(net_networks[i].name,"TFALINK")==0) && (!filenet))
                {
                    filenet=4;
                    notify_net=i;
                    i=net_num_max;
                    continue;
                }
            else
                if ((stricmp(net_networks[i].name,"DREAMNET")==0) && (!filenet))
                {
                    filenet=5;
                    notify_net=i;
                    i=net_num_max;
                    continue;
                }
            else
                if ((stricmp(net_networks[i].name,"FILENET")==0) && (!filenet))
                {
                    filenet=6;
                    notify_net=i;
                    i=net_num_max;
                    continue;
                }
        }
    }
    if (!net_networks)
    {
        net_networks=mallocx(sizeof(net_networks_rec), "networks.dat");
        net_num_max=1;
        strcpy(net_networks->name,"WWIVnet");
        strcpy(net_networks->dir, syscfg.datadir);
        net_networks->sysnum=syscfg.systemnumber;
    }

    ss=getenv("WWIV_INSTANCE");
    instance=atoi(ss);
    if (ss)
    {
        i=atoi(ss);
        if (i>0)
            sprintf(nete,".%3.3d",i);
        else
            strcpy(nete,".NET");
        instance=i;
    } else
    {
        strcpy(nete,".NET");
        instance=1;
    }
    sprintf(s,"%sSTATUS.DAT",syscfg.datadir);
    statusfile=sh_open1(s,O_RDONLY | O_BINARY);
    if (statusfile<0)
    {
        printf("þ Error opening STATUS.DAT!");
    }
    sh_read(statusfile,(void *)(&status), sizeof(statusrec));
    statusfile=sh_close(statusfile);
}

/* ---------------------------------------- */
/*  Memory Allocation For WWIV Structures   */
/* ---------------------------------------- */

void far *mallocx(unsigned long l, char *where)
{
    void *x;
    char huge *xx;

    if (!l)
        l=1;
    x=farmalloc(l);
    if (!x)
    {
        printf("Insufficient memory (%ld bytes) for %s.\n",l,where);
        exit(2);
    }

    xx=(char huge *)x;
    while (l>0)
    {
        if (l>32768)
        {
            memset((void *)xx,0,32768);
            l-=32768;
        } else
        {
            memset((void *)xx,0,l);
            break;
        }
    }

    return(x);
}

/* ------------------------------------- */
/*  Read User Information From USER.LST */
/* ------------------------------------- */

void read_user(unsigned int un, userrec *u)
{
    char s[81];
    long pos;
    int f, nu;

    sprintf(s,"%sUSER.LST",syscfg.datadir);

    f=sh_open1(s,O_RDONLY | O_BINARY);
    if (f<0)
    {
        printf("þ Error!  Can't open USER.LST!\r\n");
        write_log("Error!  CAn't open USER.LST");
        return;
    }

    nu=((int) (filelength(f)/syscfg.userreclen)-1);

    if (un>nu)
    {
        sh_close(f);
        return;
    }
    pos=((long) syscfg.userreclen) * ((long) un);
    sh_lseek(f,pos,SEEK_SET);
    sh_read(f, (void *)u, syscfg.userreclen);
    sh_close(f);
}

/* ---------------------------- */
/*  Get Number Of User Records  */
/* ---------------------------- */

int number_userrecs(void)
{
    char s[81];
    int f,i;

    sprintf(s,"%sUSER.LST",syscfg.datadir);
    f=sh_open1(s,O_RDONLY | O_BINARY);
    if (f>0)
    {
        i=((int) (filelength(f)/syscfg.userreclen)-1);
        f=sh_close(f);
    } else
        i=0;
    return(i);
}

/* -------------------------------- */
/*  Find Recipient Of Incoming Mail */
/* -------------------------------- */

void incoming_mail(void)
{
    int i,j;
    char s[81],log[81];
    user_to_num=0;
    j=number_userrecs();
    if (debug)
        printf("þ NetMail Detected!  Searching for %s\r\n",fido_to_user);
    user_to_num=skip_user(fido_to_user);
    if (user_to_num!=-1)
        return;
    if ((stricmp(fido_to_user,"SYSOP")==0))
    {
        user_to_num=1;
        return;
    }
    for (i=1;i<(j+1);i++)
    {
        read_user(i,&thisuser);
        if ( (stricmp(thisuser.realname,fido_to_user)==0) || (stricmp(thisuser.name,fido_to_user)==0))
        {
            if (thisuser.inact == inact_deleted)
            {
                if (debug)
                    printf("þ Match found, but user has been deleted! %s = User # %d (%s)\r\n",fido_to_user,i,thisuser.name);
                sprintf(log,"þ Match found, but user has been deleted! %s = User # %d (%s)\r\n",fido_to_user,i,thisuser.name);
                write_log(log);
                bad_mail++;
                cur_mail++;
                incoming_invalid=1;
                return;
            }
            if (debug)
                printf("þ Match Found : %s = User # %d (%s)\r\n",fido_to_user,i,thisuser.name);
            sprintf(log,"þ Match Found : %s = User # %d (%s)\r\n",fido_to_user,i,thisuser.name);
            write_log(log);
            user_to_num=i;
            if (!cfg.skip_mail)
                valid_mail++;
            else
                write_log("þ Skip Mail Option on - Skipping Mail\n");
            cur_mail++;
            return;
        }
    }
    if (debug)
        printf("þ Error!  No user %s found in USER.LST!\r\n",fido_to_user);
    sprintf(s,"þ Error!  No user %s found in USER.LST!\r\n",fido_to_user);
    write_log(s);
    bad_mail++;
    cur_mail++;
    incoming_invalid=2;
}

/* ---------------------------------------- */
/*  Get Info For Sender Of Outgoing Message */
/* ---------------------------------------- */

void outgoing_mail(int mode)
{

    read_user(fromuser,&thisuser);
    if (mode==1)
    {
        if (!thisarea.alias_ok)
            strcpy(fido_from_user,thisuser.realname);
        else
            strcpy(fido_from_user,thisuser.name);
    }
    if (mode==2)
        strcpy(fido_from_user,thisuser.realname);
}

void center(unsigned char *s)
/*
 * This function centers a string on a line, and takes into account the
 * embedded WWIV color codes (ASCII 003's).
 *
 */
{
    char s1[160];
    int i, len;

    len = strlen(s);
    i = (80-len)/2;
    strcpy(s1,"");
    while (i>0)
    {
        strcat(s1," ");
        i--;
    }
    strcat(s1,s);
    cprintf("%s\r\n",s1);

}
void process_incoming_netmail(void)
{
    int counter;
    int length,i;
    char log[80];
    user_to_num=0;
    net_num=-1;
    skipit=0;
    for (counter=0;counter<11;counter++)
    {
        if ((cfg.aka_list[counter].net==fdmsg.DestNet) && (cfg.aka_list[counter].node==fdmsg.DestNode))
        {
            net_num=cfg.aka_list[counter].wwiv_netnum;
            sprintf(wwiv_node,"%d",cfg.aka_list[counter].wwiv_node);
            my_zone=cfg.aka_list[counter].zone;
            my_net=cfg.aka_list[counter].net;
            my_node=cfg.aka_list[counter].node;
            my_point=cfg.aka_list[counter].point;
            counter=12;
        }
    }
    if (net_num>net_num_max)
    {
        printf("þ Error!  Network Number %d Not defined in INIT!\r\n",net_num);
        sprintf(log,"þ Error!  Network Number %d Not defined in INIT!\r\n",net_num);
        write_log(log);
        skipit=1;
        return;

    }

    if (net_num<0)
    {
        printf("þ Mail Not Destined for this node!\r\n");
        sprintf(log,"þ Mail Destined for %d/%d - Not this system.  Skipping.\n",fdmsg.DestNet,fdmsg.DestNode);
        write_log(log);
        skipit=1;
        return;
    }
    strcpy(net_data,net_networks[net_num].dir);
    strcpy(fido_to_user,fdmsg.ToUser);
    strcpy(fido_from_user,fdmsg.FromUser);
    strcpy(fido_title,fdmsg.Subject);
    sprintf(origi_node,"%d/%d",fdmsg.OrigNet,fdmsg.OrigNode);
    out_net=fdmsg.OrigNet;
    out_node=fdmsg.OrigNode;
    strcpy(fido_date_line,fdmsg.Date);
    length=strlen(fido_to_user);
    for (i=length;i>0;i--)
    if ((fido_to_user[i]==32) || (fido_to_user[i]==13) || (fido_to_user[i]==0))
    {
        fido_to_user[i]=0;
    }
    else
        i=0;
    if ((stricmp(fido_to_user,"AREAFIX")==0) || (stricmp(fido_to_user,"ALLFIX")==0))
    {
        msgtype=4;
        process_areafix_message();
        return;
    }

    if ((fido_to_user[0]==96) && (fido_to_user[1]==96))
    {
        msgtype=7;
        user_to_num=0;
        gated_mail++;
    } else
    {
        if (!strnicmp(fido_to_user,"WWIVTOSS GATE",13))
        {
             user_to_num=0;
             msgtype=7;
        }
        else
        {
            incoming_mail();
            msgtype=2;
            if (user_to_num==0)
            {
                skipit=1;
                msgtype=1;
            }
        }
    }
    sub_type=0;

    load_workspace();

}

void fidoadr_split(char *addr, FIDOADR *fadr)
{
    char *p;

    /*
     * Zone
     */
    p = strchr(addr, ':');
    if (p) fadr->zone = (word) atol(addr);
    else fadr->zone = 0;
    /*
     * Net
     */
    p = strchr(addr, '/');
    if (p) {
        p--;
        while (strchr("0123456789", *p) && (p >= addr)) p--;
        p++;
        fadr->net = (word) atol(p);
    }
    else fadr->net = 0;
    /*
     * Node
     */
    p = strchr(addr, '/');
    if (p) {
        p++;
        fadr->node = (word) atol(p);
    }
    else fadr->node = (word) atol(addr);
    /*
     * Point
     */
    p = strchr(addr, '.');
    if (p) {
        p++;
        fadr->point = (word) atol(p);
    }
    else fadr->point = 0;
    /*
     * Domain
     */
    p = strchr(addr, '@');
    if (p) {
        p++;
        strcpy(fadr->domain, p);
    }
    else *(fadr->domain) = '\0';
}

void unarchive_packets(void)
{
    char s[80],drive[10],dir[80],file[15],ext[5],temp[50];
    char cmd[161],t1[80],t2[80],t3[80],t4[50],log[81];
    unsigned char *firstchar;
    int f1,counter,compress,j,match,netnum,unsuccessful,infile,try;
    FIDOADR fadr;
    sprintf(s,"%s*.*",cfg.inbound_path);
    f1=findfirst(s,&ff,0);
    cfg.default_compression=5;
    try=2;
    while (f1==0)
    {
        fnsplit(ff.ff_name,drive,dir,file,ext);
        if ((strstr(ext,"MO")!=NULL) || (strstr(ext,"TU")!=NULL) || (strstr(ext,"WE")!=NULL) || (strstr(ext,"TH")!=NULL) || (strstr(ext,"FR")!=NULL) || (strstr(ext,"SA")!=NULL) || (strstr(ext,"SU")!=NULL))
        {
            match=0;
            for (counter=0;counter<MAX_NODES;counter++)
            {
                fidoadr_split(nodes[counter].address,&fadr);
                if (fadr.zone!=0)
                {
                    for (j=0;j<11;j++)
                    {
                        if (cfg.aka_list[j].zone!=0)
                        {
                            sprintf(temp,"%04X%04X",fadr.net-cfg.aka_list[j].net,fadr.node-cfg.aka_list[j].node);
                            if (stricmp(temp,file)==0)
                            {
                                match=1;
                                j=12;
                                compress=nodes[counter].compression;
                                netnum=counter;
                                counter=MAX_NODES;
                            }
                        }
                    }
                }
            }
            if (match)
            {
                printf("þ Match! %s%s is from %s!\r\n",file,ext,nodes[netnum].address);
                sprintf(log,"þ Match! %s%s is from %s!\n",file,ext,nodes[netnum].address);
                write_log(log);
            }
            else
            {
                printf("þ No Compression Listed For This Node.  Attempting to Determine Compression of %s%s\r\n",file,ext);
                sprintf(log,"þ No Compression Listed For This Node.  Attempting To Determine Compression of %s%s\n",file,ext);
                write_log(log);
                compress=cfg.default_compression;
                sprintf(t2,"%s%s",cfg.inbound_path,ff.ff_name);
                compress=0;
                firstchar = (char *) malloc(10);
                infile = sh_open1(t2,O_RDONLY|O_BINARY);
                if (infile>0)
                    sh_read(infile,firstchar,10);
                sh_close(infile);
                firstchar[0] = toupper(firstchar[0]);
                firstchar[1] = toupper(firstchar[1]);
                firstchar[2] = toupper(firstchar[2]);
                if ((firstchar[0] == 0x60) && (firstchar[1] == 0xEA))
                    compress=2; /* ARJ */
                else
                if ((firstchar[0] == 'P') && (firstchar[1] == 'K'))
                    compress=5; /* ZIP */
                else
                if ((firstchar[0] == 'Z') && (firstchar[1] == 'O') && (firstchar[2] == 'O'))
                    compress=6; /* ZOO */
                else
                if ((firstchar[0] == 0x1A) && ( (firstchar[1] ==0x0a) || (firstchar[1]==0x0b) ))
                    compress=4; /* PAK */
                else
                if ((firstchar[0] == 0x1A) && ( (firstchar[1] !=0x0a) && (firstchar[1]!=0x0b) ))
                {
                    compress=1; /* ARC */
                }
                else
                if ((firstchar[2] == '-') && (firstchar[3] == 'l') && (firstchar[4]=='h') && (firstchar[6]=='-'))
                    compress=3; /* LZH */
                else
                if ((firstchar[0] == 'R') && (firstchar[1] == 'A') && (firstchar[2] == 'R') && (firstchar[3] == '!'))
                    compress=8; /* RAR */
                if (!compress)
                {
                    printf("þ Could Not Determine Archive Type for %s%s- using default compression of %s\r\n",file,ext,cfg.archive[compress].name);
                    sprintf(log,"þ Could Not Determine Archive Type for %s%s - using default compression of %s\n",file,ext,cfg.archive[compress].name);
                    write_log(log);
                    compress=cfg.default_compression;
                }
            }
            do
            {
                sprintf(cmd,"%s ",cfg.archive[compress].unarchive_file);
                sprintf(t1,"%s ",cfg.archive[compress].unarchive_switches);
                if (cfg.archive[compress].unarchive_switches[0]==0)
                    strcpy(t1,"");
                sprintf(t2,"%s%s ",cfg.inbound_path,ff.ff_name);
                sprintf(t3,"%s",cfg.inbound_path);
                strcpy(t4,">NULL");
                unsuccessful=spawnlp(P_WAIT,cmd," ",t1,t2,t3,NULL);
                if ((unsuccessful) && (match))
                {
                    printf("þ Could Not Use Compression Listed For This Node.\r\n   Attempting to Determine Compression of %s%s\r\n",file,ext);
                    sprintf(log,"þ Could Not Use Compression Listed For This Node.\r\n  Attempting To Determine Compression of %s%s\n",file,ext);
                    write_log(log);
                    compress=cfg.default_compression;
                    sprintf(t2,"%s%s",cfg.inbound_path,ff.ff_name);
                    compress=0;
                    firstchar = (char *) malloc(10);
                    infile = sh_open1(t2,O_RDONLY|O_BINARY);
                    if (infile>0)
                        sh_read(infile,firstchar,10);
                    sh_close(infile);
                    firstchar[0] = toupper(firstchar[0]);
                    firstchar[1] = toupper(firstchar[1]);
                    firstchar[2] = toupper(firstchar[2]);
                    if ((firstchar[0] == 0x60) && (firstchar[1] == 0xEA))
                        compress=2;
                    else
                    if ((firstchar[0] == 'P') && (firstchar[1] == 'K'))
                        compress=5;
                    else
                    if ((firstchar[0] == 'Z') && (firstchar[1] == 'O') && (firstchar[2] == 'O'))
                        compress=6;
                    else
                    if ((firstchar[0] == 0x1A) && ( (firstchar[1] ==0x0a) || (firstchar[1]==0x0b) ))
                        compress=4;
                    else
                    if ((firstchar[0] == 0x1A) && ( (firstchar[1] !=0x0a) && (firstchar[1]!=0x0b) ))
                        compress=1;
                    else
                    if ((firstchar[2] == '-') && (firstchar[3] == 'l') && (firstchar[4]=='h') && (firstchar[6]=='-'))
                        compress=3;
                    else
                    if ((firstchar[0] == 'R') && (firstchar[1] == 'A') && (firstchar[2] == 'R') && (firstchar[3] == '!'))
                        compress=8;
                    if (!compress)
                    {
                        printf("þ Could Not Determine Archive Type for %s%s- using default compression of %s\r\n",file,ext,cfg.archive[compress].name);
                        sprintf(log,"þ Could Not Determine Archive Type for %s%s\r\n   - using default compression of %s\n",file,ext,cfg.archive[compress].name);
                        write_log(log);
                        compress=cfg.default_compression;
                    }
                    match=0;
                }
                try--;
                if (!unsuccessful)
                    try=0;
            } while (try>0);
            sprintf(temp,"%s%s",cfg.inbound_path,ff.ff_name);
            if (!unsuccessful)
                unlink(temp);
            else
            {
                printf("þ Error %d : Couldn't Unarchive %s.  Moving to %s...\r\n",unsuccessful,ff.ff_name,cfg.badecho_path);
                sprintf(log,"    þ Error %d : Couldn't Unarchive %s.  Moving to ...\n",unsuccessful,ff.ff_name,cfg.badecho_path);
                write_log(log);
                sprintf(cmd,"%s%s",cfg.badecho_path,ff.ff_name);
                rename(temp,cmd);
                unlink(temp);
                send_nag("4WWIVTOSS: Error!  Corrupt Packet Arrived!  Check your Bad Echo Directory!");
            }
        }
        f1=findnext(&ff);
        try=2;
    }
}

void toss_to_bad(message_header_struct message_header,char *message, int type)
{
    // type=0: bad message
    // type=1: write to thisarea.directory

    char fn[81],current_dir[161];
	fdmsg.FromUser[0]=0;

    getcwd(current_dir,60);
    strcpy(fdmsg.FromUser,fido_from_user);
    strcat(fdmsg.FromUser,"\0");
    strcpy(fdmsg.ToUser,fido_to_user);
    strcat(fdmsg.ToUser,"\0");
    strcpy(fdmsg.Subject,fido_title);

    strcpy(fdmsg.Date,fido_date_line);
    strcat(fdmsg.Date,"\0");
    fdmsg.TimesRead=0;
    fdmsg.DestNode=message_header.dest_node;
    fdmsg.OrigNode=message_header.orig_node;
    fdmsg.Cost=0;
    fdmsg.OrigNet=message_header.orig_net;
    fdmsg.DestNet=message_header.dest_net;
    fdmsg.DateWritten=0;
    fdmsg.DateArrived=0;
    fdmsg.ReplyTo=0;
    fdmsg.Attr = 259;  /* Crash and Local */
    fdmsg.ReplyNext=0;
    if (type==0)
        sprintf(fn,"%s%d.MSG",cfg.badecho_path,findnextmsg(2));
    else
    {
        sprintf(fn,"%s%d.MSG",thisarea.directory,findnextmsg(3));
        if (chdir(thisarea.directory))
        {
	    printf("þ ! Error!  Directory '%s' Does Not Exist!\r\n",thisarea.directory);
            sprintf(fn,"%s%d.MSG",cfg.badecho_path,findnextmsg(2));
        }
        chdir(current_dir);
    }
    outbound_file=-1;
    outbound_file=sh_open1(fn,O_RDWR | O_BINARY);
    if (outbound_file<0)
    {
    /*  --------------------------------------------------------------
     *  The file doesn't already exist - make one and write the header
     *  --------------------------------------------------------------  */
        outbound_file=sh_open(fn,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        if (outbound_file<0)
        {
            printf("error opening outbound file '%s'. errno=%d.1\n",outbound_packet_name,errno);
            exit_status=3;
            return;
        }
    }
    sh_write(outbound_file,(void *)&fdmsg,sizeof(FIDOMSG));
    sh_write(outbound_file,message,(strlen(message)+1));
    sh_close(outbound_file);
}

void write_log(char *string)
{
    FILE *fp;

    if (cfg.log_file[0]==0)
        return;

    if ((fp= fsh_open(cfg.log_file,"a+"))==NULL) {
        return;
    }
    fputs(string,fp);
    fsh_close(fp);

}

void header(void)
{
    char s[81];
    clrscr();
    printf("     [34mÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜÜ\r\n");
    printf("     [44m[1mÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ [37mWWIVTOSS WWIV/Fido Mail Tosser [34m[40m[44mÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ[0;30;44m¿[40m\r\n");
    printf("     [44m[1;34mÀ[0;30;44mÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ[0m[0m\r\n\r\n");
    textcolor(YELLOW);
    sprintf(s,"Written by: Craig Dooley");
    center(s);
    textcolor(YELLOW);
    sprintf(s,"Updated by: Mark Hofmann");
    center(s);
    textcolor(LIGHTCYAN);
    sprintf(s,"Version %s",VERSION);
    center(s);
    textcolor(WHITE);
    printf("\r\n");

}

void send_notification(void)
{
    char s[81],netfile[161],s1[81];
    int p0file;
    net_header_rec nh;
    struct date d;

    getdate(&d);
    if (!filenet)
        return;
    strcpy(net_data,net_networks[notify_net].dir);
    nh.touser=1;
    nh.main_type=15;
    nh.minor_type=0;
    time ((long *)&nh.daten);
    nh.method=139;
    nh.list_len=0;
    if (filenet==1)
        nh.tosys=6100;
    if (filenet==2)
        nh.tosys=8315;
    if (filenet==3)
        nh.tosys=18315;
    if (filenet==4)
        nh.tosys=8304;
    if (filenet==5)
        nh.tosys=1;
    if (filenet==6)
        nh.tosys=350;
    nh.fromuser=1;
    nh.fromsys=net_networks[notify_net].sysnum;
    if ((filenet==1) && (nh.fromsys==6100))
        return;
    if ((filenet==2) && (nh.fromsys==8315))
        return;
    if ((filenet==3) && (nh.fromsys==18315))
        return;
    if ((filenet==4) && (nh.fromsys==8304))
        return;
    if ((filenet==5) && (nh.fromsys==1))
        return;

    sprintf(s1,"%d",d.da_year);
    if (cfg.installed==0)
        sprintf(s,"6WWIVToss %s Installed by @%d.%s On %d/%d/%c%c",VERSION,nh.fromsys,net_networks[notify_net].name,d.da_mon,d.da_day,s1[2],s1[3]);
    else
        sprintf(s,"6WWIVToss %s In Use by @%d.%s (%d/%d/%c%c)",VERSION,nh.fromsys,net_networks[notify_net].name,d.da_mon,d.da_day,s1[2],s1[3]);
    if (!cfg.registered)
    {
        sprintf(s1," [%d Days!]",days_used);
        strcat(s,s1);
    }
    else
        strcat(s," Reg'd!");

    nh.length=sizeof(notify_rec);
    sprintf(netfile,"%sP0-WT.NET",net_data);
    p0file=sh_open(netfile,O_RDWR | O_BINARY | O_APPEND | O_CREAT, S_IREAD | S_IWRITE);
    sh_lseek(p0file,0L,SEEK_END);
    sh_write(p0file,(void *)&nh,sizeof(net_header_rec));
    sh_write(p0file,(void *)&notify,nh.length);
    sh_close(p0file);
    write_log("¯ Usage Notification Send to Author ®\n");
    if (!registered)
    {
        nh.fromuser=0;
        nh.tosys=net_networks[notify_net].sysnum;
        strcpy(s,"6Don't forget to register WWIVToss!!!!!!");
        send_nag(s);
    }
}

void send_nag(char *s)
{
    char netfile[161];
    int p0file;
    net_header_rec nh;
    struct date d;

    getdate(&d);
    strcpy(net_data,net_networks[0].dir);
    nh.touser=1;
    nh.main_type=15;
    nh.minor_type=0;
    time ((long *)&nh.daten);
    nh.method=0;
    nh.list_len=0;
    nh.fromuser=0;
    nh.tosys=nh.fromsys=net_networks[0].sysnum;
    nh.length=strlen(s);
    sprintf(netfile,"%sP0-WT.NET",net_data);
    p0file=sh_open(netfile,O_RDWR | O_BINARY | O_APPEND | O_CREAT, S_IREAD | S_IWRITE);
    sh_lseek(p0file,0L,SEEK_END);
    sh_write(p0file,(void *)&nh,sizeof(net_header_rec));
    sh_write(p0file,(void *)s,nh.length);
    sh_close(p0file);
}

void process_areafix_message(void)
{
    char *reply;
    char *reply_ptr;
    char *temp1;
    char temp[50],s[181],address[50],t1[80],t2[80],s3[81];
    unsigned char echo_name[81],command[81],log[181];
    int i,counter,cycle,match,current_node,match1,there,done,ok_to_do,echo_count,done1;
    int tzone,tnet,tnode,tpoint,hit,hit1,nogo,ignore;
    int help,list,query,unlinked,comp,pass,connect_all,disconnect_all;
    unsigned char ch;
    long msgid;
    struct date dt;
    struct time tm;
    char *month[] = {"Bug","Jan","Feb","Mar","Apr","May","Jun",
                     "Jul","Aug","Sep","Oct","Nov","Dec"};
    FIDOADR fadr;
    FILE *fp;

    if (!cfg.use_areafix)
        return;

    getdate(&dt);
    gettime(&tm);
    sprintf(s3,"%d",dt.da_year);
    sprintf(fido_date_line,"%02d %s %c%c  %02d:%02d:%02d",
        dt.da_day,month[dt.da_mon],s3[2],s3[3],
        tm.ti_hour,tm.ti_min,tm.ti_sec+(tm.ti_hund+50)/100);

    help=list=query=unlinked=comp=pass=connect_all=disconnect_all=hit=0;
    ok_to_do=1;
    ignore=0;

    if ((reply = (char *)farmalloc(10000)) == NULL)
    {
        printf("þ Allocation error for areafix reply text\n");
        exit_status = 8;
        farfree((void *)reply);
        return;
    }
    temp1=malloc(8*1024);
    msgid=time(NULL);
    reply_ptr=reply;

    sprintf(reply_ptr,"MSGID: %d:%d/%d %lX\r",my_zone,my_net,my_node,msgid++);
    reply_ptr=strchr(reply_ptr,0);
    sprintf(reply_ptr,"PID: WWIV BBS v. %u \r",status.wwiv_version);
    reply_ptr=strchr(reply_ptr,0);
    sprintf(reply_ptr,"TID: WWIVTOSS v. %s \r",VERSION);
    reply_ptr=strchr(reply_ptr,0);

    sprintf(reply_ptr,"**** AreaFix Report for %d:%d/%d.%d ****\015\012",my_zone,out_net,out_node,out_point);
    reply_ptr=strchr(reply_ptr,0);
    sprintf(log,"\nþ Areafix Request from %d/%d\n",out_net,out_node);
    write_log(log);
    printf("%s\r",log);
    sprintf(temp,"%d/%d",out_net,out_node);

    match=match1=0;
    for (i=1;i<MAX_NODES;i++)
    {
        fidoadr_split(nodes[i].address,&fadr);
        if ((fadr.net==out_net) && (fadr.node==out_node))
        {
            strcpy(address,nodes[i].address);
            match1=1;
            tzone=fadr.zone;
            tnet=fadr.net;
            tnode=fadr.node;
            tpoint=fadr.point;
            if (stricmp(nodes[i].areafix_pw,fido_title)==0)
            {
                match=1;
                current_node=i;
                i=MAX_NODES;
            }
        }
    }
    /* No Matching Node */
    if (!match1)
    {
        printf("þ Areafix Request for %d/%d Failed!  No Node Defined!\r\n",out_net,out_node);
        sprintf(log,"þ Areafix Request for %d/%d Failed!  No Node Defined!\r\n",out_net,out_node);
        write_log(log);
        sprintf(reply_ptr,"Areafix Request Failed!\015\012");
        reply_ptr = strchr(reply_ptr,0);
        sprintf(reply_ptr,"Your node was not found in my database.  Please contact %s immediately!\015\012",cfg.sysop_name);
        reply_ptr = strchr(reply_ptr,0);
        ok_to_do=0;
    }
    /* Node found, password incorrect */
    if (!match)
    {
        printf("þ Areafix Request for %d/%d Failed!  Invalid Password (%s)!\r\n",out_net,out_node,fido_title);
        sprintf(log,"þ Areafix Request for %d/%d Failed!  Invalid Password (%s)!\r\n",out_net,out_node,fido_title);
        write_log(log);
        sprintf(reply_ptr,"Invalid Password (%s)!  Areafix Request Refused!\015\012",fido_title);
        reply_ptr = strchr(reply_ptr,0);
        ok_to_do=0;
    }
//    if (!nodes[current_node].allow_areafix)
//    {
//        printf("þ Node Not allowed to use Areafix!\r\n");
//        write_log("Node Not Allowed to use Areafix!\n");
//        sprintf(reply_ptr,"Sorry, You are Not Allowed to use the Areafix Capabilities of this system!\015\012");
//        reply_ptr = strchr(reply_ptr,0);
//        ok_to_do=0;
//    }
    strcpy(temp1,"");
    strcpy(command,"");
    done=0;
    while (!done)
    {
        strcpy(command,"");
        do
        {
            sh_read(incoming_file,(void *)&ch,1);
            if (ch==0)
            {
                done=1;
                break;
            }
            if ((ch!=13) && (ch!=10))
            {
                sprintf(temp1,"%c",ch);
                strcat(command,temp1);
            }
        } while ((ch!=13)); //13
        ch=0;
        if (ignore)
            command[0]=0;
        if (debug)
            if (command[0]!=1)
            {
                printf("%s\r\n",command);
            }

        if (command[0]=='%')
        {
            sprintf(reply_ptr,"*********\015\012");
            reply_ptr = strchr(reply_ptr,0);
            sprintf(reply_ptr,"þ Command = '%s'\015\012",command);
            reply_ptr=strchr(reply_ptr,0);
            strcpy(&command[0],&command[1]);
            if (!strnicmp(command,"HELP",4))
                help=1;
            if (!strnicmp(command,"LIST",4))
                list=1;
            if (!strnicmp(command,"QUERY",4))
                query=1;
            if (!strnicmp(command,"UNLINKED",8))
                unlinked=1;
            if (!strnicmp(command,"COMPRESSION",11))
                comp=1;
            if (!strnicmp(command,"PASSWORD",8))
                pass=1;
            if (!strnicmp(command,"+ALL",4))
                connect_all=1;
            if (!strnicmp(command,"-ALL",4))
                disconnect_all=1;
            if (!ok_to_do)
                query=unlinked=comp=pass=connect_all=disconnect_all=list=0;
            if (pass)
            {
                sscanf(command,"%s %s",&t1,&t2);
                sprintf(log, "Changed Password from '%s' to '%s'\n",nodes[current_node].areafix_pw,t2);
                write_log(log);
                sprintf(reply_ptr, "    þ Changed Password from '%s' to '%s'\n",nodes[current_node].areafix_pw,t2);
                reply_ptr = strchr(reply_ptr,0);
                strcpy(nodes[current_node].areafix_pw,t2);
                write_nodes_config();
                pass=0;
            }
            if (comp)
            {
                sscanf(command,"%s %s",&t1,&t2);
                done1=0;
                for (i=0;i<=NUM_ARCHIVERS;i++)
                {
                    if (stricmp(cfg.archive[i].name,t2)==0)
                    {
                        nodes[current_node].compression=i;
                        done1=1;
                    }

                }
                if (done1)
                {
                    sprintf(log, "Changed Compression to '%s'\n",cfg.archive[nodes[current_node].compression].name);
                    write_log(log);
                    sprintf(reply_ptr, "    þ Changed Compression to '%s'\015\012",cfg.archive[nodes[current_node].compression].name);
                    reply_ptr = strchr(reply_ptr,0);
                    write_nodes_config();
                    comp=0;
                }
                else
                {
                    sprintf(log,"Unknown Compression Type! (%s)\n",t2);
                    write_log(log);
                    sprintf(reply_ptr,"I'm sorry, I don't recognize compression type '%s'\015\012",t2);
                    reply_ptr = strchr(reply_ptr,0);
                    sprintf(reply_ptr,"The Valid Compression Types are : \015\012");
                    reply_ptr = strchr(reply_ptr,0);
                    for (i=0;i<=NUM_ARCHIVERS;i++)
                    {
                        sprintf(reply_ptr,"          %s\015\012",cfg.archive[i].name);
                        reply_ptr = strchr(reply_ptr,0);
                    }
                }
                comp=0;
            }

            if (help)
            {
                sprintf(log,"    þ Requested Help File.\n");
                write_log(log);
                if ((fp = fsh_open("AREAFIX.HLP","r"))==NULL)
                {
                    write_log("    þ Error!  Can't Open AREAFIX.HLP!\n");
                    send_nag("Error Opening AREAFIX.HLP!");
                    printf("þ Error Reading AREAFIX.HLP!\r\n");
                    sprintf(reply_ptr," Sorry - An error occured reading the help file.  Instructions Are not Availible at this time.\015\012");
                    reply_ptr=strchr(reply_ptr,0);
                }
                else
                {
                    write_log("    þ Sent Help file 'AREAFIX.HLP'\n");
                    while (!feof(fp))
                    {
                        if (fgets(s,80,fp)!=NULL)
                        {
                            sprintf(reply_ptr,"%s",s);
                            reply_ptr = strchr(reply_ptr,0);
                        }
                    }
                    fsh_close(fp);
                }
                help=0;
            }
            if (list)
            {
                echo_count=0;
                sprintf(log,"    þ Requested List of Areas Availible\n");
                write_log(log);
                sprintf(reply_ptr,"List of Areas Availible To You\015\012");
                reply_ptr = strchr(reply_ptr,0);
                sprintf(reply_ptr,"==============================\015\012");
                reply_ptr = strchr(reply_ptr,0);
                for (i=1;i<=num_areas;i++)
                {
                    read_area(i,&thisarea);
                    if (nodes[current_node].groups & thisarea.group)
                    {
                        sprintf(reply_ptr,"%s\015\012",thisarea.name);
                        reply_ptr = strchr(reply_ptr,0);
                        echo_count++;
                    }
                }
                if (echo_count)
                    sprintf(reply_ptr,"Total %d Areas\015\012",echo_count);
                else
                    sprintf(reply_ptr,"    þ No Areas Area Availible To You.\015\012");
                reply_ptr = strchr(reply_ptr,0);
                list=0;
            }
            if (query)
            {
                echo_count=0;
                sprintf(log,"    þ Requested List of Areas Currently Connected\n");
                write_log(log);
                sprintf(reply_ptr,"List of Areas that You Are Currently Connected To\015\012");
                reply_ptr = strchr(reply_ptr,0);
                sprintf(reply_ptr,"=================================================\015\012");
                reply_ptr = strchr(reply_ptr,0);
                for (i=1;i<=num_areas;i++)
                {
                    read_area(i,&thisarea);
                    if (exist_export_node(address,i))
                    {
                            sprintf(reply_ptr,"%s\015\012",thisarea.name);
                            reply_ptr = strchr(reply_ptr,0);
                            echo_count++;
                    }
                }
                if (echo_count)
                    sprintf(reply_ptr,"Total %d Areas\015\012",echo_count);
                else
                    sprintf(reply_ptr,"    þ You are not connected to any areas.\015\012");
                reply_ptr = strchr(reply_ptr,0);
                query=0;
            }
            if (unlinked)
            {
                echo_count=0;
                sprintf(log,"    þ Requested List of Areas Not Currently Connected\n");
                write_log(log);
                sprintf(reply_ptr,"List of Areas that You Are Currently NOT Connected To\015\012");
                reply_ptr = strchr(reply_ptr,0);
                sprintf(reply_ptr,"=====================================================\015\012");
                reply_ptr = strchr(reply_ptr,0);
                for (i=1;i<=num_areas;i++)
                {
                    read_area(i,&thisarea);
                    if (!(exist_export_node(address,i)))
                    {
                        if (nodes[current_node].groups & thisarea.group)
                        {
                            sprintf(reply_ptr,"%s\015\012",thisarea.name);
                            reply_ptr = strchr(reply_ptr,0);
                            echo_count++;
                        }
                    }
                }
                sprintf(reply_ptr,"Total %d Areas\015\012",echo_count);
                reply_ptr = strchr(reply_ptr,0);
                unlinked=0;

            }
            if (connect_all)
            {
                echo_count=0;
                sprintf(log,"    þ Requested To Connect to ALL Availible Areas\n");
                write_log(log);
                sprintf(reply_ptr,"You Have Been Connected To The Following Areas\015\012");
                reply_ptr = strchr(reply_ptr,0);
                sprintf(reply_ptr,"==============================================\015\012");
                reply_ptr = strchr(reply_ptr,0);
                for (i=1;i<=num_areas;i++)
                {
                    read_area(i,&thisarea);
                    done1=0;
                    if (!(exist_export_node(address,i)))
                    {
                        if (nodes[current_node].groups & thisarea.group)
                        {
                            for (counter=0;counter<MAX_NODES;counter++)
                            {
                                if (((thisarea.feed[counter].zone==999) ||
                                    (thisarea.feed[counter].zone==0)) && (!done1))
                                    {
                                        thisarea.feed[counter].zone=tzone;
                                        thisarea.feed[counter].net=tnet;
                                        thisarea.feed[counter].node=tnode;
                                        thisarea.feed[counter].point=tpoint;
                                        done1=1;
                                        sprintf(s,"þ Added %d:%d/%d.%d To %s\n",thisarea.feed[counter].zone,thisarea.feed[counter].net,thisarea.feed[counter].node,thisarea.feed[counter].point,thisarea.name);
                                        counter=MAX_NODES;
                                        write_log(s);
                                        if (debug)
                                            printf("%s\r",s);
                                        sprintf(reply_ptr,"    þ You have been added to %s\015\012",thisarea.name);
                                        reply_ptr = strchr(reply_ptr,0);
                                        sort_exports();
                                        echo_count++;
                                    }
                            }
                        }
                    }
                write_area(i,&thisarea);
                }
                if (echo_count)
                    sprintf(reply_ptr,"Total %d Areas\015\012",echo_count);
                else
                    sprintf(reply_ptr,"    þ No Availible echos for you to connect to.\015\012");
                reply_ptr = strchr(reply_ptr,0);
                connect_all=0;
            }
            if (disconnect_all)
            {
                echo_count=0;
                sprintf(log,"þ Requested To Disconnect From ALL Areas\n");
                write_log(log);
                sprintf(reply_ptr,"Disconnected from these areas:\015\012");
                reply_ptr = strchr(reply_ptr,0);
                sprintf(reply_ptr,"==============================\015\012");
                reply_ptr = strchr(reply_ptr,0);
                for (i=1;i<=num_areas;i++)
                {
                    read_area(i,&thisarea);
                    if (exist_export_node(address,i))
                    {
                        for (counter=0;counter<MAX_NODES;counter++)
                        {
                            if ((thisarea.feed[counter].zone==tzone) &&
                                (thisarea.feed[counter].net==tnet)   &&
                                (thisarea.feed[counter].node==tnode) &&
                                (thisarea.feed[counter].point==tpoint))
                                {
                                    thisarea.feed[counter].zone=999;
                                    thisarea.feed[counter].net=0;
                                    thisarea.feed[counter].node=0;
                                    thisarea.feed[counter].point=0;
                                    sprintf(s,"þ Removed %d:%d/%d.%d from %s\n", tzone,tnet,tnode,tpoint,thisarea.name);
                                    write_log(s);
                                    counter=12;
                                    sort_exports();
                                    sprintf(reply_ptr,
                                                "    þ Area: %s - deleted %d/%d from scan\015\012"
						,thisarea.name
                                                ,out_net
                                                ,out_node);
                                    reply_ptr = strchr(reply_ptr,0);
                                    echo_count++;
                            }
                        }
                    }
                write_area(i,&thisarea);
                }
                if (echo_count)
                    sprintf(reply_ptr,"Total %d Areas\015\012",echo_count);
                else
                    sprintf(reply_ptr,"    þ You Are Not Connected To Any Echos.\015\012");
                reply_ptr = strchr(reply_ptr,0);
                disconnect_all=0;

            }
        sprintf(reply_ptr,"*********\015\012");
        reply_ptr = strchr(reply_ptr,0);
        } else
        {
            /*  ----------------------------------------------
             *  see if it's an add request or a delete request
             *  ----------------------------------------------  */
            if (!ok_to_do)
                break;
            if (command[0]=='-')
            {
                if (command[1] == '-')                 /*  look for the tear line  */
                {
                    ignore=1;
                    continue;
//                    break;
                }
                sprintf(reply_ptr,"*********\015\012");
                reply_ptr = strchr(reply_ptr,0);
                sprintf(reply_ptr,"þ Command = '%s'\015\012",command);
                reply_ptr=strchr(reply_ptr,0);

                strcpy(&command[0],&command[1]);
                /*  -------------------------------------------------
                 *  it's a delete request.   Get the name of the echo
                 *  -------------------------------------------------  */
                strcpy(echo_name,"");
                strcpy(echo_name, command);
                sprintf(log,"Delete Request for Echo =%s\r\n",echo_name);
                if (debug)
                    printf(log);
                write_log(log);
                hit=0;
                hit1=0;
                for (cycle=1;cycle<num_areas+1;cycle++)
                {
                    read_area(cycle,&thisarea);
                    if (!stricmp(thisarea.name,echo_name))
                    {
                        hit1=1;
                        curr_area=cycle;
                        cycle=num_areas;
                        there=0;
                        for (counter=0;counter<MAX_NODES;counter++)
                        {
                            if ((thisarea.feed[counter].zone==tzone) &&
                                (thisarea.feed[counter].net==tnet)   &&
                                (thisarea.feed[counter].node==tnode) &&
                                (thisarea.feed[counter].point==tpoint))
                                {
                                    thisarea.feed[counter].zone=999;
                                    thisarea.feed[counter].net=0;
                                    thisarea.feed[counter].node=0;
                                    thisarea.feed[counter].point=0;
                                    sprintf(s,"þ Removed %d:%d/%d.%d from %s\n", tzone,tnet,tnode,tpoint,echo_name);
                                    write_log(s);
                                    sort_exports();
                                    there=1;
                                    hit=1;
                                    sprintf(reply_ptr,
                                                "    þ Area: %s - deleted %d/%d from scan\015\012"
                                                ,thisarea.name
                                                ,out_net
                                                ,out_node);
                                    reply_ptr = strchr(reply_ptr,0);

                            }
                        }
                        write_area(curr_area,&thisarea);
//                        write_areas_config();
                    }
                }
                sort_exports();
                if (!hit1)
                {
                    sprintf(log,"Could not remove - Area %s Not listed on this system!\n",echo_name);
                    write_log(log);
                    sprintf(reply_ptr,"    þ Could not remove - Area %s Not listed on this system!\015\012",echo_name);
                    reply_ptr = strchr(reply_ptr,0);
                }
                else
                if (!hit)
                {
                    sprintf(log,"Could not remove from %s - node not listed in the export list!\n",echo_name);
                    write_log(log);
                    sprintf(reply_ptr,"    þ Could not remove from %s - you're not listed in the export list!\015\012",echo_name);
                    reply_ptr = strchr(reply_ptr,0);
                }
                sprintf(reply_ptr,"*********\015\012");
                reply_ptr = strchr(reply_ptr,0);
            }
            else
            if (command[0]== '+')
            {
                    sprintf(reply_ptr,"*********\015\012");
                    reply_ptr = strchr(reply_ptr,0);
                    sprintf(reply_ptr,"þ Command = '%s'\015\012",command);
                    reply_ptr=strchr(reply_ptr,0);

                /*  -------------------------------------------------
                 *  it's an add request.   Get the name of the echo
                 *  -------------------------------------------------  */
                strcpy(&command[0],&command[1]);
                strcpy(echo_name,"");
                strcpy(echo_name, command);
                sprintf(log,"Add Request for Echo %s\n",command);
                write_log(log);
                if (debug)
                    printf("%s\r",log);
                there=0;
                done1=0;
                hit=0;
                hit1=0;
                for (cycle=1;cycle<num_areas+1;cycle++)
                {
                    read_area(cycle,&thisarea);
                    if (!stricmp(thisarea.name,echo_name))
                    {
                        hit=1;
                        curr_area=cycle;
                        cycle=num_areas;
                        if (nodes[current_node].groups & thisarea.group)
                        {
                            for (counter=0;counter<MAX_NODES;counter++)
                            {
                                if ((thisarea.feed[counter].zone==tzone) &&
                                    (thisarea.feed[counter].net==tnet)   &&
                                    (thisarea.feed[counter].node==tnode) &&
                                    (thisarea.feed[counter].point==tpoint))
                                    {
                                        there=1;
                                    }
                            }
                            if (!there)
                            {
                                for (counter=0;counter<MAX_NODES;counter++)
                                {
                                    if (((thisarea.feed[counter].zone==999) ||
                                        (thisarea.feed[counter].zone==0)) && (!done1))
                                        {
                                            thisarea.feed[counter].zone=tzone;
                                            thisarea.feed[counter].net=tnet;
                                            thisarea.feed[counter].node=tnode;
                                            thisarea.feed[counter].point=tpoint;
                                            done1=1;
                                            sprintf(s,"    þ Added %d:%d/%d.%d To %s\n",thisarea.feed[counter].zone,thisarea.feed[counter].net,thisarea.feed[counter].node,thisarea.feed[counter].point,echo_name);
                                            counter=MAX_NODES;
                                            write_log(s);
                                            if (debug)
                                                printf("%s\r",s);
                                            sprintf(reply_ptr,"You have been added to %s\015\012",echo_name);
                                            reply_ptr = strchr(reply_ptr,0);
                                            sort_exports();
                                        }
                                }
                            }
                            else
                            {
                                sprintf(log,"Couldn't add to %s - Node already subscribes!\n",echo_name);
                                write_log(log);
                                if (debug)
                                    printf("%s\r",log);
                                sprintf(reply_ptr,"    þ Could not add to %s - you're already subscribed!\015\012",echo_name);
                                reply_ptr = strchr(reply_ptr,0);
                            }
                        }
                        else
                        {
                            sprintf(log,"Couldn't Add you to %s - That Echo is Not Availible To You!\n",echo_name);
                            write_log(log);
                            sprintf(reply_ptr,"    þ Couldn't Add you to %s - That Echo is Not Availible To You!\015\012",echo_name);
                            reply_ptr=strchr(reply_ptr,0);
                        }
                        write_area(curr_area,&thisarea);
                    }
                }
                if (!hit)
                {
                    nogo=1;
                    if (uplinkable)
                    {
                        nogo=check_uplink(1,echo_name);   // check for uplinks
                        if (!nogo)
                            nogo=check_uplink(2,echo_name);   // Check for Zone
                        if (!nogo)
                            nogo=check_uplink(3,echo_name); // Check for areafile
                        if (!nogo)
                            nogo=check_uplink(4,echo_name); // check for Listed
                        if (!nogo)
                        {
                            sprintf(t2,"%d:%d/%d.%d",tzone,tnet,tnode,tpoint);
                            uplink_request(echo_name,t2);
                        }
                        if (nogo)
                        {
                            sprintf(log,"Couldn't Add you to %s - That Echo is Not Availible From Here!\n",echo_name);
                            write_log(log);
                            sprintf(reply_ptr,"    þ Couldn't Add you to %s - That Echo is Not Availible From Here!\015\012",echo_name);
                            reply_ptr=strchr(reply_ptr,0);
                        }
                        else
                        {
                            sprintf(log,"Sent Request for %s to %s!\n",echo_name,uplink[current_uplink].address);
                            write_log(log);
                            sprintf(reply_ptr,"    þ Echo %s Not Carried Here.  Add Request Passed On To My Uplink\015\012",echo_name);
                            reply_ptr=strchr(reply_ptr,0);
                        }
                    } else
                    {
                        sprintf(log,"Could not add - Area %s Not listed on this system!\n",echo_name);
                        write_log(log);
                        sprintf(reply_ptr,"    þ Could not add - Area %s Not listed on this system!\015\012",echo_name);
                        reply_ptr = strchr(reply_ptr,0);
                    }
                }
                sprintf(reply_ptr,"*********\015\012");
                reply_ptr = strchr(reply_ptr,0);
            }
            else
            {
                if ((command[0]!=1) && (command[0]!=0))
                {
                    sprintf(reply_ptr,"*********\015\012");
                    reply_ptr = strchr(reply_ptr,0);
                    sprintf(reply_ptr,"þ Command = '%s'\015\012",command);
                    reply_ptr=strchr(reply_ptr,0);
                    sprintf(reply_ptr,"    þ Sorry, I don't recognize '%s' as a valid command!\015\012",command);
                    reply_ptr = strchr(reply_ptr,0);
                    sprintf(reply_ptr,"*********\015\012");
                    reply_ptr = strchr(reply_ptr,0);
                    sprintf(log,"Unknown Command '%s'!\n",command);
                    write_log(log);
                }
            }
        }
    }

    /* sprintf(reply_ptr,"\r--- WWIVToss v.%s %segistered\r",VERSION,registered?"R":"Unr"); */
    sprintf(reply_ptr,"\r--- WWIVToss v.%s\r",VERSION); /* MMH mod */
    reply_ptr = strchr(reply_ptr,0);
    sprintf(reply_ptr," * Origin:  %s (%d:%d/%d.%d)\r",cfg.origin_line,my_zone,my_net,my_node,my_point);
    reply_ptr = strchr(reply_ptr,0);

    strcpy(fido_to_user,fido_from_user);
    strcpy(fido_from_user,"WWIVToss AreaFix");
    strcpy(fido_title,"* Areafix Report *");
    send_message(2,reply);
    export_mail++;
    free(temp1);
    free(reply);
    free(reply_ptr);
//    this_pos=tell(incoming_file);
}

char *ctim2(double d)
{

    static char ch[30],ch1[30];
    long da,h,m,s;

    if (d==0)
        return("0");
    if (d<0)
        d += 24.0*3600.0;

    da=(long) (d/86400.0);
    d-=(double) (da*86400);
    h=(long) (d/3600.0);
    d-=(double) (h*3600);
    m=(long) (d/60.0);
    d-=(double) (m*60);
    s=(long) (d);

    strcpy(ch,"");
    if (da!=0)
    {
        sprintf(ch1,"%ld days",da);
        strcat(ch,ch1);
    }

    if (h!=0)
    {
        if (da!=0)
            strcat(ch,", ");
        sprintf(ch1,"%ld hrs",h);
        strcat(ch,ch1);
    }
    if (m!=0)
    {
        if ((da!=0) || (h!=0))
            strcat(ch,", ");
        sprintf(ch1,"%ld mins",m);
        strcat(ch,ch1);
    }

    if ((s!=0) && (da==0))
    {
        if ((h!=0) || (m!=0))
            strcat(ch,", ");
        sprintf(ch1,"%ld secs.",s);
        strcat(ch,ch1);
    }

    return(ch);
}
char *ctim3(double d)
{

    static char ch[30],ch1[30];
    long da,h,m,s;

    if (d==0)
        return("0.1");
    if (d<0)
        d += 24.0*3600.0;

    da=(long) (d/86400.0);
    d-=(double) (da*86400);
    h=(long) (d/3600.0);
    d-=(double) (h*3600);
    m=(long) (d/60.0);
    d-=(double) (m*60);
    s=(long) (d);

    strcpy(ch,"");
    if (!m)
        strcat(ch1,"0.");
    else
        sprintf(ch1,"%ld.",m);
    strcat(ch,ch1);
    da=s/60;
    if (!da)
        da=1;
    sprintf(ch1,"%.1ld",da);
    strcat(ch,ch1);
    return(ch);
}

void bounce_mail(void)
{
    char *mess,s[80],re[80],to[80],date[80];
    int i,cycle;
    long msgid;
    FIDOADR fadr;

    msgid=time(NULL);

    sprintf(s,"4WWIVTOSS: Bad Email for %s from %s - Bounced!",fido_to_user, fido_from_user);
    send_nag(s);
    sprintf(re,"RE: %s\r",fido_title);
    sprintf(to,"TO: %s\r",fido_to_user);
    sprintf(date,"ON: %s\r",fido_date_line);

    strcpy(fido_title,"Returned Email");
    strcpy(fido_to_user,fido_from_user);
    sprintf(fido_from_user,"WWIVToss v%s",VERSION);
    mess=malloc(32*1024);

    strcpy(mess,"");
    sprintf(s,"MSGID: %d:%d/%d %lX\r",my_zone,my_net,my_node,msgid++);
    strcat(mess,s);
    sprintf(s,"PID: WWIV BBS v. %u \r",status.wwiv_version);
    strcat(mess,s);
    sprintf(s,"TID: WWIVTOSS v. %s \r",VERSION);
    strcat(mess,s);
    if (cfg.direct)
    {
        sprintf(s,"FLAGS DIR\r");
        strcat(mess,s);
    }

    if (out_point)
    {
        sprintf(s,"TOPT %d\r",out_point);
        strcat(mess,s);
    }
    if (my_point)
    {
        sprintf(s,"FMPT %d\r",my_point);
        strcat(mess,s);
    }
    if (!out_zone)
        out_zone=my_zone;

    if (out_zone!=my_zone)
    {
     /* MMH mod */ /*  sprintf(s,"INTL %d:%d/%d.%d %d:%d/%d.%d\r",out_zone,out_net,out_node,out_point,my_zone,my_net,my_node,my_point);  */
	    sprintf(s,"INTL %d:%d/%d %d:%d/%d\r",out_zone,out_net,out_node,my_zone,my_net,my_node); 
        strcat(mess,s);
    }
    sprintf(s,"  * * * WWIVToss v. %s Returned Email * * *\r",VERSION);
    strcat(mess,s);
    strcat(mess,"   This is to let you know that a piece of mail that you sent to this\r");
    strcat(mess,"   system did not arrive at it's destination.  The reason for this bounce\r");
    strcat(mess,"   is : ");
    if (incoming_invalid==1)
        strcat(mess,"That user is no longer a user of this system.\r");
    if (incoming_invalid==2)
        strcat(mess,"There is no such user by that name on this BBS.\r");
    if (incoming_invalid==3)
        strcat(mess,"Your node is not allowed to Areafix this system.\r");
    strcat(mess,"   Please check your addressing to make sure you did not make a\r");
    strcat(mess,"   typographical error.  If you feel that you have received this message\r");
    strcat(mess,"   in error, please contact ");
    sprintf(s,"%s, SysOp of %d:%d/%d.%d\r",cfg.sysop_name,my_zone,my_net,my_node,my_point);
    strcat(mess,s);
    strcat(mess,"\r\r          Returned email follows below\r");
    strcat(mess,"          ===================================\r");
    strcat(mess,re);
    strcat(mess,to);
    strcat(mess,date);
    strcat(mess,"===================================\r\r");
    for (i=0;i<strlen(buffer);i++)
    {
        if (buffer[i]==4)
        {
            buffer[i]=1;
            sprintf(s,"%c",buffer[i]);
            strcat(mess,s);
            i++;
        }
        else
        {
            if (buffer[i]!=10)
            {
                sprintf(s,"%c",buffer[i]);
                strcat(mess,s);
            }
        }
    }
    strcat(mess,"\r\r");
  /*  sprintf(s,"\r--- WWIVToss v.%s %segistered\r",VERSION,registered?"R":"Unr");  */
    sprintf(s,"\r--- WWIVToss v.%s\r",VERSION);  /* MMH mod */
    strcat(mess,s);
    sprintf(s," * Origin:  %s (%d:%d/%d.%d)\r",cfg.origin_line,my_zone,my_net,my_node,my_point);
    strcat(mess,s);

    fidoadr_split(origi_node,&fadr);
    out_zone=fadr.zone;
    out_net=fadr.net;
    out_node=fadr.node;
    out_point=fadr.point;
    if (!out_zone)
        out_zone=my_zone;
    if (my_net==out_net)
    {
        who_route.zone=who_to.zone=out_zone;
        who_route.net=who_to.net=out_net;
        who_route.node=who_to.node=out_node;
        who_route.point=who_to.point=out_point;
        send_message(2,mess);
    }
    else
    {
        for (cycle=0;cycle<MAX_NODES;cycle++)
        {
            if (out_zone==cfg.route_to[cycle].route_zone)
                break;
        }
        who_route.zone=who_to.zone=cfg.route_to[cycle].zone;
        who_route.net=who_to.net=cfg.route_to[cycle].net;
        who_route.node=who_to.node=cfg.route_to[cycle].node;
        who_route.point=who_to.point=cfg.route_to[cycle].point;
        attribute=0;
        attribute=attribute | (MSGLOCAL) | (MSGPRIVATE);
        send_message(3,mess);
    }
    free(mess);
    outchr(00);
    outchr(00);
    sh_close(outbound_file);
    incoming_invalid=0;
}

void sort_exports(void)
{
    int t_zone,t_net,t_node,t_point;
    int i,j,k;

    for (k=0;k<MAX_NODES;k++)
    {
        for (i=0;i<MAX_NODES;i++)
        {
            if ((thisarea.feed[i].zone==999) || (thisarea.feed[i].net==0))
            {
                for (j=i;j<MAX_NODES-1;j++)
                {
                    thisarea.feed[j].zone=thisarea.feed[j+1].zone;
                    thisarea.feed[j].net=thisarea.feed[j+1].net;
                    thisarea.feed[j].node=thisarea.feed[j+1].node;
                    thisarea.feed[j].point=thisarea.feed[j+1].point;
                }
            }
        }
    }
    for (k=0;k<MAX_NODES;k++)
        if (thisarea.feed[k].zone==0)
            thisarea.feed[k].zone=999;
    for (k=0;k<MAX_NODES;k++)
    {
        for (j=0;j<MAX_NODES;j++)
        {
            if (thisarea.feed[k].zone<thisarea.feed[j].zone)
            {
                t_zone=thisarea.feed[j].zone;
                t_net=thisarea.feed[j].net;
                t_node=thisarea.feed[j].node;
                t_point=thisarea.feed[j].point;
                thisarea.feed[j].zone=thisarea.feed[k].zone;
                thisarea.feed[j].net=thisarea.feed[k].net;
                thisarea.feed[j].node=thisarea.feed[k].node;
                thisarea.feed[j].point=thisarea.feed[k].point;
                thisarea.feed[k].zone=t_zone;
                thisarea.feed[k].net=t_net;
                thisarea.feed[k].node=t_node;
                thisarea.feed[k].point=t_point;
            }
            if (thisarea.feed[k].zone==thisarea.feed[j].zone)
            {
                if (thisarea.feed[k].net<thisarea.feed[j].net)
                {
                    t_zone=thisarea.feed[j].zone;
                    t_net=thisarea.feed[j].net;
                    t_node=thisarea.feed[j].node;
                    t_point=thisarea.feed[j].point;
                    thisarea.feed[j].zone=thisarea.feed[k].zone;
                    thisarea.feed[j].net=thisarea.feed[k].net;
                    thisarea.feed[j].node=thisarea.feed[k].node;
                    thisarea.feed[j].point=thisarea.feed[k].point;
                    thisarea.feed[k].zone=t_zone;
                    thisarea.feed[k].net=t_net;
                    thisarea.feed[k].node=t_node;
                    thisarea.feed[k].point=t_point;
                }
                if (thisarea.feed[k].net==thisarea.feed[j].net)
                {
                    if (thisarea.feed[k].node<thisarea.feed[j].node)
                    {
                        t_zone=thisarea.feed[j].zone;
                        t_net=thisarea.feed[j].net;
                        t_node=thisarea.feed[j].node;
                        t_point=thisarea.feed[j].point;
                        thisarea.feed[j].zone=thisarea.feed[k].zone;
                        thisarea.feed[j].net=thisarea.feed[k].net;
                        thisarea.feed[j].node=thisarea.feed[k].node;
                        thisarea.feed[j].point=thisarea.feed[k].point;
                        thisarea.feed[k].zone=t_zone;
                        thisarea.feed[k].net=t_net;
                        thisarea.feed[k].node=t_node;
                        thisarea.feed[k].point=t_point;
                    }
                    if (thisarea.feed[k].node==thisarea.feed[j].node)
                        {
                        if (thisarea.feed[k].point<thisarea.feed[j].point)
                        {
                            t_zone=thisarea.feed[j].zone;
                            t_net=thisarea.feed[j].net;
                            t_node=thisarea.feed[j].node;
                            t_point=thisarea.feed[j].point;
                            thisarea.feed[j].zone=thisarea.feed[k].zone;
                            thisarea.feed[j].net=thisarea.feed[k].net;
                            thisarea.feed[j].node=thisarea.feed[k].node;
                            thisarea.feed[j].point=thisarea.feed[k].point;
                            thisarea.feed[k].zone=t_zone;
                            thisarea.feed[k].net=t_net;
                            thisarea.feed[k].node=t_node;
                            thisarea.feed[k].point=t_point;
                        }
                    }
                }
            }
        }
    }
}

void semaphore(void)
{
    char *ptr;
    char s[80],env[80],file1[80],file2[80],line[80];
    time_t timer;
    struct tm *tblock;
    struct ftime filet;
    FILE *fp;

    timer=time(NULL);
    tblock=localtime(&timer);

    filet.ft_hour=tblock->tm_hour;
    filet.ft_tsec=tblock->tm_sec/2;
    filet.ft_min=tblock->tm_min;
    filet.ft_day=tblock->tm_mday;
    filet.ft_month=tblock->tm_mon+1;
    filet.ft_year=tblock->tm_year-80;


    switch (cfg.mailer)
    {
        case 0:
            strcpy(env,"FD");
            strcpy(file1,"\\FDRESCAN.NOW");
            strcpy(file2,"\\FMRESCAN.NOW");
            break;
        case 1:
            strcpy(env,"IM");
            strcpy(file1,"\\IMRESCAN.NOW");
            strcpy(file2,"\\IERESCAN.NOW");
            break;
        case 2:
            strcpy(env,"BINKLEY");
            return;
//	    break;
	case 3:
	    strcpy(env,"PORTAL");
	    return;
//            break;
        case 4:
            strcpy(env,"DBRIDGE");
            strcpy(file1,"\\DBRIDGE.RSN");
            strcpy(file2,"\\DBRIDGE.NSW");
            /* MMH Mod - next two lines */
            fp=fsh_open("\\DB\\DBRIDGE.RSN","w");
            fprintf(fp,"Testing...\n");
            fsh_close(fp);
            break;
    }
  /*  ptr=getenv(env);
    if (ptr==NULL)
	return;

    strcpy(s,ptr);
    sprintf(line,"%s%s",s,file1);
    if ((fp = fsh_open(line,"w"))==NULL)
	return;
    setftime(fileno(fp),&filet);
    fsh_close(fp);
    sprintf(line,"%s%s",s,file2);

    if ((fp = fsh_open(line,"w"))==NULL)
	return;
    setftime(fileno(fp),&filet);
    fsh_close(fp);   MMH mod removed */


}

int skip_user (char *user_name)
{
    char s[80],name[80];
    int number,i;

    FILE *fp;

    if (!exist("SKIPMAIL.LST"))
        return (-1);

    if ((fp = fsh_open("SKIPMAIL.LST","r+")) == NULL)
    {
        return (-1);
    }

    do
    {
        fgets(s,79,fp);
        sscanf(s,"%s %d",&name,&number);
        for (i=0;i<strlen(name);i++)
            if (name[i]=='_')
                name[i]=32;
        if (stricmp(user_name,name)==0)
        {
            if (number==0)
            {
                sprintf(s,"þ %s Set to 0 in SKIPMAIL.LST.  Skipping Mail...\n",user_name);
                bad_mail++;

            }
            else
            {
                sprintf(s,"þ %s Set to %d in SKIPMAIL.LST.  Redirecting Mail...\n",user_name,number);
                valid_mail++;
            }
            write_log(s);
            fsh_close(fp);
            return(number);
	}
    } while (!feof(fp));
    fsh_close(fp);
    return (-1);

}
int exist_export_node(char *address,int area)
{
    int j;
    FIDOADR fadr;

    fidoadr_split(address, &fadr);
    read_area(area,&thisarea);
    for (j=0;j<12;j++)
    {
        if ((thisarea.feed[j].zone==fadr.zone) &&
            (thisarea.feed[j].net==fadr.net) &&
            (thisarea.feed[j].node==fadr.node) &&
            (thisarea.feed[j].point==fadr.point))
            {
                return (1);
            }
    }
    return (0);
}
void purge_log(void)
{
    char s[80],str[81];
    char temp[80],temp1[80];
    int m1,m2,d1,d2,y1,y2,d3;
    int keep,length;
    FILE *fp,*fq;
    struct date d;

    getdate(&d);

    if (cfg.log_file[0]==0)
        return;

    strcpy(s,"WT-TEMP.LOG");
    if (exist(s))
        unlink(s);

    if ((fp= fsh_open(cfg.log_file,"r+"))==NULL)
    {
        fsh_close(fp);
        return;
    }
    if ((fq=fsh_open(s,"a+"))==NULL)
    {
        fsh_close(fp);
        return;
    }
    keep=0;
    while ((!(fgets(str,81,fp)==NULL)))
    {
        if (strstr(str,"Processing Begun At")!=NULL)
        {
            keep=0;
            length=strlen(str);
            strcpy(temp,&str[length-9]);
            length=strlen(temp);
            temp[length-1]=0;
            sprintf(temp1,"%c%c",temp[0],temp[1]);
            m2=atoi(temp1);
            sprintf(temp1,"%c%c",temp[3],temp[4]);
            d2=atoi(temp1);
            sprintf(temp1,"%c%c",temp[6],temp[7]);
            y2=atoi(temp1);
            d1=d.da_day;
            m1=d.da_mon;
            sprintf(temp,"%d",d.da_year);
            sprintf(temp1,"%c%c",temp[2],temp[3]);
            y1=atoi(temp1);
            if (y1<90)
                y1+=100;

            if (y1>y2)
            {
                m1=12;
                d3=month[12]+d1;
                d1=d3;
            }

            if (m1==m2)
            {
                if ((d1-cfg.log_days)<=d2)
                {
                    keep=1;
                }
            }
            if (m1<m2)
            {
                keep=1;
            }
            if (m1>m2)
            {
                if ((d1-cfg.log_days)>=1)
                    keep=1;
                else
                {
                    if ((m1-m2)>1)
                        keep=0;
                    else
                    {
                        d3=d1+month[m1];
                        if (d3-cfg.log_days<=d2)
                            keep=1;
                    }
                }
            }
        }
        if (keep)
        {
            fputs(str,fq);
        }
    }

    fsh_close(fp);
    fsh_close(fq);
    unlink(cfg.log_file);
    rename(s,cfg.log_file);
    unlink(s);
}


unsigned char *filedate(unsigned char *fpath)
{
    struct ftime d;
    static unsigned char s[10];
    int i;

    if (!exist(fpath))
        return("");

    i=open(fpath,O_RDONLY,S_IREAD | S_IWRITE);
    if (i==-1)
        return("");
    getftime(i,&d);
    sprintf(s,"%02d/%02d/%02d",d.ft_month,d.ft_day,d.ft_year+80);
    close(i);
    return(s);
}

void cleanup_zero_byte_msgs(void)
{
    char s[80],s1[80],fdate[80],tdate[20],s2[10];
    int f1;
    int f;
    struct date d;

    getdate(&d);

    sprintf(s,"%d",d.da_year);
    sprintf(s2,"%c%c",s[2],s[3]);
    sprintf(s,"%s*.*",cfg.outbound_path);
    f1=findfirst(s,&ff,0);

    while (f1==0)
    {
        sprintf(s1,"%s%s",cfg.outbound_path,ff.ff_name);
        f=sh_open1(s1,O_RDONLY | O_BINARY);
        if (filelength(f)==0)
        {
            close(f);
            strcpy(fdate,filedate(s1));
            sprintf(tdate,"%-2.2d/%2.2d/%s",d.da_mon,d.da_day,s2);
            if (stricmp(fdate,tdate)!=0)
                unlink(s1);
	}
        else
            close(f);
        f1=findnext(&ff);
    }
}
void write_net_log(double total)
{
    char s[80],tmp1[81],str[80],k[80];
    FILE *fp,*fs;
    int i;
    for (i=0;i<12;i++)
    {
        if (track[i].used)
        {
            sprintf(s,"%sNET.LOG",syscfg.gfilesdir);
            sprintf(tmp1,"%sNET.$$$",syscfg.gfilesdir);
            if ((fp = fopen(tmp1,"w+"))==NULL)
            {
		break;
            }
        if ((fs = fopen(s,"a+"))==NULL)
	    {
            fclose(fp);
            break;
	    }
        if (track[i].out_used)
            strcpy(k,"To");
        else
            strcpy(k,"Fr");
        sprintf(str,"%s %s %s %5d, ",now_date,now_time,k,track[i].wwiv_node);
        if (track[i].out_used)
	{
            if (track[i].out_k<1)
                track[i].out_k=1;
            sprintf(k,"S:%4dk, ",track[i].out_k);
        }
        else
            strcpy(k,"       , ");
	    strcat(str,k);
        if (track[i].in_used)
        {
            if (track[i].in_k<1)
                track[i].in_k=1;
            sprintf(k,"R:%4dk, ",track[i].in_k);
	}
	else
	    strcpy(k,"       , ");

	    strcat(str,k);
	    strcat(str,"   0 cps, ");
        sprintf(k,"%s min  %s\n",ctim3(total),track[i].net_name);
	    strcat(str,k);
	    fputs(str,fp);
	    while ((!(fgets(str,81,fs)==NULL)))
	    {
            fputs(str,fp);
	    }
	    fclose(fs);
        fclose(fp);
        unlink(s);
        rename(tmp1,s);
        unlink(tmp1);
        }
    }
}
int twit_me(char *username, char *fido_node)
{
    char s[80],name[80],node[80];
    int i;

    FILE *fp;

    if (!exist("TWIT.LST"))
        return (0);

    if ((fp = fsh_open("TWIT.LST","r+")) == NULL)
    {
        return (0);
    }

    do
    {
        fgets(s,79,fp);
        sscanf(s,"%s %s",&name,&node);
        for (i=0;i<strlen(name);i++)
            if (name[i]=='_')
                name[i]=32;
        if ((stricmp(username,name)==0) && (stricmp(fido_node,node)==0))
        {
            sprintf(s,"þ %s (%s)found in TWIT.LST - Skipping Message...\n",username,fido_node);
            write_log(s);
            fsh_close(fp);
            return(1);
        }
    } while (!feof(fp));
    fsh_close(fp);
    return (0);

}

void Read_Hidden_Data_File(void)
{
    char s[80],s1[181];
    int configfile,notify_now;
    time_t t;

    t=time(NULL);

    sprintf(s,"%sWWIVTOSS.HID",syscfg.datadir);
    notify_now=0;
    if (!(exist(s)))
        notify_now=1;
    configfile=sh_open1(s,O_RDWR | O_BINARY);
    if (configfile<0)
    {
        configfile=sh_open1(s,O_RDWR|O_CREAT|O_TRUNC|O_BINARY);
        if (configfile<0)
            return;
    }
    sh_read(configfile,(void *) (&notify), sizeof (notify_rec));
    strcpy(notify.bbs_name,cfg.bbs_name);
    strcpy(notify.sysop_name,cfg.sysop_name);
    strcpy(notify.version,VERSION);
    strcpy(notify.system_address,cfg.system_address);
    notify.date_installed=cfg.date_installed;
    notify.registered=cfg.registered;
    strcpy(notify.network,net_networks[notify_net].name);
    notify.node=net_networks[notify_net].sysnum;
    if ((((t-notify.date_notified)/86400)>10) || (notify_now))
    {
        notify.date_notified=t;
        /* send_notification(); MMH Mod */
    }
    sh_close(configfile);
    configfile=sh_open1(s,O_RDWR|O_CREAT|O_TRUNC|O_BINARY);
    sh_write(configfile,(void *) (&notify), sizeof (notify_rec));
    sh_close(configfile);
    sprintf(s1,"ATTRIB +H %s",s);
    system(s1);

}
int check_uplink(int mode, char *echo_name)
{
    int i;
    char temp[81], temp1[81];
    FIDOADR fadr;
    FILE *fp;
    switch (mode)
    {
        case 1:
            if (uplinkable)
                if ((uplink[1].address[0]==0) || (uplink[1].address[0]==NULL))
                {
                    return (1);
                }
                else
                {
                    return (0);
                }
            else
            {
                return (1);
            }
//            break;
        case 2:
            for (i=0;i<num_uplinks;i++)
            {
                strcpy(temp,uplink[i].address);
                fidoadr_split(temp, &fadr);
                if (fadr.zone==my_zone)
                {
                    current_uplink=i;
                    i=num_uplinks;
                    return(0);
                }
            }
            break;
        case 3:
            if (uplink[current_uplink].unconditional)
                return(0);
            strcpy(temp,uplink[current_uplink].areas_filename);
            if (exist(temp))
            {
                return(0);
            }
            break;
        case 4:
            if (uplink[current_uplink].unconditional)
                return(0);
            strcpy(temp,uplink[current_uplink].areas_filename);
            fp=fsh_open(temp,"r");
            while (!feof(fp))
            {
                fgets(temp1,81,fp);
                if (striinc(echo_name,temp1)!=NULL)
                {
                    fsh_close(fp);
                    return(0);
                }
            }
            fsh_close(fp);
            break;
    }
    return (1);
}
void uplink_request(char *echo_name, char *address)
{
	char s[81];
    int i;
    char *reply2;
    char *reply_ptr2;
    char *temp2;
    long msgid2;
    int temp_net,temp_node;
    FIDOADR fadr,tadr;


    if ((reply2 = (char *)farmalloc(10000)) == NULL)
    {
        printf("þ Allocation error for areafix reply text\n");
        exit_status = 8;
        farfree((void *)reply2);
        return;
    }
    temp2=malloc(8*1024);
    msgid2=time(NULL);
    reply_ptr2=reply2;

    sprintf(reply_ptr2,"MSGID: %d:%d/%d %lX\r",my_zone,my_net,my_node,msgid2++);
    reply_ptr2=strchr(reply_ptr2,0);
    sprintf(reply_ptr2,"PID: WWIV BBS v. %u \r",status.wwiv_version);
    reply_ptr2=strchr(reply_ptr2,0);
    sprintf(reply_ptr2,"TID: WWIVTOSS v. %s \r",VERSION);
    reply_ptr2=strchr(reply_ptr2,0);
    if (uplink[current_uplink].add_plus)
        sprintf(reply_ptr2,"+%s\015\012",echo_name);
    else
        sprintf(reply_ptr2,"%s\015\012",echo_name);
    reply_ptr2=strchr(reply_ptr2,0);
    fidoadr_split(uplink[current_uplink].address,&fadr);
    fidoadr_split(address,&tadr);
    /* sprintf(reply_ptr2,"\r--- WWIVToss v.%s %segistered\r",VERSION,registered?"R":"Unr"); */
    sprintf(reply_ptr2,"\r--- WWIVToss v.%s\r",VERSION); /* MMH mod */
    reply_ptr2 = strchr(reply_ptr2,0);
    sprintf(reply_ptr2," * Origin:  %s (%d:%d/%d.%d)\r",cfg.origin_line,fadr.zone,fadr.net,fadr.node,fadr.point);
    reply_ptr2 = strchr(reply_ptr2,0);

    strcpy(fido_to_user,uplink[current_uplink].areafix_prog);
    strcpy(fido_from_user,cfg.sysop_name);
    strcpy(fido_title,uplink[current_uplink].areafix_pw);
    temp_net=out_net;
    temp_node=out_node;
    out_net=fadr.net;
    out_node=fadr.node;
    send_message(2,reply2);
    export_mail++;
    free(temp2);
    free(reply2);
    free(reply_ptr2);
    out_net=temp_net;
    out_node=temp_node;

    write_log("Adding to AREAS.DAT\n");
    sprintf(s,"1WWIVTOSS:Auto-Added %s - Please Setup in WSETUP!",echo_name);
    send_nag(s);
    num_areas=cfg.num_areas;
    i=cfg.num_areas;
    strcpy(thisarea.name,echo_name);
    strcpy(thisarea.comment,"Auto-Added Area");
    strcpy(thisarea.subtype,"-1");
    strcpy(thisarea.origin,cfg.origin_line);
    thisarea.type=0;
    thisarea.alias_ok=0;
    thisarea.pass_color=0;
    thisarea.high_ascii=0;
    thisarea.net_num=0;
    thisarea.group=0;
    thisarea.count=0;
    thisarea.keep_tag=0;
    thisarea.def_origin=1;
    thisarea.translate=0;
    thisarea.feed[0].zone=fadr.zone;
    thisarea.feed[0].net=fadr.net;
    thisarea.feed[0].node=fadr.node;
    thisarea.feed[0].point=fadr.point;
    thisarea.feed[1].zone=tadr.zone;
    thisarea.feed[1].net=tadr.net;
    thisarea.feed[1].node=tadr.node;
    thisarea.feed[1].point=tadr.point;
    write_area(i,&thisarea);
    cfg.num_areas++;
    write_config();
//    return(0);
}
void dv_pause(void)
{
  __emit__(0xb8, 0x1a, 0x10, 0xcd, 0x15);
  __emit__(0xb8, 0x00, 0x10, 0xcd, 0x15);
  __emit__(0xb8, 0x25, 0x10, 0xcd, 0x15);
}

void win_pause(void)
{
  __emit__(0x55, 0xb8, 0x80, 0x16, 0xcd, 0x2f, 0x5d);
}

int get_dos_version(void)
{
  _AX = 0x3000;
  geninterrupt(0x21);
  if (_AX % 256 >= 10) {
    multitasker |= MT_OS2;
  }
  return (_AX);
}

int get_dv_version(void)
{
  int v;

  if (multitasker & MT_OS2)
    return 0;
  _AX = 0x2b01;
  _CX = 0x4445;
  _DX = 0x5351;
  geninterrupt(0x21);
  if (_AL == 0xff) {
    return 0;
  } else {
    v = _BX;
    multitasker |= MT_DESQVIEW;
    return v;
  }
}

int get_win_version(void)
{
  int v = 0;

  __emit__(0x55, 0x06, 0x53);
  _AX = 0x352f;
  geninterrupt(0x21);
  _AX = _ES;
  if (_AX | _BX) {
    _AX = 0x1600;
    geninterrupt(0x2f);
    v = _AX;
    if (v % 256 <= 1)
      v = 0;
  }
  __emit__(0x5b, 0x07, 0x5d);
  if (v != 0)
    multitasker |= MT_WINDOWS;
  return (v);
}

int get_nb_version(void)
{
  _AX = 0;
  geninterrupt(0x2A);
  return (_AH);
}

void detect_multitask(void)
{
  get_dos_version();
  get_win_version();
  get_dv_version();
  if (multitasker < 2)
    if (get_nb_version())
      multitasker = MT_NB;
}

void giveup_timeslice(void)
{
  if (multitasker) {
    switch (multitasker) {
 case 1:
 case 3:
        dv_pause();
        break;
      case 2:
      case 4:
      case 5:
      case 6:
      case 7:
        win_pause();
        break;
      default:
        break;
    }
  }
}
int check_for_dupe(void)
{
    char s[81];
    int f, nu,un;
    long pos;

    if (cfg.check_dupes==0)
        return(0);
    if ((msgtype==4) || (msgtype==2) || (msgtype==7))
        return(0);
    strcpy(s,"WTMSGID.DAT");
    f=sh_open1(s,O_RDWR | O_BINARY | O_CREAT);
    if (f<0)
    {
        return (0);
    }

    nu=((int) (filelength(f)/sizeof(msgdupe)));
    for (un=0;un<nu;un++)
    {
        pos=((long) sizeof(msgdupe)) * ((long) un);
        sh_lseek(f,pos,SEEK_SET);
        sh_read(f, (void *) (&dupe), sizeof(msgdupe));
        if ((stricmp(dupe.msg_id,msgidd)==0) && (stricmp(dupe.address,dupeadr)==0))
        {
            sh_close(f);
            return(1);
        }
    }
    strcpy(dupe.msg_id,msgidd);
    strcpy(dupe.address,dupeadr);
    pos=((long) sizeof(msgdupe)) * ((long) nu);
    sh_lseek(f,pos,SEEK_SET);
    sh_write(f, (void *) (&dupe),sizeof(msgdupe));
    sh_close(f);
    return(0);

}
