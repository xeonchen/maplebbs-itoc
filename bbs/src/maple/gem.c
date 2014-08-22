/*-------------------------------------------------------*/
/* gem.c	( NTHU CS MapleBBS Ver 3.00 )		 */
/*-------------------------------------------------------*/
/* target : 精華區閱讀、編選			     	 */
/* create : 95/03/29				     	 */
/* update : 97/02/02				     	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern XZ xz[];
extern char xo_pool[];
extern char brd_bits[];

extern int TagNum;
extern TagItem TagList[];


#define	GEM_WAY		3
static int gem_way;		/* 0:只印標題 1:標題加檔名 2:標題加編者 */

static int GemBufferNum;	/* Thor.990414: 提前宣告，用於gem_head */

static char GemAnchor[64];	/* 定錨區的路徑 */
static char GemSailor[20];	/* 定錨區的標題 */

static int gem_add_all();
static int gem_paste();
static int gem_anchor();


static void
gem_item(num, hdr, level)
  int num;
  HDR *hdr;
  int level;
{
  int xmode, gtype;

  /* ◎☆★◇◆□■▽▼ : A1B7 ... */

  xmode = hdr->xmode;
  gtype = (char) 0xba;

  /* 目錄用實心，不是目錄用空心 */
  if (xmode & GEM_FOLDER)			/* 文章:◇ 卷宗:◆ */
    gtype += 1;

  if (hdr->xname[0] == '@')			/* 資料:☆ 分類:★ */
    gtype -= 2;
  else if (xmode & (GEM_BOARD | GEM_LINE))	/* 分隔:□ 看板:■ */
    gtype += 2;

  prints("%6d%c%c\241%c ", num, tag_char(hdr->chrono), xmode & GEM_RESTRICT ? ')' : ' ', gtype);

  if ((xmode & GEM_RESTRICT) && !(level & GEM_M_BIT))
    outs(MSG_DATA_CLOAK);				/* itoc.000319: 限制級文章保密 */
  else if (gem_way == 0)
    prints("%.*s\n", d_cols + 64, hdr->title);
  else
    prints("%-*.*s%-13s%s\n", d_cols + 46, d_cols + 45, hdr->title, (gem_way == 1 ? hdr->xname : hdr->owner), hdr->date);
}


static int
gem_body(xo)
  XO *xo;
{
  HDR *hdr;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    outs("\n\n《精華區》尚在吸取天地間的日精月華 :)");

    if (xo->key & GEM_W_BIT)
    {
      switch (vans("(A)新增資料 (P)貼複 (G)海錨功\能 [N]無所事事 "))
      {
      case 'a':
	max = gem_add_all(xo);
    	if (xo->max > 0)
	  return max;
	break;

      case 'p':
	max = gem_paste(xo);
	if (xo->max > 0)
	  return max;
	break;

      case 'g':
	gem_anchor(xo);
	break;
      }
    }
    else
    {
      vmsg(NULL);
    }
    return XO_QUIT;
  }

  hdr = (HDR *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  move(3, 0);
  tail = xo->key;	/* 借用 tail */
  do
  {
    gem_item(++num, hdr++, tail);
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
}


static int
gem_head(xo)
  XO *xo;
{
  char buf[20];

  vs_head("精華文章", xo->xyz);

  if ((xo->key & GEM_W_BIT) && GemBufferNum > 0)
    sprintf(buf, "(剪貼簿 %d 篇)", GemBufferNum);
  else
    buf[0] = '\0';

  prints(NECKER_GEM, buf, d_cols, "");
  return gem_body(xo);
}


static int
gem_toggle(xo)
  XO *xo;
{
  gem_way++;
  gem_way %= GEM_WAY;

  /* 只有站長能看到檔名 */
  if (!(xo->key & GEM_X_BIT) && gem_way == 1)
    gem_way++;

  return gem_body(xo);
}


static int
gem_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(HDR));
  return gem_head(xo);
}


static int
gem_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(HDR));
  return gem_body(xo);
}


/* ----------------------------------------------------- */
/* gem_check : attribute check out		 	 */
/* ----------------------------------------------------- */


#define	GEM_PLAIN	0x01	/* 預期是 plain text */


static HDR *		/* NULL:無權讀取 */
gem_check(xo, fpath, op)
  XO *xo;
  char *fpath;
  int op;
{
  HDR *hdr;
  int gtype;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);
  gtype = hdr->xmode;

  if ((gtype & GEM_RESTRICT) && !(xo->key & GEM_M_BIT))
    return NULL;

  if (op && (gtype & GEM_LINE))
    return NULL;

  if ((op & GEM_PLAIN) && (gtype & GEM_FOLDER))
    return NULL;

  if (fpath)
  {
    if (gtype & GEM_BOARD)
      gem_fpath(fpath, hdr->xname, fn_dir);
    else
      hdr_fpath(fpath, xo->dir, hdr);
  }
  return hdr;
}


