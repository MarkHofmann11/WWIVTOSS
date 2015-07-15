/* ------------------------------------------------------------------------ */
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
/*  File    : DAWG.C                                                        */
/*                                                                          */
/*  Purpose : Handles Writing of Incoming Messages Directly To WWIV         */
/*              Data Bases.                                                 */
/*                                                                          */
/*  Credit for this section belongs to Lawrence C. Bensinger (Dawg) who     */
/*  provided the basic code, which I was able to modify for WWIVTOSS        */
/*  purposes.  Permission was granted to use this code section in WWIVTOSS. */
/*                                                                          */
/* ------------------------------------------------------------------------ */

#include <errno.h>
#include "vardec.h"
#include "vars.h"
#include "fcns.h"
#include "net.h"
#include "subxtr.c"
#define MAX_TO_CACHE 15             /* max postrecs to hold in cache */

/****************************************************************************/
void add_post(postrec *pp)
{
    postrec p;
    int close_file=0;

  /* open the sub, if necessary  */
    
    if (sub_f <= 0)
    {
        open_sub(1);
        close_file=1;
    }

    if (sub_f>=0)
    {
        /* get updated info */
        read_status();
        sh_lseek(sub_f, 0L, SEEK_SET);
        sh_read(sub_f, &p, sizeof(postrec));

        /* one more post */
        p.owneruser++;
        nummsgs=p.owneruser;
        sh_lseek(sub_f, 0L, SEEK_SET);
        sh_write(sub_f, &p, sizeof(postrec));

        /* add the new post */
        sh_lseek(sub_f, ((long)nummsgs)*sizeof(postrec), SEEK_SET);
        sh_write(sub_f, pp, sizeof(postrec));

        /* we've modified the sub */
        believe_cache=0;
        subchg=0;
        sub_dates[curlsub]=pp->qscan;
    }

    if (close_file)
        close_sub();
}

postrec *get_post(unsigned short mn)
/*
 * Returns info for a post.  Maintains a cache.  Does not correct anything
 * if the sub has changed.
 */
{
  postrec p;
  int close_file;

  if (mn==0)
    return(NULL);

  if (subchg==1) {
    /* sub has changed (detected in read_status); invalidate cache */
    believe_cache=0;

    /* kludge: subch==2 leaves subch indicating change, but the '2' value */
    /* indicates, to this routine, that it has been handled at this level */
    subchg=2;
  }

  /* see if we need new cache info */
  if ((!believe_cache) ||
      (mn<cache_start) ||
      (mn>=(cache_start+MAX_TO_CACHE))) {

    if (sub_f < 0) {
      /* open the sub data file, if necessary */
      if (open_sub(0)<0)
        return(NULL);

      close_file = 1;
    } else {
      close_file = 0;
    }

    /* re-read # msgs, if needed */
    if (subchg==2) {
      sh_lseek(sub_f, 0L, SEEK_SET);
      sh_read(sub_f, &p, sizeof(postrec));
      nummsgs=p.owneruser;

      /* another kludge: subch==3 indicates we have re-read # msgs also */
      /* only used so we don't do this every time through */
      subchg=3;

      /* adjust msgnum, if it is no longer valid */
      if (mn>nummsgs)
        mn=nummsgs;
    }

    /* select new starting point of cache */
    if (mn >= last_msgnum) {
      /* going forward */

      if (nummsgs<=MAX_TO_CACHE)
        cache_start=1;

      else if (mn>(nummsgs-MAX_TO_CACHE))
        cache_start=nummsgs-MAX_TO_CACHE+1;

      else
        cache_start = mn;

    } else {
      /* going backward */
      if (mn>MAX_TO_CACHE)
        cache_start = mn-MAX_TO_CACHE+1;
      else
        cache_start = 1;
    }

    if (cache_start<1)
      cache_start=1;

    /* read in some sub info */
    sh_lseek(sub_f, ((long) cache_start)*sizeof(postrec), SEEK_SET);
    sh_read(sub_f, cache, MAX_TO_CACHE*sizeof(postrec));

    /* now, close the file, if necessary */
    if (close_file) {
      close_sub();
    }

    /* cache is now valid */
    believe_cache = 1;
  }

  /* error if msg # invalid */
  if ((mn<1) || (mn>nummsgs))
    return(NULL);

  last_msgnum=mn;

  return(cache+(mn-cache_start));
}

