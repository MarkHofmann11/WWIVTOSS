/*****************************************************************************

				WWIV Version 4
                    Copyright (C) 1988-1995 by Wayne Bell

Distribution of the source code for WWIV, in any form, modified or unmodified,
without PRIOR, WRITTEN APPROVAL by the author, is expressly prohibited.
Distribution of compiled versions of WWIV is limited to copies compiled BY
THE AUTHOR.  Distribution of any copies of WWIV not compiled by the author
is expressly prohibited.


*****************************************************************************/

#pragma hdrstop

#include <stdio.h>
#include <string.h>
#include <alloc.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>


#ifdef __OS2__
#define far
#define huge
#endif /* __OS2__ */

#include "subxtr.h"


/****************************************************************************/

static char *mallocin_file(char *fn, long *len)
{
  char *ss;
  int f;

  *len=0;
  ss=NULL;

  f=sh_open1(fn, O_RDONLY|O_BINARY);
  if (f>0) {
    *len=filelength(f);
    ss=(char *)bbsmalloc(*len+20);
    if (ss) {
      read(f,ss,*len);
      ss[*len]=0;
    }
    sh_close(f);
    return(ss);
  }
  return(ss);
}

/****************************************************************************/

static char *skipspace(char *ss2)
{
  while ((*ss2) && ((*ss2!=' ') && (*ss2!='\t')))
    ++ss2;
  if (*ss2)
    *ss2++=0;
  while ((*ss2==' ') || (*ss2=='\t'))
    ++ss2;
  return(ss2);
}


/****************************************************************************/

static xtrasubsnetrec *xsubsn;
static xtrasubsnetrec *xnp;
static xtrasubsrec *xtr;

static int nn;

static xtrasubsnetrec *fsub(int netnum, unsigned short type)
{
  int i;

  if (type) {
    for (i=0; i<nn; i++) {
      if ((xsubsn[i].net_num==netnum) && (xsubsn[i].type==type))
        return(&(xsubsn[i]));
    }
  }
  return(NULL);
}

