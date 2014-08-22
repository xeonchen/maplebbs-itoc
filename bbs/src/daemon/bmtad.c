/*-------------------------------------------------------*/
/* util/bmtad.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : Mail Transport Agent for BBS		 */
/* create : 96/11/20					 */
/* update : 96/12/15					 */
/*-------------------------------------------------------*/
/* syntax : bmtad					 */
/*-------------------------------------------------------*/
/* notice : brdshm (board shared memory) synchronize	 */
/*-------------------------------------------------------*/


#define	FORGE_CHECK
#undef	SMTP_CONN_CHECK
#define	HELO_CHECK

#define ANTI_HTMLMAIL		/* itoc.021014: 擋 html_mail */
#define	ANTI_NOTMYCHARSETMAIL	/* itoc.030513: 擋 not-mycharset mail */


#include "bbs.h"


/* Thor.990221: 儘量 fflush 出 log */
#undef DEBUG

#define ADM_ALIASES     {"root", "mailer-daemon", NULL}


#include <sys/wait.h>
#include <netinet/tcp.h>
#include <sys/resource.h>


#define SERVER_USAGE
#define WATCH_DOG


#define BMTA_PIDFILE    "run/bmta.pid"
/* #define BMTA_LOGFILE    "run/bmta.log" */	/* 搬去 global.h */
#define BMTA_DEBUGFILE	"run/bmta.debug"


#define BMTA_PERIOD	(60 * 15)	/* 每 15 分鐘 check 一次 */
#define BMTA_TIMEOUT	(60 * 30)	/* 超過 30 分鐘的連線就視為錯誤 */
#define BMTA_FRESH	86400		/* 每 1 天整理一次 */
#define BMTA_FAULT	100


#define TCP_BACKLOG	3
#define TCP_BUFSIZ	4096
#define TCP_LINSIZ	256
#define TCP_RCVSIZ	2048


#define MIN_DATA_SIZE	8000
#define MAX_DATA_SIZE	262143		/* 每一封信的大小限制(byte) */
#define MAX_CMD_LEN	1024
#define MAX_RCPT	7		/* 同一信有超過 7 個收信者就擋掉 */
#define MAX_HOST_CONN	2


#define SPAM_MHOST_LIMIT	1000	/* 同一個 @host 寄進來超過 1000 封信，就將此 @host 視為廣告商 */
#define SPAM_MFROM_LIMIT	128	/* 同一個 from 寄進來超過 128 封信，就將此 from 視為廣告商 */

#define SPAM_TITLE_LIMIT	50	/* 同一個標題寄進來超過 50 次就特別記錄 */
#define SPAM_FORGE_LIMIT	10	/* 同一個 @domain 錯 10 次以上，就認定不是筆誤，而是故意的 */


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
  char *help;
}      Command;


/* ----------------------------------------------------- */
/* client connection structure				 */
/* ----------------------------------------------------- */


typedef struct RCPT
{
  struct RCPT *rnext;
  char userid[0];
}    RCPT;


typedef struct Agent
{
  struct Agent *anext;
  int sock;
  int sno;
  int state;
  int mode;
  int letter;			/* 1:寄給 *.bbs@  0:寄給 *.brd@ */
  u_long ip_addr;

  time_t uptime;
  time_t tbegin;

  int xsize;
  int xrcpt;
  int xdata;
  int xerro;
  int xspam;

  char ident[80];
  char memo[80];
  char fpath[80];

  char from[80];
  char title[80];

  char addr[80];
  char nick[256];		/* Thor.000131: 有的人發瘋給太長 */

  int nrcpt;			/* number of rcpt */
  RCPT *rcpt;

  char *data;
  int used;
  int size;
}     Agent;


static int servo_sno;


/* ----------------------------------------------------- */
/* connection state					 */
/* ----------------------------------------------------- */


#define CS_FREE     0x00
#define CS_RECV     0x01
#define CS_REPLY    0x02
#define CS_SEND     0x03
#define CS_FLUSH    0x04	/* flush data and quit */


/* ----------------------------------------------------- */
/* AM : Agent Mode					 */
/* ----------------------------------------------------- */


#define AM_VALID    0x001	/* 收件人是 bbsreg@MYHOSTNAME */
#define AM_BBSADM   0x002	/* 收件人是 ADM_ALIASES@MYHOSTNAME */

#define AM_DROP     0x010	/* swallow command */
#define AM_SWALLOW  0x020	/* swallow data */
#define AM_DATA     0x040	/* data mode */

#define AM_SPAM     0x100

#define AM_HELO     0x200	/* HELOed */

#ifdef DEBUG
#define AM_DEBUG    0x400	/* for tracing malicious connection */
#endif

/* ----------------------------------------------------- */
/* operation log and debug information			 */
/* ----------------------------------------------------- */
/* @START | ... | time					 */
/* @CONN | [sno] ident | time				 */
/* ----------------------------------------------------- */


static FILE *flog;
static int gline;
static char gtext[100];


#ifdef  WATCH_DOG
#define MYDOG   gline = __LINE__
#else
#define MYDOG			/* NOOP */
#endif


extern int errno;
extern char *crypt();


static void
log_fresh()
{
  int count;
  char fsrc[64], fdst[64];
  char *fpath = BMTA_LOGFILE;

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

#ifdef DEBUG
  fflush(flog);
#endif
}


static void
log_open()
{
  FILE *fp;

  umask(077);

  if (fp = fopen(BMTA_PIDFILE, "w"))
  {
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
  }

  flog = fopen(BMTA_LOGFILE, "a");
  logit("START", "MTA daemon");
}


static inline void
agent_log(ap, key, msg)
  Agent *ap;
  char *key;
  char *msg;
{
  fprintf(flog, "%s\t[%d] %s\n", key, ap->sno, msg);

#ifdef DEBUG
  fflush(flog);
#endif
}


static void
agent_reply(ap, msg)
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


/* ----------------------------------------------------- */
/* server side routines					 */
/* ----------------------------------------------------- */


#ifdef EMAIL_JUSTIFY
static int
is_badid(userid)
  char *userid;
{
  int ch;
  char *str;

  if (strlen(userid) < 2)
    return 1;

  if (!is_alpha(*userid))
    return 1;

  str = userid;
  while (ch = *(++str))
  {
    if (!is_alnum(ch))
      return 1;
  }
  return 0;
}


static int
acct_fetch(userid, acct)
  char *userid;
  ACCT *acct;
{
  int fd;
  char fpath[64];

  if (is_badid(userid))
    return -1;

  usr_fpath(fpath, userid, FN_ACCT);
  fd = open(fpath, O_RDWR, 0600);
  if (fd >= 0)
  {
    if (read(fd, acct, sizeof(ACCT)) != sizeof(ACCT))
    {
      close(fd);
      fd = -1;
    }
  }
  return fd;
}
#endif


/* ----------------------------------------------------- */
/* board：shm 部份須與 cache.c 相容			 */
/* ----------------------------------------------------- */


static BCACHE *bshm;


static void
init_bshm()
{
  /* itoc.030727: 在開啟 bbsd 之前，應該就要執行過 account，
     所以 bshm 應該已設定好 */

  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

  if (bshm->uptime <= 0)	/* bshm 未設定完成 */
    exit(0);
}


static BRD *
brd_get(bname)
  char *bname;
{
  BRD *bhdr, *tail;

  bhdr = bshm->bcache;
  tail = bhdr + bshm->number;
  do
  {
    if (!str_cmp(bname, bhdr->brdname))
      return bhdr;
  } while (++bhdr < tail);
  return NULL;
}


static int			/* 1: 找到了這個板，且板名在 brdname */
getbrdname(brdname)
  char *brdname;
{
  BRD *brd;

  if (brd = brd_get(brdname))
  {
    strcpy(brdname, brd->brdname);	/* 換成正確的大小寫 */
    return 1;
  }
  return 0;
}


static void
update_btime(brdname)
  char *brdname;
{
  BRD *brd;

  if (brd = brd_get(brdname))
    brd->btime = -1;
}


/* ----------------------------------------------------- */
/* user：shm 部份須與 cache.c 相容			 */
/* ----------------------------------------------------- */


static UCACHE *ushm;


static inline void
init_ushm()
{
  ushm = shm_new(UTMPSHM_KEY, sizeof(UCACHE));
}


static inline void
bbs_biff(userno)
  int userno;
{
  UTMP *utmp, *uceil;
  usint offset;

  offset = ushm->offset;
  if (offset > (MAXACTIVE - 1) * sizeof(UTMP))	/* Thor.980805: 不然call不到 */
    offset = (MAXACTIVE - 1) * sizeof(UTMP);

  utmp = ushm->uslot;
  uceil = (void *) utmp + offset;

  do
  {
    if (utmp->userno == userno)
      utmp->status |= STATUS_BIFF;
  } while (++utmp <= uceil);
}


/* ----------------------------------------------------- */
/* Hash Table						 */
/* ----------------------------------------------------- */


#define	HASH_TABLE_SIZE		256
#define	HASH_TABLE_SEED		101


typedef struct HashEntry
{
  struct HashEntry *next;
  usint hv;			/* hashing value */
  time_t uptime;
  int visit;			/* reference counts */
  int score;
  int fsize;			/* file size (只有在 title_ht 才有用) */
  struct HashEntry *ttl;	/* title (只有在 mfrom_ht 才有用) */
  char key[0];
}         HashEntry;


typedef struct
{
  int mask;
  int keylen;			/* 0 : string */
  int tale;
  int leak;
  int (*comp) (const void *k1, const void *k2, int len);
  int (*hash) (const void *key, int len);
  HashEntry *bucket[0];
}      HashTable;


static int
he_hash(key, len)
  const unsigned char *key;
  int len;			/* 0 : string */
{
  usint seed, shft;

  seed = HASH_TABLE_SEED;
  shft = 0;

  if (len > 0)
  {
    while (len-- > 0)
    {
      seed += (*key++) << shft;
      shft = (shft + 1) & 7;
    }
  }
  else
  {
    while (len = *key)
    {
      key++;
      seed += len << shft;
      shft = (shft + 1) & 7;
    }
  }

  return seed;
}


