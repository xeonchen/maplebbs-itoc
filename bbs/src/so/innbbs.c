/*-------------------------------------------------------*/
/* innbbs.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 轉信設定					 */
/* create : 04/04/25					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"


extern BCACHE *bshm;


/* ----------------------------------------------------- */
/* nodelist.bbs 子函式					 */
/* ----------------------------------------------------- */


static void
nl_item(num, nl)
  int num;
  nodelist_t *nl;
{
  prints("%6d %-13s%-*.*s %s(%d)\n", num, 
    nl->name, d_cols + 45, d_cols + 45, nl->host, nl->xmode & INN_USEIHAVE ? "IHAVE" : "POST", nl->port);
}


static void
nl_query(nl)
  nodelist_t *nl;
{
  move(3, 0);
  clrtobot();
  prints("\n\n轉信站台：%s\n站台位址：%s\n站台協定：%s(%d)\n被 餵 信：%s", 
    nl->name, nl->host, nl->xmode & INN_USEIHAVE ? "IHAVE" : "POST", nl->port, nl->xmode & INN_FEEDED ? "是" : "否");
  vmsg(NULL);
}


static int	/* 1:成功 0:失敗 */
nl_add(fpath, old, pos)
  char *fpath;
  nodelist_t *old;
  int pos;
{
  nodelist_t nl;
  int ch, port;
  char ans[8];
  char msg1[] = "協定：(1)IHAVE (2)POST [1] ";
  char msg2[] = "此站台會主動餵信給本站嗎(Y/N)？[N] ";

  if (old)
    memcpy(&nl, old, sizeof(nodelist_t));
  else
    memset(&nl, 0, sizeof(nodelist_t));

  if (vget(b_lines, 0, "英文站名：", nl.name, sizeof(nl.name), GCARRY) &&
    vget(b_lines, 0, "站址：", nl.host, /* sizeof(nl.host) */ 70, GCARRY))
  {
    msg1[24] = (nl.xmode & INN_USEPOST) ? '2' : '1';	/* 新增資料預設 INN_HAVE */
    ch = vans(msg1);
    if (ch != '1' && ch != '2')
      ch = msg1[24];

    if (ch == '1')
    {
      nl.xmode = INN_USEIHAVE | INN_FEEDED;	/* IHAVE 一定是被餵信 */
      vget(b_lines, 0, "Port：[7777] ", ans, 6, DOECHO);
      if ((port = atoi(ans)) <= 0)
	port = 7777;
    }
    else /* if (ch == '2') */
    {
      nl.xmode = INN_USEPOST;
      vget(b_lines, 0, "Port：[119] ", ans, 6, DOECHO);
      if ((port = atoi(ans)) <= 0)
	port = 119;

      msg2[32] = (old && old->xmode & INN_FEEDED) ? 'Y' : 'N';	/* 新增資料預設不餵信 */
      ch = vans(msg2);
      if (ch != 'y' && ch != 'n')
	ch = msg2[32] | 0x20;

      if (ch == 'y')
	nl.xmode |= INN_FEEDED;
    }
    nl.port = port;

    if (old)
      rec_put(fpath, &nl, sizeof(nodelist_t), pos, NULL);
    else
      rec_add(fpath, &nl, sizeof(nodelist_t));
    return 1;
  }
  return 0;
}


static int
nl_cmp(a, b)
  nodelist_t *a, *b;
{
  /* 依 name 排序 */
  return str_cmp(a->name, b->name);
}


static int
nl_search(nl, key)
  nodelist_t *nl;
  char *key;
{
  return (int) (str_str(nl->name, key) || str_str(nl->host, key));
}


/* ----------------------------------------------------- */
/* newsfeeds.bbs 子函式					 */
/* ----------------------------------------------------- */


