#define PROG_NAME "WWIVToss DE139 Processor"
#define VERS "v1.0"
#include <dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <sys\stat.h>
#include <string.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <process.h>
#include "fts.h"

char sysop_name[51], bbs_name[51], netpath[81], fdlpath[81];
char data_path[81], dirname[13], gfile_path[81], flog[81];

void main(int argc, char *argv[]);
void spit_id(void);
void get_net(void);
int  exist(char *s);
void write_back(char *msg);
net_networks_rec net;
typedef struct {
        char            bbs_name[81],
                        sysop_name[81],
                        system_address[20],
                        network[20],
                        version[10];
        unsigned long   date_installed,
                        date_notified;
        unsigned int    node,
                        registered;
} notify_rec;

void main(int argc,char *argv[])
{
  FILE *fl, *ft;
  char s[81], s1[81], s2[81], f1[81], temp[81], dbname[13], temp1[81];
  char now_date[20];
  int i;
  long days_run;
  net_header_rec headrec;
  notify_rec notify;
  struct ffblk ffblk;
  time_t t;
  struct tm *systime;
    if (!argv[1]) {
      spit_id();
      exit(0);
    }
    if (argc<1) {
      puts("Too few command line parameters.");
      exit(1);
    }
      spit_id();
      get_net();
      strcpy(f1,argv[1]);
      strcpy(netpath,argv[1]);
      fl=fopen(f1,"rb");

      if (fl==NULL) {
         puts("þ Error Reading TEMPDE.XXX");
         exit(1);
      }
    t=time(NULL);
    systime=localtime(&t);
    sprintf(now_date,"%-2.2d/%-2.2d/%-2.2d",systime->tm_mon+1,systime->tm_mday,systime->tm_year);

      fseek(fl,0,SEEK_END);
      fseek(fl,0,SEEK_SET);
      fread(&notify,sizeof(notify_rec),1,fl);  /* Read in Request */
      sprintf(s,"%sWWIVTOSS.REC",gfile_path);
      ft=fopen(s,"a+");
      strcpy(s2,"*");
      for (i=0;i<75;i++)
        strcat(s2,"*");
      strcat(s2,"\n");
      fputs(s2,ft);
      sprintf(s1,"%s  WWIVToss %s In Use by @%u.%s   (%s)\n",now_date,notify.version,notify.node,notify.network,notify.system_address);
      fputs(s1,ft);
      sprintf(s1,"          BBS       : %s\n",notify.bbs_name);
      fputs(s1,ft);
      sprintf(s1,"          SysOp     : %s\n",notify.sysop_name);
      fputs(s1,ft);
      sprintf(s1,"          Installed : %s",ctime(&notify.date_installed));
      fputs(s1,ft);
      if (notify.registered)
        strcpy(temp,"R");
      else
        strcpy(temp,"Unr");
      sprintf(s1,"          Run for   : %u Days",((t-notify.date_installed)/86400));
      sprintf(temp1," %segistered!\n",temp);
      strcat(s1,temp1);
      fputs(s1,ft);
      fclose(ft);
      fclose(fl);
    sprintf(s1,"3WWIVToss %s In Used By 1@%u.%s (%u Days!)",notify.version,notify.node,notify.network,((t-notify.date_installed)/86400));
    unlink(argv[1]);
    write_back(s1);
}

void spit_id(void)
{
  printf("\r\n%s %s\r\n",PROG_NAME,VERS);
  printf("(c)1997 by Craig Dooley\r\n\r\n");
}

void get_net()
{
  FILE *cf, *nf;
  char s1[81];
  int i, num_nets;
  long fpos;
  configrec cfg;

    cf=fopen("CONFIG.DAT","rb");
    if (cf==NULL) {
      puts("þ Error Reading CONFIG.DAT");
      exit(3);
    }
    fread(&cfg,sizeof(configrec),1,cf);
    strcpy(sysop_name,cfg.sysopname);
    strcpy(bbs_name,cfg.systemname);
    strcpy(data_path,cfg.datadir);
    strcpy(gfile_path,cfg.gfilesdir);
    fclose(cf);
}

void write_back(char *msg)
{
  FILE *ll;
  char s[81], s1[81], s2[81], r1[81];

    strcpy(s,netpath);
    ll=fopen(s,"a");

    if (ll==NULL) puts("þ Error Creating TEMPDE.XXX");
    else {
        strcpy(r1,msg);

      }
      fputs(r1,ll);                                      /*MSG*/
      fclose(ll);
}

