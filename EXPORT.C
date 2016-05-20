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
/*  File    : EXPORT.C                                                      */
/*                                                                          */
/*  Purpose : Handles Exporting of WWIV Messages to Fidonet Packets         */
/*                                                                          */
/* ------------------------------------------------------------------------ */

#include "vardec.h"
#include "vars.h"
#include "net.h"
#include "fcns.h"

struct  date    work_date;
struct  time    work_time;
unsigned char *translate_letters[] = {
  "abcdefghijklmnopqrstuvwxyz‡„†‚”¤",
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ€Žš™¥",
  0L,
};

/* ------------------------------------------------------------------------ */
/* Checks to see if file (s) exists.  Returns 1 if exists, 0 if not         */
/* ------------------------------------------------------------------------ */

int exist(char *s)
{
  int i;
  struct ffblk ff;

  i=findfirst(s,&ff,FA_HIDDEN);
  if (i)
    return(0);
  else
    return(1);
}

/* ------------------------------------------------------------------------ */
/* Takes outgoing WWIV packet (SXXXX.NET), reads header, and processes msg  */
/*      to go out via Fidonet                                               */
/* ------------------------------------------------------------------------ */



/*  MMH Added below */

char *rtrim( char *tbuffer )
{
	char *stest = 0;

	stest = tbuffer + strlen( tbuffer ) - 1;

	/* while( (stest >= tbuffer) && ((*stest == ' ') || (*stest == '\t') || (*stest == '\r') || (*stest == '\n')) ) */
          while( (stest >= tbuffer) && ((*stest == ' ')))
	{
		*stest = '\0';
		stest--;
	}

	return tbuffer;
}


/* MMH mod end */


