/*-------------------------------------------------------*/
/* bnntpd.c		( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : BBS's NNTP daemon				 */
/* create : 03/12/14					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"


#include <sys/wait.h>
#include <netinet/tcp.h>
#include <sys/resource.h>


#define SERVER_USAGE


#define BNNTP_PIDFILE	"run/bnntp.pid"
#define BNNTP_LOGFILE	"run/bnntp.log"


#define BNNTP_PERIOD	(60 * 15)	/* 每 15 分鐘 check 一次 */
#define BNNTP_TIMEOUT	(60 * 30)	/* 超過 30 分鐘的連線就視為錯誤 */
#define BNNTP_FRESH	86400		/* 每 1 天整理一次 */


#define TCP_BACKLOG	3
#define TCP_BUFSIZ	4096
#define TCP_LINSIZ	256
#define TCP_RCVSIZ	2048


#define MIN_DATA_SIZE	2048
#define MAX_CMD_LEN	1024


/* Thor.000425: POSIX 用 O_NONBLOCK */

#ifndef O_NONBLOCK
#define M_NONBLOCK  FNDELAY
#else
#define M_NONBLOCK  O_NONBLOCK
#endif

/* ----------------------------------------------------- */
/* SMTP commands					 */
/* ----------------------------------------------------- */


typedef struct
{
  void (*func) ();
  char *cmd;
  int len;		/* strlen(Command.cmd) */
}      Command;


/* ----------------------------------------------------- */
/* client connection structure				 */
/* ----------------------------------------------------- */


typedef struct Agent
{
  struct Agent *anext;
  int sock;
  int sno;
  int state;
  int mode;
  unsigned int ip_addr;

  time_t tbegin;		/* 連線開始時間 */
  time_t uptime;		/* 上次下指令的時間 */

  char newsgroup[BNLEN + 1];	/* 目前所在的看板 */

  char *data;
  int used;
  int size;			/* 目前 data 所 malloc 的空間大小 */
}     Agent;


static int servo_sno = 0;


/* ----------------------------------------------------- */
/* connection state					 */
/* ----------------------------------------------------- */


#define CS_FREE     0x00
#define CS_RECV     0x01
#define CS_SEND     0x02
#define CS_FLUSH    0x03	/* flush data and quit */


/* ----------------------------------------------------- */
/* AM : Agent Mode					 */
/* ----------------------------------------------------- */


#define AM_DROP     0x010	/* swallow command */


/* ----------------------------------------------------- */
/* operation log and debug information			 */
/* ----------------------------------------------------- */
/* @START | ... | time					 */
/* ----------------------------------------------------- */


static FILE *flog;


extern int errno;
extern char *crypt();


static void
log_fresh()
{
  int count;
  char fsrc[64], fdst[64];
  char *fpath = BNNTP_LOGFILE;

  if (flog)
    fclose(flog);

  count = 9;
  do
  {
    sprintf(fdst, "%s.%d", fpath, count);
    sprintf(fsrc, "%s.%d", fpath, --count);
    rename(fsrc, fdst);
  } while (count);

  rename(fpath, fsrc);
  flog = fopen(fpath, "a");
}


static void
logit(key, msg)
  char *key;
  char *msg;
{
  time_t now;
  struct tm *p;

  time(&now);
  p = localtime(&now);
  /* Thor.990329: y2k */
  fprintf(flog, "%s\t%s\t%02d/%02d/%02d %02d:%02d:%02d\n",
    key, msg, p->tm_year % 100, p->tm_mon + 1, p->tm_mday,
    p->tm_hour, p->tm_min, p->tm_sec);
}


static void
log_open()
{
  FILE *fp;

  umask(077);

  if (fp = fopen(BNNTP_PIDFILE, "w"))
  {
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
  }

  flog = fopen(BNNTP_LOGFILE, "a");
  logit("START", "MTA daemon");
}


static inline void
agent_log(ap, key, msg)
  Agent *ap;
  char *key;
  char *msg;
{
  fprintf(flog, "%s\t[%d] %s\n", key, ap->sno, msg);
}