#if 0
static int	/* -1:不是看板精華區  >=0:bno */
gem_bno(xo)
  XO *xo;
{
  char *dir, *str;
  int bno;

  /* 由 xo->dir 找出目前在哪一個板的精華區 */
  dir = xo->dir;

  /* 檢查是否為 gem/brd/brdname/.DIR 的格式，
     避免若在 gem/.DIR 或 usr/u/userid/gem/.DIR 會造成錯誤 */
  if (dir[0] == 'g' && dir[4] == 'b')
  {
    dir += 8;	/* 跳過 "gem/brd/" */
    if (str = strchr(dir, '/'))
    {
      *str = '\0';
      bno = brd_bno(dir);
      *str = '/';
      return bno;
    }
  }

  return -1;
}
#endif


/* ----------------------------------------------------- */
/* 資料之新增：append / insert				 */
/* ----------------------------------------------------- */


/* itoc.060605:
  1. 在 hdr_stamp() 中用檔案是否已經存在的方法來確認是否為 unique chrono，
     然而精華區的檔案可以有三種 token (F/A/L)，所以可以會發生 F1234567/A1234567/L1234567
     三個不同檔名但卻相同 chrono 的錯誤。
  2. Tagger() 要求 unique chrono，否則如果 tag 到相同 chrono 的檔案，該 chrono 的所有檔案
     都會 tag 失效。
  3. 所以就從 hdr_stamp() 改寫一隻 gem_hdr_stamp()。
*/

static int
gem_hdr_stamp(folder, token, hdr, fpath)
  char *folder;
  int token;		/* 只會有 F/A/L | HDR_LINK/HDR_COPY */
  HDR *hdr;
  char *fpath;
{
  char *fname, *family;
  int rc, chrono;
  char *flink, buf[128];
  int i;
  int Token;		/* token 的大寫 */
  char *ptr;		/* token 所在處 */
  char *pool = "FAL";
  static time_t chrono0;

  flink = NULL;
  if (token & (HDR_LINK | HDR_COPY))
  {
    flink = fpath;
    fpath = buf;
  }

  fname = fpath;
  while (rc = *folder++)
  {
    *fname++ = rc;
    if (rc == '/')
      family = fname;
  }
  if (*family != '.')
  {
    fname = family;
    family -= 2;
  }
  else
  {
    fname = family + 1;
    *fname++ = '/';
  }

  Token = token & 0xdf;	/* 變大寫 */
  ptr = fname;
  fname++;

  chrono = time(0);

  /* itoc.060605: 由於精華區往往有大量複製貼上，所以就乾脆把上一次最後的 chrono 記下來，這次就不用從頭 try */
  if (chrono <= chrono0)
    chrono = chrono0 + 1;

  for (;;)
  {
    *family = radix32[chrono & 31];
    archiv32(chrono, fname);

    /* 要確保這個 chrono 的 F/A/L 都沒有檔案 */
    for (i = 0; i < 3; i++)
    {
      if (pool[i] != Token)
      {
	*ptr = pool[i];
	if (dashf(fpath))
	  goto next_chrono;
      }
    }

    *ptr = Token;

    if (flink)
    {
      if (token & HDR_LINK)
	rc = f_ln(flink, fpath);
      else
        rc = f_cp(flink, fpath, O_EXCL);
    }
    else
    {
      rc = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600);
    }

    if (rc >= 0)
    {
      memset(hdr, 0, sizeof(HDR));
      hdr->chrono = chrono;
      str_stamp(hdr->date, &hdr->chrono);
      strcpy(hdr->xname, --fname);
      break;
    }

    if (errno != EEXIST)
      break;

next_chrono:
    chrono++;
  }

  chrono0 = chrono;

  return rc;
}


void
brd2gem(brd, gem)
  BRD *brd;
  HDR *gem;
{
  memset(gem, 0, sizeof(HDR));
  time(&gem->chrono);
  str_stamp(gem->date, &gem->chrono);
  strcpy(gem->xname, brd->brdname);
  sprintf(gem->title, "%-13s%-5s%s", brd->brdname, brd->class, brd->title);
  gem->xmode = GEM_BOARD | GEM_FOLDER;
}


#if 0	/* itoc.010218: 換新的 gem_log() */
static void
gem_log(folder, action, hdr)
  char *folder;
  char *action;
  HDR *hdr;
{
  char fpath[64], buf[256];

  if (hdr->xmode & (GEM_RESTRICT | GEM_RESERVED))
    return;

  str_folder(fpath, folder, "@/@log");
  sprintf(buf, "[%s] %s (%s) %s\n%s\n\n",
    action, hdr->xname, Now(), cuser.userid, hdr->title);
  f_cat(fpath, buf);
}
#endif


static void
gem_log(folder, action, hdr)
  char *folder;
  char *action;
  HDR *hdr;
{
  char fpath1[64], fpath2[64];
  FILE *fp1, *fp2;

  if (hdr->xmode & (GEM_RESTRICT | GEM_RESERVED))
    return;

  /* mv @log @log.old */
  str_folder(fpath1, folder, "@/@log");
  str_folder(fpath2, folder, "@/@log.old");
  f_mv(fpath1, fpath2);  

  if (!(fp1 = fopen(fpath1, "a")))
    return;

  /* 把新的異動放在最上面，並依異動順序編號 */

  fprintf(fp1, "<01> %s %-12s [%s] %s\n     %s\n\n", Now(),
    cuser.userid, action, hdr->xname, hdr->title);

  if (fp2 = fopen(fpath2, "r"))
  {
    char buf[STRLEN];
    int i = 6;				/* 從第二篇開始 */
    int j;

    while (fgets(buf, STRLEN, fp2))
    {
      if (++i > 63)			/* 只保留最新 20 筆異動 */
	break;

      j = i % 3;
      if (j == 1)			/* 第一行 */
	fprintf(fp1, "<%02d> %s", i / 3, buf + 5);
      else if (j == 2)			/* 第二行 */
	fprintf(fp1, "%s\n", buf);
					/* 第三行是空行 */
    }
    fclose(fp2);
  }
  fclose(fp1);
}