void process_outgoing(void)
{
    char s3[181],netfile[80],s1[81],*message,s2[80],temp1[80],*buffer;
    char via_date[80],temp_type[20],log[81],initials[5],ext_tag[81],ext_fn[81];
    unsigned char ch,s[80];
    int i,cycle,count,done,prev,j,k,length,ok3,ok2,dotcount,center,oldnetnum,success,cycle1,success1,flag,override,valid_origin;
    unsigned short ttt;
    net_header_rec temp;
    long this_pos, msgid,end_pos;
    FILE *fn;

    char *month[] = {"Bug","Jan","Feb","Mar","Apr","May","Jun",
                     "Jul","Aug","Sep","Oct","Nov","Dec"};

    FILE *in;
    int handle;
    FIDOADR fadr;
    struct date dt;
    struct time tm;
    time_t t;
    struct tm *gmt;

    export_error=0;
    valid_origin=0;
    msgid=time(NULL);

    curr_out_netnode.net=curr_out_netnode.node=0;
    sprintf(outbound_packet_name,"%s%lx.PKT",cfg.outbound_path,time(NULL));

    oldnetnum=-1;
    for (cycle1=0;cycle1<11;cycle1++)
    {
        if (cfg.aka_list[cycle1].zone!=0)
        {
            strcpy(net_data,net_networks[cfg.aka_list[cycle1].wwiv_netnum].dir);
            net_num=cfg.aka_list[cycle1].wwiv_netnum;
            sprintf(netfile,"%sS%d.NET",net_data,cfg.aka_list[cycle1].wwiv_node);
            if (oldnetnum!=net_num)
            {
                printf("þ Checking for Pending Messages on %s\r\n",net_networks[net_num].name);
                sprintf(log,"þ Checking for Pending Messages (%s) on %s\n",netfile,net_networks[net_num].name);
                write_log(log);
            }
            if (!exist(netfile))
            {
                if (oldnetnum!=net_num)
                {
                    printf("þ No Outgoing Messages for %s\r\n",net_networks[net_num].name);
                    sprintf(log,"þ No Outgoing Messages for %s\n",net_networks[net_num].name);
                    write_log(log);
                }
                oldnetnum=net_num;
            }
            else
            {
                oldnetnum=net_num;
                track[cycle1].wwiv_node=cfg.aka_list[cycle1].wwiv_node;
                track[cycle1].used=1;
                track[cycle1].out_used=1;
                strcpy(track[cycle1].net_name,net_networks[net_num].name);
                handle = sh_open1(netfile,O_RDONLY | O_BINARY);
                track[cycle1].out_k+=filelength(handle)/1000;
                sh_close(handle);
                in = fsh_open(netfile,"rb");
                printf("þ Processing %s to %s\r\n\r\n",netfile,net_networks[net_num].name);
                sprintf(log,"þ Processing %s to %s\n\n",netfile,net_networks[net_num].name);

                write_log(log);
                if (!in)
                {
                    printf("\r\n\7þ Error!  Couldn't open %s for processing!\r\n",netfile);
                    sprintf(log,"\nþ Error!  Couldn't open %s for processing!\n",netfile);
                    write_log(log);
                    return;
                }
                my_zone=my_net=my_node=my_point=0;
                my_zone=cfg.aka_list[cycle1].zone;;
                my_net=cfg.aka_list[cycle1].net;
                my_node=cfg.aka_list[cycle1].node;
                my_point=cfg.aka_list[cycle1].point;

                if (debug)
                    printf(" -----------------------------\r\n");

                fseek(in,0,SEEK_SET);

                while(1)
                {
                    if (fsh_read((void *)&temp, sizeof(net_header_rec), 1, in) == NULL)
                        break;
                    fseek(in, temp.list_len * 2, SEEK_CUR); // seek past list
                    this_pos = ftell(in);
                    fseek(in,temp.length,SEEK_CUR);
                    end_pos = ftell(in);
                    fseek(in,this_pos,SEEK_SET);
                    if (debug)
                    {
                        sprintf(log,"Message type : %u/%u\r\nTo:%u\r\nLength=%ld\r\n",temp.main_type,temp.minor_type,temp.tosys,temp.length);
                        printf("%s",log);
                    }

                    if ( (temp.main_type==5) || (temp.main_type==3) || (temp.main_type==26))  /* Message is a post! */
                    {
                        valid_origin=0;
                        count=temp.length;
                        if (temp.main_type!=26)
                        {
                            sub_type=temp.minor_type;
                            for (cycle=1;cycle<=num_areas;cycle++)
                            {
                                read_area(cycle,&thisarea);
                                ttt=atoi(thisarea.subtype);
                                if (ttt==sub_type)
                                {
                                    curr_area=cycle;
                                    cycle=num_areas;
                                    strcpy(curr_conf_name,thisarea.name);
                                }
                            }
                        } else
                        {
                            done=0;
                            prev=0;
                            strcpy(temp_type,"");
                            while (!done)
                            {
                                ch=getc(in);
                                count--;
                                if (ch!=0)
                                {
                                    sprintf(s,"%c",ch);
                                    strcat(temp_type,s);
                                }
                                if (ch==0)
                                {
                                    done=1;
                                }
                            }
                            for (cycle=1;cycle<=num_areas;cycle++)
                            {
                                read_area(cycle,&thisarea);
                                if (stricmp(temp_type,thisarea.subtype)==0)
                                {
                                    curr_area=cycle;
                                    cycle=num_areas;
                                    strcpy(curr_conf_name,thisarea.name);
                                }
                            }
                        }

                        message= malloc(40*1024);
                        if (!message)
                        {
                            printf("þþ Error: Couldn't Allocate Memory!\7\n\n");
                            write_log("þ Error!  Couldn't Allocate Memory to export message!\n");
                            exit(1);
                        }

                        done=0;
                        prev=0;
                        pass_ascii=thisarea.high_ascii;
                        pass_colors=thisarea.pass_color;
                        strcpy(message,"");
                        while (!done)
                        {
                            ch=getc(in);
                            if ((ch>127) && (!pass_ascii))
                                ch=32;
                            if ((ch==3) && (!pass_colors))
                                prev=1;
                            if (pass_colors)
                                prev=0;
                            if ( ((ch!=3) && (prev!=1)) || (pass_colors))
                            {
                                sprintf(s,"%c",ch);
                                strcat(message,s);
                            }
                            if ( (ch!=3) && (prev))
                                prev=0;
                            count--;
                            if (ch==0)
                            {
                                done=1;
                                for (i=0;i<81;i++)
                                    fido_title[i]=0;
                                strcpy(fido_title,message);
                                strcpy(message,"");
                            }
                        }
                        done=0;
                        while (!done)
                        {
                            ch=getc(in);
                            if ((ch>127) && (!pass_ascii))
                                ch=32;
                            if ( (ch==3) && (!pass_colors))
                                prev=1;
                            if (pass_colors)
                                prev=0;
                            if ( ((ch!=3) && (prev!=1)) || (pass_colors))
                            {
                                sprintf(s,"%c",ch);
                                strcat(message,s);
                            }
                            if ( (ch!=3) && (prev))
                                prev=0;
                            count--;
                            if (ch==13)
                            {
                                done=1;
                                for (i=0;i<81;i++)
                                    fido_from_user[i]=0;
                                strcpy(fido_from_user,message);
                                i=strlen(fido_from_user);
                                if (fido_from_user[i]==13)
                                    fido_from_user[i]=0;
                                if (fido_from_user[i-1]==13)
                                    fido_from_user[i-1]=0;
                                strcpy(message,"");
                                ch=getc(in);
                                count--;
                            }
                        }
                        done=0;
                        while (!done)
                        {
                            ch=getc(in);
                            if ((ch>127) && (!pass_ascii))
                                ch=32;
                            sprintf(s,"%c",ch);
                            if ( (ch==3) && (!pass_colors))
                                prev=1;
                            if (pass_colors)
                                prev=0;
                            if ( ((ch!=3) && (prev!=1)) || (pass_colors))
                            {
                                sprintf(s,"%c",ch);
                                strcat(message,s);
                            }
                            if ( (ch!=3) && (prev))
                                prev=0;
                            count--;
                            if (ch==13)
                            {
                                done=1;
                                for (i=0;i<81;i++)
                                    fido_date_line[i]=0;

                                strcpy(fido_date_line,message);
                                strcpy(message,"");
                                ch=getc(in);
                                count--;
                            }
                        }
                        unixtodos(temp.daten, &dt, &tm);
                        sprintf(s3,"%d",dt.da_year);
                        sprintf(fido_date_line,"%02d %s %c%c  %02d:%02d:%02d",
                            dt.da_day,month[dt.da_mon],s3[2],s3[3],
                            tm.ti_hour,tm.ti_min,tm.ti_sec+(tm.ti_hund+50)/100);
                        strcpy(fido_to_user,"ALL");
                        sprintf(message,"AREA:%s\r",curr_conf_name);
                        if (my_point)
                            sprintf(s3,"MSGID: %d:%d/%d.%d %lX\r",my_zone,my_net,my_node,my_point,msgid++);
                        else
                            sprintf(s3,"MSGID: %d:%d/%d %lX\r",my_zone,my_net,my_node,msgid++);
                        strcat(message,s3);
                        sprintf(s3,"PID: WWIV BBS v. %u \r",status.wwiv_version);
                        strcat(message,s3);
                        sprintf(s3,"TID: WWIVTOSS v. %s \r\r",VERSION);
                        strcat(message,s3);
                        ok3=count;
                        this_pos = ftell(in);
                        for (i=0;i<81;i++)
                            fido_to_user[i]=0;
                        strcpy(fido_to_user,"ALL");
                        while (ok3>0)
                        {
                            fgets(s,80,in);
                            if (cfg.pass_origin)
                            {
                                if (!strnicmp(s,"--- ",4))
                                {
                                    ok3-=strlen(s);
                                    fgets(s,80,in);
                                    if (!strnicmp(s," * Origin:",10))
                                        valid_origin=1;  // should be 1
                                }
                            }
                            if (!strnicmp(s,"0FIDOADDR: ",12))
                            {
                                if (s[12]==' ')
                                    sprintf(fido_to_user,"%s",&s[13]);
                                else
                                    sprintf(fido_to_user,"%s",&s[12]);
                                for (k=0;k<strlen(fido_to_user);k++)
                                {
                                    if (fido_to_user[k]=='(')
                                        fido_to_user[k]=0;
                                    if (fido_to_user[k]=='@')
                                        fido_to_user[k]=0;
                                }
                                for (i=0;i<strlen(fido_to_user);i++)
                                {
                                    if ((fido_to_user[i]==13) || (fido_to_user[i]==10))
                                        fido_to_user[i]=0;
                                }
                                for (i=0;i<strlen(fido_to_user);i++)
                                {
                                    if ((fido_to_user[i]==3))
                                    {
                                        strcpy(&fido_to_user[i],&fido_to_user[i+2]);
                                    }
                                }
                                if (fido_to_user[0]==' ')
                                    strcpy(&fido_to_user[0],&fido_to_user[1]);
                                this_pos = ftell(in);
                                count=ok3;
                                count-=strlen(s);
                            }
                            if ((!strnicmp(s,"BY:",3)) && (stricmp(fido_to_user,"ALL")==0))
                            {
                                if (s[3]==' ')
                                    sprintf(fido_to_user,"%s",&s[4]);
                                else
                                    sprintf(fido_to_user,"%s",&s[3]);
                                for (k=0;k<strlen(fido_to_user);k++)
                                {
                                    if (fido_to_user[k]=='(')
                                        fido_to_user[k]=0;
                                    if (fido_to_user[k]=='@')
                                        fido_to_user[k]=0;
                                }
                                for (i=0;i<strlen(fido_to_user);i++)
                                {
                                    if ((fido_to_user[i]==13) || (fido_to_user[i]==10))
                                        fido_to_user[i]=0;
                                }
                                for (i=0;i<strlen(fido_to_user);i++)
                                {
                                    if ((fido_to_user[i]==3))
                                    {
                                        strcpy(&fido_to_user[i],&fido_to_user[i+2]);
                                    }
                                }
                                if (fido_to_user[0]==' ')
                                    strcpy(&fido_to_user[0],&fido_to_user[1]);
                            }
                            if ((!strnicmp(s,"BY:",3))) //|| (!strnicmp(s,"RE:",3)))
                            {
                                this_pos = ftell(in);
                                count=ok3;
                                count-=strlen(s);
                            }
                            ok3-=strlen(s);
                            if (strlen(s)==0)
                                ok3--;

                        }
                        if ((cfg.initial_quote) && (stricmp(fido_to_user,"ALL")!=0))
                        {
                            sprintf(initials,"%c",fido_to_user[0]);
                            for (i=1;i<strlen(fido_to_user);i++)
                            {
                                if (fido_to_user[i]==' ')
                                {
                                    i++;
                                    sprintf(s,"%c",fido_to_user[i]);
                                    strcat(initials,s);
                                }
                            }
                        }
                        if (strlen(fido_to_user)>35)
                        {
                            sprintf(s3,"REPLYADDR %s\r",fido_to_user);
                            strcat(message,s3);
                            sprintf(s3,"REPLYTO %d:%d/%d.%d WWIVTOSS GATE\r",my_zone,my_net,my_node,my_point);
                            strcat(message,s3);
                        }
                        fseek(in,this_pos,SEEK_SET);
                        center=0;
                        flag=0;
                        while (count>0)
                        {
                            ok2=0;
                            ch=getc(in);
                            if (debug)
                                printf("%c",ch);
                            count--;
/*
                            if (ch=='-')
                            {
                                this_pos=ftell(in);
                                ch=getc(in);
                                if (ch=='-')
                                {
                                    ch=getc(in);
                                    if (ch=='-')
                                    {
                                        fseek(in,this_pos,SEEK_SET);
                                        ch=getc(in);
*/
                            if (ch==4)
                            {
                                this_pos=ftell(in);
                                ch=getc(in);
                                if (ch==48)
                                {
                                    ch=getc(in);
                                    if (ch=='Q')
                                    {
                                        fseek(in,this_pos,SEEK_SET);
                                        ch=getc(in);
                                        ch=getc(in);
                                        ok2=1;
                                    }
                                }
                                if ((thisarea.keep_tag))
                                {
                                    fseek(in,this_pos,SEEK_SET);
                                    ch=getc(in);
                                    if (ch!=48)
                                    {
                                        ok2=1;
                                        ch=getc(in);
                                    }
                                }
                            }
                            else
                                ok2=1;

                            if (ok2)
                            {
                                strcpy(s1,"");
                                strcpy(s2,"");
                                ok3=0;
                                do
                                {
                                    if( (ch==3) && (!pass_colors))
                                    {
                                        ch=getc(in);
                                        ch=getc(in);
                                        count-=2;
                                    } else
                                    if (ch==1)
                                    {
                                        ch=getc(in);
                                        ch=getc(in);
                                        count-=2;
                                        ch=32;
                                        ok3=1;
                                    }
                                    else
                                    if (ch==2)
                                    {
                                        if (cfg.center)
                                        {
                                            strcpy(s1,"");
                                            strcpy(s2,"");
                                            do
                                            {
                                                ch=getc(in);
                                                count--;
                                                sprintf(s2,"%c",ch);
                                                strcat(s1,s2);
                                            } while (ch!=13);
                                            length=(40-(strlen(s1)/2));
                                            sprintf(s2,"");
                                            for (j=0;j<length;j++)
                                                strcat(s2," ");

                                            strcat(s2,s1);
                                            center=1;
                                            strcpy(s1,s2);
                                        }
                                        else
                                        {
                                            ch=getc(in);
                                            count--;
                                        }
                                    } else
                                        ok3=1;
                                } while (!ok3);

                                if ( (ch>127) && (!pass_ascii))
                                    ch=32;
                                /* if ((ch>33) && (thisarea.translate))
                                     ch=xlat[ch-33].out; MMH */
                                if (ch==10)
                                    flag=0;
                                if ((ch!=10) && (!center))
                                {
                                    flag++;
                                    if ((flag==1) && (cfg.initial_quote) && ((ch=='>') || (ch=='¯')))
                                    {
                                        sprintf(s1,"%s> ",initials);
                                    }
                                    else
                                    {
                                        sprintf(s2,"%c",ch);
                                        strcat(s1,s2);
                                    }
                                }
                                if (debug==2)
                                    printf("%s",s1);
                                strcat(message,s1);
                                center=0;
                            }
                            else
                            do
                            {
                                ch=getc(in);
                                count--;
                            } while (ch!=10);
                        }
                        if (!valid_origin)
                        {
                           /* sprintf(s3,"\r--- WWIVToss v.%s %segistered ",VERSION,registered?"R":"Unr"); */
                            sprintf(s3,"\r--- WWIVToss v.%s ",VERSION);  /* MMH mod */
                            strcat(message,s3);
                           /* if (!registered)
                            {
                                sprintf(s3,"[%d days run!]",days_used);
                                if (days_used>45)
                                    strcat(s3,"Remind Me To Register WWIVToss!");
                                strcat(message,s3);
                            } MMH removed mod  */
                            strcat(message,"\r");
                            sprintf(s3,"");
                            if (thisarea.origin[0]=='@')
                            {
                                strcpy(ext_fn,&thisarea.origin[1]);
                                if (exist(ext_fn))
                                {
                                    fn=fsh_open(ext_fn,"rb");
                                    fgets(ext_tag,81,fn);
                                    fsh_close(fn);
                                    strcpy(s3,ext_tag);
                                }
                                else
                                    strcpy(thisarea.origin,cfg.bbs_name);
                            }
                            if (s3[0]==0)
                                sprintf(s3," * Origin:  %s (%d:%d/%d.%d)\r",thisarea.origin,my_zone,my_net,my_node,my_point);
                            strcat(message,s3);
                        }
                        else
                            strcat(message,"\r");
                        sprintf(s3,"SEEN-BY: %d/%d\r",my_net,my_node);
                        strcat(message,s3);
                        sprintf(s3,"PATH: %d/%d\r",my_net,my_node);
                        strcat(message,s3);
                        fromuser=temp.fromuser;
                        if (fromuser)
                            outgoing_mail(1);
                        thisarea.count_out++;
                        if (debug)
                            {
                            printf(" ¯ Title  = %s\r\n",fido_title);
                            printf(" ¯ Sender = %s\r\n",fido_from_user);
                            printf(" ¯ Area   = %s\r\n",curr_conf_name);
                            printf(" ¯ Date   = %s\r\n",fido_date_line);
                            printf(" -----------------------------\r\n");
                        }
                        export_post++;
                        cycle=0;
                        out_zone=out_net=out_node=out_point=who_to.zone=who_to.net=who_to.node=who_to.point=0;
                        for (cycle=0;cycle<MAX_NODES;cycle++)
                        {
                            if ((thisarea.feed[cycle].zone!=0) && (thisarea.feed[cycle].zone!=999))
                            {
                                out_zone=who_to.zone=thisarea.feed[cycle].zone;
                                out_net=who_to.net=thisarea.feed[cycle].net;
                                out_node=who_to.node=thisarea.feed[cycle].node;
                                out_point=who_to.point=thisarea.feed[cycle].point;
                                sprintf(log,"Message Going to %d:%d/%d.%d\n",who_to.zone,who_to.net,who_to.node,who_to.point);
                                write_log(log);
                                attribute=0;
                                send_message(1,message);
                                out_zone=out_net=out_node=out_point=who_to.zone=who_to.net=who_to.node=who_to.point=0;
                            }
                        }
                        free(message);
                        fseek(in,end_pos,SEEK_SET);
                    } else
                    if (temp.main_type==7)  /* Message is email! */
                    {
                        out_zone=out_net=out_node=out_point=who_to.zone=who_to.net=who_to.node=who_to.point=0;
                        pass_colors=pass_ascii=0;
                        pass_ascii=cfg.high_ascii;
                        sub_type=temp.minor_type;
                        fromuser=temp.fromuser;
                        message= malloc(32*1024);
                        buffer = malloc(32*1024);
                        if (!message)
                        {
                            printf("No mem!\7\n\n");
                            exit(1);
                        }
                        count=temp.length;
                        strcpy(message,"");
                        strcpy(buffer,"");
                        done=0;
                        prev=0;
                        while (!done)
                        {
                            ch=getc(in);
                            if ((ch>127) && (!pass_ascii))
                                ch=32;
                            if ((ch==3) && (!pass_colors))
                                prev=1;
                            if (pass_colors)
                                prev=0;
                            if ( ((ch!=3) && (prev!=1)) || (pass_colors))
                            {
                                sprintf(s,"%c",ch);
                                strcat(message,s);
                            }
                            if ( (ch!=3) && (prev))
                                prev=0;
                            count--;
                            if (ch==0)
                            {
                                done=1;
                                strcpy(fido_to_user,message);
                                strcpy(message,"");
                                strcpy(s3,"");
                                strcpy(temp1,"");
                                if (fido_to_user[0]!=0)
                                {
                                    for (i=0;i<strlen(fido_to_user);i++)
                                    {
                                        if (fido_to_user[i]!='(')
                                        {
                                            sprintf(message,"%c",fido_to_user[i]);
                                            strcat(s3,message);
                                        }
                                        else
                                        {
                                            for (j=i;j<strlen(fido_to_user);j++)
                                            {
                                                sprintf(message,"%c",fido_to_user[j]);
                                                strcat(temp1,message);
                                            }
                                            temp1[0]=32;
                                            temp1[strlen(temp1)-1]=0;
                                            i=234;
                                        }
                                    }
                                    strcpy(fido_to_user,s3);
                                    fido_to_user[strlen(fido_to_user)-1]=0;
                                    for (i=length;i>0;i--)
                                        if ((fido_to_user[i]==20) || (fido_to_user[i]==13))
                                        {
                                            fido_to_user[i]=0;
                                        }
                                        else
                                            i=0;

                                    fidoadr_split(temp1,&fadr);
                                    out_zone=fadr.zone;
                                    out_net=fadr.net;
                                    out_node=fadr.node;
                                    out_point=fadr.point;
                                }
                            }
                        }
                        temp_crash=0;
                        temp_crash=cfg.crash;
                        override=0;
                        if (fido_to_user[0]=='*')
                        {
                            if (fromuser==1)
                            {
                                cfg.crash=2;
                                override=1;
                            }
                            strcpy(&fido_to_user[0],&fido_to_user[1]);
                        }
                        strcpy(message,"");
                        done=0;
                        prev=0;
                        while (!done)
                        {
                            ch=getc(in);
                            sprintf(s,"%c",ch);
                            strcat(buffer,s);
                            if ((ch>127) && (!pass_ascii))
                                ch=32;
                            if ((ch==3) && (!pass_colors))
                                prev=1;
                            if (pass_colors)
                                prev=0;
                            if ( ((ch!=3) && (prev!=1)) || (pass_colors))
                            {
                                sprintf(s,"%c",ch);
                                strcat(message,s);
                            }
                            if ((ch!=3) && (prev))
                                prev=0;
                            count--;
                            if (ch==0)
                            {
                                done=1;
                                strcpy(fido_title,message);
                                strcpy(message,"");
                            }
                        }
                        done=0;
                        while (!done)
                        {
                            ch=getc(in);
                            if ((ch>127) && (!pass_ascii))
                                ch=32;
                            if ( (ch==3) && (!pass_colors))
                                prev=1;
                            if (pass_colors)
                                prev=0;
                            if ( ((ch!=3) && (prev!=1)) || (pass_colors))
                            {
                                sprintf(s,"%c",ch);
                                strcat(message,s);
                            }
                            if ( (ch!=3) && (prev))
                                prev=0;
                            count--;
                            if (ch==13)
                            {
                                done=1;
                                strcpy(fido_from_user,message);
                                strcpy(message,"");
                                ch=getc(in);
                                count--;
                            }
                        }
                        length=strlen(fido_from_user);
                        for (i=length;i>0;i--)
                            if ((fido_from_user[i]==20) || (fido_from_user[i]==13) || (fido_from_user[i]==0))
                            {
                                fido_from_user[i]=0;
                            }
                            else
                                i=0;
                        done=0;

                        while (!done)
                        {
                            ch=getc(in);
                            if ((ch>127) && (!pass_ascii))
                                ch=32;
                            sprintf(s,"%c",ch);
                            if ( (ch==3) && (!pass_colors))
                                prev=1;
                            if (pass_colors)
                                prev=0;
                            if ( ((ch!=3) && (prev!=1)) || (pass_colors))
                            {
                                sprintf(s,"%c",ch);
                                strcat(message,s);
                            }
                            if ( (ch!=3) && (prev))
                                prev=0;
                            count--;
                            if (ch==13)
                            {
                                done=1;
                                strcpy(fido_date_line,message);
                                strcpy(message,"");
                                ch=getc(in);
                                count--;
                            }
                        }
                        unixtodos(temp.daten, &dt, &tm);
                        sprintf(s3,"%d",dt.da_year);
                        sprintf(fido_date_line,"%02d %s %c%c  %02d:%02d:%02d",
                            dt.da_day,month[dt.da_mon],s3[2],s3[3],
                            tm.ti_hour,tm.ti_min,tm.ti_sec+(tm.ti_hund+50)/100);
                        if (my_point)
                            sprintf(s3,"MSGID: %d:%d/%d.%d %lX\r",my_zone,my_net,my_node,my_point,msgid++);
                        else
                            sprintf(s3,"MSGID: %d:%d/%d %lX\r",my_zone,my_net,my_node,msgid++);
                        strcat(message,s3);
                        sprintf(s3,"PID: WWIV BBS v. %u \r",status.wwiv_version);
                        strcat(message,s3);
                        sprintf(s3,"TID: WWIVTOSS v. %s \r",VERSION);
                        strcat(message,s3);
                        sprintf(s3,"FLAGS ");
                        if (cfg.direct)
                        {
                            switch(cfg.direct)
                            {
                                case 1:
                                    if (my_net==out_net)
                                    {
                                        strcat(s3,"DIR ");
                                    }
                                    break;
                                case 2:
                                    strcat(s3,"DIR ");
                                    break;
                                default:
                                    break;
                            }
                        }
                        if (cfg.set_immediate)
                            strcat(s3,"IMM ");
                        if ((cfg.direct) || (cfg.crash))
                        {
                            strcat(s3,"\r");
                            strcat(message,s3);
                        }
   
                     /* Updated by MMH */
                     /* if (out_zone!=my_zone) */
                        /* { */
                            /* sprintf(s3,"INTL %d:%d/%d.%d %d:%d/%d.%d\r",out_zone,out_net,out_node,out_point,my_zone,my_net,my_node,my_point);  */
							sprintf(s3,"INTL %d:%d/%d %d:%d/%d\r",out_zone,out_net,out_node,my_zone,my_net,my_node);
                            strcat(message,s3);
                       /* } */
                      /* End of update */ 
                     
					   if (out_point)
                        {
                            sprintf(s3,"TOPT %d\r",out_point);
                            strcat(message,s3);
                        }
                        if (my_point)
                        {
                            sprintf(s3,"FMPT %d\r",my_point);
                            strcat(message,s3);
                        }
					 					  
                        center=0;
                        while (count>0)
                        {
                            ch=getc(in);
                            sprintf(s,"%c",ch);
                            strcat(buffer,s);
                            count--;
                            if (ch!=4)
                            {
                                strcpy(s1,"");
                                strcpy(s2,"");
                                ok3=0;
                                do
                                {
                                    if( (ch==3) && (!pass_colors))
                                    {
                                        ch=getc(in);
                                        ch=getc(in);
                                        count-=2;
                                    } else
                                    if (ch==1)
                                    {
                                        ch=getc(in);
                                        count--;
                                    }
                                    else
                                    if (ch==2)
                                    {
                                        if (cfg.center)
                                        {
                                            strcpy(s1,"");
                                            strcpy(s2,"");
                                            do
                                            {
                                                ch=getc(in);
                                                count--;
                                                sprintf(s2,"%c",ch);
                                                strcat(s1,s2);
                                            } while (ch!=13);
                                            length=(40-(strlen(s1)/2));
                                            sprintf(s2,"");
                                            for (j=0;j<length;j++)
                                                strcat(s2," ");

                                            strcat(s2,s1);
                                            center=1;
                                            strcpy(s1,s2);
                                        }
                                        else
                                        {
                                            ch=getc(in);
                                            count--;
                                        }
                                    } else
                                    ok3=1;
                                } while (!ok3);

                                    if ( (ch>127) && (!pass_ascii))
                                        ch=32;
                                    if ((ch!=10) && (!center))
                                    {
                                        sprintf(s2,"%c",ch);
                                        strcat(s1,s2);
                                    }

                                if( (strnicmp(s1,"RE:",3)) ||  (strnicmp(s1,"BY:",3)))
                                    strcat(message,s1);
                                center=0;
                                }
                                else
                                {
                                do
                                {
                                    ch=getc(in);
                                    count--;
                                } while (ch!=10);
                            }
                        }
                        if ((fido_from_user[0]=='`') && (fido_from_user[1]=='`'))
                        {
                            sprintf(s3,"\r [ Gated via %s %s at (%d:%d/%d.%d) ]",PROGRAM,VERSION,my_zone,my_net,my_node,my_point);
                            strcat(message,s3);
                        }
                        /* sprintf(s3,"\r--- WWIVToss v.%s %segistered ",VERSION,registered?"R":"Unr"); */
                           sprintf(s3,"\r--- WWIVToss v.%s ",VERSION);  /* MMH mod */
                        strcat(message,s3);
                       /* if (!registered)
                        {
                            sprintf(s3,"[%d days run!]",days_used);
                            strcat(message,s3);
                            if (days_used>45)
                                strcat(s3,"Remind Me To Register WWIVToss!");
                            strcat(message,s3);
                        }  MMH mod removed */
                        strcat(message,"\r");
                        sprintf(s3,"");
                        if (cfg.origin_line[0]=='@')
                        {
                            strcpy(ext_fn,&cfg.origin_line[1]);
                            if (exist(ext_fn))
                            {
//                                printf("Using External Origin Line from %d\r\n",ext_fn);
                                fn=fsh_open(ext_fn,"rb");
                                fgets(ext_tag,81,fn);
                                fsh_close(fn);
                                strcpy(s3,ext_tag);
                            }
                        }
                        else
                            strcpy(cfg.origin_line,cfg.bbs_name);
                        if (s3[0]==0)
                            sprintf(s3," * Origin:  %s (%d:%d/%d.%d)\r",cfg.origin_line,my_zone,my_net,my_node,my_point);
                        strcat(message,s3);
                        fromuser=temp.fromuser;
                        if (fromuser)
                            outgoing_mail(2);
                        if (debug)
                        {
                            printf(" ¯ Title  = %s\r\n",fido_title);
                            printf(" ¯ To     = %s\r\n",fido_to_user);
                            printf(" ¯ Sender = %s\r\n",fido_from_user);
                            printf(" -----------------------------\r\n");
                        }
                        sprintf(log,"Outgoing Mail to %s (%d:%d/%d.%d) from %s\n",fido_to_user,out_zone,out_net,out_node,out_point,fido_from_user);
                        write_log(log);
                        if (!out_zone)
                            out_zone=my_zone;
                        if (out_zone)
                            export_mail++;
                        else
                        {
                            bad_mail++;
                            sprintf(s3,"þ Invalid address on outgoing mail from %u@%u\n",temp.fromuser,temp.fromsys);
                            write_log(s3);
                            return_message(temp,buffer);
                        }
                        t = time(NULL);
                        gmt = gmtime(&t);
                        sprintf(via_date,"%s",asctime(gmt));
                        i=strlen(via_date);
                        via_date[i-1]=13;
                        success1=0;
                        for (cycle=0;cycle<MAX_NODES;cycle++)
                        {
                            /* Here ! */
                            sprintf(s3,"%d:%d/%d.%d",out_zone,out_net,out_node,out_point);
                            if (stricmp(s3,nodes[cycle].address)==0)
                            {
                                cfg.crash=nodes[cycle].archive_status;
                                success1=1;
                                cycle=MAX_NODES;
                            }

                        }
                        if (!cfg.route_me)
                            success1=1;
                        if (override)
                            success1=1;
                        if (success1)
                        {
                            who_to.zone=out_zone;
                            who_to.net=out_net;
                            who_to.node=out_node;
                            who_to.point=out_point;
                            attribute=0;
                            attribute= attribute | (MSGPRIVATE) | (MSGLOCAL);
                            if ((!cfg.route_me) || (override))
                                switch (cfg.crash)
                                {
                                    case 0:
                                        break;
                                    case 1:
                                        attribute = attribute | (MSGHOLD);
                                        break;
                                    case 2:
                                        attribute = attribute | (MSGCRASH);
                                        break;
                                    default:
                                        attribute = attribute | (MSGHOLD);
                                        break;
                                }
                            send_message(6,message);
                        }
                        else
                        {
                            switch (cfg.crash)
                            {
                                case 0:
                                    success=0;
                                    for (cycle=0;cycle<11;cycle++)
                                    {
                                        if (out_zone==cfg.route_to[cycle].route_zone)
                                        {
                                            success=1;
                                            break;
                                        }
                                    }
                                    if (success)
                                    {
                                        who_route.zone=cfg.route_to[cycle].zone;
                                        who_route.net=cfg.route_to[cycle].net;
                                        who_route.node=cfg.route_to[cycle].node;
                                        who_route.point=cfg.route_to[cycle].point;
//                                        sprintf(log,"1 Mail Going to %d:%d/%d.%d\n",who_route.zone,who_route.net,who_route.node,who_route.point);
//                                        write_log(log);

                                        sprintf(s3,"Via WWIVTOSS %s %d:%d/%d.%d, %s\r",VERSION,my_zone,my_net,my_node,my_point,via_date);
                                        strcat(message,s3);
                                        attribute=0;
                                        attribute=attribute | (MSGLOCAL) | (MSGPRIVATE) | (MSGKILL);
                                    }
                                    else
                                    {
                                        who_route.zone=who_to.zone=out_zone;
                                        who_route.net=who_to.net=out_net;
                                        who_route.node=who_to.node=out_node;
                                        who_route.point=who_to.point=out_point;
                                    }
                                    if (out_zone==0)
                                    {
                                        out_zone=my_zone;
                                        attribute=attribute | (MSGHOLD);
                                        send_message(2,message);
                                    }
                                    else
                                        send_message(3,message);
                                    break;

                                case 1:
                                    if (my_net==out_net)
                                    {
                                        who_to.zone=out_zone;
                                        who_to.net=out_net;
                                        who_to.node=out_node;
                                        who_to.point=out_point;
//                                        sprintf(log,"2 Mail Going to %d:%d/%d.%d\n",who_to.zone,who_to.net,who_to.node,who_to.point);
//                                        write_log(log);
                                        send_message(2,message);
                                    }
                                    else
                                    {
                                        success=0;
//                                        sprintf(log,"Out to %d:%d/%d.%d\n",out_zone,out_net,out_node,out_point);
//                                        write_log(log);
                                        for (cycle=0;cycle<11;cycle++)
                                        {
                                            if (out_zone==cfg.route_to[cycle].route_zone)
                                            {
                                                success=1;
                                                break;
                                            }
                                        }
                                        if (success)
                                        {
//                                            sprintf(log,"Out to %d:%d/%d.%d\n",cfg.route_to[cycle].zone,cfg.route_to[cycle].net,cfg.route_to[cycle].node,cfg.route_to[cycle].point);
//                                            write_log(log);
                                            who_to.zone=who_route.zone=cfg.route_to[cycle].zone;
                                            who_to.net=who_route.net=cfg.route_to[cycle].net;
                                            who_to.node=who_route.node=cfg.route_to[cycle].node;
                                            who_to.point=who_route.point=cfg.route_to[cycle].point;
//                                            sprintf(log,"3 Mail Going to %d:%d/%d.%d\n",who_route.zone,who_route.net,who_route.node,who_route.point);
//                                            write_log(log);
                                            sprintf(s3,"Via WWIVTOSS %s %d:%d/%d.%d, %s",VERSION,my_zone,my_net,my_node,my_point,via_date);
                                            strcat(message,s3);
                                            attribute=0;
                                            attribute=attribute | (MSGLOCAL) | (MSGPRIVATE);
                                            send_message(3,message);
                                        }
                                        else
                                        {
                                            who_route.zone=who_to.zone;
                                            who_route.net=who_to.net;
                                            who_route.node=who_to.node;
                                            who_route.point=who_to.point;
                                            send_message(3,message);
                                        }
                                    }
                                    break;

                               case 2:
                                    who_to.zone=out_zone;
                                    who_to.net=out_net;
                                    who_to.node=out_node;
                                    who_to.point=out_point;
//                                    sprintf(log,"4 Mail Going to %d:%d/%d.%d\n",who_to.zone,who_to.net,who_to.node,who_to.point);
//                                    write_log(log);

                                    send_message(2,message);
                                    break;

                            }
                        }
                        free(message);
                        free(buffer);
                        cfg.crash=temp_crash;
                        fseek(in,end_pos,SEEK_SET);
                    }
                    else
                    {
                        fseek(in, temp.length, SEEK_CUR); /* dunno - skip it */
                    }
                }
                fsh_close(in);
                printf("þ Processing on %s finished!\r\n\r\n",net_networks[net_num].name);
                unlink(netfile);
                outchr(00);
                outchr(00);             /* message-type == 0000 (hex) terminates packet */
                sh_close(outbound_file);
                if (exist(outbound_packet_name))
                {
                    unlink(netfile);
                }
            }
        }
    }
    for (dotcount=0;dotcount<=num_areas;dotcount++)
    {
	read_area(dotcount,&thisarea);
        if (thisarea.count_out)
        {
            sprintf(log,"     Area [%-15s] Exported %-4d Messages\n",thisarea.name,thisarea.count_out);
            if (debug)
                printf("%s\r\n",log);
            write_log(log);
        }
    }
}

