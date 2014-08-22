/*-------------------------------------------------------*/
/* board.c	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : 看板、群組功能	 			 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;
extern XZ xz[];
extern char xo_pool[];


char brd_bits[MAXBOARD];

#ifndef ENHANCED_VISIT
time_t brd_visit[MAXBOARD];		/* 最近瀏覽時間 */
#endif


static char *class_img = NULL;
static XO board_xo;


/* ----------------------------------------------------- */
/* 看板閱讀記錄 .BRH (Board Reading History)		 */
/* ----------------------------------------------------- */


typedef struct BoardReadingHistory
{
  time_t bstamp;		/* 建立看板的時間, unique */	/* Thor.brh_tail */
  time_t bvisit;		/* 上次閱讀時間 */		/* Thor.980904: 沒在讀時放上次讀的時間, 正在讀時放 bhno */
  int bcount;			/* Thor.980902: 沒用到 */

  /* --------------------------------------------------- */
  /* time_t {final, begin} / {final | BRH_SIGN}		 */
  /* --------------------------------------------------- */
                           /* Thor.980904.註解: BRH_SIGN代表final begin 相同 */
                           /* Thor.980904.註解: 由大到小排列,存放已讀interval */
}                   BRH;


#define	BRH_EXPIRE	180		/* Thor.980902.註解: 保留多少天 */
#define BRH_MAX		200		/* Thor.980902.註解: 每板最多有幾個標籤 */
#define BRH_PAGE	2048		/* Thor.980902.註解: 每次多配量, 用不到了 */
#define	BRH_MASK	0x7fffffff	/* Thor.980902.註解: 最大量為2038年1月中*/
#define	BRH_SIGN	0x80000000	/* Thor.980902.註解: zap及壓final專用 */
#define	BRH_WINDOW	(sizeof(BRH) + sizeof(time_t) * BRH_MAX * 2)


static int *brh_base;		/* allocated memory */
static int *brh_tail;		/* allocated memory */
static int brh_size;		/* allocated memory size */
static time_t brh_expire;


static int *
brh_alloc(tail, size)
  int *tail;
  int size;
{
  int *base, n;

  base = brh_base;
  n = (char *) tail - (char *) base;
  size += n;
  if (size > brh_size)
  {
    /* size = (size & -BRH_PAGE) + BRH_PAGE; */
    size += n >> 4;		/* 多預約一些記憶體 */
    base = (int *) realloc((char *) base, size);

    if (base == NULL)
      abort_bbs();

    brh_base = base;
    brh_size = size;
    tail = (int *) ((char *) base + n);
  }

  return tail;
}


static void
brh_put()
{
  int *list;

  /* compact the history list */

  list = brh_tail;

  if (*list)
  {
    int *head, *tail, n, item, chrono;

    n = *++list;   /* Thor.980904: 正讀時是bhno */
    brd_bits[n] |= BRD_H_BIT;
    time((time_t *) list);    /* Thor.980904.註解: bvisit time */

    item = *++list;
    head = ++list;
    tail = head + item;

    while (head < tail)
    {
      chrono = *head++;
      n = *head++;
      if (n == chrono) /* Thor.980904.註解: 相同的時候壓起來 */
      {
	n |= BRH_SIGN;
	item--;
      }
      else
      {
	*list++ = chrono;
      }
      *list++ = n;
    }

    list[-item - 1] = item;
    *list = 0;
    brh_tail = list;  /* Thor.980904: 新的空brh */
  }
}


void
brh_get(bstamp, bhno)
  time_t bstamp;		/* board stamp */
  int bhno;
{
  int *head, *tail;
  int size, bcnt, item;
  char buf[BRH_WINDOW];

  if (bstamp == *brh_tail) /* Thor.980904.註解: 該板已在 brh_tail上 */
    return;

  brh_put();

  bcnt = 0;
  tail = brh_tail;

  if (brd_bits[bhno] & BRD_H_BIT)
  {
    head = brh_base;
    while (head < tail)
    {
      item = head[2];
      size = item * sizeof(time_t) + sizeof(BRH);

      if (bstamp == *head)
      {
	bcnt = item;
	memcpy(buf, head + 3, size);
	tail = (int *) ((char *) tail - size);
	if (item = (char *) tail - (char *) head)
	  memcpy(head, (char *) head + size, item);
	break;
      }
      head = (int *) ((char *) head + size);
    }
  }

  brh_tail = tail = brh_alloc(tail, BRH_WINDOW);

  *tail++ = bstamp;
  *tail++ = bhno;

  if (bcnt)			/* expand history list */
  {
    int *list;

    size = bcnt;
    list = tail;
    head = (int *) buf;

    do
    {
      item = *head++;
      if (item & BRH_SIGN)
      {
	item ^= BRH_SIGN;
	*++list = item;
	bcnt++;
      }
      *++list = item;
    } while (--size);
  }

  *tail = bcnt;
}


int
brh_unread(chrono)
  time_t chrono;
{
  int *head, *tail, item;

  /* itoc.010407.註解: BRH_EXPIRE (180) 天前的文章都設為已讀 */
  if (chrono <= brh_expire)
    return 0;

  head = brh_tail + 2;
  if ((item = *head) > 0)
  {
    /* check {final, begin} history list */

    head++;
    tail = head + item;
    do
    {
      if (chrono > *head)
	return 1;

      head++;
      if (chrono >= *head)
	return 0;

    } while (++head < tail);
  }
  return 1;
}


void
brh_visit(mode)
  int mode;			/* 0 : visit, 1: un-visit */
{				/* itoc.010207: 或是傳入chrono, 代表讀至哪 */
  int *list;

  list = (int *) brh_tail + 2;
  *list++ = 2;
  if (mode)
  {
    *list = mode;
  }
  else
  {
    time((time_t *)list);
  }
  /* *++list = mode; */
  *++list = 0;	/* itoc.010207: 強定為 0, for 部分 visit */
}