static HashTable *
ht_new(size, keylen)
  int size;			/* 2's power */
  int keylen;			/* 0 : key is string */
{
  HashTable *ht;
  int he_len;

  if (size <= 0)
    size = HASH_TABLE_SIZE;

  he_len = size * sizeof(HashEntry *);
  if (ht = (HashTable *) malloc(sizeof(HashTable) + he_len))
  {
    ht->mask = size - 1;
    ht->keylen = keylen;
    ht->tale = 0;
    ht->leak = 0;
    if (keylen)
    {
      ht->hash = he_hash;
      ht->comp = (void *) memcmp;

    }
    else
    {
      ht->hash = (void *) hash32;
      ht->comp = (void *) str_cmp;
    }

    memset(ht->bucket, 0, he_len);
  }
  return ht;
}


#if 0
static void
ht_free(ht)
  HashTable *ht;
{
  int i, len;
  HashEntry *node, *next;

  len = ht->keylen;
  for (i = ht->mask; i >= 0; i--)
  {
    node = ht->bucket[i];
    while (node)
    {
      next = node->next;
      if (len > 0)
	free(node->ttl);
      free(node);
      node = next;
    }
  }

  free(ht);
}


static void
ht_apply(ht, func)
  HashTable *ht;
  int (*func) (const HashEntry * he);
{
  int i, len;
  HashEntry *he, **hp;

  len = ht->len;
  for (i = ht->mask; i >= 0; i--)
  {
    hp = &(ht->bucket[i]);
    while (he = *hp)
    {
      if (func(he) < 0)		/* unlink this entry */
      {
	*hp = he->next;
	if (len > 0)
	  free(he->ttl);
	free(he);
	ht->tale--;
      }
      else
      {
	hp = &(he->next);
      }
    }
  }
}


static HashEntry *
ht_look(ht, key)
  HashTable *ht;
  const void *key;
{
  int len;
  usint hv;
  HashEntry *he;
  int (*comp) ();

  len = ht->keylen;
  comp = ht->comp;
  hv = ht->hash(key, len);
  he = ht->bucket[hv & (ht->mask)];
  while (he)
  {
    if (hv == he->hv && !comp(key, he->key, len))
      break;
    he = he->next;
  }
  return he;
}
#endif


static HashEntry *
ht_add(ht, key)
  HashTable *ht;
  const void *key;
{
  HashEntry *he, **hp;
  int len;
  usint hv;
  int (*comp) ();

  len = ht->keylen;
  comp = ht->comp;
  hv = ht->hash(key, len);
  hp = &(ht->bucket[hv & (ht->mask)]);

  for (;;)
  {
    he = *hp;
    if (he == NULL)
    {
      if (len == 0)
	len = strlen(key) + 1;
      if (he = (HashEntry *) malloc(sizeof(HashEntry) + len))
      {
	*hp = he;
	he->hv = hv;
	he->next = NULL;
	he->visit = 0;
	he->score = 0;
	he->fsize = 0;
	he->ttl = NULL;
	memcpy(he->key, key, len);
	ht->tale++;
	ht->leak++;
      }
      break;
    }

    if (hv == he->hv && !comp(key, he->key, len))
      break;

    hp = &(he->next);
  }

  he->visit++;
  return he;
}


static void
ht_expire(ht, expire)
  HashTable *ht;
  time_t expire;
{
  int i, delta, tale, score;
  HashEntry *he, **hp;

  tale = ht->tale;
  delta = 2 + (ht->leak >> 4) + (tale >> 6);
  ht->leak = 0;

  for (i = ht->mask; i >= 0; i--)
  {
    hp = &(ht->bucket[i]);
    while (he = *hp)
    {
      if (he->uptime < expire)
      {
	score = he->score - delta;
	if (score <= 0 && he->visit <= 0)	/* unlink this entry */
	{
	  *hp = he->next;
	  free(he);
	  tale--;
	  continue;
	}

	if (score < 0)
	  score = 0;
	he->score -= score;
      }

      hp = &(he->next);
    }
  }

  ht->tale = tale;
}


/* ----------------------------------------------------- */
/* Host Hash Table					 */
/* ----------------------------------------------------- */


#define	HOST_HASH_TABLE_SIZE	256
#define	HOST_HASH_ENTRY_LIFE	(30 * 60)
#define	HOST_HASH_ENTRY_DELTA	8


typedef union
{
  unsigned long addr;
  unsigned char ipv4[4];
}     HostAddr;


typedef struct HostHashEntry
{
  unsigned long hostaddr;
  struct HostHashEntry *next;
  time_t uptime;
  int ref;
  int hflag;
  char hostname[0];
}             HostHashEntry;


static HostHashEntry *HostHashTable[HOST_HASH_TABLE_SIZE];


static int
hht_look(addr, host)
  HostAddr *addr;
  char *host;
{
  int hv;
  HostHashEntry *he;

  /* hash value of 140.114.87.5 : 114 ^ 87 ^ 5 */

  hv = (addr->ipv4[1] ^ addr->ipv4[2] ^ addr->ipv4[3]) &
    (HOST_HASH_TABLE_SIZE - 1);

  he = HostHashTable[hv];

  for (;;)
  {
    if (!he)
    {
      int hflag, len;

      hflag = dns_name((char *)addr, host);
      len = strlen(host) + 1;
      he = (HostHashEntry *) malloc(sizeof(HostHashEntry) + len);
      he->hostaddr = addr->addr;
      he->next = HostHashTable[hv];
      he->ref = 0;
      he->hflag = hflag;
      memcpy(he->hostname, host, len);
      HostHashTable[hv] = he;
      break;
    }

    if (he->hostaddr == addr->addr)
    {
      strcpy(host, he->hostname);
      break;
    }

    he = he->next;
  }

  he->ref++;
  time(&he->uptime);

  return he->hflag;
}


static void
hht_expire(now)
  time_t now;
{
  int i;
  HostHashEntry **hp, *he;

  now -= HOST_HASH_ENTRY_LIFE;

  for (i = 0; i < HOST_HASH_TABLE_SIZE; i++)
  {
    hp = &HostHashTable[i];

    while (he = *hp)
    {
      if (he->uptime < now)
      {
	if (he->ref <= HOST_HASH_ENTRY_DELTA)
	{
	  *hp = he->next;
	  free(he);
	  continue;
	}

	he->ref -= HOST_HASH_ENTRY_DELTA;
      }

      hp = &(he->next);
    }
  }
}


/* ----------------------------------------------------- */
/* Anti Spam						 */
/* ----------------------------------------------------- */


static HashTable *mrcpt_ht;
static HashTable *mhost_ht;
static HashTable *mfrom_ht;
static HashTable *title_ht;


#ifdef FORGE_CHECK
static HashTable *forge_ht;

static int		/* 1: host/domain 是假造的 */
is_forge(host)
  char *host;
{
  HashEntry *he;
  int score;
  unsigned long addr;

  he = ht_add(forge_ht, host);
  if ((score = he->score) == 0)
  {
    /* he->uptime = addr = dns_addr(host); */

    /* Thor.990811: check forge by dns mx & a record */
    char mxlist[MAX_MXLIST];
    /* if (dns_aton(domain) != INADDR_NONE) return; */
    dns_mx(host, mxlist);
    if (!*mxlist && dns_a(host) == INADDR_NONE)
      he->uptime = addr = INADDR_NONE;	/* Thor.990811: 是負的, 會被 expire */
    else
      he->uptime = addr = time(0);	/* Thor.990811: 反正只要不是INADDR_NONE就可以了 */
  }
  else
    addr = he->uptime;

  /* if (addr != INADDR_NONE) */	/* Thor.990811: 存在的host才加分，留著 */
  he->score = ++score;

  if (score == SPAM_FORGE_LIMIT)
  {
    fprintf(flog, "FORGE_L\t%s\n", host);
    he->score = 0;		/* Thor.000623: release this hashing entry for expire aging */
  }

  return (addr == INADDR_NONE);
}

#else

static int		/* 1: host/domain 是假造的 */
is_forge(host)
  char *host;
{
  int cc;
  char *str;

  str = NULL;

  while (cc = *host)
  {
    host++;
    if (cc == '.')
      str = host;
  }

  return ((str == NULL) || (host - str <= 1));
}
#endif		/* FORGE_CHECK */


static void
spam_add(he)
  HashEntry *he;
{
  FILE *fp;

  if (fp = fopen(UNMAIL_ACLFILE, "a"))
  {
    struct tm *p;

    p = localtime(&he->uptime);
    str_lower(he->key, he->key);
    /* Thor.990329: y2k */
    fprintf(fp, "%s # %d %02d/%02d/%02d %02d:%02d:%02d\n",
      he->key, he->score,
      p->tm_year % 100, p->tm_mon + 1, p->tm_mday,
      p->tm_hour, p->tm_min, p->tm_sec);
    fclose(fp);
  }

  he->score = 0;		/* release this hashing entry */
}


/* ----------------------------------------------------- */
/* statistics of visitor				 */
/* ----------------------------------------------------- */


#include "splay.h"


static int
vx_cmp(x, y)
  HashEntry *x;
  HashEntry *y;
{
  int dif;

  dif = y->visit - x->visit;
  if (dif)
    return dif;
  return str_cmp(x->key, y->key);
}


static void
vx_out(fp, top)
  FILE *fp;
  SplayNode *top;
{
  HashEntry *he;

  if (top == NULL)
    return;

  vx_out(fp, top->left);

  he = (HashEntry *) top->data;

  fprintf(fp, "%6d %s\n", he->visit, he->key);

  vx_out(fp, top->right);
}


static void
splay_free(top)
  SplayNode *top;
{
  SplayNode *node;

  if (top == NULL)
    return;

  if (node = top->left)
    splay_free(node);

  if (node = top->right)
    splay_free(node);

  MYDOG;
  free(top);
  MYDOG;
}


static void
vx_log(fp, tag, ht)
  FILE *fp;
  char *tag;
  HashTable *ht;
{
  int i, nentry, nvisit;
  SplayNode *top;
  HashEntry *he;

  fprintf(fp, "[%s]\n\n", tag);

  top = NULL;
  nentry = 0;
  nvisit = 0;

  /* splay sort */

  for (i = ht->mask; i >= 0; i--)
  {
    for (he = ht->bucket[i]; he; he = he->next)
    {
      nentry++;
      nvisit += he->visit;
      top = splay_in(top, he, vx_cmp);
    }
  }

  /* report */

  fprintf(fp, "%d entry, %d visit:\n\n", nentry, nvisit);
  vx_out(fp, top);

  /* free memory */

  splay_free(top);

  for (i = ht->mask; i >= 0; i--)
  {
    for (he = ht->bucket[i]; he; he = he->next)
    {
      he->visit = 0;
    }
  }
}


