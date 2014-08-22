/*-------------------------------------------------------*/
/* xpost.c      ( NTHU CS MapleBBS Ver 2.39 )		 */
/*-------------------------------------------------------*/
/* target : bulletin boards' routines		 	 */
/* create : 95/03/29				 	 */
/* update : 96/04/05				 	 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern XZ xz[];

extern char xo_pool[];


/*------------------------------------------------------------------------
  Thor.980509:
  新的 文章搜尋模式 可指定一 keyword, 列出所有keyword相關之文章列表

  在 tmp/ 下開 xpost.{pid} 作為 folder, 另建一map陣列, 用作與原 post 作 map
  記載該文章是在原 post 的何處, 如此可作 mark, gem, edit, title 等功能,
  且能離開時回至對應文章處
  <以上想法 obsolete...>

  Thor.980510:
  建立文章討論串, like tin, 將文章串 index 放入 memory 中,
  不使用 thread, 因為 thread要用 folder 檔...

  分為兩種 Mode, Title & post list

  但考慮提供簡化的 上下鍵移動..

  O->O->O->...
  |  |  |
  o  o  o
  |  |  |

  index含field {next, text} 均為int, 配置也用 int
  第一層 sorted by title, 插入時用 binary search
  且 MMAP only , 第一層顯示 # and +

  不提供任何區段刪除動作, 避免混亂
-------------------------------------------------------------------------*/

#if 0	/* itoc.060206.註解 */

  當使用者輸入搜尋條件後，會進入 XoXpost() 將 xo->dir 這檔案中所記錄的所有 HDR
  一一瀏覽，然後將滿足條件的 HDR 在 xo->dir 中所對應位置記錄在 xpostIndex[]，
  接著進入 xover(XZ_XPOST) 會呼叫 xpost_init()，再於 xpost_pick() 將 xpostIndex[]
  所記錄的位置從 xo->dir 抄到 xo_pool[]。

  當增加條件做二次搜尋時，此時不需要掃整個 xo->dir 內的所有 HDR，只瀏覽記錄在
  xpostIndex[] 裡面的那些。由於二次搜尋時要看同一個 xo->dir，所以 every_Z 時
  要禁止進入 XZ_XPOST 二次。

  已知問題是：當使用者還在 XZ_XPOST 裡面時，若 xo->dir 的順序有異動時 (例如刪除)，
  而使用者要求 xpick_pick() 時 (例如翻頁、二次搜尋)，由於 xpostIndex[] 記錄的是在 
  xo->dir 的位置，此時結果會出錯。

#endif

/* ----------------------------------------------------- */
/* 串列搜尋主程式					 */
/* ----------------------------------------------------- */


#ifdef EVERY_Z
#define MSG_XYDENY	"請先退出使用 ^Z 以前的串接/新聞功\能"
extern int z_status;
#endif

extern KeyFunc xpost_cb[];
extern KeyFunc xmbox_cb[];

static int *xpostIndex;		/* Thor: first ypost pos in ypost_xo.key */
static int comebackPos;		/* 記錄最後閱讀那篇文章的位置 */


static char HintWord[TTLEN + 1];
static char HintAuthor[IDLEN + 1];


static int
XoXpost(xo, hdr, on, off, fchk)		/* Thor: eXtended post : call from post_cb */
  XO *xo;
  HDR *hdr;		/* 搜尋的條件 */
  int on, off;		/* 搜尋的範圍 (on~off-1) */
  int (*fchk) ();	/* 搜尋的函式 */
{
  int *list, fsize, max, locus, count, i;
  char *fimage;
  HDR *head;
  XO *xt;
#ifdef HAVE_XYNEWS
  int returnPos;
#endif

  if (xo->max <= 0)	/* Thor.980911.註解: 以防萬一 */
    return XO_FOOT;
  
  /* build index according to input condition */

  fimage = f_map(xo->dir, &fsize);

  if (fimage == (char *) -1)
  {
    vmsg("目前無法開啟索引檔");
    return XO_FOOT;
  }

  /* allocate index memory, remember free first */

  /* Thor.990113: 怕問title, author的瞬間又有人post */
  max = xpostIndex ?  xo->max : fsize / sizeof(HDR);
  list = (int *) malloc(sizeof(int) * max);

  count = 0;			/* 總共有幾篇滿足條件 */

  if (max > off)
    max = off;

  for (i = on; i < max; i++)
  {
    if (xpostIndex)		/* 增加條件再次搜尋時，只需要找在 xpostIndex[] 裡面的 */
      locus = xpostIndex[i];
    else			/* 整個 xo->dir 都掃一次 */
      locus = i;

    head = (HDR *) fimage + locus;

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(head))
      continue;
#endif

    /* check condition */
    if (!(* fchk) (head, hdr))
      continue;

    list[count++] = locus;
  }

  munmap(fimage, fsize);

  if (count <= 0)
  {
    free(list);
    vmsg(MSG_XY_NONE);
    return XO_FOOT;
  }

  /* 增加條件再次搜尋 */
  if (xpostIndex)
  {
    free(xpostIndex);
    xpostIndex = list;

    xo->pos = 0;
    xo->max = count;

    return xpost_init(xo);
  }

  /* 首次搜尋 */
  xpostIndex = list;

  /* build XO for xpost_xo */

  comebackPos = xo->pos;	/* Thor: record pos, future use */
