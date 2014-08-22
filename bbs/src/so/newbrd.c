/*-------------------------------------------------------*/
/* newbrd.c   ( YZU_CSE WindTop BBS )                    */
/*-------------------------------------------------------*/
/* target : 連署功能    			 	 */
/* create : 00/01/02				 	 */
/* update : 02/04/29				 	 */
/*-------------------------------------------------------*/
/* run/newbrd/_/.DIR - newbrd control header		 */
/* run/newbrd/_/@/@_ - newbrd description file		 */
/* run/newbrd/_/@/G_ - newbrd voted id loG file		 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef HAVE_COSIGN

extern XZ xz[];
extern char xo_pool[];
extern BCACHE *bshm;		/* itoc.010805: 開新板用 */

static int nbrd_add();
static int nbrd_body();
static int nbrd_head();

static char *split_line = "\033[33m──────────────────────────────\033[m\n";


typedef struct
{
  char userid[IDLEN + 1];
  char email[60];
}      LOG;


static int
cmpbtime(nbrd)
  NBRD *nbrd;
{
  return nbrd->btime == currchrono;
}


static char
nbrd_attr(nbrd)
  NBRD *nbrd;
{
  int xmode = nbrd->mode;

  /* 筆劃越少的，越傾向結案 */
  if (xmode & NBRD_FINISH)
    return ' ';
  if (xmode & NBRD_END)
    return '-';
#ifdef SYSOP_START_COSIGN
  if (xmode & NBRD_START)
    return '+';
  else
    return 'x';
#else
  return '+';
#endif
}


static int
nbrd_stamp(folder, nbrd, fpath)
  char *folder;
  NBRD *nbrd;
  char *fpath;
{
  char *fname;
  char *family = NULL;
  int rc;
  int token;

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }

  fname = family;
  *family++ = '@';

  token = time(0);

  archiv32(token, family);

  rc = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600);
  nbrd->btime = token;
  str_stamp(nbrd->date, &nbrd->btime);
  strcpy(nbrd->xname, fname);

  return rc;
}


static void
nbrd_fpath(fpath, folder, nbrd)
  char *fpath;
  char *folder;
  NBRD *nbrd;
{
  char *str;
  int cc;

  while (cc = *folder++)
  {
    *fpath++ = cc;
    if (cc == '/')
      str = fpath;
  }
  strcpy(str, nbrd->xname);
}


static int
nbrd_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(NBRD));
  return nbrd_head(xo);
}


static int
nbrd_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(NBRD));
  return nbrd_body(xo);
}


static void
nbrd_item(num, nbrd)
  int num;
  NBRD *nbrd;
{
  prints("%6d %c %-5s %-13s [%s] %.*s\n", 
    num, nbrd_attr(nbrd), nbrd->date + 3, nbrd->owner, 
    (nbrd->mode & NBRD_NEWBOARD) ? nbrd->brdname : "\033[1;33m本站公投\033[m", d_cols + 20, nbrd->title);
}


static int
nbrd_body(xo)
  XO *xo;
{
  NBRD *nbrd;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (vans("要新增連署項目嗎(Y/N)？[N] ") == 'y')
      return nbrd_add(xo);
    return XO_QUIT;
  }

  nbrd = (NBRD *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;

  if (max > tail)
    max = tail;

  move(3, 0);  
  do
  {
    nbrd_item(++num, nbrd++);
  } while (num < max);
  clrtobot();

  return XO_FOOT;
}


static int
nbrd_head(xo)
  XO *xo;
{
  vs_head("連署系統", str_site);
  prints(NECKER_COSIGN, d_cols, "");
  return nbrd_body(xo);
}