static int
gem_add(xo, gtype)
  XO *xo;
  int gtype;
{
  int level, fd, ans;
  char title[TTLEN + 1], fpath[64], *dir;
  HDR hdr;

  level = xo->key;
  if (!(level & GEM_W_BIT))
    return XO_NONE;

  if (!gtype)
  {
    gtype = vans((level & GEM_X_BIT) ?
      /* "新增 A)rticle B)oard C)lass D)ata F)older L)ine P)aste Q)uit [Q] " : */
      "新增 (A)文章 (B)看板 (C)分類 (D)資料 (F)卷宗 (L)分隔 (P)貼複 (Q)取消？[Q] " : 
      "新增 (A)文章 (F)卷宗 (L)分隔 (P)貼複 (Q)取消？[Q] ");
  }

  if (gtype == 'p')
    return gem_paste(xo);

  if (gtype != 'a' && gtype != 'f' && gtype != 'l' && 
    (!(level & GEM_X_BIT) || (gtype != 'b' && gtype != 'c' && gtype != 'd')))
    return XO_FOOT;

  dir = xo->dir;
  fd = -1;

  if (gtype == 'b')
  {
    BRD *brd;

    if (!(brd = ask_board(fpath, BRD_L_BIT, NULL)))
      return gem_head(xo);

    brd2gem(brd, &hdr);
    gtype = 0;
  }
  else
  {
    if (!vget(b_lines, 0, "標題：", title, TTLEN + 1, DOECHO))
      return XO_FOOT;

    if (gtype == 'c' || gtype == 'd')
    {
      if (!vget(b_lines, 0, "檔名：", fpath, BNLEN + 1, DOECHO))
	return XO_FOOT;

      if (strchr(fpath, '/'))
      {
	zmsg("不合法的檔案名稱");
	return XO_FOOT;
      }

      memset(&hdr, 0, sizeof(HDR));
      time(&hdr.chrono);
      str_stamp(hdr.date, &hdr.chrono);
      sprintf(hdr.xname, "@%s", fpath);
      if (gtype == 'c')
      {
	strcat(fpath, "/");
	sprintf(hdr.title, "%-13s分類 □ %.50s", fpath, title);
	hdr.xmode = GEM_FOLDER;
      }
      else
      {
	strcpy(hdr.title, title);
	hdr.xmode = 0;
      }
      gtype = 1;
    }
    else
    {
      if ((fd = gem_hdr_stamp(dir, gtype, &hdr, fpath)) < 0)
	return XO_FOOT;
      close(fd);

      if (gtype == 'a')
      {
	if (vedit(fpath, 0))	/* Thor.981020: 注意被talk的問題 */
	{
	  unlink(fpath);
	  zmsg(msg_cancel);
	  return gem_head(xo);
	}
	gtype = 0;
      }
      else if (gtype == 'f')
      {
	gtype = GEM_FOLDER;
      }
      else if (gtype == 'l')
      {
	gtype = GEM_LINE;
      }

      hdr.xmode = gtype;
      strcpy(hdr.title, title);
    }
  }

  /* ans = vans("存放位置 A)ppend I)nsert N)ext Q)uit [A] "); */
  ans = vans("存放位置 A)加到最後 I/N)插入目前位置 Q)離開 [A] ");

  if (ans == 'q')
  {
    if (fd >= 0)
      unlink(fpath);
    return (gtype ? XO_FOOT : gem_head(xo));
  }

  strcpy(hdr.owner, cuser.userid);

  if (ans == 'i' || ans == 'n')
    rec_ins(dir, &hdr, sizeof(HDR), xo->pos + (ans == 'n'), 1);
  else
    rec_add(dir, &hdr, sizeof(HDR));

  gem_log(dir, "新增", &hdr);

  return (gtype ? gem_load(xo) : gem_init(xo));
}


static int
gem_add_all(xo)
  XO *xo;
{
  return gem_add(xo, 0);
}


static int
gem_add_article(xo)		/* itoc.010419: 快速鍵 */
  XO *xo;
{
  return gem_add(xo, 'a');
}


static int
gem_add_folder(xo)		/* itoc.010419: 快速鍵 */
  XO *xo;
{
  return gem_add(xo, 'f');
}


/* ----------------------------------------------------- */
/* 資料之修改：edit / title				 */
/* ----------------------------------------------------- */


