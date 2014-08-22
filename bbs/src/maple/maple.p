/* acct.c */
int acct_load(ACCT *acct, char *userid);
void acct_save(ACCT *acct);
int acct_userno(char *userid);
int acct_get(char *msg, ACCT *acct);
void bitmsg(char *msg, char *str, int level);
usint bitset(usint pbits, int count, int maxon, char *msg, char *perms[]);
void acct_show(ACCT *u, int adm);
void acct_setup(ACCT *u, int adm);
void acct_setperm(ACCT *u, usint leveup, usint leveldown);
void addmoney(int addend);
void addgold(int addend);
int brd_new(BRD *brd);
void brd_edit(int bno);
void brd_title(int bno);
void x_file(int mode, char *xlist[], char *flist[]);
int m_trace(void);

/* bbsd.c */
void alog(char *mode, char *msg);
void blog(char *mode, char *msg);
void u_exit(char *mode);
void abort_bbs(void);

/* bmw.c */
int can_override(UTMP *up);
int can_see(UTMP *my, UTMP *up);
int bmw_send(UTMP *callee, BMW *bmw);
void bmw_edit(UTMP *up, char *hint, BMW *bmw);
int bmw_reply_CtrlRT(int ch);
void bmw_reply(void);
void bmw_rqst(void);
void do_write(UTMP *up);
void bmw_log(void);
int t_bmw(void);
int t_display(void);

/* board.c */
void brh_get(time_t bstamp, int bhno);
int brh_unread(time_t chrono);
void brh_visit(int mode);
void brh_add(time_t prev, time_t chrono, time_t next);
int bstamp2bno(time_t stamp);
void brh_save(void);
void brd_force(void);
void class_item(int num, int bno, int brdpost);
int is_bm(char *list, char *userid);
void mantime_add(int outbno, int inbno);
int XoPost(int bno);
int Select(void);
int Class(void);
int MFclass_browse(char *name);
void board_main(void);
int Boards(void);

/* cache.c */
void ushm_init(void);
void utmp_mode(int mode);
int utmp_new(UTMP *up);
void utmp_free(UTMP *up);
UTMP *utmp_find(int userno);
UTMP *utmp_get(int userno, char *userid);
UTMP *utmp_seek(HDR *hdr);
void utmp_admset(int userno, usint status);
int utmp_count(int userno, int show);
UTMP *utmp_search(int userno, int order);
void bshm_init(void);
void bshm_reload(void);
int brd_bno(char *bname);
void fshm_init(void);
void film_out(int tag, int row);

/* edit.c */
char *tbf_ask(void);
FILE *tbf_open(void);
void ve_backup(void);
void ve_recover(void);
void ve_header(FILE *fp);
void ve_banner(FILE *fp, int modify);
int ve_subject(int row, char *topic, char *dft);
int vedit(char *fpath, int ve_op);

/* favorite.c */
void mf_fpath(char *fpath, char *userid, char *fname);
int MyFavorite(void);
void mf_main(void);

/* gem.c */
int gem_link(char *brdname);
void brd2gem(BRD *brd, HDR *gem);
void gem_buffer(char *dir, HDR *hdr, int (*fchk)());
int gem_gather(XO *xo);
void XoGem(char *folder, char *title, int level);
void gem_main(void);

/* mail.c */
void ll_new(void);
void ll_add(char *name);
int ll_del(char *name);
int ll_has(char *name);
void ll_out(int row, int column, char *msg);
int bsmtp(char *fpath, char *title, char *rcpt, int method);
int m_zip(void);
int m_verify(void);
usint m_quota(void);
usint m_query(char *userid);
void m_biff(int userno);
void mail_hold(char *fpath, char *rcpt, char *title, int hold);
int mail_external(char *addr);
int mail_send(char *rcpt);
int my_send(char *userid);
int m_send(void);
int m_internet(void);
int m_sysop(void);
void mail_self(char *fpath, char *owner, char *title, usint xmode);
int mail_him(char *fpath, char *rcpt, char *title, usint xmode);
int m_list(void);
int do_mreply(HDR *hdr, int noreply);
int mbox_edit(XO *xo);
void mbox_main(void);

/* menu.c */
int pad_view(void);
void vs_head(char *title, char *mid);
void movie(void);
void menu(void);

/* more.c */
char *mgets(int fd);
void *mread(int fd, int len);
int more(char *fpath, char *footer);

