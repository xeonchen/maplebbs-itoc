/*-------------------------------------------------------*/
/* post.c       ( NTHU CS MapleBBS Ver 2.39 )		 */
/*-------------------------------------------------------*/
/* target : bulletin boards' routines		 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;
extern XZ xz[];


extern int wordsnum;		/* itoc.010408: 計算文章字數 */
extern int TagNum;
extern char xo_pool[];
extern char brd_bits[];


#ifdef HAVE_ANONYMOUS
extern char anonymousid[];	/* itoc.010717: 自定匿名 ID */
#endif


int
cmpchrono(hdr)
  HDR *hdr;
{
  return hdr->chrono == currchrono;
}


/* ----------------------------------------------------- */
/* 改良 innbbsd 轉出信件、連線砍信之處理程序		 */
/* ----------------------------------------------------- */


void
btime_update(bno)
  int bno;
{
  if (bno >= 0)
    (bshm->bcache + bno)->btime = -1;	/* 讓 class_item() 更新用 */
}


#ifndef HAVE_NETTOOL
static 			/* 給 enews.c 用 */
#endif
void
outgo_post(hdr, board)
  HDR *hdr;
  char *board;
{
  bntp_t bntp;

  memset(&bntp, 0, sizeof(bntp_t));

  if (board)		/* 新信 */
  {
    bntp.chrono = hdr->chrono;
  }
  else			/* cancel */
  {
    bntp.chrono = -1;
    board = currboard;
  }
  strcpy(bntp.board, board);
  strcpy(bntp.xname, hdr->xname);
  strcpy(bntp.owner, hdr->owner);
  strcpy(bntp.nick, hdr->nick);
  strcpy(bntp.title, hdr->title);
  rec_add("innd/out.bntp", &bntp, sizeof(bntp_t));
}


void
cancel_post(hdr)
  HDR *hdr;
{
  if ((hdr->xmode & POST_OUTGO) &&		/* 外轉信件 */
    (hdr->chrono > ap_start - 7 * 86400))	/* 7 天之內有效 */
  {
    outgo_post(hdr, NULL);
  }
}


static inline int		/* 回傳文章 size 去扣錢 */
move_post(hdr, folder, by_bm)	/* 將 hdr 從 folder 搬到別的板 */
  HDR *hdr;
  char *folder;
  int by_bm;
{
  HDR post;
  int xmode;
  char fpath[64], fnew[64], *board;
  struct stat st;

  xmode = hdr->xmode;
  hdr_fpath(fpath, folder, hdr);

  if (!(xmode & POST_BOTTOM))	/* 置底文被砍不用 move_post */
  {
#ifdef HAVE_REFUSEMARK
    board = by_bm && !(xmode & POST_RESTRICT) ? BN_DELETED : BN_JUNK;	/* 加密文章丟去 junk */
#else
    board = by_bm ? BN_DELETED : BN_JUNK;
#endif

    brd_fpath(fnew, board, fn_dir);
    hdr_stamp(fnew, HDR_LINK | 'A', &post, fpath);

    /* 直接複製 trailing data：owner(含)以下所有欄位 */

    memcpy(post.owner, hdr->owner, sizeof(HDR) -
      (sizeof(post.chrono) + sizeof(post.xmode) + sizeof(post.xid) + sizeof(post.xname)));

    if (by_bm)
      sprintf(post.title, "%-13s%.59s", cuser.userid, hdr->title);

    rec_bot(fnew, &post, sizeof(HDR));
    btime_update(brd_bno(board));
  }

  by_bm = stat(fpath, &st) ? 0 : st.st_size;

  unlink(fpath);
  btime_update(currbno);
  cancel_post(hdr);

  return by_bm;
}


#ifdef HAVE_DETECT_CROSSPOST
/* ----------------------------------------------------- */
/* 改良 cross post 停權					 */
/* ----------------------------------------------------- */


#define MAX_CHECKSUM_POST	20	/* 記錄最近 20 篇文章的 checksum */
#define MAX_CHECKSUM_LINE	6	/* 只取文章前 6 行來算 checksum */


typedef struct
{
  int sum;			/* 文章的 checksum */
  int total;			/* 此文章已發表幾篇 */
}      CHECKSUM;


static CHECKSUM checksum[MAX_CHECKSUM_POST];
static int checknum = 0;


static inline int
checksum_add(str)		/* 回傳本列文字的 checksum */
  char *str;
{
  int i, len, sum;

  len = strlen(str);

  sum = len;	/* 當字數太少時，前四分之一很可能完全相同，所以將字數也加入 sum 值 */
  for (i = len >> 2; i > 0; i--)	/* 只算前四分之一字元的 sum 值 */
    sum += *str++;

  return sum;
}


static inline int		/* 1:是cross-post 0:不是cross-post */
checksum_put(sum)
  int sum;
{
  int i;

  if (sum)
  {
    for (i = 0; i < MAX_CHECKSUM_POST; i++)
    {
      if (checksum[i].sum == sum)
      {
	checksum[i].total++;

	if (checksum[i].total > MAX_CROSS_POST)
	  return 1;
	return 0;	/* total <= MAX_CROSS_POST */
      }
    }

    if (++checknum >= MAX_CHECKSUM_POST)
      checknum = 0;
    checksum[checknum].sum = sum;
    checksum[checknum].total = 1;
  }
  return 0;
}


static int			/* 1:是cross-post 0:不是cross-post */
checksum_find(fpath)
  char *fpath;
{
  int i, sum;
  char buf[ANSILINELEN];
  FILE *fp;

  sum = 0;
  if (fp = fopen(fpath, "r"))
  {
    for (i = -(LINE_HEADER + 1);;)	/* 前幾列是檔頭 */
    {
      if (!fgets(buf, ANSILINELEN, fp))
	break;

      if (i < 0)	/* 跳過檔頭 */
      {
	i++;
	continue;
      }

      if (*buf == QUOTE_CHAR1 || *buf == '\n' || !strncmp(buf, "※", 2))	 /* 跳過引言 */
	continue;

      sum += checksum_add(buf);
      if (++i >= MAX_CHECKSUM_LINE)
	break;
    }

    fclose(fp);
  }

  return checksum_put(sum);
}


static int
check_crosspost(fpath, bno)
  char *fpath;
  int bno;			/* 要轉去的看板 */
{
  char *blist, folder[64];
  ACCT acct;
  HDR hdr;

  if (HAS_PERM(PERM_ALLADMIN))
    return 0;

  /* 板主在自己管理的看板不列入跨貼檢查 */
  blist = (bshm->bcache + bno)->BM;
  if (HAS_PERM(PERM_BM) && blist[0] > ' ' && is_bm(blist, cuser.userid))
    return 0;

  if (checksum_find(fpath))
  {
    /* 如果是 cross-post，那麼轉去 BN_SECURITY 並直接停權 */
    brd_fpath(folder, BN_SECURITY, fn_dir);
    hdr_stamp(folder, HDR_COPY | 'A', &hdr, fpath);
    strcpy(hdr.owner, cuser.userid);
    strcpy(hdr.nick, cuser.username);
    sprintf(hdr.title, "%s %s Cross-Post", cuser.userid, Now());
    rec_bot(folder, &hdr, sizeof(HDR));
    btime_update(brd_bno(BN_SECURITY));

    bbstate &= ~STAT_POST;
    cuser.userlevel &= ~PERM_POST;
    cuser.userlevel |= PERM_DENYPOST;
    if (acct_load(&acct, cuser.userid) >= 0)
    {
      acct.tvalid = time(NULL) + CROSSPOST_DENY_DAY * 86400;
      acct_setperm(&acct, PERM_DENYPOST, PERM_POST);
    }
    board_main();
    mail_self(FN_ETC_CROSSPOST, str_sysop, "Cross-Post 停權", 0);
    vmsg("您因為過度 Cross-Post 已被停權");
    return 1;
  }
  return 0;
}
#endif	/* HAVE_DETECT_CROSSPOST */