/* ------------------------------------------------------------------------ */
/* Takes processed message and actually creates outgoing fidonet packet.    */
/*      Mode determines type of packet.                                     */
/*          1 = OutGoing Message & Routed Email                             */
/*          2 = Crash Netmail                                               */
/*          3 = Routed Netmail                                              */
/*          4 = Bad Message                                                 */
/*          5 = File Attach for Outbound Packets                            */
/*          6 = Normal Mail going to a node                                 */
/* ------------------------------------------------------------------------ */

void send_message(int mode,char *message)
{
    char fn[80];
    int i,f;

    properize(fido_to_user);  
	/*  MMH Mod above to remove upper case Usernames/Names */
    for (i=strlen(fido_from_user);i<sizeof(fido_from_user);i++)
    {
               fido_from_user[i]='\0';

    }

/* MMH mod begin */

 

fido_to_user==rtrim(fido_to_user);
          
     
/* MMH mod - fix fido_from_user from being all upper case */

f=1;
for (i=0;i<strlen(fido_from_user);i++)
  if (f) {
      if ((fido_from_user[i] >= 'A') && (fido_from_user[i] <= 'Z'))
         f=0;
     } else {
      if ((fido_from_user[i] >= 'A') && (fido_from_user[i] <= 'Z'))
         fido_from_user[i] = fido_from_user[i] - 'A' + 'a';
       else {
         if ((fido_from_user[i] >= ' ') && (fido_from_user[i] <= '/'))
         f=1;
     }
    }



          
/* MMH mod end */
                  
    for (i=strlen(fido_to_user);i<sizeof(fido_to_user);i++)
    {
	       fido_to_user[i]='\0';
               /* fido_to_user[i]=0;   MMH mod */
    }
    for (i=strlen(fido_title);i<sizeof(fido_title);i++)
    {
	      fido_title[i]='\0'; 
              
    }
    if (strlen(fido_from_user)>35)
        fido_from_user[35]=0;
    if (strlen(fido_to_user)>35)
        fido_to_user[35]=0;
    if (strlen(fido_title)>71)
        fido_title[71]=0;
    if ((cfg.mailer==2) && ( (mode==2) || (mode==3) || (mode==6)))
    {
        switch(mode)
        {
            case 1:
                break;
            case 2:
                attribute = 0;
                attribute = attribute | (MSGPRIVATE) | (MSGCRASH) | (MSGLOCAL);
                break;
            case 3:
                who_to=who_route;
                break;
            case 6:
                break;
        }
        mode=1;
    }
    if (mode==1)
    {
        sprintf(fn,"%04X%04X.QQQ",out_net,out_node);
        open_outbound_packet(fn,who_to,pktt_type);
        message_header.id=0x0002;
        message_header.orig_node=my_node;
        message_header.dest_node=out_node;
        message_header.orig_net=my_net;
        message_header.dest_net=out_net;
        message_header.attribute=attribute;
        message_header.cost=0;
        strcpy(message_header.datetime,fido_date_line);
        sh_write(outbound_file,(void *)&message_header.id,14);
        sh_write(outbound_file,(void *)&message_header.datetime,(strlen(message_header.datetime)+1));
        sh_write(outbound_file,(void *)&fido_to_user,(strlen(fido_to_user)+1));  /* MMH had changed */
        sh_write(outbound_file,(void *)&fido_from_user,(strlen(fido_from_user)+1));
        sh_write(outbound_file,(void *)&fido_title,(strlen(fido_title)+1));
        sh_write(outbound_file,message,(strlen(message)+1));
    } else if (mode==2)
    {
        if (outbound_file>0)
        {
            outchr(00);
            outchr(00);
            curr_out_netnode.net=curr_out_netnode.node=0;
            sh_close(outbound_file);
        }
        strcpy(fdmsg.FromUser,fido_from_user);
        strcpy(fdmsg.ToUser,fido_to_user);
        strcpy(fdmsg.Subject,fido_title);
        strcpy(fdmsg.Date,fido_date_line);
        fdmsg.TimesRead=0;
        fdmsg.DestNode=out_node;
        fdmsg.OrigNode=my_node;
        fdmsg.Cost=0;
        fdmsg.OrigNet=my_net;
        fdmsg.DestNet=out_net;
        fdmsg.DateWritten=0;
        fdmsg.DateArrived=0;
        fdmsg.ReplyTo=0;
        fdmsg.Attr=0;
        fdmsg.Attr = fdmsg.Attr | (MSGPRIVATE) | (MSGCRASH) | (MSGLOCAL);
        fdmsg.ReplyNext=0;
        sprintf(fn,"%s%d.MSG",cfg.netmail_path,findnextmsg(1));
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
                printf("error opening outbound file '%s'. errno=%d.2\n",outbound_packet_name,errno);
                exit_status=3;
                return;
            }
        }
        sh_write(outbound_file,(void *)&fdmsg,sizeof(FIDOMSG));
        sh_write(outbound_file,message,(strlen(message)+1));
        sh_close(outbound_file);

    } else
    if (mode==3)
    {
        sprintf(fn,"%04X%04X.QQQ",out_net,out_node);
        who_to=who_route;
        open_outbound_packet(fn,who_to,pktt_type);
        message_header.id=0x0002;
        message_header.orig_node=my_node;
        message_header.dest_node=out_node;
        message_header.orig_net=my_net;
        message_header.dest_net=out_net;
        message_header.attribute=attribute;
        message_header.cost=0;
        strcpy(message_header.datetime,fido_date_line);
        sh_write(outbound_file,(void *)&message_header.id,14);
        sh_write(outbound_file,(void *)&message_header.datetime,(strlen(message_header.datetime)+1));
        sh_write(outbound_file,(void *)&fido_to_user,(strlen(fido_to_user)+1));
        sh_write(outbound_file,(void *)&fido_from_user,(strlen(fido_from_user)+1));
        sh_write(outbound_file,(void *)&fido_title,(strlen(fido_title)+1));
        sh_write(outbound_file,message,(strlen(message)+1));
    } else if (mode==4)
    {
        if (outbound_file>0)
        {
            outchr(00);
            outchr(00);
            sh_close(outbound_file);
            curr_out_netnode.net=curr_out_netnode.node=0;
        }
        strcpy(fdmsg.FromUser,fido_from_user);
        strcpy(fdmsg.ToUser,fido_to_user);
        strcpy(fdmsg.Subject,fido_title);
        strcpy(fdmsg.Date,fido_date_line);
        fdmsg.TimesRead=0;
        fdmsg.DestNode=my_node;
        fdmsg.OrigNode=out_node;
        fdmsg.Cost=0;
        fdmsg.OrigNet=out_net;
        fdmsg.DestNet=my_net;
        fdmsg.DateWritten=0;
        fdmsg.DateArrived=0;
        fdmsg.ReplyTo=0;
        fdmsg.Attr = 0;
        fdmsg.Attr = fdmsg.Attr | (MSGCRASH) | (MSGLOCAL);
        fdmsg.ReplyNext=0;
        sprintf(fn,"%s%d.MSG",cfg.badecho_path,findnextmsg(1));
        outbound_file=-1;
        outbound_file=sh_open1(fn,O_RDWR | O_BINARY);
        if (outbound_file<0) {
        /*  --------------------------------------------------------------
         *  The file doesn't already exist - make one and write the header
         *  --------------------------------------------------------------  */
            outbound_file=sh_open(fn,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
            if (outbound_file<0) {
                printf("error opening outbound file '%s'. errno=%d.3\n",outbound_packet_name,errno);
                exit_status=3;
                return;
            }
        }
        sh_write(outbound_file,(void *)&fdmsg,sizeof(FIDOMSG));
        sh_write(outbound_file,message,(strlen(message)+1));
        sh_close(outbound_file);
    } else if (mode==5)
    {
        if (outbound_file>0)
        {
            outchr(00);
            outchr(00);
            sh_close(outbound_file);
            curr_out_netnode.net=curr_out_netnode.node=0;
        }
        strcpy(fdmsg.FromUser,fido_from_user);
        strcpy(fdmsg.ToUser,fido_to_user);
        strcpy(fdmsg.Subject,fido_title);
        strcpy(fdmsg.Date,fido_date_line);
        fdmsg.TimesRead=0;
        fdmsg.DestNode=out_node;
        fdmsg.OrigNode=my_node;
        fdmsg.Cost=0;
        fdmsg.OrigNet=my_net;
        fdmsg.DestNet=out_net;
        fdmsg.DateWritten=0;
        fdmsg.DateArrived=0;
        fdmsg.ReplyTo=0;
        fdmsg.Attr=attribute;
        fdmsg.Attr = fdmsg.Attr | (MSGPRIVATE) ;
        fdmsg.ReplyNext=0;
        sprintf(fn,"%s%d.MSG",cfg.netmail_path,findnextmsg(1));
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
                printf("error opening outbound file '%s'. errno=%d.4\n",outbound_packet_name,errno);
                exit_status=3;
                return;
            }
        }
        sh_write(outbound_file,(void *)&fdmsg,sizeof(FIDOMSG));
        sh_write(outbound_file,message,(strlen(message)+1));
        sh_close(outbound_file);
    } else if (mode==6)
    {
        if (outbound_file>0)
        {
            outchr(00);
            outchr(00);
            curr_out_netnode.net=curr_out_netnode.node=0;
            sh_close(outbound_file);
        }
        strcpy(fdmsg.FromUser,fido_from_user);
        strcpy(fdmsg.ToUser,fido_to_user);
        strcpy(fdmsg.Subject,fido_title);
        strcpy(fdmsg.Date,fido_date_line);
        fdmsg.TimesRead=0;
        fdmsg.DestNode=out_node;
        fdmsg.OrigNode=my_node;
        fdmsg.Cost=0;
        fdmsg.OrigNet=my_net;
        fdmsg.DestNet=out_net;
        fdmsg.DateWritten=0;
        fdmsg.DateArrived=0;
        fdmsg.ReplyTo=0;
        fdmsg.Attr=0;
        fdmsg.Attr = attribute;
        fdmsg.ReplyNext=0;
        sprintf(fn,"%s%d.MSG",cfg.netmail_path,findnextmsg(1));
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
                printf("error opening outbound file '%s'. errno=%d.5\n",outbound_packet_name,errno);
                exit_status=3;
                return;
            }
        }
        sh_write(outbound_file,(void *)&fdmsg,sizeof(FIDOMSG));
        sh_write(outbound_file,message,(strlen(message)+1));
        sh_close(outbound_file);

    }
}