static void
nf_item(num, nf)
  int num;
  newsfeeds_t *nf;
{
  int bno;
  BRD *brd;
  char outgo, income;

  if ((bno = brd_bno(nf->board)) >= 0)
  {
    if (nf->xmode & INN_ERROR)
    {
      outgo = income = '?';
    }
    else
    {
      brd = bshm->bcache + bno;
      outgo = brd->battr & BRD_NOTRAN ? ' ' : '<';
      income = nf->xmode & INN_NOINCOME ? ' ': '>';
    }
  }
  else
  {
    outgo = income = 'X';
  }

  prints("%6d %-13s%-*.*s %c-%c %-13s %.7s\n", num, 
    nf->path, d_cols + 32, d_cols + 32, nf->newsgroup, outgo, income, nf->board, nf->charset);
}


static void
nf_query(nf)
  newsfeeds_t *nf;
{
  nodelist_t nl;
  int fd;
  int rc = 0;
  BRD *brd;
  char *outgo, *income;

  /* 找出該站台在 nodelist.bbs 中的資訊 */
  if ((fd = open("innd/nodelist.bbs", O_RDONLY)) >= 0)
  {
    while (read(fd, &nl, sizeof(nodelist_t)) == sizeof(nodelist_t))
    {
      if (!strcmp(nl.name, nf->path))
      {
	rc = 1;
	break;
      }
    }
    close(fd);
  }
  if (!rc)
  {
    memset(&nl, 0, sizeof(nodelist_t));
    strcpy(nl.host, "\033[1;33m此站台不在 nodelist.bbs 中\033[m");
  }

  /* 看板狀態 */
  if ((rc = brd_bno(nf->board)) >= 0)
  {
    brd = bshm->bcache + rc;
    outgo = brd->battr & BRD_NOTRAN ? "\033[1;33m不轉出\033[m"  : "轉出";
    income = nf->xmode & INN_NOINCOME ? "且\033[1;33m不轉進\033[m" : "且轉進";
  }
  else
  {
    outgo = "\033[1;33m此看板不存在\033[m";
    income = "";
  }

  move(3, 0);
  clrtobot();
  prints("\n\n轉信站台：%s\n站台位址：%s\n站台協定：%s(%d)\n"
    "轉信群組：%s%s\n本站看板：%s (%s%s)\n使用字集：%s", 
    nf->path, nl.host, nl.xmode & INN_USEIHAVE ? "IHAVE" : "POST", nl.port, 
    nf->newsgroup, nf->xmode & INN_ERROR ? " (\033[1;33m此群組不存在\033[m)" : "", 
    nf->board, outgo, income, nf->charset);
  if (rc && !(nl.xmode & INN_FEEDED))
    prints("\n目前篇數：%d", nf->high);
  vmsg(NULL);
}


static int	/* 1:成功 0:失敗 */
nf_add(fpath, old, pos)
  char *fpath;
  newsfeeds_t *old;
  int pos;
{
  newsfeeds_t nf;
  int high;
  char ans[12];
  BRD *brd;

  if (old)
    memcpy(&nf, old, sizeof(newsfeeds_t));
  else
  {
    memset(&nf, 0, sizeof(newsfeeds_t));
    nf.high = INT_MAX;		/* 第一次取信強迫 reload */
  }

  if ((brd = ask_board(nf.board, BRD_L_BIT, NULL)) &&
    vget(b_lines, 0, "英文站名：", nf.path, sizeof(nf.path), GCARRY) &&
    vget(b_lines, 0, "群組：", nf.newsgroup, /* sizeof(nf.newsgroup) */ 70, GCARRY))
  {
    if (!vget(b_lines, 0, "字集 [" MYCHARSET "]：", nf.charset, sizeof(nf.charset), GCARRY))
      str_ncpy(nf.charset, MYCHARSET, sizeof(nf.charset));
    nf.xmode = (vans("是否轉進(Y/N)？[Y] ") == 'n') ? INN_NOINCOME : 0;

    if (vans("是否更改轉信的 high-number 設定，這設定對被餵信的群組無效(Y/N)？[N] ") == 'y')
    {
      sprintf(ans, "%d", nf.high);
      vget(b_lines, 0, "目前篇數：", ans, 11, GCARRY);
      if ((high = atoi(ans)) >= 0)
	nf.high = high;
    }

    if (old)
      rec_put(fpath, &nf, sizeof(newsfeeds_t), pos, NULL);
    else
      rec_add(fpath, &nf, sizeof(newsfeeds_t));

    if ((brd->battr & BRD_NOTRAN) && vans("本板屬性目前為不轉出，是否改為轉出(Y/N)？[Y] ") != 'n')
    {
      high = brd - bshm->bcache;
      brd->battr &= ~BRD_NOTRAN;
      rec_put(FN_BRD, brd, sizeof(BRD), high, NULL);
    }

    return 1;
  }
  return 0;
}


