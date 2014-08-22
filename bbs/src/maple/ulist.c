/*-------------------------------------------------------*/
/* ulist.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : ulist routines	 			 */
/* create : 95/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern UCACHE *ushm;
extern XZ xz[];


/*-------------------------------------------------------*/
/* 選單式聊天介面					 */
/*-------------------------------------------------------*/


static int pickup_ship = 0;	/* 0:故鄉 !=0:友誼敘述 */

typedef UTMP *pickup;

/* 此順序即為排序的順序 */
#define FTYPE_SELF	0x01
#define FTYPE_BOTHGOOD	0x02
#define FTYPE_MYGOOD	0x04
#define FTYPE_OGOOD	0x08
#define FTYPE_NORMAL	0x10
#define FTYPE_MYBAD	0x20

static int mygood_num;		/* 對方設我為好友 */
static int ogood_num;		/* 我設對方為好友 */

static pickup ulist_pool[MAXACTIVE];
/* static */ int ulist_userno[MAXACTIVE];	/* 對應 ushm 中各欄的 userno */
static int ulist_ftype[MAXACTIVE];		/* 對應 ushm 中各欄的朋友種類 */

static int ulist_init();
static int ulist_head();
static XO ulist_xo;


#if 0
static char *
pal_ship(ftype, userno)	/* itoc.020811: 傳回朋友敘述 */
  int ftype, userno;
{
  int fd;
  PAL *pal;
  char fpath[64];
  static char palship[46];

  if (ftype & (FTYPE_BOTHGOOD | FTYPE_MYGOOD | FTYPE_MYBAD))	/* 互設好友、我的好友、壞人才有友誼敘述 */
  {
    usr_fpath(fpath, cuser.userid, fn_pal);
    if ((fd = open(fpath, O_RDONLY)) >= 0)
    {
      mgets(-1);
      while (pal = mread(fd, sizeof(PAL)))
      {
	if (userno == pal->userno)
	{
	  strcpy(palship, pal->ship);
	  close(fd);
	  return palship;
	}
      }
      close(fd);
    }
  }
  return "";
}
#endif


#if 1	/* itoc.020901: 用 cache 雖然比較好，但是蠻浪費記憶體的 */
typedef struct
{
  int userno;
  char ship[20];	/* 不需要和 PAL.ship 一樣大，只要夠 ulist_body() 顯示即可 */
}	PALSHIP;

        
static char *
pal_ship(ftype, userno)	/* itoc.020811: 傳回朋友敘述 */
  int ftype, userno;
{
  static PALSHIP palship[PAL_MAX] = {0};
  PALSHIP *pp;

  /* itoc.020901: 把 palship 收進記憶體，不要一直 I/O 了 */
  if (!palship[0].userno)	/* initialize *palship[] */
  {
    int fd;
    char fpath[64];
    PAL *pal;

    /* 為求效率，每次上站僅做一次，故若改變朋友敘述，要重新上站才生效 */
    usr_fpath(fpath, cuser.userid, fn_pal);
    if ((fd = open(fpath, O_RDONLY)) >= 0)
    {
      pp = palship;
      mgets(-1);
      while (pal = mread(fd, sizeof(PAL)))
      {
	if (pal->ship[0])	/* 有友誼才收入 palship[] */
	{
	  pp->userno = pal->userno;
	  str_ncpy(pp->ship, pal->ship, sizeof(pp->ship));
	  pp++;
	}
      }
      close(fd);
    }    
  }

  if (ftype & (FTYPE_BOTHGOOD | FTYPE_MYGOOD | FTYPE_MYBAD))	/* 互設好友、我的好友、壞人才有友誼敘述 */
  {
    /* 經 pal_sync 以後的朋友名單是依 ID 排序的，考慮是否用 binary search? */
    pp = palship;
    while (pp->userno)
    {
      if (pp->userno == userno)
	return pp->ship;
      pp++;
    }
  }
  return "";
}
#endif