void open_outbound_packet(char *filename,netnode who_to,int pktt_type)
{
    char fn[65],s3[80],pass[80];
    int cycle;
//    char dm[80];


    if (outbound_file>0)
    {
        if ((curr_out_netnode.net == who_to.net)
        &&  (curr_out_netnode.node == who_to.node))
            return;
        outchr(00);
        outchr(00);
        sh_close(outbound_file);
    }
    strcpy(pass,"");
    for (cycle=0;cycle<MAX_NODES;cycle++)
    {
        sprintf(s3,"%d:%d/%d.%d",who_to.zone,who_to.net,who_to.node,who_to.point);
        if (stricmp(s3,nodes[cycle].address)==0)
        {
            strcpy(pass,nodes[cycle].pkt_pw);
            switch (nodes[cycle].packet_type)
            {
                case 0:
                    pktt_type=0;
                    break;
                case 1:
                    pktt_type=1;
                    break;
                case 2:
                    pktt_type=2;
                    break;
                case 3:
                    pktt_type=3;
                    break;
                default:
                    pktt_type=0;
		    break;
            }
            cycle=MAX_NODES;
        }

    }

    strcpy(fn,filename);
    sprintf(outbound_packet_name,"%s%s",cfg.outbound_temp_path,fn);
    if (testing)
        printf("Opening outbound file: %s\n",fn);
    outbound_file=-1;
    outbound_file=sh_open1(outbound_packet_name,O_RDWR | O_BINARY);
    if (outbound_file<0)
    {
        outbound_file=sh_open(outbound_packet_name,O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
        if (outbound_file<0)
        {
            printf("error opening outbound file '%s'. errno=%d.6\n [%s]",outbound_packet_name,errno,sys_errlist[errno]);
            exit_status=3;
            return;
        }
        getdate(&work_date);
        gettime(&work_time);
//	sprintf(dm,"%d",work_date.da_year);
//	sprintf(s,"%c%c",dm[2],dm[3]);
//        yr=atoi(s);
        switch (pktt_type)
        {
            case 0: // Stoneage 2.0
                packet_header.orig_node     = my_node;
                packet_header.dest_node     = who_to.node;
                packet_header.year          = work_date.da_year;
                packet_header.month         = work_date.da_mon;
                packet_header.day           = work_date.da_day;
                packet_header.hour          = work_time.ti_hour;
                packet_header.minute        = work_time.ti_min;
                packet_header.second        = work_time.ti_sec;
                packet_header.baud          = 0;
                packet_header.packet_type   = 0x0002;
                packet_header.orig_net      = my_net;
                packet_header.dest_net      = who_to.net;
                packet_header.PCodeLo       = 254;
                packet_header.PRevMajor     = VERSION1;
                strcpy(packet_header.Password,pass);
                packet_header.QMOrigZone    = 1;
                packet_header.QMDestZone    = 1;
                strcpy(packet_header.domain,"");  //change
                packet_header.OrigZone      = my_zone;
                packet_header.DestZone      = who_to.zone;
                packet_header.OrigPoint     = my_point;
                packet_header.DestPoint     = who_to.point;
                packet_header.LongData      = 0;
                sh_write(outbound_file,(void *)&packet_header,sizeof(packet_header_struct));
                break;
            case 1: // Packet Type 2.+ (FSC-0039)

                FSC39_header.orig_node      = my_node;
                FSC39_header.dest_node      = who_to.node;
                FSC39_header.year           = work_date.da_year;
                FSC39_header.month          = work_date.da_mon;
                FSC39_header.day            = work_date.da_day;
                FSC39_header.hour           = work_time.ti_hour;
                FSC39_header.minute         = work_time.ti_min;
                FSC39_header.second         = work_time.ti_sec;
                FSC39_header.rate           = 0;
                FSC39_header.ver            = 0x0002;
                FSC39_header.orig_net       = my_net;
                FSC39_header.dest_net       = who_to.net;
                FSC39_header.product        = 254;
                FSC39_header.rev_lev        = VERSION1;
                strcpy(FSC39_header.password,pass);
                FSC39_header.qm_orig_zone   = my_zone;
                FSC39_header.qm_dest_zone   = who_to.zone;
                strcpy(FSC39_header.filler,"A");
                FSC39_header.product2       = 0;
                FSC39_header.rev_lev2       = VERSION2;
                FSC39_header.capword        = 2;
                FSC39_header.capword2       = ~(FSC39_header.capword);
                FSC39_header.orig_zone      = my_zone;
                FSC39_header.dest_zone      = who_to.zone;
                FSC39_header.orig_point     = my_point;
                FSC39_header.dest_point     = who_to.point;
                FSC39_header.pr_data        = 0;
                sh_write(outbound_file,(void *)&FSC39_header,sizeof(FSC39_header_struct));
                break;
            case 2: // Packet Type 2.N (FSC-0048)
                FSC48_header.orig_node      = my_node;
                FSC48_header.dest_node      = who_to.node;
                FSC48_header.year           = work_date.da_year;
                FSC48_header.month          = work_date.da_mon;
                FSC48_header.day            = work_date.da_day;
                FSC48_header.hour           = work_time.ti_hour;
                FSC48_header.minute         = work_time.ti_min;
                FSC48_header.second         = work_time.ti_sec;
                FSC48_header.rate           = 0;
                FSC48_header.ver            = 0x0002;
                FSC48_header.orig_net       = my_net;
                FSC48_header.dest_net       = who_to.net;
                FSC48_header.product        = 254;
                FSC48_header.rev_lev        = VERSION1;
                strcpy(FSC48_header.password,pass);
                FSC48_header.qm_orig_zone   = who_to.zone;
                FSC48_header.qm_dest_zone   = my_zone;
                FSC48_header.aux_net        = 1;
                FSC48_header.product2       = 0;
                FSC48_header.rev_lev2       = VERSION2;
                FSC48_header.capword        = 2;
                FSC48_header.capword2       = FSC48_header.capword;
                FSC48_header.orig_zone      = my_zone;
                FSC48_header.dest_zone      = who_to.zone;
                FSC48_header.orig_point     = my_point;
                FSC48_header.dest_point     = who_to.point;
                FSC48_header.pr_data        = 0;
                sh_write(outbound_file,(void *)&FSC48_header,sizeof(FSC48_header_struct));
                break;
            case 3: // Packet Type 2.2 (FSC-0045)
                FSC45_header.onode          = my_node;
                FSC45_header.dnode          = who_to.node;
                FSC45_header.opoint         = my_point;
                FSC45_header.dpoint         = who_to.point;
                strcpy(FSC45_header.zeros,"0000");
                FSC45_header.subver         = 2;
                FSC45_header.version        = 2;
                FSC45_header.onet           = my_net;
                FSC45_header.dnet           = who_to.net;
                FSC45_header.product        = 254;
                FSC45_header.rev_lev        = VERSION1;
                strcpy(FSC45_header.password,pass);
                FSC45_header.ozone          = 1;
                FSC45_header.dzone          = 1;
                strcpy(FSC45_header.odomain,"");
                strcpy(FSC45_header.ddomain,""); //change!
                FSC45_header.specific       = 0;
                sh_write(outbound_file,(void *)&FSC45_header,sizeof(FSC45_header_struct));
                break;
        }
    } else
    {
        lseek(outbound_file,-2L,SEEK_END);
    }
    curr_out_netnode.net = who_to.net;
    curr_out_netnode.node = who_to.node;
}

void invent_pkt_name (char * string)
{
    struct time t;

    struct tm *tp;
    time_t ltime;
    gettime(&t);

    time (&ltime);
    tp = localtime (&ltime);
    sprintf (string, "%02d%02d%02d%02d",
        tp->tm_mday, t.ti_hour,t.ti_min,t.ti_hund);
}

void outchr(unsigned char ch)
{
    sh_write(outbound_file,(void *)&ch,1);
    if (testing)
    {
        _AL=ch;
        _AH=0x0e;
        _BL=0x07;
        geninterrupt(0x10);
    }
}

/* Find Next free message number in FIDOnet folder */
int findnextmsg(int mode)
{
    int rc=0;       /* Return code */
    int t;          /* Possible return value */
    int done;       /* Loop Control */
    char scratch[80];
    struct ffblk ffblk; /* Control Block */
    if (mode==1)
        sprintf(scratch,"%s\\*.MSG",cfg.netmail_path);
    else
    if (mode==2)
        sprintf(scratch,"%s\\*.MSG",cfg.badecho_path);
    else
    if (mode==3)
        sprintf(scratch,"%s\\*.MSG",thisarea.directory);

    done = findfirst(scratch,&ffblk,0);
    while (!done)
    {
        t = atoi(strtok(ffblk.ff_name,"."));
        if (t > rc)
            rc = t;
        done=findnext(&ffblk);
    }
    rc++;           /* Return next number */
    return(rc);     /* Return next number */
}

void write_nodes_config(void)
{
    FILE *fp;

    if ((fp = fsh_open("NODES.DAT","w"))==NULL)
	exit(1);

    fsh_write(nodes, sizeof nodes,1,fp);
    fsh_close(fp);
    cfg.num_nodes=num_nodes;
}

void pack_outbound_packets(void)
{
    char s[80],temp[80],cmd[161],out_file[81],fn[80],tempfn[80],temp1[80];
    char t1[50],t2[50],t3[50],t4[50],td[80],log[181];
    int unsuccessful;
    char drive[10],dir[80],name[20],ext[20],*message,s3[81],old_name[81];
    int f1,exit_status,ok2,counter,compress,node_num,cycle;
    int i,length;
    char ch;

    int handle;
    long msgid;
    struct ffblk ff;
    time_t timer;
    struct tm *tblock;
    struct date dt;
    struct time tm;
    FILE *attach_file;
    char *month[] = {"Bug","Jan","Feb","Mar","Apr","May","Jun",
         "Jul","Aug","Sep","Oct","Nov","Dec"};

    timer = time(NULL);
    tblock = localtime(&timer);
    getdate(&dt);
    gettime(&tm);
    msgid=time(NULL);
    exit_status=0;

    sprintf(s,"%s*.QQQ",cfg.outbound_temp_path);
    f1=findfirst(s,&ff,0);
    while ((f1==0) && (exit_status==0))
    {
        strcpy(s,cfg.outbound_temp_path);
        strcat(s,(ff.ff_name));
        strcpy(old_name,s);
        fnsplit(s,drive,dir,name,ext);
        strcpy(ext,".PKT");
        invent_pkt_name(fn);
        strcpy(name,fn);
        fnmerge(temp,drive,dir,name,ext);
        errno=0;
        rename(s,temp);
        strcpy(s,temp);
        incoming_file=-1;
        incoming_file=sh_open1(s,O_RDWR | O_BINARY);
        if (incoming_file<0)
        {
            printf("\7þ Error opening outbound packet: %s. errno=%d\n",s,errno);
            sprintf(log,"þ Error opening outbound packet: %s. errno=%d\n [%s]",s,errno,sys_errlist[errno]);
            write_log(log);
            rename(s,old_name);
            exit_status=6;
            break;
        }
        sh_read(incoming_file,(void *)&packet_header,sizeof(packet_header_struct));
        sh_close(incoming_file);
        ok2=0;
        if (packet_header.OrigZone==0)
            packet_header.OrigZone=packet_header.QMOrigZone;
        if (packet_header.DestZone==0)
            packet_header.DestZone=packet_header.QMDestZone;
        /* set my_net, etc. */
        for (cycle=0;cycle<12;cycle++)
        {
            if (cfg.aka_list[cycle].zone==packet_header.DestZone)
            {
                my_zone=cfg.aka_list[cycle].zone;
                my_net=cfg.aka_list[cycle].net;
                my_node=cfg.aka_list[cycle].node;
                my_point=cfg.aka_list[cycle].point;
                break;
            }
        }
        who_to.zone=packet_header.DestZone;
        who_to.net=packet_header.dest_net;
        who_to.node=packet_header.dest_node;
        who_to.point=packet_header.DestPoint;
        compress=0;
        for(counter=1;counter<MAX_NODES;counter++)
        {
            sprintf(temp,"%d:%d/%d.%d",packet_header.DestZone,packet_header.dest_net,packet_header.dest_node,packet_header.DestPoint);
            if (stricmp(temp,nodes[counter].address)==0)
            {
                compress=nodes[counter].compression;
                node_num=counter;
                ok2=1;
                counter=MAX_NODES;
            }
        }
        if ((cfg.mailer==2) && (my_zone!=cfg.aka_list[0].zone))
        {
             length=strlen(cfg.outbound_path);
             strcpy(td,"");
             for (i=0;i<length-1;i++)
             {
                 sprintf(temp1,"%c",cfg.outbound_path[i]);
                 strcat(td,temp1);
             }
        }
        if (compress>0)
        {
            if (tblock->tm_yday!=nodes[node_num].daynum);
            {
                nodes[node_num].counter=1;
                nodes[node_num].daynum=tblock->tm_yday;
                write_nodes_config();
            }
            ok2=0;
            do
            {
                sprintf(temp1,"%s",suffixes[tblock->tm_wday]);
                if (nodes[node_num].counter<10)
                    sprintf(temp,"%c%c%d",temp1[0],temp1[1],nodes[node_num].counter);
                else
                {
                    if (nodes[node_num].counter>99)
                       sprintf(temp,"%c%c%c",temp1[0],temp1[1],((nodes[node_num].counter-100)+'A'));
                    else
                        sprintf(temp,"%c%02d",temp1[0],nodes[node_num].counter);
                }
                sprintf(out_file,"%04X%04X.%s",(my_net-packet_header.dest_net),(my_node-packet_header.dest_node),temp);
                if ((cfg.mailer==2) && (my_zone!=cfg.aka_list[0].zone))
                {
                    sprintf(temp,"%s.%03X\\%s",td,my_zone,out_file);
                } else
                    sprintf(temp,"%s%s",cfg.outbound_path,out_file);
                if (!(exist(temp)))
                {
                    ok2=2;
                }
                else
                {
                    handle = sh_open1(temp,O_RDONLY | O_BINARY);
                    if (handle<0)
                    {
                        ok2=2;
                    }
                    if (handle>=0)
                    {
                        ok2=1;
                    }
                    if ((filelength(handle)<1) && (ok2!=2))
                    {
                        ok2=0;
                    }
                    sh_close(handle);
                }
                if (!ok2)
                    nodes[node_num].counter++;
                if (nodes[node_num].counter>200)
                {
                    ok2=2;
                    nodes[node_num].counter=1;
                    write_nodes_config();
                }
            } while (!ok2);
            sh_close(handle);

            if (ok2==2)
                printf("þ Creating File %s for %d:%d/%d.%d\r\n",out_file,packet_header.DestZone,packet_header.dest_net,packet_header.dest_node,packet_header.DestPoint);
            printf("þ Updating %s with %s\r\n",out_file,s);
            sprintf(cmd,"%s ",cfg.archive[compress].archive_file);
            sprintf(t1,"%s ",cfg.archive[compress].archive_switches);
            if ((cfg.mailer==2) && (my_zone!=cfg.aka_list[0].zone))
                sprintf(t2,"%s.%03X\\%s",td,my_zone,out_file);
            else
                sprintf(t2,"%s%s ",cfg.outbound_path,out_file);
            sprintf(t3,"%s ",s);
            strcpy(t4,">NULL");
            unsuccessful=spawnlp(P_WAIT,cmd," ",t1,t2,t3,NULL);
            if (unsuccessful)
            {
                if (unsuccessful<0)
                {
                    printf("þ Error with spawn!  Error= %s\r\n",sys_errlist[errno]);
                    sprintf(log,"þ Error with spawn!  Error= %s\n",sys_errlist[errno]);
                    write_log(log);
                }
                else
                {
                    printf("þ Error Archiving Packet! (%s error code %d)\r\n",cfg.archive[compress].name,unsuccessful);
                    sprintf(log,"þ Error Archiving Packet! (%s error code %d)\n",cfg.archive[compress].name,unsuccessful);
                    write_log(log);
                }
                rename(s,old_name);
                return;
            }
        }
        else
        {
            ok2=2;
            strcpy(tempfn,s);
            fnsplit(s,drive,dir,name,ext);
            if ((cfg.mailer==2) && (my_zone!=cfg.aka_list[0].zone))
                sprintf(dir,"%s.%03X\\",td,my_zone);
            else
                strcpy(dir,cfg.outbound_path);

            if (cfg.mailer==2)
            {
                switch (nodes[node_num].archive_status)
                {
                    case 0:
                        strcpy(ext,".OUT");
                        break;
                    case 1:
                        strcpy(ext,".HUT");
                        break;
                    case 2:
                        strcpy(ext,".CUT");
                        break;
                }
            }
            else
                strcpy(ext,".PKT");
            sprintf(temp,"%s%s%s",dir,name,ext);
            rename(s,temp);
            sprintf(out_file,"%s%s",name,ext);
            strcpy(s,tempfn);
        }

        if (ok2==2)
        {
            printf("þ Creating File Attach Message for %d:%d/%d.%d\r\n",packet_header.DestZone,packet_header.dest_net,packet_header.dest_node,packet_header.DestPoint);
            if (cfg.mailer==2)
            {
                if (my_zone!=cfg.aka_list[0].zone)
                    sprintf(fn,"%s.%03X\\%04x%04x.",td,my_zone,who_to.net,who_to.node);
                else
                    sprintf(fn,"%s%04x%04x.", cfg.outbound_path,who_to.net,who_to.node);
                switch (nodes[node_num].archive_status)
                {
                    case 0:
                        strcpy(ext,"FLO");
                        break;
                    case 1:
                        strcpy(ext,"HLO");
                        break;
                    case 2:
                        strcpy(ext,"CLO");
                        break;
                    default:
                        strcpy(ext,"FLO");
                        break;
                }
                strcat(fn,ext);
                if (my_zone!=cfg.aka_list[0].zone)
                {
                    if (!compress)
                        sprintf(temp,"^%s.%03X\\%s",td,my_zone,out_file);
                    else
                        sprintf(temp,"#%s.%03X\\%s",td,my_zone,out_file);
                }
                else
                {
                    if (!compress)
                        sprintf(temp,"^%s%s",cfg.outbound_path,out_file);
                    else
                        sprintf(temp,"#%s%s",cfg.outbound_path,out_file);
                }

                /*  --------------------------------------------------
                 *  if there's an attach-file already, look through it
                 *  to see if the .MOx file is already listed there
                 *  --------------------------------------------------  */
                ch = 'N';
                attach_file=fsh_open(fn,"rt");
                sprintf(log,"þ Creating File Attach message %s\n",fn);
                write_log(log);
                if (attach_file != NULL)
                {
                    while (i=fscanf(attach_file, "%s", s),   ((i!= EOF) && (i!=0)) )
                    {
                        if (!strcmp(s,temp))
                        {
                            ch = 'Y';
                        }
                    }
                    fsh_close(attach_file);
                }
                /*  -------------------------------------------
                 *  add a new line to the attach file if needed
                 *  -------------------------------------------  */
                if (ch == 'N')
                {
                    attach_file=fsh_open(fn,"a+t");
                    if (attach_file == NULL)
                    {
                        printf("error opening attach file '%s'. errno=%d\n",fn,errno);
                        exit_status=3;
                        return;
                    }
                    /*  --------------------------------------------------------------------
                     *  send it with an "^" in front of name to delete file after sending it
                     *  --------------------------------------------------------------------  */

                    fprintf(attach_file,"%s\n",temp);
                    fsh_close(attach_file);
                }
                strcpy(s,tempfn);
            }
            else
            {
                attribute=0;
                attribute = MSGLOCAL;
                switch (nodes[node_num].archive_status)
                {
                    case 0:
                        break;
                    case 1:
                        attribute = attribute | (MSGHOLD);
                        break;
                    case 2:
                        attribute = attribute | (MSGCRASH);
                        break;
                }
                message = malloc(32*1024);
                sprintf(fdmsg.FromUser,"%s %s",PROGRAM,VERSION);
                strcpy(fdmsg.ToUser,"SYSOP");
                sprintf(fdmsg.Subject,"%s%s",cfg.outbound_path,out_file);
                sprintf(s3,"%d",dt.da_year);
                sprintf(fdmsg.Date,"%02d %s %c%c  %02d:%02d:%02d",
                    dt.da_day,month[dt.da_mon],s3[2],s3[3],
                    tm.ti_hour,tm.ti_min,tm.ti_sec+(tm.ti_hund+50)/100);

                fdmsg.TimesRead=0;
                fdmsg.DestNode=packet_header.dest_node;
                fdmsg.OrigNode=my_node;
                fdmsg.Cost=0;
                fdmsg.OrigNet=my_net;
                fdmsg.DestNet=packet_header.dest_net;
                fdmsg.DateWritten=0;
                fdmsg.DateArrived=0;
                fdmsg.ReplyTo=0;
                fdmsg.Attr = attribute | (MSGFILE) | (MSGKILL);
                if (cfg.crash==2)
                {
                    if (!(fdmsg.Attr & MSGCRASH) == (MSGCRASH))
                        fdmsg.Attr = fdmsg.Attr | (MSGCRASH);
                }
                fdmsg.ReplyNext=0;

                /* Updated by MMH */
                /* if (packet_header.DestZone!=my_zone) */
                  /*  sprintf(message,"INTL %d:%d/%d.%d %d:%d/%d.%d\r",packet_header.DestZone,packet_header.dest_net,packet_header.dest_node,packet_header.DestPoint,my_zone,my_net,my_node,my_point);  */
               sprintf(message,"INTL %d:%d/%d %d:%d/%d\r",packet_header.DestZone,packet_header.dest_net,packet_header.dest_node,my_zone,my_net,my_node);
			   /* else
                    sprintf(message,""); */
               /* End of update */

                if (my_point)
                    sprintf(s3,"MSGID: %d:%d/%d.%d %lX\r",my_zone,my_net,my_node,my_point,msgid++);
                else
                    sprintf(s3,"MSGID: %d:%d/%d %lX\r",my_zone,my_net,my_node,msgid++);
                strcat(message,s3);
                sprintf(s3,"PID: WWIVTOSS v. %s\r",VERSION);
                strcat(message,s3);
                sprintf(s3,"FLAGS TFS");
                strcat(message,s3);
                if (cfg.set_immediate)
                    sprintf(s3," IMM\r");
                else
                    sprintf(s3,"\r");
                strcat(message,s3);
                    
			    if (out_point)    /* Begin MMH Mod */
                    {
                        sprintf(s3,"TOPT %d\r",out_point);
                        strcat(message,s3);
                    }
                if (my_point)
                    {
                        sprintf(s3,"FMPT %d\r",my_point);
                        strcat(message,s3);
                    }   /* End MMH Mod */

                sprintf(fn,"%s%d.MSG",cfg.netmail_path,findnextmsg(1));
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
                        printf("error opening outbound file '%s'. errno=%d.7\n",outbound_packet_name,errno);
                        exit_status=3;
                        return;
                    }
                }
                sh_write(outbound_file,(void *)&fdmsg,sizeof(FIDOMSG));
                sh_write(outbound_file,message,(strlen(message)+1));
                sh_close(outbound_file);
                free(message);
            }

        }
        unlink(s);
        f1=findnext(&ff);
    }