void
brh_add(prev, chrono, next)
  time_t prev, chrono, next;
{
  int *base, *head, *tail, item, final, begin;

  head = base = brh_tail + 2;
  item = *head++;
  tail = head + item;

  begin = BRH_MASK;

  while (head < tail)
  {
    final = *head;
    if (chrono > final)
    {
      if (prev <= final)
      {
	if (next < begin)	/* increase */
	  *head = chrono;
	else
	{			/* merge */
	  *base = item - 2;
	  base = head - 1;
	  do
	  {
	    *base++ = *++head;
	  } while (head < tail);
	}
	return;
      }

      if (next >= begin)
      {
	head[-1] = chrono;
	return;
      }

      break;
    }

    begin = *++head;
    head++;
  }

  /* insert or append */

  /* [21, 22, 23] ==> [32, 30] [15, 10] */

  if (item < BRH_MAX)
  {
    /* [32, 30] [22, 22] [15, 10] */

    *base = item + 2;
    tail += 2;
  }
  else
  {
    /* [32, 30] [22, 10] */  /* Thor.980923: how about [6, 7, 8] ? [15, 7] ? */

    tail--;
  }

  prev = chrono;
  for (;;)
  {
    final = *head;
    *head++ = chrono;

    if (head >= tail)
      return;

    begin = *head;
    *head++ = prev;

    if (head >= tail)
      return;

    chrono = final;
    prev = begin;
  }
}


/* ----------------------------------------------------- */
/* board permission check				 */
/* ----------------------------------------------------- */


int			/* >=1:第幾個板主 0:不是板主 */
is_bm(list, userid)
  char *list;		/* 板主：BM list */
  char *userid;
{
  return str_has(list, userid, strlen(userid));
}


static inline int
Ben_Perm(bno, ulevel)
  int bno;
  usint ulevel;
{
  usint readlevel, postlevel, bits;
  char *blist, *bname;
  BRD *brd;
#ifdef HAVE_MODERATED_BOARD
  BPAL *bpal;
  int ftype;	/* 0:一般ID 1:板好 2:板壞 */

  /* itoc.040103: 看板閱讀等級說明表

  ┌────┬────┬────┬────┐
  │        │一般用戶│看板好友│看板壞人│
  ├────┼────┼────┼────┤
  │一般看板│權限決定│  水桶  │ 看不見 │    看不見：在看板列表中無法看到這個板，也進不去
  ├────┼────┼────┼────┤    進不去：在看板列表中可以看到這個板，但是進不去
  │好友看板│ 進不去 │  完整  │  水桶  │    水  桶：在看板列表中可以看到這個板，也進得去，但是不能發文
  ├────┼────┼────┼────┤    完  整：在看板列表中可以看到這個板，也進得去及發文
  │秘密看板│ 看不見 │  完整  │  水桶  │
  └────┴────┴────┴────┘
  */

  static int bit_data[9] =
  {                /* 一般用戶   看板好友                           看板壞人 */
    /* 公開看板 */    0,         BRD_L_BIT | BRD_R_BIT,             0,
    /* 好友看板 */    BRD_L_BIT, BRD_L_BIT | BRD_R_BIT | BRD_W_BIT, BRD_L_BIT | BRD_R_BIT,
    /* 秘密看板 */    0,         BRD_L_BIT | BRD_R_BIT | BRD_W_BIT, BRD_L_BIT | BRD_R_BIT,
  };
#endif

  brd = bshm->bcache + bno;
  bname = brd->brdname;
  if (!*bname)
    return 0;

  readlevel = brd->readlevel;

#ifdef HAVE_MODERATED_BOARD
  bpal = bshm->pcache + bno;
  ftype = is_bgood(bpal) ? 1 : is_bbad(bpal) ? 2 : 0;

  if (readlevel == PERM_SYSOP)		/* 秘密看板 */
    bits = bit_data[6 + ftype];
  else if (readlevel == PERM_BOARD)	/* 好友看板 */
    bits = bit_data[3 + ftype];
  else if (ftype)			/* 公開看板，若在板好/板壞名單中 */
    bits = bit_data[ftype];
  else					/* 公開看板，其他依權限判定 */
#endif

  if (!readlevel || (readlevel & ulevel))
  {
    bits = BRD_L_BIT | BRD_R_BIT;

    postlevel = brd->postlevel;
    if (!postlevel || (postlevel & ulevel))
      bits |= BRD_W_BIT;
  }
  else
  {
    bits = 0;
  }

  /* Thor.980813.註解: 特別為 BM 考量，板主有該板的所有權限 */
  blist = brd->BM;
  if ((ulevel & PERM_BM) && blist[0] > ' ' && is_bm(blist, cuser.userid))
    bits = BRD_L_BIT | BRD_R_BIT | BRD_W_BIT | BRD_X_BIT | BRD_M_BIT;

  /* itoc.030515: 看板總管重新判斷 */
  else if (ulevel & PERM_ALLBOARD)
    bits = BRD_L_BIT | BRD_R_BIT | BRD_W_BIT | BRD_X_BIT;

  return bits;
}


/* ----------------------------------------------------- */
/* 載入 currboard 進行若干設定				 */
/* ----------------------------------------------------- */


int
bstamp2bno(stamp)
  time_t stamp;
{
  BRD *brd;
  int bno, max;

  bno = 0;
  brd = bshm->bcache;
  max = bshm->number;
  for (;;)
  {
    if (stamp == brd->bstamp)
      return bno;
    if (++bno >= max)
      return -1;
    brd++;
  }
}