/* ----------------------------------------------------- */
/* 發表、回應、編輯、轉錄文章				 */
/* ----------------------------------------------------- */


int
is_author(hdr)
  HDR *hdr;
{
  /* 這裡沒有檢查是不是 guest，注意使用此函式時要特別考慮 guest 情況 */

  /* itoc.070426: 當帳號被清除後，新註冊相同 ID 的帳號並不擁有過去該 ID 發表的文章之所有權 */
  return !strcmp(hdr->owner, cuser.userid) && (hdr->chrono > cuser.firstlogin);
}


#ifdef HAVE_REFUSEMARK
int
chkrestrict(hdr)
  HDR *hdr;
{
  return !(hdr->xmode & POST_RESTRICT) || is_author(hdr) || (bbstate & STAT_BM);
}
#endif  


#ifdef HAVE_ANONYMOUS
static void
log_anonymous(fname)
  char *fname;
{
  char buf[512];

  sprintf(buf, "%s %-13s(%s)\n%-13s %s %s\n", 
    Now(), cuser.userid, fromhost, currboard, fname, ve_title);
  f_cat(FN_RUN_ANONYMOUS, buf);
}
#endif


#ifdef HAVE_UNANONYMOUS_BOARD
static void
do_unanonymous(fpath)
  char *fpath;
{
  HDR hdr;
  char folder[64];

  brd_fpath(folder, BN_UNANONYMOUS, fn_dir);
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  strcpy(hdr.owner, cuser.userid);
  strcpy(hdr.title, ve_title);

  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(brd_bno(BN_UNANONYMOUS));
}
#endif


static int
do_post(xo, title)
  XO *xo;
  char *title;
{
  /* Thor.981105: 進入前需設好 curredit 及 quote_file */
  HDR hdr, buf;
  char fpath[64], *folder, *nick, *rcpt;
  int mode;
  time_t spendtime, prev, chrono;

  if (!(bbstate & STAT_POST))
  {
#ifdef NEWUSER_LIMIT
    if (cuser.lastlogin - cuser.firstlogin < 3 * 86400)
      vmsg("新手上路，三日後始可張貼文章");
    else
#endif      
      vmsg("對不起，您沒有在此發表文章的權限");
    return XO_FOOT;
  }

  film_out(FILM_POST, 0);

  prints("發表文章於【 %s 】看板", currboard);

#ifdef POST_PREFIX
  /* 借用 mode、rcpt、fpath */

  if (title)
  {
    rcpt = NULL;
  }
  else		/* itoc.020113: 新文章選擇標題分類 */
  {
#define NUM_PREFIX 6
    char *prefix[NUM_PREFIX] = {"[公告] ", "[新聞] ", "[閒聊] ", "[文件] ", "[問題] ", "[測試] "};

    move(21, 0);
    outs("類別：");
    for (mode = 0; mode < NUM_PREFIX; mode++)
      prints("%d.%s", mode + 1, prefix[mode]);

    mode = vget(20, 0, "請選擇文章類別（按 Enter 跳過）：", fpath, 3, DOECHO) - '1';
    if (mode >= 0 && mode < NUM_PREFIX)		/* 輸入數字選項 */
      rcpt = prefix[mode];
    else					/* 空白跳過 */
      rcpt = NULL;
  }

  if (!ve_subject(21, title, rcpt))
#else
  if (!ve_subject(21, title, NULL))
#endif
      return XO_HEAD;

  /* 未具備 Internet 權限者，只能在站內發表文章 */
  /* Thor.990111: 沒轉信出去的看板, 也只能在站內發表文章 */

  if (!HAS_PERM(PERM_INTERNET) || (currbattr & BRD_NOTRAN))
    curredit &= ~EDIT_OUTGO;

  utmp_mode(M_POST);
  fpath[0] = '\0';
  time(&spendtime);
  if (vedit(fpath, 1) < 0)
  {
    unlink(fpath);
    vmsg(msg_cancel);
    return XO_HEAD;
  }
  spendtime = time(0) - spendtime;	/* itoc.010712: 總共花的時間(秒數) */

  /* build filename */

  folder = xo->dir;
  hdr_stamp(folder, HDR_LINK | 'A', &hdr, fpath);

  /* set owner to anonymous for anonymous board */

#ifdef HAVE_ANONYMOUS
  /* Thor.980727: lkchu新增之[簡單的選擇性匿名功能] */
  if (curredit & EDIT_ANONYMOUS)
  {
    rcpt = anonymousid;	/* itoc.010717: 自定匿名 ID */
    nick = STR_ANONYMOUS;

    /* Thor.980727: lkchu patch: log anonymous post */
    /* Thor.980909: gc patch: log anonymous post filename */
    log_anonymous(hdr.xname);

#ifdef HAVE_UNANONYMOUS_BOARD
    do_unanonymous(fpath);
#endif

    /* gaod.091205: 發匿名文後不取消 EDIT_ANONYMOUS 會導致轉錄文章 header 資訊有誤，Thanks for om@cpu.tfcis.org. */
    curredit &= ~EDIT_ANONYMOUS;
  }
  else
#endif
  {
    rcpt = cuser.userid;
    nick = cuser.username;
  }
  title = ve_title;
  mode = (curredit & EDIT_OUTGO) ? POST_OUTGO : 0;
#ifdef HAVE_REFUSEMARK
  if (curredit & EDIT_RESTRICT)
    mode |= POST_RESTRICT;
#endif

  hdr.xmode = mode;
  strcpy(hdr.owner, rcpt);
  strcpy(hdr.nick, nick);
  strcpy(hdr.title, title);

  rec_bot(folder, &hdr, sizeof(HDR));
  btime_update(currbno);

  if (mode & POST_OUTGO)
    outgo_post(&hdr, currboard);

#if 1	/* itoc.010205: post 完文章就記錄，使不出現未閱讀的＋號 */
  chrono = hdr.chrono;
  prev = ((mode = rec_num(folder, sizeof(HDR)) - 2) >= 0 && !rec_get(folder, &buf, sizeof(HDR), mode)) ? buf.chrono : chrono;
  brh_add(prev, chrono, chrono);
#endif

  clear();
  outs("順利貼出文章，");

  if (currbattr & BRD_NOCOUNT || wordsnum < 30)
  {				/* itoc.010408: 以此減少灌水現象 */
    outs("文章不列入紀錄，敬請包涵。");
  }
  else
  {
    /* itoc.010408: 依文章長度/所費時間來決定要給多少錢；幣制才會有意義 */
    mode = BMIN(wordsnum, spendtime) / 10;	/* 每十字/秒 一元 */
    prints("這是您的第 %d 篇文章，得 %d 銀。", ++cuser.numposts, mode);
    addmoney(mode);
  }

  /* 回應到原作者信箱 */

  if (curredit & EDIT_BOTH)
  {
    rcpt = quote_user;

    if (strchr(rcpt, '@'))	/* 站外 */
      mode = bsmtp(fpath, title, rcpt, 0);
    else			/* 站內使用者 */
      mode = mail_him(fpath, rcpt, title, 0);

    outs(mode >= 0 ? "\n\n成功\回應至作者信箱" : "\n\n作者無法收信");
  }

  unlink(fpath);

  vmsg(NULL);

  return XO_INIT;
}


