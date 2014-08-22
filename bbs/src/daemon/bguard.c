/*-------------------------------------------------------*/
/* util/bguard.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : BBS finger daemon 列出站內使用者資料	 */
/* create : 96/11/20				 	 */
/* update : 96/12/15				 	 */
/*-------------------------------------------------------*/
/* syntax : bguard					 */
/*-------------------------------------------------------*/
/* notice : ushm (utmp shared memory) synchronize	 */
/*-------------------------------------------------------*/


#define	_MODES_C_
#define	VERBOSE
#define WATCH_DOG


#include "bbs.h"


#include <sys/ipc.h>

#ifdef	HAVE_SEM
#include <sys/sem.h>
#endif

#include <sys/shm.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <netinet/tcp.h>


static int gline;


#ifdef  WATCH_DOG
# define MYDOG  gline = __LINE__
#else
# define MYDOG			/* NOOP */
#endif


#define	GUARD_LOGFILE	"run/bguard.log"
#define	GUARD_PIDFILE	"run/bguard.pid"


#define	LOAD_INTERVAL	90	/* check system load */
#define	GUARD_INTERVAL	60	/* check user status */
#define	FINGER_INTERVAL	(30 * 60)


#define FINGER_TIMEOUT	(60 * 3)


#define	TCP_QLEN	3
#define	TCP_BUFSIZ	(512 * 7)
#define	TCP_LINSIZ	256
#define	TCP_RCVSIZ	128


/* ----------------------------------------------------- */
/* client connection structure				 */
/* ----------------------------------------------------- */


typedef struct Agent Agent;


struct Agent
{
  Agent *next;
  int sock;
  int state;
  int locus;
  time_t uptime;		/* 建立 connection 的時間 */

  int count;			/* 線上共有多少人？ */
  UTMP *uentp;

  char pool[TCP_BUFSIZ];	/* buffered I/O pool */

  /* Thor:980726: ending zero for string */
  char ZERO;
};


/* ----------------------------------------------------- */
/* connection state					 */
/* ----------------------------------------------------- */


#define	CS_FREE		0x00
#define	CS_READING	0x01
#define	CS_WRITING	0x02


/* ----------------------------------------------------- */
/* operation log and debug information			 */
/* ----------------------------------------------------- */


static FILE *flog;		/* log file descriptor */


extern int errno;


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
log_open()
{
  FILE *fp;

  umask(077);

  if (fp = fopen(GUARD_PIDFILE, "w"))
  {
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
  }

  flog = fopen(GUARD_LOGFILE, "a");
  logit("START", "guard (finger) daemon");
}


/*-------------------------------------------------------*/
/* .UTMP cache						 */
/*-------------------------------------------------------*/


static UCACHE *ushm;
static UTMP *ushm_head, *ushm_tail;


static void
attach_err(shmkey, name)
  int shmkey;
  char *name;
{
  char buf[80];

  sprintf(buf, "error, key = %x", shmkey);
  logit(name, buf);
  exit(1);
}


#ifdef	HAVE_SEM
/* ----------------------------------------------------- */
/* semaphore : for critical section			 */
/* ----------------------------------------------------- */


static int mysemid;


static void
resolve_sem()
{
  int semid;

  union semun
  {
    int val;
    struct semid_ds *buf;
    ushort *array;
  }     arg =
  {
    1
  };

  semid = semget(BSEM_KEY, 1, 0);
  if (semid == -1)
  {
    semid = semget(BSEM_KEY, 1, IPC_CREAT | BSEM_FLG);
    if (semid == -1)
      attach_err(BSEM_KEY, "semget");
    semctl(semid, 0, SETVAL, arg);
  }
  mysemid = semid;
}


static void
sem_lock(op)
  int op;			/* op is BSEM_ENTER or BSEM_LEAVE */
{
  struct sembuf sops;

  sops.sem_num = 0;
  sops.sem_flg = SEM_UNDO;
  sops.sem_op = op;
  semop(mysemid, &sops, 1);
}
#endif	/* HAVE_SEM */