static inline void
brh_load()
{
  BRD *brdp;
  usint ulevel;
  int n, cbno;
  char *bits;

  int size, *base;
  time_t expire;
  char fpath[64];

#ifndef ENHANCED_VISIT
  time_t *bstp;
#endif

  memset(bits = brd_bits, 0, sizeof(brd_bits));
#ifndef ENHANCED_VISIT
  memset(bstp = brd_visit, 0, sizeof(brd_visit));
#endif

  ulevel = cuser.userlevel;
  n = 0;
  cbno = bshm->number;

  do
  {
    *bits++ = Ben_Perm(n, ulevel);
  } while (++n < cbno);

  /* --------------------------------------------------- */
  /* 將 .BRH 載入 memory				 */
  /* --------------------------------------------------- */

  size = 0;
  cbno = -1;
  brh_expire = expire = time(0) - BRH_EXPIRE * 86400;

  if (ulevel)
  {
    struct stat st;

    usr_fpath(fpath, cuser.userid, FN_BRH);
    if (!stat(fpath, &st))
      size = st.st_size;
  }

  /* --------------------------------------------------- */
  /* 多保留 BRH_WINDOW 的運作空間			 */
  /* --------------------------------------------------- */

  /* brh_size = n = ((size + BRH_WINDOW) & -BRH_PAGE) + BRH_PAGE; */
  brh_size = n = size + BRH_WINDOW;
  brh_base = base = (int *) malloc(n);

  if (size && ((n = open(fpath, O_RDONLY)) >= 0))
  {
    int *head, *tail, *list, bstamp, bhno;

    size = read(n, base, size);
    close(n);

    /* compact reading history : remove dummy/expired record */

    head = base;
    tail = (int *) ((char *) base + size);
    bits = brd_bits;

    while (head < tail && head >= brh_base)
    {
      bstamp = *head;

      if (bstamp & BRH_SIGN)	/* zap */
      {
	bstamp ^= BRH_SIGN;
	bhno = bstamp2bno(bstamp);
	if (bhno >= 0)
	{
	  /* itoc.001029: NOZAP時, 仍會出現 */
	  brdp = bshm->bcache + bhno;
	  if (!(brdp->battr & BRD_NOZAP))
	    bits[bhno] |= BRD_Z_BIT;
	}
	head++;
	continue;
      }

      bhno = bstamp2bno(bstamp);
      list = head + 2;

      if (list > tail)
	break;

      n = *list;
      size = n + 3;

      /* 這個看板存在、沒有被 zap 掉、可以 list */

      if (bhno >= 0 && (bits[bhno] & BRD_L_BIT))
      {
	bits[bhno] |= BRD_H_BIT;/* 已有閱讀記錄 */

#ifndef ENHANCED_VISIT
	bstp[bhno] = head[1];	/* 上次閱讀時間 */
#endif

	cbno = bhno;

	if (n > 0)
	{
	  list += n;	/* Thor.980904.註解: 最後一個 tag */

	  if (list > tail)
	    break;

	  do
	  {
	    bhno = *list;
	    if ((bhno & BRH_MASK) > expire)
	      break;

	    if (!(bhno & BRH_SIGN))
	    {
	      if (*--list > expire)
		break;
	      n--;
	    }

	    list--;
	    n--;
	  } while (n > 0);

	  head[2] = n;
	}

	n = n * sizeof(time_t) + sizeof(BRH);
	if (base != head)
	  memcpy(base, head, n);
	base = (int *) ((char *) base + n);
      }
      head += size;
    }
  }

  *base = 0;
  brh_tail = base;
}


void
brh_save()
{
  int *base, *head, *tail, bhno, size;
  BRD *bhdr, *bend;
  char *bits;

  /* Thor.980830: lkchu patch:  還沒 load 就不用 save */ 
  if (!(base = brh_base))
    return;

  brh_put();

  /* save history of un-zapped boards */

  bits = brd_bits;
  head = base;
  tail = brh_tail;
  while (head < tail)
  {
    bhno = bstamp2bno(*head);
    size = head[2] * sizeof(time_t) + sizeof(BRH);
    if (bhno >= 0 && !(bits[bhno] & BRD_Z_BIT))
    {
      if (base != head)
	memcpy(base, head, size);
      base = (int *) ((char *) base + size);
    }
    head = (int *) ((char *) head + size);
  }

  /* save zap record */

  tail = brh_alloc(base, sizeof(time_t) * MAXBOARD);

  bhdr = bshm->bcache;
  bend = bhdr + bshm->number;
  do
  {
    if (*bits++ & BRD_Z_BIT)
    {
      *tail++ = bhdr->bstamp | BRH_SIGN;
    }
  } while (++bhdr < bend);

  /* OK, save it */

  base = brh_base;
  if ((size = (char *) tail - (char *) base) > 0)
  {
    char fpath[64];
    int fd;

    usr_fpath(fpath, cuser.userid, FN_BRH);
    if ((fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0600)) >= 0)
    {
      write(fd, base, size);
      close(fd);
    }
  }
}


/* ----------------------------------------------------- */
/* 分類 zap 記錄 .CZH (Class Zap History)		 */
/* ----------------------------------------------------- */


typedef struct ClassZapHistory
{
  char brdname[BNLEN + 1];	/* 分類的 brdname */
}                   CZH;


static char class_bits[CH_MAX + 3];


static int			/* >=0:pos  -1:不在 .CZH */
czh_find(fpath, brdname)	/* 檢查是否已經在 .CZH 內 */
  char *fpath;
  char *brdname;
{
  CZH czh;
  int fd, pos;
  int rc = -1;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    pos = 0;
    while (read(fd, &czh, sizeof(CZH)) == sizeof(CZH))
    {
      if (!strcmp(czh.brdname, brdname))
      {
	rc = pos;
	break;
      }
      pos++;
    }
    close(fd);
  }
  return rc;
}


static int		/* >=0:在 .CZH  -1:不在 .CZH */
czh_put(brdname)
  char *brdname;
{
  char fpath[64];
  int pos;

  usr_fpath(fpath, cuser.userid, FN_CZH);

  /* 若已經在 .CZH 內的話，則刪除此筆資料；若不在 .CZH 內的話，則加入此筆資料 */
  if ((pos = czh_find(fpath, brdname)) >= 0)
    rec_del(fpath, sizeof(CZH), pos, NULL);
  else
    rec_add(fpath, brdname, sizeof(CZH));

  return pos;
}


static void
czh_load()
{
  int fsize, chn, min_chn;
  short *chx;
  char *img, fpath[64];
  CZH *chead, *ctail, *czh;

  memset(class_bits, 0, sizeof(class_bits));

  usr_fpath(fpath, cuser.userid, FN_CZH);
  if (chead = (CZH *) f_img(fpath, &fsize))
  {
    min_chn = bshm->min_chn;

    czh = chead;
    ctail = chead + fsize / sizeof(CZH);
    img = class_img;
    do
    {
      for (chn = CH_END - 2;; chn--)	/* 在所有的分類中找一個板名相同的 */
      {
	chx = (short *) img + (CH_END - chn);
	if (!strncmp(czh->brdname, img + *chx, BNLEN))
	{
	  class_bits[-chn] |= BRD_Z_BIT;
	  break;
	}
	if (chn <= min_chn)	/* 如果找不到，表示該分類已消失，從 .CZH 刪除 */
	{
	  czh_put(czh->brdname);
	  break;
	}
      }
    } while (++czh < ctail);
    free(chead);
  }
}


/*-------------------------------------------------------*/

void
mantime_add(outbno, inbno)
  int outbno;
  int inbno;
{
  /* itoc.050613.註解: 人氣的減少不是在離開看板時，而是在進入新的看板或是離站時，
     這是為了避免 switch 跳看板會算錯人氣 */
  if (outbno >= 0)
    bshm->mantime[outbno]--;		/* 退出上一個板 */
  if (inbno >= 0)
    bshm->mantime[inbno]++;		/* 進入新的板 */
}


