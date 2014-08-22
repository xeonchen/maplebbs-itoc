/*-------------------------------------------------------*/
/* util/expire.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : 自動砍過期文章工具				 */
/* create : 95/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/
/* syntax : expire [day max min board]		 	 */
/* notice : 加上 board 時，可 expire+sync 某一 board	 */
/*-------------------------------------------------------*/


#include "bbs.h"


#if 0	/* itoc.030325: 簡易說明 */

  expire.c 包含二種動作：expire 和 sync

  1) 所謂「過期」，就是 文章日期過久/看板篇數過多

  2) 所謂「expire」，就是在 .DIR 索引中找出過期的文章，把這 hdr 從索引中移除，
     並將該檔案刪除

  3) 所謂「sync」，為了避免硬碟有多餘的垃圾，系統每 32 天會對看板內的檔案一一
     檢視，若其不在 .DIR 中，表示這個檔案已經從索引中遺失了，此時系統會直接將
     這檔案刪除

#endif


#define	EXPIRE_LOG	"run/expire.log"


typedef struct
{
  char bname[BNLEN + 1];	/* board ID */
  int days;			/* expired days */
  int maxp;			/* max post */
  int minp;			/* min post */
}      Life;


static int
life_cmp(a, b)
  Life *a, *b;
{
  return str_cmp(a->bname, b->bname);
}


/* ----------------------------------------------------- */
/* board：shm 部份須與 cache.c 相容                      */
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


/* ----------------------------------------------------- */
/* synchronize folder & files				 */
/* ----------------------------------------------------- */


static time_t synctime;


typedef struct
{
  time_t chrono;
  char prefix;		/* 檔案的 family */
  char exotic;		/* 1:不在.DIR中  0:在.DIR中 */
} SyncData;


static SyncData *sync_pool;
static int sync_size, sync_head;


#define	SYNC_DB_SIZE	2048


static int
sync_cmp(a, b)
  SyncData *a, *b;
{
  return a->chrono - b->chrono;
}


static void
sync_init(fname)
  char *fname;
{
  int ch, prefix;
  time_t chrono;
  char *str, fpath[80];
  struct dirent *de;
  DIR *dirp;

  SyncData *xpool;
  int xsize, xhead;

  if (xpool = sync_pool)
  {
    xsize = sync_size;
  }
  else
  {
    xpool = (SyncData *) malloc(SYNC_DB_SIZE * sizeof(SyncData));
    xsize = SYNC_DB_SIZE;
  }

  xhead = 0;

  ch = strlen(fname);
  memcpy(fpath, fname, ch);
  fname = fpath + ch;
  *fname++ = '/';
  fname[1] = '\0';

  /* itoc.030325.註解: 先把 brd/brdname/?/* 的所有檔案都丟進去 xpool[]
     這份觀察名單中，然後回到 expire() 中檢查該檔案是否在 .DIR 中
     若在 .DIR 中，就把 exotic 設回 0
     最後在 sync_check() 中把觀察名單中 exotic 還是 1 的檔案都刪除 */

  ch = '0';
  for (;;)
  {
    *fname = ch++;

    if (dirp = opendir(fpath))
    {
      while (de = readdir(dirp))
      {
	str = de->d_name;
	prefix = *str;
	if (prefix == '.')
	  continue;

	chrono = chrono32(str);

	if (chrono > synctime)	/* 這是最近剛發表的文章，不需要加入 xpool[] 去 sync */
	  continue;

	if (xhead >= xsize)
	{
	  xsize += (xsize >> 1);
	  xpool = (SyncData *) realloc(xpool, xsize * sizeof(SyncData));
	}

	xpool[xhead].chrono = chrono;
	xpool[xhead].prefix = prefix;
	xpool[xhead].exotic = 1;	/* 先全部令為 1，於 expire() 中再改回 0 */
	xhead++;
      }

      closedir(dirp);
    }

    if (ch == 'W')
      break;

    if (ch == '9' + 1)
      ch = 'A';
  }

  if (xhead > 1)
    qsort(xpool, xhead, sizeof(SyncData), sync_cmp);

  sync_pool = xpool;
  sync_size = xsize;
  sync_head = xhead;
}


static void
sync_check(flog, fname)
  FILE *flog;
  char *fname;
{
  char *str, fpath[80];
  SyncData *xpool, *xtail;
  time_t cc;

  if ((cc = sync_head) <= 0)
    return;

  xpool = sync_pool;
  xtail = xpool + cc;

  sprintf(fpath, "%s/ /", fname);
  str = strchr(fpath, ' ');
  fname = str + 3;

  do
  {
    if (xtail->exotic)
    {
      cc = xtail->chrono;
      fname[-1] = xtail->prefix;
      *str = radix32[cc & 31];
      archiv32(cc, fname);
      unlink(fpath);

      fprintf(flog, "-\t%s\n", fpath);
    }
  } while (--xtail >= xpool);
}