int
do_reply(xo, hdr)
  XO *xo;
  HDR *hdr;
{
  curredit = 0;

  switch (vans("▲ 回應至 (F)看板 (M)作者信箱 (B)二者皆是 (Q)取消？[F] "))
  {
  case 'm':
    hdr_fpath(quote_file, xo->dir, hdr);
    return do_mreply(hdr, 0);

  case 'q':
    return XO_FOOT;

  case 'b':
    /* 若無寄信的權限，則只回看板 */
    if (HAS_PERM(strchr(hdr->owner, '@') ? PERM_INTERNET : PERM_LOCAL))
      curredit = EDIT_BOTH;
    break;
  }

  /* Thor.981105: 不論是轉進的, 或是要轉出的, 都是別站可看到的, 所以回信也都應該轉出 */
  if (hdr->xmode & (POST_INCOME | POST_OUTGO))
    curredit |= EDIT_OUTGO;

  hdr_fpath(quote_file, xo->dir, hdr);
  strcpy(quote_user, hdr->owner);
  strcpy(quote_nick, hdr->nick);
  return do_post(xo, hdr->title);
}


static int
post_reply(xo)
  XO *xo;
{
  if (bbstate & STAT_POST)
  {
    HDR *hdr;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif

    return do_reply(xo, hdr);
  }
  return XO_NONE;
}


static int
post_add(xo)
  XO *xo;
{
  curredit = EDIT_OUTGO;
  *quote_file = '\0';
  return do_post(xo, NULL);
}


/* ----------------------------------------------------- */
/* 印出 hdr 標題					 */
/* ----------------------------------------------------- */


int
tag_char(chrono)
  int chrono;
{
  return TagNum && !Tagger(chrono, 0, TAG_NIN) ? '*' : ' ';
}


#ifdef HAVE_DECLARE
static inline int
cal_day(date)		/* itoc.010217: 計算星期幾 */
  char *date;
{
#if 0
   蔡勒公式是一個推算哪一天是星期幾的公式.
   這公式是:
         c                y       26(m+1)
    W= [---] - 2c + y + [---] + [---------] + d - 1
         4                4         10
    W → 為所求日期的星期數. (星期日: 0  星期一: 1  ...  星期六: 6)
    c → 為已知公元年份的前兩位數字.
    y → 為已知公元年份的後兩位數字.
    m → 為月數
    d → 為日數
   [] → 表示只取該數的整數部分 (地板函數)
    ps.所求的月份如果是1月或2月,則應視為上一年的13月或14月.
       所以公式中m的取值範圍不是1到12,而是3到14
#endif

  /* 適用 2000/03/01 至 2099/12/31 */

  int y, m, d;

  y = 10 * ((int) (date[0] - '0')) + ((int) (date[1] - '0'));
  d = 10 * ((int) (date[6] - '0')) + ((int) (date[7] - '0'));
  if (date[3] == '0' && (date[4] == '1' || date[4] == '2'))
  {
    y -= 1;
    m = 12 + (int) (date[4] - '0');
  }
  else
  {
    m = 10 * ((int) (date[3] - '0')) + ((int) (date[4] - '0'));
  }
  return (-1 + y + y / 4 + 26 * (m + 1) / 10 + d) % 7;
}
#endif


void
hdr_outs(hdr, cc)		/* print HDR's subject */
  HDR *hdr;
  int cc;			/* 印出最多 cc - 1 字的標題 */
{
  /* 回覆/轉錄/原創/閱讀中的同主題回覆/閱讀中的同主題轉錄/閱讀中的同主題原創 */
  static char *type[6] = {"Re", "Fw", "◇", "\033[1;33m=>", "\033[1;33m=>", "\033[1;32m◆"};
  uschar *title, *mark;
  int ch, len;
  int in_chi;		/* 1: 在中文字中 */
#ifdef HAVE_DECLARE
  int square;		/* 1: 要處理方括 */
#endif
#ifdef CHECK_ONLINE
  UTMP *online;
#endif

  /* --------------------------------------------------- */
  /* 印出日期						 */
  /* --------------------------------------------------- */

#ifdef HAVE_DECLARE
  /* itoc.010217: 改用星期幾來上色 */
  prints("\033[1;3%dm%s\033[m ", cal_day(hdr->date) + 1, hdr->date + 3);
#else
  outs(hdr->date + 3);
  outc(' ');
#endif

  /* --------------------------------------------------- */
  /* 印出作者						 */
  /* --------------------------------------------------- */

#ifdef CHECK_ONLINE
  if (online = utmp_seek(hdr))
    outs(COLOR7);
#endif

  mark = hdr->owner;
  len = IDLEN + 1;
  in_chi = 0;

  while (ch = *mark)
  {
    if (--len <= 0)
    {
      /* 把超過 len 長度的部分直接切掉 */
      /* itoc.060604.註解: 如果剛好切在中文字的一半就會出現亂碼，不過這情況很少發生，所以就不管了 */
      ch = '.';
    }
    else
    {
      /* 站外的作者把 '@' 換成 '.' */
      if (in_chi || IS_ZHC_HI(ch))	/* 中文字尾碼是 '@' 的不算 */
	in_chi ^= 1;
      else if (ch == '@')
	ch = '.';
    }
      
    outc(ch);

    if (ch == '.')
      break;

    mark++;
  }

  while (len--)
    outc(' ');

#ifdef CHECK_ONLINE
  if (online)
    outs(str_ransi);
#endif

  /* --------------------------------------------------- */
  /* 印出標題的種類					 */
  /* --------------------------------------------------- */

  /* len: 標題是 type[] 裡面的那一種 */
  title = str_ttl(mark = hdr->title);
  len = (title == mark) ? 2 : (*mark == 'R') ? 0 : 1;
  if (!strcmp(currtitle, title))
    len += 3;
  outs(type[len]);
  outc(' ');

  /* --------------------------------------------------- */
  /* 印出標題						 */
  /* --------------------------------------------------- */

  mark = title + cc;

#ifdef HAVE_DECLARE	/* Thor.980508: Declaration, 嘗試使某些title更明顯 */
  square = in_chi = 0;
  if (len < 3)
  {
    if (*title == '[')
    {
      outs("\033[1m");
      square = 1;
    }
  }
#endif

  /* 把超過 cc 長度的部分直接切掉 */
  /* itoc.060604.註解: 如果剛好切在中文字的一半就會出現亂碼，不過這情況很少發生，所以就不管了 */
  while ((ch = *title++) && (title < mark))
  {
#ifdef HAVE_DECLARE
    if (square)
    {
      if (in_chi || IS_ZHC_HI(ch))	/* 中文字的第二碼若是 ']' 不算是方括 */
      {
	in_chi ^= 1;
      }
      else if (ch == ']')
      {
	outs("]\033[m");
	square = 0;			/* 只處理一組方括，方括已經處理完了 */
	continue;
      }
    }
#endif

    outc(ch);
  }

#ifdef HAVE_DECLARE
  if (square || len >= 3)	/* Thor.980508: 變色還原用 */
#else
  if (len >= 3)
#endif
    outs("\033[m");

  outc('\n');
}


/* ----------------------------------------------------- */
/* 看板功能表						 */
/* ----------------------------------------------------- */


static int post_body();
static int post_head();


static int
post_init(xo)
  XO *xo;
{
  xo_load(xo, sizeof(HDR));
  return post_head(xo);
}


static int
post_load(xo)
  XO *xo;
{
  xo_load(xo, sizeof(HDR));
  return post_body(xo);
}


