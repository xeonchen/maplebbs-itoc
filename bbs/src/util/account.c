/*-------------------------------------------------------*/
/* util/account.c	( NTHU CS MapleBBS Ver 3.00 )	 */
/*-------------------------------------------------------*/
/* target : 上站人次統計、系統資料備份、開票		 */
/* create : 95/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/
/* syntax : 本程式宜以 crontab 執行，設定時間為每小時	 */
/* 1-5 分 之間						 */
/*-------------------------------------------------------*/
/* notice : brdshm (board shared memory) synchronize	 */
/*-------------------------------------------------------*/


#include "bbs.h"

#include <sys/ipc.h>
#include <sys/shm.h>


#define	MAX_LINE	16
#define	ADJUST_M	10	/* adjust back 10 minutes */


/* itoc.011004.註解: 用 - 代表這次的，用 = 代表上次的。在 gem/@/ 下有很多這樣的範例 */

static char fn_today[] = "gem/@/@-act";		/* 今日上站人次統計 */
static char fn_yesday[] = "gem/@/@=act";	/* 昨日上站人次統計 */
static char log_file[] = FN_RUN_USIES "=";
static char act_file[] = "run/var/act";		/* 上站人次統計紀錄檔 */

static time_t now;			/* 執行程式的時間 */


/* ----------------------------------------------------- */
/* 開票：shm 部份須與 cache.c 相容			 */
/* ----------------------------------------------------- */


static BCACHE *bshm;


static int
boardname_cmp(a, b)
  BRD *a, *b;
{
  return str_cmp(a->brdname, b->brdname);
}


static void
fix_brd()
{
  BRD allbrd[MAXBOARD], brd;
  FILE *fp;
  int i, num;

  if (!(fp = fopen(FN_BRD, "r")))
    return;

  num = 0;
  for (i = 0; i < MAXBOARD; i++)
  {
    if (fread(&brd, sizeof(BRD), 1, fp) != 1)
      break;

    if (!*brd.brdname)	/* 此板已被刪除 */
      continue;

    memcpy(&allbrd[num], &brd, sizeof(BRD));
    num++;
  }

  fclose(fp);

  /* itoc.041110: 在第一次載入 bshm 時，將 bno 依板名排序 */
  if (num > 1)
    qsort(allbrd, num, sizeof(BRD), boardname_cmp);

  unlink(FN_BRD);

  if (num)
    rec_add(FN_BRD, allbrd, sizeof(BRD) * num);
}


#ifdef HAVE_MODERATED_BOARD
static int
int_cmp(a, b)
  int *a, *b;
{
  return *a - *b;
}
#endif


static void
init_allbrd()
{
  BRD *head, *tail;
#ifdef HAVE_MODERATED_BOARD
  int fd;
  char fpath[64];
  BPAL *bpal;
#endif

  head = bshm->bcache;
  tail = head + bshm->number;
#ifdef HAVE_MODERATED_BOARD
  bpal = bshm->pcache;
#endif

  do
  {
    /* itoc.040314: 板主更改看板敘述或是站長更改看板時才會把 bpost/blast 寫進 .BRD 中 
       所以 .BRD 裡的 bpost/blast 未必是對的，要重新 initial。
       initial 的方法是將 btime 設成 -1，讓 class_item() 去更新 */
    head->btime = -1;

#ifdef HAVE_MODERATED_BOARD
    brd_fpath(fpath, head->brdname, FN_PAL);
    if ((fd = open(fpath, O_RDONLY)) >= 0)
    {
      struct stat st;
      PAL *pal, *up;
      int count;

      fstat(fd, &st);
      if (pal = (PAL *) malloc(count = st.st_size))
      {
	count = read(fd, pal, count) / sizeof(PAL);
	if (count > 0 && count <= PAL_MAX)
	{
	  int *userno;
	  int c = count;

	  userno = bpal->pal_spool;
	  up = pal;
	  do
	  {
	    *userno++ = (up->ftype & PAL_BAD) ? -(up->userno) : up->userno;
	    up++;
	  } while (--c);

	  if (count > 1)
	    qsort(bpal->pal_spool, count, sizeof(int), int_cmp);
	  bpal->pal_max = count;
	}
	free(pal);
      }
      close(fd);
    }

    bpal++;
#endif

  } while (++head < tail);     
}