#ifdef HAVE_XYNEWS
  returnPos = comebackPos;
#endif

  xz[XZ_XPOST - XO_ZONE].xo = xt = xo_new(xo->dir);
  xz[XZ_XPOST - XO_ZONE].cb = (xo->dir[0] == 'b') ? xpost_cb : xmbox_cb;
  xt->pos = 0;
  xt->max = count;
  xt->xyz = xo->xyz;
  xt->key = XZ_XPOST;

  xover(XZ_XPOST);

  /* set xo->pos for new location */

#ifdef HAVE_XYNEWS
  if (xz[XZ_NEWS - XO_ZONE].xo)
    xo->pos = returnPos;	/* 從 XZ_XPOST 回到 XZ_NEWS 游標移去原來的地方 */
  else
#endif
    xo->pos = comebackPos;	/* 從 XZ_XPOST 回到 XZ_POST 游標移去原來的地方或所閱讀文章的真正位置 */

  /* free xpost_xo */

  if (xt = xz[XZ_XPOST - XO_ZONE].xo)
  {
    free(xt);
    xz[XZ_XPOST - XO_ZONE].xo = NULL;
  }

  /* free index memory, remember check free pointer */

  if (xpostIndex)
  {
    free(xpostIndex);
    xpostIndex = NULL;
  }

  return XO_INIT;
}


  /* --------------------------------------------------- */
  /* 搜尋作者/標題					 */
  /* --------------------------------------------------- */


static int			/* 0:不滿足條件  !=0:滿足條件 */
filter_select(head, hdr)
  HDR *head;	/* 待測物 */
  HDR *hdr;	/* 條件 */
{
  char *title;
  usint str4;

  /* 借用 hdr->xid 當 strlen(hdr->owner) */

  /* Thor.981109: 特別注意，為了降低 load，author 是從頭 match，不是 substr match */
  if (hdr->xid && str_ncmp(head->owner, hdr->owner, hdr->xid))
    return 0;

  if (hdr->title[0])
  {
    title = head->title;
    str4 = STR4(title);
    if (str4 == STR4(STR_REPLY) || str4 == STR4(STR_FORWARD))	/* Thor.980911: 先把 Re:/Fw: 除外 */
      title += 4;
    if (!str_sub(title, hdr->title))
      return 0;
  }

  return 1;
}


int
XoXselect(xo)
  XO *xo;
{
  HDR hdr;
  char *key;

#ifdef EVERY_Z
  if (z_status && xz[XZ_XPOST - XO_ZONE].xo)	/* itoc.020308: 不得累積進入二次 */
  {
    vmsg(MSG_XYDENY);
    return XO_FOOT;
  }
#endif

  /* input condition */

  key = hdr.title;
  if (vget(b_lines, 0, MSG_XYPOST1, key, 30, DOECHO))
  {
    strcpy(HintWord, key);
    str_lowest(key, key);
  }
  else
  {
    HintWord[0] = '\0';
  }

  key = hdr.owner;
  if (vget(b_lines, 0, MSG_XYPOST2, key, IDLEN + 1, DOECHO))
  {
    strcpy(HintAuthor, key);
    str_lower(key, key);
    hdr.xid = strlen(key);
  }
  else
  {
    HintAuthor[0] = '\0';
    hdr.xid = 0;
  }
  
  if (!hdr.title[0] && !hdr.xid)
    return XO_FOOT;

  return XoXpost(xo, &hdr, 0, INT_MAX, filter_select);
}


  /* --------------------------------------------------- */
  /* 搜尋作者						 */
  /* --------------------------------------------------- */