static int
post_attr(hdr)
  HDR *hdr;
{
  int mode, attr;

  mode = hdr->xmode;

  /* 已閱讀為小寫，未閱讀為大寫 */
  /* 由於置底文沒有閱讀記錄，所以視為已讀 */
  /* 加密文章視為已讀 */
#ifdef HAVE_REFUSEMARK
  attr = ((mode & POST_BOTTOM) || !brh_unread(hdr->chrono) || !chkrestrict(hdr)) ? 0x20 : 0;
#else
  attr = ((mode & POST_BOTTOM) || !brh_unread(hdr->chrono)) ? 0x20 : 0;
#endif

#ifdef HAVE_REFUSEMARK
  if (mode & POST_RESTRICT)
    attr |= 'X';  
  else
#endif
#ifdef HAVE_LABELMARK
  if (mode & POST_DELETE)
    attr |= 'T';
  else
#endif
  if (mode & POST_MARKED)
    attr |= 'M';
  else if (!attr)
    attr = '+';

  return attr;
}


static void
post_item(num, hdr)
  int num;
  HDR *hdr;
{
#ifdef HAVE_SCORE
  static char scorelist[36] =
  {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
  };

  prints("%6d%c%c", (hdr->xmode & POST_BOTTOM) ? -1 : num, tag_char(hdr->chrono), post_attr(hdr));
  if (hdr->xmode & POST_SCORE)
  {
    num = hdr->score;
    prints("\033[1;3%cm%c\033[m ", num >= 0 ? '1' : '2', scorelist[abs(num)]);
  }
  else
  {
    outs("  ");
  }
  hdr_outs(hdr, d_cols + 46);	/* 少一格來放分數 */
#else
  prints("%6d%c%c ", (hdr->xmode & POST_BOTTOM) ? -1 : num, tag_char(hdr->chrono), post_attr(hdr));
  hdr_outs(hdr, d_cols + 47);
#endif
}


static int
post_body(xo)
  XO *xo;
{
  HDR *hdr;
  int num, max, tail;

  max = xo->max;
  if (max <= 0)
  {
    if (bbstate & STAT_POST)
    {
      if (vans("要新增資料嗎(Y/N)？[N] ") == 'y')
	return post_add(xo);
    }
    else
    {
      vmsg("本看板尚無文章");
    }
    return XO_QUIT;
  }

  hdr = (HDR *) xo_pool;
  num = xo->top;
  tail = num + XO_TALL;
  if (max > tail)
    max = tail;

  move(3, 0);
  do
  {
    post_item(++num, hdr++);
  } while (num < max);
  clrtobot();

  /* return XO_NONE; */
  return XO_FOOT;	/* itoc.010403: 把 b_lines 填上 feeter */
}


static int
post_head(xo)
  XO *xo;
{
  vs_head(currBM, xo->xyz);
  prints(NECKER_POST, d_cols, "", currbattr & BRD_NOSCORE ? "╳" : "○", bshm->mantime[currbno]);
  return post_body(xo);
}


/* ----------------------------------------------------- */
/* 資料之瀏覽：browse / history				 */
/* ----------------------------------------------------- */


static int
post_visit(xo)
  XO *xo;
{
  int ans, row, max;
  HDR *hdr;

  ans = vans("設定所有文章 (U)未讀 (V)已讀 (W)前已讀後未讀 (Q)取消？[Q] ");
  if (ans == 'v' || ans == 'u' || ans == 'w')
  {
    row = xo->top;
    max = xo->max - row + 3;
    if (max > b_lines)
      max = b_lines;

    hdr = (HDR *) xo_pool + (xo->pos - row);
    /* brh_visit(ans == 'w' ? hdr->chrono : ans == 'u'); */
    /* weiyu.041010: 在置底文上選 w 視為全部已讀 */
    brh_visit((ans == 'u') ? 1 : (ans == 'w' && !(hdr->xmode & POST_BOTTOM)) ? hdr->chrono : 0);

    hdr = (HDR *) xo_pool;
    row = 3;
    do
    {
      move(row, 7);
      outc(post_attr(hdr++));
    } while (++row < max);
  }
  return XO_FOOT;
}


static void
post_history(xo, hdr)		/* 將 hdr 這篇加入 brh */
  XO *xo;
  HDR *hdr;
{
  time_t prev, chrono, next;
  int pos, top;
  char *dir;
  HDR buf;

  if (hdr->xmode & POST_BOTTOM)	/* 置底文不加入閱讀記錄 */
    return;

  chrono = hdr->chrono;
  if (!brh_unread(chrono))	/* 如果已在 brh 中，就無需動作 */
    return;

  dir = xo->dir;
  pos = xo->pos;
  top = xo->top;

  pos--;
  if (pos >= top)
  {
    prev = hdr[-1].chrono;
  }
  else
  {
    /* amaki.040302.註解: 在畫面以上，只好讀硬碟 */
    if (!rec_get(dir, &buf, sizeof(HDR), pos))
      prev = buf.chrono;
    else
      prev = chrono;
  }

  pos += 2;
  if (pos < top + XO_TALL && pos < xo->max)
  {
    next = hdr[1].chrono;
  }
  else
  {
    /* amaki.040302.註解: 在畫面以下，只好讀硬碟 */
    if (!rec_get(dir, &buf, sizeof(HDR), pos))
      next = buf.chrono;
    else
      next = chrono;
  }

  brh_add(prev, chrono, next);
}


static int
post_browse(xo)
  XO *xo;
{
  HDR *hdr;
  int xmode, pos, key;
  char *dir, fpath[64];

  dir = xo->dir;

  for (;;)
  {
    pos = xo->pos;
    hdr = (HDR *) xo_pool + (pos - xo->top);
    xmode = hdr->xmode;

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      break;
#endif

    hdr_fpath(fpath, dir, hdr);

    /* Thor.990204: 為考慮more 傳回值 */   
    if ((key = more(fpath, FOOTER_POST)) < 0)
      break;

    post_history(xo, hdr);
    strcpy(currtitle, str_ttl(hdr->title));

re_key:
    switch (xo_getch(xo, key))
    {
    case XO_BODY:
      continue;

    case 'y':
    case 'r':
      if (bbstate & STAT_POST)
      {
	if (do_reply(xo, hdr) == XO_INIT)	/* 有成功地 post 出去了 */
	  return post_init(xo);
      }
      break;

    case 'm':
      if ((bbstate & STAT_BOARD) && !(xmode & (POST_MARKED | POST_DELETE)))
      {
	/* hdr->xmode = xmode ^ POST_MARKED; */
	/* 在 post_browse 時看不到 m 記號，所以限制只能 mark */
	hdr->xmode = xmode | POST_MARKED;
	currchrono = hdr->chrono;
	rec_put(dir, hdr, sizeof(HDR), pos, cmpchrono);
      }
      break;

#ifdef HAVE_SCORE
    case '%':
      post_score(xo);
      return post_init(xo);
#endif

    case '/':
      if (vget(b_lines, 0, "搜尋：", hunt, sizeof(hunt), DOECHO))
      {
	more(fpath, FOOTER_POST);
	goto re_key;
      }
      continue;

    case 'E':
      return post_edit(xo);

    case 'C':	/* itoc.000515: post_browse 時可存入暫存檔 */
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
      xo_help("post");
      break;
    }
    break;
  }

  return post_head(xo);
}


/* ----------------------------------------------------- */
/* 精華區						 */
/* ----------------------------------------------------- */


static int
post_gem(xo)
  XO *xo;
{
  int level;
  char fpath[64];

  strcpy(fpath, "gem/");
  strcpy(fpath + 4, xo->dir);

  level = 0;
  if (bbstate & STAT_BOARD)
    level ^= GEM_W_BIT;
  if (HAS_PERM(PERM_SYSOP))
    level ^= GEM_X_BIT;
  if (bbstate & STAT_BM)
    level ^= GEM_M_BIT;

  XoGem(fpath, "精華區", level);
  return post_init(xo);
}