//    write_nodes_config();
}

void return_message(net_header_rec temp, char *message)
{
    char s[81],title[81],*mess,source[81],mailtime[81],netfile[81];
    int p0file;
    unsigned int *list;
    int size;
    net_header_rec nh;

    mess = malloc(32*1024);
    nh.tosys=temp.fromsys;
    nh.touser=temp.fromuser;
    nh.fromsys=net_networks[net_num].sysnum;
    nh.fromuser=1;
    nh.list_len=0;
    nh.main_type=2;
    nh.minor_type=1;
    nh.method=0;
    time((long *)&(nh.daten));
    strcpy(mailtime, ctime((time_t *)&nh.daten));
    mailtime[strlen(mailtime)-1] = '\r\n';

    strcpy(netfile,net_data);
    sprintf(source,"%s v. %s @%u",PROGRAM,VERSION,nh.fromsys);
    strcat(source,"\r\n");
    strcpy(title,"Invalid Message Return!");
    strcpy(mess,"6þ2 This Message is being returned to you because the addressing was\r\n");
    strcat(mess,"6þ2 incorrect.  Please check your setup and try again.\r\n");
    sprintf(s,"6þ1 Mail formatting for Fidonet is USERNAME (FIDO_NODE) @%u\r\n",nh.fromsys);
    strcat(mess,s);
    strcat(mess,"\r\n5              Text of your messages follows\r\n");
    strcat(mess,"6====================================================\r\n\r\n");
    strcat(mess,"   TO: ");
    strcat(mess,fido_to_user);
    strcat(mess,"\r\n");
    strcat(mess,"   RE: ");
    strcat(mess,fido_title);
    strcat(mess,"\r\n");
    strcat(mess,message);
    strcat(mess,"\r\n6====================================================\r\n");
    sprintf(s,"2                  WWIVToss Version %s\r\n",VERSION);
    strcat(mess,s);
    size=strlen(mess);
    nh.length=size;
    strcat(netfile,"P-WT-R.NET");
    p0file = sh_open(netfile, O_RDWR | O_APPEND | O_BINARY, S_IREAD | S_IWRITE);
    if (p0file == -1)
    p0file = sh_open(netfile, O_RDWR | O_BINARY | O_CREAT, S_IREAD | S_IWRITE);
    if (p0file == -1)
    {
	perror("Could not open LOCAL.NET file for append or write! Tossing to Bad");
	write_log("Could Not Open LOCAL.NET file for append or write!");
    free(mess);
	return;
    }
    lseek(p0file,0L,SEEK_END);


    /* Get date/time of Email creation. */
    /* Ditto the \015 for the time/date string. */

/*
* Compute the actual length of network packet. The extra 1 is
* to allow for the NULL at then end of the title string.
*/

    nh.length = (unsigned long) size +
                (unsigned long) sizetag +
                (unsigned long) strlen(source) +
                (unsigned long) strlen(title) + 1 +
                (unsigned long) strlen(mailtime);

/*
* Write out the netpacket in chunks;
* Let DOS buffer them together.
*/

    if (sh_write(p0file, &nh, sizeof(nh)) == -1)
    {
        perror("Error writing Net Header to LOCAL.NET");
        free(mess);
        return;
    }

    if (sh_write(p0file, (void *)list, 2*(nh.list_len)) == -1)
    {
        perror("Error writing Net Header to LOCAL.NET");
        free(mess);
        return;
    }

/* Title needs to be stored with its terminating NULL */
    if (sh_write(p0file, title, strlen(title) + 1) == -1)
    {
        perror("Error writing title to LOCAL.NET");
        free(mess);
        return;
    }

    if (sh_write(p0file, source, strlen(source)) == -1)
    {
        perror("Error writing source address to LOCAL.NET");
        free(mess);
        return;
    }

    if (sh_write(p0file, mailtime, strlen(mailtime)) == -1)
    {
        perror("Error writing time string to LOCAL.NET");
        free(mess);
        return;
    }

    if (sh_write(p0file, mess, size) == -1)
    {
        perror("Error writing MailFile to LOCAL.NET");
        free(mess);
        return;
    }

    sh_close(p0file);
    free(mess);


}