int
XoPost(bno)
  int bno;
{
  XO *xo;
  BRD *brd;
  int bits;
  char *str, fpath[64];

  brd = bshm->bcache + bno;
  if (!brd->brdname[0])	/* 已刪除的看板 */
    return -1;

  bits = brd_bits[bno];

  if (currbno != bno)	/* 看板沒換通常是因為 every_Z() 回原看板 */
  {
#ifdef HAVE_MODERATED_BOARD
    if (!(bits & BRD_R_BIT))
    {
      vmsg("對不起，此板只准板友進入，請向板主申請入境許\可");
      return -1;
    }
#endif

    /* 處理權限 */
    bbstate = STAT_STARTED;
    if (bits & BRD_M_BIT)
      bbstate |= (STAT_BM | STAT_BOARD | STAT_POST);
    else if (bits & BRD_X_BIT)
      bbstate |= (STAT_BOARD | STAT_POST);
    else if (bits & BRD_W_BIT)
      bbstate |= STAT_POST;

    mantime_add(currbno, bno);

    currbno = bno;
    currbattr = brd->battr;
    strcpy(currboard, brd->brdname);
    str = brd->BM;
    sprintf(currBM, "板主：%s", *str <= ' ' ? "徵求中" : str);
#ifdef HAVE_BRDMATE
    strcpy(cutmp->reading, currboard);
#endif

    brd_fpath(fpath, currboard, fn_dir);

#ifdef AUTO_JUMPPOST
    xz[XZ_POST - XO_ZONE].xo = xo = xo_get_post(fpath, brd);	/* itoc.010910: 為 XoPost 量身打造一支 xo_get() */
#else
    xz[XZ_POST - XO_ZONE].xo = xo = xo_get(fpath);
#endif
    xo->key = XZ_POST;
    xo->xyz = brd->title;
  }

  /* itoc.011113: 第一次進板一定要看進板畫面，第二次以後則取決 ufo 設定 */
  if (!(bits & BRD_V_BIT) || (cuser.ufo & UFO_BRDNOTE))
  {
    brd_bits[bno] = bits | BRD_V_BIT;
    brd_fpath(fpath, currboard, fn_note);
    more(fpath, NULL);
  }

  brh_get(brd->bstamp, bno);

  return 0;
}


#ifdef HAVE_FORCE_BOARD
void
brd_force()			/* itoc.010407: 強制閱讀公告板，且強迫讀最後一篇 */
{
  if (cuser.userlevel)		/* guest 跳過 */
  {
    int bno;
    BRD *brd;

    if ((bno = brd_bno(BN_ANNOUNCE)) < 0)
      return;
    brd = bshm->bcache + bno;
    if (brd->btime < 0)		/* 尚未更新 brd->blast 就不強制閱讀公告板 */
      return;

#ifdef ENHANCED_VISIT
    brh_get(brd->bstamp, bno);
    while (brh_unread(brd->blast))
#else
    if (brd->blast > brd_visit[bno])
#endif
    {
      vmsg("有新公告！請先閱\讀完新公告後再離開");
      XoPost(bno);
      xover(XZ_POST);

#ifndef ENHANCED_VISIT
      time(&brd_visit[bno]);
#endif
    }
  }
}
#endif	/* HAVE_FORCE_BOARD */


/* ----------------------------------------------------- */
/* Class [分類群組]					 */
/* ----------------------------------------------------- */


#ifdef AUTO_JUMPBRD
static int class_jumpnext = 0;	/* itoc.010910: 是否跳去下一個未讀板 1:要 0:不要 */
#endif


static int class_bfo = 0;

#define	BFO_YANK	0x01


static int
class_load(xo)
  XO *xo;
{
  short *cbase, *chead, *ctail;
  int chn;			/* ClassHeader number */
  int pos, max, val, zap;
  BRD *brd;
  char *bits;

  /* lkchu.990106: 防止 未用 account 造出 class.img 或沒有 class 的情況 */
  if (!class_img)
    return 0;

  chn = CH_END - xo->key;

  cbase = (short *) class_img;
  chead = cbase + chn;

  pos = chead[0] + CH_TTLEN;
  max = chead[1];

  chead = (short *) ((char *) cbase + pos);
  ctail = (short *) ((char *) cbase + max);

  max -= pos;

  if (cbase = (short *) xo->xyz)
    cbase = (short *) realloc(cbase, max);
  else
    cbase = (short *) malloc(max);

  xo->xyz = (char *) cbase;

  max = 0;
  brd = bshm->bcache;
  bits = brd_bits;
  zap = (class_bfo & BFO_YANK) ? 0 : BRD_Z_BIT;

  do
  {
    chn = *chead++;
    if (chn >= 0)	/* 一般看板 */
    {
      val = bits[chn];
      if (!(val & BRD_L_BIT) || (val & zap) || !(brd[chn].brdname[0]))
	continue;
    }
    else		/* 分類群組 */
    {
      if (class_bits[-chn] & zap)
	continue;
    }

    max++;
    *cbase++ = chn;
  } while (chead < ctail);

  xo->max = max;
  if (xo->pos >= max)
    xo->pos = xo->top = 0;

  return max;
}


static int
XoClass(chn)
  int chn;
{
  XO xo, *xt;

  /* Thor.980727: 解決 XO xo的不確定性, 
                  class_load內部會 initial xo.max, 其他不確定 */
  xo.pos = xo.top = 0;

  xo.key = chn;
  xo.xyz = NULL;
  if (!class_load(&xo))
  {
    if (xo.xyz)
      free(xo.xyz);
    return 0;
  }

  xt = xz[XZ_CLASS - XO_ZONE].xo;
  xz[XZ_CLASS - XO_ZONE].xo = &xo;

#ifdef AUTO_JUMPBRD
  if (cuser.ufo & UFO_JUMPBRD)
    class_jumpnext = 1;	/* itoc.010910: 主動跳去下一個未讀看板 */
#endif
  xover(XZ_CLASS);

  free(xo.xyz);
  xz[XZ_CLASS - XO_ZONE].xo = xt;

  return 1;
}


static inline void
btime_refresh(brd)
  BRD *brd;
{
  /* itoc.020123: 不管有無 UFO_BRDPOST，一概更新，以免未讀燈不會亮 */
  if (brd->btime < 0)
  {
    int fd, fsize;
    char folder[64];
    struct stat st;

    brd->btime = 1;
    brd_fpath(folder, brd->brdname, fn_dir);
    if ((fd = open(folder, O_RDONLY)) >= 0)
    {
      if (!fstat(fd, &st) && (fsize = st.st_size) >= sizeof(HDR))
      {
#ifdef ENHANCED_BSHM_UPDATE
	HDR hdr;

	brd->bpost = fsize / sizeof(HDR);
	/* itoc.020829: 找最後一篇未被加密、不是置底的 HDR */
	while ((fsize -= sizeof(HDR)) >= 0)
	{
	  lseek(fd, fsize, SEEK_SET);
	  read(fd, &hdr, sizeof(HDR));
	  if (!(hdr.xmode & (POST_RESTRICT | POST_BOTTOM)))
	    break;
	}
	brd->blast = hdr.chrono;
#else
	brd->bpost = fsize / sizeof(HDR);
	lseek(fd, fsize - sizeof(HDR), SEEK_SET);
	read(fd, &brd->blast, sizeof(time_t));
#endif
      }
      else
      {
	brd->blast = brd->bpost = 0;
      }
      close(fd);
    }
  }
}


