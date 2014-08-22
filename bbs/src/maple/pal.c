/*-------------------------------------------------------*/
/* pal.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : pal routines	 	 		 */
/* create : 95/03/29				 	 */
/* update : 97/03/29				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern XZ xz[];
extern char xo_pool[];


static int pal_max = 0;		/* 即 cutmp->pal_max，因為太常檢查朋友名單，所以記下來加速 */
static int *pal_pool = NULL;	/* 即 cutmp->pal_spool，因為太常檢查朋友名單，所以記下來加速 */


/* ----------------------------------------------------- */
/* 朋友判別						 */
/* ----------------------------------------------------- */


static int			/* 1: userno 在 pool 名單上 */
belong_pal(pool, max, userno)
  int *pool;
  int max;
  int userno;
{
  int *up, datum, mid;

  up = pool;
  while (max > 0)
  {
    datum = up[mid = max >> 1];
    if (userno == datum)
    {
      return 1;
    }
    if (userno > datum)
    {
      up += (++mid);
      max -= mid;
    }
    else
    {
      max = mid;
    }
  }
  return 0;
}


int
is_mygood(userno)		/*  1: 我設對方為好友 */
  int userno;
{
  return belong_pal(pal_pool, pal_max, userno);
}


int
is_mybad(userno)		/*  1: 我設對方為壞人 */
  int userno;
{
#ifdef HAVE_BADPAL
  return belong_pal(pal_pool, pal_max, -userno);
#else
  return 0;
#endif
}


int
is_ogood(up)			/* 1: 對方設我為好友 */
  UTMP *up;
{
  return belong_pal(up->pal_spool, up->pal_max, cuser.userno);
}


int
is_obad(up)			/* 1: 對方設我為壞人 */
  UTMP *up;
{
#ifdef HAVE_BADPAL
  return belong_pal(up->pal_spool, up->pal_max, -cuser.userno);
#else
  return 0;
#endif
}


#ifdef HAVE_MODERATED_BOARD
int
is_bgood(bpal)			/* 1: 我是該板的板好 */
  BPAL *bpal;
{
  return belong_pal(bpal->pal_spool, bpal->pal_max, cuser.userno);
}


int
is_bbad(bpal)			/* 1: 我是該板的板壞 */
  BPAL *bpal;
{
#ifdef HAVE_BADPAL
  return belong_pal(bpal->pal_spool, bpal->pal_max, -cuser.userno);
#else
  return 0;
#endif
}
#endif


/* ----------------------------------------------------- */
/* 朋友名單：新增、刪除、修改、載入、同步		 */
/* ----------------------------------------------------- */


static int
int_cmp(a, b)
  int *a;
  int *b;
{
  return *a - *b;
}


int
image_pal(fpath, pool)
  char *fpath;
  int *pool;
{
  int fsize;
  int *plist;
  PAL *phead, *ptail;
  char *fimage;

  if (fimage = f_img(fpath, &fsize))
  {
    if (fsize <= PAL_MAX * sizeof(PAL))	/* 如果超過 PAL_MAX，就不載入 */
    {
      plist = pool;
      phead = (PAL *) fimage;
      ptail = (PAL *) (fimage + fsize);
      fsize /= sizeof(PAL);

      do
      {
	/* 若是壞人，存 -userno；若是好友，存 +userno */
	*plist++ = (phead->ftype & PAL_BAD) ? -(phead->userno) : phead->userno;
      } while (++phead < ptail);

      if (fsize > 1)
	qsort(pool, fsize, sizeof(int), int_cmp);
    }
    else
    {
      fsize = 0;
    }
    free(fimage);
  }
  else
  {
    fsize = 0;
  }

  return fsize;
}


void
pal_cache()
{
  char fpath[64];

  if (!pal_pool)	/* 第一次設定，是在 utmp_new() 裡面 */
    pal_pool = cutmp->pal_spool;

  usr_fpath(fpath, cuser.userid, fn_pal);

  pal_max = image_pal(fpath, pal_pool);
  cutmp->pal_max = pal_max;
}