unsigned char upcase(unsigned char ch)
/* This converts a character to uppercase */
{
  unsigned char *ss;

  ss=strchr(translate_letters[0],ch);
  if (ss)
    ch=translate_letters[1][ss-translate_letters[0]];
  return(ch);
}

/****************************************************************************/

unsigned char locase(unsigned char ch)
/* This converts a character to lowercase */
{
  unsigned char *ss;

  ss=strchr(translate_letters[1],ch);
  if (ss)
    ch=translate_letters[0][ss-translate_letters[1]];
  return(ch);
}

void properize(char *s)
{
  int i;

  for (i=0;s[i];i++)
    s[i]=locase(s[i]);

  s[0]=upcase(s[0]);

  for (i=0; s[i]; i++) {
    if (((s[i]==' ') || (s[i]=='-')) && ((s[i+1] > '`') && (s[i+1] < '{'))) {
      i++;
      if (((s[i]=='m') && (s[i+1]=='c')) || ((s[i]=='o') && (s[i+1]=='\''))) {
        s[i]=upcase(s[i]);
        i+=2;
        s[i]=upcase(s[i]);
      }
      if ((s[i]=='i') && (s[i+1]=='b') && (s[i+2]=='m')) {
        s[i++]=upcase(s[i]);
        s[i++]=upcase(s[i]);
        s[i]=upcase(s[i]);
      }
      if ((s[i]=='b') && (s[i+1]=='b') && (s[i+2]=='s')) {
        s[i++]=upcase(s[i]);
        s[i++]=upcase(s[i]);
        s[i]=upcase(s[i]);
      }
      if ((s[i]=='w') && (s[i+1]=='w') && (s[i+2]=='i')  && (s[i+3]=='v')) {
        s[i++]=upcase(s[i]);
        s[i++]=upcase(s[i]);
        s[i++]=upcase(s[i]);
        s[i]=upcase(s[i]);
      }
      if ((s[i]=='p') && (s[i+1]=='.') && (s[i+2]=='o')  && (s[i+3]=='.')) {
        s[i++]=upcase(s[i]);
        s[++i]=upcase(s[i]);
      }
      if ((s[i]=='p') && (s[i+1]=='c') && (s[i+2]=='-') && (s[i+3]=='b')) {
        s[i++]=upcase(s[i]);
        s[i++]=upcase(s[i]);
        s[i++]=upcase(s[i]);
        s[i]=upcase(s[i]);
      }
      if ((s[i]=='t') && (s[i+1]=='a') && (s[i+2]=='g') && (s[i+3]==' ')) {
        s[i++]=upcase(s[i]);
        s[i++]=upcase(s[i]);
        s[i]=upcase(s[i]);
      }
    else
      s[i]=upcase(s[i]);
    }
  }
}