static int
gem_edit(xo)
  XO *xo;
{
  int level;
  char fpath[64];
  HDR *hdr;

  if (!(hdr = gem_check(xo, fpath, GEM_PLAIN)))
    return XO_NONE;

  level = xo->key;

  if (!(level & GEM_W_BIT) || ((hdr->xmode & GEM_RESERVED) && !(level & GEM_X_BIT)))
  {
    vedit(fpath, -1);
  }
  else
  {
    if (vedit(fpath, 0) >= 0)
      gem_log(xo->dir, "修改", hdr);
  }

  return gem_head(xo);
}


static int
gem_title(xo)
  XO *xo;
{
  HDR *fhdr, mhdr;
  int pos, cur;

  if (!(xo->key & GEM_W_BIT) || !(fhdr = gem_check(xo, NULL, 0)))
    return XO_NONE;

  memcpy(&mhdr, fhdr, sizeof(HDR));

  vget(b_lines, 0, "標題：", mhdr.title, TTLEN + 1, GCARRY);

  if (xo->key & GEM_X_BIT)
  {
    vget(b_lines, 0, "編者：", mhdr.owner, IDLEN + 1, GCARRY);
    /* vget(b_lines, 0, "暱稱：", mhdr.nick, sizeof(mhdr.nick), GCARRY); */	/* 精華區此欄位為空 */
    vget(b_lines, 0, "日期：", mhdr.date, sizeof(mhdr.date), GCARRY);
  }

  if (memcmp(fhdr, &mhdr, sizeof(HDR)) && vans(msg_sure_ny) == 'y')
  {
    pos = xo->pos;
    cur = pos - xo->top;

    memcpy(fhdr, &mhdr, sizeof(HDR));
    rec_put(xo->dir, fhdr, sizeof(HDR), pos, NULL);

    move(3 + cur, 0);
    gem_item(++pos, fhdr, xo->key);

    gem_log(xo->dir, "標題", fhdr);
  }
  return XO_FOOT;
}


static int
gem_refuse(xo)
  XO *xo;
{
  HDR *hdr;
  int num;

  if ((xo->key & GEM_M_BIT) && (hdr = gem_check(xo, NULL, 0)))
  {
    hdr->xmode ^= GEM_RESTRICT;

    num = xo->pos;
    rec_put(xo->dir, hdr, sizeof(HDR), num, NULL);
    num++;
    move(num - xo->top + 2, 0);
    gem_item(num, hdr, xo->key);
  }

  return XO_NONE;
}


static int
gem_state(xo)
  XO *xo;
{
  HDR *hdr;
  char fpath[64];
  struct stat st;

  if ((xo->key & GEM_W_BIT) && (hdr = gem_check(xo, fpath, 0)))
  {
    move(12, 0);
    clrtobot();
    prints("\nDir : %s", xo->dir);
    prints("\nName: %s", hdr->xname);
    prints("\nFile: %s", fpath);

    if (!stat(fpath, &st))
    {
      prints("\nTime: %s", Btime(&st.st_mtime));
      prints("\nSize: %d", st.st_size);
    }

    vmsg(NULL);
    return gem_body(xo);
  }

  return XO_NONE;
}


/* ----------------------------------------------------- */
/* 資料之瀏覽：edit / title				 */
/* ----------------------------------------------------- */


int			/* -1:無權限 */
gem_link(brdname)	/* 檢查連結去其他看板精華區的權限 */
  char *brdname;
{
  int bno, level;

  if ((bno = brd_bno(brdname)) < 0 || !((bno = brd_bits[bno]) & BRD_R_BIT))
    return -1;

  level = 0;
  if (bno & BRD_X_BIT)
    level ^= GEM_W_BIT;
  if (HAS_PERM(PERM_SYSOP))
    level ^= GEM_X_BIT;
  if (bno & BRD_M_BIT)
    level ^= GEM_M_BIT;

  return level;
}


static int
gem_browse(xo)
  XO *xo;
{
  HDR *hdr;
  int op, xmode;
  char fpath[64], title[TTLEN + 1], *ptr;

  op = 0;

  for (;;)
  {    
    if (!(hdr = gem_check(xo, fpath, op)))
      break;

    xmode = hdr->xmode;

    /* browse folder */

    if (xmode & GEM_FOLDER)
    {
      strcpy(title, hdr->title);

      if (xmode & GEM_BOARD)
      {
	if ((op = gem_link(hdr->xname)) < 0)
	{
	  vmsg("對不起，此板精華區只准板友進入，請向板主申請入境許\可");
	  return XO_FOOT;
	}
      }
      else			/* 一般卷宗才有小板主 */
      {
	op = xo->key;		/* 繼承母卷宗的權限 */

	/* itoc.011217: [userA/userB 的多位小板主模式也適用 */
	if ((ptr = strrchr(title, '[')) && is_bm(ptr + 1, cuser.userid))
	  op |= GEM_W_BIT | GEM_M_BIT;
      }

      XoGem(fpath, title, op);
      return gem_init(xo);
    }

    /* browse article */

    /* Thor.990204: 為考慮more 傳回值 */   
    if ((xmode = more(fpath, FOOTER_GEM)) < 0)
      break;

    op = GEM_PLAIN;

re_key:
    switch (xo_getch(xo, xmode))
    {
    case XO_BODY:
      continue;

    case '/':
      if (vget(b_lines, 0, "搜尋：", hunt, sizeof(hunt), DOECHO))
      {
	more(fpath, FOOTER_GEM);
	goto re_key;
      }
      continue;

    case 'E':
      return gem_edit(xo);

    case 'C':
      {
	FILE *fp;
	if (fp = tbf_open())
	{
	  f_suck(fp, fpath);
	  fclose(fp);
	}
      }
      break;

    case 'h':
      xo_help("gem");
      break;
    }
    break;
  }

  return gem_head(xo);
}


