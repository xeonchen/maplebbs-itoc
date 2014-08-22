/*-------------------------------------------------------*/
/* util/reaper.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : 使用者帳號定期清理				 */
/* create : 95/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/
/* syntax : reaper					 */
/*-------------------------------------------------------*/
/* notice : ~bbs/usr/@/     - expired users's data	 */
/*          run/reaper      - list of expired users	 */
/*          run/manager     - list of managers		 */
/*          run/lazybm      - list of lazy bm		 */
/*          run/emailaddr   - list of same email addr    */
/*-------------------------------------------------------*/


#include "bbs.h"


/* Thor.980930: 若要關閉以下功能時用 undef 即可 */

#define CHECK_LAZYBM		/* 檢查偷懶板主 */
#define EADDR_GROUPING		/* 檢查共用 email */


#ifdef CHECK_LAZYBM
#define DAY_LAZYBM	7	/* 7 天以上未上站的板主即記錄 */
#endif

#ifdef EADDR_GROUPING
#define EMAIL_REG_LIMIT 3	/* 3 個以上使用者用同一 email 即記錄 */
#endif


static time_t due_newusr;
static time_t due_forfun;
static time_t due_occupy;


static int visit = 0;	/* 總 ID 的數目 */
static int prune = 0;	/* 被清除ID的數目 */
static int manager = 0;	/* 管理者的數目 */
static int invalid = 0;	/* 未認證通過ID的數目 */


static FILE *flog;
static FILE *flst;

#ifdef CHECK_LAZYBM
static time_t due_lazybm;
static int lazybm = 0;	/* 偷懶板主的數目 */
static FILE *fbm;
#endif

#ifdef EADDR_GROUPING
static FILE *faddr;
#endif


/* ----------------------------------------------------- */
/* 清空 .USR 裡面的欄位					 */
/* ----------------------------------------------------- */


static int funo;
static int max_uno;