void
class_item(num, bno, brdpost)
  int num, bno, brdpost;
{
  BRD *brd;
  char *str1, *str2, *str3, token, buf[16];

  brd = bshm->bcache + bno;

  btime_refresh(brd);

  /* 處理 編號/篇數 */
  if (brdpost)
    num = brd->bpost;

  /* 處理 zap/friend/secret 板的符號 */
  if (brd_bits[bno] & BRD_Z_BIT)
    token = TOKEN_ZAP_BRD;
  else if (brd->readlevel == PERM_SYSOP)
    token = TOKEN_SECRET_BRD;
  else if (brd->readlevel == PERM_BOARD)
    token = TOKEN_FRIEND_BRD;
  else
    token = ' ';

  /* 處理 已讀/未讀 */
#ifdef ENHANCED_VISIT
  /* itoc.010407: 改用最後一篇已讀/未讀來判斷 */
  brh_get(brd->bstamp, bno);
  str1 = brh_unread(brd->blast) ? ICON_UNREAD_BRD : ICON_READ_BRD;
#else
  str1 = brd->blast > brd_visit[bno] ? ICON_UNREAD_BRD : ICON_READ_BRD;
#endif

  /* 處理 投票/轉信 */
  if (brd->bvote)
    str2 = (brd->bvote > 0) ? ICON_VOTED_BRD : ICON_GAMBLED_BRD;
  else
    str2 = (brd->battr & BRD_NOTRAN) ? ICON_NOTRAN_BRD : ICON_TRAN_BRD;

  /* 處理 人氣 */
  bno = bshm->mantime[bno];
  if (bno > 99)
    str3 = "\033[1;31m爆\033[m";
  else if (bno > 0)
    sprintf(str3 = buf, "%2d", bno);
  else
    str3 = "  ";

  prints("%6d%c%s%-13s\033[1;3%dm%-5s\033[m%s ",
    num, token, str1, brd->brdname,
    brd->class[3] & 7, brd->class, str2);
  /* itoc.060530: 借用 str1、num 來處理看板敘述顯示的中文斷字 */
  str1 = brd->title;
  num = (d_cols >> 1) + 33;
  prints("%-*.*s", num, IS_ZHC_LO(str1, num - 1) ? num - 2 : num - 1, str1);
  prints("%s %.*s\n", str3, d_cols - (d_cols >> 1) + 12, brd->BM);
}


static int
class_body(xo)
  XO *xo;
{
  short *chp;
  BRD *bcache;
  int n, cnt, max, chn, brdpost;
#ifdef AUTO_JUMPBRD
  int nextpos;
#endif

  bcache = bshm->bcache;
  max = xo->max;
  cnt = xo->top;

#ifdef AUTO_JUMPBRD
  nextpos = 0;

  /* itoc.010910: 搜尋下一個未讀看板 */
  if (class_jumpnext)
  {
    class_jumpnext = 0;
    n = xo->pos;
    chp = (short *) xo->xyz + n;

    while (n < max)
    {
      chn = *chp++;
      if (chn >= 0)
      {
	BRD *brd;

	brd = bcache + chn;

#ifdef ENHANCED_VISIT
	/* itoc.010407: 改用最後一篇已讀/未讀來判斷 */
	brh_get(brd->bstamp, chn);
	if (brh_unread(brd->blast))
#else
	if (brd->blast > brd_visit[chn])
#endif
	{
	  nextpos = n;
	  break;
	}
      }
      n++;
    }

    /* 下一個未讀板在別頁，要翻過去 */
    if (nextpos >= cnt + XO_TALL)
      return nextpos + XO_MOVE;
  }
#endif

  brdpost = cuser.ufo & UFO_BRDPOST;
  chp = (short *) xo->xyz + cnt;

  n = 3;
  move(3, 0);
  do
  {
    chn = *chp;
    if (cnt < max)
    {
      clrtoeol();
      cnt++;
      if (chn >= 0)		/* 一般看板 */
      {
	class_item(cnt, chn, brdpost);
      }
      else			/* 分類群組 */
      {
	short *chx;
	char *img, *str;

	img = class_img;
	chx = (short *) img + (CH_END - chn);
	str = img + *chx;
	prints("%6d%c  %-13.13s\033[1;3%dm%-5.5s\033[m%s\n", 
	  cnt, class_bits[-chn] & BRD_Z_BIT ? TOKEN_ZAP_BRD : ' ', 
	  str, str[BNLEN + 4] & 7, str + BNLEN + 1, str + BNLEN + 1 + BCLEN + 1);
      }
      chp++;
    }
    else
    {
      clrtobot();
      break;
    }
  } while (++n < b_lines);

#ifdef AUTO_JUMPBRD
  /* itoc.010910: 下一個未讀板在本頁，要把游標移過去 */
  outf(FEETER_CLASS);   
  return nextpos ? nextpos + XO_MOVE : XO_NONE;
#else
  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
#endif
}


static int
class_neck(xo)
  XO *xo;
{
  move(1, 0);
  prints(NECKER_CLASS, 
    cuser.ufo & UFO_BRDPOST ? "總數" : "編號", 
    d_cols >> 1, "", d_cols - (d_cols >> 1), "");
  return class_body(xo);
}


static int
class_head(xo)
  XO *xo;
{
  vs_head("看板列表", str_site);
  return class_neck(xo);
}


static int
class_init(xo)			/* re-init */
  XO *xo;
{
  class_load(xo);
  return class_head(xo);
}


static int
class_postmode(xo)
  XO *xo;
{
  cuser.ufo ^= UFO_BRDPOST;
  cutmp->ufo = cuser.ufo;
  return class_neck(xo);
}


static int
class_namemode(xo)		/* itoc.010413: 看板依照字母/分類排列 */
  XO *xo;
{
  static time_t last = 0;
  time_t now;
 
  if (time(&now) - last < 10)
  {
    vmsg("每十秒鐘只能切換一次");
    return XO_FOOT;
  }
  last = now;

  if (cuser.userlevel)
    brh_save();			/* itoc.010711: 儲存閱讀記錄檔 */
  cuser.ufo ^= UFO_BRDNAME;
  cutmp->ufo = cuser.ufo;
  board_main();			/* 重新載入 class_img */
  return class_neck(xo);
}