static int
nbrd_find(fpath, brdname)
  char *fpath, *brdname;
{
  NBRD old;
  int fd;
  int rc = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(NBRD)) == sizeof(NBRD))
    {
      if (!str_cmp(old.brdname, brdname) && !(old.mode & NBRD_FINISH))
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
nbrd_add(xo)
  XO *xo;
{
  int fd, ans, days, numbers;
  char *dir, fpath[64], path[64];
  char *brdname, *class, *title;
  FILE *fp;
  NBRD nbrd;

  if (HAS_PERM(PERM_ALLADMIN))
  {
    ans = vans("連署模式 1)開新板 2)記名 3)無記名：[Q] ");
    if (ans < '1' || ans > '3')
      return xo->max ? XO_FOOT : nbrd_body(xo);	/* itoc.020122: 如果沒有任何連署，要回到 nbrd_body() */
    /* itoc.030613: 其實下面的 return XO_FOOT; 也應該這樣改 */
  }
  else if (HAS_PERM(PERM_POST))
  {
    /* 一段使用者只能開新板連署 */
    ans = '1';
  }
  else
  {
    vmsg("對不起，本看板是唯讀的");
    return XO_FOOT;
  }

  memset(&nbrd, 0, sizeof(NBRD));

  brdname = nbrd.brdname;
  class = nbrd.class;
  title = nbrd.title;

  if (ans == '1')	/* 新板連署 */
  {
    if (!vget(b_lines, 0, "英文板名：", brdname, sizeof(nbrd.brdname), DOECHO))
      return XO_FOOT;

    if (brd_bno(brdname) >= 0 || !valid_brdname(brdname))
    {
      vmsg("已有此板或板名不合法");
      return XO_FOOT;
    }
    if (nbrd_find(xo->dir, brdname))
    {
      vmsg("正在連署中");
      return XO_FOOT;
    }

    if (!vget(b_lines, 0, "看板分類：", class, sizeof(nbrd.class), DOECHO) ||
      !vget(b_lines, 0, "看板主題：", title, sizeof(nbrd.title), DOECHO))
      return XO_FOOT;

    days = NBRD_DAY_BRD;
    numbers = NBRD_NUM_BRD;

#ifdef SYSOP_START_COSIGN
    nbrd.mode = NBRD_NEWBOARD;
#else
    nbrd.mode = NBRD_NEWBOARD | NBRD_START;
#endif
  }
  else			/* 其他連署 */
  {
    char tmp[8];

    if (!vget(b_lines, 0, "連署主題：", title, sizeof(nbrd.title), DOECHO))
      return XO_FOOT;

    /* 連署日期最多 30 天，連署人數最多 500 人 */
    if (!vget(b_lines, 0, "連署天數：", tmp, 5, DOECHO))
      return XO_FOOT;
    days = atoi(tmp);
    if (days > 30 || days < 1)
      return XO_FOOT;
    if (!vget(b_lines, 0, "連署人數：", tmp, 6, DOECHO))
      return XO_FOOT;
    numbers = atoi(tmp);
    if (numbers > 500 || numbers < 1)
      return XO_FOOT;

    nbrd.mode = (ans == '2') ? (NBRD_OTHER | NBRD_START) : (NBRD_OTHER | NBRD_START | NBRD_ANONYMOUS);
  }

  vmsg("開始編輯 [看板說明與板主抱負或連署原因]");
  sprintf(path, "tmp/%s.nbrd", cuser.userid);	/* 連署原因的暫存檔案 */
  if (fd = vedit(path, 0))
  {
    unlink(path);
    vmsg(msg_cancel);
    return nbrd_head(xo);
  }

  dir = xo->dir;
  if ((fd = nbrd_stamp(dir, &nbrd, fpath)) < 0)
    return nbrd_head(xo);
  close(fd);

  nbrd.etime = nbrd.btime + days * 86400;
  nbrd.total = numbers;
  strcpy(nbrd.owner, cuser.userid);

  fp = fopen(fpath, "a");
  fprintf(fp, "作者: %s (%s) 站內: 連署系統\n", cuser.userid, cuser.username);
  fprintf(fp, "標題: %s\n", title);
  fprintf(fp, "時間: %s\n\n", Now());

  if (ans == '1')
  {
    fprintf(fp, "英文板名：%s\n", brdname);
    fprintf(fp, "看板分類：%s\n", class);
    fprintf(fp, "看板主題：%s\n", title);
    fprintf(fp, "板主名稱：%s\n", cuser.userid);
    fprintf(fp, "電子信箱：%s\n", cuser.email);
  }
  else
  {
    fprintf(fp, "連署主題：%s\n", title);
  }
  fprintf(fp, "舉辦日期：%s\n", nbrd.date);
  fprintf(fp, "到期天數：%d\n", days);
  fprintf(fp, "需連署人：%d\n", numbers);
  fprintf(fp, "%s", split_line);
  fprintf(fp, "連署說明：\n");
  f_suck(fp, path);
  unlink(path);
  fprintf(fp, split_line);
  fclose(fp);

  rec_add(dir, &nbrd, sizeof(NBRD));

#ifdef SYSOP_START_COSIGN
  vmsg(ans == '1' ? "送交申請了，請等候核准吧" : "連署開始了！");
#else
  vmsg("連署開始了！");
#endif
  return nbrd_init(xo);
}


static int
nbrd_seek(fpath)
  char *fpath;
{
  LOG old;
  int fd;
  int rc = 0;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    while (read(fd, &old, sizeof(LOG)) == sizeof(LOG))
    {
      if (!strcmp(old.userid, cuser.userid) || !str_cmp(old.email, cuser.email))
      {
	rc = 1;
	break;
      }
    }
    close(fd);
  }
  return rc;
}