static void
visit_fresh()
{
  char folder[64], fpath[64];
  FILE *fp;
  HDR hdr;

  brd_fpath(folder, BN_JUNK, FN_DIR);

  if (!(fp = fdopen(hdr_stamp(folder, 'A', &hdr, fpath), "w")))
    return;

  vx_log(fp, "Host", mhost_ht);
  vx_log(fp, "From", mfrom_ht);
  vx_log(fp, "Rcpt", mrcpt_ht);
  vx_log(fp, "標題", title_ht);
  fclose(fp);

  hdr.xmode = POST_MARKED;
  strcpy(hdr.owner, "<BMTA>");
  strcpy(hdr.title, "統計資料");
  rec_bot(folder, &hdr, sizeof(HDR));

  update_btime(BN_JUNK);
}


/* ----------------------------------------------------- */
/* memo of mail header / body				 */
/* ----------------------------------------------------- */


static void
mta_memo(ap, mark)
  Agent *ap;
  int mark;
{
  /* char folder[64], fpath[64], nick[80], *memo; */
  char folder[64], fpath[64], nick[256], *memo;	/* Thor.000131: 以防萬一 */
  FILE *fp;
  HDR hdr;

  memo = ap->nick;
  if (*memo)
    sprintf(nick, " (%s)", memo);
  else
    nick[0] = '\0';

  memo = ap->memo;
  if (!*memo)
  {
    memo = ap->fpath;
    if (!*memo)
    {
      memo = ap->title;
    }
  }

  brd_fpath(folder, BN_JUNK, FN_DIR);

  if (!(fp = fdopen(hdr_stamp(folder, 'A', &hdr, fpath), "w")))
    return;

  /* Thor.990915: 顯示 mail from 以便追蹤 */
  fprintf(fp, "MAIL FROM: <%s>\nFrom: %s%s\nSubj: %s\nDate: %s\n"
    "Host: %s\nMemo: %s\nFile: %s\nSize: %d\n%s",
    ap->from, ap->addr, nick, ap->title, Btime(&hdr.chrono),
    ap->ident, ap->memo, ap->fpath, ap->used, ap->data);
  fclose(fp);

  if (mark)
    hdr.xmode = POST_MARKED;

  strcpy(hdr.owner, "<BMTA>");
  strcpy(hdr.title, memo);
  rec_bot(folder, &hdr, sizeof(HDR));

  update_btime(BN_JUNK);
}


/* ----------------------------------------------------- */
/* mailers						 */
/* ----------------------------------------------------- */


static int
bbs_mail(ap, data, userid)
  Agent *ap;
  char *data;
  char *userid;
{
  HDR hdr;
  int fd, method, sno;
  FILE *fp;
  char folder[80], buf[256], from[256], *author, *fpath, *title;
  struct stat st;

  fp = flog;
  sno = ap->sno;

  usr_fpath(folder, userid, FN_DIR);

  /* Thor.990617: get file stat */
  if (!stat(folder, &st) && st.st_size > MAX_BBSMAIL * sizeof(HDR))
  {
    fprintf(fp, "MAIL-\t[%d] <%s> over-spammed\n", sno, userid);
    return -1;
  }

  /* allocate a file for the new mail */

  fpath = ap->fpath;
  method = *fpath ? HDR_LINK : 0;
  if ((fd = hdr_stamp(folder, method, &hdr, fpath)) < 0)
  {
    fprintf(fp, "MAIL-\t[%d] <%s> stamp error\n", sno, userid);
    return -2;
  }

  author = ap->addr;
  title = ap->nick;
  sprintf(ap->memo, "%s -> %s", author, userid);

  if (*title)
    sprintf(from, "%s (%s)", author, title);
  else
    strcpy(from, author);

  title = ap->title;
  if (!method)
  {
    if (!*title)
      sprintf(title, "來自 %.64s", author);

    sprintf(buf, "作者: %.72s\n標題: %.72s\n時間: %s\n\n",
      from, title, Btime(&hdr.chrono));

    write(fd, buf, strlen(buf));
    write(fd, data, ap->data + ap->used - data);
    close(fd);
  }

  hdr.xmode = MAIL_INCOME;
  str_ncpy(hdr.owner, author, sizeof(hdr.owner));
  str_ncpy(hdr.title, title, sizeof(hdr.title));
  rec_add(folder, &hdr, sizeof(HDR));

  fprintf(fp, "%d\t%s\t%s\t%s/%s\n", sno, author, title, userid, hdr.xname);
  ap->xrcpt++;
  ap->xdata += ap->used;

  /* --------------------------------------------------- */
  /* 通知 user 有新信件					 */
  /* --------------------------------------------------- */

  sprintf(folder, "usr/%c/%s/.ACCT", *userid, userid);
  fd = open(folder, O_RDONLY);
  if (fd >= 0)
  {
    if ((read(fd, &sno, sizeof(sno)) == sizeof(sno)) && (sno > 0))
      bbs_biff(sno);
    close(fd);
  }

  mta_memo(ap, 0);
  return 0;
}


static int
bbs_brd(ap, data, brdname)	/* itoc.030323: 寄信給看板 */
  Agent *ap;
  char *data;
  char *brdname;
{
  HDR hdr;
  int fd, method, sno;
  FILE *fp;
  char folder[80], buf[256], from[256], *author, *fpath, *title;

  fp = flog;
  sno = ap->sno;

  brd_fpath(folder, brdname, FN_DIR);

  /* allocate a file for the new post */

  fpath = ap->fpath;
  method = *fpath ? HDR_LINK | 'A' : 'A';
  if ((fd = hdr_stamp(folder, method, &hdr, fpath)) < 0)
  {
    fprintf(fp, "MAIL-\t[%d] <%s> stamp error\n", sno, brdname);
    return -2;
  }

  author = ap->addr;
  title = ap->nick;
  sprintf(ap->memo, "%s -> %s", author, brdname);

  if (*title)
    sprintf(from, "%s (%s)", author, title);
  else
    strcpy(from, author);

  title = ap->title;
  if (method == 'A')
  {
    if (!*title)
      sprintf(title, "來自 %.64s", author);

    sprintf(buf, "發信人: %.50s 看板: %s\n標  題: %.72s\n發信站: %s\n\n",
      from, brdname, title, Btime(&hdr.chrono));

    write(fd, buf, strlen(buf));
    write(fd, data, ap->data + ap->used - data);
    close(fd);
  }

  hdr.xmode = POST_INCOME;
  str_ncpy(hdr.owner, author, sizeof(hdr.owner));
  str_ncpy(hdr.title, title, sizeof(hdr.title));
  rec_bot(folder, &hdr, sizeof(HDR));

  update_btime(brdname);

  fprintf(fp, "%d\t%s\t%s\t%s/%s\n", sno, author, title, brdname, hdr.xname);
  ap->xrcpt++;
  ap->xdata += ap->used;

  mta_memo(ap, 0);
  return 0;
}


#ifdef EMAIL_JUSTIFY
static int
bbs_valid(ap)
  Agent *ap;
{
  int fd, sno;
  FILE *fp;
  char pool[256], folder[64], justify[128], *str, *ptr, *userid;
  ACCT acct;
  HDR hdr;

  fp = flog;
  sno = ap->sno;
  ptr = ap->title;

  /* itoc.註解: mail.c: TAG_VALID " userid(regkey) [VALID]" */
  if (!(str = strstr(ptr, TAG_VALID)))
  {
    sprintf(ap->memo, "REG : %.64s", ptr);
    fprintf(fp, "REG-\t[%d] %s\n", sno, ptr);
    return -2;
  }

  userid = pool;
  strcpy(userid, str + sizeof(TAG_VALID));
  if (!(str = strchr(userid, '(')))
    return -1;

  *str = '\0';

  if (!(ptr = (char *) strchr(str + 1, ')')) || !strstr(ptr, "[VALID]"))
  {
    sprintf(ap->memo, "REG - %s (format)", userid);
    fprintf(fp, "REG-\t[%d] <%s> format\n", sno, userid);
    return -1;
  }

  *ptr++ = 0;

  if ((fd = acct_fetch(userid, &acct)) < 0)
  {
    sprintf(ap->memo, "REG - %s (not exist)", userid);
    fprintf(fp, "REG-\t[%d] <%s> not exist\n", sno, userid);
    return -1;
  }

  if (str_hash(acct.email, acct.tvalid) != chrono32(str))
  {
    close(fd);
    sprintf(ap->memo, "REG - %s (checksum)", userid);
    fprintf(fp, "REG-\t[%d] <%s>  check sum: %s\n", sno, acct.userid, str + 1);
    return -1;
  }

  /* 提升權限 */
  acct.userlevel |= PERM_VALID;
  time(&acct.tvalid);
  lseek(fd, 0, SEEK_SET);
  write(fd, &acct, sizeof(ACCT));
  close(fd);

  usr_fpath(folder, userid, FN_DIR);
  if (!hdr_stamp(folder, HDR_LINK, &hdr, FN_ETC_JUSTIFIED))
  {
    strcpy(hdr.title, MSG_REG_VALID);
    strcpy(hdr.owner, STR_SYSOP);
    hdr.xmode = MAIL_NOREPLY;
    rec_add(folder, &hdr, sizeof(HDR));
  }

  ptr = ap->nick;
  if (*ptr)
    sprintf(justify, "RPY: %s (%s)", ap->addr, ptr);
  else
    sprintf(justify, "RPY: %s", ap->addr);

  usr_fpath(folder, userid, FN_JUSTIFY);
  if (fp = fopen(folder, "a"))
  {
    fprintf(fp, "%s\n", justify);
    fclose(fp);
  }

  /* 在 usr 目錄留下完整回信記錄 */
  usr_fpath(folder, userid, FN_EMAIL);
  if (fp = fopen(folder, "w"))
  {
    fprintf(fp, "ID: %s\nVALID: %s\nHost: %s\nFrom: %s\n%s\n",
      userid, justify, ap->ident, ap->addr, ap->data);
    fclose(fp);
  }

  /* Thor.990414: 加印 Timestamp, 方便追查 */
  fprintf(fp, "REG\t[%d] <%s> %s %s\n", sno, acct.userid, justify, str + 1);
  sprintf(ap->memo, "REG + %s", userid);
  ap->xrcpt++;

  return 0;
}
#endif


/* ----------------------------------------------------- */
/* Access Control List routines				 */
/* ----------------------------------------------------- */