/* ----------------------------------------------------- */
/* 進板畫面						 */
/* ----------------------------------------------------- */


static int
post_memo(xo)
  XO *xo;
{
  char fpath[64];

  brd_fpath(fpath, currboard, fn_note);
  /* Thor.990204: 為考慮more 傳回值 */   
  if (more(fpath, NULL) < 0)
  {
    vmsg("本看板尚無「進板畫面」");
    return XO_FOOT;
  }

  return post_head(xo);
}


/* ----------------------------------------------------- */
/* 功能：tag / switch / cross / forward			 */
/* ----------------------------------------------------- */


static int
post_tag(xo)
  XO *xo;
{
  HDR *hdr;
  int tag, pos, cur;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if (xo->key == XZ_XPOST)
    pos = hdr->xid;

  if (tag = Tagger(hdr->chrono, pos, TAG_TOGGLE))
  {
    move(3 + cur, 6);
    outc(tag > 0 ? '*' : ' ');
  }

  /* return XO_NONE; */
  return xo->pos + 1 + XO_MOVE; /* lkchu.981201: 跳至下一項 */
}


static int
post_switch(xo)
  XO *xo;
{
  int bno;
  BRD *brd;
  char bname[BNLEN + 1];

  if (brd = ask_board(bname, BRD_R_BIT, NULL))
  {
    if ((bno = brd - bshm->bcache) >= 0 && currbno != bno)
    {
      XoPost(bno);
      return XZ_POST;
    }
  }
  else
  {
    vmsg(err_bid);
  }
  return post_head(xo);
}


int
post_cross(xo)
  XO *xo;
{
  /* 來源看板 */
  char *dir, *ptr;
  HDR *hdr, xhdr;

  /* 欲轉去的看板 */
  int xbno;
  usint xbattr;
  char xboard[BNLEN + 1], xfolder[64];
  HDR xpost;

  int tag, rc, locus, finish;
  int method;		/* 0:原文轉載 1:從公開看板/精華區/信箱轉錄文章 2:從秘密看板轉錄文章 */
  usint tmpbattr;
  char tmpboard[BNLEN + 1];
  char fpath[64], buf[ANSILINELEN];
  FILE *fpr, *fpw;

  if (!cuser.userlevel)	/* itoc.000213: 避免 guest 轉錄去 sysop 板 */
    return XO_NONE;

  tag = AskTag("轉錄");
  if (tag < 0)
    return XO_FOOT;

  dir = xo->dir;

  if (!ask_board(xboard, BRD_W_BIT, "\n\n\033[1;33m請挑選適當的看板，切勿轉錄超過三板。\033[m\n\n") ||
    (*dir == 'b' && !strcmp(xboard, currboard)))	/* 信箱、精華區中可以轉錄至currboard */
    return XO_HEAD;

  hdr = tag ? &xhdr : (HDR *) xo_pool + (xo->pos - xo->top);	/* lkchu.981201: 整批轉錄 */

  /* 原作者轉錄自己文章時，可以選擇「原文轉載」 */
  method = (HAS_PERM(PERM_ALLBOARD) || (!tag && is_author(hdr))) &&
    (vget(2, 0, "(1)原文轉載 (2)轉錄文章？[1] ", buf, 3, DOECHO) != '2') ? 0 : 1;

  if (!tag)	/* lkchu.981201: 整批轉錄就不要一一詢問 */
  {
    if (method)
      sprintf(ve_title, "Fw: %.68s", str_ttl(hdr->title));	/* 已有 Re:/Fw: 字樣就只要一個 Fw: */
    else
      strcpy(ve_title, hdr->title);

    if (!vget(2, 0, "標題：", ve_title, TTLEN + 1, GCARRY))
      return XO_HEAD;
  }

#ifdef HAVE_REFUSEMARK    
  rc = vget(2, 0, "(S)存檔 (L)站內 (X)密封 (Q)取消？[Q] ", buf, 3, LCECHO);
  if (rc != 'l' && rc != 's' && rc != 'x')
#else
  rc = vget(2, 0, "(S)存檔 (L)站內 (Q)取消？[Q] ", buf, 3, LCECHO);
  if (rc != 'l' && rc != 's')
#endif
    return XO_HEAD;

  if (method && *dir == 'b')	/* 從看板轉出，先檢查此看板是否為秘密板 */
  {
    /* 借用 tmpbattr */
    tmpbattr = (bshm->bcache + currbno)->readlevel;
    if (tmpbattr == PERM_SYSOP || tmpbattr == PERM_BOARD)
      method = 2;
  }

  xbno = brd_bno(xboard);
  xbattr = (bshm->bcache + xbno)->battr;

  /* Thor.990111: 在可以轉出前，要檢查有沒有轉出的權力? */
  if ((rc == 's') && (!HAS_PERM(PERM_INTERNET) || (xbattr & BRD_NOTRAN)))
    rc = 'l';

  /* 備份 currboard */
  if (method)
  {
    /* itoc.030325: 一般轉錄呼叫 ve_header，會使用到 currboard、currbattr，先備份起來 */
    strcpy(tmpboard, currboard);
    strcpy(currboard, xboard);
    tmpbattr = currbattr;
    currbattr = xbattr;
  }

  locus = 0;
  do	/* lkchu.981201: 整批轉錄 */
  {
    if (tag)
    {
      EnumTag(hdr, dir, locus, sizeof(HDR));

      if (method)
	sprintf(ve_title, "Fw: %.68s", str_ttl(hdr->title));	/* 已有 Re:/Fw: 字樣就只要一個 Fw: */
      else
	strcpy(ve_title, hdr->title);
    }

    if (hdr->xmode & GEM_FOLDER)	/* 非 plain text 不能轉 */
      continue;

#ifdef HAVE_REFUSEMARK
    if (hdr->xmode & POST_RESTRICT)
      continue;
#endif

    hdr_fpath(fpath, dir, hdr);

#ifdef HAVE_DETECT_CROSSPOST
    if (check_crosspost(fpath, xbno))
      break;
#endif

    brd_fpath(xfolder, xboard, fn_dir);

    if (method)		/* 一般轉錄 */
    {
      /* itoc.030325: 一般轉錄要重新加上 header */
      fpw = fdopen(hdr_stamp(xfolder, 'A', &xpost, buf), "w");
      ve_header(fpw);

      /* itoc.040228: 如果是從精華區轉錄出來的話，會顯示轉錄自 [currboard] 看板，
	 然而 currboard 未必是該精華區的看板。不過不是很重要的問題，所以就不管了 :p */
      fprintf(fpw, "※ 本文轉錄自 [%s] %s\n\n", 
	*dir == 'u' ? cuser.userid : method == 2 ? "秘密" : tmpboard, 
	*dir == 'u' ? "信箱" : "看板");

      /* Kyo.051117: 若是從秘密看板轉出的文章，刪除文章第一行所記錄的看板名稱 */
      finish = 0;
      if ((method == 2) && (fpr = fopen(fpath, "r")))
      {
	if (fgets(buf, sizeof(buf), fpr) && 
	  ((ptr = strstr(buf, str_post1)) || (ptr = strstr(buf, str_post2))) && (ptr > buf))
	{
	  ptr[-1] = '\n';
	  *ptr = '\0';

	  do
	  {
	    fputs(buf, fpw);
	  } while (fgets(buf, sizeof(buf), fpr));
	  finish = 1;
	}
	fclose(fpr);
      }
      if (!finish)
	f_suck(fpw, fpath);

      fclose(fpw);

      strcpy(xpost.owner, cuser.userid);
      strcpy(xpost.nick, cuser.username);
    }
    else		/* 原文轉錄 */
    {
      /* itoc.030325: 原文轉錄直接 copy 即可 */
      hdr_stamp(xfolder, HDR_COPY | 'A', &xpost, fpath);

      strcpy(xpost.owner, hdr->owner);
      strcpy(xpost.nick, hdr->nick);
      strcpy(xpost.date, hdr->date);	/* 原文轉載保留原日期 */
    }

    strcpy(xpost.title, ve_title);

    if (rc == 's')
      xpost.xmode = POST_OUTGO;
#ifdef HAVE_REFUSEMARK
    else if (rc == 'x')
      xpost.xmode = POST_RESTRICT;
#endif

    rec_bot(xfolder, &xpost, sizeof(HDR));

    if (rc == 's')
      outgo_post(&xpost, xboard);
  } while (++locus < tag);

  btime_update(xbno);

  /* Thor.981205: check 被轉的板有沒有列入紀錄? */
  if (!(xbattr & BRD_NOCOUNT))
    cuser.numposts += tag ? tag : 1;	/* lkchu.981201: 要算 tag */

  /* 復原 currboard、currbattr */
  if (method)
  {
    strcpy(currboard, tmpboard);
    currbattr = tmpbattr;
  }

  vmsg("轉錄完成");
  return XO_HEAD;
}


