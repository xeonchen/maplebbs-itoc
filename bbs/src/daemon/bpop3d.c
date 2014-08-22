/*-------------------------------------------------------*/
/* util/bpop3d.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : Simple POP3 server for BBS user		 */
/* create : 96/05/10				 	 */
/* update : 96/11/15				 	 */
/*-------------------------------------------------------*/
/* notice : single process concurrent server		 */
/*-------------------------------------------------------*/


#include "bbs.h"


#undef	DEBUG
#define	WATCH_DOG
#define SERVER_USAGE


#include <netinet/tcp.h>
#include <sys/wait.h>
#include <sys/resource.h>


#ifdef	HAVE_DNS_LOOKUP
#include "dns.ic"
#endif


#define POP3_HOME	(BBSHOME "/usr")
#define POP3_TIMEOUT	(60 * 30)

#define	POP3_LOGFILE	(BBSHOME "/run/pop3.log")
#define	POP3_PIDFILE	(BBSHOME "/run/pop3.pid")
#define POP3_DEBUGFILE	(BBSHOME "/run/pop3.debug")

#define	POP3_FQDN	(".bbs@" MYHOSTNAME)

#define	SOCK_BACKLOG	3


#define	SNDBUFSIZ	(256 * 14)
#define	SNDLINSIZ	256 /* Thor.990522: 註解: 送出每行最長 */
#define	RCVBUFSIZ	128 /* Thor.990522: 註解: 收到每行最長 */


/* ----------------------------------------------------- */
/* MapleBBS pop3d message strings			 */
/* ----------------------------------------------------- */


#define	POP3_BYE_MSG	"+OK POP3 server sign off\r\n"
#define	POP3_BYE_LEN	(sizeof(POP3_BYE_MSG) - 1)


#define	POP3_ERRCMD_MSG	"-ERR invalid command\r\n"
#define	POP3_ERRARG_MSG	"-ERR invalid argument\r\n"
#define POP3_ERRNUM_MSG "-ERR message number out of range\r\n"
#define	POP3_DELETE_MSG	"-ERR message has been deleted\r\n"


/* ----------------------------------------------------- */
/* POPH : pop3header extended from HDR			 */
/* ----------------------------------------------------- */


typedef struct
{
  time_t chrono;		/* timestamp */
  int xmode;

  int xid;			/* reserved */

  char xname[24];		/* 檔案名稱 */
  int pmode;			/* Thor.990522: 是否刪信 等 */
  int psize;			/* Thor.990522: 本信傳送總大小 */

  char owner[80];		/* 作者 (E-mail address) */
  char nick[50];		/* 暱稱 */

  char date[9];			/* [96/12/01] */
  char title[TTLEN + 1];	/* 主題 */
}      POPH;


/* ----------------------------------------------------- */
/* client connection structure				 */
/* ----------------------------------------------------- */


typedef struct Client
{
  struct Client *next;
  int sno;			/* serial number */

  int sock;
  int state;
  time_t tbirth;
  time_t uptime;

  int mode;
#ifdef DEBUG
  int debug;
#endif
  int pcount;			/* Thor.990522: 信件數 */
  int pbytes;			/* Thor.990522: 總大小 */
  POPH *cache;			/* Thor.990522: 存 .DIR */

  char userid[IDLEN + 1];
  char passwd[PASSLEN + 1];
  char home[IDLEN + 4];		/* Thor.990122: 4個字元中, 2個是'/', 1個是英文字 1個是'\0' */

  char pool[SNDBUFSIZ];		/* output pool */
  int locus;			/* Thor.990522: output pool 內可送data之量 */

  char *body;			/* mail body */
  char *bptr;			/* Thor.990522: body pointer, 已送多少 */

  int lcur;			/* Thor.990522: 目前送至哪行 */
  int lmax;			/* Thor.990522: 最大行數 */

  int xbytes;			/* Thor.990522: 傳送總量 */
}      Client;


/* ----------------------------------------------------- */
/* connection state					 */
/* ----------------------------------------------------- */


#define	CS_FREE		0x00	/* Thor.990522: 目前未用 */
#define	CS_READ		0x01	/* Thor.990522: 等待user輸入 */
#define	CS_INDEX	0x02	/* Thor.990522: 信箱條列 */
#define	CS_UIDL		0x03	/* Thor.990522: uniq id條列 */
#define	CS_FILE		0x04	/* Thor.990522: 信件傳送中 */
#define	CS_WRITE	0x05	/* Thor.990522: 傳送中 */
#define	CS_FLUSH	0x06	/* Thor.990522: 最終一次傳送 */


#define	CM_LOGIN	1
#define	CM_DIRTY	2	/* 有刪除信件 */


/* ----------------------------------------------------- */
/* operation log and debug information			 */
/* ----------------------------------------------------- */


static FILE *flog;		/* log file descriptor */
static int gline;


#ifdef	WATCH_DOG
# define MYDOG	gline = __LINE__
#else
# define MYDOG			/* NOOP */
#endif


static void
logit(key, msg)
  char *key;
  char *msg;
{
  time_t now;
  struct tm *p;

  time(&now);
  p = localtime(&now);
  fprintf(flog, "%02d/%02d %02d:%02d:%02d %-8s%s\n",
    p->tm_mon + 1, p->tm_mday,
    p->tm_hour, p->tm_min, p->tm_sec, key, msg);
}