/* ----------------------------------------------------- */
/* ACL config file format				 */
/* ----------------------------------------------------- */
/* user:	majordomo@	bad@cs.nthu.edu.tw	 */
/* host:	cs.nthu.edu.tw	140.114.77.1		 */
/* subnet:	.nthu.edu.tw	140.114.77.		 */
/* ----------------------------------------------------- */



typedef struct ACL_t
{
  struct ACL_t *nacl;		/* next acl */
  int locus;
  unsigned char filter[0];
}     ACL_t;


static int
str_cpy(dst, src, n)
  unsigned char *dst;
  unsigned char *src;
  int n;
{
  int cc, len;

  len = 0;
  for (;;)
  {
    cc = *src;
    if (cc >= 'A' && cc <= 'Z')
      cc |= 0x20;
    *dst = cc;
    if ((len > n) || (!cc))	/* lkchu.990511: 避免 overflow */
      break;
    src++;
    dst++;
    len++;
  }
  return len;
}


static ACL_t *
acl_add(root, filter)
  ACL_t *root;
  unsigned char *filter;
{
  int at, cc, len;
  char *str;
  ACL_t *ax;

  str = filter;
  at = len = 0;
  for (;;)
  {
    cc = *str;
    if (cc == '\n' || cc == ' ' || cc == '#' || cc == '\t' || cc == '\r')
    {
      *str = '\0';
      break;
    }
    if (!cc)
      break;

    str++;
    len++;
    if (cc == '@')
      at = -len;
  }

  if (len <= 0)
    return root;

  ax = (ACL_t *) malloc(sizeof(ACL_t) + len + 1);

  ax->nacl = root;
  ax->locus = at ? at : len;

  str = ax->filter;
  do
  {
    cc = *filter++;
    if (cc >= 'A' && cc <= 'Z')
      cc |= 0x20;
  } while (*str++ = cc);

  return ax;
}


static ACL_t *
acl_load(fpath, root)
  char *fpath;
  ACL_t *root;
{
  FILE *fp;
  char buf[256];
  ACL_t *ax, *nacl;

  /* 先清空 */
  if (ax = root)
  {
    do
    {
      nacl = ax->nacl;
      free(ax);
    } while (ax = nacl);

    ax = NULL;
  }

  if (fp = fopen(fpath, "r"))
  {
    fpath = buf;
    while (fgets(fpath, sizeof(buf), fp))
    {
      if (!*fpath)
	break;

      if (*fpath == '#')
	continue;

      ax = acl_add(ax, fpath);
    }
    fclose(fp);
  }

  return ax;
}


static int
acl_match(root, ruser, rhost)
  ACL_t *root;  
  unsigned char *ruser;
  unsigned char *rhost;
{
  ACL_t *ax;
  unsigned char *filter, xuser[80], xhost[128];
  int lhost, luser, locus, len;

  if (!(ax = root))
    return 0;

  /* lkchu.990511: rhost 和 ruser 都沒有先檢查長度就 copy, 
                   很可能發生 segmentation fault */
  luser = str_cpy(xuser, ruser, sizeof(xuser));
  lhost = str_cpy(xhost, rhost, sizeof(xhost));
  ruser = xuser;
  rhost = xhost;

  do
  {
    filter = ax->filter;
    locus = ax->locus;

    /* match remote user name */

    if (locus < 0)
    {
      len = -1 - locus;
      if ((len != luser) || memcmp(ruser, filter, luser))
	continue;

      filter -= locus;
      if (*filter == '\0')	/* majordomo@ */
	return 1;
      locus = strlen(filter);
    }

    /* match remote host name */

#if 0
    if (locus == lhost)
    {
      if (!strcmp(filter, rhost))
	return 1;
    }
    else if (locus < lhost)
    {
      if (*filter == '.')
      {
	if (!strcmp(filter, rhost + lhost - locus))
	  return 1;
      }
      else if (filter[locus - 1] == '.')
      {
	if (!memcmp(filter, rhost, locus))
	  return 1;
      }
    }
#endif

    /* subnet 相同也算 match */
    if (locus <= lhost)
    {
      if (!strcmp(rhost + lhost - locus, filter))
	return 1;
    }
  } while (ax = ax->nacl);

  return 0;
}


static ACL_t *mail_root = NULL;		/* MAIL_ACLFILE 的 acl_root */
static ACL_t *unmail_root = NULL;	/* UNMAIL_ACLFILE 的 acl_root */


static int		/* 1: spam */
acl_spam(ruser, rhost)
  unsigned char *ruser;
  unsigned char *rhost;
{
  /* 不在白名單上或在黑名單上 */
  return (!acl_match(mail_root, ruser, rhost) || acl_match(unmail_root, ruser, rhost));
}


/* ----------------------------------------------------- */
/* mail header routines					 */
/* ----------------------------------------------------- */

/* ----------------------------------------------------- */
/* From xyz Wed Dec 24 17:05:37 1997			 */
/* From: xyz (nick)					 */
/* From user@domain  Wed Dec 24 18:00:26 1997		 */
/* From: user@domain (nick)				 */
/* ----------------------------------------------------- */


static char *
mta_from(ap, str)
  Agent *ap;
  unsigned char *str;		/* Thor.990629: 中文? */
{
  int cc;
  char pool[512], *head, *tail;

  head = pool;
  tail = head + sizeof(pool) - 1;

  for (;;)
  {
    /* skip leading space */
    while (*str == ' ' || *str == '\t')
      str++;

    /* copy the <From> to buffer pool */

    for (;;)
    {
      cc = *str;

      if (cc == '\0')
      {
	*head = '\0';
	sprintf(head, "%s %s", pool, ap->addr);
	agent_log(ap, "From:", head);
	return str;
      }

      str++;
      if (cc == '\n')
	break;

      *head++ = cc;

      if (head >= tail)
	return str;
    }

    /* if (*str != ' ') */
    if (*str > ' ' || *str == '\n')	/* Thor.990617: for merge multiline */
      break;

    /* go on to merge multi-line <From> */
  }

  *head = '\0';

  str_from(pool, head = ap->addr, ap->nick);

  /* Thor.000909: 沒有head (from)的就自己補上了 */
  if (!*head)
    strcpy(head, ap->from);

  if (str_cmp(head, ap->from))
  {	/* Thor.000911.註解: 如果不一樣的話, 要check; 一樣的之前check過了 */
    if (tail = strchr(head, '@'))	/* Thor.000911.註解: 正常的addr的話 */
    {
      *tail++ = '\0';
      
      if (is_forge(tail))	/* Thor.990811: 假造的, 想都別想 */
	return NULL;

      /* 檢查 From: 是否在黑白名單上 */
      if (!(ap->mode & (AM_VALID | AM_BBSADM)) && acl_spam(head, tail))
      {
	tail[-1] = '@';
	agent_log(ap, "SPAM-M", head);
	return NULL;
      }

      tail[-1] = '@';
    }
    else	/* Thor.000911: 不收不正常的 addr */
    {
      return NULL;
    }
  }

  return str;
}


static char *
mta_subject(ap, str)
  Agent *ap;
  unsigned char *str;
{
  int cc;
  char pool[640], *head, *tail;	/* *line */

  head = pool;
  tail = head + sizeof(pool) - 128;

  /* Thor.980831: modified for multi-line header */

  /* skip leading space */

  /* while (*str == ' ') str++; */
  while (*str == ' ' || *str == '\t')
    str++;

  /* copy the <Subject> to buffer pool */

  for (;;)
  {
    cc = *str;

    if (cc == '\0')
    {
      sprintf(head, "%s %s", ap->from, ap->addr);
      agent_log(ap, "Subj:", pool /* head */ );	/* Thor.980904:想看subj全貌 */
      return str;
    }

    str++;
    /* Thor.980906: |no body |header       |body seperator */
    if (cc == '\n')
    {
      if (!*str || *str > ' ' || *str == '\n')
	break;
      /* Thor.991014: skip next line leading space */
      while (*str == ' ' || *str == '\t')
	str++;
      continue;
    }

    *head++ = cc;

    if (head >= tail)		/* line too long */
    {
      agent_log(ap, "Subj:", pool);	/* Xshadow.980906: really line too long? */
      return str;
    }
  }

  *head = 0;

  str_decode(pool);
  str_ansi(ap->title, pool, sizeof(ap->title));
  return str;
}


static inline int
is_host_alias(addr)
  char *addr;
{
  int i;
  char *str;
  static char *alias[] = HOST_ALIASES;

  /* check the aliases */

  for (i = 0; str = alias[i]; i++)
  {
    if (!str_cmp(addr, str))
      return 1;
  }
  return 0;
}


/* Thor.980901: mail decode, only for ONE part, not for "This is a multi-part message in MIME format." */

static char *
mta_decode(ap, str, code)
  Agent *ap;
  unsigned char *str;
  char *code;
{
  str = mm_getencode(str, code);

  /* skip whole line */
  while (*str && *str++ != '\n')
    ;

  return str;
}


static char *
mta_boundary(ap, str, boundary)
  Agent *ap;
  unsigned char *str;
  char *boundary;
{
  int cc;
  char *base = boundary;

  *boundary = 0;
  /* skip leading space */
  while (*str == ' ')
    str++;

  if (!str_ncmp(str, "multipart", 9))
  {
    char *tmp;

    if (tmp = str_str(str, "boundary="))
    {
      /* Thor.990221: 居然有的人不用 " */
      tmp += 9;
      if (*tmp == '"')
	tmp++;

      while (*tmp && *tmp != '"' && *tmp != '\n')
	*boundary++ = *tmp++;
      /* *boundary++ = '\n'; */	/* Thor.980907: 之後會被無意間跳過 */
      *boundary = 0;
    }
    logit("MULTI", base);
  }

  while (cc = *str)
  {
    str++;
    if (cc == '\n' && (!*str || *str > ' ' || *str == '\n'))
      break;
  }

  return str;
}


/* Thor.980907: support multipart mime */
/* ----------------------------------------------------- */
/* multipart decoder					 */
/* ----------------------------------------------------- */