static void
init_ushm()
{
  UCACHE *xshm;
  int shmsize;
  int shmid;

  shmsize = sizeof(UCACHE);
  shmid = shmget(UTMPSHM_KEY, shmsize, 0);
  if (shmid < 0)
  {
    shmid = shmget(UTMPSHM_KEY, shmsize, IPC_CREAT | 0600);
    if (shmid < 0)
      attach_err(UTMPSHM_KEY, "shmget");
  }
  else
  {
    shmsize = 0;
  }

  xshm = (UCACHE *) shmat(shmid, NULL, 0);
  if (xshm == (UCACHE *) - 1)
    attach_err(UTMPSHM_KEY, "shmat");

  if (shmsize)
  {
    memset(xshm, 0, shmsize);
    if (xshm->mbase < xshm->mpool)
      xshm->mbase = xshm->mpool;
  }

  ushm = xshm;
  ushm_head = xshm->uslot;
  ushm_tail = ushm_head + MAXACTIVE;
}


static void
ushm_guard()
{
  static int flip;
  int flop;
  usint count;
  UTMP *uentp, *uceil, *utail;
  int idle;
  pid_t pid;
  UCACHE *xshm;
  char buf[128];
#ifdef DETAIL_IDLETIME
  time_t now;
#endif

  flop = ++flip;
  count = 0;
  uentp = ushm_head;
  utail = ushm_tail;
  uceil = uentp;

#ifdef	HAVE_SEM
  sem_lock(BSEM_ENTER);
#endif

#ifdef DETAIL_IDLETIME
  time(&now);
#endif

  do
  {
    flop++;
    if (pid = uentp->pid)
    {
#ifdef TIME_KICKER
#ifdef DETAIL_IDLETIME
      idle = (now - uentp->idle_time) / 60;
#else
      idle = uentp->idle_time;
#endif
#endif
      if (flop & 15)
      {
#ifdef TIME_KICKER
	if (idle >= IDLE_TIMEOUT)
	{
	  errno = 0;
	  if ((kill(pid, SIGTERM) < 0) && (errno == ESRCH))
	  {
	    uentp->pid = uentp->userno = 0;
	    ushm->count--;
	  }
	}
	else
#endif
	{
	  if (uceil < uentp)
	    uceil = uentp;
	  count++;
	}
      }
      else
      {
	int sig;

	errno = sig = 0;

#ifdef TIME_KICKER
	if (idle >= IDLE_TIMEOUT)
	  sig = SIGTERM;
#endif

	if ((kill(pid, sig) < 0) && (errno == ESRCH))
	{
	  uentp->pid = uentp->userno = 0;
	  ushm->count--;
	}
	else
	{
	  if (!sig)
	  {
	    if (uceil < uentp)
	      uceil = uentp;
	    count++;
	  }
	}
      }
    }
  } while (++uentp < utail);

  xshm = ushm;
  xshm->count = count;
  xshm->offset = (void *) uceil - (void *) ushm_head;

#ifdef	HAVE_SEM
  sem_lock(BSEM_LEAVE);
#endif

#ifdef	VERBOSE
  sprintf(buf, "%d %p (%p - %p)", count, uceil, ushm_head, utail);
  logit("count", buf);
#endif
}


/*-------------------------------------------------------*/
/* check system / memory / CPU loading			 */
/*-------------------------------------------------------*/


static int fkmem;


#ifdef SOLARIS
static void
chkload_init()
{
#include <nlist.h>
#define VMUNIX  "/dev/ksyms"
#define KMEM    "/dev/kmem" 
   /* Thor.981207: 記得check permission 要bbs readable */

  static struct nlist nlst[] = 
  {
    {"avenrun"},
    {0}
  };

  int kmem;
  long offset;

  nlist(VMUNIX, nlst);
  if (nlst[0].n_type == 0)
    exit(1);
  offset = (long) nlst[0].n_value;

  if ((kmem = open(KMEM, O_RDONLY)) == -1)
    exit(1);

  if (lseek(kmem, offset, L_SET) == -1)
    exit(1);

  fkmem = kmem;
}
#endif