int
XoXauthor(xo)
  XO *xo;
{
  HDR hdr;
  char *author;

#ifdef EVERY_Z
  if (z_status && xz[XZ_XPOST - XO_ZONE].xo)	/* itoc.020308: 不得累積進入二次 */
  {
    vmsg(MSG_XYDENY);
    return XO_FOOT;
  }
#endif

  author = hdr.owner;
  if (!vget(b_lines, 0, MSG_XYPOST2, author, IDLEN + 1, DOECHO))
    return XO_FOOT;

  HintWord[0] = '\0';
  strcpy(HintAuthor, author);

  hdr.title[0] = '\0';
  str_lower(author, author);
  hdr.xid = strlen(author);

  return XoXpost(xo, &hdr, 0, INT_MAX, filter_select);
}


  /* --------------------------------------------------- */
  /* 搜尋標題						 */
  /* --------------------------------------------------- */


int
XoXtitle(xo)
  XO *xo;
{
  HDR hdr;
  char *title;

#ifdef EVERY_Z
  if (z_status && xz[XZ_XPOST - XO_ZONE].xo)	/* itoc.020308: 不得累積進入二次 */
  {
    vmsg(MSG_XYDENY);
    return XO_FOOT;
  }
#endif

  title = hdr.title;
  if (!vget(b_lines, 0, MSG_XYPOST1, title, 30, DOECHO))
    return XO_FOOT;

  strcpy(HintWord, title);
  HintAuthor[0] = '\0';

  str_lowest(title, title);
  hdr.xid = 0;

  return XoXpost(xo, &hdr, 0, INT_MAX, filter_select);
}


  /* --------------------------------------------------- */
  /* 搜尋相同標題					 */
  /* --------------------------------------------------- */


static int			/* 0:不滿足條件  !=0:滿足條件 */
filter_search(head, hdr)
  HDR *head;	/* 待測物 */
  HDR *hdr;	/* 條件 */
{
  char *title, buf[TTLEN + 1];
  usint str4;

  title = head->title;
  str4 = STR4(title);
  if (str4 == STR4(STR_REPLY) || str4 == STR4(STR_FORWARD))	/* Thor.980911: 先把 Re:/Fw: 除外 */
    title += 4;
  str_lowest(buf, title);
  return !strcmp(buf, hdr->title);
}


int
XoXsearch(xo)
  XO *xo;
{
  HDR hdr, *mhdr;
  char *title;
  usint str4;

#ifdef EVERY_Z
  if (z_status && xz[XZ_XPOST - XO_ZONE].xo)	/* itoc.020308: 不得累積進入二次 */
  {
    vmsg(MSG_XYDENY);
    return XO_FOOT;
  }
#endif

  mhdr = (HDR *) xo_pool + (xo->pos - xo->top);

  title = mhdr->title;
  str4 = STR4(title);
  if (str4 == STR4(STR_REPLY) || str4 == STR4(STR_FORWARD))	/* Thor.980911: 先把 Re:/Fw: 除外 */
    title += 4;

  strcpy(HintWord, title);
  HintAuthor[0] = '\0';

  str_lowest(hdr.title, title);

  return XoXpost(xo, &hdr, 0, INT_MAX, filter_search);
}


  /* --------------------------------------------------- */
  /* 全文搜尋						 */
  /* --------------------------------------------------- */


static char *search_folder;
static int search_fit;		/* >=0:找到幾篇 -1:中斷搜尋 */
static int search_all;		/* 已搜尋幾篇 */