static int
multipart(src, dst, boundary)
  unsigned char *src;
  unsigned char *dst;		/* Thor: no ending 0 */
  unsigned char *boundary;	/* Thor: should include "--" */
{
  unsigned char *base = dst;
  char *bound;
  unsigned char *tmp;
  char decode;
  char buf[512] = "--";		/* Thor: sub mime boundary */
  int cc;
  int boundlen;

  boundlen = strlen(boundary);
  if (boundlen < 6)
    return 0;			/* Thor: boundary too small, "\n>  <\n" */

  while (*src)
  {
    bound = strstr(src, boundary);

    if (bound)
      *bound = 0;

    if (buf[2])			/* Thor: multipart & encoded can't be happened simutaneously */
    {
      cc = multipart(src, dst, buf);
      if (cc > 0)
	dst += cc;
      else
	goto bypass;
    }
    else
    {
  bypass:
      while (*src)
	*dst++ = *src++;
    }

    /* reset */
    buf[2] = 0;
    decode = 0;

    if (!bound)
      break;

    src = bound + boundlen;	/* Thor: src over boundary */

    tmp = dst + boundlen - 3 /* " <\n" */ ;

    *dst++ = '\n';
    *dst++ = '>';
    *dst++ = ' ';

    while (dst < tmp)
      *dst++ = '-';

    *dst++ = ' ';
    *dst++ = '<';
    *dst++ = '\n';

    for (;;)			/* Thor: processing sub-header */
    {
      if (!str_ncmp(src, "Content-Transfer-Encoding:", 26))
      {				/* Thor.980901: 解 rfc1522 body code */
	src = mta_decode(NULL, src + 26, &decode);
      }
      else if (!str_ncmp(src, "Content-Type:", 13))
      {
	src = mta_boundary(NULL, src + 13, buf + 2);
      }
      else
      {
	while (cc = *src)
	  /* Thor.980907: 剛好跳了boundary後的 \n,不是故意的 :p */
	{
	  src++;
	  if (cc == '\n')
	    break;
	}
      }
      if (!*src || *src == '\n')
	break;			/* null body or end of mail header */
    }

  }
  return dst - base;
}


/* ----------------------------------------------------- */
/* mailer						 */
/* ----------------------------------------------------- */


static int
mta_mailer(ap)
  Agent *ap;
{
  char *data, *str, *addr, *delimiter, decode = 0;
  char boundary[512] = "--";
  int cc, mode;
  RCPT *rcpt;

  mode = ap->mode;
  data = ap->data + 1;		/* skip leading stuff */

  /* --------------------------------------------------- */
  /* parse the mail header and filter spam mails	 */
  /* --------------------------------------------------- */

  for (;;)
  {
    if (!str_ncmp(data, "From:", 5))
    {
      data += 5;
      data = mta_from(ap, data);
      if (!data)
      {
	agent_reply(ap, "550 you are not in my access list");
	return -1;
      }
    }
    else if (!str_ncmp(data, "Subject:", 8))
    {
      data = mta_subject(ap, data + 8);
    }
    else if (!str_ncmp(data, "Content-Transfer-Encoding:", 26))
    {				/* Thor.980901: 解 rfc1522 body code */
      data = mta_decode(ap, data + 26, &decode);
    }
    else if (!str_ncmp(data, "Content-Type:", 13))
    {				/* Thor.980907: 解 multi-part */
#ifdef ANTI_HTMLMAIL
      /* 一般 BBS 使用者通常只寄文字郵件或是從其他 BBS 站寄文章到自己的信箱
         而廣告信件通常是 html 格式或是裡面有夾帶其他檔案
         利用郵件的檔頭有 Content-Type: 的屬性把除了 text/plain (文字郵件) 的信件都擋下來 */
      char *content = data + 14;
      if (*content != '\0' && str_ncmp(content, "text/plain", 10))
      {
	agent_reply(ap, "550 we only accept plain text");
	return -1;
      }
#endif

#ifdef ANTI_NOTMYCHARSETMAIL
      {
	char charset[32];
	mm_getcharset(data + 13, charset, sizeof(charset));
	if (str_cmp(charset, MYCHARSET) && str_cmp(charset, "us-ascii"))
	{
	  agent_reply(ap, "550 non-supported charset");
	  return -1;
	}
      }
#endif

      data = mta_boundary(ap, data + 13, boundary + 2);
    }
    else
    {
      /* skip this line */
      for (;;)
      {
	cc = *data;

	if (cc == '\0')		/* null body */
	{
	  data--;
	  goto mta_mail_body;
	  /* return 0; */
	}

	data++;

	if (cc == '\n')
	  break;
      }
    }

    if (*data == '\n')
      break;			/* end of mail header */
  }

  /* --------------------------------------------------- */
  /* process the mail body				 */
  /* --------------------------------------------------- */

mta_mail_body:

  if (mode & AM_BBSADM)
  {
    sprintf(ap->memo, "ADM: %.64s", ap->from);
    mta_memo(ap, 1);		/* lkchu: mark ADM's letter */

    return 0;
  }

  *data = '\0';

  /* --------------------------------------------------- */
  /* validate user's character				 */
  /* --------------------------------------------------- */

  rcpt = ap->rcpt;

#ifdef EMAIL_JUSTIFY
  if (mode & AM_VALID)
  {
    /* Thor.000328: 註解: rcpt to bbsreg@MYHOSTNAME */
    if (bbs_valid(ap) == -2)
      *data = '\n';

    mta_memo(ap, 0);

    if (rcpt == NULL)
      return 0;
  }
#endif

  /* --------------------------------------------------- */
  /* check mail body					 */
  /* --------------------------------------------------- */

  delimiter = data;

  for (;;)
  {
    cc = *++data;
    if (cc == '\0')		/* null mail body */
    {
      /* Thor.000327: 記錄空信 */
      fprintf(flog, "NULL BODY\t[%d] from:%s nrcpt:%d\n", ap->sno, ap->from, ap->nrcpt);
      return 0;
    }

    if (cc != '\n')		/* skip empty lines in mail body */
      break;
  }

  /* --------------------------------------------------- */
  /* decode mail body					 */
  /* --------------------------------------------------- */

  /* Thor.980901: decode multipart body */
  if (boundary[2])
  {
    /* logit("MULTIDATA",data); */
    cc = multipart(data, data, boundary);
    if (cc > 0)
      ap->used = (data - ap->data) + cc;	/* (data - ap->data) 是 header 長度，cc 是信內容的長度 */
  }
  /* Thor.980901: decode mail body */
  else if (decode)
  {
    /* data[mmdecode(data,decode,data)]=0; */
    /* Thor.980901: 因 decode必為 b or q, 故 mmdecode不為-1, *
     * 必定成功, 又因 mmdecode不自動加 0, 故手動加上         */

    /* Thor.980901: 不用 0 作結束, 而用長度, 計算方式如下    *
     * write(fd, data, ap->data + ap->used - data);          *
     * 故修改 ap->used, 以截掉過長 data                      */

    cc = mmdecode(data, decode, data);
    if (cc > 0)
      ap->used = (data - ap->data) + cc;	/* (data - ap->data) 是 header 長度，cc 是信內容的長度 */

    /* logit("DECODEDATA", data); */
  }

  /* --------------------------------------------------- */
  /* check E-mail address for anti-spam first		 */
  /* --------------------------------------------------- */

  addr = ap->addr;
  if ((str = strchr(addr, '@')) && str_ncmp(addr, "mailer-daemon@", 14))
  {
    HashEntry *he, *hx;
    int nrcpt, score, delta;
    time_t uptime;

    uptime = ap->uptime;
    nrcpt = ap->nrcpt;

    /* -------------------------------------------------- */
    /* 檢查這個 @host 寄進來的信有無超過 SPAM_MHOST_LIMIT */
    /* -------------------------------------------------- */

    he = ht_add(mhost_ht, ++str);
    he->uptime = uptime;
    he->score += (nrcpt > 0) ? nrcpt : 1;	/* 一個收件人都沒有也算一次訪問 */

    if (he->score >= SPAM_MHOST_LIMIT)
    {
      unmail_root = acl_add(unmail_root, str);
      spam_add(he);
      fprintf(flog, "SPAM-H\t[%d] %s\n", ap->sno, str);

      sprintf(ap->memo, "SPAM : %s", str);
      *delimiter = '\n';
    }

    /* -------------------------------------------- */
    /* 檢查這個 title 的信有無超過 SPAM_TITLE_LIMIT */
    /* -------------------------------------------- */

    if (nrcpt > 0)
    {
      hx = ht_add(title_ht, str_ttl(ap->title));
      hx->uptime = uptime;
      hx->visit += nrcpt - 1;	/* title_ht 的 visit 是記錄這標題的信有幾人收過 */
      hx->score += nrcpt;
      if (hx->score >= SPAM_TITLE_LIMIT)
	fprintf(flog, "TITLE\t[%d] %s\n", ap->sno, ap->title);
 
      /* 如果這次來信和上次同標題的來信檔案差不多大，那麼這次來信很可能是廣告信 */
      score = nrcpt;
      delta = hx->fsize - ap->used;
      if (delta >= -16 && delta <= 16)
	score +=  SPAM_MFROM_LIMIT >> 6;
      hx->fsize = ap->used;		/* 記錄用這標題的最後一封信之檔案大小 */
    }
    else
    {
      score = 1;
    }
    
    /* ------------------------------------------------- */
    /* 檢查這個 from 寄進來的信有無超過 SPAM_MFROM_LIMIT */
    /* ------------------------------------------------- */

    he = ht_add(mfrom_ht, addr);
    he->uptime = uptime;

    if (nrcpt > 0)	/* 有 title_ht 的 HashEntry hx-> */
    {
      /* itoc.060420.註解: 有些使用者會從別的 BBS 站一次轉寄整個討論串的文章(同標題)
         來本站，就會因為下面這條 rule 而被視為廣告商 */

      /* 如果這個 from 在這次來信和他自己上次來信的標題相同，那麼這個 from 很可能是廣告商 */
      if (he->ttl == hx)
	score += SPAM_MFROM_LIMIT >> 5;
      else
	he->ttl = hx;		/* 記錄這個 from 在這次來信的標題 */
    }

    he->score += score;

    if (he->score >= SPAM_MFROM_LIMIT)
    {
      unmail_root = acl_add(unmail_root, addr);
      spam_add(he);
      fprintf(flog, "SPAM-F\t[%d] %s\n", ap->sno, addr);

      sprintf(ap->memo, "SPAM : %s", addr);
      *delimiter = '\n';
    }

    /* ------------------------------------------------- */

    if (*delimiter)	/* 若 *delimiter == '\n'，表示超過 SPAM_*_LIMIT */
    {
      MYDOG;
      mta_memo(ap, 0);
      MYDOG;
      return 0;
    }
  }

  /* --------------------------------------------------- */
  /* mail user						 */
  /* --------------------------------------------------- */

  if (rcpt)
  {
    char *dot;

    addr = ap->addr;
    if (dot = strchr(addr, '.'))
    {
      if (!str_cmp(dot, ".bbs@" MYHOSTNAME))	/* itoc.020125.註解: 若寄信者為本站發信，只留 ID */
	*dot = '\0';
      else
	dot = NULL;
    }

    do
    {
      str = rcpt->userid;
      if (ap->letter)
	bbs_mail(ap, data, str);
      else
	bbs_brd(ap, data, str);
    } while (rcpt = rcpt->rnext);

    if (dot)
      *dot = '.';
  }

  return 0;
}