static void
agent_reply(ap, msg)		/* 將 msg 送出去 */
  Agent *ap;
  char *msg;
{
  int cc;
  char *base, *head;

  head = base = ap->data;
  while (cc = *msg++)
  {
    *head++ = cc;
  }
  *head++ = '\r';
  *head++ = '\n';
  ap->used = head - base;
  ap->state = CS_SEND;
}


static void
agent_write(ap, ftemp)		/* 將 ftemp 裡面的內容送出去 */
  Agent *ap;
  char *ftemp;
{
  int fd, fsize;
  char *data;
  struct stat st;

  if ((fd = open(ftemp, O_RDONLY)) >= 0)
  {
    fstat(fd, &st);
    if ((fsize = st.st_size) > 0)
    {
      data = ap->data;
      if (fsize > ap->size)
      {
	ap->data = data = realloc(data, fsize);
	ap->size = fsize;
      }
      read(fd, data, fsize);
    }

    close(fd);
    unlink(ftemp);

    ap->used = fsize;
    ap->state = CS_SEND;
  }
}


/* ----------------------------------------------------- */
/* board：shm 部份須與 cache.c 相容			 */
/* ----------------------------------------------------- */


static BCACHE *bshm;


static void
init_bshm()
{
  /* itoc.030727: 在開啟 bbsd 之前，應該就要執行過 account，
     所以 bshm 應該已設定好 */

  if (bshm)
    return;

  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

  if (bshm->uptime <= 0)	/* bshm 未設定完成 */
    exit(0);
}


/* ----------------------------------------------------- */
/* command dispatch					 */
/* ----------------------------------------------------- */


  /* --------------------------------------------------- */
  /* 特殊指令						 */
  /* --------------------------------------------------- */


static void
cmd_what(ap)
  Agent *ap;
{
  agent_reply(ap, "500 Bad command use");
}


static void
cmd_help(ap)
  Agent *ap;
{
  agent_reply(ap, "100 Legal commands\r\n"
    "  article [Number]\r\n"
    "  group newsgroup\r\n"
    "  head [Number]\r\n"
    "  help\r\n"
    "  list\r\n"
    "  mode reader\r\n"
    "  quit\r\n"
    "  xhdr header [range]\r\n"
    "  xover [range]\r\n"
    "Report problems to the "STR_SYSOP".bbs@"MYHOSTNAME);
}


static void
cmd_quit(ap)
  Agent *ap;
{
  char *data;

  data = ap->data;
  strcpy(data, "205 closing connection - goodbye!\r\n");
  ap->used = strlen(data);
  ap->state = CS_FLUSH;
}


  /* --------------------------------------------------- */
  /* 模式						 */
  /* --------------------------------------------------- */


static void
cmd_mode(ap)
  Agent *ap;
{
  if (ap->data[4] == '\0')	/* mode */
    agent_reply(ap, "reader");
  else				/* mode reader */
    agent_reply(ap, "200 MapleBBS NNTP server ready (read only)");
}


  /* --------------------------------------------------- */
  /* 列出所有看板					 */
  /* --------------------------------------------------- */


static void
cmd_list(ap)
  Agent *ap;
{
  char ftemp[64];
  BRD *brdp, *bend;
  FILE *fp;

  sprintf(ftemp, "tmp/bnntp.%d", ap->sno);
  fp = fopen(ftemp, "w");

  fputs("215 list of newsgroups follows\r\n", fp);

  brdp = bshm->bcache;
  bend = brdp + bshm->number;

  do
  {
    if (brdp->brdname[0] && !brdp->readlevel)	/* guest 可讀 */
      fprintf(fp, "%s %d 1 y\r\n", brdp->brdname, brdp->bpost);
  } while (++brdp < bend);

  fputs(".\r\n", fp);
  fclose(fp);

  agent_write(ap, ftemp);
}


  /* --------------------------------------------------- */
  /* 切換群組						 */
  /* --------------------------------------------------- */


static BRD *
brd_get(brdname)
  char *brdname;
{
  BRD *bhdr, *tail;

  bhdr = bshm->bcache;
  tail = bhdr + bshm->number;

  do
  {
    if (!str_cmp(brdname, bhdr->brdname))
    {
      if (!bhdr->readlevel)	/* guest 可讀 */
	return bhdr;
      return NULL;
    }
  } while (++bhdr < tail);

  return NULL;
}