static void
log_init()
{
  FILE *fp;

  if (fp = fopen(POP3_PIDFILE, "w"))
  {
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
  }

  flog = fopen(POP3_LOGFILE, "a");
  logit("START", "pop3 daemon");
}


/* ----------------------------------------------------- */
/* parse string token					 */
/* ----------------------------------------------------- */


#define	LOWER	1


static char *trail_token;


static char *
parse_token(str, lower)
  char *str;
  int lower;
{
  char *token;
  int ch;

  if (str == NULL)
  {
    str = trail_token;
    if (str == NULL)
      return NULL;
  }

  token = NULL;
  while (ch = *str)
  {
    if (ch == ' ' /* || ch == '\t' || ch == '\r' || ch == '\n' */ )
    {
      if (token)
      {
	*str++ = '\0';
	break;
      }
    }
    else
    {
      if (token == NULL)
	token = str;

      if (lower && ch >= 'A' && ch <= 'Z')
	*str = ch | 0x20;
    }
    str++;
  }

  trail_token = str;
  return token;
}


/* ----------------------------------------------------- */
/* server side stuff					 */
/* ----------------------------------------------------- */


static void
mbox_open(cn)
  Client *cn;
{
  int fd, fsize, pbytes, pad;
  struct stat st;
  char fpath[80], *fname;
  POPH *hdr;

  cn->pcount = cn->pbytes = 0;
  /* Thor.990522: cache, body 等, 已在 accept時initial */

  sprintf(fpath, "%s"FN_DIR, cn->home);
  fd = open(fpath, O_RDONLY);

  if (fd < 0)
    return;

  if (fstat(fd, &st) || (fsize = st.st_size) < sizeof(HDR))
  {
    close(fd);
    return;
  }

  hdr = (POPH *) malloc(fsize);
  fsize = read(fd, hdr, fsize) / sizeof(HDR);
  close(fd);

  if (fsize <= 0)
  {
    free(hdr);
    return;
  }

  cn->cache = hdr;
  cn->pcount = fsize;
  pbytes = 0;
  fname = strchr(fpath, '.');
  *fname++ = '@';
  *fname++ = '/';

  pad = 10 + strlen(cn->userid) + strlen(POP3_FQDN) + 9 + 44;
  for (;;)
  {
    char *author;
    int psize;

    strcpy(fname, hdr->xname);
    psize = 0;
    if (!stat(fpath, &st))
      psize = st.st_size;

    author = hdr->owner;
    if (!strchr(author, '@'))
      strcat(author, POP3_FQDN);

    psize += pad + strlen(author) + strlen(hdr->title) + 10 + 40 + 80;

    hdr->pmode = 0;
    hdr->psize = psize;
    pbytes += psize;

    if (--fsize == 0)
      break;

    hdr++;
  }
  cn->pbytes = pbytes;
}


static void
mbox_read(cn, phdr, lmax)
  Client *cn;
  POPH *phdr;
  int lmax;	/* Thor.990522: 允許之總行數 */
{
  char *pool, *head, *body, buf[80];
  /* struct tm *mytm; */
  int fd;

  /* mytm = localtime(&phdr->chrono);
  strftime(buf, 46, "%a, %e %h %Y %T +0800 (%Z)", mytm); */
  

  pool = cn->pool;
  sprintf(pool, "+OK %d octets\r\nFrom: %s\r\nTo: %s%s\r\n"
    "Subject: %s\r\nDate: %s\r\n%s\r\n",
    phdr->psize, phdr->owner, cn->userid, POP3_FQDN,
    phdr->title, Atime(&phdr->chrono), (phdr->xmode & MAIL_READ ? "Status: RO\r\n" : ""));

  head = pool + strlen(pool);
  if (!lmax)
  {
    *head++ = '.';
    *head++ = '\r';
    *head++ = '\n';
    cn->locus = head - pool;
    cn->state = CS_WRITE;
    return;
  }

  cn->locus = head - pool;
  cn->state = CS_FILE;

  if (body = cn->body)
  {
    /* Thor.990522: 有執行的機會嗎? */
    free(body);
  }
  body = NULL;

  sprintf(buf, "%s@/%s", cn->home, phdr->xname);
  fd = open(buf, O_RDONLY);
  if (fd >= 0)
  {
    int size;
    struct stat st;

    if (!fstat(fd, &st) && ((size = st.st_size) > 0))
    {
      if (body = (char *) malloc(size + 1))
      {
	size = read(fd, body, size);
	if (size > 0)
	{
	  body[size] = '\0';
	}
	else
	{
	  free(body);
	  body = NULL;
	}
      }
    }
#if 0
    else
    {
      phdr->pmode = MAIL_DELETE;
      *head++ = '.';
      *head++ = '\r';
      *head++ = '\n';
      cn->locus = head - pool;
      cn->state = CS_WRITE;
      lmax = 0;
    }
#endif
    close(fd);
  }

  if (!body) /* Thor.991220: for open fail, size==0, and read fail */
  {
    phdr->pmode = MAIL_DELETE;
    cn->mode = CM_DIRTY; /* Thor.991221: for incorrect mail delete */
    *head++ = '.';
    *head++ = '\r';
    *head++ = '\n';
    cn->locus = head - pool;
    cn->state = CS_WRITE;
    lmax = 0;
  }

  cn->bptr = cn->body = body;
  cn->lcur = 0;
  cn->lmax = lmax;
}