/* ----------------------------------------------------- */
/* command dispatch					 */
/* ----------------------------------------------------- */


static void
agent_free_rcpt(ap)
  Agent *ap;
{
  RCPT *rcpt, *next;

  ap->nrcpt = 0;

  if (rcpt = ap->rcpt)
  {
    ap->rcpt = NULL;
    do
    {
      next = rcpt->rnext;
      free(rcpt);
    } while (rcpt = next);
  }
}


static void
agent_spam(ap)
  Agent *ap;
{
  int cc;
  char *from;
  FILE *fp;

  from = ap->from;
  cc = *from;
  if (cc == ' ' || cc == '\0')
    return;

  unmail_root = acl_add(unmail_root, from);

  if (fp = fopen(UNMAIL_ACLFILE, "a"))
  {
    struct tm *p;

    p = localtime(&ap->uptime);
    /* Thor.990329: y2k */
    fprintf(fp, "%s # 100 %02d/%02d/%02d %02d:%02d:%02d\n",
      from,
      p->tm_year % 100, p->tm_mon + 1, p->tm_mday,
      p->tm_hour, p->tm_min, p->tm_sec);
    fclose(fp);
  }
}


/* ----------------------------------------------------- */
/* command dispatch					 */
/* ----------------------------------------------------- */


static void
cmd_what(ap)
  Agent *ap;
{
  ap->xerro++;
  agent_reply(ap, "500 Command unrecognized");
}


static void
cmd_help(ap)
  Agent *ap;
{
  agent_reply(ap, "214-Commands:\r\n"
    "214-    HELO    MAIL    RCPT    DATA\r\n"
    "214-    NOOP    QUIT    RSET    HELP\r\n"
    "214-See RFC-821 for more info.\r\n"
    "214 End of HELP info");
}


static void
cmd_noop(ap)
  Agent *ap;
{
  agent_reply(ap, "250 OK");
}


static void
agent_reset(ap)
  Agent *ap;
{
  MYDOG;

#ifdef HELO_CHECK
  ap->mode &= AM_HELO;
#else
  ap->mode = 0;
#endif

  ap->memo[0] = '\0';
  ap->fpath[0] = '\0';
  ap->from[0] = '\0';
  ap->title[0] = '\0';
  ap->addr[0] = '\0';
  ap->nick[0] = '\0';
  MYDOG;
  agent_free_rcpt(ap);
  MYDOG;
}


static void
cmd_rset(ap)
  Agent *ap;
{
  agent_reset(ap);
  agent_reply(ap, "250 Reset state");
}


/* ----------------------------------------------------- */
/* 0 : OK , -1 : error, -2 : <>, 1 : relayed		 */
/* ----------------------------------------------------- */


static int
parse_addr(addr, user, domain)
  char *addr, **user, **domain;
{
  int ch, relay;
  char *ptr, *str;

  /* <[@domain_list:]user@doamin> */

  addr = strchr(addr, '<');
  if (!addr)
    return -1;

  ptr = strrchr(++addr, '>');
  if (!ptr)
    return -1;

  if (ptr == addr)
    return -2;			/* <> null domain */

  *ptr = '\0';
  if (ptr = strrchr(addr, ':'))
  {
    relay = 1;
    addr = ptr + 1;
  }
  else
  {
    relay = 0;
  }

  /* check the E-mail address format */

  *user = str = addr;

  /* Thor.000607: dequote */
  if (**user == '"')
    (*user)++;

  for (ptr = NULL; ch = *addr; addr++)
  {
    if (ch == '@')
    {
      if (ptr)
	return -1;

      ptr = addr;
      continue;
    }

    /* Thor.000607: dequote */
    if (ch == '"')
    {
      *addr = 0;
      continue;
    }

    if (ch <= 32 || ch >= 127)
      return -1;
    if (strchr("<>()[]\\,;:", ch))
      return -1;
  }

  if (!ptr)
    return -1;

  *ptr++ = '\0';
  *domain = ptr;

  if (!*user)			/* more ... */
  {
    return -1;
  }

  return relay;
}


static void
cmd_mail(ap)
  Agent *ap;
{
  char *data, *from, *user, *domain, *ptr;
  int cc;

#ifdef HELO_CHECK
  if (!(ap->mode & AM_HELO))
  {
    ap->xerro++;
    agent_reply(ap, "503 Polite people say HELO first");
    return;
  }
#endif

  from = ap->from;
  if (*from)
  {
    ap->xerro++;
    agent_reply(ap, "503 Sender already specified");
    return;
  }

  /* mail from:<[@domain_list:]user@doamin> */

  data = ap->data;
  MYDOG;
  cc = parse_addr(data, &user, &domain);
  MYDOG;

  if (cc)
  {
    if (cc == -2)		/* null domain */
    {
      from[0] = ' ';
      from[1] = '\0';

      strcpy(data, "250 Sender ok\r\n");
      ap->used = strlen(data);
      ap->state = CS_SEND;
      return;
    }

    ap->xerro++;
    agent_reply(ap, cc < 0 ? "501 Syntax error" :
      "551 we dont accept relayed mail");
    return;
  }

  /* Thor.990811: 抓假造的domain */
  MYDOG;
  if (is_forge(domain))
  {
    ap->xspam++;
    ap->mode |= AM_SPAM;
    agent_log(ap, "FORGE", domain);
    agent_reply(ap, "501 Sender host must exist");
    return;
  }

  /* Thor.990817: 如果host假造，連加都不想加入mfrom_ht */
  MYDOG;
  ptr = data + MAX_CMD_LEN;
  sprintf(ptr, "%s@%s", user, domain);
  ht_add(mfrom_ht, ptr);

  MYDOG;
  /* 檢查 MAIL FROM: 是否在黑白名單上 */
  if (acl_spam(user, domain))
  {
    ap->xspam++;
    ap->mode |= AM_SPAM;
    agent_log(ap, "SPAM-M", ptr);
    agent_reply(ap, "550 you are not in my access list");
    return;
  }

  str_ncpy(from, ptr, sizeof(ap->from));
  sprintf(data, "250 <%s> Sender ok\r\n", ptr);
  ap->used = strlen(data);
  ap->state = CS_SEND;
  MYDOG;
}


static int		/* AM_VALID:認證信  AM_BBSADM:bbsadm  0:一般信  -1:拒收 */
is_rcpt(rcpt, letter)
  char *rcpt;
  int *letter;		/* 回傳 1:寄給 *.bbs@  0:寄給 *.brd@ */
{
  int len;
  char *str, fpath[64];
  char *alias[] = ADM_ALIASES;

#ifdef EMAIL_JUSTIFY
  if (!str_cmp(rcpt, "bbsreg"))
    return AM_VALID;
#endif

  /* check the aliases */

  for (len = 0; str = alias[len]; len++)
  {
    if (!str_cmp(rcpt, str))
      return AM_BBSADM;
  }

  /* check the users */

  len = strlen(rcpt);
  if (len > 4)		/* ".bbs" 或 ".brd" => 4 */
  {
    str = rcpt + len - 4;

    if (!str_cmp(str, ".bbs"))
    {
      if (len <= IDLEN + 4)
      {
	*str = '\0';
	str_lower(rcpt, rcpt);
	sprintf(fpath, "usr/%c/%s/@", *rcpt, rcpt);
	if (dashd(fpath))
	{
	  *letter = 1;
	  return 0;
	}
      }
    }
    else if (!str_cmp(str, ".brd"))
    {
      if (len <= BNLEN + 4)
      {
	*str = '\0';
	if (getbrdname(rcpt))
	{
	  *letter = 0;
	  return 0;
	}
      }
    }
  }

  return -1;
}


static void
cmd_rcpt(ap)
  Agent *ap;
{
  char *data, *user, *domain;
  int cc, letter;
  RCPT *rcpt;

  if (!ap->from[0])
  {
    ap->xerro++;
    agent_reply(ap, "503 MAIL first");
    return;
  }

  if (ap->nrcpt > MAX_RCPT)
  {
    /* maybe spammer */

    /* Thor.000131: 這樣就不用一直去刪unmail.acl重覆的entry :) */
    if (!(ap->mode & AM_SPAM))
      agent_spam(ap);		/* 同一信寄給太多收信者直接擋掉 */

    ap->mode |= AM_SPAM;
    ap->xspam += ap->nrcpt;
    agent_log(ap, "SPAM-R", "too many recipients");
    agent_reply(ap, "552 Too many recipients");
    return;
  }

  /* rcpt to:<[@domain_list:]user@doamin> */

  data = ap->data;
  cc = parse_addr(data, &user, &domain);
  if (cc)
  {
    ap->xerro++;
    agent_reply(ap, cc < 0 ? "501 Syntax error" : "551 we dont accept relayed mail");

    return;
  }

  if (domain == NULL)
  {
    agent_reply(ap, "550 null domain");
    return;
  }

  /* if (str_cmp(domain, MYHOSTNAME)) */
  if (!is_host_alias(domain))	/* HOST_ALIAS 裡的都可以 */
  {
    ap->xerro++;
    agent_reply(ap, "550 we dont relay mail");
    return;
  }

  ht_add(mrcpt_ht, user);

  cc = is_rcpt(user, &letter);

  if (cc < 0)		/* 無此使用者或看板 */
  {
    ap->xerro++;
    agent_reply(ap, "550 no such user or board");
    return;
  }

  if (cc)		/* AM_VALID、AM_BBSADM */
  {
    ap->mode |= cc;
  }
  else			/* Thor.991130.註解: 一般 *.bbs@ 或 *.brd@ 情況 */
  {
#if 1	/* Thor.981227: 確定不是 bbsreg 才擋，delay 擋連線 */
    /* format: sprintf(servo_ident, "[%d] %s ip:%08x", ++servo_sno, rhost, csin.sin_addr.s_addr); */
    char rhost[256], *s;

    if (s = strchr(ap->ident, ' '))
    {
      strcpy(rhost, s + 1);
      if (s = strchr(rhost, ' '))
      {
	*s = '\0';

	/* 檢查 發信的機器 是否在黑白名單上 */
	if (acl_spam("*", rhost))
	{
	  ap->mode |= AM_SPAM;
	  agent_log(ap, "SPAM-M", rhost);
	  agent_reply(ap, "550 deny connection");
	  return;
	}
      }
    }
#endif

#if 1	/* Thor.981223: 確定不是 bbsreg 就擋 */
    if (ap->mode & AM_SPAM)
    {
      agent_reply(ap, "550 spam mail");
      return;
    }
#endif

    ap->letter = letter;
    ap->nrcpt++;
    cc = strlen(user) + 1;
    MYDOG;
    rcpt = (RCPT *) malloc(sizeof(RCPT) + cc);
    MYDOG;
    if (!rcpt)			/* Thor.990205: 記錄空間不夠 */
      logit("ERROR", "Not enough space in cmd_rcpt()");

    rcpt->rnext = ap->rcpt;
    memcpy(rcpt->userid, user, cc);
    ap->rcpt = rcpt;
  }

  agent_reply(ap, "250 Recipient ok");
}