static void
addreply(hdd, ram)
  NBRD *hdd, *ram;
{
  if (--hdd->total <= 0)
  {
    if (hdd->mode & NBRD_NEWBOARD)	/* 新板連署掛 END */
      hdd->mode |= NBRD_END;
    else				/* 其他連署掛 FINISH */
      hdd->mode |= NBRD_FINISH;
  }
}


static int
nbrd_reply(xo)
  XO *xo;
{
  NBRD *nbrd;
  char *fname, fpath[64], reason[80];
  LOG mail;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
  fname = NULL;

  if (nbrd->mode & (NBRD_FINISH | NBRD_END))
    return XO_NONE;

#ifdef SYSOP_START_COSIGN
  if (!(nbrd->mode & NBRD_START))
  {
    vmsg("尚未開始連署");
    return XO_FOOT;
  }
#endif

  if (time(0) >= nbrd->etime)
  {
    currchrono = nbrd->btime;
    if (nbrd->mode & NBRD_NEWBOARD)	/* 新板連署掛 END */
    {
      if (!(nbrd->mode & NBRD_END))
      {
	nbrd->mode ^= NBRD_END;
	currchrono = nbrd->btime;
	rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);
      }
    }
    else				/* 其他連署掛 FINISH */
    {
      if (!(nbrd->mode & NBRD_FINISH))
      {
	nbrd->mode ^= NBRD_FINISH;
	currchrono = nbrd->btime;
	rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);
      }
    }
    vmsg("連署已經截止了");
    return XO_FOOT;
  }


  /* --------------------------------------------------- */
  /* 檢查是否已經連署過					 */
  /* --------------------------------------------------- */

  nbrd_fpath(fpath, xo->dir, nbrd);
  fname = strrchr(fpath, '@');
  *fname = 'G';

  if (nbrd_seek(fpath))
  {
    vmsg("您已經連署過了！");
    return XO_FOOT;
  }

  /* --------------------------------------------------- */
  /* 開始連署						 */
  /* --------------------------------------------------- */

  *fname = '@';

  if (vans("要加入連署嗎(Y/N)？[N] ") == 'y' && 
    vget(b_lines, 0, "我有話要說：", reason, 65, DOECHO))
  {
    FILE *fp;

    if (fp = fopen(fpath, "a"))
    {
      if (nbrd->mode & NBRD_ANONYMOUS)
	fprintf(fp, "%3d -> " STR_ANONYMOUS "\n    %s\n", nbrd->total, reason);
      else
	fprintf(fp, "%3d -> %s (%s)\n    %s\n", nbrd->total, cuser.userid, cuser.email, reason);
      fclose(fp);
    }

    currchrono = nbrd->btime;
    rec_ref(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime, addreply);

    memset(&mail, 0, sizeof(LOG));
    strcpy(mail.userid, cuser.userid);
    strcpy(mail.email, cuser.email);
    *fname = 'G';
    rec_add(fpath, &mail, sizeof(LOG));

    vmsg("加入連署完成");
    return nbrd_init(xo);
  }

  return XO_FOOT;
}


#ifdef SYSOP_START_COSIGN
static int
nbrd_start(xo)
  XO *xo;
{
  NBRD *nbrd;
  char fpath[64], buf[80], tmp[10];
  time_t etime;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);

  if (nbrd->mode & (NBRD_FINISH | NBRD_END | NBRD_START))
    return XO_NONE;

  if (vans("請確定開始連署(Y/N)？[N] ") != 'y')
    return XO_FOOT;

  nbrd_fpath(fpath, xo->dir, nbrd);
  etime = time(0) + NBRD_DAY_BRD * 86400;

  str_stamp(tmp, &etime);
  sprintf(buf, "開始連署：      到期日期：%s\n", tmp);
  f_cat(fpath, buf);
  f_cat(fpath, split_line);

  nbrd->etime = etime;
  nbrd->mode ^= NBRD_START;
  currchrono = nbrd->btime;
  rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);

  return nbrd_head(xo);
}
#endif


