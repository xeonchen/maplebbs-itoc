#ifndef _INNTOBBS_H_
#define _INNTOBBS_H_

/* inntobbs.c */
extern char *NODENAME;
extern char *BODY;
extern char *SUBJECT, *FROM, *DATE, *PATH, *GROUP, *MSGID, *SITE, *POSTHOST, *CONTROL;

/* inntobbs.c */
extern int readlines(char *data);

/* history.c */
extern void HISmaint();
extern void HISadd(char *msgid, char *board, char *xname);
extern int *HISfetch(char *msgid, char *board, char *xname);

#endif	/* _INNTOBBS_H_ */