void close_sub(void)
{
  if (sub_f>=0) {
    sub_f=sh_close(sub_f);
  }
}

/****************************************************************************/

int open_sub(int wr)
{
  postrec p;
  if (sub_f>=0)
    close_sub();
  if (wr) {
    sub_f = sh_open1(subdat_fn, O_RDWR|O_BINARY|O_CREAT);
    if (sub_f>=0) {

      /* re-read info from file, to be safe */
      believe_cache = 0;
      sh_lseek(sub_f, 0L, SEEK_SET);
      sh_read(sub_f, &p, sizeof(postrec));
      nummsgs=p.owneruser;
    }
  } else {
    sub_f = sh_open1(subdat_fn, O_RDONLY|O_BINARY);
  }

  return(sub_f);
}


void set_net_num(int n)
{
  if ((n>=0) && (n<net_num_max)) {
    net_num=n;
    net_name=net_networks[net_num].name;
    strcpy(net_data,net_networks[net_num].dir);
    net_sysnum=net_networks[net_num].sysnum;
    sprintf(wwiv_net_no,"WWIV_NET=%d",net_num);
  }
}

void addline(char *b, char *s, long *len)
{
  strcpy(&(b[*len]),s);
  *len +=strlen(s);
  strcpy(&(b[*len]),"\r\n");
  *len += 2;
}


#define MAX_XFER 61440 /* 60 * 1024 */

long huge_xfer(int fd, void huge *buf, unsigned sz, unsigned nel, int wr)
{
  long nxfr=0, len=((long)sz) * ((long)nel);
  char huge *xbuf = (char huge *)buf;
  unsigned cur,cur1;

  while (len>0) {

    if (len>MAX_XFER)
      cur=MAX_XFER;
    else
      cur=len;

    if (wr)
      cur1=sh_write(fd,(char *)xbuf,cur);
    else
      cur1=sh_read(fd,(char *)xbuf,cur);

    if (cur1!=65535) {
      len  -= cur1;
      nxfr += cur1;
      xbuf  = ((char huge *)buf) + nxfr;
      /* not xbuf += cur1 due to bug in certain version of compiler */
    }

    if (cur1!=cur)
      break;
  }

  return(nxfr);
}


int read_subs(void)
{
  int f;
  char s[81];

  if(subboards)
    farfree(subboards);
  subboards=NULL;
  max_subs=syscfg.max_subs;
  subboards=(subboardrec *) mallocx(max_subs*sizeof(subboardrec), "subboards");
  sprintf(s,"%sSUBS.DAT",syscfg.datadir);
  f=sh_open1(s,O_RDONLY | O_BINARY);
  if (f<0) {
    printf("%s NOT FOUND.\n",s);
    return(1);
  }
  num_subs=(sh_read(f,subboards, (max_subs*sizeof(subboardrec))))/
    sizeof(subboardrec);
  f=sh_close(f);
  return(read_subs_xtr(max_subs, num_subs, subboards));
}




void save_status(void)
/* saves system status in memory to disk */
{
  char s[81];
  if (statusfile<0) {
    sprintf(s,"%sSTATUS.DAT",syscfg.datadir);
    statusfile=sh_open1(s,O_RDWR | O_BINARY);
  } else {
    lseek(statusfile, 0L, SEEK_SET);
  }
  if (statusfile<0) {
    printf("STATUS.DAT >CANNOT< be saved!!");
  } else {
    sh_write(statusfile, (void *)(&status), sizeof(statusrec));
    statusfile=sh_close(statusfile);
  }
}