static void
mbox_close(cn)
  Client *cn;
{
  int n, max, saved;
  FILE *fpr, *fpw;
  char fpath[80], fold[80], fnew[80], *fname;
  HDR mhdr;
  POPH *phdr;

  sprintf(fold, "%s"FN_DIR, cn->home);
  if ((fpr = fopen(fold, "r")) == NULL)
    return;

  fpw = f_new(fold, fnew);
  if (fpw == NULL)
  {
    fclose(fpr);
    return;
  }

  strcpy(fpath, fold);
  fname = strchr(fpath, '.');
  *fname++ = '@';
  *fname++ = '/';

  n = saved = 0;
  max = cn->pcount;
  phdr = cn->cache;

  while (fread(&mhdr, sizeof(mhdr), 1, fpr) == 1)
  {
    if (n >= max || !phdr->pmode)
    {
      saved = fwrite(&mhdr, sizeof(mhdr), 1, fpw);
      if (saved <= 0)
      {
	saved = -1;
	break;
      }
    }
    else
    {
      strcpy(fname, mhdr.xname);
      unlink(fpath);
    }
    n++;
    phdr++;
  }
  fclose(fpr);
  fclose(fpw);

  if (saved < 0)
  {
    unlink(fnew);
  }
  else if (saved)
  {
    rename(fnew, fold);
  }
  else
  {
    unlink(fnew);
    unlink(fold);
  }
}


static void
mbox_index(cn, state)
  Client *cn;
  int state;
{
  char *pool, *head, *tail;
  int cur, max;
  POPH *hdr;

  pool = cn->pool;
  tail = pool + SNDBUFSIZ - 32;

  cur = cn->lcur;
  if (cur)
  {
    head = pool + cn->locus;
  }
  else
  {
    memcpy(pool, "+OK\r\n", 5);
    head = pool + 5;
  }

  hdr = &cn->cache[cur];
  max = cn->pcount;
  do
  {
    cur++;

    MYDOG;

    if (cur > max)
    {
      *head++ = '.';
      *head++ = '\r';
      *head++ = '\n';
      cn->state = CS_WRITE;
      break;
    }

    if (!hdr->pmode)
    {
      if (state == CS_UIDL)
	sprintf(head, "%d %s\r\n", cur, hdr->xname + 1);
      else
	sprintf(head, "%d %d\r\n", cur, hdr->psize);
      head += strlen(head);
    }
    hdr++;

  } while (head < tail);

  MYDOG;

  cn->locus = head - pool;
  cn->lcur = cur;
}


static void
mbox_file(cn)
  Client *cn;
{
  int lcur, lmax, ch, cx, cc; /* Thor.990522: 控制單行最大傳送量, Char Count */
  char *bptr, *pool, *head, *tail;

  /* send out a text file in MIME mode */

  bptr = cn->bptr;
  if (!bptr)
    return;

  MYDOG;

  pool = cn->pool;
  head = pool + cn->locus;
  tail = pool + SNDBUFSIZ - SNDLINSIZ - 4 - 3; /* Thor.991129: 結尾的.\r\n 3字 */

  lcur = cn->lcur;
  lmax = cn->lmax;
  cx = '\n'; cc = 0;

  while (ch = *bptr)
  {
    /* Thor.990522: 控制單行最大傳送量, Char Count. 寫得醜了點, 會跑就好:p */
    if (cc++ >= SNDLINSIZ)
      ch = '\n';
    else
      bptr++;

    if (ch == '\r')
      continue;

    if (ch == '\n')
    {
      cc = 0; /* Thor.990522: 控制單行最大傳送量, Char Count */

      *head++ = '\r';
      *head++ = '\n';
      if (++lcur >= lmax)
	break;

      if (head > tail)
      {
	cn->state = CS_FILE;
	cn->bptr = bptr;
	cn->lcur = lcur;
	cn->locus = head - pool;
	return;
      }
      cx = ch;
      continue;
    }

    if (ch == '.' && cx == '\n')
      *head++ = ch;

    *head++ = ch;
    cx = ch;
  }

  /* end of mail body transmission */

#if 1
  /* Thor.981206: Odysseus patch:
 作者  Odysseus.bbs@bbs.cs.nccu.edu.tw (由奢入儉難~~),      看板  plan
 標題  Re: bpop3d?
 時間  政大貓空行館 (Thu Oct 15 01:19:56 1998)
───────────────────────────────────────
※ 引述《dreamer.bbs@Rouge.Dorm10.NCTU.edu.tw (碎心拼圖)》之銘言：
:         但是實際用 Netscape Messager 去試試看,
:         得到總共信件數目之後,
:         當要去 retreive 第一封信就當住了....-_-
:         ps. Solaris 2.5.1, Maple 3.02 
  */
  *head++ = '\r';
  *head++ = '\n';
#endif

  *head++ = '.';
  *head++ = '\r';
  *head++ = '\n';
  cn->locus = head - pool;
  cn->state = CS_WRITE;

  MYDOG;
  free(cn->body);
  MYDOG;
  cn->bptr = cn->body = NULL;
}


/* ----------------------------------------------------- */
/* supporting commands                         		 */
/* ----------------------------------------------------- */


static void cmd_xxxx();
static void client_flush(); /* Thor.991221: 註解: 不用在msg加 \r\n, 程式會加 */