/* ----------------------------------------------------- */
/* 精華區之刪除						 */
/* ----------------------------------------------------- */


static int
chkgem(hdr)
  HDR *hdr;
{
  return (hdr->xmode & (GEM_RESTRICT | GEM_RESERVED));
}


static int
vfygem(hdr, pos)
  HDR *hdr;
  int pos;
{
  return (Tagger(hdr->chrono, pos, TAG_NIN) || chkgem(hdr));
}


static void
delgem(xo, hdr)
  XO *xo;
  HDR *hdr;
{
  char folder[64];
  HDR fhdr;
  FILE *fp;

  if (hdr->xmode & GEM_FOLDER)		/* 卷宗/分類/看板 */
  {
    hdr_fpath(folder, xo->dir, hdr);

    /* Kyo.050328: 定錨區被刪除時要拔錨 */
    if (!strcmp(GemAnchor, folder))
      GemAnchor[0] = '\0';

    /* 卷宗要進子目錄刪除；看板/分類則不需要 */
    if (hdr->xmode == GEM_FOLDER && hdr->xname[0] != '@')
    {
      if (fp = fopen(folder, "r"))
      {
	while (fread(&fhdr, sizeof(HDR), 1, fp) == 1)
	  delgem(xo, &fhdr);

	fclose(fp);
	unlink(folder);
      }
    }
  }
  else					/* 文章/資料 */
  {
    /* 文章要刪除檔案；資料則不刪除檔案 */
    if (hdr->xname[0] != '@')
    {
      hdr_fpath(folder, xo->dir, hdr);
      unlink(folder);
    }
  }
}


static int
gem_delete(xo)
  XO *xo;
{
  HDR *hdr;
  int xmode;

  if (!(xo->key & GEM_W_BIT) || !gem_check(xo, NULL, 0))
    return XO_NONE;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);
  xmode = hdr->xmode;

  if (hdr->xmode & (GEM_RESTRICT | GEM_RESERVED))
    return XO_NONE;

  if (vans(msg_del_ny) == 'y')
  {
    delgem(xo, hdr);

    if (!rec_del(xo->dir, sizeof(HDR), xo->pos, NULL))
    {
      gem_log(xo->dir, "刪除", hdr);
      return gem_load(xo);
    }
  }

  return XO_FOOT;
}


static int
gem_rangedel(xo)	/* itoc.010726: 提供區段刪除 */
  XO *xo;
{
  if (!(xo->key & GEM_W_BIT) || !gem_check(xo, NULL, 0))
    return XO_NONE;

  return xo_rangedel(xo, sizeof(HDR), chkgem, delgem);
}


static int
gem_prune(xo)
  XO *xo;
{
  if (!(xo->key & GEM_W_BIT))
    return XO_NONE;
  return xo_prune(xo, sizeof(HDR), vfygem, delgem);
}


/* ----------------------------------------------------- */
/* 精華區之複製、貼上、移動				 */
/* ----------------------------------------------------- */


static char GemFolder[64];

static HDR *GemBuffer;
/* static int GemBufferNum; */	/* Thor.990414: 提前宣告給gem_head用 */


/* 配置足夠的空間放入 header */


static HDR *
gbuf_malloc(num)
  int num;
{
  HDR *gbuf;
  static int GemBufferSiz;	/* 目前 GemBuffer 的 size 是 GemBufferSiz * sizeof(HDR) */

  if (gbuf = GemBuffer)
  {
    if (GemBufferSiz < num)
    {
      num += (num >> 1);
      GemBufferSiz = num;
      GemBuffer = gbuf = (HDR *) realloc(gbuf, sizeof(HDR) * num);
    }
  }
  else
  {
    GemBufferSiz = num;
    GemBuffer = gbuf = (HDR *) malloc(sizeof(HDR) * num);
  }

  return gbuf;
}


void
gem_buffer(dir, hdr, fchk)
  char *dir;
  HDR *hdr;			/* NULL 代表放入 TagList, 否則將傳入的放入 */
  int (*fchk)();		/* 允許放入 gbuf 的條件 */
{
  int max, locus, num;
  HDR *gbuf, buf;

  if (hdr)
  {
    max = 1;
  }
  else
  {
    max = TagNum;
    if (max <= 0)
      return;
  }

  gbuf = gbuf_malloc(max);
  num = 0;

  if (hdr)
  {
    if (!fchk || fchk(hdr))
    {
      memcpy(gbuf, hdr, sizeof(HDR));
      num++;
    }
  }
  else
  {
    locus = 0;
    do
    {
      EnumTag(&buf, dir, locus, sizeof(HDR));

      if (!fchk || fchk(&buf))
      {
	memcpy(gbuf + num, &buf, sizeof(HDR));
	num++;
      }
    } while (++locus < max);
  }

  strcpy(GemFolder, dir);
  GemBufferNum = num;
}


