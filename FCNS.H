#include "vardec.h"
/* File: WWIVTOSS.C */
unsigned char upcase(unsigned char ch);
int read_config(int mode);
int get_config(void);
void send_mail(void);
void load_workspace(void);
void write_area(unsigned int un, arearec *a);
void read_area(unsigned int un, arearec *a);
void read_uplink_config(void);
int process_bad_message(void);
void process_incoming_message(void);
void get_K9_config(void);
void main (int argc, char *argv[]);
int read_subs(void);
void read_config_dat(void);
void far *mallocx(unsigned long l, char *where);
void read_user(unsigned int un, userrec *u);
int number_userrecs(void);
void incoming_mail(void);
void outgoing_mail(int mode);
void center(unsigned char *s);
void process_incoming_netmail(void);
void fidoadr_split(char *addr, FIDOADR *fadr);
void unarchive_packets(void);
void fidoadr_split(char *addr, FIDOADR *fadr);
void toss_to_bad(message_header_struct message_header,char *message,int type);
void header(void);
void write_log(char *string);
void send_notification(void);
void send_nag(char *s);
void process_areafix_message(void);
char *ctim2(double d);
char *ctim3(double d);
void bounce_mail(void);
void sort_exports(void);
void semaphore(void);
int skip_user(char *user_name);
int exist_export_node(char *address,int area);
void write_areas_config(void);
void write_config(void);
void purge_log(void);
void check_mail(void);
void cleanup_zero_byte_msgs(void);
void write_net_log(double total);
int twit_me(char *username, char *fido_node);
void Read_Hidden_Data_File(void);
int check_uplink(int mode, char *echo_name);
void uplink_request(char *echoname, char *address);
void detect_multitask(void);
void giveup_timeslice(void);
int check_for_dupe(void);

/* File: share.c */

int sh_open(char *path, int file_access, unsigned mode);
int sh_open1(char *path, int access);
FILE *fsh_open(char *path, char *mode);
int sh_close(int f);
void fsh_close(FILE *f);
void share_installed(void);
long sh_lseek(int handle, long offset, int fromwhere);
int sh_read(int handle, void *buf, unsigned len);
int sh_write(int handle, void *buf, unsigned len);
size_t fsh_read(void *ptr, size_t size, size_t n, FILE *stream);
size_t fsh_write(void *ptr, size_t size, size_t n, FILE *stream);

void far *malloca(unsigned long nbytes);

/* File : Export.c */
int exist(char *s);
void process_outgoing(void);
void send_message(int mode, char *message);
void open_outbound_packet(char *filename, netnode who_to, int pktt_type);
void invent_pkt_name (char * string);
void get_areafix_config(void);
void outchr(unsigned char ch);
int findnextmsg(int mode);
void write_nodes_config(void);
void pack_outbound_packets(void);
void return_message(net_header_rec temp, char *message);
void properize(char *s);

/* subacc.c */
void close_sub(void);
int open_sub(int wr);
int iscan1(int si, int quick);
int iscan(int b);
postrec *get_post(unsigned short mn);
/* dawg.c */
void add_post(postrec *pp);
void save_status(void);
void set_net_num(int n);
long huge_xfer(int fd, void huge *buf, unsigned sz, unsigned nel, int wr);
int read_subs(void);
void get_status(int mode, int lock);
void lock_status(void);
void read_status(void);
void savebase(int b);
int open_file(char *fn);
void remove_link(messagerec *m1, char *aux);
void set_gat_section(int f, int section);
void save_gat(int f);
void savefile(char *b, long l1, messagerec *m1, char *aux);
char *readfile(messagerec *m1, char *aux, long *l);
unsigned char *getnetfile(void);
void gate_msg(net_header_rec *nh, char *text, unsigned short nn, char *byname,unsigned int *list, unsigned short fnn);
void send_net(net_header_rec *nh, unsigned int *list, char *text, char *byname);
void send_net_post(postrec *p, char *extra, int subnum);
void deletemsg(int mn);
void addline(char *b, char *s, long *len);
void send_email2(void);
int open_sub(int wr);