static void
ulist_item(num, up, slot, now, sysop)
  int num;
  UTMP *up;
  int slot;
  time_t now;
  int sysop;
{
  time_t diff, ftype;
  int userno, ufo;
  char pager, buf[32], *fcolor;

  if (!(userno = up->userno))
  {
    outs("      < 此位網友正巧離開 >\n");
    return;
  }

  /* itoc.011022: 若生日當天上站，借用 idle 欄位來放壽星 */
  if (up->status & STATUS_BIRTHDAY)
  {
    strcpy(buf, "\033[1;31m壽星\033[m");
  }
  else
  {
#ifdef DETAIL_IDLETIME
    if ((diff = now - up->idle_time) >= 60)	/* 超過 60 秒才算閒置 */
      sprintf(buf, "%3d'%02d", diff / 60, diff % 60);
#else
    if (diff = up->idle_time)
      sprintf(buf, "%2d", diff);
#endif
    else
      buf[0] = '\0';
  }

  ufo = up->ufo;

  /*         pager 狀態                       */
  /*  #：不接受任何人呼叫，也不接受任何人廣播 */
  /*  *：只接受好友呼叫，且只接受好友廣播     */
  /*  !：只接受好友呼叫，但不接受任何人廣播   */
  /*  -：接受任何人呼叫，但不接受任何人廣播   */
  /*   ：都沒有就是沒有限制啦                 */
  if (ufo & UFO_QUIET)
  {
    pager = '#';
  }
  else if (ufo & UFO_PAGER)
  {
#ifdef HAVE_NOBROAD
    if (ufo & UFO_RCVER)
      pager = '!';
    else
#endif
      pager = '*';
  }
#ifdef HAVE_NOBROAD
  else if (ufo & UFO_RCVER)
  {
    pager = '-';
  }
#endif
  else
  {
    pager = ' ';
  }

  ftype = ulist_ftype[slot];

  fcolor = 
#ifdef HAVE_BRDMATE
# ifdef HAVE_ANONYMOUS
    up->mode == M_READA && !(currbattr & BRD_ANONYMOUS) && !strcmp(currboard, up->reading) ? COLOR_BRDMATE :
# else
    up->mode == M_READA && !strcmp(currboard, up->reading) ? COLOR_BRDMATE :
#  endif
#endif
    ftype & FTYPE_NORMAL ? COLOR_NORMAL : 
    ftype & FTYPE_BOTHGOOD ? COLOR_BOTHGOOD : 
    ftype & FTYPE_MYGOOD ? COLOR_MYGOOD : 
    ftype & FTYPE_OGOOD ? COLOR_OGOOD : 
    ftype & FTYPE_SELF ? COLOR_SELF : 
    ftype & FTYPE_MYBAD ? COLOR_MYBAD : 
    "";

  prints("%6d%c%c%s%-13s%-*.*s\033[m%-*.*s%-11.10s%s\n",
    num, ufo & UFO_CLOAK ? ')' : ' ', pager, 
    fcolor, up->userid, 
    (d_cols >> 1) + 21, (d_cols >> 1) + 20, up->username, 
    d_cols - (d_cols >> 1) + 19, d_cols - (d_cols >> 1) + 18, 
    pickup_ship ? pal_ship(ftype, up->userno) : 
#ifdef GUEST_WHERE
    (pager == ' ' || sysop || ftype & (FTYPE_SELF | FTYPE_BOTHGOOD | FTYPE_OGOOD) || !up->userlevel) ? 	/* 可看見 guest 的故鄉 */
#else
    (pager == ' ' || sysop || ftype & (FTYPE_SELF | FTYPE_BOTHGOOD | FTYPE_OGOOD)) ?			/* 對方設我為好友可看見對方來源 */
#endif
    up->from : "*", bmode(up, 0), buf);
}