static int
chkpal(pal)
  PAL *pal;
{
  int userno;

  userno = pal->userno;
  return (userno > 0 && userno == acct_userno(pal->userid));
}


void
pal_sync(fpath)
  char *fpath;
{
  int fsize;

  fsize = rec_sync(fpath, sizeof(PAL), str_cmp, chkpal);

  if (fsize > PAL_MAX * sizeof(PAL))
    vmsg(msg_list_over);
}


int
pal_list(reciper)
  int reciper;
{
  int userno, fd;
  char buf[32], fpath[64];
  ACCT acct;
#ifdef HAVE_LIST
  int ch;
#endif

  userno = 0;

  for (;;)
  {
#ifdef HAVE_LIST
    switch (ch = vget(1, 0, "(A)增加 (D)刪除 (F)好友 (G)群組 (M)定案 (1~5)特別名單 (Q)取消？[M] ", buf, 3, LCECHO))
#else
    switch (vget(1, 0, "(A)增加 (D)刪除 (F)好友 (G)群組 (M)定案 (Q)取消？[M] ", buf, 3, LCECHO))
#endif
    {
    case 'a':
      while (acct_get("請輸入代號(只按 ENTER 結束新增): ", &acct) > 0)
      {
	if (!ll_has(acct.userid))
	{
	  ll_add(acct.userid);
	  reciper++;
	  ll_out(3, 0, MSG_LL);
	}
      }
      break;

    case 'd':
      while (reciper)
      {
	if (!vget(1, 0, "請輸入代號(只按 ENTER 結束刪除): ", buf, IDLEN + 1, GET_LIST))
	  break;
	if (ll_del(buf))
	  reciper--;
	ll_out(3, 0, MSG_LL);
      }
      break;

#ifdef HAVE_LIST
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
      /* itoc.010923: 引用特別名單，順便使用群組條件 */
      sprintf(buf, "%s.%c", FN_LIST, ch);
      usr_fpath(fpath, cuser.userid, buf);
#endif

    case 'g':
      if (userno = vget(b_lines, 0, "群組條件：", buf, 16, DOECHO))
	str_lowest(buf, buf);

    case 'f':
#ifdef HAVE_LIST
      if (ch == 'g' || ch == 'f')
#endif
	usr_fpath(fpath, cuser.userid, fn_pal);

      if ((fd = open(fpath, O_RDONLY)) >= 0)
      {
	PAL *pal;
	char *userid;

	mgets(-1);
	while (pal = mread(fd, sizeof(PAL)))
	{
	  userid = pal->userid;
	  if (!ll_has(userid) &&
	    !(pal->ftype & PAL_BAD) &&
	    (!userno || str_sub(pal->ship, buf)))
	  {
	    ll_add(userid);
	    reciper++;
	  }
	}
	close(fd);
      }
      ll_out(3, 0, MSG_LL);
      userno = 0;
      break;

    case 'q':
      return 0;

    default:
      return reciper;
    }
  }
}



/* ----------------------------------------------------- */
/* 朋友名單：選單式操作界面描述				 */
/* ----------------------------------------------------- */


static int pal_add();


static void
pal_item(num, pal)
  int num;
  PAL *pal;
{
#ifdef CHECK_ONLINE
  UTMP *online = utmp_get(pal->userno, NULL);

  prints("%6d%c%-3s%s%-14s%s%s\n", 
    num, tag_char(pal->userno), pal->ftype & PAL_BAD ? "Ｘ" : "", 
    online ? COLOR7 : "", pal->userid, online ? str_ransi : "", pal->ship);
#else
  prints("%6d%c%-3s%-14s%s\n", 
    num, tag_char(pal->userno), pal->ftype & PAL_BAD ? "Ｘ" : "", pal->userid, pal->ship);
#endif
}


static int
pal_body(xo)
  XO *xo;
{
  PAL *pal;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (vans("要交新朋友嗎(Y/N)？[N] ") == 'y')
      return pal_add(xo);
    return XO_QUIT;
  }

  pal = (PAL *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    pal_item(++num, pal++);
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
}