/* Thor.991221: 註解: 要在msg加 \r\n, 程式不加 */
#define CMD_MSG(cn, msg)	\
  cn->state = CS_WRITE; memcpy(cn->pool, msg, cn->locus = sizeof(msg) - 1);


static void
do_argument(cn)
  Client *cn;
{
  CMD_MSG(cn, POP3_ERRARG_MSG);
}


/* return value : -1(all), 0, n */


static int
do_number(cn, n)
  Client *cn;
  int n;			/* 0 ==> exact 1 arg  -1 ==> 1 or 0 arg */
{
  char *cmd;

  if (cn->mode < CM_LOGIN)
  {
    cmd_xxxx(cn);
    return 0;
  }

  cmd = parse_token(NULL, 0);

  if (!cmd || !*cmd)
  {
    if (n == 0)
      do_argument(cn);
    return n;
  }

  n = atoi(cmd);
  if (n <= 0 || n > cn->pcount)
  {
    CMD_MSG(cn, POP3_ERRNUM_MSG);
    return 0;
  }

  return n;
}


/* ----------------------------------------------------- */
/* main commands                            		 */
/* ----------------------------------------------------- */


static void
cmd_xxxx(cn)
  Client *cn;
{
  CMD_MSG(cn, POP3_ERRCMD_MSG);
}


static void
cmd_noop(cn)
  Client *cn;
{
  CMD_MSG(cn, "+OK\r\n");
}


static void
cmd_user(cn)
  Client *cn;
{
  int fd;
  ACCT acct;
  char *userid, *ptr, fpath[80], msg[128];
  char *msg_no_user = "-ERR no such user in our server";

  MYDOG;

  if (cn->mode >= CM_LOGIN)
  {
    cmd_xxxx(cn);
    return;
  }

  userid = parse_token(NULL, LOWER);

  if (!userid || !*userid)
  {
    do_argument(cn);
    return;
  }

  /* userid is "userid.bbs" or "userid" */

  if (ptr = strchr(userid, '.'))
  {
    if (strcmp(ptr, ".bbs"))
    {
      client_flush(cn, msg_no_user);
      return;
    }
    *ptr = '\0';
  }

  /* Thor.981122: chc@gaisnews.iis.sinica.edu.tw patch: 
     可能發生的問題:
     當 connect 到 bpop3d 時, 如果輸入 userid 太長, 就會造成 overflow, 
     狀況輕時會 disconnect, 嚴重時會使 bpop3d segmentation fault. 
     有心人士可能會以此破壞系統的安全性. */
  /* Thor.990122: check 完 *.bbs 再看 idlen */
  if (strlen(userid) > IDLEN)
  {
    client_flush(cn, msg_no_user);
    return;
  }

#ifdef DEBUG
  if (!strcmp(userid, STR_SYSOP))	/* 只有 sysop 取信才 debug */
  {
    cn->debug = 1;
  }
#endif

  ptr = cn->home;
  sprintf(ptr, "%c/%s/", *userid, userid);
  sprintf(fpath, "%s.ACCT", ptr);
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    read(fd, &acct, sizeof(ACCT));
    close(fd);

    if (acct.userlevel & PERM_DENYLOGIN)
    {
      sprintf(msg, "-ERR %s denied", acct.userid);
    }
    else
    {
      strcpy(cn->userid, acct.userid);
      memcpy(cn->passwd, acct.passwd, PASSLEN + 1);
      /* SoC: follow the behavior of qpopper */
      /* sprintf(msg, "+OK Password required for %s%s", acct.userid, POP3_FQDN); */
      sprintf(msg, "+OK Password required for %s.", acct.userid);
    }
  }
  else
  {
    strcpy(msg, msg_no_user);
  }

  client_flush(cn, msg);
}


static void
cmd_password(cn)
  Client *cn;
{
  char *cmd, *passwd, msg[128];

  MYDOG;

  if (cn->mode >= CM_LOGIN)
  {
    cmd_xxxx(cn);
    return;
  }

  passwd = cn->passwd;
  MYDOG;
  if (!*passwd)
  {
    CMD_MSG(cn, "-ERR need USER command\r\n"); /* Thor.991221: 加上 \r\n */
    return;
  }

  cmd = trail_token;		/* to support password with [space] */

  /* cmd = parse_token(NULL, 0); */

  if (!cmd || !*cmd)
  {
    do_argument(cn);
    return;
  }

  MYDOG;
  /* Thor.990214: 統一用 dao library */
  /* if (!checkpasswd(passwd, cmd)) */
  if (chkpasswd(passwd, cmd))
  {
    fprintf(flog, "PASS\t[%d]\t%s\n", cn->sno, cn->userid);

  MYDOG;
    *passwd = '\0';
    CMD_MSG(cn, "-ERR password incorrect\r\n"); /* Thor.991221: 加上 \r\n */
    return;
  }

  MYDOG;
  mbox_open(cn);

  MYDOG;
  sprintf(msg, "+OK %s has %d messages (%d octets)",
    cn->userid, cn->pcount, cn->pbytes);

  MYDOG;
  client_flush(cn, msg);

  MYDOG;
  fprintf(flog, "ENTER\t[%d]%s\n", cn->sno, msg + 3);

  MYDOG;
  cn->mode = CM_LOGIN;
}