static int			/* 0:不滿足條件  !=0:滿足條件 */
filter_full(head, hdr)
  HDR *head;	/* 待測物 */
  HDR *hdr;	/* 條件 */
{
  char buf[80], *fimage;
  int rc, fsize;
  struct timeval tv = {0, 10};

  if (search_fit < 0)		/* 中斷搜尋 */
    return 0;

  if (search_all % 100 == 0)	/* 每 100 篇才報告一次進度 */
  {
    sprintf(buf, "目前找到 \033[1;33m%d / %d\033[m 篇，全文搜尋中\033[5m...\033[m按任意鍵中斷", 
      search_fit, search_all);
    outz(buf);
    refresh();
  }
  search_all++;

  hdr_fpath(buf, search_folder, head);

  fimage = f_map(buf, &fsize);
  if (fimage == (char *) -1)
    return 0;

  rc = 0;
  if (str_sub(fimage, hdr->title))
  {
    rc = 1;
    search_fit++;
  }

  munmap(fimage, fsize);

  /* 使用者可以中斷搜尋 */
  fsize = 1;
  if (select(1, (fd_set *) &fsize, NULL, NULL, &tv) > 0)
  {
    vkey();
    search_fit = -1;
  }

  return rc;
}


int
XoXfull(xo)
  XO *xo;
{
  HDR hdr;
  char *key, ans[8];
  int head, tail;

#ifdef EVERY_Z
  if (z_status && xz[XZ_XPOST - XO_ZONE].xo)	/* itoc.020308: 不得累積進入二次 */
  {
    vmsg(MSG_XYDENY);
    return XO_FOOT;
  }
#endif

  /* input condition */

  key = hdr.title;
  if (!vget(b_lines, 0, "內文關鍵字：", key, 30, DOECHO))
    return XO_FOOT;

  vget(b_lines, 0, "[設定搜尋範圍] 起點：(Enter)從頭開始 ", ans, 6, DOECHO);
  if ((head = atoi(ans)) <= 0)
    head = 1; 

  vget(b_lines, 44, "終點：(Enter)找到最後 ", ans, 6, DOECHO);
  if ((tail = atoi(ans)) < head)
    tail = INT_MAX;

  head--;

  sprintf(HintWord, "[全文搜尋] %s", key);
  HintAuthor[0] = '\0';
  str_lowest(key, key);

  search_folder = xo->dir;
  search_fit = 0;
  search_all = 0;

  return XoXpost(xo, &hdr, head, tail, filter_full);
}


  /* --------------------------------------------------- */
  /* 搜尋 mark						 */
  /* --------------------------------------------------- */


static int			/* 0:不滿足條件  !=0:滿足條件 */
filter_mark(head, hdr)
  HDR *head;	/* 待測物 */
  HDR *hdr;	/* 條件 */
{
  return (head->xmode & POST_MARKED);
}


int
XoXmark(xo)
  XO *xo;
{
#ifdef EVERY_Z
  if (z_status && xz[XZ_XPOST - XO_ZONE].xo)	/* itoc.020308: 不得累積進入二次 */
  {
    vmsg(MSG_XYDENY);
    return XO_FOOT;
  }
#endif

  strcpy(HintWord, "\033[1;33m所有 mark 文章\033[m");
  HintAuthor[0] = '\0';

  return XoXpost(xo, NULL, 0, INT_MAX, filter_mark);
}


  /* --------------------------------------------------- */
  /* 搜尋本地						 */
  /* --------------------------------------------------- */


static int			/* 0:不滿足條件  !=0:滿足條件 */
filter_local(head, hdr)
  HDR *head;	/* 待測物 */
  HDR *hdr;	/* 條件 */
{
  return !(head->xmode & POST_INCOME);
}


int
XoXlocal(xo)
  XO *xo;
{
  if (currbattr & BRD_NOTRAN)
  {
    vmsg("本板為不轉信板，全部都是本地文章");
    return XO_FOOT;
  }

#ifdef EVERY_Z
  if (z_status && xz[XZ_XPOST - XO_ZONE].xo)	/* itoc.020308: 不得累積進入二次 */
  {
    vmsg(MSG_XYDENY);
    return XO_FOOT;
  }
#endif

  strcpy(HintWord, "\033[1;33m所有非轉進文章\033[m");
  HintAuthor[0] = '\0';

  return XoXpost(xo, NULL, 0, INT_MAX, filter_local);
}


/* ----------------------------------------------------- */
/* 串列搜尋界面						 */
/* ----------------------------------------------------- */