static int
pal_head(xo)
  XO *xo;
{
  char *head[] = {"朋友名單", "群組名單", "板友名單", "限制投票名單"};

  vs_head(head[xo->key], str_site);
  prints(NECKER_PAL, d_cols, "");
  return pal_body(xo);
}


static int
pal_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(PAL));
  return pal_body(xo);
}


static int
pal_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(PAL));
  return pal_head(xo);
}


void
pal_edit(key, pal, echo)
  int key;
  PAL *pal;
  int echo;
{
  if (echo == DOECHO)
    memset(pal, 0, sizeof(PAL));
  vget(b_lines, 0, "友誼：", pal->ship, sizeof(pal->ship), echo);
#ifdef HAVE_BADPAL
  if (key != PALTYPE_VOTE)	/* 限制投票名單沒有壞人 */
    pal->ftype = vans("壞人(Y/N)？[N] ") == 'y' ? PAL_BAD : 0;
  else
#endif
    pal->ftype = 0;
}


/* static */ 			/* itoc.020117: 給 vote.c 用 */
int
pal_find(fpath, userno)		/* itoc.010923: 朋友名單中是否已有此人 */
  char *fpath;
  int userno;
{
  PAL old;
  int fd;
  int rc = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(PAL)) == sizeof(PAL))
    {
      if (userno == old.userno)
      {
	rc = 1;
	break;
      }
    }
    close(fd);
  }
  return rc;
}


static int
pal_add(xo)
  XO *xo;
{
  ACCT acct;
  int userno;

  if (xo->max >= PAL_MAX)
  {
    vmsg(msg_list_over);
    return XO_FOOT;
  }

  userno = acct_get(msg_uid, &acct);

  if (userno == cuser.userno)	/* lkchu.981201: 朋友名單不可加自己 */
  {
    vmsg("自己不須加入朋友名單中");
  }
  else if (pal_find(xo->dir, userno))
  {
    vmsg("名單中已有此人");
  }
  else if (userno > 0)
  {
    PAL pal;

    pal_edit(xo->key, &pal, DOECHO);
    strcpy(pal.userid, acct.userid);
    pal.userno = userno;
    rec_add(xo->dir, &pal, sizeof(PAL));

    if (xo->key == PALTYPE_PAL)
      utmp_admset(userno, STATUS_PALDIRTY);

    xo->pos = XO_TAIL;		/* 放在最後 */
    return pal_init(xo);
  }

  return pal_head(xo);
}


static int
pal_delete(xo)
  XO *xo;
{
  if (vans(msg_del_ny) == 'y')
  {
    if (!rec_del(xo->dir, sizeof(PAL), xo->pos, NULL))
    {
      if (xo->key == PALTYPE_PAL)
      {
	PAL *pal;
	pal = (PAL *) xo_pool + (xo->pos - xo->top);
	utmp_admset(pal->userno, STATUS_PALDIRTY);
      }
      return pal_load(xo);
    }
  }
  return XO_FOOT;
}


static void
changestatus(xo, pal)
  XO *xo;
  PAL *pal;
{
  if (xo->key == PALTYPE_PAL)
    utmp_admset(pal->userno, STATUS_PALDIRTY);
}


static int
pal_rangedel(xo)
  XO *xo;
{
  return xo_rangedel(xo, sizeof(PAL), NULL, changestatus);
}


static int
vfypal(pal, pos)
  PAL *pal;
  int pos;
{
  return Tagger(pal->userno, pos, TAG_NIN);
}


static int
pal_prune(xo)
  XO *xo;
{
  return xo_prune(xo, sizeof(PAL), vfypal, changestatus);
}