static void
cmd_stat(cn)
  Client *cn;
{
  char *ptr;
  
  if (cn->mode < CM_LOGIN)
  {
    cmd_xxxx(cn);
    return;
  }

  ptr = cn->pool;
  sprintf(ptr, "+OK %d %d\r\n", cn->pcount, cn->pbytes);
  cn->locus = strlen(ptr);
  cn->state = CS_WRITE;
}


static void
cmd_last(cn)
  Client *cn;
{
  char *ptr;

  if (cn->mode < CM_LOGIN)
  {
    cmd_xxxx(cn);
    return;
  }

  ptr = cn->pool;
  sprintf(ptr, "+OK %d\r\n", cn->pcount);
  cn->locus = strlen(ptr);
  cn->state = CS_WRITE;
}


static void
cmd_reset(cn)
  Client *cn;
{
  int n, max;
  POPH *phdr;
  char *ptr;

  n = cn->mode;
  if (n < CM_LOGIN)
  {
    cmd_xxxx(cn);
    return;
  }

  max = cn->pcount;
  if (n == CM_DIRTY)
  {
    for (n = 0, phdr = cn->cache; n < max; phdr++, n++)
      phdr->pmode = 0;
  }

  ptr = cn->pool;
  sprintf(ptr, "+OK mail reset %d messages %d octets\r\n", max, cn->pbytes);
  cn->locus = strlen(ptr);
  cn->mode = CM_LOGIN;
  cn->state = CS_WRITE;
}


static void
cmd_retrive(cn)
  Client *cn;
{
  int n;
  POPH *phdr;

  n = do_number(cn, 0);
  if (!n)
    return;

  phdr = &cn->cache[n - 1];
  if (phdr->pmode)
  {
    CMD_MSG(cn, POP3_DELETE_MSG);
    return;
  }

  mbox_read(cn, phdr, 0x400000);
}


static void
cmd_top(cn)
  Client *cn;
{
  int n;
  POPH *phdr;
  char *cmd;

  MYDOG;

  n = do_number(cn, 0);
  if (!n)
    return;

  phdr = &cn->cache[n - 1];
  if (phdr->pmode)
  {
    CMD_MSG(cn, POP3_DELETE_MSG);
    return;
  }

  cmd = parse_token(NULL, 0);

  if (!cmd || !*cmd)
  {
    do_argument(cn);
    return;
  }

  n = atoi(cmd);
  if (n < 0)
  {
    CMD_MSG(cn, POP3_ERRNUM_MSG);
    return;
  }

  mbox_read(cn, phdr, n);
}


static void
cmd_delete(cn)
  Client *cn;
{
  int n;
  POPH *phdr;

  MYDOG;

  if (!(n = do_number(cn, 0)))
    return;

  phdr = &cn->cache[n - 1];
  if (phdr->pmode)
  {
    CMD_MSG(cn, POP3_DELETE_MSG);
    return;
  }

  phdr->pmode = MAIL_DELETE;
  cn->mode = CM_DIRTY;
  CMD_MSG(cn, "+OK message deleted\r\n");
}


static void
cmd_list(cn)
  Client *cn;
{
  int n;
  POPH *phdr;
  char buf[80];

  MYDOG;

  n = do_number(cn, -1);

  MYDOG;

  if (n < 0)
  {
    cn->lcur = 0;
    cn->state = CS_INDEX;
  }
  else if (n > 0)
  {
    phdr = &cn->cache[n - 1];
    if (phdr->pmode)
    {
      CMD_MSG(cn, POP3_DELETE_MSG);
    }
    else
    {
      sprintf(buf, "+OK %d %d", n, phdr->psize);
      client_flush(cn, buf);
    }
  }
}


static void
cmd_uidl(cn)
  Client *cn;
{
  int n;
  POPH *phdr;
  char buf[80];

  n = do_number(cn, -1);

  if (n < 0)
  {
    cn->lcur = 0;
    cn->state = CS_UIDL;
  }
  else if (n > 0)
  {
    phdr = &cn->cache[n - 1];
    if (phdr->pmode)
    {
      CMD_MSG(cn, POP3_DELETE_MSG);
    }
    else
    {
      sprintf(buf, "+OK %d %s", n, phdr->xname + 1);
      client_flush(cn, buf);
    }
  }
}


static void
cmd_quit(cn)
  Client *cn;
{
  if (cn->mode >= CM_DIRTY)
  {
    mbox_close(cn);
  }

  fprintf(flog, "QUIT\t[%d] %s S%d T%d\n",
    cn->sno, cn->userid, cn->xbytes, time(0) - cn->tbirth);

  cn->state = CS_FLUSH;
  strcpy(cn->pool, POP3_BYE_MSG);
  cn->locus = POP3_BYE_LEN;
}


typedef struct
{
  char *cmd;
  void (*fun) ();
}      PopCmd;


static PopCmd cmdlist[] =
{
  "list", cmd_list,
  "uidl", cmd_uidl,
  "retr", cmd_retrive,
  "dele", cmd_delete,

  "user", cmd_user,
  "pass", cmd_password,
  "stat", cmd_stat,
  "quit", cmd_quit,

  "last", cmd_last,
  "top", cmd_top,

  "rset", cmd_reset,
  "noop", cmd_noop,
  NULL, cmd_xxxx
};


/* ----------------------------------------------------- */
/* put a line into client's buffer, padding with "\r\n"	 */
/* ----------------------------------------------------- */