static void
chkload()
{

  struct
  {
    int avgload;
    double sysload[3];
  }      myload;
#define	cpu_load	myload.sysload


#if defined(LINUX)
  FILE *fp;

  MYDOG;
  fp = fopen("/proc/loadavg", "r");
  if (!fp)
    cpu_load[0] = cpu_load[1] = cpu_load[2] = 0;
  else
  {
    float av[3];

    fscanf(fp, "%g %g %g", av, av + 1, av + 2);
    fclose(fp);
    cpu_load[0] = av[0];
    cpu_load[1] = av[1];
    cpu_load[2] = av[2];
  }

#elif defined(BSD44)
  getloadavg(cpu_load, 3);

#else

  long avenrun[3];
  int i;

  i = fkmem;

  if (read(i, (char *) avenrun, sizeof(avenrun)) == -1)
    exit(1);

  lseek(i, -(off_t) sizeof(avenrun), SEEK_CUR);

#define loaddouble(la) ((double)(la) / (1 << 8))

  for (i = 0; i < 3; i++)
    cpu_load[i] = loaddouble(avenrun[i]);

  /* Thor.980728: lkchu patch: linux部分及 bsd部 分已有定 cpu_load */

#endif

  myload.avgload = cpu_load[0] + cpu_load[1] * 4;
  /* memcpy(&ushm->avgload, &myload, sizeof(myload)); */

  ushm->avgload = myload.avgload;
  ushm->sysload[0] = myload.sysload[0];
  ushm->sysload[1] = myload.sysload[1];
  ushm->sysload[2] = myload.sysload[2];
}


/* ----------------------------------------------------- */
/* server side stuff					 */
/* ----------------------------------------------------- */


static char *
mail_string(fpath)
  char *fpath;
{
  char *answer;
  int fd, size;
  struct stat st;

  answer = "都看過了";
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    if (!fstat(fd, &st) && (size = st.st_size) > 0)
    {
      char *fimage;
      HDR *fhdr;

      fimage = (char *) malloc(size);
      read(fd, fimage, size);
      fhdr = (HDR *) (fimage + size);
      while (--fhdr >= (HDR *) fimage)
      {
	if (!(fhdr->xmode & MAIL_READ))
	{
	  answer = "有新信件";
	  break;
	}
      }
      free(fimage);
    }
    close(fd);
  }

  return answer;
}


static void
serve_finger(ap)
  Agent *ap;
{
  char *base, *head, fpath[128];
  int fd;

  base = head = ap->pool;

  /* Thor.980726: pool內容不會超過 idlen */
  head[IDLEN] = '\0';
  

  sprintf(fpath, "usr/%c/%s/.ACCT", *head, head);

  MYDOG;
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    char *str, *mailstr, *modestr;
    UTMP *uentp, *utail;
    int len;
    ACCT acct;

    read(fd, &acct, sizeof(ACCT));
    close(fd);

    /* 是否有新信件還沒看？ */

    str = (char *) strchr(fpath, '.');
    strcpy(str, FN_DIR);
    MYDOG;
    mailstr = mail_string(fpath);

    /* 是否在線上？ */

    MYDOG;

    uentp = ushm_head;
    utail = ushm_tail;
    fd = acct.userno;
    modestr = "不在站上";

    do
    {
      if (fd == uentp->userno && !(uentp->ufo & (UFO_CLOAK | UFO_SUPERCLOAK)))
      {
	modestr = ModeTypeTable[uentp->mode];
	break;
      }
    } while (++uentp < utail);

    sprintf(head, "%s(%s) 共上站 %d 次，發表文章 %d 篇。\n"
      "最近(%s)來自(%s)\n%s通過身分認證 [動態] %s [信箱] %s\n",
      acct.userid, acct.username, acct.numlogins, acct.numposts,
      Btime(&(acct.lastlogin)), acct.lasthost,
      acct.userlevel & PERM_VALID ? "已經" : "尚未",
      modestr, mailstr);

    head += strlen(head);

    /* 顯示 [名片/計畫檔] */

    MYDOG;
    strcpy(str, FN_PLANS);
    if ((fd = open(fpath, O_RDONLY)) >= 0)
    {
      strcpy(head, "[計畫]\n");
      head += strlen(head);

      /* 假設 buffer 綽綽有餘，讓事情簡化 */

      len = read(fd, head, TCP_BUFSIZ - (head - base) - 10);
      close(fd);

      if (len > 0)
      {
	head += len;
	strcpy(head, "\033[m\n");
	head += strlen(head);
      }
    }
  }
  else
  {
    strcat(head, " ==> not exist here.\n");
    head += strlen(head);
  }

  ap->locus = head - base;
  ap->count = -1;		/* 代表 end of transmission */
}