int
xpost_head(xo)
  XO *xo;
{
  vs_head("主題串列", xo->xyz);

  /* itoc.010323: 同時提示作者/主題 */
  outs("[串接系列] ");
  if (*HintAuthor)
    prints("作者：%-13s   ", HintAuthor);
  if (*HintWord)
    prints("標題：%.30s", HintWord);

  prints(NECKER_XPOST, d_cols, "", currbattr & BRD_NOSCORE ? "╳" : "○");

  return XO_BODY;
}


static void
xpost_pick(xo)
  XO *xo;
{
  int *list, fsize, pos, max, top, num;
  HDR *fimage, *hdr;

  fimage = (HDR *) f_map(xo->dir, &fsize);
  if (fimage == (HDR *) - 1)
    return;

  hdr = (HDR *) xo_pool;
  list = xpostIndex;

  pos = xo->pos;
  xo->top = top = (pos / XO_TALL) * XO_TALL;
  max = xo->max;
  pos = top + XO_TALL;
  if (max > pos)
    max = pos;
  num = fsize / sizeof(HDR);

  do
  {
    pos = list[top++];
    if (pos >= num)	/* hightman.030528: 避免 .DIR 被刪減時，會沒有文章可以顯示 */
      continue;
    memcpy(hdr, fimage + pos, sizeof(HDR));
    hdr->xid = pos;		/* 用 hdr->xid 來記錄其原先在看板中的 pos */
    hdr++;
  } while (top < max);

  munmap(fimage, fsize);
}


int
xpost_init(xo)
  XO *xo;
{
  /* load into pool */

  xpost_pick(xo);

  return xpost_head(xo);
}


int
xpost_load(xo)
  XO *xo;
{
  /* load into pool */

  xpost_pick(xo);

  return XO_BODY;
}


static void
xpost_history(xo, fhdr)		/* 將 fhdr 這篇加入 brh */
  XO *xo;
  HDR *fhdr;
{
  time_t prev, chrono, next;
  int pos;
  char *dir;
  HDR buf;

  chrono = fhdr->chrono;
  if (!brh_unread(chrono))	/* 如果已在 brh 中，就無需動作 */
    return;

  dir = xo->dir;
  pos = fhdr->xid;

  if (!rec_get(dir, &buf, sizeof(HDR), pos - 1))
    prev = buf.chrono;
  else
    prev = chrono;

  if (!rec_get(dir, &buf, sizeof(HDR), pos + 1))
    next = buf.chrono;
  else
    next = chrono;

  brh_add(prev, chrono, next);
}