static int IamBM;

static int
chkgemrestrict(hdr)
  HDR *hdr;
{
  if (hdr->xmode & GEM_BOARD)		/* 看板不能被複製/貼上 */
    return 0;

  if ((hdr->xmode & GEM_RESTRICT) && !IamBM)
    return 0;

  return 1;
}


static int
gem_copy(xo)
  XO *xo;
{
  int tag;

  tag = AskTag("精華區拷貝");

  if (tag < 0)
    return XO_FOOT;

  IamBM = (xo->key & GEM_M_BIT);
  gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), chkgemrestrict);

  zmsg("拷貝完成。[注意] 貼上後才能刪除原文！");
  /* return XO_FOOT; */
  return gem_head(xo);		/* Thor.990414: 讓剪貼篇數更新 */
}


static inline int
gem_extend(xo, num)
  XO *xo;
  int num;
{
  char *dir, fpath[64], gpath[64];
  FILE *fp;
  time_t chrono;
  HDR *hdr;

  if (!(hdr = gem_check(xo, fpath, GEM_PLAIN)))
    return -1;

  if (!(fp = fopen(fpath, "a")))
    return -1;

  dir = xo->dir;
  chrono = hdr->chrono;

  for (hdr = GemBuffer; num--; hdr++)
  {
    if ((hdr->chrono != chrono) && !(hdr->xmode & (GEM_FOLDER | GEM_RESTRICT | GEM_RESERVED)))
    {
      hdr_fpath(gpath, GemFolder, hdr);	/* itoc.010924: 修正不同目錄會 extend 失敗 */
      fputs(str_line, fp);
      f_suck(fp, gpath);
    }
  }

  fclose(fp);
  return 0;
}


static int					/* 1: 無窮迴圈  0: 合法 */
invalid_loop(srcDir, dstDir, hdr, depth)	/* itoc.010727: 檢查是否會造成無窮迴圈 for gem_paste() */
  char *srcDir, *dstDir;
  HDR *hdr;
  int depth;				/* 0: 遞迴第一圈  1: 遞迴中 */
{
  static int valid;

  int fd;
  char fpath1[64], fpath2[64];
  HDR fhdr;

  if (!depth)
  {
    if (!(hdr->xmode & GEM_FOLDER))	/* plain text */
      return 0;

    str_folder(fpath1, srcDir, fn_dir);
    str_folder(fpath2, dstDir, fn_dir);

    if (strcmp(fpath1, fpath2))		/* 跨區拷貝一定不會造成無窮迴圈 */
      return 0;

    hdr_fpath(fpath1, srcDir, hdr);	/* 把自己拷到自己裡面 */
    if (!strcmp(fpath1, dstDir))
      return 1;

    valid = 0;
  }
  else
  {
    if (valid)		/* 在某一個遞迴中找到非法證據就停止搜證工作 */
      return 1;

    hdr_fpath(fpath1, srcDir, hdr);
  }

  if ((fd = open(fpath1, O_RDONLY)) >= 0)
  {
    while (read(fd, &fhdr, sizeof(HDR)) == sizeof(HDR))
    {  
      if (fhdr.xmode & GEM_FOLDER)	/* plain text 不會造成無窮迴圈 */
      {
	hdr_fpath(fpath2, srcDir, &fhdr);
	if (!strcmp(fpath2, dstDir))
	{
	  valid = 1;
	  return 1;
	}

	/* recursive 地一層一層目錄進去檢查是否會造成無窮迴圈 */
	invalid_loop(fpath1, dstDir, &fhdr, 1);
      }
    }
    close(fd);
  }

  return valid;
}


static void
gem_do_paste(srcDir, dstDir, hdr, pos)		/* itoc.010725: for gem_paste() */
  char *srcDir;		/* source folder */
  char *dstDir;		/* destination folder */
  HDR *hdr;		/* source hdr */
  int pos;		/* -1: 附加在最後  >=0: 貼上的位置 */
{
  int xmode, fsize;
  char folder[64], fpath[64];
  HDR fhdr, *data, *head, *tail;

  xmode = hdr->xmode;

  if (xmode & GEM_FOLDER)	/* 卷宗/分類 */
  {
    /* 在複製/貼上後一律變成卷宗，因為分類是站長專用特殊用途的 */
    if ((fsize = gem_hdr_stamp(dstDir, 'F', &fhdr, fpath)) < 0)
      return;
    close(fsize);

    fhdr.xmode = GEM_FOLDER;
  }
  else if (xmode & GEM_LINE)	/* 分隔 */
  {
    if ((fsize = gem_hdr_stamp(dstDir, 'L', &fhdr, fpath)) < 0)
      return;
    close(fsize);

    fhdr.xmode = GEM_LINE;
  }
  else				/* 文章/資料 */
  {
    hdr_fpath(folder, srcDir, hdr);

    /* 在複製/貼上後一律變成文章，因為資料是站長專用特殊用途的 */
    gem_hdr_stamp(dstDir, HDR_COPY | 'A', &fhdr, folder);
  }

  if (hdr->xmode & GEM_RESTRICT)
    fhdr.xmode ^= GEM_RESTRICT;
  strcpy(fhdr.owner, cuser.userid);
  strcpy(fhdr.title, hdr->title);
  if (pos < 0)
    rec_add(dstDir, &fhdr, sizeof(HDR));
  else
    rec_ins(dstDir, &fhdr, sizeof(HDR), pos, 1);
  gem_log(dstDir, "複製", &fhdr);

  if (xmode & GEM_FOLDER)	/* 卷宗/分類 */
  {
    /* 建立完自己這個卷宗以後，再 recursive 地一層一層目錄進去一篇一篇另存新檔 */
    hdr_fpath(folder, srcDir, hdr);
    if (data = (HDR *) f_img(folder, &fsize))
    {
      head = data;
      tail = data + (fsize / sizeof(HDR));
      do
      {
	/* 只有 gem_copy() 才可能有 GEM_FOLDER，所以用精華區的 chkgemrestrict */
	if (chkgemrestrict(head))
	  gem_do_paste(folder, fpath, head, -1);
      } while (++head < tail);

      free(data);
    }
  }
}