static int
pal_change(xo)
  XO *xo;
{
  PAL *pal, oldpal;
  int pos, cur;
  
  pos = xo->pos;
  cur = pos - xo->top;
  pal = (PAL *) xo_pool + cur;

  memcpy(&oldpal, pal, sizeof(PAL));
  pal_edit(xo->key, pal, GCARRY);

  if (memcmp(&oldpal, pal, sizeof(PAL)))
  {
    rec_put(xo->dir, pal, sizeof(PAL), pos, NULL);
    if (xo->key == PALTYPE_PAL)
      utmp_admset(pal->userno, STATUS_PALDIRTY);
    move(3 + cur, 0);
    pal_item(++pos, pal);
  }

  return XO_FOOT;
}


static int
pal_mail(xo)
  XO *xo;
{
  PAL *pal;

  pal = (PAL *) xo_pool + (xo->pos - xo->top);
  return my_send(pal->userid);
}


static int
pal_write(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_PAGE))
  {
    PAL *pal;
    UTMP *up;

    pal = (PAL *) xo_pool + (xo->pos - xo->top);

    if (up = utmp_find(pal->userno))
      do_write(up);
  }
  return XO_NONE;
}


static int
pal_broadcast(xo)
  XO *xo;
{
  int fd;
  BMW bmw;
  PAL *pal;
  UTMP *up;

  if (!HAS_PERM(PERM_PAGE))
    return XO_NONE;

  bmw.caller = NULL;
  bmw_edit(NULL, "★廣播：", &bmw);

  if (bmw.caller)	/* bmw_edit() 中回答 Yes 要送出廣播 */
  {
    /* itoc.000213: 加 "> " 為了與一般水球區分 */
    sprintf(bmw.userid, "%s> ", cuser.userid);

    if ((fd = open(xo->dir, O_RDONLY)) >= 0)
    {
      mgets(-1);
      while (pal = mread(fd, sizeof(PAL)))
      {
	if (pal->ftype & PAL_BAD)
	  continue;

	if (!(up = utmp_find(pal->userno)))
	  continue;

#ifdef HAVE_NOBROAD
	if (up->ufo & UFO_RCVER)
	  continue;
#endif

	if (can_override(up))
	{
	  bmw.recver = up->userno;
	  bmw_send(up, &bmw);
	}
      }
      close(fd);
    }
  }

  return XO_NONE;
}


#if (defined(HAVE_MODERATED_BOARD) || defined(HAVE_LIST))
static int
pal_cite(xo)
  XO *xo;
{
  int fd, num;
  char fpath[64], *dir;
  PAL *pal;

  fd = vans("要引入 (P)朋友名單 "
#ifdef HAVE_MODERATED_BOARD
    "(B)板友名單 "
#endif
#ifdef HAVE_LIST
    "(1-5)特別名單"
#endif
    "？[Q] ");

  if (fd == 'p')
  {
    usr_fpath(fpath, cuser.userid, fn_pal);
  }
#ifdef HAVE_MODERATED_BOARD
  else if (fd == 'b')
  {
    if (currbno < 0 || !(bbstate & STAT_BOARD))
    {
      vmsg("您尚未選定看板，或是您不是該板的板主");
      return XO_FOOT;
    }
    brd_fpath(fpath, currboard, fn_pal);
  }
#endif
#ifdef HAVE_LIST
  else if (fd >= '1' && fd <= '5')
  {
    char buf[32];
    sprintf(buf, "%s.%c", FN_LIST, fd);
    usr_fpath(fpath, cuser.userid, buf);
  }
#endif
  else
  {
    return XO_FOOT;
  }

  dir = xo->dir;
  if (!strcmp(dir, fpath))
  {
    vmsg("不能引入同一份名單");
    return XO_FOOT;
  }

  if ((fd = open(fpath, O_RDONLY)) < 0)
    return XO_FOOT;
 
  num = PAL_MAX - xo->max;

  mgets(-1);
  while (pal = mread(fd, sizeof(PAL)))
  {
    if (!(pal->ftype & PAL_BAD) && !pal_find(dir, pal->userno))
    {
      if (--num < 0)		/* itoc.001224: 引入名單只加到 PAL_MAX */
	break;

      rec_add(dir, pal, sizeof(PAL));
      xo->pos = XO_TAIL;	/* 若有引入名單，就把游標放在最後 */
    }
  }
  close(fd);

  return pal_load(xo);
}
#endif