static void
client_flush(cn, msg)
  Client *cn;
  char *msg;
{
  char *pool, *head;

  head = pool = cn->pool;


#if 0	/* itoc.010606: 如果 client 是以換行字元來判別訊息結束的話，就會卡死了 */
  while (*head++ = *msg++)
    ; 
#endif
  while (*head = *msg++)
    head++;
  
  
  /* SoC: Previous trick causes the output msg ending with "00 0D 0A" */
  head--;

  *head++ = '\r';
  *head++ = '\n';
  cn->locus = head - pool;
  cn->state = CS_WRITE;
}


/* ----------------------------------------------------- */
/* send data to client					 */
/* ----------------------------------------------------- */


static int
client_write(cn)
  Client *cn;
{
  int len, cc;
  char *data;

  len = cn->locus;
  data = cn->pool;
  MYDOG;
  cc = send(cn->sock, data, len, 0);
  MYDOG;

  if (cc <= 0)
  {
  MYDOG;
    cc = errno;
    if (cc != EWOULDBLOCK)
    {
      fprintf(flog, "SEND\t[%d] %s\n", cn->sno, strerror(cc));
      return 0;
    }

    return -1;			/* would block, so leave it to do later */
  }

#ifdef DEBUG
  if (cn->debug)
  {
    char msg[SNDBUFSIZ + 10];
    sprintf(msg, "%s\t[%d]bpop3>>>\n", Now(), cn->sno);
    f_cat(POP3_DEBUGFILE, msg);
    str_ncpy(msg, data, len+1);
    f_cat(POP3_DEBUGFILE, msg);
    sprintf(msg, "\t[%d]bpop3<<<\n", cn->sno);
    f_cat(POP3_DEBUGFILE, msg);
  MYDOG;
  }
#endif

  MYDOG;
  cn->xbytes += cc;
  MYDOG;
  len -= cc;
  MYDOG;
  cn->locus = len;
  MYDOG;
  if (len)
  {
  MYDOG;
    memcpy(data, data + cc, len);
  MYDOG;
    return cc;
  }

  MYDOG;
  len = cn->state;
  MYDOG;

  if (len == CS_FLUSH)
    return 0;

  if (len == CS_WRITE)
    cn->state = CS_READ;

  MYDOG;
  return cc;
}


/* ----------------------------------------------------- */
/* client's service dispatcher				 */
/* ----------------------------------------------------- */


static void
client_serve(cn)
  Client *cn;
{
  char *cmd, *str;
  PopCmd *pc;

  cn->locus = 0;		/* reset buffer pool position */

  cmd = parse_token(cn->pool, LOWER);

  if (!cmd || !*cmd)
    return;

  for (pc = cmdlist; str = pc->cmd; pc++)
  {
    if (!strcmp(cmd, str))
      break;
  }

#ifdef	DEBUG
  logit(cmd, cn->userid);
#endif

   MYDOG;
  (*pc->fun) (cn);
   MYDOG;

#ifdef	DEBUG
  {
    char buf[80];

    /* Thor.990222: 怕 str找不到cmd時 為 NULL */
    /* sprintf(buf, "%s-", str); */
    sprintf(buf, "%s-", cmd);
    logit(buf, cn->userid); 
  }
#endif
}


/* ----------------------------------------------------- */
/* receive command from client				 */
/* ----------------------------------------------------- */


static int
client_read(cn)
  Client *cn;
{
  int cc, pos, len;
  char *str;

  MYDOG;

  pos = cn->locus;
  str = &cn->pool[pos];
  len = recv(cn->sock, str, RCVBUFSIZ, 0);

  if (len <= 0)
  {
    cc = errno;
    if (cc != EWOULDBLOCK)
    {
      fprintf(flog, "RECV\t[%d] %s\n", cn->sno, strerror(cc));
      return 0;
    }

    return -1;
  }

  str[len] = '\0';

#ifdef DEBUG
  if (cn->debug)
  {
    char msg[80];
    sprintf(msg, "%s\t[%d]peer>>>\n", Now(), cn->sno);
    f_cat(POP3_DEBUGFILE, msg);
    f_cat(POP3_DEBUGFILE, str);
    sprintf(msg, "\t[%d]peer<<<\n", cn->sno);
    f_cat(POP3_DEBUGFILE, msg);
  }
#endif

  while (cc = *str)
  {
    switch (cc)
    {
    case '\r':
    case '\n':
      *str = '\0';
      client_serve(cn);
      return 1;

    case '\t':
      *str = ' ';
    }
    str++;
  }

  pos += len;
  if (pos >= RCVBUFSIZ)
  {
    sprintf(str, "[%d] buffer overflow", cn->sno);
    logit("HACK", str);
    return 0;
  }

  cn->locus = pos;
  return 1;
}


/* ----------------------------------------------------- */
/* release idle/timeout connections			 */
/* ----------------------------------------------------- */


static void
client_close(cn)
  Client *cn;
{
  int sock;
  char *ptr;

  if (cn->mode >= CM_LOGIN)
  {
    if (ptr = (char *) cn->cache)
      free(ptr);

    if (ptr = cn->body)
      free(ptr);
  }

  sock = cn->sock;
  shutdown(sock, 2);
  close(sock);
}


/* ----------------------------------------------------- */
/* server core routines					 */
/* ----------------------------------------------------- */