static int
nf_cmp(a, b)
  newsfeeds_t *a, *b;
{
  /* path/newsgroup 交叉比對 */
  int k = str_cmp(a->path, b->path);
  return k ? k : str_cmp(a->newsgroup, b->newsgroup);
}


static int
nf_search(nf, key)
  newsfeeds_t *nf;
  char *key;
{
  return (int) (str_str(nf->newsgroup, key) || str_str(nf->board, key));
}


/* ----------------------------------------------------- */
/* ncmperm.bbs 子函式					 */
/* ----------------------------------------------------- */


static void
ncm_item(num, ncm)
  int num;
  ncmperm_t *ncm;
{
  prints("%6d %-*.*s%-23.23s %s\n", num, 
    d_cols + 44, d_cols + 44, ncm->issuer, ncm->type, ncm->perm ? "○" : "╳");
}


static void
ncm_query(ncm)
  ncmperm_t *ncm;
{
  move(3, 0);
  clrtobot();
  prints("\n\n發行站台：%s\n砍信種類：%s\n允許\砍信：%s", 
    ncm->issuer, ncm->type, ncm->perm ? "○" : "╳");
  vmsg(NULL);
}


static int	/* 1:成功 0:失敗 */
ncm_add(fpath, old, pos)
  char *fpath;
  ncmperm_t *old;
  int pos; 
{
  ncmperm_t ncm;

  if (old)
    memcpy(&ncm, old, sizeof(ncmperm_t));
  else
    memset(&ncm, 0, sizeof(ncmperm_t));

  if (vget(b_lines, 0, "發行：", ncm.issuer, /* sizeof(ncm.issuer) */ 70, GCARRY) &&
    vget(b_lines, 0, "種類：", ncm.type, sizeof(ncm.type), GCARRY))
  {
    ncm.perm = (vans("允許\此 NCM message 砍信(Y/N)？[N] ") == 'y');

    if (old)
      rec_put(fpath, &ncm, sizeof(ncmperm_t), pos, NULL);
    else
      rec_add(fpath, &ncm, sizeof(ncmperm_t));
    return 1;
  }
  return 0;
}


static int
ncm_cmp(a, b)
  ncmperm_t *a, *b;
{
  /* issuer/type 交叉比對 */
  int k = str_cmp(a->issuer, b->issuer);
  return k ? k : str_cmp(a->type, b->type);
}


static int
ncm_search(ncm, key)
  ncmperm_t *ncm;
  char *key;
{
  return (int) (str_str(ncm->issuer, key) || str_str(ncm->type, key));
}


/* ----------------------------------------------------- */
/* spamrule.bbs 子函式					 */
/* ----------------------------------------------------- */


static char *
spam_compare(xmode)
  int xmode;
{
  if (xmode & INN_SPAMADDR)
    return "作者";
  if (xmode & INN_SPAMNICK)
    return "暱稱";
  if (xmode & INN_SPAMSUBJECT)
    return "標題";
  if (xmode & INN_SPAMPATH)
    return "路徑";
  if (xmode & INN_SPAMMSGID)
    return "MSID";
  if (xmode & INN_SPAMBODY)
    return "本文";
  if (xmode & INN_SPAMSITE)
    return "組織";
  if (xmode & INN_SPAMPOSTHOST)
    return "來源";
  return "？？";
}