static int
class_help(xo)
  XO *xo;
{
  xo_help("class");
  return class_head(xo);
}


static int
class_search(xo)
  XO *xo;
{
  int num, pos, max;
  char buf[BNLEN + 1];

  if (vget(b_lines, 0, MSG_BID, buf, BNLEN + 1, DOECHO))
  {
    short *chp, chn;
    BRD *bcache, *brd;

    str_lowest(buf, buf);

    bcache = bshm->bcache;
    pos = num = xo->pos;
    max = xo->max;
    chp = (short *) xo->xyz;

    do
    {
      if (++pos >= max)
	pos = 0;
      chn = chp[pos];
      if (chn >= 0)
      {
	brd = bcache + chn;
	if (str_str(brd->brdname, buf) || str_sub(brd->title, buf))
	{
	  outf(FEETER_CLASS);	/* itoc.010913: 把 b_lines 填上 feeter */
	  return pos + XO_MOVE;
	}
      }
    } while (pos != num);
  }

  return XO_FOOT;
}


static int
class_searchBM(xo)
  XO *xo;
{
  int num, pos, max;
  char buf[IDLEN + 1];

  if (vget(b_lines, 0, "請輸入板主：", buf, IDLEN + 1, DOECHO))
  {
    short *chp, chn;
    BRD *bcache, *brd;

    str_lower(buf, buf);

    bcache = bshm->bcache;
    pos = num = xo->pos;
    max = xo->max;
    chp = (short *) xo->xyz;

    do
    {
      if (++pos >= max)
	pos = 0;
      chn = chp[pos];
      if (chn >= 0)
      {
	brd = bcache + chn;
	if (str_str(brd->BM, buf))
	{
	  outf(FEETER_CLASS);	/* itoc.010913: 把 b_lines 填上 feeter */
	  return pos + XO_MOVE;
	}
      }
    } while (pos != num);
  }

  return XO_FOOT;
}


static int
class_yank(xo)
  XO *xo;
{
  /* itoc.001029: 所有的class,board列表下, key < 0, 1 則為找尋作者模式
                  使其不能跑 XO_INIT(裏面的class_load), 如 class_yank,
                  除了防止找出的作者看板列表消失, 也防踢人  */
  if (xo->key >= 0)
    return XO_NONE;
    
  class_bfo ^= BFO_YANK;
  return class_init(xo);
}


static int
class_zap(xo)
  XO *xo;
{
  BRD *brd;
  short *chp;
  int pos, num, chn;
  char token;

  pos = xo->pos;
  chp = (short *) xo->xyz + pos;
  chn = *chp;
  if (chn >= 0)		/* 一般看板 */
  {
    brd = bshm->bcache + chn;
    if (!(brd->battr & BRD_NOZAP))
    {
      /* itoc.010909: 要隨 class_item() 版面變 */
      move(3 + pos - xo->top, 6);
      num = brd_bits[chn] ^= BRD_Z_BIT;

      /* 處理 zap/friend/secret 板的符號 */
      if (num & BRD_Z_BIT)
	token = TOKEN_ZAP_BRD;
      else if (brd->readlevel == PERM_SYSOP)
	token = TOKEN_SECRET_BRD;
      else if (brd->readlevel == PERM_BOARD)
	token = TOKEN_FRIEND_BRD;
      else
	token = ' ';

      outc(token);
    }
  }
  else			/* 分類群組 */
  {
    short *chx;
    char *img, brdname[BNLEN + 1];

    /* itoc.010909: 要隨 class_body() 版面變 */
    move(3 + pos - xo->top, 6);
    num = class_bits[-chn] ^= BRD_Z_BIT;
    outc(num & BRD_Z_BIT ? TOKEN_ZAP_BRD : ' ');

    img = class_img;
    chx = (short *) img + (CH_END - chn);
    str_ncpy(brdname, img + *chx, BNLEN + 1);
    czh_put(brdname);
  }

  /* return XO_NONE; */
  return XO_MOVE + pos + 1;	/* itoc.020219: 跳至下一項 */
}


static int
class_zapall(xo)
  XO *xo;
{
  BRD *brdp, *bend;
  int ans, bno;

  ans = vans("設定所有看板 (U)訂閱\ (Z)不訂閱\ (Q)取消？ [Q] ");
  if (ans != 'z' && ans != 'u')
    return XO_FOOT;

  brdp = bshm->bcache;
  bend = brdp + bshm->number;
  bno = 0;
  do
  {
    if (ans == 'z')
    {
      if (!(brdp->battr & BRD_NOZAP))
	brd_bits[bno] |= BRD_Z_BIT;
    }
    else
    {
      brd_bits[bno] &= ~BRD_Z_BIT;
    }

    bno++;
  } while (++brdp < bend);

  class_bfo |= BFO_YANK;	/* 強迫 yank 起來看結果 */
  return class_init(xo);
}


static int
class_visit(xo)		/* itoc.010128: 看板列表設定看板已讀 */
  XO *xo;
{
  short *chp;
  int chn;

  chp = (short *) xo->xyz + xo->pos;
  chn = *chp;
  if (chn >= 0)
  {
    BRD *brd;
    brd = bshm->bcache + chn;
    brh_get(brd->bstamp, chn);
    brh_visit(0);
#ifndef ENHANCED_VISIT
    time(&brd_visit[chn]);
#endif
  }
  return class_body(xo);
}


static int
class_unvisit(xo)		/* itoc.010129: 看板列表設定看板未讀 */
  XO *xo;
{
  short *chp;
  int chn;

  chp = (short *) xo->xyz + xo->pos;
  chn = *chp;
  if (chn >= 0)
  {
    BRD *brd;
    brd = bshm->bcache + chn;
    brh_get(brd->bstamp, chn);
    brh_visit(1);
#ifndef ENHANCED_VISIT
    brd_visit[chn] = 0;	/* itoc.010402: 最近瀏覽時間歸零，使看板列表中顯示未讀 */
#endif
  }
  return class_body(xo);
}