static int
ulist_body(xo)
  XO *xo;
{
  pickup *pp;
  UTMP *up;
  int num, max, tail, sysop, seecloak, slot;
#ifdef HAVE_SUPERCLOAK
  int seesupercloak;
#endif
#ifdef DETAIL_IDLETIME
  time_t now;
#endif

  max = xo->max;
  if (max <= 0)
  {
    if (vans("目前沒有好友上站，要看看其他使用者嗎(Y/N)？[Y] ") != 'n')
    {
      cuser.ufo ^= UFO_PAL;
      cutmp->ufo = cuser.ufo;
      return ulist_init(xo);
    }
    return XO_QUIT;
  }

  num = xo->top;
  pp = &ulist_pool[num];
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  sysop = HAS_PERM(PERM_ALLACCT);
  seecloak = HAS_PERM(PERM_SEECLOAK);
#ifdef HAVE_SUPERCLOAK
  seesupercloak = cuser.ufo & UFO_SUPERCLOAK;
#endif
#ifdef DETAIL_IDLETIME
  time(&now);
#endif

  move(3, 0);
  do
  {
    up = *pp;
    slot = up - ushm->uslot;

    /* itoc.011124: 如果在 ulist_body() 中發現 userno 不合，表示手上這份名單實在太舊了，強迫更新 */
    if (ulist_userno[slot] != up->userno)
      return ulist_init(xo);

#ifdef DETAIL_IDLETIME
    ulist_item(++num, up, slot, now, sysop);
#else
    ulist_item(++num, up, slot, NULL, sysop);
#endif

    pp++;
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
}


static int
ulist_cmp_userid(i, j)
  UTMP **i, **j;
{
  int k = ulist_ftype[(*i) - ushm->uslot] - ulist_ftype[(*j) - ushm->uslot];
  return k ? k : str_cmp((*i)->userid, (*j)->userid);
}


static int
ulist_cmp_host(i, j)
  UTMP **i, **j;
{
  int k = ulist_ftype[(*i) - ushm->uslot] - ulist_ftype[(*j) - ushm->uslot];
  /* return k ? k : (*i)->in_addr - (*j)->in_addr; */
  /* Kyo.050112: in_addr 是 unsigned int (u_long)，直接減會造成 int 誤判 */
  return k ? k : (*i)->in_addr > (*j)->in_addr ? 1 : (*i)->in_addr < (*j)->in_addr ? -1 : 0;
}


static int
ulist_cmp_mode(i, j)
  UTMP **i, **j;
{
  int k = ulist_ftype[(*i) - ushm->uslot] - ulist_ftype[(*j) - ushm->uslot];
  return k ? k : (*i)->mode - (*j)->mode;
}


#ifdef HAVE_BRDMATE
static int
ulist_cmp_brdmate(i, j)
  UTMP **i, **j;
{
#ifdef HAVE_ANONYMOUS
  if (!(currbattr & BRD_ANONYMOUS) || HAS_PERM(PERM_SYSOP))	/* 閱讀匿名板則不列入 */
#endif
  {
    int ibrdmate = (*i)->mode == M_READA && !strcmp(currboard, (*i)->reading);
    int jbrdmate = (*j)->mode == M_READA && !strcmp(currboard, (*j)->reading);

    /* 板伴優先 */
    if (ibrdmate && !jbrdmate)
      return -1;
    if (jbrdmate && !ibrdmate)
      return 1;
  }

  /* 都不是或都是板伴的話，按 ID 排序 */
  return ulist_cmp_userid(i, j);
}
#endif


#ifdef HAVE_BRDMATE
#define PICKUP_WAYS	4
#else
#define PICKUP_WAYS	3
#endif

static int pickup_way = 0;	/* 預設排列方式 0:代號 1:故鄉 2:動態 3:板伴 */


static int (*ulist_cmp[PICKUP_WAYS]) () =
{
  ulist_cmp_userid,
  ulist_cmp_host,
  ulist_cmp_mode,
#ifdef HAVE_BRDMATE
  ulist_cmp_brdmate,
#endif
};


static char *msg_pickup_way[PICKUP_WAYS] =
{
  "網友代號",
  "客途故鄉",
  "網友動態",
#ifdef HAVE_BRDMATE
  "板伴代號",
#endif
};


static int
ulist_paltype(up)		/* 朋友種類 */
  UTMP *up;
{
  const int userno = up->userno;

  if (userno == cuser.userno)
    return FTYPE_SELF;
  if (is_mybad(userno))
    return FTYPE_MYBAD;
  if (is_mygood(userno))
    return is_ogood(up) ? FTYPE_BOTHGOOD : FTYPE_MYGOOD;
  return is_ogood(up) ? FTYPE_OGOOD : FTYPE_NORMAL;
}


#if 0	/* itoc.041001: ulist_init() 註解 */

  1. ushm->uslot 記錄全站的 UTMP (使用者名單資料)，這是讓所有人共用的

  2. 每個人手上，各有以下三份：
     ulist_pool 是 ushm->uslot 的索引，記錄著我可以看到哪些人
     ulist_userno[i] 記錄著 ushm->uslot[i] 這位子坐了誰
     ulist_ftype[i] 記錄著 ushm->uslot[i] 這位子是我的 好友/壞人/一般人

  3. 在 ulist_init() 裡面去 ushm->uslot 瀏覽所有位子，考慮 ushm->uslot[i]
     如果我可以看到這個人，就把它抄進來 ulist_pool
     若 ushm->uslot[i].userno != ulist_userno[i]，表示這個位子坐了一個新上站的人，
     我就去查他是不是我的好友/壞人，並記錄在 ulist_ftype[i]；
     若 ushm->uslot[i].userno == ulist_userno[i]，表示這個位子沒換人，
     我就直接拿 ulist_ftype[i] 來當作朋友種類

  4. 有了 ulist_pool[] 以後，再把 ulist_pool[] 依我想要的方式排序，
     在 ulist_item() 印出來的顏色則是參照 ulist_ftype[]

  5. 若有人將我在他的朋友名單中異動，那他會更動我的 cutmp->status，
     所以當我檢查到 HAS_STATUS(STATUS_PALDIRTY)，我就要重整我的 ulist_ftype[]

#endif


static int
ulist_init(xo)
  XO *xo;
{
  UTMP *up, *uceil;
  pickup *pp;
  int filter, slot, userno, paldirty;

  pp = ulist_pool;
  filter = cuser.ufo & UFO_PAL;
  if (paldirty = HAS_STATUS(STATUS_PALDIRTY))
    cutmp->status ^= STATUS_PALDIRTY;

  slot = 0;
  up = ushm->uslot;
  uceil = (void *) up + ushm->offset;

  mygood_num = ogood_num = 0;

  /* 從 ushm->uslot[] 抄到 ulist_pool[] */
  do
  {
    userno = up->userno;

    if (userno > 0)
    {
      /* 新上站的使用者，看看他是哪一類的朋友；STATUS_PALDIRTY 重整 ulist_ftype[] */
      if (ulist_userno[slot] != userno || paldirty)
      {
	ulist_userno[slot] = userno;
	ulist_ftype[slot] = ulist_paltype(up);
      }

      if (can_see(cutmp, up))
      {
	userno = ulist_ftype[slot];
	if (!filter || userno & (FTYPE_SELF | FTYPE_BOTHGOOD | FTYPE_MYGOOD))
	  *pp++ = up;

	/* 算有幾個好友 */
	if (userno & (FTYPE_BOTHGOOD | FTYPE_MYGOOD))
	  mygood_num++;
	if (userno & (FTYPE_BOTHGOOD | FTYPE_OGOOD))
	  ogood_num++;
      }
    }
    slot++;
  } while (++up <= uceil);

  xo->max = slot = pp - ulist_pool;

  if (xo->pos >= slot)
    xo->pos = xo->top = 0;

  if (slot > 1)
    qsort(ulist_pool, slot, sizeof(pickup), ulist_cmp[pickup_way]);

  /* itoc.010928: 由於 ushm->count 常不對，所以用 total_user 來校正，
     剛上站時就啟始化 total_user 為 ushm->count，此後若使用者沒有來使用者名單，就不更新 total_user */
  if (!filter)
    total_user = slot;

  return ulist_head(xo);
}


static int
ulist_neck(xo)
  XO *xo;
{
  move(1, 0);

  prints("  排列方式：[\033[1m%s/%s\033[m] 站上人數：%d "
    COLOR_MYGOOD " 我的好友：%d " COLOR_OGOOD " 與我為友：%d\033[m", 
    msg_pickup_way[pickup_way], 
    cuser.ufo & UFO_PAL ? "好友" : "全部", 
    total_user, mygood_num, ogood_num);

  prints(NECKER_ULIST, d_cols >> 1, "", d_cols - (d_cols >> 1) + 4, pickup_ship ? "友誼" : "故鄉");
  return ulist_body(xo);
}


static int
ulist_head(xo)
  XO *xo;
{
  vs_head("網友列表", str_site);
  return ulist_neck(xo);
}


static int
ulist_toggle(xo)
  XO *xo;
{
  int ans, max;

#ifdef HAVE_BRDMATE
  ans = vans("排列方式 [1]代號 [2]來源 [3]動態 [4]板伴 ") - '1';
#else
  ans = vans("排列方式 [1]代號 [2]來源 [3]動態 ") - '1';
#endif
  if (ans >= 0 && ans < PICKUP_WAYS && ans != pickup_way)	/* Thor.980705: from 0 .. PICKUP_WAYS-1 */
  {
    pickup_way = ans;
    max = xo->max;

    if (max > 1)
    {
      qsort(ulist_pool, max, sizeof(pickup), ulist_cmp[pickup_way]);
      return ulist_neck(xo);
    }
  }

  return XO_FOOT;
}


static int
ulist_pal(xo)
  XO *xo;
{
  cuser.ufo ^= UFO_PAL;
  cutmp->ufo = cuser.ufo;
  return ulist_init(xo);
}


static int
ulist_search(xo, step)
  XO *xo;
  int step;
{
  int num, pos, max;
  pickup *pp;
  char buf[IDLEN + 1];

  if (vget(b_lines, 0, "請輸入代號或暱稱：", buf, IDLEN + 1, DOECHO))
  {
    str_lowest(buf, buf);
    
    pos = num = xo->pos;
    max = xo->max;
    pp = ulist_pool;
    do
    {
      pos += step;
      if (pos < 0) /* Thor.990124: 假設 max 不為0 */
	 pos = max - 1;
      else if (pos >= max)
	pos = 0;

      if (str_str(pp[pos]->userid, buf) ||	/* lkchu.990127: 找部份 id 好像比較好用 :p */
	str_sub(pp[pos]->username, buf)) 	/* Thor.990124: 可以找 部分 nickname */
      {
	outf(FEETER_ULIST);	/* itoc.010913: 把 b_lines 填上 feeter */	
	return pos + XO_MOVE;
      }

    } while (pos != num);
  }

  return XO_FOOT;
}


static int
ulist_search_forward(xo)
  XO *xo;
{
  return ulist_search(xo, 1); /* step = +1 */
}


static int
ulist_search_backward(xo)
  XO *xo;
{
  return ulist_search(xo, -1); /* step = -1 */
}


static int
ulist_addpal(xo)
  XO *xo;
{
  if (cuser.userlevel)
  {
    UTMP *up;
    int userno;

    up = ulist_pool[xo->pos];
    userno = up->userno;
    if (userno > 0 && (userno != cuser.userno) &&	/* lkchu.981217: 自己不可為朋友 */
      !is_mygood(userno) && !is_mybad(userno))		/* 尚未列入朋友名單 */
    {
      PAL pal;
      char fpath[64];

      pal_edit(PALTYPE_PAL, &pal, DOECHO);
      pal.userno = userno;
      strcpy(pal.userid, up->userid);
      usr_fpath(fpath, cuser.userid, fn_pal);

      /* itoc.001222: 檢查朋友個數 */
      if (rec_num(fpath, sizeof(PAL)) < PAL_MAX)
      {
	rec_add(fpath, &pal, sizeof(PAL));
	pal_cache();				/* 朋友名單同步 */
	utmp_admset(userno, STATUS_PALDIRTY);
	return ulist_init(xo);
      }
      else
      {
	vmsg("您的朋友名單太多，請善加整理");
	return XO_FOOT;
      }
    }
  }
  return XO_NONE;
}


static int
cmppal(pal)
  PAL *pal;
{
  return pal->userno == currchrono;
}


static int
ulist_delpal(xo)
  XO *xo;
{
  if (cuser.userlevel)
  {
    UTMP *up;
    int userno;

    up = ulist_pool[xo->pos];
    userno = up->userno;
    if (userno > 0 && (is_mygood(userno) || is_mybad(userno)))	/* 在朋友名單中 */
    {
      if (vans(msg_del_ny) == 'y')
      {
	char fpath[64];

	usr_fpath(fpath, cuser.userid, fn_pal);

	currchrono = userno;
	if (!rec_del(fpath, sizeof(PAL), 0, cmppal))
	{
	  pal_cache();				/* 朋友名單同步 */
	  utmp_admset(userno, STATUS_PALDIRTY);
	  return ulist_init(xo);
	}
      }
      return XO_FOOT;
    }
  }
  return XO_NONE;
}


static int
ulist_mail(xo)
  XO *xo;
{
  char userid[IDLEN + 1];

  /* 先複製一份到手上，以免在寫信時 ushm 變動了 */
  strcpy(userid, ulist_pool[xo->pos]->userid);

  if (!*userid)
  {
    vmsg(MSG_USR_LEFT);
    return XO_FOOT;
  }

  return my_send(userid);
}


static int
ulist_query(xo)
  XO *xo;
{
  move(1, 0);
  clrtobot();
  my_query(ulist_pool[xo->pos]->userid);
  return ulist_neck(xo);
}


static int
ulist_broadcast(xo)
  XO *xo;
{
  int num, sysop;
  pickup *pp;
  UTMP *up;
  BMW bmw;

  num = cuser.userlevel;
  sysop = num & PERM_ALLADMIN;
  if (!sysop && (!(num & PERM_PAGE) || !(cuser.ufo & UFO_PAL)))
    return XO_NONE;

  num = xo->max;
  if (num <= 1)		/* 如果只有自己，不能廣播 */
    return XO_NONE;

  /* itoc.030101: 如果站長用的是好友廣播，視同一般 ID 廣播 */
  sysop = sysop && !(cuser.ufo & UFO_PAL);

  bmw.caller = NULL;
  bmw_edit(NULL, "★廣播：", &bmw);

  if (bmw.caller)	/* bmw_edit() 中回答 Yes 要送出廣播 */
  {
    /* itoc.000213: 加 "> " 為了與一般水球區分 */
    sprintf(bmw.userid, "%s> ", cuser.userid);

    pp = ulist_pool;
    while (--num >= 0)
    {
      up = pp[num];

      if (!sysop)
      {
#ifdef HAVE_NOBROAD
	if (up->ufo & UFO_RCVER)
	  continue;
#endif

	/* itoc.011126: 若 up-> 已下站，被其他 user 所取代時，
	   會有廣播誤植的問題，得重新檢查是否為我的好友 */
	if (!is_mygood(up->userno))
	  continue;
      }

      if (can_override(up))
      {
	bmw.recver = up->userno;
	bmw_send(up, &bmw);
      }
    }
  }

  return XO_NONE;
}


static int
ulist_talk(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_PAGE))
  {
    UTMP *up;

    up = ulist_pool[xo->pos];
    if (can_override(up))
      return talk_page(up) ? ulist_head(xo) : XO_FOOT;
  }
  return XO_NONE;
}