static void
spam_item(num, spam)
  int num;
  spamrule_t *spam;
{
  char *path, *board;

  path = spam->path;
  board = spam->board;
  prints("%6d %-13s%-13s[%s] 包含 %.*s\n", 
    num, *path ? path : "所有站台", *board ? board : "所有看板", 
    spam_compare(spam->xmode), d_cols + 30, spam->detail);
}


static void
spam_query(spam)
  spamrule_t *spam;
{
  char *path, *board;

  path = spam->path;
  board = spam->board;

  move(3, 0);
  clrtobot();
  prints("\n\n適用站台：%s\n適用看板：%s\n比較項目：%s\n比較內容：%s", 
    *path ? path : "所有站台", *board ? board : "所有看板", spam_compare(spam->xmode), spam->detail);
  vmsg("若滿足此規則，會被視為廣告而無法轉信進來");
}


static int	/* 1:成功 0:失敗 */
spam_add(fpath, old, pos)
  char *fpath;
  spamrule_t *old;
  int pos; 
{
  spamrule_t spam;

  if (old)
    memcpy(&spam, old, sizeof(spamrule_t));
  else
    memset(&spam, 0, sizeof(spamrule_t));

  vget(b_lines, 0, "英文站名：", spam.path, sizeof(spam.path), GCARRY);
  ask_board(spam.board, BRD_L_BIT, NULL);

  switch (vans("擋信規則 1)作者 2)暱稱 3)標題 4)路徑 5)MSGID 6)本文 7)組織 8)來源 [Q] "))
  {
  case '1':
    spam.xmode = INN_SPAMADDR;
    break;
  case '2':
    spam.xmode = INN_SPAMNICK;
    break;
  case '3':
    spam.xmode = INN_SPAMSUBJECT;
    break;
  case '4':
    spam.xmode = INN_SPAMPATH;
    break;
  case '5':
    spam.xmode = INN_SPAMMSGID;
    break;
  case '6':
    spam.xmode = INN_SPAMBODY;
    break;
  case '7':
    spam.xmode = INN_SPAMSITE;
    break;
  case '8':
    spam.xmode = INN_SPAMPOSTHOST;
    break;
  default:
    return 0;
  }

  if (vget(b_lines, 0, "包含：", spam.detail, /* sizeof(spam.detail) */ 70, GCARRY))
  {
    if (old)
      rec_put(fpath, &spam, sizeof(spamrule_t), pos, NULL);
    else
      rec_add(fpath, &spam, sizeof(spamrule_t));
    return 1;
  }
  return 0;
}


static int
spam_cmp(a, b)
  spamrule_t *a, *b;
{
  /* path/board/xmode/detail 交叉比對 */
  int i = strcmp(a->path, b->path);
  int j = strcmp(a->board, b->board);
  int k = a->xmode - b->xmode;
  return i ? i : j ? j : k ? k : str_cmp(a->detail, b->detail);
}


static int
spam_search(spam, key)
  spamrule_t *spam;
  char *key;
{
  return (int) (str_str(spam->detail, key));
}


/* ----------------------------------------------------- */
/* 轉信設定主函式					 */
/* ----------------------------------------------------- */