static void
init_bshm()
{
  time_t *uptime;
  int n, turn;

  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));
  uptime = &(bshm->uptime);

  turn = 0;
  for (;;)
  {
    n = *uptime;
    if (n > 0)		/* bshm 已 initial 完成 */
      return;

    if (n < 0)
    {
      if (++turn < 30)
      {
	sleep(2);
	continue;
      }
    }

    *uptime = -1;

    fix_brd();			/* itoc.030725: 在第一次載入 bshm 前，先整理 .BRD */

    if ((n = open(FN_BRD, O_RDONLY)) >= 0)
    {
      turn = read(n, bshm->bcache, MAXBOARD * sizeof(BRD)) / sizeof(BRD);
      close(n);
      bshm->number = bshm->numberOld = turn;

      init_allbrd();
    }

    /* 等所有 boards 資料更新後再設定 uptime */

    time(uptime);
    fprintf(stderr, "[account]\tCACHE\treload bcache\n");
    return;
  }
}


/* ----------------------------------------------------- */
/* keep log in board					 */
/* ----------------------------------------------------- */


static void
update_btime(brdname)
  char *brdname;
{
  BRD *brdp, *bend;

  brdp = bshm->bcache;
  bend = brdp + bshm->number;
  do
  {
    if (!strcmp(brdname, brdp->brdname))
    {
      brdp->btime = -1;
      break;
    }
  } while (++brdp < bend);
}


static void
keeplog(fnlog, board, title, mode)
  char *fnlog;
  char *board;
  char *title;
  int mode;			/* 0:load  1:rename  2:unlink */
{
  HDR hdr;
  char folder[64], fpath[64];
  int fd;
  FILE *fp;

  if (!dashf(fnlog))	/* Kudo.010804: 檔案是空的就不 keeplog */
    return;

  if (!board)
    board = BN_RECORD;

  brd_fpath(folder, board, FN_DIR);
  fd = hdr_stamp(folder, 'A', &hdr, fpath);
  if (fd < 0)
    return;

  if (mode == 1)
  {
    close(fd);
    /* rename(fnlog, fpath); */
    f_mv(fnlog, fpath);		/* Thor.990409: 可跨partition */
  }
  else
  {
    fp = fdopen(fd, "w");
    fprintf(fp, "作者: %s (%s)\n標題: %s\n時間: %s\n\n",
      STR_SYSOP, SYSOPNICK, title, Btime(&hdr.chrono));
    f_suck(fp, fnlog);
    fclose(fp);
    if (mode)
      unlink(fnlog);
  }

  strcpy(hdr.title, title);
  strcpy(hdr.owner, STR_SYSOP);
  rec_bot(folder, &hdr, sizeof(HDR));

  update_btime(board);
}


/* ----------------------------------------------------- */
/* build vote result					 */
/* ----------------------------------------------------- */


struct Tchoice
{
  int count;
  vitem_t vitem;
};


static int
TchoiceCompare(i, j)
  struct Tchoice *i, *j;
{
  return j->count - i->count;
}