static void
/* start_daemon() */
servo_daemon(inetd)
  int inetd;
{
  int fd, value;
  char buf[80];
  struct sockaddr_in sin;
  struct linger ld;
#ifdef HAVE_RLIMIT
  struct rlimit limit;
#endif

  /*
   * More idiot speed-hacking --- the first time conversion makes the C
   * library open the files containing the locale definition and time zone.
   * If this hasn't happened in the parent process, it happens in the
   * children, once per connection --- and it does add up.
   */

  time((time_t *)&value);
  gmtime((time_t *)&value);
  strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", localtime((time_t *)&value));

#ifdef HAVE_RLIMIT
  /* --------------------------------------------------- */
  /* adjust the resource limit				 */
  /* --------------------------------------------------- */

  limit.rlim_cur = limit.rlim_max = 8 * 1024 * 1024;
  setrlimit(RLIMIT_DATA, &limit);

#ifdef SOLARIS
#define RLIMIT_RSS RLIMIT_AS	/* Thor.981206: port for solaris 2.6 */
#endif

  setrlimit(RLIMIT_RSS, &limit);

  limit.rlim_cur = limit.rlim_max = 0;
  setrlimit(RLIMIT_CORE, &limit);
#endif

  /* --------------------------------------------------- */
  /* detach daemon process				 */
  /* --------------------------------------------------- */

  close(2);
  close(1);

  if (inetd)
    return;

  close(0);

  if (fork())
    exit(0);

  setsid();

  if (fork())
    exit(0);

  /* --------------------------------------------------- */
  /* setup socket					 */
  /* --------------------------------------------------- */

  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  value = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &value, sizeof(value));

  ld.l_onoff = ld.l_linger = 0;
  setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld));

  sin.sin_family = AF_INET;
  sin.sin_port = htons(POP3_PORT);
  sin.sin_addr.s_addr = INADDR_ANY;
  memset(sin.sin_zero, 0, sizeof(sin.sin_zero));

  if (bind(fd, (struct sockaddr *) & sin, sizeof(sin)) ||
    listen(fd, SOCK_BACKLOG))
    exit(1);
}


static void
abort_server()
{
  fclose(flog);
  exit(0);
}


static void
reaper()
{
  while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
}


static void
sig_trap(sig)
  int sig;
{
  char buf[32];

  sprintf(buf, "[%d] at %d (err: %d)", sig, gline, errno);
  logit("TRAP", buf);
  abort_server();
}


#ifdef	SERVER_USAGE
static void
server_usage()
{
  struct rusage ru;

  if (getrusage(RUSAGE_SELF, &ru))
    return;

  fprintf(flog, "\n[Server Usage]\n\n"
    "user time: %.6f\n"
    "system time: %.6f\n"
    "maximum resident set size: %lu P\n"
    "integral resident set size: %lu\n"
    "page faults not requiring physical I/O: %ld\n"
    "page faults requiring physical I/O: %ld\n"
    "swaps: %ld\n"
    "block input operations: %ld\n"
    "block output operations: %ld\n"
    "messages sent: %ld\n"
    "messages received: %ld\n"
    "signals received: %ld\n"
    "voluntary context switches: %ld\n"
    "involuntary context switches: %ld\n"
    "gline: %d\n\n",

    (double) ru.ru_utime.tv_sec + (double) ru.ru_utime.tv_usec / 1000000.0,
    (double) ru.ru_stime.tv_sec + (double) ru.ru_stime.tv_usec / 1000000.0,
    ru.ru_maxrss,
    ru.ru_idrss,
    ru.ru_minflt,
    ru.ru_majflt,
    ru.ru_nswap,
    ru.ru_inblock,
    ru.ru_oublock,
    ru.ru_msgsnd,
    ru.ru_msgrcv,
    ru.ru_nsignals,
    ru.ru_nvcsw,
    ru.ru_nivcsw,
    gline);

  fflush(flog);
}
#endif


/* ----------------------------------------------------- */
/* signal routines					 */
/* ----------------------------------------------------- */


static void
main_signals()
{
  struct sigaction act;

  /* sigblock(sigmask(SIGPIPE)); */ /* Thor.981206: 統一 POSIX 標準用法  */

  /* act.sa_mask = 0; */ /* Thor.981105: 標準用法 */
  sigemptyset(&act.sa_mask);      
  act.sa_flags = 0;

  act.sa_handler = sig_trap;
  sigaction(SIGBUS, &act, NULL);
  sigaction(SIGSEGV, &act, NULL);

  act.sa_handler = reaper;
  sigaction(SIGCHLD, &act, NULL);

  act.sa_handler = abort_server;
  sigaction(SIGTERM, &act, NULL);

#ifdef  SERVER_USAGE
  act.sa_handler = server_usage;
  sigaction(SIGPROF, &act, NULL);
#endif

  /* Thor.981206: lkchu patch: 統一 POSIX 標準用法  */
  /* 在此借用 sigset_t act.sa_mask */
  sigaddset(&act.sa_mask, SIGPIPE);
  sigprocmask(SIG_BLOCK, &act.sa_mask, NULL);

}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int main_sno, csock, nfds, state;
  Client **FBI, *Scully, *Mulder, *cn;
  fd_set rset, wset, xset;
  struct timeval tv;
  time_t uptime, tcheck;

  state = 0;

  while ((nfds = getopt(argc, argv, "hid")) != -1)
  {
    switch (nfds)
    {
    case 'i':
      state = 1;
      break;

    case 'd':
      break;

    case 'h':
    default:

      fprintf(stderr, "Usage: %s [options]\n"
        "\t-i  start from inetd with wait option\n"
        "\t-d  debug mode\n"
        "\t-h  help\n",
        argv[0]);
      exit(0);
    }
  }

  servo_daemon(state);
  /* start_daemon(); */

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(POP3_HOME);
  main_signals();

  umask(077);
  log_init();