static void
cmd_helo(ap)
  Agent *ap;
{
  char *data;

#ifdef SMTP_CONN_CHECK
  /* Thor.990806: check if the peer is a normal smtp server, otherwise possibly a dialup spammer */

  struct sockaddr_in sin;
  int sock = sizeof(sin);

  if (getpeername(ap->sock, (struct sockaddr *) & sin, &sock) >= 0)
  {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0)
    {
      int t;

      sin.sin_port = htons(BMTA_PORT);
      t = connect(sock, (struct sockaddr *) & sin, sizeof sin);
      close(sock);

      if (t < 0)
      {
	data = ap->data;
	strcpy(data, "550 reject non-smtp server\r\n");
	ap->used = strlen(data);
	ap->state = CS_FLUSH;
	return;
      }
    }
  }
#endif

  data = ap->data;
  sprintf(data, "250 Hello %s\r\n", ap->ident);
  ap->used = strlen(data);
  ap->state = CS_SEND;

#ifdef HELO_CHECK
  ap->mode |= AM_HELO;
#endif
}


static void
cmd_data(ap)
  Agent *ap;
{
  int mode;

  mode = ap->mode;
  if (!ap->rcpt && !(mode & (AM_VALID | AM_BBSADM)))
  {
    ap->xerro++;
    agent_reply(ap, "503 RCPT first");
    return;
  }

  ap->mode = mode | AM_DATA;
  agent_reply(ap, "354 go ahead");
}


static void
cmd_quit(ap)
  Agent *ap;
{
  char *data;

  data = ap->data;
  strcpy(data, "221 bye\r\n");
  ap->used = strlen(data);
  ap->state = CS_FLUSH;
}


static void
cmd_nogo(ap)
  Agent *ap;
{
  agent_reply(ap, "502 operation not allowed");
  return;
}


static Command cmd_table[] =
{
  cmd_helo, "helo", "HELO <hostname> - Introduce yourself",
  cmd_nogo, "ehlo", "",		/* Thor.980929: 不支援enhanced smtp */
  cmd_mail, "mail", "MAIL FROM: <sender> - Specifies the sender",
  cmd_rcpt, "rcpt", "RCPT TO: <recipient> - Specifies the recipient",
  cmd_data, "data", "",
  cmd_quit, "quit", "QUIT -     Exit sendmail (SMTP)",
  cmd_noop, "noop", "NOOP -     Do nothing",
  cmd_rset, "rset", "RSET -     Resets the system",
  cmd_help, "help", "HELP [ <topic> ] - gives help info",
  cmd_nogo, "expn", "",
  cmd_nogo, "vrfy", "",
  cmd_what, NULL, NULL
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

#ifdef AM_DEBUG
  if (ap->mode & AM_DEBUG)
  {
    char buf[1024];

    sprintf(buf, "%s\t[%d]\tbmtad>>>\n", Now(), ap->sno);
    f_cat(BMTA_DEBUGFILE, buf);
    str_ncpy(buf, data, len + 1);
    f_cat(BMTA_DEBUGFILE, buf);
    sprintf(buf, "\t[%d]\tbmtad<<<\n", ap->sno);
    f_cat(BMTA_DEBUGFILE, buf);
  }
#endif

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
  int cc, mode, size, used;
  char *data, *head, *dest;

  mode = ap->mode;
  used = ap->used;
  data = ap->data;

  if (mode & AM_DATA)
  {
    if (used <= 0)
    {
      /* pre-set data */

      *data = '\n';
      used = 1;
    }
    else
    {
      /* check the available space */

      size = ap->size;
      cc = size - used;

      if (cc < TCP_RCVSIZ + 3)
      {
	if (size < MAX_DATA_SIZE)
	{
	  size += TCP_RCVSIZ + (size >> 2);

	  if (data = (char *) realloc(data, size))
	  {
	    ap->data = data;
	    ap->size = size;
	  }
	  else
	  {
	    fprintf(flog, "ERROR\t[%d] malloc: %d\n", ap->sno, size);

#ifdef DEBUG
	    fflush(flog);
#endif

	    return 0;
	  }
	}
	else
	{
	  /* DATA 太長了，通通吃下來，並假設 mail header 不會超過 HEADER_SIZE */

#define HEADER_SIZE   8192
	  data[HEADER_SIZE - 2] = data[used - 2];
	  data[HEADER_SIZE - 1] = data[used - 1];
	  used = HEADER_SIZE;
#undef  HEADER_SIZE
	  ap->mode = (mode |= AM_SWALLOW);
	  ap->xerro++;
	  fprintf(flog, "ERROR\t[%d] data too long\n", ap->sno);

#ifdef DEBUG
	  fflush(flog);
#endif
	}
      }
    }
  }

  head = data + used;
  MYDOG;
  cc = recv(ap->sock, head, TCP_RCVSIZ, 0);
  MYDOG;

  if (cc <= 0)
  {
    cc = errno;
    MYDOG;
    if (cc != EWOULDBLOCK)
    {
      agent_log(ap, "RECV", strerror(cc));
      return 0;
    }

    /* would block, so leave it to do later */

    return -1;
  }
  MYDOG;

  head[cc] = '\0';
  ap->xsize += cc;

#ifdef AM_DEBUG
  if (mode & AM_DEBUG)
  {
    char buf[80];
    sprintf(buf, "%s\t[%d]\tpeer>>>\n", Now(), ap->sno);
    f_cat(BMTA_DEBUGFILE, buf);
    f_cat(BMTA_DEBUGFILE, head);
    sprintf(buf, "\t[%d]\tpeer<<<\n", ap->sno);
    f_cat(BMTA_DEBUGFILE, buf);
  }
#endif

  /* --------------------------------------------------- */
  /* DATA mode						 */
  /* --------------------------------------------------- */

  if (mode & AM_DATA)
  {
    dest = head - 1;

    for (;;)
    {
      cc = *head;

      if (!cc)
      {
	ap->used = dest - data + 1;
	return 1;
      }

      head++;

      if (cc == '\r')
	continue;

      if (cc == '\n')
      {
	/* Thor.990604: \n後馬上check, 以立刻排除 \r\n.\r\n 的狀況 */
	used = dest - data + 1;
	if (used >= 2 && *dest == '.' && dest[-1] == '\n')
	  break;		/* end of mail body */

	for (;;)
	{
	  used = *dest;

	  if (used == ' ' || used == '\t')
	  {
	    dest--;		/* strip the trailing space */
	    continue;
	  }
	  break;
	}

	/* Thor.990604: strip leading ".." to "." */
	{
	  char *first = dest;

	  while (first >= data && *first != '\n')
	    first--;
	  first++;

	  if (first <= dest && *first == '.')
	  {
	    while (first < dest)
	    {
	      *first = first[1];
	      first++;
	    }
	    dest--;
	  }
	}
      }

      *++dest = cc;
    }
    MYDOG;

#if 1		/* Thor.990906: null from 就接地:p */
    cc = *ap->from;
    if (cc == ' ' || cc == '\0')
    {
      agent_reply(ap, "250 Message dropped");
      agent_log(ap, "SPAM-NULL", ap->ident);
      MYDOG;
      return -1;
    }
#endif

    if (mode & AM_SWALLOW)
    {
      agent_reset(ap);
      agent_reply(ap, "552 Too much mail data");
      MYDOG;
      return -1;
    }

    /* strip the trailing empty lines */

    dest -= 2;			/* 3; */

    while (*dest == '\n' && dest > data)
      dest--;
    dest += 2;
    *dest = '\0';

    ap->used = dest - data;

    MYDOG;
    /* Thor.981223: 確定不是 bbsreg 才擋 */
    if (!(mode & (AM_VALID | AM_BBSADM)) && (mode & AM_SPAM))
    {
      sprintf(ap->memo, "SPAM : %s", ap->from);
      mta_memo(ap, 0);

      agent_reply(ap, "250 Message dropped");
      agent_log(ap, "SPAM", "mail");
    }
    else
    {
      MYDOG;
      cc = mta_mailer(ap);
      MYDOG;
      if (!cc)
      {
	agent_reply(ap, "250 Message accepted");
	MYDOG;
      }
      else if (cc < 0)
      {
	ap->xspam++;
	MYDOG;
	agent_log(ap, "SPAM", "mail");
	MYDOG;
      }
    }

    MYDOG;
    agent_reset(ap);
    MYDOG;
    return 1;
  }

  /* --------------------------------------------------- */
  /* command mode					 */
  /* --------------------------------------------------- */

  used += cc;

  if (used >= MAX_CMD_LEN)
  {
    fprintf(flog, "CMD\t[%d] too long (%d) %.32s\n",
      ap->sno, used, data);

#ifdef DEBUG
    fflush(flog);
#endif

    ap->mode = (mode |= AM_DROP);
    ap->xerro += 10;		/* are you hacker ? */
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
	agent_reset(ap);
	agent_reply(ap, "552 command too long");
	MYDOG;
	return -1;
      }

      for (cmd = cmd_table; head = cmd->cmd; cmd++)
      {
	if (!str_ncmp(data, head, 4))
	  break;
      }

      MYDOG;
      ap->used = 0;

      sprintf(gtext, "ip:%08lx ", ap->ip_addr);
      str_ncpy(gtext + 3 + 8 + 1, data, 50);

      (*cmd->func) (ap);

      *gtext = 0;
      MYDOG;
      return 1;
    }

    if (cc == '\t')
      *head = ' ';

    head++;
  }

  MYDOG;
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
  char *data, *key, xerro[32], xspam[32];

  num = ap->sock;
  if (num > 0)
  {
    MYDOG;
    fcntl(num, F_SETFL, M_NONBLOCK);
    MYDOG;

#define MSG_ABORT   "\r\n450 buggy, closing ...\r\n"
    send(num, MSG_ABORT, sizeof(MSG_ABORT) - 1, 0);
#undef  MSG_ABORT
    MYDOG;
    shutdown(num, 2);
    MYDOG;
    close(num);
    MYDOG;

    key = "END";
  }
  else
  {
    key = "BYE";
  }

  MYDOG;
  agent_free_rcpt(ap);
  MYDOG;

  /* log */

  data = ap->data;

  *xerro = *xspam = '\0';
  if ((num = ap->xerro) > 0)
    sprintf(xerro, " X%d", num);
  if ((num = ap->xspam) > 0)
    sprintf(xspam, " Z%d", num);

  sprintf(data, "[%d] T%d R%d D%d S%d%s%s", ap->sno, time(0) - ap->tbegin,
    ap->xrcpt, ap->xdata, ap->xsize, xerro, xspam);
  logit(key, data);

  MYDOG;
  free(data);
  MYDOG;
}