static int
class_nextunread(xo)
  XO *xo;
{
  int max, pos, chn;
  short *chp;
  BRD *bcache, *brd;

  bcache = bshm->bcache;
  max = xo->max;
  pos = xo->pos;
  chp = (short *) xo->xyz + pos;

  while (++pos < max)
  {
    chn = *(++chp);
    if (chn >= 0 && !(brd_bits[chn] & BRD_Z_BIT))	/* 跳過分類及 zap 掉的看板 */
    {
      brd = bcache + chn;

#ifdef ENHANCED_VISIT
      /* itoc.010407: 改用最後一篇已讀/未讀來判斷 */
      brh_get(brd->bstamp, chn);
      if (brh_unread(brd->blast))
#else
      if (brd->blast > brd_visit[chn])
#endif
	return pos + XO_MOVE;
    }
  }

  return XO_NONE;
}


static int
class_edit(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_ALLBOARD | PERM_BM))
  {
    short *chp;
    int chn;

    chp = (short *) xo->xyz + xo->pos;
    chn = *chp;
    if (chn >= 0)
    {
      if (!HAS_PERM(PERM_ALLBOARD))
	brd_title(chn);		/* itoc.000312: 板主修改中文敘述 */
      else
	brd_edit(chn);
      return class_init(xo);
    }
  }
  return XO_NONE;
}


static int
hdr_cmp(a, b)
  HDR *a;
  HDR *b;
{
  /* 先比對分類，再比對板名 */
  int k = strncmp(a->title + BNLEN + 1, b->title + BNLEN + 1, BCLEN);
  return k ? k : str_cmp(a->xname, b->xname);
}


static int
class_newbrd(xo)
  XO *xo;
{
  BRD newboard;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  memset(&newboard, 0, sizeof(BRD));

  /* itoc.010211: 新看板預設 postlevel = PERM_POST; battr = 不轉信 */
  newboard.postlevel = PERM_POST;
  newboard.battr = BRD_NOTRAN;

  if (brd_new(&newboard) < 0)
    return class_head(xo);

  if (xo->key < CH_END)		/* 在分類群組裡面 */
  {
    short *chx;
    char *img, *str;
    char xname[BNLEN + 1], fpath[64];
    HDR hdr;

    img = class_img;
    chx = (short *) img + (CH_END - xo->key);
    str = img + *chx;

    str_ncpy(xname, str, sizeof(xname));
    if (str = strchr(xname, '/'))
      *str = '\0';

    /* 加入分類群組 */
    sprintf(fpath, "gem/@/@%s", xname);
    brd2gem(&newboard, &hdr);
    rec_add(fpath, &hdr, sizeof(HDR));
    rec_sync(fpath, sizeof(HDR), hdr_cmp, NULL);

    vmsg("新板成立");
  }
  else				/* 在看板列表裡面 */
  {
    vmsg("新板成立，記著加入分類群組");
  }

  return class_init(xo);
}


static int
class_browse(xo)
  XO *xo;
{
  short *chp;
  int chn;

  chp = (short *) xo->xyz + xo->pos;
  chn = *chp;
  if (chn < 0)		/* 進入分類 */
  {
    if (!XoClass(chn))
      return XO_NONE;
  }
  else			/* 進入看板 */
  {
    if (XoPost(chn))	/* 無法閱讀該板 */
      return XO_FOOT;
    xover(XZ_POST);
#ifndef ENHANCED_VISIT
    time(&brd_visit[chn]);
#endif
  }

#ifdef AUTO_JUMPBRD
  if (cuser.ufo & UFO_JUMPBRD)
    class_jumpnext = 1;		/* itoc.010910: 只有在離開看板回到看板列表時才需要跳去下一個未讀看板 */
#endif

  return class_head(xo);	/* Thor.980701: 無法清少一點, 因為 XoPost */
}


int
Select()
{
  int bno;
  BRD *brd;
  char bname[BNLEN + 1];

  if (brd = ask_board(bname, BRD_R_BIT, NULL))
  {
    bno = brd - bshm->bcache;
    XoPost(bno);
    xover(XZ_POST);
#ifndef ENHANCED_VISIT
    time(&brd_visit[bno]);
#endif
  }
  else
  {
    vmsg(err_bid);
  }

  return 0;
}


static int
class_switch(xo)
  XO *xo;
{
  Select();
  return class_head(xo);
}


#ifdef MY_FAVORITE

/* ----------------------------------------------------- */
/* MyFavorite [我的最愛]				 */
/* ----------------------------------------------------- */


static inline int
in_favor(brdname)
  char *brdname;
{
  MF mf;
  int fd;
  int rc = 0;
  char fpath[64];

  if (brdname[0])
  {
    mf_fpath(fpath, cuser.userid, FN_MF);

    if ((fd = open(fpath, O_RDONLY)) >= 0)
    {
      while (read(fd, &mf, sizeof(MF)) == sizeof(MF))
      {
	if (!strcmp(brdname, mf.xname))
	{
	  rc = 1;
	  break;
	}
      }
    }
    close(fd);
  }
  return rc;
}


static int 
class_addMF(xo)
  XO *xo;  
{    
  short *chp;
  int chn;
  MF mf;
  char fpath[64];

  if (!cuser.userlevel)
    return XO_NONE;
  
  chp = (short *) xo->xyz + xo->pos;
  chn = *chp;
      
  if (chn >= 0)		/* 一般看板 */
  {
    BRD *bhdr;

    bhdr = bshm->bcache + chn;

    if (!in_favor(bhdr->brdname))
    {
      memset(&mf, 0, sizeof(MF));
      time(&mf.chrono);
      mf.mftype = MF_BOARD;
      strcpy(mf.xname, bhdr->brdname);

      mf_fpath(fpath, cuser.userid, FN_MF);
      rec_add(fpath, &mf, sizeof(MF));
      vmsg("已將此看板加入我的最愛");
    }
    else
    {
      vmsg("此看板已在最愛中。若要重覆加入，請進我的最愛裡新增");
    }
  }
  else			/* 分類群組 */
  {
    short *chx;
    char *img, *str, *ptr;

    img = class_img;
    chx = (short *) img + (CH_END - chn);
    str = img + *chx;

    memset(&mf, 0, sizeof(MF));
    time(&mf.chrono);
    mf.mftype = MF_CLASS;
    ptr = strchr(str, '/');
    strncpy(mf.xname, str, ptr - str);
    strncpy(mf.class, str + BNLEN + 1, BCLEN);
    strcpy(mf.title, str + BNLEN + 1 + BCLEN + 1);

    mf_fpath(fpath, cuser.userid, FN_MF);
    rec_add(fpath, &mf, sizeof(MF));
    vmsg("已將此分類加入我的最愛");
  }

  return XO_FOOT;
}