static int
ulist_write(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_PAGE))
  {
    UTMP *up;

    if (up = ulist_pool[xo->pos])
      do_write(up);
  }
  return XO_NONE;
}


static int
ulist_edit(xo)			/* Thor: 可線上查看及修改使用者 */
  XO *xo;
{
  ACCT acct;

  if (!HAS_PERM(PERM_ALLACCT) || acct_load(&acct, ulist_pool[xo->pos]->userid) < 0)
    return XO_NONE;

  vs_bar("使用者設定");
  acct_setup(&acct, 1);
  return ulist_head(xo);
}


static int
ulist_kick(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_ALLACCT))
  {
    UTMP *up;
    pid_t pid;
    char buf[80];

    up = ulist_pool[xo->pos];
    if (pid = up->pid)
    {
      if (vans(msg_sure_ny) != 'y' || pid != up->pid)
	return XO_FOOT;

      sprintf(buf, "%s (%s)", up->userid, up->username);

      if ((kill(pid, SIGTERM) == -1) && (errno == ESRCH))
	utmp_free(up);
      else
	sleep(3);		/* 被踢的人這時候正在自我了斷 */

      blog("KICK", buf);
      return ulist_init(xo);
    }
  }
  return XO_NONE;
}


#ifdef HAVE_CHANGE_NICK
static int
ulist_nickchange(xo)
  XO *xo;
{
  char *str, buf[UNLEN + 1];

  if (!cuser.userlevel)
    return XO_NONE;

  strcpy(buf, str = cutmp->username);
  if (vget(b_lines, 0, "請輸入新的暱稱：", buf, UNLEN + 1, GCARRY))
  {  
    if (strcmp(buf, str))
    {
      strcpy(str, buf);
      strcpy(cuser.username, buf);	/* 暱稱也一併更換 cuser. */
      return ulist_body(xo);
    }
  }
  return XO_FOOT;
}
#endif


