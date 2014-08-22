#ifndef _BBSLIB_H_
#define _BBSLIB_H_

/* bbslib.c */
extern int NLCOUNT;
extern nodelist_t *NODELIST;
extern int nl_bynamecmp();

/* bbslib.c */
extern int NFCOUNT;
extern newsfeeds_t *NEWSFEEDS;
extern newsfeeds_t *NEWSFEEDS_B;
extern newsfeeds_t *NEWSFEEDS_G;
extern int nf_byboardcmp();
extern int nf_bygroupcmp();

/* bbslib.c */
extern int SPAMCOUNT;
extern spamrule_t *SPAMRULE;

/* bbslib.c */
extern int initial_bbs();
extern void bbslog(char *fmt, ...);

/* rec_article.c */
extern void init_bshm();
extern int cancel_article();
extern int receive_article();
#ifdef _NoCeM_
extern int receive_nocem();
#endif

#endif	/* _BBSLIB_H_ */
