/*-------------------------------------------------------*/
/* vote.c	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : boards' vote routines		 	 */
/* create : 95/03/29				 	 */
/* update : 95/12/15				 	 */
/*-------------------------------------------------------*/
/* brd/_/.VCH  : Vote Control Header    目前所有投票索引 */
/* brd/_/@vote : vote history           過去的投票歷史	 */
/* brd/_/@/@_  : vote description       投票說明	 */
/* brd/_/@/I_  : vote selection Items   投票選項	 */
/* brd/_/@/O_  : users' Opinions        使用者有話要說	 */
/* brd/_/@/L_  : can vote List          可投票名單	 */
/* brd/_/@/G_  : voted id loG file      已領票名單	 */
/* brd/_/@/Z_  : final/temporary result	投票結果	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;
extern XZ xz[];
extern char xo_pool[];


static char *
vch_fpath(fpath, folder, vch)
  char *fpath, *folder;
  VCH *vch;
{
  /* VCH 和 HDR 的 xname 欄位匹配，所以直接借用 hdr_fpath() */
  hdr_fpath(fpath, folder, (HDR *) vch);
  return strrchr(fpath, '@');
}


static int vote_add();


int
vote_result(xo)
  XO *xo;
{
  char fpath[64];

  setdirpath(fpath, xo->dir, "@/@vote");
  /* Thor.990204: 為考慮more 傳回值 */   
  if (more(fpath, NULL) >= 0)
    return XO_HEAD;	/* XZ_POST 和 XZ_VOTE 共用 vote_result() */

  vmsg("目前沒有任何開票的結果");
  return XO_FOOT;
}


static void
vote_item(num, vch)
  int num;
  VCH *vch;
{
  prints("%6d%c%c%c%c%c %-9.8s%-12s %.44s\n",
    num, tag_char(vch->chrono), vch->vgamble, vch->vsort, vch->vpercent, vch->vprivate, 
    vch->cdate, vch->owner, vch->title);
}


static int
vote_body(xo)
  XO *xo;
{
  VCH *vch;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (bbstate & STAT_BOARD)
    {
      if (vans("要舉辦投票嗎(Y/N)？[N] ") == 'y')
	return vote_add(xo);
    }
    else
    {
      vmsg("目前並無投票舉行");
    }
    return XO_QUIT;
  }

  vch = (VCH *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    vote_item(++num, vch++);
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
}


static int
vote_head(xo)
  XO *xo;
{
  vs_head(currBM, "投票所");
  prints(NECKER_VOTE, d_cols, "");
  return vote_body(xo);
}


static int
vote_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(VCH));
  return vote_head(xo);
}


static int
vote_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(VCH));
  return vote_body(xo);
}


static void
vch_edit(vch, item, echo)
  VCH *vch;
  int item;		/* vlist 有幾項 */
  int echo;
{
  int num, row;
  char ans[8], buf[80];

  clear();

  row = 3;

  if (echo == DOECHO)	/* 只有新增時才能決定是否為賭盤 */
    vch->vgamble = (vget(++row, 0, "是否為賭盤(Y/N)？[N] ", ans, 3, LCECHO) == 'y') ? '$' : ' ';

  if (vch->vgamble == ' ')
  {
    sprintf(buf, "請問每人最多可投幾票？([1]～%d)：", item);
    vget(++row, 0, buf, ans, 3, DOECHO);
    num = atoi(ans);
    if (num < 1)
      num = 1;
    else if (num > item)
      num = item;
    vch->maxblt = num;
  }
  else if (echo == DOECHO)	/* 只有新增時才能改變賭盤的票價 */
  {
    /* 賭盤就只能選一項 */
    vch->maxblt = 1;

    vget(++row, 0, "請問每票售價多少銀幣？(100～100000)：", ans, 7, DOECHO);
    num = atoi(ans);
    if (num < 100)
      num = 100;
    else if (num > 100000)
      num = 100000;
    vch->price = num;
  }

  vget(++row, 0, "本項投票進行幾小時 (至少一小時)？[1] ", ans, 5, DOECHO);
  num = atoi(ans);
  if (num < 1)
    num = 1;
  vch->vclose = vch->chrono + num * 3600;
  str_stamp(vch->cdate, &vch->vclose);

  if (vch->vgamble == ' ')	/* 賭盤一定排序、及顯示百分比 */
  {
    vch->vsort = (vget(++row, 0, "開票結果是否排序(Y/N)？[N] ", ans, 3, LCECHO) == 'y') ? 's' : ' ';
    vch->vpercent = (vget(++row, 0, "開票結果是否顯示百分比例(Y/N)？[N] ", ans, 3, LCECHO) == 'y') ? '%' : ' ';
  }
  else
  {
    vch->vsort = 's';
    vch->vpercent = '%';
  }

  vch->vprivate = (vget(++row, 0, "是否限制投票名單(Y/N)？[N] ", ans, 3, LCECHO) == 'y') ? ')' : ' ';

  if (vch->vprivate == ' ' && vget(++row, 0, "是否限制投票資格(Y/N)？[N] ", ans, 3, LCECHO) == 'y')
  {
    vget(++row, 0, "請問要登入幾次以上才可以參加本次投票？([0]～9999)：", ans, 5, DOECHO);
    num = atoi(ans);
    if (num < 0)
      num = 0;
    vch->limitlogins = num;

    vget(++row, 0, "請問要發文幾次以上才可以參加本次投票？([0]～9999)：", ans, 5, DOECHO);
    num = atoi(ans);
    if (num < 0)
      num = 0;
    vch->limitposts = num;
  }
}