static void
expire(flog, life, sync)
  FILE *flog;
  Life *life;
  int sync;
{
  HDR hdr;
  struct stat st;
  char fpath[128], fnew[128], index[128], *fname, *bname, *str;
  int done, keep, total;
  FILE *fpr, *fpw;

  int days, maxp, minp;
  int duetime;

  SyncData *xpool, *xsync;
  int xhead;

  days = life->days;
  maxp = life->maxp;
  minp = life->minp;
  bname = life->bname;

  fprintf(flog, "%s\n", bname);

  sprintf(index, "%s/.DIR", bname);
  if (!(fpr = fopen(index, "r")))
  {
    fprintf(flog, "\tError open file: %s\n", index);
    return;
  }

  fpw = f_new(index, fnew);
  if (!fpw)
  {
    fprintf(flog, "\tExclusive lock: %s\n", fnew);
    fclose(fpr);
    return;
  }

  if (sync)
  {
    sync_init(bname);
    xpool = sync_pool;
    xhead = sync_head;
    if (xhead <= 0)
      sync = 0;
    else
      fprintf(flog, "\t%d files to sync\n\n", xhead);
  }

  strcpy(fpath, index);
  str = (char *) strrchr(fpath, '.');
  fname = str + 1;
  *fname++ = '/';

  done = 1;
  duetime = synctime - days * 86400;

  fstat(fileno(fpr), &st);
  total = st.st_size / sizeof(HDR);

  while (fread(&hdr, sizeof(HDR), 1, fpr) == 1)
  {
    if (hdr.xmode & (POST_MARKED | POST_BOTTOM) || total <= minp)
      keep = 1;
    else if (hdr.chrono < duetime || total > maxp)
      keep = 0;
    else
      keep = 1;

    if (sync && (hdr.chrono < synctime))
    {
      if (xsync = (SyncData *) bsearch(&hdr.chrono, xpool, xhead, sizeof(SyncData), sync_cmp))
      {
	xsync->exotic = 0;	/* 這篇在 .DIR 中，不 sync */
      }
      else
      {
        keep = 0;		/* 一律不保留 */
      }
    }

    if (keep)
    {
      if (fwrite(&hdr, sizeof(HDR), 1, fpw) != 1)
      {
	fprintf(flog, "\tError in write DIR.n: %s\n", hdr.xname);
	done = 0;
        sync = 0; /* Thor.990127: 沒作成, 就別砍了吧 */
	break;
      }
    }
    else
    {
      *str = hdr.xname[7];
      strcpy(fname, hdr.xname);
      unlink(fpath);
      fprintf(flog, "\t%s\n", fname);
      total--;
    }
  }
  fclose(fpr);
  fclose(fpw);

  if (done)
  {
    sprintf(fpath, "%s.o", index);
    if (!rename(index, fpath))
    {
      if (rename(fnew, index))
        rename(fpath, index);		/* 換回來 */
    }
  }
  unlink(fnew);

  if (sync)
    sync_check(flog, bname);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  FILE *fp;
  int number, count;
  Life db, table[MAXBOARD], *key;
  char *ptr, *bname, buf[256];
  BRD *brdp, *bend;

  /* itoc.030325: 要嘛不指定參數，要嘛指定所有參數 */
  if (argc != 1 && argc != 5)
  {
    printf("%s [day max min board]\n", argv[0]);
    exit(-1);
  }

  /* 若無指定參數，則用預設值 */
  db.days = ((argc == 5) && (number = atoi(argv[1])) > 0) ? number : BRD_EXPIRE_DAYS;
  db.maxp = ((argc == 5) && (number = atoi(argv[2])) > 0) ? number : BRD_EXPIRE_MAXP;
  db.minp = ((argc == 5) && (number = atoi(argv[3])) > 0) ? number : BRD_EXPIRE_MINP;

  /* --------------------------------------------------- */
  /* load expire.conf					 */
  /* --------------------------------------------------- */

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(BBSHOME);

  init_bshm();

  count = 0;
  if (argc != 5)	/* 若沒有指定參數，才需要去讀 expire.conf */
  {
    if (fp = fopen(FN_ETC_EXPIRE, "r"))
    {
      while (fgets(buf, sizeof(buf), fp))
      {
	if (buf[0] == '#')
	  continue;

	bname = (char *) strtok(buf, " \t\r\n");
	if (bname && *bname)
	{
	  ptr = (char *) strtok(NULL, " \t\r\n");
	  if (ptr && (number = atoi(ptr)) > 0)
	  {
	    key = &(table[count]);
	    strcpy(key->bname, bname);
	    key->days = number;
	    key->maxp = db.maxp;
	    key->minp = db.minp;

	    ptr = (char *) strtok(NULL, " \t\r\n");
	    if (ptr && (number = atoi(ptr)) > 0)
	    {
	      key->maxp = number;

	      ptr = (char *) strtok(NULL, " \t\r\n");
	      if (ptr && (number = atoi(ptr)) > 0)
		key->minp = number;
	    }

	    /* expire.conf 可能沒有維護而超過 MAXBOARD 個板 */
	    if (++count >= MAXBOARD)
	      break;
	  }
	}
      }
      fclose(fp);
    }

    if (count > 1)
      qsort(table, count, sizeof(Life), life_cmp);
  }

  /* --------------------------------------------------- */
  /* visit all boards					 */
  /* --------------------------------------------------- */

  fp = fopen(EXPIRE_LOG, "w");

  chdir("brd");

  synctime = time(NULL) - 10 * 60;	/* 十分鐘內的新文章不需要 sync */
  number = synctime / 86400;

  brdp = bshm->bcache;
  bend = brdp + bshm->number;
  do
  {
    bname = brdp->brdname;
    if (!*bname)
      continue;

    /* Thor.981027: 加上 board 時，可 expire+sync 某一 board */
    if (argc == 5)
    {
      if (str_cmp(argv[4], bname))
	continue;

      number = 0;	/* 強迫 sync 這板 */
    }

    if (count)
    {
      key = (Life *) bsearch(bname, table, count, sizeof(Life), life_cmp);
      if (!key)
	key = &db;
    }
    else
    {
      key = &db;
    }

    strcpy(key->bname, bname);		/* 換成正確的大小寫 */
    expire(fp, key, !(number & 31));	/* 每隔 32 天 sync 一次 */
    number++;
    brdp->btime = -1;
  } while (++brdp < bend);

  fclose(fp);
  exit(0);
}