static void
userno_free(uno)
  int uno;
{
  off_t off;
  int fd;
  static SCHEMA schema;	/* itoc.031216.註解: 用 static 是因為 schema.userid 要一直被清成全零 */

  fd = funo;

  /* flock(fd, LOCK_EX); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_exlock(fd);

  time(&schema.uptime);
  off = (uno - 1) * sizeof(SCHEMA);
  if (lseek(fd, off, SEEK_SET) < 0)
    exit(2);
  if (write(fd, &schema, sizeof(SCHEMA)) != sizeof(SCHEMA))
    exit(2);

  /* flock(fd, LOCK_UN); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_unlock(fd);
}


/* ----------------------------------------------------- */
/* 顯示用函式						 */
/* ----------------------------------------------------- */


static void
levelmsg(str, level)
  char *str;
  int level;
{
  static char perm[] = STR_PERM;
  int len = 32;
  char *p = perm;

  do
  {
    *str = (level & 1) ? *p : '-';
    p++;
    str++;
    level >>= 1;
  } while (--len);
  *str = '\0';
}


static void
datemsg(str, chrono)
  char *str;
  time_t *chrono;
{
  struct tm *t;

  t = localtime(chrono);
  /* Thor.990329: y2k */
  sprintf(str, "%02d/%02d/%02d%3d:%02d:%02d ",
    t->tm_year % 100, t->tm_mon + 1, t->tm_mday,
    t->tm_hour, t->tm_min, t->tm_sec);
}


/*-------------------------------------------------------*/
/* 看板權限部分須與 board.c 相容			 */
/*-------------------------------------------------------*/


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


static int num_bm = 0;
static char all_bm[MAXBOARD * BMLEN / 3][IDLEN + 1];	/* 每板至多有 (BMLEN / 3) 個板主 */


static int
bm_cmp(a, b)		/* 其實就是 str_cmp */
  char *a, *b;
{
  return str_cmp(a, b);
}


static char *		/* 1: 在全站板主名單內 */
check_allBM(userid)
  char *userid;		/* lower case userid */
{
  return bsearch(userid, all_bm, num_bm, IDLEN + 1, bm_cmp);
}


static void
collect_allBM()
{
  BRD *head, *tail;
  char *ptr, *str, buf[BMLEN + 1];

  /* 去 bshm 中抓出所有 brd->BM */

  head = bshm->bcache;
  tail = head + bshm->number;
  do				/* 至少有 note 一板，不必對看板做檢查 */
  {
    ptr = buf;
    strcpy(ptr, head->BM);

    while (*ptr)	/* 把 brd->BM 中 bm1/bm2/bm3/... 各個 bm 抓出來 */
    {
      if (str = strchr(ptr, '/'))
	*str = '\0';
      if (!check_allBM(ptr))
      {
	strcpy(all_bm[num_bm], ptr);
	num_bm++;
	qsort(all_bm, num_bm, IDLEN + 1, bm_cmp);
      }
      if (!str)
	break;
      ptr = str + 1;
    }
  } while (++head < tail);
}


/* ----------------------------------------------------- */
/* 檢查共用 email					 */
/* ----------------------------------------------------- */

#ifdef EADDR_GROUPING
/*
   Thor.980930: 將同一email addr的account, 收集起來並列表 工作原理: 
   1. _hash() 將 email addr 數值化
   2.用binary search, 找到則append userno, 找不到則 insert 新entry
   3.將 userno list 整理列出

   資料結構: chain: int hash, int link_userno plist: int next_userno

   暫時預估email addr總數不超過100000, 
   暫時預估user總數不超過 100000
 */

typedef struct
{
  int hash;
  int link;
}      Chain;

static Chain *chain;
static int *plist;
static int numC;

static void
eaddr_group(userno, eaddr)
  int userno;
  char *eaddr;
{
  int left, right, mid, i;
  int hash = str_hash(eaddr, 0);

  left = 0;
  right = numC - 1;
  for (;;)
  {
    int cmp;
    Chain *cptr;

    if (left > right)		/* Thor.980930: 找沒 */
    {
      for (i = numC; i > left; i--)
	chain[i] = chain[i - 1];

      cptr = &chain[left];
      cptr->hash = hash;
      cptr->link = userno;
      plist[userno] = 0;	/* Thor: tail */
      numC++;
      break;
    }

    mid = (left + right) >> 1;
    cptr = &chain[mid];
    cmp = hash - cptr->hash;

    if (!cmp)
    {
      plist[userno] = cptr->link;
      cptr->link = userno;
      break;
    }

    if (cmp < 0)
      right = mid - 1;
    else
      left = mid + 1;
  }
}


static void
report_eaddr_group()
{
  int i, j, cnt;
  off_t off;
  int fd;
  SCHEMA s;

  fprintf(faddr, "Email registration over %d times list\n\n", EMAIL_REG_LIMIT);

  for (i = 0; i < numC; i++)
  {
    for (cnt = 0, j = chain[i].link; j; cnt++, j = plist[j])
      ;

    if (cnt > EMAIL_REG_LIMIT)
    {
      fprintf(faddr, "\n> %d\n", chain[i].hash);
      for (j = chain[i].link; j; j = plist[j])
      {
	off = (j - 1) * sizeof(SCHEMA);
	if (lseek(funo, off, SEEK_SET) < 0)
	{
	  fprintf(faddr, "==> %d) can't lseek\n", j);
	  continue;
	}
	if (read(funo, &s, sizeof(SCHEMA)) != sizeof(SCHEMA))
	{
	  fprintf(faddr, "==> %d) can't read\n", j);
	  continue;
	}
	else
	{
	  ACCT acct;
	  char buf[256];

	  if (s.userid[0] <= ' ')
	  {
	    fprintf(faddr, "==> %d) has been reapered\n", j);
	    continue;
	  }
	  usr_fpath(buf, s.userid, FN_ACCT);
	  fd = open(buf, O_RDONLY);
	  if (fd < 0)
	  {
	    fprintf(faddr, "==> %d)%-13s can't open\n", j, s.userid);
	    continue;
	  }
	  if (read(fd, &acct, sizeof(ACCT)) != sizeof(ACCT))
	  {
	    fprintf(faddr, "==> %d)%-13s can't read\n", j, s.userid);
	    continue;
	  }
	  close(fd);

	  datemsg(buf, &acct.lastlogin);
	  fprintf(faddr, "%5d) %-13s%s[%d]\t%s\n", acct.userno, acct.userid, buf, acct.numlogins, acct.email);
	}
      }
    }
  }
}
#endif


/* ----------------------------------------------------- */
/* 主程式						 */
/* ----------------------------------------------------- */


static void
reaper(fpath, lowid)
  char *fpath;
  char *lowid;
{
  int fd, login, userno;
  usint ulevel;
  time_t life;
  char buf[256], data[40];
  ACCT acct;

  sprintf(buf, "%s/" FN_ACCT, fpath);
  fd = open(buf, O_RDWR);
  if (fd < 0)
  {				/* Thor.981001: 加些 log */
    fprintf(flog, "acct can't open %-13s ==> %s\n", lowid, buf);
    return;
  }

  if (read(fd, &acct, sizeof(ACCT)) != sizeof(ACCT))
  {
    fprintf(flog, "acct can't read %-13s ==> %s\n", lowid, buf);
    close(fd);
    return;
  }

  ulevel = acct.userlevel;

  /* 有 PERM_BM 但不在全站板主名單內，移除其 PERM_BM */
  if ((ulevel & PERM_BM) && !check_allBM(lowid))
  {
    ulevel ^= PERM_BM;
    acct.userlevel = ulevel;
    lseek(fd, 0, SEEK_SET);
    write(fd, &acct, sizeof(ACCT));
  }

  close(fd);

  userno = acct.userno;

  if ((userno <= 0) || (userno > max_uno))
  {
    fprintf(flog, "%5d) %-13s ==> %s\n", userno, acct.userid, buf);
    return;
  }

  life = acct.lastlogin;	/* 必不為 0 */
  login = acct.numlogins;

#ifdef EADDR_GROUPING
  if (ulevel & PERM_VALID)	/* Thor.980930: 只看通過認證的 email, 全算 */
    eaddr_group(userno, acct.email);
#endif

  if (ulevel & (PERM_XEMPT | PERM_BM | PERM_ALLADMIN))	/* 有這些權限者不砍 */
  {
    datemsg(buf, &acct.lastlogin);
    levelmsg(data, ulevel);
    fprintf(flst, "%5d) %-13s%s[%s] %d\n", userno, acct.userid, buf, data, login);
    manager++;

#ifdef CHECK_LAZYBM
    if ((ulevel & PERM_BM) && life < due_lazybm)
    {
      fprintf(fbm, "%5d) %-13s%s %d\n", userno, acct.userid, buf, login);
      lazybm++;
    }
#endif
  }
  else if (ulevel)		/* guest.ulevel == 0, 永遠保留 */
  {
    if (ulevel & PERM_PURGE)	/* lkchu.990221: 「清除帳號」 */
    {
      life = 0;
    }
    else if (ulevel & PERM_DENYLOGIN)	/* itoc.010927: 有「禁止上站」的不砍，但若有「清除帳號」權限照砍 */
    {
      /* life = 1; */	/* 不需要，前面有了 */
    }
    else if (ulevel & PERM_VALID)
    {
      if (life < due_occupy)
	life = 0;
    }
    else
    {
      if (login <= 3 && life < due_newusr)
        life = 0;
      else if (life < due_forfun)
	life = 0;
      else
	invalid++;
    }

    if (!life)
    {
      /* 清除的帳號放在 usr/@ */
      sprintf(buf, "usr/@/%s", lowid);
      if (rename(fpath, buf) < 0)
        f_rm(fpath);

      userno_free(userno);
      datemsg(buf, &acct.lastlogin);
      fprintf(flog, "%5d) %-13s%s%d\n", userno, acct.userid, buf, login);
      prune++;
    }
  }

  visit++;
}


static void
traverse(fpath)
  char *fpath;
{
  DIR *dirp;
  struct dirent *de;
  char *fname, *str;

  /* visit the second hierarchy */

  if (!(dirp = opendir(fpath)))
  {
    fprintf(flog, "## unable to enter hierarchy [%s]\n", fpath);
    return;
  }

  for (str = fpath; *str; str++);
  *str++ = '/';

  while (de = readdir(dirp))
  {
    fname = de->d_name;
    if (fname[0] > ' ' && fname[0] != '.')
    {
      strcpy(str, fname);
      reaper(fpath, fname);
    }
  }
  closedir(dirp);
}


int
main()
{
  int ch;
  time_t start, end;
  struct tm *ptime;
  struct stat st;
  char *fname, fpath[256];

  setuid(BBSUID);
  setgid(BBSGID);
  chdir(BBSHOME);
  umask(077);

  flog = fopen(FN_RUN_REAPER, "w");
  if (flog == NULL)
    exit(1);

  flst = fopen(FN_RUN_MANAGER, "w");
  if (flst == NULL)
    exit(1);

#ifdef CHECK_LAZYBM
  fbm = fopen(FN_RUN_LAZYBM, "w");
  if (fbm == NULL)
    exit(1);
#endif

#ifdef EADDR_GROUPING
  faddr = fopen(FN_RUN_EMAILADDR, "w");
  if (faddr == NULL)
    exit(1);
#endif

#ifdef EADDR_GROUPING
  funo = open(FN_SCHEMA, O_RDWR | O_CREAT, 0600);	/* Thor.980930: for read name */
#else
  funo = open(FN_SCHEMA, O_WRONLY | O_CREAT, 0600);
#endif

  if (funo < 0)
    exit(1);

  /* 假設清除帳號期間，新註冊人數不會超過 300 人 */

  fstat(funo, &st);
  max_uno = st.st_size / sizeof(SCHEMA) + 300;

  init_bshm();
  collect_allBM();

  time(&start);
  ptime = localtime(&start);

  /* itoc.011002.註解: 不能在一開學就馬上 apply 嚴格的時間限制，
     否則很多 user 會因為整個暑假沒有上站，在一開學就被 reaper 掉 */   

  if ((ptime->tm_mon >= 6 && ptime->tm_mon <= 8) ||	/* 7 月到 9 月是暑假 */
    (ptime->tm_mon >= 1 && ptime->tm_mon <= 2))		/* 2 月到 3 月是寒假 */
  {
    due_newusr = start - REAPER_VAC_NEWUSR * 86400;
    due_forfun = start - REAPER_VAC_FORFUN * 86400;
    due_occupy = start - REAPER_VAC_OCCUPY * 86400;
  }
  else
  {
    due_newusr = start - REAPER_DAY_NEWUSR * 86400;
    due_forfun = start - REAPER_DAY_FORFUN * 86400;
    due_occupy = start - REAPER_DAY_OCCUPY * 86400;
  }

#ifdef CHECK_LAZYBM
  due_lazybm = start - DAY_LAZYBM * 86400;
#endif

#ifdef EADDR_GROUPING
  chain = (Chain *) malloc(max_uno * sizeof(Chain));
  plist = (int *)malloc((max_uno + 1) * sizeof(int));
  if (!chain || !plist)
  {
    fprintf(faddr, "out of memory....\n");
    exit(1);
  }
#endif

  strcpy(fname = fpath, "usr/@");
  mkdir(fname, 0700);
  fname = (char *)strchr(fname, '@');

  /* visit the first hierarchy */

  for (ch = 'a'; ch <= 'z'; ch++)
  {
    fname[0] = ch;
    fname[1] = '\0';
    traverse(fpath);
  }

#ifdef EADDR_GROUPING
  report_eaddr_group();		/* Thor.980930: before close funo */
#endif

  close(funo);

  fprintf(flst, "\nManager: %d\n", manager);
  fclose(flst);

  time(&end);
  fprintf(flog, "# 開始時間：%s\n", Btime(&start));
  fprintf(flog, "# 結束時間：%s\n", Btime(&end));
  end -= start;
  start = end % 60;
  end /= 60;
  fprintf(flog, "# 總計耗時：%d:%d:%d\n", end / 60, end % 60, start);
  fprintf(flog, "# 註冊人數：%d\n", visit);	/* 未清除前的總數 */
  fprintf(flog, "# 清除人數：%d\n", prune);
  fprintf(flog, "# 未認證數：%d\n", invalid);
  fclose(flog);

#ifdef CHECK_LAZYBM
  fprintf(fbm, "\nLazy BM for %d days: %d\n", DAY_LAZYBM, lazybm);
  fclose(fbm);
#endif

#ifdef EADDR_GROUPING
  free(chain);
  free(plist);
  fclose(faddr);
#endif

  exit(0);
}