static void
serve_userlist(ap)
  Agent *ap;
{
  int count;
  UTMP *uentp, *utail;
  char *base, *head, *tail;

  count = ap->count;
  uentp = ap->uentp;
  utail = ushm_tail;

  base = ap->pool;
  head = base + ap->locus;
  tail = base + TCP_BUFSIZ - TCP_LINSIZ;

  for (;;)
  {
    if (uentp->pid && uentp->userno && 
#ifdef HAVE_SUPERCLOAK
      !(uentp->ufo & UFO_SUPERCLOAK) &&
#endif
      !(uentp->ufo & UFO_CLOAK))	/* lkchu.990118: 隱身不顯示 */
    {
      sprintf(head, "%-13s%-25s%-30.29s%s\n",
	uentp->userid, uentp->username, uentp->from,
	ModeTypeTable[uentp->mode]);
      count++;
      head += strlen(head);
      if (head > tail)
      {
	uentp++;
	break;
      }
    }

    if (++uentp >= utail)
    {
      sprintf(head, "============ ======================== ============================= ==========\n"
	"【" BBSNAME "】 Total users = %d\n", count);
      head += strlen(head);
      count = -1;
      break;
    }
  }

  ap->uentp = uentp;
  ap->count = count;
  ap->locus = head - base;
}


/* ----------------------------------------------------- */
/* client's service dispatcher				 */
/* ----------------------------------------------------- */


static void
agent_serve(ap)
  Agent *ap;
{
  char *cmd, *str;
  int ch;

  cmd = str = ap->pool;

  while (ch = *str)
  {
    str++;
    if (ch != ' ' && ch != '\t')
    {
      if (ch >= 'A' && ch <= 'Z')
	ch |= 0x20;
      else if (ch == '.' || ch == '@')
      {
	*cmd = '\0';
	break;
      }
      *cmd++ = ch;
    }
  }

  str = ap->pool;
  ap->state = CS_WRITING;
  if (str == cmd)
  {
    strcpy(str, "ID		Nick			From			    Mode\n"
      "============ ======================== ============================= ==========\n");
    ap->locus = strlen(str);
    ap->count = 0;
    ap->uentp = ushm_head;
    serve_userlist(ap);
  }
  else
  {
    serve_finger(ap);
  }
}


/* ----------------------------------------------------- */
/* send output to client				 */
/* ----------------------------------------------------- */


static int
agent_write(ap)
  Agent *ap;
{
  int len, bytes;
  char *str;

  len = ap->locus;
  str = ap->pool;
  bytes = send(ap->sock, str, len, 0);
  if (bytes <= 0)
  {
    len = errno;
    if (len != EWOULDBLOCK)
    {
      logit("write", strerror(len));
      return 0;
    }

    return -1;
  }

  len -= bytes;
  ap->locus = len;
  if (len)
  {
    memcpy(str, str + bytes, len);
    return bytes;
  }

  if (ap->count >= 0)
  {
    serve_userlist(ap);
    return bytes;
  }

  return 0;
}