#ifdef HAVE_CHANGE_FROM
static int
ulist_fromchange(xo)
  XO *xo;
{
  char *str, buf[34];
  
  if (!cuser.userlevel)
    return XO_NONE;
  
  strcpy(buf, str = cutmp->from);
  if (vget(b_lines, 0, "請輸入新的故鄉：", buf, sizeof(cutmp->from), GCARRY))
  {
    if (strcmp(buf, str))
    {
      strcpy(str, buf);
      return ulist_body(xo);
    }
  }

  return XO_FOOT;
}
#endif


#ifdef HAVE_CHANGE_ID
static int
ulist_idchange(xo)
  XO *xo;
{
  char *str, buf[IDLEN + 1];

  /* itoc.010717.註解: 這功能提供站長可以在使用者名單暫時改自己的 ID，
     但是由於 ulist 大部分是用 userno 來判斷，所以只有好看而已 */
  
  if (!HAS_PERM(PERM_ALLADMIN))
    return XO_NONE;
  
  strcpy(buf, str = cutmp->userid);
  if (vget(b_lines, 0, "請輸入新的ＩＤ：", buf, IDLEN + 1, GCARRY))
  {
    if (strcmp(buf, str))
    {
      strcpy(str, buf);
      return ulist_body(xo);
    }
  }

  return XO_FOOT;
}
#endif