int
post_forward(xo)
  XO *xo;
{
  ACCT muser;
  HDR *hdr;

  if (!HAS_PERM(PERM_LOCAL))
    return XO_NONE;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);

  if (hdr->xmode & GEM_FOLDER)	/* 非 plain text 不能轉 */
    return XO_NONE;

#ifdef HAVE_REFUSEMARK
  if (!chkrestrict(hdr))
    return XO_NONE;
#endif

  if (acct_get("轉達信件給：", &muser) > 0)
  {
    strcpy(quote_user, hdr->owner);
    strcpy(quote_nick, hdr->nick);
    hdr_fpath(quote_file, xo->dir, hdr);
    sprintf(ve_title, "%.64s (fwd)", hdr->title);
    move(1, 0);
    clrtobot();
    prints("轉達給: %s (%s)\n標  題: %s\n", muser.userid, muser.username, ve_title);

    mail_send(muser.userid);
    *quote_file = '\0';
  }
  return XO_HEAD;
}


/* ----------------------------------------------------- */
/* 板主功能：mark / delete / label			 */
/* ----------------------------------------------------- */


static int
post_mark(xo)
  XO *xo;
{
  if (bbstate & STAT_BOARD)
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

#ifdef HAVE_LABELMARK
    if (xmode & POST_DELETE)	/* 待砍的文章不能 mark */
      return XO_NONE;
#endif

    hdr->xmode = xmode ^ POST_MARKED;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 7);
    outc(post_attr(hdr));
  }
  return XO_NONE;
}


static int
post_bottom(xo)
  XO *xo;
{
  if (bbstate & STAT_BOARD)
  {
    HDR *hdr, post;
    char fpath[64];

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

    hdr_fpath(fpath, xo->dir, hdr);
    hdr_stamp(xo->dir, HDR_LINK | 'A', &post, fpath);
#ifdef HAVE_REFUSEMARK
    post.xmode = POST_BOTTOM | (hdr->xmode & POST_RESTRICT);
#else
    post.xmode = POST_BOTTOM;
#endif
    strcpy(post.owner, hdr->owner);
    strcpy(post.nick, hdr->nick);
    strcpy(post.title, hdr->title);

    rec_add(xo->dir, &post, sizeof(HDR));
    /* btime_update(currbno); */	/* 不需要，因為置底文章不列入未讀 */

    return post_load(xo);	/* 立刻顯示置底文章 */
  }
  return XO_NONE;
}


#ifdef HAVE_REFUSEMARK
static int
post_refuse(xo)		/* itoc.010602: 文章加密 */
  XO *xo;
{
  HDR *hdr;
  int pos, cur;

  if (!cuser.userlevel)	/* itoc.020114: guest 不能對其他 guest 的文章加密 */
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if (is_author(hdr) || (bbstate & STAT_BM))
  {
    hdr->xmode ^= POST_RESTRICT;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 7);
    outc(post_attr(hdr));
  }

  return XO_NONE;
}
#endif


#ifdef HAVE_LABELMARK
static int
post_label(xo)
  XO *xo;
{
  if (bbstate & STAT_BOARD)
  {
    HDR *hdr;
    int pos, cur, xmode;

    pos = xo->pos;
    cur = pos - xo->top;
    hdr = (HDR *) xo_pool + cur;
    xmode = hdr->xmode;

    if (xmode & (POST_MARKED | POST_RESTRICT))	/* mark 或 加密的文章不能待砍 */
      return XO_NONE;

    hdr->xmode = xmode ^ POST_DELETE;
    currchrono = hdr->chrono;
    rec_put(xo->dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono);

    move(3 + cur, 7);
    outc(post_attr(hdr));

    return pos + 1 + XO_MOVE;	/* 跳至下一項 */
  }

  return XO_NONE;
}


static int
post_delabel(xo)
  XO *xo;
{
  int fdr, fsize, xmode;
  char fnew[64], fold[64], *folder;
  HDR *hdr;
  FILE *fpw;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  if (vans("確定要刪除待砍文章嗎(Y/N)？[N] ") != 'y')
    return XO_FOOT;

  folder = xo->dir;
  if ((fdr = open(folder, O_RDONLY)) < 0)
    return XO_FOOT;

  if (!(fpw = f_new(folder, fnew)))
  {
    close(fdr);
    return XO_FOOT;
  }

  fsize = 0;
  mgets(-1);
  while (hdr = mread(fdr, sizeof(HDR)))
  {
    xmode = hdr->xmode;

    if (!(xmode & POST_DELETE))
    {
      if ((fwrite(hdr, sizeof(HDR), 1, fpw) != 1))
      {
	close(fdr);
	fclose(fpw);
	unlink(fnew);
	return XO_FOOT;
      }
      fsize++;
    }
    else
    {
      /* 連線砍信 */
      cancel_post(hdr);

      hdr_fpath(fold, folder, hdr);
      unlink(fold);
    }
  }
  close(fdr);
  fclose(fpw);

  sprintf(fold, "%s.o", folder);
  rename(folder, fold);
  if (fsize)
    rename(fnew, folder);
  else
    unlink(fnew);

  btime_update(currbno);

  return post_load(xo);
}
#endif


static int
post_delete(xo)
  XO *xo;
{
  int pos, cur, by_BM;
  HDR *hdr;
  char buf[80];

  if (!cuser.userlevel ||
    !strcmp(currboard, BN_DELETED) ||
    !strcmp(currboard, BN_JUNK))
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

  if ((hdr->xmode & POST_MARKED) || (!(bbstate & STAT_BOARD) && !is_author(hdr)))
    return XO_NONE;

  by_BM = bbstate & STAT_BOARD;

  if (vans(msg_del_ny) == 'y')
  {
    currchrono = hdr->chrono;

    if (!rec_del(xo->dir, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono))
    {
      pos = move_post(hdr, xo->dir, by_BM);

      if (!by_BM && !(currbattr & BRD_NOCOUNT) && !(hdr->xmode & POST_BOTTOM))
      {
	/* itoc.010711: 砍文章要扣錢，算檔案大小 */
	pos = pos >> 3;	/* 相對於 post 時 wordsnum / 10 */

	/* itoc.010830.註解: 漏洞: 若 multi-login 砍不到另一隻的錢 */
	if (cuser.money > pos)
	  cuser.money -= pos;
	else
	  cuser.money = 0;

	if (cuser.numposts > 0)
	  cuser.numposts--;
	sprintf(buf, "%s，您的文章減為 %d 篇", MSG_DEL_OK, cuser.numposts);
	vmsg(buf);
      }

      if (xo->key == XZ_XPOST)
      {
	vmsg("原列表經刪除後混亂，請重進串接模式！");
	return XO_QUIT;
      }
      return XO_LOAD;
    }
  }
  return XO_FOOT;
}