static int
gem_paste(xo)
  XO *xo;
{
  int num, ans, pos;
  char *dir;
  HDR *head, *tail;

  if (!(xo->key & GEM_W_BIT))
    return XO_NONE;

  if (!(num = GemBufferNum))
  {
    zmsg("請先執行 copy 命令後再 paste");
    return XO_FOOT;
  }

  dir = xo->dir;

  /* switch (ans = vans("存放位置 A)ppend I)nsert N)ext E)xtend Q)uit [A] ")) */
  switch (ans = vans("存放位置 A)加到最後 I/N)插入目前位置 E)附加檔案 Q)離開 [A] "))
  {
  case 'q':
    return XO_FOOT;

  case 'e':
    if (gem_extend(xo, num))
      zmsg("[Extend 檔案附加] 動作並未完全成功\");
    return XO_FOOT;

  default:
    pos = (ans == 'n') ? xo->pos + 1 : (ans == 'i') ? xo->pos : -1;

    head = GemBuffer;
    tail = head + num;
    do
    {
      if (invalid_loop(GemFolder, dir, head, 0))	/* itoc.010727: 造成迴圈者不允許貼上 */
      {
	vmsg("造成迴圈的卷宗將無法收錄");
	continue;
      }
      else
      {
	gem_do_paste(GemFolder, dir, head, pos);
	if (pos >= 0)		/* Insert/Next:要繼續往下貼 Append:一直貼在最後 */
	  pos++;
      }
    } while (++head < tail);
  }

  return gem_load(xo);
}


static int
gem_move(xo)
  XO *xo;
{
  HDR *hdr;
  char *dir, buf[40];
  int pos, newOrder;

  if (!(xo->key & GEM_W_BIT) || !(hdr = gem_check(xo, NULL, 0)))
    return XO_NONE;

  pos = xo->pos;
  sprintf(buf, "請輸入第 %d 選項的新位置：", pos + 1);
  if (!vget(b_lines, 0, buf, buf, 5, DOECHO))
    return XO_FOOT;

  newOrder = atoi(buf) - 1;
  if (newOrder < 0)
    newOrder = 0;
  else if (newOrder >= xo->max)
    newOrder = xo->max - 1;

  if (newOrder != pos)
  {
    dir = xo->dir;
    if (!rec_del(dir, sizeof(HDR), pos, NULL))
    {
      rec_ins(dir, hdr, sizeof(HDR), newOrder, 1);
      xo->pos = newOrder;
      return gem_load(xo);
    }
  }
  return XO_FOOT;
}


static int
gem_anchor(xo)
  XO *xo;
{
  int ans;
  char *folder;

  if (!(xo->key & GEM_W_BIT))	/* Thor.981020: 只要板主以上即可使用anchor */
    return XO_NONE; 		/* Thor.981020: 不開放一般 user 使用是為了防止板主試出另一個小 bug :P */
  
  ans = vans("精華區 A)定錨 D)拔錨 J)就位 Q)取消 [A] ");
  if (ans != 'q')
  {
    folder = GemAnchor;

    if (ans == 'j')
    {
      if (!*folder)			/* 沒有定錨 */
	return XO_FOOT;

      XoGem(folder, "● 精華定錨區 ●", xo->key);
      return gem_init(xo);
    }
    else if (ans == 'd')
    {
      *folder = '\0';
    }
    else
    {
      strcpy(folder, xo->dir);
      str_ncpy(GemSailor, xo->xyz, sizeof(GemSailor));
    }

    zmsg("錨動作完成");
  }

  return XO_FOOT;
}


static int
chkgather(hdr)
  HDR *hdr;
{
  if (hdr->xmode & GEM_RESTRICT)	/* 限制級精華區不能定錨收錄 */
    return 0;
    
  if (hdr->xmode & GEM_FOLDER)		/* 查 hdr 是否 plain text (即文章/資料) */
    return 0;

  return 1;
}


int
gem_gather(xo)
  XO *xo;
{
  int tag;
  char *dir, *folder, fpath[80], title[TTLEN + 1];
  FILE *fp;
  HDR *head, *tail, hdr;

  folder = GemAnchor;

  if (!*folder)
  {
    zmsg("請先定錨以後再直接收錄至定錨區");
    return XO_FOOT;
  }

  sprintf(fpath, "收錄至定錨區 (%s)", GemSailor);
  tag = AskTag(fpath);

  if (tag < 0)
    return XO_FOOT;

  /* gather 視同 copy，可準備作 paste */
  dir = xo->dir;
  gem_buffer(dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), chkgather);

  if (!GemBufferNum)
  {
    zmsg("無可收錄文章");
    return XO_FOOT;
  }

  fp = NULL;

  if (tag > 0)
  {
    switch (vans("串列文章 1)合成一篇 2)分別建檔 Q)取消 [1] "))
    {
    case 'q':
      return XO_FOOT;

    case '2':
      break;

    default:
      strcpy(title, currtitle);
      if (!vget(b_lines, 0, "標題：", title, TTLEN + 1, GCARRY))
	return XO_FOOT;
      fp = fdopen(gem_hdr_stamp(folder, 'A', &hdr, fpath), "w");
      strcpy(hdr.owner, cuser.userid);
      strcpy(hdr.title, title);
    }
  }

  head = GemBuffer;
  tail = head + GemBufferNum;
  do
  {
    hdr_fpath(fpath, dir, head);

    if (fp)	/* 合成一篇 */
    {
      f_suck(fp, fpath);
      fputs(str_line, fp);
    }
    else	/* 分別建檔 */
    {
      gem_do_paste(dir, folder, head, -1);
    }
  } while (++head < tail);

  if (fp)
  {
    fclose(fp);
    rec_add(folder, &hdr, sizeof(HDR));
    gem_log(folder, "新增", &hdr);
  }

  zmsg("收錄完成，但是加密文章不會被收錄");

  if (*dir == 'g')	/* 在精華區中 gem_gather() 才要重繪剪貼簿篇數，在看板/信箱裡都不用 */
  {
    move(1, 59);
    clrtoeol();
    prints("(剪貼簿 %d 篇)\n", GemBufferNum);
  }
  return XO_FOOT;
}