static int
pal_sort(xo)
  XO *xo;
{
  pal_sync(xo->dir);
  return pal_load(xo);
}


static int
pal_query(xo)
  XO *xo;
{
  PAL *pal;

  pal = (PAL *) xo_pool + (xo->pos - xo->top);
  move(1, 0);
  clrtobot();
  my_query(pal->userid);
  return pal_head(xo);
}


static int
pal_tag(xo)
  XO *xo;
{
  PAL *pal;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  pal = (PAL *) xo_pool + cur;

  if (tag = Tagger(pal->userno, pos, TAG_TOGGLE))
  {
    move(3 + cur, 6);
    outc(tag > 0 ? '*' : ' ');
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE;	/* lkchu.981201: 跳至下一項 */
}


static int
pal_help(xo)
  XO *xo;
{
  xo_help("pal");
  return pal_head(xo);
}


KeyFunc pal_cb[] =
{
  XO_INIT, pal_init,
  XO_LOAD, pal_load,
  XO_HEAD, pal_head,
  XO_BODY, pal_body,

  'a', pal_add,
  'c', pal_change,
  'd', pal_delete,
  'D', pal_rangedel,
  'm', pal_mail,
  'w', pal_write,
  'B', pal_broadcast,
  'r', pal_query,
  Ctrl('Q'), pal_query,
  's', pal_sort,
  't', pal_tag,
  Ctrl('D'), pal_prune,

#if (defined(HAVE_MODERATED_BOARD) || defined(HAVE_LIST))
  'f', pal_cite,
#endif

  'h', pal_help
};


int
t_pal()
{
  XO *xo;
  char fpath[64];

  usr_fpath(fpath, cuser.userid, fn_pal);
  xz[XZ_PAL - XO_ZONE].xo = xo = xo_new(fpath);
  xo->key = PALTYPE_PAL;
  xover(XZ_PAL);
  free(xo);

  /* itoc.041211.註解: 在離開朋友名單再一併同步 cache 是有問題的，
     當我尚未離開朋友名單時，此時 cutmp->pal_spool 尚未同步，
     但被我異動朋友狀態的對方已經加入了 STATUS_PALDIRTY，
     若對方在我尚未離開朋友名單時就先行進入使用者名單，
     這時雖然他已經收到我給他的 STATUS_PALDIRTY，然而卻因為我的 pal_spool 尚未同步，
     於是他並沒有成功變更我的朋友狀態，但 STATUS_PALDIRTY 已消失。
     要解決這個問題，得在異動每一筆朋友時的 utmp_admset(STATUS_PALDIRTY) 之前就先 pal_cache()，
     不過現在還沒有人抱怨這問題，所以就省點工好了 :p */

  pal_cache();	/* itoc.010923: 離開朋友名單再一併同步 cache */

  return 0;
}


#ifdef HAVE_LIST
int
t_list()
{
  int n;
  char fpath[64], buf[8];
  XO *xo;

  move(MENU_XPOS, 0);
  clrtobot();

  for (n = 1; n <= 5; n++)
  {
    move(n + MENU_XPOS - 1, MENU_YPOS - 1);
    prints("(\033[1;36m%d\033[m) 群組名單.%d", n, n);
  }

  n = vans("請選擇檔案編號，或按 [0] 取消：") - '0';
  if (n <= 0 || n > 5)
    return 0;

  sprintf(buf, "%s.%d", FN_LIST, n);
  usr_fpath(fpath, cuser.userid, buf);

  switch (vget(b_lines, 36, "(D)刪除 (E)編輯 [Q]取消？", buf, 3, LCECHO))
  {
  case 'd':
    unlink(fpath);
    break;

  case 'e':
    /* 借用 XZ_PAL 即可 */
    xz[XZ_PAL - XO_ZONE].xo = xo = xo_new(fpath);
    xo->key = PALTYPE_LIST;
    xover(XZ_PAL);
    free(xo);
    break;
  }

  return 0;
}
#endif