static int  
ulist_cloak(xo)			/* itoc.010908: 快速隱身 */
  XO *xo;
{
  if (HAS_PERM(PERM_CLOAK))
  {
    cuser.ufo ^= UFO_CLOAK;
    cutmp->ufo = cuser.ufo;
    return ulist_init(xo);
  }
  return XO_NONE;
}


#ifdef HAVE_SUPERCLOAK
static int
ulist_supercloak(xo)		/* itoc.010908: 快速紫隱 */
  XO *xo;
{
  if (cuser.ufo & UFO_SUPERCLOAK)	/* 取消紫隱，不必管權限 */
  {
    cuser.ufo &= ~(UFO_CLOAK | UFO_SUPERCLOAK);
    cutmp->ufo = cuser.ufo;
    return ulist_init(xo);
  }
  else if (HAS_PERM(PERM_ALLADMIN))	/* 進入紫隱 */
  {
    cuser.ufo |= (UFO_CLOAK | UFO_SUPERCLOAK);
    cutmp->ufo = cuser.ufo;
    return ulist_init(xo);
  }
  return XO_NONE;
}
#endif


static int
ulist_ship(xo)
  XO *xo;
{
  pickup_ship = ~pickup_ship;
  return ulist_neck(xo);
}


static int
ulist_recall(xo)
  XO *xo;
{
  if (cuser.userlevel)
  {
    t_bmw();
    return ulist_head(xo);
  }
  return XO_NONE;
}