static int
chkpost(hdr)
  HDR *hdr;
{
  return (hdr->xmode & POST_MARKED);
}


static int
vfypost(hdr, pos)
  HDR *hdr;
  int pos;
{
  return (Tagger(hdr->chrono, pos, TAG_NIN) || chkpost(hdr));
}


static void
delpost(xo, hdr)
  XO *xo;
  HDR *hdr;
{
  char fpath[64];

  cancel_post(hdr);
  hdr_fpath(fpath, xo->dir, hdr);
  unlink(fpath);
}


static int
post_rangedel(xo)
  XO *xo;
{
  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  btime_update(currbno);

  return xo_rangedel(xo, sizeof(HDR), chkpost, delpost);
}


static int
post_prune(xo)
  XO *xo;
{
  int ret;

  if (!(bbstate & STAT_BOARD))
    return XO_NONE;

  ret = xo_prune(xo, sizeof(HDR), vfypost, delpost);

  btime_update(currbno);

  if (xo->key == XZ_XPOST && ret == XO_LOAD)
  {
    vmsg("原列表經批次刪除後混亂，請重進串接模式！");
    return XO_QUIT;
  }

  return ret;
}


static int
post_copy(xo)	   /* itoc.010924: 取代 gem_gather */
  XO *xo;
{
  int tag;

  tag = AskTag("看板文章拷貝");

  if (tag < 0)
    return XO_FOOT;

#ifdef HAVE_REFUSEMARK
  gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), chkrestrict);
#else
  gem_buffer(xo->dir, tag ? NULL : (HDR *) xo_pool + (xo->pos - xo->top), NULL);
#endif

  if (bbstate & STAT_BOARD)
  {
#ifdef XZ_XPOST
    if (xo->key == XZ_XPOST)
    {
      zmsg("檔案標記完成。[注意] 您必須先離開串接模式才能進入精華區。");
      return XO_FOOT;
    }
    else
#endif
    {
      zmsg("拷貝完成。[注意] 貼上後才能刪除原文！");
      return post_gem(xo);	/* 拷貝完直接進精華區 */
    }
  }

  zmsg("檔案標記完成。[注意] 您只能在擔任(小)板主所在或個人精華區貼上。");
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 站長功能：edit / title				 */
/* ----------------------------------------------------- */


int
post_edit(xo)
  XO *xo;
{
  char fpath[64];
  HDR *hdr;
  FILE *fp;

  hdr = (HDR *) xo_pool + (xo->pos - xo->top);

  hdr_fpath(fpath, xo->dir, hdr);

  if (HAS_PERM(PERM_ALLBOARD))			/* 站長修改 */
  {
#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      return XO_NONE;
#endif
    vedit(fpath, 0);
  }
  else if (cuser.userlevel && is_author(hdr))	/* 原作者修改 */
  {
    if (!vedit(fpath, 0))	/* 若非取消則加上修改資訊 */
    {
      if (fp = fopen(fpath, "a"))
      {
	ve_banner(fp, 1);
	fclose(fp);
      }
    }
  }
  else		/* itoc.010301: 提供使用者修改(但不能儲存)其他人發表的文章 */
  {
#ifdef HAVE_REFUSEMARK
    if (hdr->xmode & POST_RESTRICT)
      return XO_NONE;
#endif
    vedit(fpath, -1);
  }

  /* return post_head(xo); */
  return XO_HEAD;	/* itoc.021226: XZ_POST 和 XZ_XPOST 共用 post_edit() */
}


void
header_replace(xo, hdr)		/* itoc.010709: 修改文章標題順便修改內文的標題 */
  XO *xo;
  HDR *hdr;
{
  FILE *fpr, *fpw;
  char srcfile[64], tmpfile[64], buf[ANSILINELEN];
  
  hdr_fpath(srcfile, xo->dir, hdr);
  strcpy(tmpfile, "tmp/");
  strcat(tmpfile, hdr->xname);
  f_cp(srcfile, tmpfile, O_TRUNC);

  if (!(fpr = fopen(tmpfile, "r")))
    return;

  if (!(fpw = fopen(srcfile, "w")))
  {
    fclose(fpr);
    return;
  }

  fgets(buf, sizeof(buf), fpr);		/* 加入作者 */
  fputs(buf, fpw);

  fgets(buf, sizeof(buf), fpr);		/* 加入標題 */
  if (!str_ncmp(buf, "標", 2))		/* 如果有 header 才改 */
  {
    strcpy(buf, buf[2] == ' ' ? "標  題: " : "標題: ");
    strcat(buf, hdr->title);
    strcat(buf, "\n");
  }
  fputs(buf, fpw);

  while(fgets(buf, sizeof(buf), fpr))	/* 加入其他 */
    fputs(buf, fpw);

  fclose(fpr);
  fclose(fpw);
  f_rm(tmpfile);
}


static int
post_title(xo)
  XO *xo;
{
  HDR *fhdr, mhdr;
  int pos, cur;

  if (!cuser.userlevel)	/* itoc.000213: 避免 guest 在 sysop 板改標題 */
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  fhdr = (HDR *) xo_pool + cur;
  memcpy(&mhdr, fhdr, sizeof(HDR));

  if (!is_author(&mhdr) && !HAS_PERM(PERM_ALLBOARD))
    return XO_NONE;

  vget(b_lines, 0, "標題：", mhdr.title, TTLEN + 1, GCARRY);

  if (HAS_PERM(PERM_ALLBOARD))  /* itoc.000213: 原作者只能改標題 */
  {
    vget(b_lines, 0, "作者：", mhdr.owner, 73 /* sizeof(mhdr.owner) */, GCARRY);
		/* Thor.980727: sizeof(mhdr.owner) = 80 會超過一行 */
    vget(b_lines, 0, "暱稱：", mhdr.nick, sizeof(mhdr.nick), GCARRY);
    vget(b_lines, 0, "日期：", mhdr.date, sizeof(mhdr.date), GCARRY);
  }

  if (memcmp(fhdr, &mhdr, sizeof(HDR)) && vans(msg_sure_ny) == 'y')
  {
    memcpy(fhdr, &mhdr, sizeof(HDR));
    currchrono = fhdr->chrono;
    rec_put(xo->dir, fhdr, sizeof(HDR), xo->key == XZ_XPOST ? fhdr->xid : pos, cmpchrono);

    move(3 + cur, 0);
    post_item(++pos, fhdr);

    /* itoc.010709: 修改文章標題順便修改內文的標題 */
    header_replace(xo, fhdr);
  }
  return XO_FOOT;
}


/* ----------------------------------------------------- */
/* 額外功能：write / score				 */
/* ----------------------------------------------------- */


int
post_write(xo)			/* itoc.010328: 丟線上作者水球 */
  XO *xo;
{
  if (HAS_PERM(PERM_PAGE))
  {
    HDR *hdr;
    UTMP *up;

    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

    if (!(hdr->xmode & POST_INCOME) && (up = utmp_seek(hdr)))
      do_write(up);
  }
  return XO_NONE;
}


#ifdef HAVE_SCORE

static int curraddscore;


static void
addscore(hdd, ram)
  HDR *hdd, *ram;
{
  hdd->xmode |= POST_SCORE;
  if (curraddscore > 0)
  {
    if (hdd->score < 35)
      hdd->score++;
  }
  else
  {
    if (hdd->score > -35)
      hdd->score--;
  }
}