static int
nbrd_finish(xo)
  XO *xo;
{
  NBRD *nbrd;
  char fpath[64], path[64];
  int fd;
  FILE *fp;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);

  if (nbrd->mode & NBRD_FINISH)
    return XO_NONE;

  if (vans("請確定結束連署(Y/N)？[N] ") != 'y')
    return XO_FOOT;

  vmsg("請編輯結束連署原因");
  sprintf(path, "tmp/%s", cuser.userid);	/* 連署原因的暫存檔案 */
  if (fd = vedit(path, 0))
  {
    unlink(path);
    vmsg(msg_cancel);
    return nbrd_head(xo);
  }

  nbrd_fpath(fpath, xo->dir, nbrd);

  f_cat(fpath, "結束連署原因：\n\n");
  fp = fopen(fpath, "a");
  f_suck(fp, path);
  fclose(fp);
  f_cat(fpath, split_line);
  unlink(path);

  nbrd->mode ^= NBRD_FINISH;
  currchrono = nbrd->btime;
  rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);

  return nbrd_head(xo);
}


static int			/* 1:開板成功 */
nbrd_newbrd(nbrd)		/* 開新板 */
  NBRD *nbrd;
{
  BRD newboard;
  ACCT acct;

  /* itoc.030519: 避免重覆開板 */
  if (brd_bno(nbrd->brdname) >= 0)
  {
    vmsg("已有此板");
    return 1;
  }

  memset(&newboard, 0, sizeof(BRD));

  /* itoc.010805: 新看板預設 battr = 不轉信; postlevel = PERM_POST; 看板板主為提起連署者 */
  newboard.battr = BRD_NOTRAN;
  newboard.postlevel = PERM_POST;
  strcpy(newboard.brdname, nbrd->brdname);
  strcpy(newboard.class, nbrd->class);
  strcpy(newboard.title, nbrd->title);
  strcpy(newboard.BM, nbrd->owner);

  if (acct_load(&acct, nbrd->owner) >= 0)
    acct_setperm(&acct, PERM_BM, 0);

  if (brd_new(&newboard) < 0)
    return 0;

  vmsg("新板成立，記著加入分類群組");
  return 1;
}


static int
nbrd_open(xo)		/* itoc.010805: 開新板連署，連署完畢開新看板 */
  XO *xo;
{
  NBRD *nbrd;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);

  if (nbrd->mode & NBRD_FINISH || !(nbrd->mode & NBRD_NEWBOARD))
    return XO_NONE;

  if (vans("請確定開啟看板(Y/N)？[N] ") == 'y')
  {
    if (nbrd_newbrd(nbrd))
    {
      nbrd->mode ^= NBRD_FINISH;
      currchrono = nbrd->btime;
      rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);
    }
    return nbrd_head(xo);
  }

  return XO_FOOT;
}


static int
nbrd_browse(xo)
  XO *xo;
{
  int key;
  NBRD *nbrd;
  char fpath[80];

  /* itoc.010304: 為了讓閱讀到一半也可以加入連署，考慮 more 傳回值 */
  for (;;)
  {
    nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
    nbrd_fpath(fpath, xo->dir, nbrd);

    if ((key = more(fpath, FOOTER_COSIGN)) < 0)
      break;

    if (!key)
      key = vkey();

    switch (key)
    {
    case KEY_UP:
    case KEY_PGUP:
    case '[':
    case 'k':
      key = xo->pos - 1;

      if (key < 0)
        break;

      xo->pos = key;

      if (key <= xo->top)
      {
	xo->top = (key / XO_TALL) * XO_TALL;
	nbrd_load(xo);
      }
      continue;

    case KEY_DOWN:
    case KEY_PGDN:
    case ']':
    case 'j':
    case ' ':
      key = xo->pos + 1;

      if (key >= xo->max)
        break;

      xo->pos = key;

      if (key >= xo->top + XO_TALL)
      {
	xo->top = (key / XO_TALL) * XO_TALL;
	nbrd_load(xo);
      }
      continue;

    case 'y':
    case 'r':
      nbrd_reply(xo);
      break;

    case 'h':
      xo_help("cosign");
      break;
    }
    break;
  }

  return nbrd_head(xo);
}