/* ----------------------------------------------------- */
/* receive request from client				 */
/* ----------------------------------------------------- */


static int
agent_read(ap)
  Agent *ap;
{
  int pos, len, cc;
  char *str;

  pos = ap->locus;
  str = &ap->pool[pos];
  len = recv(ap->sock, str, BMIN(TCP_RCVSIZ, sizeof(ap->pool)-pos), 0);  
  /* Thor.980726: 避免 pool overflow */

  if (len <= 0)
  {
    len = errno;
    if (len != EWOULDBLOCK)
    {
      logit("read", strerror(len));
      return 0;
    }

    return -1;			/* would block, so leave it to do later */
  }

  str[len] = '\0';

  while (cc = *str)
  {
    if (cc == '\r' || cc == '\n')
    {
      *str = '\0';
      agent_serve(ap);
      return 1;
    }
    str++;
  }

  ap->locus = pos + len;
  return 1;
}


/* ---------------------------------------------------- */
/* server core routines					 */
/* ---------------------------------------------------- */


static void
servo_daemon(inetd)
  int inetd;
{
  int fd, value;
  char buf[80];
  struct sockaddr_in sin;
  struct linger ld;

  /*
   * More idiot speed-hacking --- the first time conversion makes the C
   * library open the files containing the locale definition and time zone.
   * If this hasn't happened in the parent process, it happens in the
   * children, once per connection --- and it does add up.
   */

  time_t dummy;
  struct tm *dummy_time;

#ifdef HAVE_RLIMIT
  struct rlimit limit;
#endif

  time(&dummy);
  dummy_time = gmtime(&dummy);
  dummy_time = localtime(&dummy);
  strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", dummy_time);

#ifdef HAVE_RLIMIT
  /* --------------------------------------------------- */
  /* adjust the resource limit                           */
  /* --------------------------------------------------- */

  getrlimit(RLIMIT_NOFILE, &limit);
  limit.rlim_cur = limit.rlim_max;
  setrlimit(RLIMIT_NOFILE, &limit);

  limit.rlim_cur = limit.rlim_max = 16 * 1024 * 1024;
  setrlimit(RLIMIT_FSIZE, &limit);

  limit.rlim_cur = limit.rlim_max = 4 * 1024 * 1024;
  setrlimit(RLIMIT_DATA, &limit);

#ifdef SOLARIS
#define RLIMIT_RSS RLIMIT_AS	/* Thor.981206: port for solaris 2.6 */
#endif

  setrlimit(RLIMIT_RSS, &limit);

  limit.rlim_cur = limit.rlim_max = 0;
  setrlimit(RLIMIT_CORE, &limit);
#endif

  /* --------------------------------------------------- */
  /* detatch & daemonize				 */
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
  sin.sin_port = htons(FINGER_PORT);
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(sin.sin_zero, 0, sizeof(sin.sin_zero));

  if (bind(fd, (struct sockaddr *) & sin, sizeof(sin)) ||
    listen(fd, TCP_QLEN))
    exit(1);
}


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
}


static void
sig_trap(sig)
  int sig;
{
  char buf[80];

  close(fkmem);
  sprintf(buf, "signal %d", sig);
  logit("EXIT", buf);
  server_usage();
  fclose(flog);
  exit(1);
}


static void
reaper()
{
  while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0)
    ;
}


static void
main_signals()
{
  struct sigaction act;

  /* sigblock(sigmask(SIGPIPE)); */
  /* Thor.981206: 統一 POSIX 標準用法  */

  /* act.sa_mask = 0; */ /* Thor.981105: 標準用法 */
  sigemptyset(&act.sa_mask);      
  act.sa_flags = 0;

  act.sa_handler = reaper;
  sigaction(SIGCHLD, &act, NULL);

  act.sa_handler = sig_trap;
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGBUS, &act, NULL);
  sigaction(SIGSEGV, &act, NULL);

  act.sa_handler = server_usage;
  sigaction(SIGPROF, &act, NULL);

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
  int csock, nfds, state;
  Agent **FBI, *Scully, *Mulder, *agent;
  fd_set rset, wset, xset;
  struct timeval tv;
  time_t uptime, tcheck, tguard, tagent;

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
  chdir(BBSHOME);
  main_signals();
  log_open();