int
a_innbbs()
{
  int num, pageno, pagemax, redraw, reload;
  int ch, cur, i, dirty;
  struct stat st;
  char *data;
  int recsiz;
  char *fpath;
  char buf[40];
  void (*item_func)(), (*query_func)();
  int (*add_func)(), (*sync_func)(), (*search_func)();

  vs_bar("轉信設定");
  more("etc/innbbs.hlp", (char *) -1);

  switch (vans("請選擇 1)轉文站台列表 2)轉文看板列表 3)NoCeM擋文規則 4)廣告文名單：[Q] "))
  {
  case '1':
    fpath = "innd/nodelist.bbs";
    recsiz = sizeof(nodelist_t);
    item_func = nl_item;
    query_func = nl_query;
    add_func = nl_add;
    sync_func = nl_cmp;
    search_func = nl_search;
    break;

  case '2':
    fpath = "innd/newsfeeds.bbs";
    recsiz = sizeof(newsfeeds_t);
    item_func = nf_item;
    query_func = nf_query;
    add_func = nf_add;
    sync_func = nf_cmp;
    search_func = nf_search;
    break;

  case '3':
    fpath = "innd/ncmperm.bbs";
    recsiz = sizeof(ncmperm_t);
    item_func = ncm_item;
    query_func = ncm_query;
    add_func = ncm_add;
    sync_func = ncm_cmp;
    search_func = ncm_search;
    break;

  case '4':
    fpath = "innd/spamrule.bbs";
    recsiz = sizeof(spamrule_t);
    item_func = spam_item;
    query_func = spam_query;
    add_func = spam_add;
    sync_func = spam_cmp;
    search_func = spam_search;
    break;

  default:
    return 0;
  }

  dirty = 0;	/* 1:有新增/刪除資料 */
  reload = 1;
  pageno = 0;
  cur = 0;
  data = NULL;

  do
  {
    if (reload)
    {
      if (stat(fpath, &st) == -1)
      {
	if (!add_func(fpath, NULL, -1))
	  return 0;
	dirty = 1;
	continue;
      }

      i = st.st_size;
      num = (i / recsiz) - 1;
      if (num < 0)
      {
	if (!add_func(fpath, NULL, -1))
	  return 0;
	dirty = 1;
	continue;
      }

      if ((ch = open(fpath, O_RDONLY)) >= 0)
      {
	data = data ? (char *) realloc(data, i) : (char *) malloc(i);
	read(ch, data, i);
	close(ch);
      }

      pagemax = num / XO_TALL;
      reload = 0;
      redraw = 1;
    }

    if (redraw)
    {
      /* itoc.註解: 盡量做得像 xover 格式 */
      vs_head("轉信設定", str_site);
      prints(NECKER_INNBBS, d_cols, "");

      i = pageno * XO_TALL;
      ch = BMIN(num, i + XO_TALL - 1);
      move(3, 0);
      do
      {
	item_func(i + 1, data + i * recsiz);
	i++;
      } while (i <= ch);

      outf(FEETER_INNBBS);
      move(3 + cur, 0);
      outc('>');
      redraw = 0;
    }

    ch = vkey();
    switch (ch)
    {
    case KEY_RIGHT:
    case '\n':
    case ' ':
    case 'r':
      i = cur + pageno * XO_TALL;
      query_func(data + i * recsiz);
      redraw = 1;
      break;

    case Ctrl('P'):
      if (add_func(fpath, NULL, -1))
      {
	dirty = 1;
	num++;
	cur = num % XO_TALL;		/* 游標放在新加入的這篇 */
	pageno = num / XO_TALL;
	reload = 1;
      }
      redraw = 1;
      break;

    case 'd':
      if (vans(msg_del_ny) == 'y')
      {
	dirty = 1;
	i = cur + pageno * XO_TALL;
	rec_del(fpath, recsiz, i, NULL);
	cur = i ? ((i - 1) % XO_TALL) : 0;	/* 游標放在砍掉的前一篇 */
	reload = 1;
      }
      redraw = 1;
      break;

    case 'E':
      i = cur + pageno * XO_TALL;
      if (add_func(fpath, data + i * recsiz, i))
      {
	dirty = 1;
	reload = 1;
      }
      redraw = 1;
      break;

    case '/':
      if (vget(b_lines, 0, "關鍵字：", buf, sizeof(buf), DOECHO))
      {
	str_lower(buf, buf);
	for (i = pageno * XO_TALL + cur + 1; i <= num; i++)	/* 從游標下一個開始找 */
	{
	  if (search_func(data + i * recsiz, buf))
	  {
	    pageno = i / XO_TALL;
	    cur = i % XO_TALL;
	    break;
	  }
	}
      }
      redraw = 1;
      break;

    default:
      ch = xo_cursor(ch, pagemax, num, &pageno, &cur, &redraw);
      break;
    }
  } while (ch != 'q');

  free(data);

  if (dirty)
    rec_sync(fpath, recsiz, sync_func, NULL);
  return 0;
}