int
post_score(xo)
  XO *xo;
{
  HDR *hdr;
  int pos, cur, ans, vtlen, maxlen;
  char *dir, *userid, *verb, fpath[64], reason[80], vtbuf[12];
  FILE *fp;
#ifdef HAVE_ANONYMOUS
  char uid[IDLEN + 1];
#endif

  if ((currbattr & BRD_NOSCORE) || !cuser.userlevel || !(bbstate & STAT_POST))	/* 評分視同發表文章 */
    return XO_NONE;

  pos = xo->pos;
  cur = pos - xo->top;
  hdr = (HDR *) xo_pool + cur;

#ifdef HAVE_REFUSEMARK
  if (!chkrestrict(hdr))
    return XO_NONE;
#endif

  switch (ans = vans("◎ 評分 1)推文 2)唾棄 3)自定推 4)自定呸？[Q] "))
  {
  case '1':
    verb = "1m推";
    vtlen = 2;
    break;

  case '2':
    verb = "2m呸";
    vtlen = 2;
    break;

  case '3':
  case '4':
    if (!vget(b_lines, 0, "請輸入動詞：", fpath, 5, DOECHO))
      return XO_FOOT;
    vtlen = strlen(fpath);
    sprintf(verb = vtbuf, "%cm%s", ans - 2, fpath);
    break;

  default:
    return XO_FOOT;
  }

#ifdef HAVE_ANONYMOUS
  if (currbattr & BRD_ANONYMOUS)
    maxlen = 64 - IDLEN - vtlen;
  else
#endif
    maxlen = 64 - strlen(cuser.userid) - vtlen;

  if (!vget(b_lines, 0, "請輸入理由：", reason, maxlen, DOECHO))
    return XO_FOOT;

#ifdef HAVE_ANONYMOUS
  if (currbattr & BRD_ANONYMOUS)
  {
    userid = uid;
    if (!vget(b_lines, 0, "請輸入您想用的ID，也可直接按[Enter]，或是按[r]用真名：", userid, IDLEN, DOECHO))
      userid = STR_ANONYMOUS;
    else if (userid[0] == 'r' && userid[1] == '\0')
      userid = cuser.userid;
    else
      strcat(userid, ".");		/* 自定的話，最後加 '.' */
    maxlen = 64 - strlen(userid) - vtlen;
  }
  else
#endif
    userid = cuser.userid;

  dir = xo->dir;
  hdr_fpath(fpath, dir, hdr);

  if (fp = fopen(fpath, "a"))
  {
    time_t now;
    struct tm *ptime;

    time(&now);
    ptime = localtime(&now);

    fprintf(fp, "→ \033[36m%s \033[3%s\033[m：%-*s%02d/%02d/%02d\n", 
      userid, verb, maxlen, reason, 
      ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
    fclose(fp);
  }

  curraddscore = 0;
  if ((ans - '0') & 0x01)	/* 加分 */
  {
    if (hdr->score < 35)
      curraddscore = 1;
  }
  else				/* 扣分 */
  {
    if (hdr->score > -35)
      curraddscore = -1;
  }

  if (curraddscore)
  {
    currchrono = hdr->chrono;
    rec_ref(dir, hdr, sizeof(HDR), xo->key == XZ_XPOST ? hdr->xid : pos, cmpchrono, addscore);
    return XO_LOAD;
  }

  return XO_FOOT;
}
#endif	/* HAVE_SCORE */


static int
post_help(xo)
  XO *xo;
{
  xo_help("post");
  /* return post_head(xo); */
  return XO_HEAD;		/* itoc.001029: 與 xpost_help 共用 */
}


KeyFunc post_cb[] =
{
  XO_INIT, post_init,
  XO_LOAD, post_load,
  XO_HEAD, post_head,
  XO_BODY, post_body,

  'r', post_browse,
  's', post_switch,
  KEY_TAB, post_gem,
  'z', post_gem,

  'y', post_reply,
  'd', post_delete,
  'v', post_visit,
  'x', post_cross,		/* 在 post/mbox 中都是小寫 x 轉看板，大寫 X 轉使用者 */
  'X', post_forward,
  't', post_tag,
  'E', post_edit,
  'T', post_title,
  'm', post_mark,
  '_', post_bottom,
  'D', post_rangedel,

#ifdef HAVE_SCORE
  '%', post_score,
#endif

  'w', post_write,

  'b', post_memo,
  'c', post_copy,
  'g', gem_gather,

  Ctrl('P'), post_add,
  Ctrl('D'), post_prune,
  Ctrl('Q'), xo_uquery,
  Ctrl('O'), xo_usetup,

#ifdef HAVE_REFUSEMARK
  Ctrl('Y'), post_refuse,
#endif

#ifdef HAVE_LABELMARK
  'n', post_label,
  Ctrl('N'), post_delabel,
#endif

  'B' | XO_DL, (void *) "bin/manage.so:post_manage",
  'R' | XO_DL, (void *) "bin/vote.so:vote_result",
  'V' | XO_DL, (void *) "bin/vote.so:XoVote",

#ifdef HAVE_TERMINATOR
  Ctrl('X') | XO_DL, (void *) "bin/manage.so:post_terminator",
#endif

  '~', XoXselect,		/* itoc.001220: 搜尋作者/標題 */
  'S', XoXsearch,		/* itoc.001220: 搜尋相同標題文章 */
  'a', XoXauthor,		/* itoc.001220: 搜尋作者 */
  '/', XoXtitle,		/* itoc.001220: 搜尋標題 */
  'f', XoXfull,			/* itoc.030608: 全文搜尋 */
  'G', XoXmark,			/* itoc.010325: 搜尋 mark 文章 */
  'L', XoXlocal,		/* itoc.010822: 搜尋本地文章 */

#ifdef HAVE_XYNEWS
  'u', XoNews,			/* itoc.010822: 新聞閱讀模式 */
#endif

  'h', post_help
};


KeyFunc xpost_cb[] =
{
  XO_INIT, xpost_init,
  XO_LOAD, xpost_load,
  XO_HEAD, xpost_head,
  XO_BODY, post_body,		/* Thor.980911: 共用即可 */

  'r', xpost_browse,
  'y', post_reply,
  't', post_tag,
  'x', post_cross,
  'X', post_forward,
  'c', post_copy,
  'g', gem_gather,
  'm', post_mark,
  'd', post_delete,		/* Thor.980911: 方便板主 */
  'E', post_edit,		/* itoc.010716: 提供 XPOST 中可以編輯標題、文章，加密 */
  'T', post_title,
#ifdef HAVE_SCORE
  '%', post_score,
#endif
  'w', post_write,
#ifdef HAVE_REFUSEMARK
  Ctrl('Y'), post_refuse,
#endif
#ifdef HAVE_LABELMARK
  'n', post_label,
#endif

  '~', XoXselect,
  'S', XoXsearch,
  'a', XoXauthor,
  '/', XoXtitle,
  'f', XoXfull,
  'G', XoXmark,
  'L', XoXlocal,

  Ctrl('P'), post_add,
  Ctrl('D'), post_prune,
  Ctrl('Q'), xo_uquery,
  Ctrl('O'), xo_usetup,

  'h', post_help		/* itoc.030511: 共用即可 */
};


#ifdef HAVE_XYNEWS
KeyFunc news_cb[] =
{
  XO_INIT, news_init,
  XO_LOAD, news_load,
  XO_HEAD, news_head,
  XO_BODY, post_body,

  'r', XoXsearch,

  'h', post_help		/* itoc.030511: 共用即可 */
};
#endif	/* HAVE_XYNEWS */
