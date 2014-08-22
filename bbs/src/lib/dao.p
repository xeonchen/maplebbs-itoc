/* is_alnum.c */
int is_alnum(int ch);
/* is_alpha.c */
int is_alpha(int ch);
/* not_addr.c */
int not_addr(char *addr);

#ifdef _RESOLV_H
/* dns.c */
void dns_init(void);
/* dns_aton.c */
unsigned long dns_aton(char *name);  
int dns_query(char *name, int qtype, querybuf *ans);
/* dns_name.c */
int dns_name(unsigned char *addr, char *name);
/* dns_open.c */
int dns_open(char *host, int port);
/* dns_smtp.c */
int dns_mx(char *domain, char *mxlist);
int dns_smtp(char *host);
#endif

/* str_ansi.c */
void str_ansi(char *dst, char *str, int max);
/* str_cat.c */
void str_cat(char *dst, char *s1, char *s2);
/* str_cmp.c */
int str_cmp(char *s1, char *s2);
/* str_decode.c */
char *mm_getencode(unsigned char *str, char *code);
void mm_getcharset(const char *str, char *charset, int size);
int mmdecode(unsigned char *src, int encode, unsigned char *dst);
void str_decode(unsigned char *str);
/* str_folder.c */
void str_folder(char *fpath, char *folder, char *fname);
/* str_fpath.c */
void setdirpath(char *fpath, char *direct, char *fname);
/* str_from.c */
int str_from(char *from, char *addr, char *nick);
/* str_has.c */
int str_has(char *list, char *tag, int len);
/* str_hash.c */
int str_hash(char *str, int seed);
/* str_len.c */
int str_len(char *str);
/* str_lower.c */
void str_lower(char *dst, char *src);
/* str_lowest.c */
void str_lowest(char *dst, char *src);
/* str_ncmp.c */
int str_ncmp(char *s1, char *s2, int n);
/* str_ncpy.c */
void str_ncpy(char *dst, char *src, int n);
/* str_passwd.c */
char *genpasswd(char *pw);
int chkpasswd(char *passwd, char *test);
/* str_stamp.c */
void str_stamp(char *str, time_t *chrono);
/* str_str.c */
char *str_str(char *str, char *tag);
/* str_sub.c */
char *str_sub(char *str, char *tag);
/* str_tail.c */
char *str_tail(char *str);
/* str_time.c */
char *Atime(time_t *clock);
char *Btime(time_t *clock);
char *Now(void);
/* str_trim.c */
void str_trim(char *buf);
/* str_ttl.c */
char *str_ttl(char *title);

/* archiv32.c */
void archiv32(time_t chrono, char *fname);
/* chrono32.c */
time_t chrono32(char *str);
/* hash32.c */
int hash32(unsigned char *str);
/* radix32.c */

/* f_cat.c */
void f_cat(char *fpath, char *msg);
/* f_cp.c */
int f_cp(char *src, char *dst, int mode);
/* f_img.c */
char *f_img(char *fpath, int *fsize);
/* f_ln.c */
int f_ln(char *src, char *dst);
/* f_lock.c */
int f_exlock(int fd);
int f_unlock(int fd);
/* f_map.c */
char *f_map(char *fpath, int *fsize);
/* f_mode.c */
int f_mode(char *fpath);
/* f_mv.c */
int f_mv(char *src, char *dst);
/* f_new.c */
FILE *f_new(char *fold, char *fnew);
/* f_path.c */
void brd_fpath(char *fpath, char *board, char *fname);
void gem_fpath(char *fpath, char *board, char *fname);
void usr_fpath(char *fpath, char *user, char *fname);
/* f_rm.c */
int f_rm(char *fpath);
/* f_suck.c */
void f_suck(FILE *fp, char *fpath);
/* mak_dirs.c */
void mak_dirs(char *fpath);
void mak_links(char *fpath);

/* rec_add.c */
int rec_add(char *fpath, void *data, int size);
/* rec_bot.c */
int rec_bot(char *fpath, void *data, int size);
/* rec_num.c */
int rec_num(char *fpath, int size);
/* rec_del.c */
int rec_del(char *fpath, int size, int pos, int (*fchk)());
/* rec_get.c */
int rec_get(char *fpath, void *data, int size, int pos);
/* rec_ins.c */
int rec_ins(char *fpath, void *data, int size, int pos, int num);
/* rec_put.c */
int rec_put(char *fpath, void *data, int size, int pos, int (*fchk)());
/* rec_ref.c */
int rec_ref(char *fpath, void *data, int size, int pos, int (*fchk)(), void (*fref)());
/* rec_sync.c */
int rec_sync(char *fpath, int size, int (*fsync)(), int (*fchk)());

/* hdr_fpath.c */
void hdr_fpath(char *fpath, char *folder, HDR *hdr);
/* hdr_stamp.c */
int hdr_stamp(char *folder, int token, HDR *hdr, char *fpath);

/* shm.c */
void *shm_new(int shmkey, int shmsize);

/* splay.c */
SplayNode *splay_in(SplayNode *top, void *data, int (*compare)());

/* acl_has.c */
int acl_has(char *acl, char *user, char *host);
/* rfc2047.c */
void output_rfc2047_qp(FILE *fp, char *prefix, char *str, char *charset, char *suffix);
/* xwrite.c */
int xwrite(int fd, char *data, int size);

/* dl_lib.c */
void *DL_get(char *name);