int read_subs_xtr(int max_subs, int num_subs, subboardrec *subboards)
{
  char *ss,*ss1,*ss2;
  char huge *xx;
  long l;
  int n,i,curn;
  char s[81];
  xtrasubsnetrec *xnp;


  if (xsubs) {
    for (i=0; i<num_subs; i++) {
      if ((xsubs[i].flags&XTRA_MALLOCED) && xsubs[i].nets)
        bbsfree(xsubs[i].nets);
    }
    bbsfree((void *)xsubs);
    if (xsubsn)
      bbsfree(xsubsn);
    xsubsn=NULL;
    xsubs=NULL;
  }

  l=((long)max_subs)*sizeof(xtrasubsrec);
  xsubs=(xtrasubsrec huge *)bbsmalloc(l+1);
  if (!xsubs) {
    printf("Insufficient memory (%ld bytes) for SUBS.XTR\n", l);
    getch();
    return(1);
  }

  xx=(char huge *)xsubs;
  while (l>0) {
    if (l>32768) {
      memset((void *)xx,0,32768);
      l-=32768;
      xx += 32768;
    } else {
      memset((void *)xx,0,l);
      break;
    }
  }

  sprintf(s,"%sSUBS.XTR",syscfg.datadir);
  ss=mallocin_file(s,&l);
  nn=0;
  if (ss) {
    for (ss1=strtok(ss,"\r\n"); ss1; ss1=strtok(NULL,"\r\n")) {
      if (*ss1=='$')
        ++nn;
    }
    bbsfree(ss);
  } else {
    for (i=0; i<num_subs; i++) {
      if (subboards[i].type)
        ++nn;
    }
  }
  if (nn) {
    l=((long)nn)*sizeof(xtrasubsnetrec);
    xsubsn=(xtrasubsnetrec *)bbsmalloc(l);
    if (!xsubsn) {
      printf("Insufficient memory (%ld bytes) for net subs info\n", l);
      getch();
      return(1);
    }
    memset(xsubsn, 0, l);

    nn=0;
    ss=mallocin_file(s,&l);
    if (ss) {
      curn=-1;
      for (ss1=strtok(ss,"\r\n"); ss1; ss1=strtok(NULL,"\r\n")) {
        switch(*ss1) {
          case '!': /* sub idx */
            curn=atoi(ss1+1);
            if ((curn<0) || (curn>=num_subs))
              curn=-1;
            break;
          case '@': /* desc */
            if (curn>=0) {
              strncpy(xsubs[curn].desc,ss1+1,60);
            }
            break;
          case '#': /* flags */
            if (curn>=0) {
              xsubs[curn].flags=atol(ss1+1);
            }
            break;
          case '$': /* net info */
            if (curn>=0) {
              if (!xsubs[curn].num_nets)
                xsubs[curn].nets=&(xsubsn[nn]);
              ss2=skipspace(++ss1);
              for (i=0; i<net_num_max; i++) {
                if (stricmp(net_networks[i].name, ss1)==0)
                  break;
              }
              if ((i<net_num_max) && (*ss2)) {
                xsubsn[nn].net_num=i;
                ss1=ss2;
                ss2=skipspace(ss2);
                strncpy(xsubsn[nn].stype,ss1,7);
                xsubsn[nn].type=atoi(xsubsn[nn].stype);
                if (xsubsn[nn].type)
                  sprintf(xsubsn[nn].stype,"%u",xsubsn[nn].type);
                ss1=ss2;
                ss2=skipspace(ss2);
                xsubsn[nn].flags=atol(ss1);
                ss1=ss2;
                ss2=skipspace(ss2);
                xsubsn[nn].host=atoi(ss1);
                ss1=ss2;
                ss2=skipspace(ss2);
                xsubsn[nn].category=atoi(ss1);
                nn++;
                xsubs[curn].num_nets++;
                xsubs[curn].num_nets_max++;
              } else {
                printf("Unknown network '%s' in SUBS.XTR\n",ss1);
              }
            }
            break;
          case 0:
            break;
          default:
            break;
        }
      }
      bbsfree(ss);
    } else {
      for (curn=0; curn<num_subs; curn++) {
        if (subboards[curn].type) {
          if (subboards[curn].age&0x80)
            xsubsn[nn].net_num=subboards[curn].name[40];
          else
            xsubsn[nn].net_num=0;
          if ((xsubsn[nn].net_num>=0) && (xsubsn[nn].net_num<net_num_max)) {
            xsubs[curn].nets=&(xsubsn[nn]);
            xsubsn[nn].type=subboards[curn].type;
            sprintf(xsubsn[nn].stype,"%u",xsubsn[nn].type);
            nn++;
            xsubs[curn].num_nets=1;
            xsubs[curn].num_nets_max=1;
          }
        }
      }
      for (n=0; n<net_num_max; n++) {
        sprintf(s,"%sALLOW.NET",net_networks[n].dir);
        ss=mallocin_file(s,&l);
        if (ss) {
          for (ss1=strtok(ss," \t\r\n"); ss1; ss1=strtok(NULL," \t\r\n")) {
            xnp=fsub(n, atoi(ss1));
            if (xnp)
              xnp->flags |= XTRA_NET_AUTO_ADDDROP;
          }
          bbsfree(ss);
        }

        sprintf(s,"%sSUBS.PUB",net_networks[n].dir);
        ss=mallocin_file(s,&l);
        if (ss) {
          for (ss1=strtok(ss," \t\r\n"); ss1; ss1=strtok(NULL," \t\r\n")) {
            xnp=fsub(n, atoi(ss1));
            if (xnp)
              xnp->flags |= XTRA_NET_AUTO_INFO;
          }
          bbsfree(ss);
        }
        sprintf(s,"%sNNALL.NET",net_networks[n].dir);
        ss=mallocin_file(s,&l);
        if (ss) {
          for (ss1=strtok(ss,"\r\n"); ss1; ss1=strtok(NULL,"\r\n")) {
            while ((*ss1==' ') || (*ss1=='\t'))
              ++ss1;
            ss2=skipspace(ss1);
            xnp=fsub(n, atoi(ss1));
            if (xnp)
              xnp->host=atoi(ss2);
          }
          bbsfree(ss);
        }
      }
      for (i=0; i<nn; i++) {
        if ((xsubsn[i].type) && (!xsubsn[i].host)) {
          sprintf(s,"%sNN%u.NET",
                  net_networks[xsubsn[i].net_num].dir,
                  xsubsn[i].type);
          ss=mallocin_file(s,&l);
          if (ss) {
            for (ss1=ss; (*ss1) && ((*ss1<'0') || (*ss1>'9')); ss1++)
              ;
            xsubsn[i].host=atoi(ss1);
            bbsfree(ss);
          }
        }
      }
    }
  }

  return(0);
}
