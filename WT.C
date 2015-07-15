#include "vardec.h"
#include "vars.h"
#include "net.h"
#include "fcns.h"
#include "subacc.c"

void post(void)
{
  messagerec m;
  postrec p;
  char s[121];
  int i,dm,a,flag;
  time_t time1, time2;

    flag=0;

    m.storage_type=subboards[curlsub].storage_type;
    a=0;

    time1=time(NULL);

//  write_inst(INST_LOC_POST,curlsub,INST_FLAGS_NONE);

//  inmsg(&m,p.title,&a,1,(subboards[curlsub].filename),ALLOW_FULLSCREEN,
//    subboards[curlsub].name, (subboards[curlsub].anony&anony_no_tag)?1:0);
    savefile(buffer,length,&m,(subboards[curlsub].filename));
    if (m.stored_as!=0xffffffff)
    {
        p.anony=a;
        p.msg=m;
        p.ownersys=0;
        p.owneruser=usernum;
        lock_status();
        p.qscan=status.qscanptr++;
        save_status();
        time((long *)(&p.daten));
        p.status=0;

        open_sub(1);

        if ((xsubs[curlsub].num_nets) &&
          (subboards[curlsub].anony & anony_val_net) && (!lcs() || irt[0])) {
          p.status |= status_pending_net;
          dm=1;
          for (i=nummsgs; (i>=1) && (i>(nummsgs-28)); i--) {
            if (get_post(i)->status & status_pending_net) {
              dm=0;
              break;
        }
      }
      if (dm) {
        sprintf(s,get_stringx(1,37),subboards[curlsub].name);
        ssm(1,0,s);
      }
    }


    if (nummsgs>=subboards[curlsub].maxmsgs) {
      i=1;
      dm=0;
      while ((dm==0) && (i<=nummsgs)) {
        if ((get_post(i)->status & status_no_delete)==0)
          dm=i;
        ++i;
      }
      if (dm==0)
        dm=1;
      delete(dm);
    }

    add_post(&p);
    lock_status();
    ++status.msgposttoday;
    ++status.localposts;


    save_status();
    close_sub();

    if (xsubs[curlsub].num_nets) {
      if (!(p.status & status_pending_net))
        send_net_post(&p, subboards[curlsub].filename, curlsub);
    }
  }
}