static int
gem_tag(xo)
  XO *xo;
{
  HDR *hdr;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if (tag = Tagger(hdr->chrono, pos, TAG_TOGGLE))
  {
    move(3 + cur, 6);
    outc(tag > 0 ? '*' : ' ');
  }

  /* return XO_NONE; */
  return pos + 1 + XO_MOVE;	/* lkchu.981201: 跳至下一項 */
}


static int
gem_help(xo)
  XO *xo;
{
  xo_help("gem");
  return gem_head(xo);
}


static KeyFunc gem_cb[] =
{
  XO_INIT, gem_init,
  XO_LOAD, gem_load,
  XO_HEAD, gem_head,
  XO_BODY, gem_body,

  'r', gem_browse,

  Ctrl('P'), gem_add_all,	/* itoc.010723: gem_cb 的引數只有 xo */
  'a', gem_add_article,
  'f', gem_add_folder,

  'E', gem_edit,
  'T', gem_title,
  'd', gem_delete,
  'D', gem_rangedel,		/* itoc.010726: 提供區段刪除 */

  'c', gem_copy,
  'g', gem_gather,

  Ctrl('G'), gem_anchor,
  Ctrl('V'), gem_paste,
  'p', gem_paste,		/* itoc.010223: 使用者習慣 c/p 收錄精華區 */

  't', gem_tag,
  'x', post_cross,		/* 在 post/mbox 中都是小寫 x 轉看板，大寫 X 轉使用者 */
  'X', post_forward,
  'B', gem_toggle,
  'o', gem_refuse,
  Ctrl('Y'), gem_refuse,
  'm', gem_move,
  'M', gem_move,

  'S', gem_state,

  Ctrl('D'), gem_prune,
#if 0
  Ctrl('Q'), xo_uquery,		/* 精華區的 hdr.owner 一般是指收錄的板主，而不是作者 */
  Ctrl('O'), xo_usetup,		/* 精華區的 hdr.owner 一般是指收錄的板主，而不是作者 */
#endif

  'h', gem_help
};


void
XoGem(folder, title, level)
  char *folder;
  char *title;
  int level;
{
  XO *xo, *last;

  last = xz[XZ_GEM - XO_ZONE].xo;	/* record */

  xz[XZ_GEM - XO_ZONE].xo = xo = xo_new(folder);
  xo->pos = 0;
  xo->key = level;
  xo->xyz = title;

  xover(XZ_GEM);

  free(xo);

  xz[XZ_GEM - XO_ZONE].xo = last;	/* restore */
}


void
gem_main()
{
  XO *xo;

  /* itoc.060706.註解: 進站時就要初始化，因為使用者可能一上站就 every_Z 跳去精華區 */

  xz[XZ_GEM - XO_ZONE].xo = xo = xo_new("gem/"FN_DIR);
  xz[XZ_GEM - XO_ZONE].cb = gem_cb;
  xo->pos = 0;
  /* 看板總管在 (A)nnounce 裡面有 GEM_X_BIT 來新增看板捷徑 */
  xo->key = (HAS_PERM(PERM_ALLBOARD) ? (GEM_W_BIT | GEM_X_BIT | GEM_M_BIT) : 0);
  xo->xyz = "";
}