static int
draw_vote(brd, fpath, vch)
  BRD *brd;			/* Thor: 傳入 BRD, 可查 battr */
  char *fpath;
  VCH *vch;
{
  struct Tchoice choice[MAX_CHOICES];
  FILE *fp;
  char *fname, *bid, buf[80];
  int total, items, num, fd, ticket, bollt;
  VLOG vlog;
  char *list = "@IOLGZ";	/* itoc.註解: 清 vote file */

  bid = brd->brdname;
  fname = strrchr(fpath, '@');

  /* vote item */

  *fname = 'I';

  items = 0;
  if (fp = fopen(fpath, "r"))
  {
    while (fread(choice[items].vitem, sizeof(vitem_t), 1, fp) == 1)
    {
      choice[items].count = 0;
      items++;
    }
    fclose(fp);
  }

  if (items == 0)
    return 0;

  /* 累計投票結果 */

  *fname = 'G';
  bollt = 0;		/* Thor: 總票數歸零 */
  total = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
    {
      for (ticket = vlog.choice, num = 0; ticket && num < items; ticket >>= 1, num++)
      {
	if (ticket & 1)
	{
	  choice[num].count += vlog.numvotes;
	  bollt += vlog.numvotes;
	}
      }
      total++;
    }
    close(fd);
  }

  /* 產生開票結果 */

  *fname = 'Z';
  if (!(fp = fopen(fpath, "w")))
    return 0;

  fprintf(fp, "\n\033[1;34m" MSG_SEPERATOR "\033[m\n\n"
    "\033[1;32m◆ [%s] 看板投票：%s\033[m\n\n舉辦板主：%s\n\n舉辦日期：%s\n\n",
    bid, vch->title, vch->owner, Btime(&vch->chrono));
  fprintf(fp, "開票日期：%s\n\n\033[1;32m◆ 投票主題：\033[m\n\n", Btime(&vch->vclose));

  *fname = '@';
  f_suck(fp, fpath);

  fprintf(fp, "\n\033[1;32m◆ 投票結果：每人可投 %d 票，共 %d 人參加，投出 %d 票\033[m\n\n",
    vch->maxblt, total, bollt);

  if (vch->vsort == 's')
    qsort(choice, items, sizeof(struct Tchoice), TchoiceCompare);

  if (vch->vpercent == '%')
    fd = BMAX(1, bollt);
  else
    fd = 0;

  for (num = 0; num < items; num++)
  {
    ticket = choice[num].count;
    if (fd)
      fprintf(fp, "    %-36s%5d 票 (%4.1f%%)\n", choice[num].vitem, ticket, 100.0 * ticket / fd);
    else
      fprintf(fp, "    %-36s%5d 票\n", choice[num].vitem, ticket);
  }

  /* other opinions */

  *fname = 'O';
  fputs("\n\033[1;32m◆ 我有話要說：\033[m\n\n", fp);
  f_suck(fp, fpath);
  fputs("\n", fp);
  fclose(fp);

  fp = fopen(fpath, "w");	/* Thor: 用 O_ 暫存一下下... */
  *fname = 'Z';
  f_suck(fp, fpath);
  sprintf(buf, "brd/%s/@/@vote", bid);
  f_suck(fp, buf);
  fclose(fp);
  *fname = 'O';
  rename(fpath, buf);

  /* 將開票結果 post 到 [BN_RECORD] 與 本看板 */

  *fname = 'Z';

  /* Thor: 若看板屬性為 BRD_NOVOTE，則不 post 到 [BN_RECORD] */

  if (!(brd->battr & BRD_NOVOTE))
  {
    sprintf(buf, "[記錄] %s <<看板選情報導>>", bid);
    keeplog(fpath, NULL, buf, 0);
  }

  keeplog(fpath, bid, "[記錄] 選情報導", 2);

  while (*fname = *list++)
    unlink(fpath); /* Thor: 確定名字就砍 */

  return 1;
}


static int		/* 0:不需寫回.BRD !=0:需寫回.BRD */
draw_board(brd)
  BRD *brd;
{
  int fd, fsize, alive;
  int oldbvote, newbvote;
  VCH *vch, *head, *tail;
  char *fname, fpath[64], buf[64];
  struct stat st;

  /* 由 account 來 maintain brd->bvote */

  oldbvote = brd->bvote;

  brd_fpath(fpath, brd->brdname, FN_VCH);

  if ((fd = open(fpath, O_RDWR | O_APPEND, 0600)) < 0 || fstat(fd, &st) || (fsize = st.st_size) <= 0)
  {
    if (fd >= 0)
    {
      close(fd);
      unlink(fpath);
    }
    brd->bvote = 0;
    return oldbvote;
  }

  vch = (VCH *) malloc(fsize);

  /* flock(fd, LOCK_EX); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_exlock(fd);

  read(fd, vch, fsize);

  strcpy(buf, fpath);
  fname = strrchr(buf, '.');
  *fname++ = '@';
  *fname++ = '/';

  head = vch;
  tail = (VCH *) ((char *)vch + fsize);

  alive = 0;
  newbvote = 0;

  do
  {
    if (head->vclose < now && head->vgamble == ' ')	/* 賭盤不讓 account 開 */
    {
      strcpy(fname, head->xname);
      draw_vote(brd, buf, head);/* Thor: 傳入 BRD, 可查 battr */
      head->chrono = 0;
    }
    else
    {
      alive++;

      if (head->vgamble == '$')
	newbvote = -1;
    }
  } while (++head < tail);


  if (alive && alive != fsize / sizeof(VCH))
  {
    ftruncate(fd, 0);
    head = vch;
    do
    {
      if (head->chrono)
      {
	write(fd, head, sizeof(VCH));
      }
    } while (++head < tail);
  }

  /* flock(fd, LOCK_UN);  */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_unlock(fd);

  close(fd);

  free(vch);

  if (!alive)
    unlink(fpath);
  else if (!newbvote)
    newbvote = 1;	/* 只有投票，沒有賭盤 */

  if (oldbvote != newbvote)
  {
    brd->bvote = newbvote;
    return 1;
  }

  return 0;
}