void get_status(int mode, int lock)
{
  char s[81];
  char fc[7];
  int i,i1,f;
  long l;

  if (statusfile<0) {
    sprintf(s,"%sSTATUS.DAT",syscfg.datadir);
    if (lock)
      statusfile=sh_open1(s,O_RDWR | O_BINARY);
    else
      statusfile=sh_open1(s,O_RDONLY | O_BINARY);

  } else {
    lseek(statusfile, 0L, SEEK_SET);
  }
  if (statusfile<0) {
    printf("%s NOT FOUND.\n",s);
    exit(4);
  } else {
    l=status.qscanptr;
    for (i=0; i<7; i++)
      fc[i]=status.filechange[i];

    read(statusfile,(void *)(&status), sizeof(statusrec));
    if (!lock)
      statusfile=sh_close(statusfile);
    if (l != status.qscanptr) {
      /* kill subs cache */
      for (i1=0; i1<num_subs; i1++) {
        if (sub_dates)
          sub_dates[i1]=0;
      }
      subchg=1;
      gatfn[0]=0;
    }
    for (i=0; i<7; i++) {
      if (fc[i]!=status.filechange[i]) {
        switch(i) {
          case filechange_names: /* re-read names.lst */
            if (smallist) {
              sprintf(s,"%sNAMES.LST",syscfg.datadir);
              f=sh_open1(s,O_RDONLY | O_BINARY);
              if (f>=0) {
                huge_xfer(f, smallist, sizeof(smalrec), status.users, 0);
                sh_close(f);
              }
            }
            break;
          case filechange_upload: /* kill dirs cache */
            break;
          case filechange_posts:
            subchg=1;
            gatfn[0]=0;
            break;
          case filechange_email:
            emchg=1;
            break;
        }
      }
    }
  }
}

void lock_status(void)
{
  get_status(1, 1);
}

void read_status(void)
{
  get_status(1, 0);
}


/****************************************************************************/

int iscan1(int si, int quick)
/*
 * Initializes use of a sub value (subboards[], not usub[]).  If quick, then
 * don't worry about anything detailed, just grab qscan info.
 */
{
  postrec p;

  /* make sure we have cache space */
  if (!cache) {
    cache=malloca(MAX_TO_CACHE*sizeof(postrec));
    if (!cache)
      return(0);
  }
  /* forget it if an invalid sub # */
  if ((si<0) || (si>=num_subs))
    return(0);

  /* skip this stuff if being called from the WFC cache code */
  if (!quick) {

    /* go to correct net # */
    if (xsubs[si].num_nets)
      set_net_num(xsubs[si].nets[0].net_num);
    else
      set_net_num(0);

    /* see if a sub has changed */
    read_status();
    /* if already have this one set, nothing more to do */
    if (si==currsub)
      return(1);
  }
  currsub=si;

  /* sub cache no longer valid */
  believe_cache=0;

  /* set sub filename */
  sprintf(subdat_fn,"%s%s.SUB",syscfg.datadir,subboards[si].filename);
  /* open file, and create it if necessary */
  if (open_sub(0)<0) {
    if (open_sub(1)<0)
      return(0);
    p.owneruser=0;
    sh_write(sub_f,(void *) &p,sizeof(postrec));
  }

  /* set sub */
  curlsub=si;
  subchg=0;
  last_msgnum=0;

  /* read in first rec, specifying # posts */
  sh_lseek(sub_f,0L,SEEK_SET);
  sh_read(sub_f,&p, sizeof(postrec));
  nummsgs=p.owneruser;

  /* read in sub date, if don't already know it */
  if (sub_dates[si]==0) {
    if (nummsgs) {
      sh_lseek(sub_f, ((long) nummsgs)*sizeof(postrec), SEEK_SET);
      sh_read(sub_f, &p, sizeof(postrec));
      sub_dates[si]=p.qscan;
    } else {
      sub_dates[si]=1;
    }
  }

  /* close file */
  close_sub();

  /* iscanned correctly */
  return(1);
}


/****************************************************************************/


int iscan(int b)
/*
 * Initializes use of a sub (usub[] value, not subboards[] value).
 */
{
  return(iscan1(b, 0));
}

void savebase(int b)
/* saves message information in memory to disk */
{
  int f;
  char s[81];

  sprintf(s, "%s%s.SUB", syscfg.datadir, subboards[b].filename);
  f = sh_open(s, O_RDWR | O_BINARY | O_TRUNC | O_CREAT, S_IREAD | S_IWRITE);
  sh_lseek(f, 0L, SEEK_SET);
  msgs[0].owneruser = nummsgs;
  sh_write(f, (void *)(&msgs[0]), ((nummsgs+1) * sizeof(postrec)));
  sh_close(f);
  if (nummsgs) {
    sub_dates[b]=msgs[nummsgs].qscan;
  } else {
    sub_dates[b]=1;
  }
    lock_status();
    status.filechange[filechange_posts]++;
  save_status();
}