#ifdef	HAVE_SEM
  resolve_sem();
#endif

  init_ushm();
#ifdef SOLARIS
  chkload_init();
#endif

  /* Thor.981221: for future reservation bug */
  tcheck = tguard = tagent = 0;

  Scully = Mulder = NULL;

  for (;;) /* Thor.981221: 註解: Main loop begin */
  {
    uptime = time(0);

    /* system guard / resource and garbage collection */

    if (uptime > tcheck)
    {
      chkload();
      tcheck = uptime + LOAD_INTERVAL;

      if (uptime > tguard)
      {
	ushm_guard();
	tguard = uptime + GUARD_INTERVAL;
      }

      if (uptime > tagent)
      {
	tagent = uptime - FINGER_TIMEOUT;

	for (FBI = &Scully; agent = *FBI;)
	{
	  if (agent->uptime < tagent)
	  {
	    csock = agent->sock;
	    if (csock > 0)
	    {
	      shutdown(csock, 2);
	      close(csock);
	    }

	    *FBI = agent->next;

	    agent->next = Mulder;
	    Mulder = agent;
	  }
	  else
	  {
	    FBI = &(agent->next);
	  }
	}

	tagent = uptime + FINGER_INTERVAL;
	fflush(flog);
      }
    }

    /* Set up the fdsets. */

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&xset);

    FD_SET(0, &rset);
    nfds = 0;

    for (agent = Scully; agent; agent = agent->next)
    {
      csock = agent->sock;
      state = agent->state;

      if (nfds < csock)
	nfds = csock;

      FD_SET(csock, state == CS_READING ? &rset : &wset);

      FD_SET(csock, &xset);
    }
 
    /* Thor.981221: for future reservation bug */
    tv.tv_sec = LOAD_INTERVAL;
    tv.tv_usec = 0;

    nfds = select(nfds + 1, &rset, &wset, &xset, &tv);

    if (nfds == 0)
      continue;

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

    for (FBI = &Scully; agent = *FBI;)
    {
      csock = agent->sock;

      if (FD_ISSET(csock, &wset))
      {
	state = agent_write(agent);
      }
      else if (FD_ISSET(csock, &rset))
      {
	state = agent_read(agent);
      }
      else if (FD_ISSET(csock, &xset))
      {
	state = 0;
      }
      else
      {
	state = -1;
      }

      if (state == 0)		/* fire this agent */
      {
	shutdown(csock, 2);
	close(csock);
	*FBI = agent->next;

	agent->next = Mulder;
	Mulder = agent;
	continue;
      }

      if (state > 0)
      {
	agent->uptime = uptime;
      }

      FBI = &(agent->next);
    }

    /* ------------------------------------------------- */
    /* serve new connection				 */
    /* ------------------------------------------------- */

    if (FD_ISSET(0, &rset))
    {
      for (;;)
      {
	csock = accept(0, NULL, NULL);

	if (csock > 0)
	{
	  if (agent = Mulder)
	  {
	    Mulder = agent->next;
	  }
	  else
	  {
	    agent = (Agent *) malloc(sizeof(Agent));
	  }

	  *FBI = agent;

	  /* variable initialization */

	  agent->next = NULL;
	  agent->sock = csock;
	  agent->state = CS_READING;
	  agent->locus = 0;
	  agent->uptime = uptime;
	  agent->count = 0;
	  agent->uentp = NULL;

	  break;
	}

	state = errno;
	if (state != EINTR)
	{
	  logit("accept", strerror(state));
	  break;
	}
	while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
      }
    }

    /* ------------------------------------------------- */
    /* tail of main loop				 */
    /* ------------------------------------------------- */
  }
}