static int
vlist_edit(vlist)
  vitem_t vlist[];
{
  int item;
  char buf[80];

  clear();

  outs("請依序輸入選項 (最多 32 項)，按 ENTER 結束：");

  strcpy(buf, " ) ");
  for (;;)
  {
    item = 0;
    for (;;)
    {
      buf[0] = radix32[item];
      if (!vget((item & 15) + 3, (item / 16) * 40, buf, vlist[item], sizeof(vitem_t), GCARRY) || 
        (++item >= MAX_CHOICES))
	break;
    }
    if (item && vans("是否重新輸入選項(Y/N)？[N] ") != 'y')
      break;
  }
  return item;
}


static int
vlog_seek(fpath)
  char *fpath;
{
  VLOG old;
  int fd;
  int rc = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(VLOG)) == sizeof(VLOG))
    {
      if (!strcmp(old.userid, cuser.userid))
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
vote_add(xo)
  XO *xo;
{
  VCH vch;
  int fd, item;
  char *dir, *str, fpath[64], title[TTLEN + 1];
  vitem_t vlist[MAX_CHOICES];
  BRD *brd;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  if (!vget(b_lines, 0, "標題：", title, TTLEN + 1, DOECHO))
    return xo->max ? XO_FOOT : vote_body(xo);	/* itoc.011125: 如果沒有任何投票，要回到 vote_body() */
    /* return XO_FOOT; */

  dir = xo->dir;
  if ((fd = hdr_stamp(dir, 0, (HDR *) &vch, fpath)) < 0)
  {
    vmsg("無法建立投票說明檔");
    return XO_FOOT;
  }

  close(fd);
  vmsg("開始編輯 [投票說明]");
  fd = vedit(fpath, 0); /* Thor.981020: 注意被talk的問題 */
  if (fd)
  {
    unlink(fpath);
    vmsg("取消投票");
    return vote_head(xo);
  }

  strcpy(vch.title, title);
  str = strrchr(fpath, '@');

  /* --------------------------------------------------- */
  /* 投票選項檔 : Item					 */
  /* --------------------------------------------------- */

  memset(vlist, 0, sizeof(vlist));
  item = vlist_edit(vlist);

  *str = 'I';
  if ((fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
  {
    vmsg("無法建立投票選項檔");
    return vote_head(xo);
  }
  write(fd, vlist, item * sizeof(vitem_t));
  close(fd);

  vch_edit(&vch, item, DOECHO);

  strcpy(vch.owner, cuser.userid);

  brd = bshm->bcache + currbno;

  brd->bvote++;
  if (brd->bvote >= 0)
    brd->bvote = (vch.vgamble == '$') ? -1 : 1;
  vch.bstamp = brd->bstamp;

  rec_add(dir, &vch, sizeof(VCH));

  vmsg("開始投票了！");
  return vote_init(xo);
}


static int
vote_edit(xo)
  XO *xo;
{
  int pos;
  VCH *vch, vxx;
  char *dir, fpath[64];

  /* Thor: for 修改投票選項 */
  int fd, item;
  vitem_t vlist[MAX_CHOICES];
  char *fname;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  dir = xo->dir;
  vch = (VCH *) xo_pool + (pos - xo->top);

  /* Thor: 修改投票主題 */

  vxx = *vch;

  if (!vget(b_lines, 0, "標題：", vxx.title, TTLEN + 1, GCARRY))
    return XO_FOOT;

  fname = vch_fpath(fpath, dir, vch);
  vedit(fpath, 0);	/* Thor.981020: 注意被talk的問題  */

  /* Thor: 修改投票選項 */

  memset(vlist, 0, sizeof(vlist));
  *fname = 'I';
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    read(fd, vlist, sizeof(vlist));
    close(fd);
  }

  item = vlist_edit(vlist);

  if ((fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)
  {
    vmsg("無法建立投票選項檔");
    return vote_head(xo);
  }
  write(fd, vlist, item * sizeof(vitem_t));
  close(fd);

  vch_edit(&vxx, item, GCARRY);

  if (memcmp(&vxx, vch, sizeof(VCH)))
  {
    if (vans("確定要修改這項投票嗎(Y/N)？[N] ") == 'y')
    {
      *vch = vxx;
      currchrono = vch->chrono;
      rec_put(dir, vch, sizeof(VCH), pos, cmpchrono);
    }
  }

  return vote_head(xo);
}


static int
vote_query(xo)
  XO *xo;
{
  char *dir, *fname, fpath[64], buf[80];
  VCH *vch;
  int cc, pos;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  dir = xo->dir;
  vch = (VCH *) xo_pool + (pos - xo->top);

  fname = vch_fpath(fpath, dir, vch);
  more(fpath, (char *) -1);

  *fname = 'G';
  sprintf(buf, "共有 %d 人參加投票，確定要將開票時間改期(Y/N)？[N] ", rec_num(fpath, sizeof(VLOG)));
  if (vans(buf) == 'y')
  {
    vget(b_lines, 0, "請更改開票時間(-n提前n小時/+m延後m小時/0不改)：", buf, 5, DOECHO);
    if (cc = atoi(buf))
    {
      vch->vclose = vch->vclose + cc * 3600;
      str_stamp(vch->cdate, &vch->vclose);
      currchrono = vch->chrono;
      rec_put(dir, vch, sizeof(VCH), pos, cmpchrono);
    }
  }

  return vote_head(xo); 
}


static int
vfyvch(vch, pos)
  VCH *vch;
  int pos;
{
  return Tagger(vch->chrono, pos, TAG_NIN);
}


static void
delvch(xo, vch)
  XO *xo;
  VCH *vch;
{
  int fd;
  char fpath[64], buf[64], *fname;
  char *list = "@IOLGZ";	/* itoc.註解: 清 vote file */
  VLOG vlog;
  PAYCHECK paycheck;

  fname = vch_fpath(fpath, xo->dir, vch);

  if (vch->vgamble == '$')	/* itoc.050313: 如果是賭盤被刪除，那麼要退賭金 */
  {
    *fname = 'G';

    if ((fd = open(fpath, O_RDONLY)) >= 0)
    {
      memset(&paycheck, 0, sizeof(PAYCHECK));
      time(&paycheck.tissue);
      sprintf(paycheck.reason, "[退款] %s", currboard);

      while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
      {
	paycheck.money = vlog.numvotes * vch->price;
	usr_fpath(buf, vlog.userid, FN_PAYCHECK);
	rec_add(buf, &paycheck, sizeof(PAYCHECK));
      }
    }
    close(fd);
  }

  while (*fname = *list++)
    unlink(fpath); /* Thor: 確定名字就砍 */
}



static int
vote_delete(xo)
  XO *xo;
{
  int pos;
  VCH *vch;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  vch = (VCH *) xo_pool + (pos - xo->top);

  if (vans(msg_del_ny) == 'y')
  {
    delvch(xo, vch);

    currchrono = vch->chrono;
    rec_del(xo->dir, sizeof(VCH), pos, cmpchrono);    
    return vote_load(xo);
  }

  return XO_FOOT;
}


static int
vote_rangedel(xo)
  XO *xo;
{
  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  return xo_rangedel(xo, sizeof(VCH), NULL, delvch);
}


static int
vote_prune(xo)
  XO *xo;
{
  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  return xo_prune(xo, sizeof(VCH), vfyvch, delvch);
}


static int
vote_pal(xo)		/* itoc.020117: 編輯限制投票名單 */
  XO *xo;
{
  char *fname, fpath[64];
  VCH *vch;
  XO *xt;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  vch = (VCH *) xo_pool + (xo->pos - xo->top);

  if (vch->vprivate != ')')
    return XO_NONE;

  fname = vch_fpath(fpath, xo->dir, vch);
  *fname = 'L';

  xz[XZ_PAL - XO_ZONE].xo = xt = xo_new(fpath);
  xt->key = PALTYPE_VOTE;
  xover(XZ_PAL);		/* Thor: 進xover前, pal_xo 一定要 ready */

  free(xt);
  return vote_init(xo);
}


static int
vote_join(xo)
  XO *xo;
{
  VCH *vch, vbuf;
  VLOG vlog;
  int count, fd;
  usint choice;
  char *dir, *fname, fpath[64], buf[80], ans[4], *slist[MAX_CHOICES];
  vitem_t vlist[MAX_CHOICES];

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XO_FOOT;
  }

  vch = (VCH *) xo_pool + (xo->pos - xo->top);

  /* --------------------------------------------------- */
  /* 檢查是否已經結束投票				 */
  /* --------------------------------------------------- */

  if (time(0) > vch->vclose)
  {
    vmsg("投票已經截止了，請靜候開票");
    return XO_FOOT;
  }

  /* --------------------------------------------------- */
  /* 檢查是否有足夠錢					 */
  /* --------------------------------------------------- */

  if (vch->vgamble == '$')
  {
    if (cuser.money < vch->price)
    {
      vmsg("您的錢不夠參加賭盤");
      return XO_FOOT;
    }
  }

  /* --------------------------------------------------- */
  /* 投票檔案						 */
  /* --------------------------------------------------- */

  dir = xo->dir;
  fname = vch_fpath(fpath, dir, vch);

  /* --------------------------------------------------- */
  /* 檢查是否已經投過票					 */
  /* --------------------------------------------------- */

  if (vch->vgamble == ' ')	/* itoc.031101: 賭盤可以一直下注 */
  {
    *fname = 'G';
    if (vlog_seek(fpath))
    {
      vmsg("您已經投過票了！");
      return XO_FOOT;
    }
  }

  /* --------------------------------------------------- */
  /* 檢查投票限制					 */
  /* --------------------------------------------------- */

  if (vch->vprivate == ' ')
  {
    if (cuser.numlogins < vch->limitlogins || cuser.numposts < vch->limitposts)
    {
      vmsg("您不夠資深喔！");
      return XO_FOOT;
    }
  }
  else		/* itoc.020117: 私人投票檢查是否在投票名單中 */
  {
    *fname = 'L';

    if (!pal_find(fpath, cuser.userno) &&
      !(bbstate & STAT_BOARD))		/* 由於並不能把自己加入朋友名單，所以要多檢查是否為板主 */
    {
      vmsg("您沒有受邀本次私人投票！");
      return XO_FOOT;
    }
  }

  /* --------------------------------------------------- */
  /* 確認進入投票					 */
  /* --------------------------------------------------- */

  if (vans(vch->vgamble == ' ' ? "是否參加投票(Y/N)？[N] " : "是否參加賭盤(Y/N)？[N] ") != 'y')
    return XO_FOOT;

  /* --------------------------------------------------- */
  /* 開始投票，顯示投票說明				 */
  /* --------------------------------------------------- */

  *fname = '@';
  more(fpath, NULL);

  /* --------------------------------------------------- */
  /* 載入投票選項檔					 */
  /* --------------------------------------------------- */

  *fname = 'I';
  if ((fd = open(fpath, O_RDONLY)) < 0)
  {
    vmsg("無法讀取投票選項檔");
    return vote_head(xo);
  }
  count = read(fd, vlist, sizeof(vlist)) / sizeof(vitem_t);
  close(fd);

  for (fd = 0; fd < count; fd++)
    slist[fd] = (char *) &vlist[fd];

  /* --------------------------------------------------- */
  /* 進行投票						 */
  /* --------------------------------------------------- */

  choice = 0;
  sprintf(buf, "投下神聖的 %d 票", vch->maxblt); /* Thor: 顯示最多幾票 */
  vs_bar(buf);
  outs("投票主題：");
  for (;;)
  {
    choice = bitset(choice, count, vch->maxblt, vch->title, slist);

    if (vch->vgamble == ' ')		/* 一般投票才能寫意見 */
      vget(b_lines - 1, 0, "我有話要說：", buf, 60, DOECHO);

    fd = vans("投票 (Y)確定 (N)重來 (Q)取消？[N] ");

    if (fd == 'q')
      return vote_head(xo);

    if ((fd == 'y') && (vch->vgamble == ' ' || choice))	/* 若是賭盤則一定要選 */
      break;
  }

  /* --------------------------------------------------- */
  /* 記錄結果：一票也未投的情況 ==> 相當於投廢票	 */
  /* --------------------------------------------------- */

  if (vch->vgamble == '$')
  {
    /* 賭盤可以買入多張 */
    for (;;)
    {
      sprintf(buf, "每張賭票 %d 銀幣，請問要買幾張？[1] ", vch->price);
      vget(b_lines, 0, buf, ans, 3, DOECHO);	/* 最多買 99 張，避免溢位 */

      if (time(0) > vch->vclose)	/* 因為有個 vget，所以還要再檢查一次 */
      {
	vmsg("投票已經截止了，請靜候開票");
	return vote_head(xo);
      }

      if ((count = atoi(ans)) < 1)
	count = 1;
      fd = count * vch->price;
      if (cuser.money >= fd)
	break;
    }
  }
  else
  {
    /* 一般投票就是一張票 */
    count = 1;
  }

  /* 確定投票尚未截止 */
  /* itoc.050514: 因為板主可以改變開票時間，為了避免使用者會龜在 vget() 或是
     利用 xo_pool[] 未同步來規避 time(0) > vclose 的檢查，所以就得重新載入 VCH */
  if (rec_get(dir, &vbuf, sizeof(VCH), xo->pos) || vch->chrono != vch->chrono || time(0) > vbuf.vclose)
  {
    vmsg("投票已經截止了，請靜候開票");
    return vote_init(xo);
  }

  if (vch->vgamble == '$')
  {
    cuser.money -= fd;	/* fd 是要付的賭金 */
  }
  else if (*buf)	/* 一般投票才能寫入使用者意見 */
  {
    FILE *fp;

    *fname = 'O';
    if (fp = fopen(fpath, "a"))
    {
      fprintf(fp, "‧%-12s：%s\n", cuser.userid, buf);
      fclose(fp);
    }
  }

  /* 加入記錄檔 */
  memset(&vlog, 0, sizeof(VLOG));
  strcpy(vlog.userid, cuser.userid);
  vlog.numvotes = count;
  vlog.choice = choice;
  *fname = 'G';
  rec_add(fpath, &vlog, sizeof(VLOG));

  vmsg("投票完成！");
  return vote_head(xo);
}


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


static char *			/* NULL:失敗(還沒有人投票) */
draw_vote(fpath, folder, vch, preview)	/* itoc.030906: 投票結果 (與 account.c:draw_vote() 格式相同) */
  char *fpath;
  char *folder;
  VCH *vch;
  int preview;		/* 1:預覽 0:開票 */
{
  struct Tchoice choice[MAX_CHOICES];
  FILE *fp;
  char *fname;
  int total, items, num, fd, ticket, bollt;
  VLOG vlog;

  fname = vch_fpath(fpath, folder, vch);

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
    return NULL;

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
    return NULL;

  fprintf(fp, "\n\033[1;34m%s\033[m\n\n"
    "\033[1;32m◆ [%s] 看板投票：%s\033[m\n\n舉辦板主：%s\n\n舉辦日期：%s\n\n",
    msg_seperator, currboard, vch->title, vch->owner, Btime(&vch->chrono));
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

  if (preview && vch->vgamble == ' ')	/* 只有預覽賭盤才需要顯示賠率 */
    preview = 0;

  for (num = 0; num < items; num++)
  {
    ticket = choice[num].count;
    if (preview)	/* 顯示加買一張時的賠率 */
      fprintf(fp, "    %-36s%5d 票 (%4.1f%%) 賠率 1:%.3f\n", choice[num].vitem, ticket, 100.0 * ticket / fd, 0.9 * (bollt + 1) / (ticket + 1));
    else if (fd)
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

  /* 最後傳回的 fpath 即為投票結果檔 */
  *fname = 'Z';
  return fname;
}


static int
vote_view(xo)
  XO *xo;
{
  char fpath[64];
  VCH *vch;

  vch = (VCH *) xo_pool + (xo->pos - xo->top);

  if (bbstate & STAT_BOARD || vch->vgamble == '$')
  {
    if (draw_vote(fpath, xo->dir, vch, 1))
    {
      more(fpath, NULL);
      unlink(fpath);
      return vote_head(xo);
    }

    vmsg("目前尚未有人投票");
    return XO_FOOT;
  }

  return XO_NONE;
}


static void
keeplog(fnlog, board, title)
  char *fnlog;
  char *board;
  char *title;
{
  HDR hdr;
  char folder[64], fpath[64];
  FILE *fp;

  if (!dashf(fnlog))	/* Kudo.010804: 檔案是空的就不 keeplog */
    return;

  brd_fpath(folder, board, fn_dir);

  if (fp = fdopen(hdr_stamp(folder, 'A', &hdr, fpath), "w"))
  {
    fprintf(fp, "作者: %s (%s)\n標題: %s\n時間: %s\n\n",
      str_sysop, SYSOPNICK, title, Btime(&hdr.chrono));
    f_suck(fp, fnlog);
    fclose(fp);

    strcpy(hdr.title, title);
    strcpy(hdr.owner, str_sysop);
    rec_bot(folder, &hdr, sizeof(HDR));

    btime_update(brd_bno(board));
  }
}


static void
vlog_pay(fpath, choice, fp, vch)/* 賠錢給押對的使用者 */
  char *fpath;			/* 記錄檔路徑 */
  usint choice;			/* 正確的答案 */
  FILE *fp;			/* 寫入的檔案 */
  VCH *vch;
{
  int fd;
  int correct, bollt;		/* 押對/全部 的票數 */
  int single, money;
  char buf[64];
  VLOG vlog;
  PAYCHECK paycheck;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    /* 第一圈算出賠率 */
    correct = bollt = 0;
    while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
    {
      bollt += vlog.numvotes;
      if (vlog.choice == choice)
	correct += vlog.numvotes;
    }

    /* 給板主抽頭 1% */
    money = (INT_MAX / vch->price) * 100;	/* BioStar.050626: 避免溢位 */
    money = (bollt > money) ? INT_MAX : vch->price / 100 * bollt;
    fprintf(fp, "板主 %s 抽頭，可獲得 %d 銀幣\n", vch->owner, money);

    memset(&paycheck, 0, sizeof(PAYCHECK));
    time(&paycheck.tissue);
    paycheck.money = money;
    sprintf(paycheck.reason, "[抽頭] %s", currboard);
    usr_fpath(buf, vch->owner, FN_PAYCHECK);
    rec_add(buf, &paycheck, sizeof(PAYCHECK));

    if (correct)	/* 如果沒人押中，就不需要發錢 */
    {
      /* 發獎金，系統抽 10% 的稅 */
      single = (float) vch->price * 0.9 * bollt / correct;
      fprintf(fp, "每張可獲 %d 銀幣，押對的使用者有：\n", single);

      /* 第二圈開始發錢 */
      lseek(fd, (off_t) 0, SEEK_SET);
      while (read(fd, &vlog, sizeof(VLOG)) == sizeof(VLOG))
      {
	if (vlog.choice == choice)
	{
	  money = INT_MAX / single;		/* BioStar.050626: 避免溢位 */
	  money = (vlog.numvotes > money) ? INT_MAX : single * vlog.numvotes;
	  fprintf(fp, "%s 買了 %d 張，共可獲得 %d 銀幣\n", vlog.userid, vlog.numvotes, money);

	  paycheck.money = money;
	  sprintf(paycheck.reason, "[賭盤] %s", currboard);
	  usr_fpath(buf, vlog.userid, FN_PAYCHECK);
	  rec_add(buf, &paycheck, sizeof(PAYCHECK));
	}
      }
    }

    close(fd);
  }
}


static int
vote_open(xo)
  XO *xo;
{
  int pos, fd, count;
  char *dir, *fname, fpath[64], buf[80];
  usint choice;
  char *slist[MAX_CHOICES];
  vitem_t vlist[MAX_CHOICES];
  VCH *vch;
  FILE *fp;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  pos = xo->pos;
  vch = (VCH *) xo_pool + (pos - xo->top);

  if (time(NULL) < vch->vclose)
  {
    if (vans("尚未到原定開票時間，確定要提早開票(Y/N)？[N] ") != 'y')
      return XO_FOOT;
  }

  dir = xo->dir;

  /* 投票結果 */

  if (!(fname = draw_vote(fpath, dir, vch, 0)))
  {
    vmsg("目前尚未有人投票");
    return XO_FOOT;
  }

  if (vch->vgamble == '$')	/* 賭盤 */
  {
    /* 板主輸入結果，並寫入投票結果 */
    while (!vget(b_lines, 0, "請輸入正確答案：", buf, 60, DOECHO))
      ;

    /* 載入投票選項檔 */
    *fname = 'I';
    if ((fd = open(fpath, O_RDONLY)) >= 0)
    {
      count = read(fd, vlist, sizeof(vlist)) / sizeof(vitem_t);
      close(fd);

      for (fd = 0; fd < count; fd++)
	slist[fd] = (char *) &vlist[fd];

      /* 板主選出正確答案 */
      choice = 0;
      vs_bar("選擇正確答案");
      outs("投票主題：");
      for (;;)
      {
	choice = bitset(choice, count, vch->maxblt, vch->title, slist);

	fd = vans("開票 (Y)確定 (N)重來 (Q)取消？[N] ");

	if (fd == 'q')
	{
	  *fname = 'Z';
	  unlink(fpath);
	  return vote_head(xo);
	}

	if (fd == 'y' && choice)	/* 若是賭盤則一定要選 */
	  break;
      }

      /* 開始發錢 */
      *fname = 'Z';
      if (fp = fopen(fpath, "a"))
      {
	fprintf(fp, "板主公佈答案：%s\n\n", buf);

	*fname = 'G';
	vlog_pay(fpath, choice, fp, vch);

	fputs("\n", fp);
	fclose(fp);
      }

      /* 開票結果 */
      *fname = 'Z';
    }
  }

  /* 將開票結果 post 到 [BN_RECORD] 與 本看板 */

  if (!(currbattr & BRD_NOVOTE))
  {
    sprintf(buf, "[記錄] %s <<看板選情報導>>", currboard);
    keeplog(fpath, BN_RECORD, buf);
  }

  keeplog(fpath, currboard, "[記錄] 選情報導");

  /* 投票結果附加到 @vote */

  setdirpath(buf, dir, "@/@vote");
  if (fp = fopen(fpath, "a"))
  {
    f_suck(fp, buf);
    fclose(fp);
    rename(fpath, buf);
  }

  /* 開完票就刪除 */
  vch->vgamble = ' ';	/* 令為非賭盤，如此在 delvch 裡面就不會退賭金 */
  delvch(xo, vch);

  currchrono = vch->chrono;
  rec_del(dir, sizeof(VCH), pos, cmpchrono);    

  vmsg("開票完畢");
  return vote_init(xo);
}


static int
vote_tag(xo)
  XO *xo;
{
  VCH *vch;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  vch = (VCH *) xo_pool + cur;

  if (tag = Tagger(vch->chrono, pos, TAG_TOGGLE))
  {
    move(3 + cur, 6);
    outc(tag > 0 ? '*' : ' ');
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE;	/* lkchu.981201: 跳至下一項 */
}


static int
vote_help(xo)
  XO *xo;
{
  xo_help("vote");
  return vote_head(xo);
}


static KeyFunc vote_cb[] =
{
  XO_INIT, vote_init,
  XO_LOAD, vote_load,
  XO_HEAD, vote_head,
  XO_BODY, vote_body,

  'r', vote_join,	/* itoc.010901: 按右鍵比較方便 */
  'v', vote_join,
  'R', vote_result,

  'V', vote_view,
  'E', vote_edit,
  'o', vote_pal,
  'd', vote_delete,
  'D', vote_rangedel,
  't', vote_tag,
  'b', vote_open,

  Ctrl('D'), vote_prune,
  Ctrl('G'), vote_pal,
  Ctrl('P'), vote_add,
  Ctrl('Q'), vote_query,

  'h', vote_help
};


int
XoVote(xo)
  XO *xo;
{
  char fpath[64];

  /* 有 post 權利的才能參加投票 */
  /* 而且要避免 guest 在 sysop 板投票 */

  if (!(bbstate & STAT_POST) || !cuser.userlevel)
    return XO_NONE;

  setdirpath(fpath, xo->dir, FN_VCH);
  if (!(bbstate & STAT_BOARD) && !rec_num(fpath, sizeof(VCH)))
  {
    vmsg("目前沒有投票舉行");
    return XO_FOOT;
  }

  xz[XZ_VOTE - XO_ZONE].xo = xo = xo_new(fpath);
  xz[XZ_VOTE - XO_ZONE].cb = vote_cb;
  xover(XZ_VOTE);
  free(xo);

  return XO_INIT;
}


int
vote_all()		/* itoc.010414: 投票中心 */
{
  typedef struct
  {
    char brdname[BNLEN + 1];
    char class[BCLEN + 1];
    char title[BTLEN + 1];
    char BM[BMLEN + 1];
    char bvote;
  } vbrd_t;

  extern char brd_bits[];
  char *str;
  char fpath[64];
  int num, pageno, pagemax, redraw;
  int ch, cur;
  BRD *bhead, *btail;
  XO *xo;
  vbrd_t vbrd[MAXBOARD], *vb;

  bhead = bshm->bcache;
  btail = bhead + bshm->number;
  cur = 0;
  num = 0;

  do
  {
    str = &brd_bits[cur];
    ch = *str;
    if (bhead->bvote && (ch & BRD_W_BIT))
    {
      vb = vbrd + num;
      strcpy(vb->brdname, bhead->brdname);
      strcpy(vb->class, bhead->class);
      strcpy(vb->title, bhead->title);
      strcpy(vb->BM, bhead->BM);
      vb->bvote = bhead->bvote;
      num++;
    }
    cur++;
  } while (++bhead < btail);

  if (!num)
  {
    vmsg("目前站內並沒有任何投票");
    return XEASY;
  }

  num--;
  pagemax = num / XO_TALL;
  pageno = 0;
  cur = 0;
  redraw = 1;

  do
  {
    if (redraw)
    {
      /* itoc.註解: 盡量做得像 xover 格式 */
      vs_head("投票中心", str_site);
      prints(NECKER_VOTEALL, d_cols >> 1, "", d_cols - (d_cols >> 1), "");

      redraw = pageno * XO_TALL;	/* 借用 redraw */
      ch = BMIN(num, redraw + XO_TALL - 1);
      move(3, 0);
      do
      {
	vb = vbrd + redraw;
	/* itoc.010909: 板名太長的刪掉、加分類顏色。假設 BCLEN = 4 */
	prints("%6d   %-13s\033[1;3%dm%-5s\033[m%s %-*.*s %.*s\n",
	  redraw + 1, vb->brdname,
	  vb->class[3] & 7, vb->class,
	  vb->bvote > 0 ? ICON_VOTED_BRD : ICON_GAMBLED_BRD,
	  (d_cols >> 1) + 34, (d_cols >> 1) + 33, vb->title, d_cols - (d_cols >> 1) + 13, vb->BM);

	redraw++;
      } while (redraw <= ch);

      outf(FEETER_VOTEALL);
      move(3 + cur, 0);
      outc('>');
      redraw = 0;
    }

    switch (ch = vkey())
    {
    case KEY_RIGHT:
    case '\n':
    case ' ':
    case 'r':
      vb = vbrd + (cur + pageno * XO_TALL);

      /* itoc.060324: 等同進入新的看板，XoPost() 有做的事，這裡幾乎都要做 */
      if (!vb->brdname[0])	/* 已刪除的看板 */
	break;

      redraw = brd_bno(vb->brdname);	/* 借用 redraw */
      if (currbno != redraw)
      {
	ch = brd_bits[redraw];

	/* 處理權限 */
	if (ch & BRD_M_BIT)
	  bbstate |= (STAT_BM | STAT_BOARD | STAT_POST);
	else if (ch & BRD_X_BIT)
	  bbstate |= (STAT_BOARD | STAT_POST);
	else if (ch & BRD_W_BIT)
	  bbstate |= STAT_POST;

	mantime_add(currbno, redraw);

	currbno = redraw;
	bhead = bshm->bcache + currbno;
	currbattr = bhead->battr;
	strcpy(currboard, bhead->brdname);
	str = bhead->BM;
	sprintf(currBM, "板主：%s", *str <= ' ' ? "徵求中" : str);
#ifdef HAVE_BRDMATE
	strcpy(cutmp->reading, currboard);
#endif

	brd_fpath(fpath, currboard, fn_dir);
#ifdef AUTO_JUMPPOST
	xz[XZ_POST - XO_ZONE].xo = xo = xo_get_post(fpath, bhead);	/* itoc.010910: 為 XoPost 量身打造一支 xo_get() */
#else
	xz[XZ_POST - XO_ZONE].xo = xo = xo_get(fpath);
#endif
	xo->key = XZ_POST;
	xo->xyz = bhead->title;
      }

      sprintf(fpath, "brd/%s/%s", currboard, FN_VCH);
      xz[XZ_VOTE - XO_ZONE].xo = xo = xo_new(fpath);
      xz[XZ_VOTE - XO_ZONE].cb = vote_cb;
      xover(XZ_VOTE);
      free(xo);
      redraw = 1;
      break;

    default:
      ch = xo_cursor(ch, pagemax, num, &pageno, &cur, &redraw);
      break;
    }
  } while (ch != 'q');

  return 0;
}