#if 0
  dns_init();
#endif

  tv.tv_sec = POP3_TIMEOUT;
  tv.tv_usec = 0;

  tcheck = time(0) + POP3_TIMEOUT;
  Scully = Mulder = NULL;
  main_sno = 0;

  for (;;)
  {
    /* resource and garbage collection */

    uptime = time(0);
    if (tcheck < uptime)
    {
      tcheck = uptime - POP3_TIMEOUT;

      for (FBI = &Scully; cn = *FBI;)
      {
	if (cn->uptime < tcheck)
	{
	  client_close(cn);

	  *FBI = cn->next;

	  cn->next = Mulder;
	  Mulder = cn;
	}
	else
	{
	  FBI = &(cn->next);
	}
      }

      fflush(flog);
      tcheck = uptime + POP3_TIMEOUT;
    }

    /* ------------------------------------------------- */
    /* Set up the fdsets				 */
    /* ------------------------------------------------- */

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&xset);

    FD_SET(0, &rset);
    nfds = 0;

    for (cn = Scully; cn; cn = cn->next)
    {
      csock = cn->sock;
      state = cn->state;

      if (nfds < csock)
	nfds = csock;

      if (state == CS_READ)
      {
	FD_SET(csock, &rset);
      }
      else
      {
	switch (state)
	{
	case CS_FILE:
	  mbox_file(cn);
	  break;

	case CS_INDEX:
	case CS_UIDL:
	  mbox_index(cn, state);
	  break;
	}

	FD_SET(csock, &wset);
      }

      FD_SET(csock, &xset);
    }

    {
    struct timeval tv_tmp = tv;
    /* Thor.981221: for future reservation bug */
    nfds = select(nfds + 1, &rset, &wset, &xset, &tv_tmp);
    }

    if (nfds == 0)
    {
      continue;
    }

    if (nfds < 0)
    {
      csock = errno;
      if (csock != EINTR)
      {
	logit("select", strerror(csock));
      }
      continue;
    }

    /* ------------------------------------------------- */
    /* serve active agents				 */
    /* ------------------------------------------------- */

    uptime = time(0);

    for (FBI = &Scully; cn = *FBI;)
    {
      csock = cn->sock;

      if (FD_ISSET(csock, &wset))
      {
        MYDOG;
	state = client_write(cn);
        MYDOG;
      }
      else if (FD_ISSET(csock, &rset))
      {
        MYDOG;
	state = client_read(cn);
        MYDOG;
      }
      else if (FD_ISSET(csock, &xset))
      {
        MYDOG;
	state = 0;
        MYDOG;
      }
      else
      {
	state = -1;
      }

      if (state == 0)		/* fire this agent */
      {
	client_close(cn);

	*FBI = cn->next;

	cn->next = Mulder;
	Mulder = cn;

	continue;
      }

      if (state > 0)
      {
        MYDOG;
	cn->uptime = uptime;
        MYDOG;
      }

      MYDOG;
      FBI = &(cn->next);
      MYDOG;
    }

    /* ------------------------------------------------- */
    /* serve new connection				 */
    /* ------------------------------------------------- */

    if (FD_ISSET(0, &rset))
    {
      /* Thor.990222: 查出對方 ip */
      int len;
      struct sockaddr_in csin;
      len = sizeof csin;

      for (;;)
      {
	/* csock = accept(0, NULL, NULL); */
        /* Thor.990222: 查出對方 ip */
	csock = accept(0, (struct sockaddr *)&csin, &len);
 
	if (csock > 0)
	  break;

	state = errno;
	if (state != EINTR)
	{
	  logit("accept", strerror(state));
	  break;
	}

	while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
      }

      if (csock <= 0)
	continue;

      if (cn = Mulder)
      {
	Mulder = cn->next;
      }
      else
      {
	cn = (Client *) malloc(sizeof(Client));
      }

      *FBI = cn;

      /* variable initialization */

      /* sprintf(cn->pool, "[%d]", ++main_sno); */
      /* Thor.990222: 查出對方 ip */
      sprintf(cn->pool, "[%d] ip:%08x", ++main_sno, csin.sin_addr.s_addr);
      logit("CONN", cn->pool);

      cn->next = NULL;
      cn->sock = csock;
      cn->sno = main_sno;

#ifdef DEBUG
      cn->debug = 0;
#endif
      cn->mode = 0;
      cn->passwd[0] = '\0';
      cn->cache = NULL;
      cn->body = NULL;
      cn->bptr = NULL;

      cn->state = CS_WRITE;
      sprintf(cn->pool, "+OK MapleBBS POP3 server ready\r\n");
      cn->locus = strlen(cn->pool);
      cn->tbirth = cn->uptime = uptime;
      cn->xbytes = 0;
    }

    /* ------------------------------------------------- */
    /* tail of main loop				 */
    /* ------------------------------------------------- */
  }
}