static int
ulist_display(xo)
  XO *xo;
{
  if (cuser.userlevel)
  {
    t_display();
    return ulist_head(xo);
  }
  return XO_NONE;
}


static int
ulist_help(xo)
  XO *xo;
{
  xo_help("ulist");
  return ulist_head(xo);
}


static KeyFunc ulist_cb[] =
{
  XO_INIT, ulist_init,
  XO_LOAD, ulist_body,
  XO_HEAD, ulist_head,
  /* XO_BODY, ulist_body, */	/* 沒有用到 */

  'f', ulist_pal,
  'y', ulist_pal,		/* itoc.010205: 有人會把 yank 的意思用在這 */
  'a', ulist_addpal,
  'd', ulist_delpal,
  't', ulist_talk,
  'w', ulist_write,
  'l', ulist_recall,		/* 水球回顧 */
  'L', ulist_display,
  'r', ulist_query,
  'q', ulist_query,		/* itoc.020109: 使用者習慣用 q 查詢 */
  'B', ulist_broadcast,
  's', ulist_init,		/* refresh status Thor: 應user要求 */
  'S', ulist_ship,

  Ctrl('K'), ulist_kick,
  Ctrl('O'), ulist_edit,
  Ctrl('Q'), ulist_query,

#ifdef HAVE_CHANGE_NICK
  Ctrl('N'), ulist_nickchange,
#endif
#ifdef HAVE_CHANGE_FROM
  Ctrl('F'), ulist_fromchange,
#endif
#ifdef HAVE_CHANGE_ID
  Ctrl('D'), ulist_idchange,
#endif

#if 0
  '/', ulist_search,
#endif
  /* Thor.990125: 可前後搜尋, id or nickname */
  '/', ulist_search_forward,
  '?', ulist_search_backward,

  'm', ulist_mail,
  KEY_TAB, ulist_toggle,

  'i', ulist_cloak,
#ifdef HAVE_SUPERCLOAK
  'H', ulist_supercloak,
#endif

  'h', ulist_help
};


void
talk_main()
{
  char fpath[64];

  xz[XZ_ULIST - XO_ZONE].xo = &ulist_xo;
  xz[XZ_ULIST - XO_ZONE].cb = ulist_cb;

  /* itoc.010715: 由於 erevy_Z 可以直接進入 bmw，所以一上站就要載入 */
  usr_fpath(fpath, cuser.userid, fn_bmw);
  xz[XZ_BMW - XO_ZONE].xo = xo_new(fpath);
}