static int
nbrd_delete(xo)
  XO *xo;
{
  NBRD *nbrd;
  char *fname, fpath[80];
  char *list = "@G";		/* itoc.註解: 清 newbrd file */

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
  if (strcmp(cuser.userid, nbrd->owner) && !HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  if (vans(msg_del_ny) != 'y')
    return XO_FOOT;

  nbrd_fpath(fpath, xo->dir, nbrd);
  fname = strrchr(fpath, '@');
  while (*fname = *list++)
  {
    unlink(fpath);	/* Thor: 確定名字就砍 */
  }

  currchrono = nbrd->btime;
  rec_del(xo->dir, sizeof(NBRD), xo->pos, cmpbtime);
  return nbrd_init(xo);
}


static int
nbrd_edit(xo)
  XO *xo;
{
  if (HAS_PERM(PERM_ALLBOARD))
  {
    char fpath[64];
    NBRD *nbrd;

    nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
    nbrd_fpath(fpath, xo->dir, nbrd);
    vedit(fpath, 0);
    return nbrd_head(xo);
  }

  return XO_NONE;
}


static int
nbrd_setup(xo)
  XO *xo;
{
  int numbers;
  char ans[6];
  NBRD *nbrd, newnh;

  if (!HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  vs_bar("連署設定");
  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
  memcpy(&newnh, nbrd, sizeof(NBRD));

  prints("看板名稱：%s\n看板說明：%4.4s %s\n連署發起：%s\n",
    newnh.brdname, newnh.class, newnh.title, newnh.owner);
  prints("開始時間：%s\n", Btime(&newnh.btime));
  prints("結束時間：%s\n", Btime(&newnh.etime));
  prints("還需人數：%d\n", newnh.total);

  if (vget(8, 0, "(E)設定 (Q)取消？[Q] ", ans, 3, LCECHO) == 'e')
  {
    vget(11, 0, MSG_BID, newnh.brdname, BNLEN + 1, GCARRY);
    vget(12, 0, "看板分類：", newnh.class, sizeof(newnh.class), GCARRY);
    vget(13, 0, "看板主題：", newnh.title, sizeof(newnh.title), GCARRY);
    sprintf(ans, "%d", newnh.total);
    vget(14, 0, "連署人數：", ans, 6, GCARRY);
    numbers = atoi(ans);
    if (numbers <= 500 && numbers >= 1)
      newnh.total = numbers;

    if (memcmp(&newnh, nbrd, sizeof(newnh)) && vans(msg_sure_ny) == 'y')
    {
      memcpy(nbrd, &newnh, sizeof(NBRD));
      currchrono = nbrd->btime;
      rec_put(xo->dir, nbrd, sizeof(NBRD), xo->pos, cmpbtime);
    }
  }

  return nbrd_head(xo);
}


static int
nbrd_uquery(xo)
  XO *xo;
{
  NBRD *nbrd;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);

  move(1, 0);
  clrtobot();
  my_query(nbrd->owner);
  return nbrd_head(xo);
}


static int
nbrd_usetup(xo)
  XO *xo;
{
  NBRD *nbrd;
  ACCT acct;

  if (!HAS_PERM(PERM_ALLACCT))
    return XO_NONE;

  nbrd = (NBRD *) xo_pool + (xo->pos - xo->top);
  if (acct_load(&acct, nbrd->owner) < 0)
    return XO_NONE;

  move(3, 0);
  acct_setup(&acct, 1);
  return nbrd_head(xo);
}


static int
nbrd_help(xo)
  XO *xo;
{
  xo_help("cosign");
  return nbrd_head(xo);
}


static KeyFunc nbrd_cb[] =
{
  XO_INIT, nbrd_init,
  XO_LOAD, nbrd_load,
  XO_HEAD, nbrd_head,
  XO_BODY, nbrd_body,

  'y', nbrd_reply,
  'r', nbrd_browse,
  'o', nbrd_open,
#ifdef SYSOP_START_COSIGN
  's', nbrd_start,
#endif
  'c', nbrd_finish,
  'd', nbrd_delete,
  'E', nbrd_edit,
  'B', nbrd_setup,

  Ctrl('P'), nbrd_add,
  Ctrl('Q'), nbrd_uquery,
  Ctrl('O'), nbrd_usetup,

  'h', nbrd_help
};


int
XoNewBoard()
{
  XO *xo;
  char fpath[64];

  sprintf(fpath, "run/newbrd/%s", fn_dir);
  xz[XZ_COSIGN - XO_ZONE].xo = xo = xo_new(fpath);
  xz[XZ_COSIGN - XO_ZONE].cb = nbrd_cb;
  xo->key = XZ_COSIGN;
  xover(XZ_COSIGN);
  free(xo);

  return 0;
}
#endif	/* HAVE_COSIGN */