static void
closepolls()
{
  BRD *bcache, *head, *tail;
  int dirty;

  dirty = 0;

  head = bcache = bshm->bcache;
  tail = head + bshm->number;
  do
  {
    dirty |= draw_board(head);
  } while (++head < tail);

  if (!dirty)
    return;

  /* write back the shm cache data */

  if ((dirty = open(FN_BRD, O_WRONLY | O_CREAT, 0600)) < 0)
    return;

  /* flock(dirty, LOCK_EX); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_exlock(dirty);

  write(dirty, bcache, (char *)tail - (char *)bcache);

  /* flock(dirty, LOCK_UN); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_unlock(dirty);

  close(dirty);
  time(&bshm->uptime);
}


/* ----------------------------------------------------- */
/* build Class image					 */
/* ----------------------------------------------------- */


#define CLASS_RUNFILE	"run/class.run"


static ClassHeader *chx[CH_MAX];
static int chn;
static BRD *bhead, *btail;


static int
class_parse(key)
  char *key;
{
  char *str, *ptr, fpath[64];
  ClassHeader *chp;
  HDR hdr;
  int i, len, count;
  FILE *fp;

  strcpy(fpath, "gem/@/@");
  str = fpath + sizeof("gem/@/@") - 1;
  for (ptr = key;; ptr++)
  {
    i = *ptr;
    if (i == '/')
      i = 0;
    *str = i;
    if (!i)
      break;
    str++;
  }

  len = ptr - key;

  /* search classes */

  for (i = 1; i < chn; i++)
  {
    str = chx[i]->title;
    if (str[len] == '/' && !memcmp(key, str, len))
      return CH_END - i;
  }

  /* build classes */

  if (fp = fopen(fpath, "r"))
  {
    int ans;
    struct stat st;

    if (fstat(fileno(fp), &st) || (count = st.st_size / sizeof(HDR)) <= 0)
    {
      fclose(fp);
      return CH_END;
    }

    /* itoc.030723: 檢查 Class 數量是否超過 CH_MAX */
    if (chn >= CH_MAX - 1)
    {
      static int show_error = 0;
      if (!show_error)		/* 錯誤訊息只印一次 */
      {
	fprintf(stderr, "[account]\t請加大您的 CH_MAX 定義，CH_MAX 必須超過 Class 的數量\n");
	show_error = 1;
      }
      fclose(fp);
      return CH_END;
    }

    chx[chn++] = chp = (ClassHeader *) malloc(sizeof(ClassHeader) + count * sizeof(short));
    memset(chp->title, 0, CH_TTLEN);
    strcpy(chp->title, key);

    ans = chn;
    count = 0;

    while (fread(&hdr, sizeof(hdr), 1, fp) == 1)
    {
      if (hdr.xmode & GEM_BOARD)
      {
	BRD *bp;

	i = -1;
	str = hdr.xname;
	bp = bhead;

	for (;;)
	{
	  i++;
	  if (!str_cmp(str, bp->brdname))
	    break;

	  if (++bp >= btail)
	  {
	    i = -1;
	    break;
	  }
	}

	if (i < 0)
	  continue;
      }
      else
      {
	/* recursive 地一層一層進去建 Class */
	i = class_parse(hdr.title);

	if (i == CH_END)
	  continue;
      }

      chp->chno[count++] = i;
    }

    fclose(fp);

    chp->count = count;
    return -ans;
  }

  return CH_END;
}


static int
brdname_cmp(i, j)
  short *i, *j;
{
  return str_cmp(bhead[*i].brdname, bhead[*j].brdname);
}


static int
brdtitle_cmp(i, j)		/* itoc.010413: 依看板中文敘述排序 */
  short *i, *j;
{
  /* return strcmp(bhead[*i].title, bhead[*j].title); */

  /* itoc.010413: 分類/板名交叉比對 */
  int k = strcmp(bhead[*i].class, bhead[*j].class);
  return k ? k : str_cmp(bhead[*i].brdname, bhead[*j].brdname);
}


static void
class_sort(cmp)
  int (*cmp) ();
{
  ClassHeader *chp;
  int i, j, max;
  BRD *bp;

  max = bshm->number;
  bhead = bp = bshm->bcache;
  btail = bp + max;

  chp = (ClassHeader *) malloc(sizeof(ClassHeader) + max * sizeof(short));

  for (i = j = 0; i < max; i++, bp++)
  {
    if (bp->brdname)
      chp->chno[j++] = i;
  }

  chp->count = j;

  qsort(chp->chno, j, sizeof(short), cmp);

  memset(chp->title, 0, CH_TTLEN);
  strcpy(chp->title, "Boards");
  chx[chn++] = chp;
}


static void
class_image()
{
  int i, times;
  FILE *fp;
  short len, pos[CH_MAX];
  ClassHeader *chp;

  for (times = 2; times > 0; times--)	/* itoc.010413: 產生二份 class image */
  {
    chn = 0;
    class_sort(times == 1 ? brdname_cmp : brdtitle_cmp);
    class_parse(CLASS_INIFILE);

    if (chn < 2)		/* lkchu.990106: 尚沒有分類 */
      return;

    len = sizeof(short) * (chn + 1);

    for (i = 0; i < chn; i++)
    {
      pos[i] = len;
      len += CH_TTLEN + chx[i]->count * sizeof(short);
    }

    pos[i++] = len;

    if (fp = fopen(CLASS_RUNFILE, "w"))
    {
      fwrite(pos, sizeof(short), i, fp);
      for (i = 0; i < chn; i++)
      {
	chp = chx[i];
	fwrite(chp->title, 1, CH_TTLEN + chp->count * sizeof(short), fp);
	free(chp);
      }
      fclose(fp);

      rename(CLASS_RUNFILE, times == 1 ? CLASS_IMGFILE_NAME : CLASS_IMGFILE_TITLE);
    }
  }

  bshm->min_chn = -chn;
}


/* ----------------------------------------------------- */
/* 上站人數統計						 */
/* ----------------------------------------------------- */


static void
error(fpath)
  char *fpath;
{
  printf("can not open [%s]\n", fpath);
  /* exit(1); */	/* itoc.011004: 上站人次統計失敗，無需中斷 account 執行 */
}


static void
ansi_puts(fp, buf, mode)
  FILE *fp;
  char *buf, mode;
{
  static char state = '0';

  if (state != mode)
    fprintf(fp, "\033[3%cm", state = mode);
  if (*buf)
  {
    fprintf(fp, "%s", buf);
    *buf = '\0';
  }
}


static void
draw_usies(ptime)
  struct tm *ptime;
{
  int fact, hour, max, item, total, i, j, over;
  char buf[256];
  FILE *fp, *fpw;
  int act[26];			/* act[0~23]:0~23時各小時的上站人次 act[24]:停留累計時間 act[25]:累積人次 */

  static char run_file[] = FN_RUN_USIES;
  static char tmp_file[] = FN_RUN_USIES ".tmp";

  rename(run_file, tmp_file);
  if (!(fp = fopen(tmp_file, "r")))
  {
    /* error(tmp_file); */	/* itoc.011004.註解: 沒有 tmp_file 表示沒有 run_file，表示從上次跑 account 到現在， */
    return;			/* 沒有人 login 過 bbs。通常發生在手動跑 account 頻繁時。 */
  }

  if (!(fpw = fopen(log_file, "a")))
  {
    fclose(fp);
    error(log_file);		/* itoc.011004.註解: log_file 是昨天 run_file。如果昨天整天都沒有人 login 過 bbs， */
    return;			/* 就會發生沒有 log_file 的狀況 */
  }

  if ((fact = open(act_file, O_RDWR | O_CREAT, 0600)) < 0)
  {
    fclose(fp);
    fclose(fpw);
    error(act_file);		/* itoc.011004.註解: 都已經有 O_CREAT 如果還沒有 act_file 的話..好自為之吧 :P */
    return;
  }

  memset(act, 0, sizeof(act));

  if (ptime->tm_hour != 0)
  {
    read(fact, act, sizeof(act));
    lseek(fact, 0, SEEK_SET);
  }

  while (fgets(buf, sizeof(buf), fp))
  {
    fputs(buf, fpw);

    if (!memcmp(buf + 24, "ENTER", 5))
    {
      hour = atoi(buf + 15);
      if (hour >= 0 && hour <= 23)
	act[hour]++;
      continue;
    }

    if (!memcmp(buf + 43, "Stay:", 5))
    {
      if (hour = atoi(buf + 49))
      {
	act[24] += hour;
	act[25]++;
      }
      continue;
    }
  }
  fclose(fp);
  fclose(fpw);
  unlink(tmp_file);

  write(fact, act, sizeof(act));
  close(fact);

  for (i = max = total = 0; i < 24; i++)
  {
    total += act[i];		/* itoc.030415.註解: act[25] 未必等於 total，有人也許不正常離站 */
    if (act[i] > max)
      max = act[i];
  }

  item = max / MAX_LINE + 1;
  over = max > 1000;

  if (!(fp = fopen(fn_today, "w")))
  {
    error(fn_today);
    return;
  }

  /* Thor.990329: y2k */
  fprintf(fp, "%24s\033[1;33;46m [%02d/%02d/%02d] 上站人次統計 \033[40m\n",
    "", ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);

  for (i = MAX_LINE + 1; i > 0; i--)
  {
    strcpy(buf, "   ");
    for (j = 0; j < 24; j++)
    {
      max = item * i;
      hour = act[j];
      if (hour && (max > hour) && (max - item <= hour))
      {
	ansi_puts(fp, buf, '3');
	if (over)
	  hour = (hour + 5) / 10;
	fprintf(fp, "%-3d", hour);
      }
      else if (max <= hour)
      {
	ansi_puts(fp, buf, '1');
	fprintf(fp, "█ ");
      }
      else
	strcat(buf, "   ");
    }
    fprintf(fp, "\n");
  }

  if (act[25] == 0)
    act[25] = 1;		/* Thor.980928: lkchu patch: 防止除數為0 */

  fprintf(fp, "\033[34m"
    "  ╒═══════════════════════════════════╕\n  \033[32m"
    "0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23\n\n"
    "        %s     \033[35m總共上站人次：\033[37m%-9d\033[35m平均使用時間：\033[37m%d\033[m\n",
    over ? "\033[35m單位：\033[37m10 人" : "", total, act[24] / act[25] + 1);
  fclose(fp);
}


/* ----------------------------------------------------- */
/* 壓縮程式						 */
/* ----------------------------------------------------- */


static void
gzip(source, target, stamp)
  char *source, *target, *stamp;
{
  char buf[128];

  if (dashf(source))
  {
    sprintf(buf, "/usr/bin/gzip -n log/%s%s", target, stamp);
    /* rename(source, buf + 17); */
    f_mv(source, buf + 17);	/* Thor.990409: 可跨 partition */
    system(buf);
  }
}


/* ----------------------------------------------------- */
/* 產生驗證信的 private key				 */
/* ----------------------------------------------------- */


#ifdef HAVE_SIGNED_MAIL
static void
private_key(ymd)
  char *ymd;
{
  srandom(time(NULL));

#if (PRIVATE_KEY_PERIOD == 0)
  if (!dashf(FN_RUN_PRIVATE))
#else
  if (!dashf(FN_RUN_PRIVATE) || (random() % PRIVATE_KEY_PERIOD) == 0)
#endif
  {
    int i, j;
    char buf[80];

    sprintf(buf, "log/prikey%s", ymd);
    f_mv(FN_RUN_PRIVATE, buf);
    i = 8;
    for (;;)
    {
      j = random() & 0xff;
      if (!j)
	continue;
      buf[--i] = j;
      if (i == 0)
	break;
    }
    rec_add(FN_RUN_PRIVATE, buf, 8);
  }
}
#endif


/* ----------------------------------------------------- */
/* 主程式						 */
/* ----------------------------------------------------- */


int
main(argc, argv)
  int argc;
  char *argv[];
{
  struct tm ntime, *ptime;
  FILE *fp;

  now = time(NULL);	/* 一開始就要馬上記錄時間 */

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(BBSHOME);
  umask(077);

  /* --------------------------------------------------- */
  /* 初始化 board shm 用來建 Class 及開票		 */
  /* --------------------------------------------------- */

  init_bshm();

  /* --------------------------------------------------- */
  /* build Class image					 */
  /* --------------------------------------------------- */

  class_image();

  /* --------------------------------------------------- */
  /* 系統開票						 */
  /* --------------------------------------------------- */

  closepolls();

  /* --------------------------------------------------- */
  /* 資料統計時間					 */
  /* --------------------------------------------------- */

  /* itoc.030911: 若加了參數，表示不是在 crontab 裡跑的，那麼不做資料統計 */
  if (argc != 1)
    exit(0);

  /* ntime 是今天 */
  ptime = localtime(&now);
  memcpy(&ntime, ptime, sizeof(ntime));	/* 先存起來，因為還要做一次 localtime() */

  /* ptime 是昨天 */
  /* itoc.011004.註解: 由於 account 是算前一小時統計，所以在零時時要把時鐘轉回 10 分鐘，到昨天去 */
  /* itoc.030911.註解: 所以 account 必須在每小時的 1-10 分內執行 */
  now -= ADJUST_M * 60;		/* back to ancent */
  ptime = localtime(&now);

  /* --------------------------------------------------- */
  /* 上站人次統計					 */
  /* --------------------------------------------------- */

  draw_usies(ptime);

  /* --------------------------------------------------- */
  /* 資料壓縮備份、熱門話題統計				 */
  /* --------------------------------------------------- */

  if (ntime.tm_hour == 0)
  {
    char date[16], ymd[16];
    char title[80];

    sprintf(ymd, "-%02d%02d%02d",
      ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);	/* Thor.990329: y2k */

    sprintf(date, "[%d 月 %d 日] ", ptime->tm_mon + 1, ptime->tm_mday);


    /* ------------------------------------------------- */
    /* 每日凌晨零時做的事				 */
    /* ------------------------------------------------- */

    gzip(log_file, "usies", ymd);		/* 備份所有 [上站] 記錄 */

#ifdef HAVE_SIGNED_MAIL
    private_key(ymd);
#endif

    sprintf(title, "%s文章篇數統計", date);
    keeplog(FN_RUN_POST_LOG, BN_SECURITY, title, 2);

    sprintf(title, "%s寄信記錄", date);
    keeplog(FN_RUN_MAIL_LOG, BN_SECURITY, title, 2);

#ifdef HAVE_ANONYMOUS
    sprintf(title, "%s匿名文章發表", date);
    keeplog(FN_RUN_ANONYMOUS, BN_SECURITY, title, 2);
#endif

#ifdef HAVE_BUY
    sprintf(title, "%s匯錢記錄", date);
    keeplog(FN_RUN_BANK_LOG, BN_SECURITY, title, 2);
#endif

    system("grep OVER " BMTA_LOGFILE " | cut -f2 | cut -d' ' -f2- | sort | uniq -c > run/over.log");
    sprintf(title, "%sE-Mail over max connection 統計", date);
    keeplog("run/over.log", BN_SECURITY, title, 2);

    sprintf(title, "%s酸甜苦辣留言板", date);
    keeplog(FN_RUN_NOTE_ALL, NULL, title, 2);

    if (fp = fopen(fn_yesday, "w"))
    {
      f_suck(fp, fn_today);
      fclose(fp);
    }
    sprintf(title, "%s上站人次統計", date);
    keeplog(fn_today, NULL, title, 1);
    unlink(act_file);


    /* ------------------------------------------------- */
    /* 每週一凌晨零時做的事				 */
    /* ------------------------------------------------- */

    if (ntime.tm_wday == 0)
    {
      sprintf(title, "%s本週熱門話題", date);
      keeplog("gem/@/@-week", NULL, title, 0);

      sprintf(title, "%s偷懶板主統計", date);
      keeplog(FN_RUN_LAZYBM, BN_SECURITY, title, 2);

      sprintf(title, "%s特殊權限使用者列表", date);
      keeplog(FN_RUN_MANAGER, BN_SECURITY, title, 2);

      sprintf(title, "%s長期未上站被清除的使用者列表", date);
      keeplog(FN_RUN_REAPER, BN_SECURITY, title, 2);

      sprintf(title, "%s同一 email 認證多次", date);
      keeplog(FN_RUN_EMAILADDR, BN_SECURITY, title, 2);
    }


    /* ------------------------------------------------- */
    /* 每月一日凌晨零時做的事				 */
    /* ------------------------------------------------- */

    if (ntime.tm_mday == 1)
    {
      sprintf(title, "%s本月熱門話題", date);
      keeplog("gem/@/@-month", NULL, title, 0);
    }


    /* ------------------------------------------------- */
    /* 每年一月一日凌晨零時做的事			 */
    /* ------------------------------------------------- */

    if (ntime.tm_yday == 1)
    {
      sprintf(title, "%s年度熱門話題", date);
      keeplog("gem/@/@-year", NULL, title, 0);
    }
  }

  exit(0);
}