int
MFclass_browse(name)
  char *name;
{
  int chn, min_chn, len;
  short *chx;
  char *img, cname[BNLEN + 2];

  min_chn = bshm->min_chn;
  img = class_img;

  sprintf(cname, "%s/", name);
  len = strlen(cname);

  for (chn = CH_END - 2; chn >= min_chn; chn--)
  {
    chx = (short *) img + (CH_END - chn);
    if (!strncmp(img + *chx, cname, len))
    {
      if (XoClass(chn))
	return 1;
      break;
    }
  }
  return 0;
}
  
#endif  /* MY_FAVORITE */


#ifdef AUTHOR_EXTRACTION
/* Thor.980818: 想改成以目前的看板列表或分類來找, 不要找全部 */


/* opus.1127 : 計畫重寫, 可 extract author/title */


static int
XoAuthor(xo)
  XO *xo;
{
  int chn, len, max, tag, value;
  short *chp, *chead, *ctail;
  BRD *brd;
  char key[30], author[IDLEN + 1];
  XO xo_a, *xoTmp;
  struct timeval tv = {0, 10};

  vget(b_lines, 0, MSG_XYPOST1, key, 30, DOECHO);
  vget(b_lines, 0, MSG_XYPOST2, author, IDLEN + 1, DOECHO);

  if (!*key && !*author)
    return XO_FOOT;

  str_lowest(key, key);
  str_lower(author, author);
  len = strlen(author);

  chead = (short *) xo->xyz;
  max = xo->max;
  ctail = chead + max;

  tag = 0;
  chp = (short *) malloc(max * sizeof(short));
  brd = bshm->bcache;

  do
  {
    if ((chn = *chead++) >= 0)	/* Thor.980818: 不為 group */
    {
      /* Thor.980701: 尋找指定作者文章, 有則移位置, 並放入 */

      int fsize;
      char *fimage;

      char folder[80];
      HDR *head, *tail;

      sprintf(folder, "《尋找指定標題作者》看板：%s \033[5m...\033[m按任意鍵中斷", brd[chn].brdname);
      outz(folder);
      refresh();

      brd_fpath(folder, brd[chn].brdname, fn_dir);
      fimage = f_map(folder, &fsize);

      if (fimage == (char *) -1)
	continue;

      head = (HDR *) fimage;
      tail = (HDR *) (fimage + fsize);

      while (head <= --tail)
      {
	if ((!*key || str_sub(tail->title, key)) &&
	  (!len || !str_ncmp(tail->owner, author, len)))
	{
	  xo_get(folder)->pos = tail - head;
	  chp[tag++] = chn;
	  break;
	}
      }

      munmap(fimage, fsize);
    }

    /* 使用者可以中斷搜尋 */
    value = 1;
    if (select(1, (fd_set *) &value, NULL, NULL, &tv) > 0)
    {
      vkey();
      break;
    }
  } while (chead < ctail);

  if (!tag)
  {
    free(chp);
    vmsg("空無一物");
    return XO_FOOT;
  }

  xo_a.pos = xo_a.top = 0;
  xo_a.max = tag;
  xo_a.key = 1;			/* all boards */
  /* Thor.990621: 所有的class,board列表下, key < 0, 以 1 與正常模式區分
                  使其不能跑 XO_INIT(裏面的class_load), 如 class_yank,
                  除了防止找出的作者看板列表消失, 也防踢人 */ 
  xo_a.xyz = (char *) chp;

  xoTmp = xz[XZ_CLASS - XO_ZONE].xo;	/* Thor.980701: 記下原來的class_xo */
  xz[XZ_CLASS - XO_ZONE].xo = &xo_a;

#ifdef AUTO_JUMPBRD
  if (cuser.ufo & UFO_JUMPBRD)
    class_jumpnext = 1;	/* itoc.010910: 主動跳去下一個未讀看板 */
#endif
  xover(XZ_CLASS);

  free(chp);
  xz[XZ_CLASS - XO_ZONE].xo = xoTmp;	/* Thor.980701: 還原 class_xo */

  return class_body(xo);
}
#endif


static KeyFunc class_cb[] =
{
  XO_INIT, class_head,
  XO_LOAD, class_body,
  XO_HEAD, class_head,
  XO_BODY, class_body,

  'r', class_browse,
  '/', class_search,
  '?', class_searchBM,
  's', class_switch,
  'c', class_postmode,
  'S', class_namemode,

  'y', class_yank,
  'z', class_zap,
  'Z', class_zapall,
  'v', class_visit,
  'V', class_unvisit,
  '`', class_nextunread,
  'E', class_edit,

#ifdef AUTHOR_EXTRACTION
  'A', XoAuthor,
#endif

#ifdef MY_FAVORITE
  'a', class_addMF,
  'f', class_addMF,
#endif

  Ctrl('P'), class_newbrd,

  'h', class_help
};


int
Class()
{
  /* XoClass(CH_END - 1); */
  /* Thor.980804: 防止 未用 account 造出 class.img 或沒有 class 的情況 */
  if (!class_img || !XoClass(CH_END - 1))
  {
    vmsg("未定義分組討論區");
    return XEASY;
  }
  return 0;
}


void
board_main()
{
  int fsize;

  brh_load();

  if (class_img)	/* itoc.030416: 第二次進入 board_main 時，要 free 掉 class_img */
  {
    free(class_img);
  }
  else			/* itoc.040102: 第一次進入 board_main 時，才需要初始化 class_bfo */
  {
    if (!cuser.userlevel)			/* guest yank all boards */
      class_bfo |= BFO_YANK;

    /* 設定 default board */
    strcpy(currboard, BN_NULL);
    currbno = -1;
  }

  /* class_img = f_img(CLASS_IMGFILE, &fsize); */
  /* itoc.010413: 依照 ufo 來載入不同的 class image */
  class_img = f_img(cuser.ufo & UFO_BRDNAME ? CLASS_IMGFILE_NAME : CLASS_IMGFILE_TITLE, &fsize);

  if (class_img == NULL)
    blog("CACHE", "class.img");
  else
    czh_load();

  board_xo.key = CH_END;
  class_load(&board_xo);

  xz[XZ_CLASS - XO_ZONE].xo = &board_xo;	/* Thor: default class_xo */
  xz[XZ_CLASS - XO_ZONE].cb = class_cb;		/* Thor: default class_xo */
}


int
Boards()
{
  /* class_xo = &board_xo; *//* Thor: 已有 default, 不需作此 */

#ifdef AUTO_JUMPBRD
  if (cuser.ufo & UFO_JUMPBRD)
    class_jumpnext = 1;	/* itoc.010910: 主動跳去下一個未讀看板 */
#endif
  xover(XZ_CLASS);

  return 0;
}