static void
cmd_group(ap)
  Agent *ap;
{
  char *brdname, msg[80];
  BRD *brd;

  brdname = ap->data + 6;

  if (*brdname && (brd = brd_get(brdname)))
  {
    strcpy(ap->newsgroup, brd->brdname);
    sprintf(msg, "211 %d 1 %d %s selected", brd->bpost, brd->bpost, brd->brdname);
  }
  else
  {
    sprintf(msg, "411 No such group %s", brdname);
  }
  agent_reply(ap, msg);
}


  /* --------------------------------------------------- */
  /* 尋問檔頭						 */
  /* --------------------------------------------------- */


static char *
postowner(owner)
  char *owner;
{
  static char ownmsg[80];

  if (strchr(owner, '@'))
    return owner;
  sprintf(ownmsg, "%s@"MYHOSTNAME, owner);
    return ownmsg;
}


static char *
Gtime(now)
  time_t now;
{
  static char datemsg[40];

  strftime(datemsg, sizeof(datemsg), "%d %b %Y %X GMT", gmtime(&now));
  return datemsg;
}


static void
cmd_xhdr(ap)
  Agent *ap;
{
  char *str, *newsgroup, *ptr1, *ptr2;
  char folder[64], ftemp[64];
  int low, high, max, fd, patern;
  HDR hdr;
  FILE *fp;

  newsgroup = ap->newsgroup;

  if (*newsgroup == '\0')
  {
    agent_reply(ap, "412 no newsgroup has been selected");
    return;
  }

  str = ap->data + 5;

  /* 只支援 subject from date message-id 這四種檔頭 */
  if (!str_ncmp(str, "subject", 7))
    patern = 1;
  else if (!str_ncmp(str, "from", 4))
    patern = 2;
  else if (!str_ncmp(str, "date", 4))
    patern = 3;
  else if (!strncmp(str, "message-id", 10))
    patern = 4;
  else
    patern = 0;

  if ((ptr1 = strchr(str, ' ')) && *(++ptr1) && (ptr2 = strchr(ptr1, '-')) && *(++ptr2))
  {
    low = atoi(ptr1) - 1;
    if (low < 0)
      low = 0;
    high = atoi(ptr2) - 1;
  }
  else
  {
    low = 0;
    high = 0;
  }

  brd_fpath(folder, newsgroup, FN_DIR);
  max = rec_num(folder, sizeof(HDR)) - 1;
  if (high > max)
    high = max;

  sprintf(ftemp, "tmp/bnntp.%d", ap->sno);
  fp = fopen(ftemp, "w");

  fputs("221 header fields follow\r\n", fp);
  if (!patern)
  {
    while (low <= high)
      fprintf(fp, "%d (none)\r\n", ++low);
  }
  else
  {
    if ((fd = open(folder, O_RDONLY)) >= 0)
    {
      lseek(fd, (off_t) (sizeof(HDR) * low), SEEK_SET);
      while (low <= high && read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
      {
        low++;

	if (patern == 1)
	{
	  fprintf(fp, "%d ", low);
	  output_rfc2047_qp(fp, "", hdr.title, MYCHARSET, "\r\n");
	}
	else if (patern == 2)
	{
	  fprintf(fp, "%d %s (%s)\r\n", low, postowner(hdr.owner), hdr.nick);
	}
	else if (patern == 3)
	{
	  fprintf(fp, "%d %s\r\n", low, Gtime(hdr.chrono));
	}
	else /* if (patern == 4) */
	{
	  fprintf(fp, "%d %s$%s@"MYHOSTNAME"\r\n", low, hdr.xname, newsgroup);
	}
      }
      close(fd);
    }
  }
  fputs(".\r\n", fp);

  fclose(fp);

  agent_write(ap, ftemp);
}


static void
cmd_xover(ap)
  Agent *ap;
{
  char *str, *newsgroup, *ptr;
  char folder[64], ftemp[64];
  int low, high, max, fd;
  HDR hdr;
  FILE *fp;

  newsgroup = ap->newsgroup;

  if (*newsgroup == '\0')
  {
    agent_reply(ap, "412 no newsgroup has been selected");
    return;
  }

  str = ap->data + 6;

  if (*str && (ptr = strchr(str, '-')) && *(++ptr))
  {
    low = atoi(str) - 1;
    if (low < 0)
      low = 0;
    high = atoi(ptr) - 1;
  }
  else
  {
    low = 0;
    high = 0;
  }

  brd_fpath(folder, newsgroup, FN_DIR);
  max = rec_num(folder, sizeof(HDR)) - 1;
  if (high > max)
    high = max;

  sprintf(ftemp, "tmp/bnntp.%d", ap->sno);
  fp = fopen(ftemp, "w");

  fputs("224 data follows\r\n", fp);
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    lseek(fd, (off_t) (sizeof(HDR) * low), SEEK_SET);
    while (low <= high && read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
    {
      /* subject from date message-id references lines xref */
      low++;
      fprintf(fp, "%d\t", low);
      output_rfc2047_qp(fp, "", hdr.title, MYCHARSET, "\t");
      fprintf(fp, "%s (%s)\t", postowner(hdr.owner), hdr.nick);
      fprintf(fp, "%s\t", Gtime(hdr.chrono));
      fprintf(fp, "<%s$%s@"MYHOSTNAME">\t", hdr.xname, newsgroup);
      fprintf(fp, "%d\t", low);
      fprintf(fp, "6\t");	/* lines 隨便給 */
      fprintf(fp, "%s.%s\r\n", newsgroup, hdr.xname);
    }
    close(fd);
  }
  fputs(".\r\n", fp);

  fclose(fp);

  agent_write(ap, ftemp);
}


  /* --------------------------------------------------- */
  /* 抓取文章						 */
  /* --------------------------------------------------- */


static void
fetch_article(ap, str, mode)
  Agent *ap;
  char *str;
  int mode;
{
  char *newsgroup;
  char folder[64], fpath[64], ftemp[64], buf[ANSILINELEN];
  int pos, max;
  HDR hdr;
  FILE *fp, *fpr;

  newsgroup = ap->newsgroup;

  if (*newsgroup == '\0')
  {
    agent_reply(ap, "412 no newsgroup has been selected");
    return;
  }

  brd_fpath(folder, newsgroup, FN_DIR);
  if (*str)
  {
    pos = atoi(str) - 1;
    max = rec_num(folder, sizeof(HDR)) - 1;
    if (pos > max)
      pos = max;
    if (pos < 0)
      pos = 0;
  }
  else
  {
    pos = 0;
  }

  sprintf(ftemp, "tmp/bnntp.%d", ap->sno);
  fp = fopen(ftemp, "w");

  fprintf(fp, "22%d %d article retrieved - %s follows\r\n", mode, pos + 1, mode ? "head" : "article");
  if (!rec_get(folder, &hdr, sizeof(HDR), pos))
  {
    /* 文章檔頭 */
    fprintf(fp, "From: %s (%s)\r\n", postowner(hdr.owner), hdr.nick);
    fprintf(fp, "Newsgroups: %s\r\n", newsgroup);
    output_rfc2047_qp(fp, "Subject: ", hdr.title, MYCHARSET, "\r\n");
    fprintf(fp, "Date: %s\r\n", Gtime(hdr.chrono));
    fprintf(fp, "Message-ID: <%s$%s@"MYHOSTNAME">\r\n", hdr.xname, newsgroup);
    fprintf(fp, "Mime-Version: 1.0\r\n");
    fprintf(fp, "Content-Type: text/plain; charset=\""MYCHARSET"\"\r\n");
    fprintf(fp, "Content-Transfer-Encoding: 8bit\r\n");

    /* 文章內容 */
    if (!mode)
    {
      fputs("\r\n", fp);	/* 檔頭和內文空一行 */

#ifdef HAVE_REFUSEMARK
      if (!(hdr.xmode & POST_RESTRICT))
      {
#endif
	hdr_fpath(fpath, folder, &hdr);
	if (fpr = fopen(fpath, "r"))
	{
	  while (fgets(buf, ANSILINELEN, fpr))
	  {
	    str_ansi(buf, buf, sizeof(buf));	/* 去除 '\n' 及控制碼 */
	    fprintf(fp, "%s\r\n", buf);
	  }
	  fclose(fpr);
	}
#ifdef HAVE_REFUSEMARK
      }
#endif
    }
  }
  fputs(".\r\n", fp);

  fclose(fp);

  agent_write(ap, ftemp);
}


static void
cmd_head(ap)
  Agent *ap;
{
  fetch_article(ap, ap->data + 5, 1);
}


static void
cmd_article(ap)
  Agent *ap;
{
  fetch_article(ap, ap->data + 8, 0);
}


  /* --------------------------------------------------- */
  /* 指令集						 */
  /* --------------------------------------------------- */


static Command cmd_table[] =
{
  cmd_help, "help", 4,
  cmd_quit, "quit", 4,

  cmd_mode, "mode", 4,
  cmd_list, "list", 4,

  cmd_group, "group ", 6,

  cmd_xhdr, "xhdr ", 5,
  cmd_xover, "xover ", 6,

  cmd_head, "head ", 5,
  cmd_article, "article ", 8,

  cmd_what, NULL, 0
};


/* ----------------------------------------------------- */
/* send output to client				 */
/* ----------------------------------------------------- */
/* return value :					 */
/* > 0 : bytes sent					 */
/* = 0 : close this agent				 */
/* < 0 : there are some error, but keep trying		 */
/* ----------------------------------------------------- */


static int
agent_send(ap)
  Agent *ap;
{
  int csock, len, cc;
  char *data;

  csock = ap->sock;
  data = ap->data;
  len = ap->used;
  cc = send(csock, data, len, 0);

  if (cc < 0)
  {
    cc = errno;
    if (cc != EWOULDBLOCK)
    {
      agent_log(ap, "SEND", strerror(cc));
      return 0;
    }

    /* would block, so leave it to do later */
    return -1;
  }

  if (cc == 0)
    return -1;

  len -= cc;
  ap->used = len;
  if (len)
  {
    memcpy(data, data + cc, len);
    return cc;
  }

  if (ap->state == CS_FLUSH)
  {
    shutdown(csock, 2);
    close(csock);
    ap->sock = -1;
    return 0;
  }

  ap->state = CS_RECV;
  return cc;
}


/* ----------------------------------------------------- */
/* receive request from client				 */
/* ----------------------------------------------------- */


static int
agent_recv(ap)
  Agent *ap;
{
  int cc, mode, used;
  char *data, *head;

  mode = ap->mode;
  used = ap->used;
  data = ap->data;

  head = data + used;
  cc = recv(ap->sock, head, TCP_RCVSIZ, 0);

  if (cc <= 0)
  {
    cc = errno;
    if (cc != EWOULDBLOCK)
    {
      agent_log(ap, "RECV", strerror(cc));
      return 0;
    }

    /* would block, so leave it to do later */

    return -1;
  }

  head[cc] = '\0';
  used += cc;

  if (used >= MAX_CMD_LEN)
  {
    fprintf(flog, "CMD\t[%d] too long (%d) %.32s\n",
      ap->sno, used, data);

    ap->mode = (mode |= AM_DROP);
    used = 32;
  }

  while (cc = *head)
  {
    if (cc == '\r' || cc == '\n')
    {
      Command *cmd;

      *head = '\0';

      if (mode & AM_DROP)
      {
	ap->mode = mode ^ AM_DROP;
	agent_reply(ap, "552 command too long");
	return -1;
      }

      for (cmd = cmd_table; head = cmd->cmd; cmd++)
      {
	if (!str_ncmp(data, head, cmd->len))
	  break;
      }

      ap->used = 0;

      (*cmd->func) (ap);

      return 1;
    }

    if (cc == '\t')
      *head = ' ';

    head++;
  }

  ap->used = used;
  return 1;
}


/* ----------------------------------------------------- */
/* close a connection & release its resource		 */
/* ----------------------------------------------------- */


static void
agent_fire(ap)
  Agent *ap;
{
  int num;
  char *data, *key;

  num = ap->sock;
  if (num > 0)
  {
    fcntl(num, F_SETFL, M_NONBLOCK);

#define MSG_ABORT   "\r\n450 buggy, closing ...\r\n"
    send(num, MSG_ABORT, sizeof(MSG_ABORT) - 1, 0);
#undef  MSG_ABORT
    shutdown(num, 2);
    close(num);

    key = "END";
  }
  else
  {
    key = "BYE";
  }

  /* log */

  data = ap->data;

  sprintf(data, "[%d] T%d", ap->sno, time(0) - ap->tbegin);
  logit(key, data);

  free(data);
}


/* ----------------------------------------------------- */
/* accept a new connection				 */
/* ----------------------------------------------------- */


static int
agent_accept()
{
  int csock;
  int value;
  struct sockaddr_in csin;

  for (;;)
  {
    value = sizeof(csin);
    csock = accept(0, (struct sockaddr *) & csin, &value);
    /* if (csock > 0) */
    if (csock >= 0)		/* Thor.000126: more proper */
      break;

    csock = errno;
    if (csock != EINTR)
    {
      logit("ACCEPT", strerror(csock));
      return -1;
    }

    while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
  }

  value = 1;

  /* Thor.000511: 註解: don't delay send to coalesce(聯合) packets */
  setsockopt(csock, IPPROTO_TCP, TCP_NODELAY, (char *) &value, sizeof(value));

  /* --------------------------------------------------- */
  /* check remote host / user name			 */
  /* --------------------------------------------------- */

  logit("CONN", "");
  return csock;
}


/* ----------------------------------------------------- */
/* signal routines					 */
/* ----------------------------------------------------- */


#ifdef  SERVER_USAGE
static void
servo_usage()
{
  struct rusage ru;

  if (getrusage(RUSAGE_SELF, &ru))
    return;

  fprintf(flog, "\n[Server Usage]\n\n"
    " user time: %.6f\n"
    " system time: %.6f\n"
    " maximum resident set size: %lu P\n"
    " integral resident set size: %lu\n"
    " page faults not requiring physical I/O: %ld\n"
    " page faults requiring physical I/O: %ld\n"
    " swaps: %ld\n"
    " block input operations: %ld\n"
    " block output operations: %ld\n"
    " messages sent: %ld\n"
    " messages received: %ld\n"
    " signals received: %ld\n"
    " voluntary context switches: %ld\n"
    " involuntary context switches: %ld\n",

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
    ru.ru_nivcsw);

  fflush(flog);
}
#endif


#define SS_CONFIG   1
#define SS_SHUTDOWN 2


static int servo_state;


static void
sig_hup()
{
  servo_state |= SS_CONFIG;
}


static void
sig_term()			/* graceful termination */
{
  servo_state |= SS_SHUTDOWN;
}


static void
sig_abort(sig)
  int sig;
{
  char buf[80];

  sprintf(buf, "abort: %d, errno: %d", sig, errno);
  logit("EXIT", buf);
  fclose(flog);
  exit(0);
}


static void
reaper()
{
  while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
}


static void
servo_signal()
{
  struct sigaction act;

  /* sigblock(sigmask(SIGPIPE)); *//* Thor.981206: 統一 POSIX 標準用法 */

  /* act.sa_mask = 0; *//* Thor.981105: 標準用法 */
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  act.sa_handler = sig_term;		/* forced termination */
  sigaction(SIGTERM, &act, NULL);

  act.sa_handler = sig_abort;		/* forced termination */
  sigaction(SIGSEGV, &act, NULL);	/* if rlimit violate */
  sigaction(SIGBUS, &act, NULL);

#if 1	/* Thor.990203: 抓 signal */
  sigaction(SIGURG, &act, NULL);
  sigaction(SIGXCPU, &act, NULL);
  sigaction(SIGXFSZ, &act, NULL);

#ifdef SOLARIS
  sigaction(SIGLOST, &act, NULL);
  sigaction(SIGPOLL, &act, NULL);
  sigaction(SIGPWR, &act, NULL);
#endif

#ifdef LINUX
  sigaction(SIGSYS, &act, NULL);
  /* sigaction(SIGEMT, &act, NULL); */
  /* itoc.010317: 我的 linux 沒有這個說 :p */
#endif

  sigaction(SIGFPE, &act, NULL);
  sigaction(SIGWINCH, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);
  sigaction(SIGILL, &act, NULL);
  sigaction(SIGTRAP, &act, NULL);
  sigaction(SIGABRT, &act, NULL);
  sigaction(SIGTSTP, &act, NULL);
  sigaction(SIGTTIN, &act, NULL);
  sigaction(SIGTTOU, &act, NULL);
  sigaction(SIGVTALRM, &act, NULL);
#endif

  act.sa_handler = sig_hup;		/* restart config */
  sigaction(SIGHUP, &act, NULL);

  act.sa_handler = reaper;
  sigaction(SIGCHLD, &act, NULL);

#ifdef  SERVER_USAGE
  act.sa_handler = servo_usage;
  sigaction(SIGPROF, &act, NULL);
#endif

  /* Thor.981206: lkchu patch: 統一 POSIX 標準用法 */
  /* 在此借用 sigset_t act.sa_mask */
  sigaddset(&act.sa_mask, SIGPIPE);
  sigprocmask(SIG_BLOCK, &act.sa_mask, NULL);
}


/* ----------------------------------------------------- */
/* server core routines					 */
/* ----------------------------------------------------- */


static void
servo_daemon(inetd)
  int inetd;
{
  int fd, value;
  char buf[80];
  struct linger ld;
  struct sockaddr_in sin;
#ifdef HAVE_RLIMIT
  struct rlimit limit;
#endif

  /* More idiot speed-hacking --- the first time conversion makes the C     *
   * library open the files containing the locale definition and time zone. *
   * If this hasn't happened in the parent process, it happens in the       *
   * children, once per connection --- and it does add up.                  */

  time((time_t *) & value);
  gmtime((time_t *) & value);
  strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", localtime((time_t *) & value));

#ifdef HAVE_RLIMIT
  /* --------------------------------------------------- */
  /* adjust the resource limit				 */
  /* --------------------------------------------------- */

  getrlimit(RLIMIT_NOFILE, &limit);
  limit.rlim_cur = limit.rlim_max;
  setrlimit(RLIMIT_NOFILE, &limit);

  limit.rlim_cur = limit.rlim_max = 16 * 1024 * 1024;
  setrlimit(RLIMIT_FSIZE, &limit);

  limit.rlim_cur = limit.rlim_max = 16 * 1024 * 1024;
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

  close(1);
  close(2);

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
  sin.sin_port = htons(BNNTP_PORT);
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  memset((char *) &sin.sin_zero, 0, sizeof(sin.sin_zero));

  if (bind(fd, (struct sockaddr *) & sin, sizeof(sin)) ||
    listen(fd, TCP_BACKLOG))
    exit(1);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int n, sock, state;
  time_t uptime, tcheck, tfresh;
  Agent **FBI, *Scully, *Mulder, *agent;
  fd_set rset, wset, xset;
  static struct timeval tv = {BNNTP_PERIOD, 0};

  state = 0;

  while ((n = getopt(argc, argv, "i")) != -1)
  {
    switch (n)
    {
    case 'i':
      state = 1;
      break;

    default:
      fprintf(stderr, "Usage: %s [options]\n"
	"\t-i  start from inetd with wait option\n",
	argv[0]);
      exit(0);
    }
  }

  servo_daemon(state);

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(BBSHOME);

  init_bshm();

  servo_signal();

  log_open();
  dns_init();

  uptime = time(0);
  tcheck = uptime + BNNTP_PERIOD;
  tfresh = uptime + BNNTP_FRESH;

  Scully = Mulder = NULL;

  for (;;)
  {
    /* maintain : resource and garbage collection */

    uptime = time(0);
    if (tcheck < uptime)
    {
      /* ----------------------------------------------- */
      /* agent_audit (uptime - BNNTP_TIMEOUT)		 */
      /* ----------------------------------------------- */

      tcheck = uptime - BNNTP_TIMEOUT;

      for (FBI = &Scully; agent = *FBI;)
      {
	if (agent->uptime < tcheck)
	{
	  agent_fire(agent);

	  *FBI = agent->anext;

	  agent->anext = Mulder;
	  Mulder = agent;
	}
	else
	{
	  FBI = &(agent->anext);
	}
      }

      /* ----------------------------------------------- */
      /* maintain SPAM & server log			 */
      /* ----------------------------------------------- */

      if (tfresh < uptime)
      {
	tfresh = uptime + BNNTP_FRESH;
#ifdef SERVER_USAGE
	servo_usage();
#endif
	log_fresh();
      }
      else
      {
	fflush(flog);
      }

      tcheck = uptime + BNNTP_PERIOD;
    }

    /* ------------------------------------------------- */
    /* check servo operation state			 */
    /* ------------------------------------------------- */

    n = 0;

    if (state = servo_state)
    {
      if (state & SS_CONFIG)
      {
	state ^= SS_CONFIG;
      }

      if (state & SS_SHUTDOWN)	/* graceful shutdown */
      {
	n = -1;
	close(0);
      }

      servo_state = state;
    }

    /* ------------------------------------------------- */
    /* Set up the fdsets				 */
    /* ------------------------------------------------- */

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&xset);

    if (n == 0)
      FD_SET(0, &rset);

    for (agent = Scully; agent; agent = agent->anext)
    {
      sock = agent->sock;
      state = agent->state;

      if (n < sock)
	n = sock;

      if (state == CS_RECV)
      {
	FD_SET(sock, &rset);
      }
      else
      {
	FD_SET(sock, &wset);
      }

      FD_SET(sock, &xset);
    }

    /* no active agent and ready to die */

    if (n < 0)
    {
      break;
    }

    {
      struct timeval tv_tmp = tv;
      /* Thor.981221: for future reservation bug */
      n = select(n + 1, &rset, &wset, &xset, &tv_tmp);
    }

    if (n == 0)
    {
      continue;
    }

    if (n < 0)
    {
      n = errno;
      if (n != EINTR)
      {
	logit("SELECT", strerror(n));
      }
      continue;
    }

    /* ------------------------------------------------- */
    /* serve active agents				 */
    /* ------------------------------------------------- */

    uptime = time(0);

    for (FBI = &Scully; agent = *FBI;)
    {
      sock = agent->sock;

      if (FD_ISSET(sock, &wset))
      {
	state = agent_send(agent);
      }
      else if (FD_ISSET(sock, &rset))
      {
	state = agent_recv(agent);
      }
      else if (FD_ISSET(sock, &xset))
      {
	state = 0;
      }
      else
      {
	state = -1;
      }

      if (state == 0)		/* fire this agent */
      {
	agent_fire(agent);

	*FBI = agent->anext;

	agent->anext = Mulder;
	Mulder = agent;

	continue;
      }

      if (state > 0)
      {
	agent->uptime = uptime;
      }

      FBI = &(agent->anext);
    }

    /* ------------------------------------------------- */
    /* serve new connection				 */
    /* ------------------------------------------------- */

    /* Thor.000209: 考慮移前此部分, 免得卡在 accept() */
    if (FD_ISSET(0, &rset))
    {
      /* Thor.990319: check maximum connection number */
      unsigned int ip_addr;
      sock = agent_accept();

      if (sock > 0)
      {
	if (agent = Mulder)
	{
	  Mulder = agent->anext;
	}
	else
	{
	  agent = (Agent *) malloc(sizeof(Agent));
	  if (!agent)		/* Thor.990205: 記錄空間不夠 */
	    logit("ERROR", "Not enough space in main()");
	}

	*FBI = agent;

	/* variable initialization */

	memset(agent, 0, sizeof(Agent));

	agent->sock = sock;
	agent->sno = ++servo_sno;
	agent->state = CS_SEND;
	agent->tbegin = agent->uptime = uptime;

	/* Thor.990319: check maximum connection number */
	agent->ip_addr = ip_addr;

	agent->data = (char *) malloc(MIN_DATA_SIZE);
	if (!agent->data)	/* Thor.990205: 記錄空間不夠 */
	  logit("ERROR", "Not enough space in agent->data");
	sprintf(agent->data, "200 MapleBBS NNTP server ready (read only)\r\n");
	agent->used = strlen(agent->data);
	agent->size = MIN_DATA_SIZE;
      }
    }

    /* ------------------------------------------------- */
    /* tail of main loop				 */
    /* ------------------------------------------------- */
  }

  logit("EXIT", "shutdown");
  fclose(flog);

  exit(0);
}