int open_file(char *fn)
/* opens a WWIV Type-2 data file */
{
  int f,i;
  char s[81];

  sprintf(s,"%s%s.DAT",syscfg.msgsdir,fn);
  f=sh_open1(s,O_RDWR | O_BINARY);
  if (f<0) {
    f=sh_open(s,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    for (i=0; i<2048; i++)
      gat[i]=0;
    sh_write(f,(void *)gat,4096);
    strcpy(gatfn,fn);
/*    chsize(f,4096L + (75L * 1024L)); */
  }
  if (strcmp(gatfn,fn)) {
    sh_lseek(f,0L,SEEK_SET);
    sh_read(f,(void *)gat,4096);
    strcpy(gatfn,fn);
  }
  return(f);
}


void remove_link(messagerec *m1, char *aux)
/* deletes a message */
{
  messagerec m;
  char s[81],s1[81];
  int f;
  long csec,nsec;

  m=*m1;
  strcpy(s,syscfg.msgsdir);
  switch(m.storage_type) {
    case 0:
    case 1:
      ltoa(m.stored_as,s1,16);
      if (m.storage_type==1) {
        strcat(s,aux);
	strcat(s,"\\");
      }
      strcat(s,s1);
      unlink(s);
      break;
    case 2:
      f=open_file(aux);
      csec=m.stored_as;
      while ((csec>0) && (csec<2048)) {
	nsec=(long) gat[csec];
	gat[csec]=0;
	csec=nsec;
      }
      sh_lseek(f,0L,SEEK_SET);
      sh_write(f,(void *)gat,4096);
      sh_close(f);
      break;
    default:
      /* illegal storage type */
      break;
  }
  setcbrk(1);
}


#define GATSECLEN (4096L+2048L*512L)
#ifndef MSG_STARTING
#define MSG_STARTING (((long)gat_section)*GATSECLEN + 4096L)
#endif

void set_gat_section(int f, int section)
{
  long l,l1;
  int i;

  if (gat_section!=section) {
    l=filelength(f);
    l1=((long)section)*GATSECLEN;
    if (l<l1) {
      chsize(f,l1);
      l=l1;
    }
    sh_lseek(f,l1,SEEK_SET);
    if (l<(l1+4096L)) {
      for (i=0; i<2048; i++)
        gat[i]=0;
      sh_write(f,(void *)gat, 4096);
    } else {
      sh_read(f,(void *)gat, 4096);
    }
    gat_section=section;
  }
}


void save_gat(int f)
{
  long l;

  l=((long)gat_section)*GATSECLEN;
  sh_lseek(f,l,SEEK_SET);
  sh_write(f,(void *)gat,4096);
  lock_status();
  status.filechange[filechange_posts]++;
  save_status();
}





void savefile(char *b, long l1, messagerec *m1, char *aux)
/* saves a message in memory to disk */
{
  int f,gatp,i5,i4,gati[128];
  messagerec m;
  char s[81],s1[81];
  int section;
  long l2;

  setcbrk(0);
  m=*m1;
  switch(m.storage_type) {
    case 0:
    case 1:
      lock_status();
      m.stored_as=status.qscanptr++;
      save_status();
      ltoa(m.stored_as,s1,16);
      strcpy(s,syscfg.msgsdir);
      if (m.storage_type==1) {
        strcat(s,aux);
        strcat(s,"\\");
      }
      strcat(s,s1);
      f=sh_open(s,O_RDWR | O_CREAT | O_BINARY,S_IREAD | S_IWRITE);
      sh_write(f, (void *)b,l1);
      sh_close(f);
      break;
    case 2:
      f=open_file(aux);
      if (f>0) {
        for (section=0; section<1024; section++) {
          set_gat_section(f,section);
          gatp=0;
          i5=(int) ((l1 + 511L)/512L);
          i4=1;
          while ((gatp<i5) && (i4<2048)) {
            if (gat[i4]==0)
              gati[gatp++]=i4;
            ++i4;
          }
          if (gatp>=i5) {
            l2=MSG_STARTING;
            gati[gatp]=-1;
            for (i4=0; i4<i5; i4++) {
              sh_lseek(f,l2 + 512L * (long)(gati[i4]),SEEK_SET);
              sh_write(f,(void *)(&b[i4*512]),512);
              gat[gati[i4]]=gati[i4+1];
            }
            save_gat(f);
            break;
          }
        }
        sh_close(f);
      }
      m.stored_as=((long) gati[0]) + ((long)gat_section)*2048L;
      break;
    case 255:
      f=sh_open(aux,O_RDWR | O_CREAT | O_BINARY,S_IREAD | S_IWRITE);
      sh_write(f, (void *)b,l1);
      sh_close(f);
      break;
    default:
      break;
  }
  farfree((void *)b);
  *m1=m;
}


char *readfile(messagerec *m1, char *aux, long *l)
/* reads a message from disk into memory */
{
  int f,csec;
  long l1,l2;
  char *b,s[81],s1[81];
  messagerec m;

  setcbrk(0);
  *l=0L;
  m=*m1;
  switch(m.storage_type) {
    case 0:
    case 1:
      strcpy(s,syscfg.msgsdir);
      ltoa(m.stored_as,s1,16);
      if (m.storage_type==1) {
        strcat(s,aux);
	strcat(s,"\\");
      }
      strcat(s,s1);
      f=sh_open1(s,O_RDONLY | O_BINARY);
      if (f==-1) {
        *l=0L;
        return(NULL);
      }
      l1=filelength(f);
      if ((b=(char *)farmalloc(l1))==NULL) {
        sh_close(f);
        return(NULL);
      }
      sh_read(f,(void *)b,l1);
      sh_close(f);
      *l=l1;
      break;
    case 2:
      f=open_file(aux);
      csec=m.stored_as;
      l1=0;
      while ((csec>0) && (csec<2048)) {
        l1+=512L;
        csec=gat[csec];
      }
      if (!l1) {
        sh_close(f);
        return(NULL);
      }
      if ((b=(char *)farmalloc(l1))==NULL) {
        sh_close(f);
        return(NULL);
      }
      csec=m.stored_as;
      l1=0;
      while ((csec>0) && (csec<2048)) {
        sh_lseek(f,4096L + 512L*csec,SEEK_SET);
        l1+=(long)read(f,(void *)(&(b[l1])),512);
        csec=gat[csec];
      }
      sh_close(f);
      l2=l1-512;
      while ((l2<l1) && (b[l2]!=26))
	++l2;
      *l=l2;
      break;
    case 255:
      f=sh_open1(aux,O_RDONLY | O_BINARY);
      if (f==-1) {
        *l=0L;
        return(NULL);
      }
      l1=filelength(f);
      if ((b=(char *)farmalloc(l1+256L))==NULL) {
        sh_close(f);
        return(NULL);
      }
      sh_read(f,(void *)b,l1);
      sh_close(f);
      *l=l1;
      break;
    default:
      /* illegal storage type */
      *l=0L;
      b=NULL;
      break;
  }
  return(b);
}

unsigned char *getnetfile(void)
{
  int i,z;
  char t[81];
  static unsigned char sy[256];

  strcpy(sy,"");
  z=0;
  for (i = 0; (z!=1) ; i++) {
    sprintf(t,"%sP-0-%d.NET",net_data,i);    
    if (!(exist(t))) {
      z=1;
      strcpy(sy,t);
      return(sy);
    }
  }
  strcpy(sy,t);
  return(sy);
}  


void gate_msg(net_header_rec *nh, char *text, unsigned short nn, char *byname,unsigned int *list, unsigned short fnn)
{
  char newname[128], fn[100], qn[100], on[100];
  char *ti, nm[82], *ss;
  int f,i;
  unsigned short ntl;

  if (strlen(text)<80) {
    ti=text;
    text+=strlen(ti)+1;
    ntl=nh->length-strlen(ti)-1;
    ss=strchr(text,'\r');
    if (ss && (ss-text<80) && (ss-text<ntl)) {
      strncpy(nm,text,ss-text);
      nm[ss-text]=0;
      ss++;
      if (*ss=='\n')
        ss++;
      nh->length-=(ss-text);
      ntl-=(ss-text);
      text=ss;

      qn[0]=on[0]=0;

      if (fnn==65535) {

        strcpy(newname,nm);
        ss=strrchr(newname,'@');
        if (ss) {
          sprintf(ss+1,"%u",net_networks[nn].sysnum);
          ss=strrchr(nm,'@');
          if (ss) {
            ++ss;
            while ((*ss>='0') && (*ss<='9'))
              ++ss;
            strcat(newname,ss);
          }
          strcat(newname,"\r\n");
        }
      } else {
        if ((nm[0]=='`') && (nm[1]=='`')) {
          for (i=strlen(nm)-2; i>0; i--) {
            if ((nm[i]=='`') && (nm[i+1]=='`')) {
              break;
            }
          }
          if (i>0) {
            i+=2;
            strncpy(qn,nm,i);
            qn[i]=' ';
            qn[i+1]=0;
          }
        } else
          i=0;
        if (qn[0]==0) {
          ss=strrchr(nm,'#');
          if (ss) {
            if ((ss[1]>='0') && (ss[1]<='9')) {
              *ss=0;
              ss--;
              while ((ss>nm) && (*ss==' ')) {
                *ss=0;
                ss--;
              }
            }
          }
          if (nm[0]) {
            if (nh->fromuser)
              sprintf(qn,"``%s`` ",nm);
            else
              strcpy(on,nm);
          }
        }
        if ((on[0]==0) && (nh->fromuser==0)) {
          strcpy(on,nm+i);
        }
        if (on[0])
          sprintf(newname,"%s%s %s AT %u\r\n",qn, net_networks[fnn].name,
                  on, nh->fromsys);
        else
          sprintf(newname,"%s%s #%u AT %u\r\n",qn, net_networks[fnn].name,
                  nh->fromuser, nh->fromsys);

        nh->fromsys=net_networks[nn].sysnum;
        nh->fromuser=0;
      }


      nh->length += strlen(newname);
      if ((nh->main_type == main_type_email_name) ||
          (nh->main_type == main_type_new_post))
        nh->length += strlen(byname)+1;
      sprintf(fn,"%sP0-WT.NET",net_data);
      f=sh_open(fn,O_RDWR|O_BINARY|O_APPEND | O_CREAT, S_IREAD|S_IWRITE);
      if (f) {
        sh_lseek(f,0L,SEEK_END);
        if (!list)
          nh->list_len=0;
        if (nh->list_len)
          nh->tosys=0;
        sh_write(f,nh, sizeof(net_header_rec));
        if (nh->list_len)
          sh_write(f,list,2*(nh->list_len));
        if ((nh->main_type == main_type_email_name) ||
            (nh->main_type == main_type_new_post))
          sh_write(f,byname, strlen(byname)+1);
        sh_write(f,ti,strlen(ti)+1);
        sh_write(f,newname,strlen(newname));
        sh_write(f,text,ntl);
        sh_close(f);
      }
    }
  }
}



void send_net(net_header_rec *nh, unsigned int *list, char *text, char *byname)
{
  int f;
  char s[100];
  long l1;

  sprintf(s,"%sP0-WT.NET",net_data);
  f=sh_open(s,O_RDWR | O_BINARY | O_APPEND | O_CREAT, S_IREAD | S_IWRITE);
  sh_lseek(f,0L,SEEK_END);
  if (!list)
    nh->list_len=0;
  if (!text)
    nh->length=0;
  if (nh->list_len)
    nh->tosys=0;
  l1=nh->length;
  if (byname && *byname)
    nh->length += strlen(byname)+1;
  sh_write(f,(void *)nh,sizeof(net_header_rec));
  if (nh->list_len)
    sh_write(f,(void *)list,2*(nh->list_len));
  if (byname && *byname)
    sh_write(f,byname, strlen(byname)+1);
  if (nh->length)
    sh_write(f,(void *)text,l1);
  sh_close(f);
}


void send_net_post(postrec *p, char *extra, int subnum)
{
  net_header_rec nh, nh1;
  char *b, *b1,s[81];
  long len1, len2;
  int f,i,onn,nn,n,nn1;
  unsigned int *list;
  xtrasubsnetrec *xnp;

  if (extra[0]==0)
    return;
  b=readfile(&(p -> msg),extra,&len1);
  if (b==NULL)
    return;
  onn=net_num;
  if (p->status & status_post_new_net)
    nn=p->title[80];
  else if (xsubs[subnum].num_nets)
    nn=xsubs[subnum].nets[0].net_num;
  else
    nn=net_num;

  nn1=nn;
  if (p->ownersys==0)
    nn=-1;

  nh1.tosys=0;
  nh1.touser=0;
  nh1.fromsys=p->ownersys;
  nh1.fromuser=p->owneruser;
  nh1.list_len=0;
  nh1.daten=p->daten;
  nh1.length=len1+1+strlen(p -> title);
  nh1.method=0;

  if (nh1.length > 32755) {
    nh1.length = 32755;
    len1=nh1.length-strlen(p->title)-1;
  }

  if ((b1=(char *)malloca(nh1.length+100))==NULL) {
    farfree(b);
    set_net_num(onn);
    return;
  }
  strcpy(b1,p -> title);
  memmove(&(b1[strlen(p -> title)+1]),b,(unsigned int) len1);
  farfree(b);

  for (n=0; n<xsubs[subnum].num_nets; n++) {
    xnp=&(xsubs[subnum].nets[n]);

    if ((xnp->net_num==nn) && (xnp->host))
      continue;
    set_net_num(xnp->net_num);

    nh=nh1;
    list=NULL;
    nh.minor_type=xnp->type;
    if (!nh.fromsys)
      nh.fromsys=net_sysnum;

    if (xnp->host) {
      nh.main_type=main_type_pre_post;
      nh.tosys=xnp->host;
    } else {
      nh.main_type=main_type_post;
      sprintf(s,"%sN%s.NET",net_data, xnp->stype);
      f=sh_open1(s,O_RDONLY|O_BINARY);
      if (f>0) {
	len1=filelength(f);
	list=(unsigned int *)malloca(len1*2+1);
	if (!list)
	  continue;
	if ((b=(char *)malloca(len1+100L))==NULL) {
	  farfree(list);
	  continue;
	}
	sh_read(f,b,len1);
	sh_close(f);
	b[len1]=0;
	len2=0;
	while (len2<len1) {
	  while ((len2<len1) && ((b[len2]<'0') || (b[len2]>'9')))
	    ++len2;
	  if ((b[len2]>='0') && (b[len2]<='9') && (len2<len1)) {
	    i=atoi(&(b[len2]));
	    if (((net_num!=nn) || (nh.fromsys!=i)) && (i!=net_sysnum))
	      list[(nh.list_len)++]=i;
	    while ((len2<len1) && (b[len2]>='0') && (b[len2]<='9'))
	      ++len2;
	  }
	}
	farfree(b);
      }
      if (!nh.list_len) {
	if (list)
	  farfree(list);
	continue;
      }
    }
    if (!xnp->type)
      nh.main_type=main_type_new_post;
    if (nn1==net_num)
      send_net(&nh, list, b1, xnp->type?NULL:xnp->stype);
    else
      gate_msg(&nh, b1, xnp->net_num, xnp->stype, list, nn);
    if (list)
      farfree(list);
  }

  farfree(b1);
  set_net_num(onn);
}


/*************************************************************************/
/************** End Wayne Bell's Code (slightly modified) ****************/
/*************************************************************************/
#define BUFSIZE 32000

void deletemsg(int mn)
{
  postrec *p1, p;
  int close_file=0;
  unsigned int nb;
  char *buf;
//  char scratch[181];
  long len, l, cp;

  /* open file, if needed */
  if (sub_f <= 0) {
    open_sub(1);
    close_file=1;
  }

  /* see if anything changed */
  read_status();

  if (sub_f >= 0) {
    if ((mn>0) && (mn<=nummsgs)) {
      buf=(char *)malloca(BUFSIZE);
      if (buf) {
        p1=get_post(mn);
        remove_link(&(p1->msg),(subboards[curlsub].filename));

        cp=((long)mn+1)*sizeof(postrec);
        len=((long)(nummsgs+1))*sizeof(postrec);

        do {
          l=len-cp;
          if (l<BUFSIZE)
            nb=(int)l;
          else
            nb=BUFSIZE;
          if (nb) {
            sh_lseek(sub_f, cp, SEEK_SET);
            sh_read(sub_f, buf, nb);
            sh_lseek(sub_f, cp-sizeof(postrec), SEEK_SET);
            sh_write(sub_f, buf, nb);
            cp += nb;
          }
        } while (nb==BUFSIZE);

        /* update # msgs */
        sh_lseek(sub_f, 0L, SEEK_SET);
        sh_read(sub_f, &p, sizeof(postrec));
        p.owneruser--;
        nummsgs=p.owneruser;
        sh_lseek(sub_f, 0L, SEEK_SET);
        sh_write(sub_f, &p, sizeof(postrec));

        /* cache is now invalid */
        believe_cache = 0;

        bbsfree(buf);
      }
    }
  }

  /* close file, if needed */
  if (close_file)
    close_sub();

}


void far *malloca(unsigned long nbytes)
{
  void *buf;

  buf=farmalloc(nbytes+1);
  return(buf);
}
void write_user(unsigned int un, userrec *u)
{
  char s[81];
  long pos;
  int f;

  if ((un<1) || (un>syscfg.maxusers))
    return;

  sprintf(s,"%sUSER.LST",syscfg.datadir);
  f=sh_open(s,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
  if (f>=0) {
    pos=((long) syscfg.userreclen) * ((long) un);
    sh_lseek(f,pos,SEEK_SET);
    sh_write(f, (void *)u, syscfg.userreclen);
    sh_close(f);
  }
}

int open_email(int wrt)
{
  char s[100];
  int f,i;

  sprintf(s,"%sEMAIL.DAT", syscfg.datadir);

  for (i=0; i<5; i++) {
    if (wrt)
      f=sh_open(s, O_RDWR|O_BINARY|O_CREAT, S_IREAD|S_IWRITE);
    else
      f=sh_open1(s, O_RDONLY|O_BINARY);
    if ((f>0) || (errno!=EACCES))
      break;
  }

  return(f);
}
void send_email2(void)
{
    char s[81],scratch[81],log[81];
    int f, i;
    long len,len2;
    char *b;
    long thetime,dat;
    int len3;
    mailrec m,m1;
    userrec ur;
    messagerec msg;
    struct date dt;
    struct time tm;
    char *month[] = {"Bug","Jan","Feb","Mar","Apr","May","Jun",
		     "Jul","Aug","Sep","Oct","Nov","Dec"};

    m.touser=user_to_num;

    time(&thetime);

    len=(unsigned) strlen(buffer);
    len2=len;
    b=((char *)malloc(len+1024));
    len=0;
    sprintf(log,"þ Incoming mail for %s from %s (%s)\n",fido_to_user,fido_from_user,origi_node);
    write_log(log);
    sprintf(s,"%s (%s)",fido_from_user,origi_node);
    addline(b,s,&len);

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
	dat = dostounix(&dt,&tm);
	strncpy(scratch,ctime(&(time_t)dat),24);
	scratch[24]='\0';
    strcat(scratch,"\r\n");
    strcpy(s,scratch);
    addline(b,s,&len);

    strcat(b,buffer);
    len += len2;
    len=(unsigned) strlen(b);

    if (b[len-1]!=26)
        b[len++]=26;

    m.msg.storage_type=2;
    msg.stored_as=0L;
    msg.storage_type=2;
    savefile(b,len,&msg,"EMAIL");
    m.msg=msg;

    i=0;
    strcpy(m.title,fido_title);
//    i=strlen(m.title);
//    m.title[i+1]=net_num;
//    m.title[i+2]=0;
    m.title[80]=net_num;
    m.title[81]=0;
//    i=0;
    read_user(user_to_num,&ur);
    ++ur.waiting;
    write_user(user_to_num,&ur);
    lock_status();
    save_status();

    m.anony=i;
    m.fromsys=atoi(wwiv_node);
    m.fromuser=0;
    m.tosys=0;
    m.status=status_new_net;
    time((long *)&(m.daten));

    f=open_email(1);
    len3=(int) filelength(f)/sizeof(mailrec);
    if (len3==0)
	i=0;
    else
    {
        i=len3-1;
        sh_lseek(f,((long) (i))*(sizeof(mailrec)), SEEK_SET);
        sh_read(f,(void *)&m1,sizeof(mailrec));
        while ((i>0) && (m1.tosys==0) && (m1.touser==0))
        {
            --i;
            sh_lseek(f,((long) (i))*(sizeof(mailrec)), SEEK_SET);
	    sh_read(f,(void *)&m1,sizeof(mailrec));
	}
        if ((m1.tosys) || (m1.touser))
            ++i;
    }
    sh_lseek(f,((long) (i))*(sizeof(mailrec)), SEEK_SET);
    sh_write(f,(void *)&m,sizeof(mailrec));
    sh_close(f);

    free(buffer);
}