/* ----------------------------------------------------- */
/* accept a new connection				 */
/* ----------------------------------------------------- */


static char servo_ident[128];


static int
agent_accept()
{
  int csock;
  int value;
  struct sockaddr_in csin;
  char rhost[160], *ident;

  for (;;)
  {
    value = sizeof(csin);
    MYDOG;
    csock = accept(0, (struct sockaddr *) & csin, &value);
    MYDOG;
    /* if (csock > 0) */
    if (csock >= 0)		/* Thor.000126: more proper */
      break;

    csock = errno;
    if (csock != EINTR)
    {
      logit("ACCEPT", strerror(csock));
      return -1;
    }

    MYDOG;
    while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
    MYDOG;
  }
  MYDOG;

  value = 1;

  /* Thor.000511: 註解: don't delay send to coalesce(聯合) packets */
  setsockopt(csock, IPPROTO_TCP, TCP_NODELAY, (char *) &value, sizeof(value));

  /* --------------------------------------------------- */
  /* check remote host / user name			 */
  /* --------------------------------------------------- */

  MYDOG;

  /* Thor.001026: 追蹤卡在哪  */
  sprintf(gtext, "ip:%08x", csin.sin_addr.s_addr);

  hht_look((HostAddr *) &csin.sin_addr, rhost);

  *gtext = 0;
  MYDOG;
  /* Thor.981207: 追蹤 ip address */
  sprintf(ident = servo_ident, "[%d] %s ip:%08x", ++servo_sno, rhost, csin.sin_addr.s_addr);

  MYDOG;
  ht_add(mhost_ht, rhost);
  logit("CONN", ident);
  MYDOG;
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
    " involuntary context switches: %ld\n"
    " gline: %d\ngtext: %s\n",

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
    ru.ru_nivcsw, gline, gtext);

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

  sprintf(buf, "abort: %d, errno: %d, gline: %d", sig, errno, gline);
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
  sin.sin_port = htons(BMTA_PORT);
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  memset((char *) &sin.sin_zero, 0, sizeof(sin.sin_zero));

  if (bind(fd, (struct sockaddr *) & sin, sizeof(sin)) ||
    listen(fd, TCP_BACKLOG))
    exit(1);
}


/* Thor.990204: 註解: 每天早上 5:30 整理一次 */
#define SERVO_HOUR  5
#define SERVO_MIN   30


static time_t
fresh_time(uptime)
  time_t uptime;
{
  struct tm *local;
  int i;

  local = localtime(&uptime);
  i = (SERVO_HOUR - local->tm_hour) * 3600 + (SERVO_MIN - local->tm_min) * 60;
  if (i < 120)			/* 保留時間差 120 秒 */
    i += BMTA_FRESH;
  return (uptime + i);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int n, sock, state;
  time_t uptime, tcheck, tfresh, tscore;
  Agent **FBI, *Scully, *Mulder, *agent;
  fd_set rset, wset, xset;
  static struct timeval tv = {BMTA_PERIOD, 0};

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
  servo_signal();

  mail_root = acl_load(MAIL_ACLFILE, mail_root);
  unmail_root = acl_load(UNMAIL_ACLFILE, unmail_root);

  log_open();
  init_bshm();
  init_ushm();
  dns_init();

  mrcpt_ht = ht_new(128, 0);
  mhost_ht = ht_new(256, 0);
  mfrom_ht = ht_new(256, 0);
  title_ht = ht_new(512, 0);

#ifdef FORGE_CHECK
  forge_ht = ht_new(256, 0);
#endif

  uptime = time(0);
  tcheck = uptime + BMTA_PERIOD;
  tfresh = fresh_time(uptime);
  tscore = uptime + 2 * 60 * 60;

  Scully = Mulder = NULL;

  for (;;)
  {
    /* maintain : resource and garbage collection */

    uptime = time(0);
    if (tcheck < uptime)
    {
      /* ----------------------------------------------- */
      /* agent_audit (uptime - BMTA_TIMEOUT)		 */
      /* ----------------------------------------------- */

      tcheck = uptime - BMTA_TIMEOUT;

      for (FBI = &Scully; agent = *FBI;)
      {
	if ((agent->uptime < tcheck) || (agent->xerro > BMTA_FAULT))
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
      /* expire SPAM HashTable				 */
      /* ----------------------------------------------- */

      if (uptime > tscore)
      {
	tscore = uptime + 120 * 60;

	/* 超過 120 分鐘沒新進記錄，就開始 expire */

	tcheck = uptime - 120 * 60;
	ht_expire(mfrom_ht, tcheck);
	ht_expire(mhost_ht, tcheck);
	ht_expire(mrcpt_ht, tcheck);
	ht_expire(title_ht, tcheck);

	/* --------------------------------------------- */
	/* expire DNS HostHashTable cache		 */
	/* --------------------------------------------- */

	hht_expire(uptime - 3 * 60 * 60);

	/* ht_expire(forge_ht, tcheck); /* never expire */

#ifdef FORGE_CHECK
	/* Thor.990811: expire掉沒用的forge host */
	ht_expire(forge_ht, tcheck);
#endif
      }

      /* ----------------------------------------------- */
      /* maintain SPAM & server log			 */
      /* ----------------------------------------------- */

      if (tfresh < uptime)
      {
	tfresh = uptime + BMTA_FRESH;
	visit_fresh();
#ifdef SERVER_USAGE
	servo_usage();
#endif
	log_fresh();
      }
      else
      {
	fflush(flog);
      }

      tcheck = uptime + BMTA_PERIOD;
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

        mail_root = acl_load(MAIL_ACLFILE, mail_root);
	unmail_root = acl_load(UNMAIL_ACLFILE, unmail_root);
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

    MYDOG;
    for (FBI = &Scully; agent = *FBI;)
    {
      sock = agent->sock;

      if (FD_ISSET(sock, &wset))
      {
	MYDOG;
	state = agent_send(agent);
	MYDOG;
      }
      else if (FD_ISSET(sock, &rset))
      {
	MYDOG;
	state = agent_recv(agent);
	MYDOG;
      }
      else if (FD_ISSET(sock, &xset))
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
	MYDOG;
	agent_fire(agent);
	MYDOG;

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
    MYDOG;

    /* ------------------------------------------------- */
    /* serve new connection				 */
    /* ------------------------------------------------- */

    /* Thor.000209: 考慮移前此部分, 免得卡在 accept() */
    if (FD_ISSET(0, &rset))
    {
      /* Thor.990319: check maximum connection number */
      u_long ip_addr;
      MYDOG;
      sock = agent_accept();
      MYDOG;

      if (sock > 0)
      {
#if 1
	/* Thor.990319: check maximum connection number */
	int num = 0;

	sscanf(strstr(servo_ident, "ip:") + 3, "%x", &ip_addr);
	for (agent = Scully; agent; agent = agent->anext)
	{
	  if (agent->ip_addr == ip_addr)
	    num++;
	}
	if (num >= MAX_HOST_CONN)
	{
	  char buf[256];

	  sprintf(buf, "421 %s over max connection\r\n", MYHOSTNAME);

	  MYDOG;
	  fcntl(sock, F_SETFL, M_NONBLOCK);	/* Thor.000511: 怕 block */
	  MYDOG;
	  send(sock, buf, strlen(buf), 0);	/* Thor.981206: 補個0上來 */
	  MYDOG;
	  shutdown(sock, 2);
	  MYDOG;
	  close(sock);
	  MYDOG;
	  logit("OVER", servo_ident);
	  MYDOG;
	  continue;
	}
#endif

	if (agent = Mulder)
	{
	  Mulder = agent->anext;
	}
	else
	{
	  MYDOG;
	  agent = (Agent *) malloc(sizeof(Agent));
	  MYDOG;
	  if (!agent)		/* Thor.990205: 記錄空間不夠 */
	    logit("ERROR", "Not enough space in main()");
	}

	*FBI = agent;

	/* variable initialization */

	memset(agent, 0, sizeof(Agent));

	agent->sock = sock;
	agent->sno = servo_sno;
	agent->state = CS_SEND;
	agent->tbegin = agent->uptime = uptime;
	strcpy(agent->ident, servo_ident);

	/* Thor.990319: check maximum connection number */
	agent->ip_addr = ip_addr;

#ifdef AM_DEBUG
	if (strstr(agent->ident, "8c7a4133"))
	{			/* Thor.990221: that's mail.cc.ntnu.edu.tw */
	  agent->mode |= AM_DEBUG;
	}
#endif

	MYDOG;
	agent->data = (char *) malloc(MIN_DATA_SIZE);
	MYDOG;
	if (!agent->data)	/* Thor.990205: 記錄空間不夠 */
	  logit("ERROR", "Not enough space in agent->data");
	sprintf(agent->data, "220 " MYHOSTNAME " SMTP ready %s\r\n", servo_ident);	/* Thor.981001: 不用 enhanced SMTP */
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