int
xpost_browse(xo)
  XO *xo;
{
  HDR *hdr;
  int key;
  char *dir, fpath[64];

  dir = xo->dir;

  for (;;)
  {
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

#if 0	/* itoc.010822: 不需要，在 XoXpost() 中已被剔除 */
#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(hdr))
      continue;
#endif
#endif

    hdr_fpath(fpath, dir, hdr);

    /* Thor.990204: 為考慮more 傳回值 */   
    if ((key = more(fpath, FOOTER_POST)) < 0)
      break;

    comebackPos = hdr->xid; 
    /* Thor.980911: 從串接模式回來時要回到看過的那篇文章位置 */

    xpost_history(xo, hdr);
    strcpy(currtitle, str_ttl(hdr->title));

re_key:
    /* Thor.990204: 為考慮more 傳回值 */   
    if (!key)
      key = vkey();

    switch (key)
    {
    case KEY_UP:
    case KEY_PGUP:
    case '[':	/* itoc.000227: 串列搜尋中，有時想用 [ 看上一篇文章 */
    case 'k':	/* itoc.000227: 串列搜尋中，有時想用 k 看上一篇文章 */
      {
	int pos = xo->pos - 1;

	/* itoc.000227: 避免看過頭 */
	if (pos < 0)
	  return xpost_head(xo);

	xo->pos = pos;

	if (pos <= xo->top)
	  xpost_pick(xo);
  
	continue;
      }

    case KEY_DOWN:
    case KEY_PGDN:
    case ']':	/* Thor.990204: 串列搜尋中，有時想用 ] 看下一篇文章 */
    case 'j':	/* Thor.990204: 串列搜尋中，有時想用 j 看下一篇文章 */
    case ' ':
      {
	int pos = xo->pos + 1;

	/* Thor.980727: 修正看過頭的bug */

	if (pos >= xo->max)
    	  return xpost_head(xo);

	xo->pos = pos;

	if (pos >= xo->top + XO_TALL)
  	  xpost_pick(xo);

	continue;
      }

    case 'y':
    case 'r':
      if (bbstate & STAT_POST)
      {
	if (do_reply(xo, hdr) == XO_INIT)	/* 有成功地 post 出去了 */
	  return xpost_init(xo);
      }
      break;

    case 'm': 
      if ((bbstate & STAT_BOARD) && !(hdr->xmode & POST_MARKED))
      {
	/* 在 xpost_browse 時看不到 m 記號，所以限制只能 mark */
	hdr->xmode ^= POST_MARKED;
	currchrono = hdr->chrono;
	rec_put(dir, hdr, sizeof(HDR), hdr->xid, cmpchrono);
      } 
      break;

#ifdef HAVE_SCORE
    case '%': 
      post_score(xo);
      return xpost_init(xo);
#endif

    case '/':
      if (vget(b_lines, 0, "搜尋：", hunt, sizeof(hunt), DOECHO))
      {
	key = more(fpath, FOOTER_POST);
	goto re_key;
      }
      continue;

    case 'E':
      return post_edit(xo);

    case 'C':	/* itoc.000515: xpost_browse 時可存入暫存檔 */
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

  return xpost_head(xo);
}


int
xmbox_browse(xo)
  XO *xo;
{
  HDR *hdr;
  char *dir, fpath[64];

  int key;

  dir = xo->dir;

  for (;;)
  {
    hdr = (HDR *) xo_pool + (xo->pos - xo->top);

    hdr_fpath(fpath, dir, hdr);

    /* Thor.990204: 為考慮more 傳回值 */   
    if ((key = more(fpath, FOOTER_MAILER)) < 0)
      break;

    comebackPos = hdr->xid; 
    /* Thor.980911: 從串接模式回來時要回到看過的那篇文章位置 */

    strcpy(currtitle, str_ttl(hdr->title));

re_key:
    /* Thor.990204: 為考慮more 傳回值 */   
    if (!key)
      key = vkey();

    switch (key)
    {
    case KEY_UP:
    case KEY_PGUP:
    case '[':	/* itoc.000227: 串列搜尋中，有時想用 [ 看上一篇文章 */
    case 'k':	/* itoc.000227: 串列搜尋中，有時想用 k 看上一篇文章 */
      {
	int pos = xo->pos - 1;

	/* itoc.000227: 避免看過頭 */
	if (pos < 0)
	  return xpost_head(xo);

	xo->pos = pos;

	if (pos <= xo->top)
	  xpost_pick(xo);
  
	continue;
      }

    case KEY_DOWN:
    case KEY_PGDN:
    case ']':	/* Thor.990204: 串列搜尋中，有時想用 ] 看下一篇文章 */
    case 'j':	/* Thor.990204: 串列搜尋中，有時想用 j 看下一篇文章 */
    case ' ':
      {
	int pos = xo->pos + 1;

	/* Thor.980727: 修正看過頭的bug */

	if (pos >= xo->max)
    	  return xpost_head(xo);

	xo->pos = pos;

	if (pos >= xo->top + XO_TALL)
  	  xpost_pick(xo);

	continue;
      }

    case 'y':
    case 'r':
      strcpy(quote_file, fpath);
      do_mreply(hdr, 1);
      break;

    case 'm': 
      if (!(hdr->xmode & POST_MARKED))
      {
	/* 在 xmbox_browse 時看不到 m 記號，所以限制只能 mark */
	hdr->xmode ^= POST_MARKED;
	currchrono = hdr->chrono;
	rec_put(dir, hdr, sizeof(HDR), hdr->xid, cmpchrono);
      } 
      break;

    case '/':
      if (vget(b_lines, 0, "搜尋：", hunt, sizeof(hunt), DOECHO))
      {
	key = more(fpath, FOOTER_MAILER);
	goto re_key;
      }
      continue;

    case 'E':
      return mbox_edit(xo);

    case 'C':	/* itoc.000515: xmbox_browse 時可存入暫存檔 */
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
      xo_help("mbox");
      break;
    }
    break;
  }

  return xpost_head(xo);
}


#ifdef HAVE_XYNEWS	/* itoc.010822: news 閱讀模式 */

#if 0	/* 構想 */

在大看板(連線看板)中常常有很多灌水文章，仿照 news 的閱讀方式，
把所有 reply 的文章都先隱藏，只顯示第一封發文。

第一輪利用 XoNews 來剔除 reply 文章
第二輪利用原有的 XoXsearch 搜尋同主題文章
如此就可以達到新聞閱讀模式的效果

#endif


static int *newsIndex;

extern KeyFunc news_cb[];


int
news_head(xo)
  XO *xo;
{
  vs_head("新聞閱\讀", xo->xyz);
  prints(NECKER_NEWS, d_cols, "");
  return XO_BODY;
}


static void
news_pick(xo)
  XO *xo;
{
  int *list, fsize, pos, max, top;
  HDR *fimage, *hdr;

  fimage = (HDR *) f_map(xo->dir, &fsize);
  if (fimage == (HDR *) - 1)
    return;

  hdr = (HDR *) xo_pool;
  list = newsIndex;

  pos = xo->pos;
  xo->top = top = (pos / XO_TALL) * XO_TALL;
  max = xo->max;
  pos = top + XO_TALL;
  if (max > pos)
    max = pos;

  do
  {
    pos = list[top++];
    memcpy(hdr, fimage + pos, sizeof(HDR));
    /* hdr->xid = pos; */	/* 在 XZ_NEWS 沒用到 xid，可以考慮保留給 reply 篇數 */
    hdr++;
  } while (top < max);

  munmap(fimage, fsize);
}


int
news_init(xo)
  XO *xo;
{
  /* load into pool */
  news_pick(xo);
  return news_head(xo);
}


int
news_load(xo)
  XO *xo;
{
  /* load into pool */
  news_pick(xo);
  return XO_BODY;
}


int
XoNews(xo)			/* itoc: News reader : call from post_cb */
  XO *xo;
{
  int returnPos;
  int *list, fsize, max, count, i;
  char *fimage;
  HDR *head;
  XO *xt;

  if (xo->max <= 0)		/* Thor.980911.註解: 以防萬一 */
    return XO_FOOT;

#ifdef EVERY_Z		/* itoc.060206: 只有用 ^Z 才可能從不同看板進入新聞模式 */
  if (xz[XZ_NEWS - XO_ZONE].xo)		/* itoc.020308: 不得累積進入二次 */
  {
    vmsg(MSG_XYDENY);
    return XO_FOOT;
  }
#endif

  /* build index according to input condition */

  fimage = f_map(xo->dir, &fsize);

  if (fimage == (char *) -1)
  {
    vmsg("目前無法開啟索引檔");
    return XO_FOOT;
  }

  /* allocate index memory, remember free first */

  /* Thor.990113: 怕問title, author的瞬間又有人post */
  max = fsize / sizeof(HDR);
  list = (int *) malloc(sizeof(int) * max);

  count = 0;

  for (i = 0; i < max; i++)
  {
    head = (HDR *) fimage + i;

#ifdef HAVE_REFUSEMARK
    if (!chkrestrict(head))
      continue;
#endif

    /* check condition */
    if (STR4(head->title) == STR4(STR_REPLY))	/* reply 的文章不要 */
      continue;

    list[count++] = i;
  }

  munmap(fimage, fsize);

  if (count <= 0)
  {
    free(list);
    vmsg(MSG_XY_NONE);
    return XO_FOOT;
  }

  newsIndex = list;

  /* build XO for news_xo */

  returnPos = xo->pos;		/* Thor: record pos, future use */
  xz[XZ_NEWS - XO_ZONE].xo = xt = xo_new(xo->dir);
  xz[XZ_NEWS - XO_ZONE].cb = news_cb;
  xt->pos = 0;
  xt->max = count;
  xt->xyz = xo->xyz;
  xt->key = XZ_NEWS;

  xover(XZ_NEWS);

  /* set xo->pos for new location */

  xo->pos = returnPos;		/* 從 XZ_NEWS 回到 XZ_POST 游標移去原來的地方 */

  /* free news_xo */

  if (xt = xz[XZ_NEWS - XO_ZONE].xo)
  {
    free(xt);
    xz[XZ_NEWS - XO_ZONE].xo = NULL;
  }

  /* free index memory, remember check free pointer */

  if (list)
    free(list);

  return XO_INIT;
}
#endif	/* HAVE_XYNEWS */