/* pal.c */
int is_mygood(int userno);
int is_mybad(int userno);
int is_ogood(UTMP *up);
int is_obad(UTMP *up);
int is_bgood(BPAL *bpal);
int is_bbad(BPAL *bpal);
int image_pal(char *fpath, int *pool);
void pal_cache(void);
void pal_sync(char *fpath);
int pal_list(int reciper);
void pal_edit(int key, PAL *pal, int echo);
int pal_find(char *fpath, int userno);
int t_pal(void);
int t_list(void);

/* post.c */
int cmpchrono(HDR *hdr);
void btime_update(int bno);
void cancel_post(HDR *hdr);
int is_author(HDR *hdr);
int chkrestrict(HDR *hdr);
int do_reply(XO *xo, HDR *hdr);
int tag_char(int chrono);
void hdr_outs(HDR *hdr, int cc);
int post_edit(XO *xo);
void header_replace(XO *xo, HDR *fhdr);
int post_cross(XO *xo);
int post_forward(XO *xo);
int post_write(XO *xo);
int post_score(XO *xo);

/* talk.c */
char *bmode(UTMP *up, int simple);
void frienz_sync(char *fpath);
void aloha();
void loginNotify();
void my_query(char *userid);
int t_loginNotify(void);
void loginNotify(void);
int talk_page(UTMP *up);
int t_pager(void);
int t_cloak(void);
int t_query(void);
int t_talk(void);
void talk_rqst(void);

/* ulist.c */
void talk_main(void);

/* user.c */
void justify_log(char *userid, char *justify);
int u_addr(void);
int u_register(void);
int u_verify(void);
int u_deny(void);
int u_info(void);
int u_setup(void);
int u_lock(void);
int u_log(void);
int u_xfile(void);

/* visio.c */
int is_zhc_low(char *str, int pos);
void prints(char *fmt, ...);
void bell(void);
void move(int x, int y);
void refresh(void);
void clear(void);
void clrtoeol(void);
void clrtobot(void);
void outc(int ch);
void outs(uschar *str);
void outx(uschar *str);
void outz(uschar *str);
void outf(uschar *str);
void scroll(void);
void rscroll(void);
void cursor_save(void);
void cursor_restore(void);
void save_foot(screenline *slp);
void restore_foot(screenline *slp, int line);
int vs_save(screenline *slp);
void vs_restore(screenline *slp);
int vmsg(char *msg);
void zmsg(char *msg);
void vs_bar(char *title);
void vio_save(void);
void vio_restore(void);
int vio_holdon(void);
void add_io(int fd, int timeout);
int igetch(void);
BRD *ask_board(char *board, int perm, char *msg);
int vget(int line, int col, uschar *prompt, uschar *data, int max, int echo);
int vans(char *prompt);
int vkey(void);

/* window.c */
int pans(int x, int y, char *title, char **desc);
int pmsg(char *msg);

/* xover.c */
XO *xo_new(char *path);
XO *xo_get(char *path);
XO *xo_get_post(char *path, BRD *brd);
void xo_load(XO *xo, int recsiz);
int xo_rangedel(XO *xo, int size, int (*fchk) (), void (*fdel) ());
int Tagger(time_t chrono, int recno, int op);
void EnumTag(void *data, char *dir, int locus, int size);
int AskTag(char *msg);
int xo_prune(XO *xo, int size, int (*fvfy) (), void (*fdel) ());
int xo_uquery(XO *xo);
int xo_usetup(XO *xo);
int xo_getch(XO *xo, int ch);
void xover(int cmd);
int every_Z(int zone);
int every_U(int zone);
int xo_cursor(int ch, int pagemax, int num, int *pageno, int *pos, int *redraw);
void xo_help(char *path);

/* xpost.c */
int xpost_head(XO *xo);
int xpost_init(XO *xo);
int xpost_load(XO *xo);
int xpost_browse(XO *xo);
int xmbox_browse(XO *xo);
int XoXselect(XO *xo);
int XoXsearch(XO *xo);
int XoXauthor(XO *xo);
int XoXtitle(XO *xo);
int XoXfull(XO *xo);
int XoXmark(XO *xo);
int XoXlocal(XO *xo);
int news_head(XO *xo);
int news_init(XO *xo);
int news_load(XO *xo);
int XoNews(XO *xo);
