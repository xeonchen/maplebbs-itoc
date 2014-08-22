/*-------------------------------------------------------*/
/* bhttpd.c		( NTHU CS MapleBBS Ver 3.10 )	 */
/*-------------------------------------------------------*/
/* target : BBS's HTTP daemon				 */
/* create : 05/07/11					 */
/* update : 05/08/04					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/* author : yiting.bbs@bbscs.tku.edu.tw			 */
/*-------------------------------------------------------*/


#if 0	/* 連結一覽表 */

  http://my.domain/                            首頁
  http://my.domain/brdlist                     看板列表
  http://my.domain/fvrlist                     我的最愛
  http://my.domain/usrlist                     使用者名單
  http://my.domain/brd?brdname&##              文章列表，列出看板 [brdname] 編號 ## 開始的 50 篇文章
  http://my.domain/gem?brdname&folder          精華區列表，列出看板 [brdname] 精華區中 folder 這個卷宗下的所有東西
  http://my.domain/mbox?##                     信箱列表，列出信箱中編號 ## 開始的 50 篇文章
  http://my.domain/bmore?brdname&##            閱讀看板文章，閱讀看板 [brdname] 的第 ## 篇文章
  http://my.domain/bmost?brdname&##            閱讀看板文章，閱讀看板 [brdname] 中所有名第 ## 篇同標題的文章
  http://my.domain/gmore?brdname&folder&##     閱讀精華區文章，閱讀看板 [brdname] 精華區中 folder 這個卷宗下的第 ## 篇文章
  http://my.domain/mmore?##                    閱讀信箱文章，閱讀信箱中第 ## 篇文章
  http://my.domain/dopost?brdname              發表文章於看板 [brdname]
  http://my.domain/domail?userid               發送信件給 [userid]
  http://my.domain/dpost?brdname&##&###        詢問確定刪除看板 [brdname] 中第 ## 篇文章 (其 chrono 是 ###)
  http://my.domain/delpost?brdname&##&###      刪除看板 [brdname] 中第 ## 篇文章 (其 chrono 是 ###)
  http://my.domain/mpost?brdname&##&###        標記看板 [brdname] 中第 ## 篇文章 (其 chrono 是 ###)
  http://my.domain/dmail?##&###                詢問確定刪除信箱中第 ## 篇文章 (其 chrono 是 ###)
  http://my.domain/delmail?##&###              刪除信箱中第 ## 篇文章 (其 chrono 是 ###)
  http://my.domain/mmail?##&###                標記信箱中第 ## 篇文章 (其 chrono 是 ###)
  http://my.domain/query?userid                查詢 userid
  http://my.domain/img?filename                顯示圖檔
  http://my.domain/rss?brdname                 各看板的RSS Feed
  http://my.domain/class?folder                列出分類中 [folder] 這個卷宗下的所有看板
  http://my.domain/robots.txt                  Robot Exclusion

#endif


#define _MODES_C_

#include "bbs.h"


#include <sys/wait.h>
#include <netinet/tcp.h>
#include <sys/resource.h>

#undef	ROBOT_EXCLUSION		/* robot exclusion */


#define SERVER_USAGE
#undef	LOG_VERBOSE		/* 詳細紀錄 */


#define BHTTP_PIDFILE	"run/bhttp.pid"
#define BHTTP_LOGFILE	"run/bhttp.log"


#define BHTTP_PERIOD	(60 * 5)	/* 每 5 分鐘 check 一次 */
#define BHTTP_TIMEOUT	(60 * 3)	/* 超過 3 分鐘的連線就視為錯誤 */
#define BHTTP_FRESH	86400		/* 每 1 天整理一次 log 檔 */


#define TCP_BACKLOG	3
#define TCP_RCVSIZ	2048


#define MIN_DATA_SIZE	(TCP_RCVSIZ + 3)
#define MAX_DATA_SIZE   262143		/* POST 的大小限制(byte) */


/* Thor.000425: POSIX 用 O_NONBLOCK */

#ifndef O_NONBLOCK
#define M_NONBLOCK  FNDELAY
#else
#define M_NONBLOCK  O_NONBLOCK
#endif

#define HTML_TALL	50	/* 列表一頁 50 篇 */


/* ----------------------------------------------------- */
/* 選單的顏色						 */
/* ----------------------------------------------------- */

#define HCOLOR_BG	"#000000"	/* 背景的顏色 */
#define HCOLOR_TEXT	"#ffffff"	/* 文字的顏色 */
#define HCOLOR_LINK	"#00ffff"	/* 未瀏覽過連結的顏色 */
#define HCOLOR_VLINK	"#c0c0c0"	/* 已瀏覽過連結的顏色 */
#define HCOLOR_ALINK	"#ff0000"	/* 連結被壓下時的顏色 */

#define HCOLOR_NECK	"#000070"	/* 脖子的顏色 */
#define HCOLOR_TIE	"#a000a0"	/* 領帶的顏色 */

#define HCOLOR_BAR	"#808080"	/* 光棒顏色 */


/* ----------------------------------------------------- */
/* HTTP commands					 */
/* ----------------------------------------------------- */

typedef struct
{
  int (*func) ();
  char *cmd;
  int len;			/* strlen(Command.cmd) */
}      Command;


/* ----------------------------------------------------- */
/* client connection structure				 */
/* ----------------------------------------------------- */

#define LEN_COOKIE	(IDLEN + PASSLEN + 3 + 1)	/* userid&p=passwd */

typedef struct Agent
{
  struct Agent *anext;
  int sock;

  unsigned int ip_addr;

  time_t tbegin;		/* 連線開始時間 */
  time_t uptime;		/* 上次下指令的時間 */

  char url[48];			/* 欲瀏覽的網頁 */
  char *urlp;

  char cookie[32];
  int setcookie;

  char modified[30];

  /* 使用者資料要先 acct_fetch() 才能使用 */
  int userno;
  char userid[IDLEN + 1];
  char username[UNLEN + 1];
  usint userlevel;

  /* 所能看到的看板列表或使用者名單 */

#if MAXBOARD > MAXACTIVE
  void *myitem[MAXBOARD];
#else
  void *myitem[MAXACTIVE];
#endif
  int total_item;

  /* input 用 */

  char *data;
  int size;			/* 目前 data 所 malloc 的空間大小 */
  int used;

  /* output 用 */

  FILE *fpw;
}     Agent;


/* ----------------------------------------------------- */
/* http state code					 */
/* ----------------------------------------------------- */

enum
{
  HS_END,

  HS_ERROR,			/* 語法錯誤 */
  HS_ERR_LOGIN,			/* 尚未登入 */
  HS_ERR_USER,			/* 帳號讀取錯誤 */
  HS_ERR_MORE,			/* 文章讀取錯誤 */
  HS_ERR_BOARD,			/* 看板讀取錯誤 */
  HS_ERR_MAIL,			/* 信件讀取錯誤 */
  HS_ERR_CLASS,			/* 分類讀取錯誤 */
  HS_ERR_PERM,			/* 權限不足 */

  HS_OK,

  HS_REDIRECT,			/* 重新導向 */
  HS_NOTMOIDIFY,		/* 檔案沒有變更 */
  HS_BADREQUEST,		/* 錯誤的要求 */
  HS_FORBIDDEN,			/* 未授權的頁面 */
  HS_NOTFOUND,			/* 找不到檔案 */

  LAST_HS
};


static char *http_msg[LAST_HS] =
{
  NULL,

  "語法錯誤",
  "您尚未登入",
  "沒有這個帳號",
  "操作錯誤：您所選取的文章不存在或已刪除",
  "操作錯誤：無此看板或您的權限不足",
  "操作錯誤：無此信件或您尚未登入",
  "操作錯誤：您所選取的分類不存在或已刪除",
  "操作錯誤：您尚未登入或權限不足，無法進行這項操作",

  "200 OK",

  "302 Found",
  "304 Not Modified",
  "400 Bad Request",
  "403 Forbidden",
  "404 Not Found",
};


#define HS_REFRESH	0x0100	/* 自動跳頁(預設是3秒) */


/* ----------------------------------------------------- */
/* AM : Agent Mode					 */
/* ----------------------------------------------------- */

#define AM_GET		0x010
#define AM_POST		0x020


/* ----------------------------------------------------- */
/* operation log and debug information			 */
/* ----------------------------------------------------- */
/* @START | ... | time					 */
/* ----------------------------------------------------- */

static FILE *flog;

extern int errno;
extern char *crypt();

static void
log_fresh()
{
  int count;
  char fsrc[64], fdst[64];
  char *fpath = BHTTP_LOGFILE;

  if (flog)
    fclose(flog);

  count = 9;
  do
  {
    sprintf(fdst, "%s.%d", fpath, count);
    sprintf(fsrc, "%s.%d", fpath, --count);
    rename(fsrc, fdst);
  } while (count);

  rename(fpath, fsrc);
  flog = fopen(fpath, "a");
}


static void
logit(key, msg)
  char *key;
  char *msg;
{
  time_t now;
  struct tm *p;

  time(&now);
  p = localtime(&now);
  /* Thor.990329: y2k */
  fprintf(flog, "%s\t%s\t%02d/%02d/%02d %02d:%02d:%02d\n",
    key, msg, p->tm_year % 100, p->tm_mon + 1, p->tm_mday,
    p->tm_hour, p->tm_min, p->tm_sec);
}


static void
log_open()
{
  FILE *fp;

  umask(077);

  if (fp = fopen(BHTTP_PIDFILE, "w"))
  {
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
  }

  flog = fopen(BHTTP_LOGFILE, "a");
  logit("START", "MTA daemon");
}


/* ----------------------------------------------------- */
/* target : ANSI text to HTML tag			 */
/* author : yiting.bbs@bbs.cs.tku.edu.tw		 */
/* ----------------------------------------------------- */

#define ANSI_TAG	27
#define is_ansi(ch)	((ch >= '0' && ch <= '9') || ch == ';' || ch == '[')

#define	HAVE_HYPERLINK		/* 處理超連結 */
#undef	HAVE_ANSIATTR		/* 很少用到而且IE不支援閃爍，乾脆不處理 :( */
#define HAVE_SAKURA		/* 櫻花日文自動轉Unicode */

#ifdef HAVE_ANSIATTR
#define ATTR_UNDER	0x1	/* 底線 */
#define ATTR_BLINK	0x2	/* 閃動 */
#define ATTR_ITALIC	0x4	/* 斜體 */

static int old_attr, now_attr;
#endif

static int old_color, now_color;
static char ansi_buf[1024];	/* ANSILINELEN * 4 */


#ifdef HAVE_HYPERLINK
static uschar *linkEnd = NULL;

static void
ansi_hyperlink(fpw, src)
  FILE *fpw;
  uschar *src;
{
  int ch;

  linkEnd = src;
  fputs("<a class=PRE target=_blank href=", fpw);
  while (ch = *linkEnd)
  {
    if (ch < '#' || ch == '<' || ch == '>' || ch > '~')
      break;
    fputc(ch, fpw);
    linkEnd++;
  }
  fputc('>', fpw);
}
#endif


#ifdef HAVE_SAKURA
static int
sakura2unicode(code)
  int code;
{
  if (code > 0xC6DD && code < 0xC7F3)
  {
    if (code > 0xC7A0)
      code -= 38665;
    else if (code > 0xC700)
      code -= 38631;
    else if (code > 0xC6E6)
      code -= 38566;
    else if (code == 0xC6E3)
      return 0x30FC;
    else
      code -= 38619;
    if (code > 0x3093)
      code += 13;
    return code;
  }
  return 0;
}
#endif


static int
ansi_remove(psrc)
  uschar **psrc;
{
  uschar *src = *psrc;
  int ch = *src;

  while (is_ansi(ch))
    ch = *(++src);

  if (ch && ch != '\n')
    ch = *(++src);

  *psrc = src;
  return ch;
}


static int
ansi_color(psrc)
  uschar **psrc;
{
  uschar *src, *ptr;
  int ch, value;
  int color = old_color;
#ifdef HAVE_ANSIATTR
  int attr = old_attr;
#endif
  uschar *cptr = (uschar *) & color;

  src = ptr = (*psrc) + 1;

  ch = *src;
  while (ch)
  {
    if (ch == ';' || ch == 'm')
    {
      *src = '\0';
      value = atoi(ptr);
      ptr = src + 1;
      if (value == 0)
      {
	color = 0x00003740;

#ifdef HAVE_ANSIATTR
	attr = 0;
#endif
      }
      else if (value >= 30 && value <= 37)
	cptr[1] = value + 18;
      else if (value >= 40 && value <= 47)
	cptr[0] = value + 24;
      else if (value == 1)
	cptr[2] = 1;

#ifdef HAVE_ANSIATTR
      else if (value == 4)
	attr |= ATTR_UNDER;
      else if (value == 5)
	attr |= ATTR_BLINK;
      else if (value == 7)	/* 反白的效果用斜體來代替 */
	attr |= ATTR_ITALIC;
#endif

      if (ch == 'm')
      {
	now_color = color;

#ifdef HAVE_ANSIATTR
	now_attr = attr;
#endif

	ch = *(++src);
	break;
      }
    }
    else if (ch < '0' || ch > '9')
    {
      ch = *(++src);
      break;
    }
    ch = *(++src);
  }

  *psrc = src;
  return ch;
}


static void
ansi_tag(fpw)
  FILE *fpw;
{
#ifdef HAVE_ANSIATTR
  /* 屬性不同才需要印出 */
  if (!(now_attr & ATTR_ITALIC) && (old_attr & ATTR_ITALIC))
  {
    fputs("</I>", fpw);
  }
  if (!(now_attr & ATTR_UNDER) && (old_attr & ATTR_UNDER))
  {
    fputs("</U>", fpw);
  }
  if (!(now_attr & ATTR_BLINK) && (old_attr & ATTR_BLINK))
  {
    fputs("</BLINK>", fpw);
  }
#endif

  /* 顏色不同才需要印出 */
  if (old_color != now_color)
  {
    fprintf(fpw, "</font><font class=A%05X>", now_color);
    old_color = now_color;
  }

#ifdef HAVE_ANSIATTR
  /* 屬性不同才需要印出 */
  if (oldattr != attr)
  {
    if ((now_attr & ATTR_ITALIC) && !(old_attr & ATTR_ITALIC))
    {
      fputs("<I>", fpw);
    }
    if ((now_attr & ATTR_UNDER) && !(old_attr & ATTR_UNDER))
    {
      fputs("<U>", fpw);
    }
    if ((now_attr & ATTR_BLINK) && !(old_attr & ATTR_BLINK))
    {
      fputs("<BLINK>", fpw);
    }
    old_attr = now_attr;
  }
#endif
}


static void
ansi_html(fpw, src)
  FILE *fpw;
  uschar *src;
{
  int ch1, ch2;
  int has_ansi = 0;

#ifdef HAVE_SAKURA
  int scode;
#endif

  ch2 = *src;
  while (ch2)
  {
    ch1 = ch2;
    ch2 = *(++src);
    if (IS_ZHC_HI(ch1))
    {
      while (ch2 == ANSI_TAG)
      {
	if (*(++src) == '[')	/* 顏色 */
	{
	  ch2 = ansi_color(&src);
	  has_ansi = 1;
	}
	else			/* 其他直接刪除 */
	  ch2 = ansi_remove(&src);
      }
      if (ch2)
      {
	if (ch2 < ' ')		/* 怕出現\n */
	  fputc(ch2, fpw);
#ifdef HAVE_SAKURA
	else if (scode = sakura2unicode((ch1 << 8) | ch2))
	  fprintf(fpw, "&#%d;", scode);
#endif
	else
	{
	  fputc(ch1, fpw);
	  fputc(ch2, fpw);
	}
	ch2 = *(++src);
      }
      if (has_ansi)
      {
	has_ansi = 0;
	if (ch2 != ANSI_TAG)
	  ansi_tag(fpw);
      }
      continue;
    }
    else if (ch1 == ANSI_TAG)
    {
      do
      {
	if (ch2 == '[')		/* 顏色 */
	  ch2 = ansi_color(&src);
	else if (ch2 == '*')	/* 控制碼 */
	  fputc('*', fpw);
	else			/* 其他直接刪除 */
	  ch2 = ansi_remove(&src);
      } while (ch2 == ANSI_TAG && (ch2 = *(++src)));
      ansi_tag(fpw);
      continue;
    }
    /* 剩下的字元做html轉換 */
    if (ch1 == '<')
    {
      fputs("&lt;", fpw);
    }
    else if (ch1 == '>')
    {
      fputs("&gt;", fpw);
    }
    else if (ch1 == '&')
    {
      fputc(ch1, fpw);
      if (ch2 == '#')		/* Unicode字元不轉換 */
      {
	fputc(ch2, fpw);
	ch2 = *(++src);
      }
      else if (ch2 >= 'A' && ch2 <= 'z')
      {
	fputs("amp;", fpw);
	fputc(ch2, fpw);
	ch2 = *(++src);
      }
    }
#ifdef HAVE_HYPERLINK
    else if (linkEnd)		/* 處理超連結 */
    {
      fputc(ch1, fpw);
      if (linkEnd <= src)
      {
	fputs("</a>", fpw);
	linkEnd = NULL;
      }
    }
#endif
    else
    {
#ifdef HAVE_HYPERLINK
      /* 其他的自己加吧 :) */
      if (!str_ncmp(src - 1, "http://", 7))
	ansi_hyperlink(fpw, src - 1);
      else if (!str_ncmp(src - 1, "telnet://", 9))
	ansi_hyperlink(fpw, src - 1);
#endif

      fputc(ch1, fpw);
    }
  }
}


static char *
str_html(src, len)
  uschar *src;
  int len;
{
  int in_chi, ch;
  uschar *dst = ansi_buf, *end = src + len;

  ch = *src;
  while (ch && src < end)
  {
    if (IS_ZHC_HI(ch))
    {
      in_chi = *(++src);
      while (in_chi == ANSI_TAG)
      {
	src++;
	in_chi = ansi_remove(&src);
      }

      if (in_chi)
      {
	if (in_chi < ' ')	/* 可能只有半個字，前半部就不要了 */
	  *dst++ = in_chi;
#ifdef HAVE_SAKURA
	else if (len = sakura2unicode((ch << 8) + in_chi))
	{
	  sprintf(dst, "&#%d;", len);	/* 12291~12540 */
	  dst += 8;
	}
#endif
	else
	{
	  *dst++ = ch;
	  *dst++ = in_chi;
	}
      }
      else
	break;
    }
    else if (ch == ANSI_TAG)
    {
      src++;
      ch = ansi_remove(&src);
      continue;
    }
    else if (ch == '<')
    {
      strcpy(dst, "&lt;");
      dst += 4;
    }
    else if (ch == '>')
    {
      strcpy(dst, "&gt;");
      dst += 4;
    }
    else if (ch == '&')
    {
      ch = *(++src);
      if (ch == '#')
      {
	if ((uschar *) strchr(src + 1, ';') >= end)	/* 可能會不是或長度沒超過 */
	  break;
	*dst++ = '&';
	*dst++ = '#';
      }
      else
      {
	strcpy(dst, "&amp;");
	dst += 5;
	continue;
      }
    }
    else
      *dst++ = ch;
    ch = *(++src);
  }

  *dst = '\0';
  return ansi_buf;
}


static int
ansi_quote(fpw, src)		/* 如果是引言，就略過所有的 ANSI 碼 */
  FILE *fpw;
  uschar *src;
{
  int ch1, ch2;

  ch1 = src[0];
  ch2 = src[1];
  if (ch2 == ' ' && (ch1 == QUOTE_CHAR1 || ch1 == QUOTE_CHAR2))	/* 引言 */
  {
    ch2 = src[2];
    if (ch2 == QUOTE_CHAR1 || ch2 == QUOTE_CHAR2)	/* 引用一層/二層不同顏色 */
      now_color = 0x00003340;
    else
      now_color = 0x00003640;
  }
  else if (ch1 == '\241' && ch2 == '\260')	/* ※ 引言者 */
  {
    now_color = 0x00013640;
  }
  else
  {
    ansi_tag(fpw);
    return 0;			/* 不是引言 */
  }

  ansi_tag(fpw);
  fputs(str_html(src, ANSILINELEN), fpw);
  now_color = 0x00003740;
  return 1;
}


static void
txt2htm(fpw, fp)
  FILE *fpw;
  FILE *fp;
{
  static const char header1[LINE_HEADER][LEN_AUTHOR1] = {"作者", "標題", "時間"};
  static const char header2[LINE_HEADER][LEN_AUTHOR2] = {"發信人", "標  題", "發信站"};
  int i;
  char *headvalue, *pbrd, *board;
  char buf[ANSILINELEN];

  fputs("<table width=760 cellspacing=0 cellpadding=0 border=0>\n", fpw);
  /* 處理檔頭 */
  for (i = 0; i < LINE_HEADER; i++)
  {
    if (!fgets(buf, ANSILINELEN, fp))	/* 雖然連檔頭都還沒印完，但是檔案已經結束，直接離開 */
    {
      fputs("</table>\n", fpw);
      return;
    }

    if (memcmp(buf, header1[i], LEN_AUTHOR1 - 1) && memcmp(buf, header2[i], LEN_AUTHOR2 - 1))	/* 不是檔頭 */
      break;

    /* 作者/看板 檔頭有二欄，特別處理 */
    if (i == 0 && ((pbrd = strstr(buf, "看板:")) || (pbrd = strstr(buf, "站內:"))))
    {
      if (board = strchr(pbrd, '\n'))
	*board = '\0';
      board = pbrd + 6;
      pbrd[-1] = '\0';
      pbrd[4] = '\0';
    }

    if (!(headvalue = strchr(buf, ':')))
      break;

    fprintf(fpw, "<tr>\n"
      "  <td align=center width=10%% class=A03447>%s</td>\n", header1[i]);

    str_html(headvalue + 2, TTLEN);
    if (i == 0 && pbrd)
    {
      fprintf(fpw, "  <td width=60%% class=A03744>&nbsp;%s</td>\n"
	"  <td align=center width=10%% class=A03447>%s</td>\n"
	"  <td width=20%% class=A03744>&nbsp;%s</td>\n</tr>\n",
	ansi_buf, pbrd, board);
    }
    else
    {
      fputs("  <td width=90% colspan=3 class=A03744>&nbsp;", fpw);
      fputs(ansi_buf, fpw);
      fputs("</td>\n</tr>\n", fpw);
    }
  }

  fputs("<tr>\n"
    "<td colspan=4><pre><font class=A03740>", fpw);

  old_color = now_color = 0x00003740;

#ifdef HAVE_ANSIATTR
  old_attr = now_attr = 0;
#endif

  if (i >= LINE_HEADER)		/* 最後一行是檔頭 */
    fgets(buf, ANSILINELEN, fp);

  /* 處理內文 */
  do
  {
    if (!ansi_quote(fpw, buf))
      ansi_html(fpw, buf);
  } while (fgets(buf, ANSILINELEN, fp));

  fputs("</font></pre></td>\n</table>\n", fpw);
}


/* ----------------------------------------------------- */
/* HTML output basic function				 */
/* ----------------------------------------------------- */

static char *
Gtime(now)
  time_t *now;
{
  static char datemsg[32];

  strftime(datemsg, sizeof(datemsg), "%a, %d %b %Y %T GMT", gmtime(now));
  return datemsg;
}


static FILE *
out_http(ap, code, type)
  Agent *ap;
  int code;
  char *type;
{
  time_t now;
  FILE *fpw;
  int state;

  fpw = ap->fpw;
  state = code & ~HS_REFRESH;

  /* HTTP 1.0 檔頭 */
  time(&now);

  fprintf(fpw, "HTTP/1.0 %s\r\n"
    "Date: %s\r\n"
    "Server: MapleBBS 3.10\r\n"
    "Connection: close\r\n", http_msg[state], Gtime(&now));

  if (state == HS_NOTMOIDIFY)
  {
    fputs("\r\n", fpw);
  }
  else if (state == HS_REDIRECT)/* Location之後不需要內容 */
  {
#if BHTTP_PORT == 80
    fprintf(fpw, "Location: http://" MYHOSTNAME "/\r\n\r\n");
#else
    fprintf(fpw, "Location: http://" MYHOSTNAME ":%d/\r\n\r\n", BHTTP_PORT);
#endif
  }
  else
  {
    if (code & HS_REFRESH)
    {
      if (!type)
	type = "/";
      fprintf(fpw, "Refresh: 3; url=%s\r\n", type);
    }
    if ((code & HS_REFRESH) || !type)
    {
      fputs("Pragma: no-cache\r\n"	/* 網頁一律不讓proxy做cache */
	"Content-Type: text/html; charset=" MYCHARSET "\r\n", fpw);
    }
    else
      fprintf(fpw, "Content-Type: %s\r\n", type);

    if (ap->setcookie)		/* cmd_login() 完以後才需要 Set-Cookie */
      fprintf(fpw, "Set-Cookie: user=%s; path=/\r\n", ap->cookie);
  }

  return fpw;
}


static void
out_error(ap, code)		/* code不可以是HS_OK */
  Agent *ap;
  int code;
{
  char *msg;

  if (code < HS_OK)
  {
    fprintf(ap->fpw, "<BR>%s<BR><BR>\n", http_msg[code]);
    return;
  }

  out_http(ap, code, NULL);
  switch (code)
  {
  case HS_BADREQUEST:
    msg = "Your browser sent a request that this server could not understand.";
    break;
  case HS_FORBIDDEN:
    msg = "You don't have permission to access the URL on this server.";
    break;
  case HS_NOTFOUND:
    msg = "The requested URL was not found on this server.";
    break;
  default:			/* HS_REDIRECT, HS_NOTMOIDIFY */
    return;
  }
  /* html 檔案開始 */
  fprintf(ap->fpw, "\r\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
    "<HTML><HEAD>\n"
    "<TITLE>%s</TITLE>\n"
    "</HEAD><BODY>\n"
    "<H1>%s</H1>\n%s\n<HR>\n"
    "<ADDRESS>MapleBBS/3.10 Server at " MYHOSTNAME "</ADDRESS>\n"
    "</BODY></HTML>\n", http_msg[code], http_msg[code] + 4, msg);
}


/* out_head() 中的 <HTML> <BODY> <CENTER> 三個大寫標籤貫穿整個 html 檔
   直到 out_tail() 才由 </HTML> </BODY> </CENTER> 還原 */


static void
out_title(fpw, title)
  FILE *fpw;
  char *title;
{
  /* html 檔案開始 */
  fprintf(fpw, "\r\n<HTML><HEAD>\n"
    "<meta http-equiv=Content-Type content=\"text/html; charset=" MYCHARSET "\">\n"
#ifdef ROBOT_EXCLUSION
    "<meta name=robots content=noindex,nofollow>\n"
#endif
    "<title>-=" BBSNAME "=- %s</title>\n", title);

  fputs("<script type=text/javascript>\n<!--\n"
    "  function mOver(obj) {obj.bgColor='" HCOLOR_BAR "';}\n"
    "  function mOut(obj) {obj.bgColor='" HCOLOR_BG "';}\n"
    "-->\n</script>\n"
    "<style type=text/css>\n"
    "  PRE {font-size: 15pt; line-height: 15pt; font-weight: lighter; background-color: #000000; color: #C0C0C0;}\n"
    "  TD  {font-size: 15pt; line-height: 15pt; font-weight: lighter;}\n"
    "</style>\n"
    "<link rel=stylesheet href=/img?ansi.css type=text/css>\n"
    "</head>\n"
    "<BODY bgcolor=" HCOLOR_BG " text=" HCOLOR_TEXT " link=" HCOLOR_LINK " vlink=" HCOLOR_VLINK " alink=" HCOLOR_ALINK "><CENTER>\n"
    "<a href=/><img src=/img?site.gif border=0></a><br>\n"
    "<input type=image src=/img?back.gif onclick=\"javascript:history.go(-1);\"> / "
    "<a href=/class><img src=/img?class.gif border=0></a> / "
    "<a href=/brdlist><img src=/img?board.gif border=0></a> / "
    "<a href=/fvrlist><img src=/img?favor.gif border=0></a> / "
    "<a href=/mbox><img src=/img?mbox.gif border=0></a> / "
    "<a href=/usrlist><img src=/img?user.gif border=0></a> / "
    "<a href=telnet://" MYHOSTNAME "><img src=/img?telnet.gif border=0></a><br>\n", fpw);
}


static FILE *
out_head(ap, title)
  Agent *ap;
  char *title;
{
  FILE *fpw = out_http(ap, HS_OK, NULL);
  out_title(fpw, title);
  return fpw;
}


static void
out_mesg(fpw, msg)
  FILE *fpw;
  char *msg;
{
  fprintf(fpw, "<BR>%s<BR><BR>\n", msg);
}


static void
out_style(fpw)
  FILE *fpw;
{
#ifdef HAVE_HYPERLINK
  fputs("<style type=text/css>\n"
    "  a:link.PRE    {COLOR: #FFFFFF}\n"
    "  a:visited.PRE {COLOR: #FFFFFF}\n"
    "  a:active.PRE {COLOR: " HCOLOR_ALINK "; TEXT-DECORATION: none}\n"
    "  a:hover.PRE  {COLOR: " HCOLOR_ALINK "; TEXT-DECORATION: none}\n"
    "</style>\n", fpw);
#endif
}


static void
out_article(fpw, fpath)
  FILE *fpw;
  char *fpath;
{
  FILE *fp;

  if (fp = fopen(fpath, "r"))
  {
    out_style(fpw);
    txt2htm(fpw, fp);
    fclose(fp);
  }
}


static void
out_tail(fpw)
  FILE *fpw;
{
  fputs("</CENTER></BODY></HTML>\n", fpw);
}


static void
out_reload(fpw, msg)		/* God.050327: 將主視窗 reload 並關掉新開視窗 */
  FILE *fpw;
  char *msg;
{
  fprintf(fpw, "</CENTER></BODY></HTML>\n"
    "<script>alert(\'%s\');\n"
    "opener.location.reload();\n"
    "parent.close();</script>", msg);
}


/* ----------------------------------------------------- */
/* 解碼分析參數						 */
/* ----------------------------------------------------- */

#define hex2int(x)	((x >= 'A') ? (x - 'A' + 10) : (x - '0'))

static int			/* 1:成功 */
arg_analyze(argc, mark, str, arg1, arg2, arg3, arg4)
  int argc;			/* 有幾個參數 */
  int mark;			/* !=0: str 要是 mark 開頭的字串 */
  char *str;			/* 引數 */
  char **arg1;			/* 參數一 */
  char **arg2;			/* 參數二 */
  char **arg3;			/* 參數三 */
  char **arg4;			/* 參數四 */
{
  int i, ch;
  char *dst;

  if ((mark && *str++ != mark) || !(ch = *str))
  {
    *arg1 = NULL;
    return 0;
  }

  *arg1 = dst = str;
  i = 2;

  while (ch)
  {
    if (ch == '&' || ch == '\r')
    {
      if (i > argc)
	break;

      *dst++ = '\0';
      if (i == 2)
	*arg2 = dst;
      else if (i == 3)
	*arg3 = dst;
      else /* if (i == 4) */
	*arg4 = dst;
      i++;
    }
    else if (ch == '+')
    {
      *dst++ = ' ';
    }
    else if (ch == '%')
    {
      ch = *(++str);
      if (isxdigit(ch) && isxdigit(str[1]))
      {
	ch = (hex2int(ch) << 4) + hex2int(str[1]);
	str++;
	if (ch != '\r')		/* '\r' 就不要了 */
	  *dst++ = ch;
      }
      else
      {
	*dst++ = '%';
	continue;
      }
    }
    else
      *dst++ = ch;

    ch = *(++str);
  }
  *dst = '\0';

  return i > argc;
}


/* ----------------------------------------------------- */
/* 由 Cookie 看使用者是否登入				 */
/* ----------------------------------------------------- */

static int guestuno = 0;

static void
guest_userno()
{
  char fpath[64];
  ACCT acct;

  usr_fpath(fpath, STR_GUEST, FN_ACCT);
  if (!rec_get(fpath, &acct, sizeof(ACCT), 0))
    guestuno = acct.userno;
}

static int			/* 1:登入成功 0:登入失敗 */
acct_fetch(ap)
  Agent *ap;
{
  char *userid, *passwd;
  char fpath[64];
  ACCT acct;

  if (ap->cookie[0])
  {
    /* u=userid&p=passwd */
    if (!arg_analyze(2, 0, ap->cookie, &userid, &passwd, NULL, NULL))
      return 0;

    passwd += 2;		/* skip "p=" */

    if (*userid && *passwd && strlen(userid) <= IDLEN && strlen(passwd) == PASSLEN)
    {
      usr_fpath(fpath, userid, FN_ACCT);
      if (!rec_get(fpath, &acct, sizeof(ACCT), 0) &&
	!(acct.userlevel & (PERM_DENYLOGIN | PERM_PURGE)) &&
	!strncmp(acct.passwd, passwd, PASSLEN))	/* 登入成功 */
      {
	ap->userno = acct.userno;
	strcpy(ap->userid, acct.userid);
	strcpy(ap->username, acct.username);
	ap->userlevel = acct.userlevel;
	return 1;
      }
    }
  }

  /* 沒有登入、登入失敗 */
  ap->userno = guestuno;		/* 填入實際guest的userno，以便做pal檢查 */
  ap->userlevel = 0;
  strcpy(ap->userid, STR_GUEST);
  strcpy(ap->username, STR_GUEST);
  return 0;
}


/* ----------------------------------------------------- */
/* UTMP shm 部分須與 cache.c 相容			 */
/* ----------------------------------------------------- */

static UCACHE *ushm;

static void
init_ushm()
{
  ushm = shm_new(UTMPSHM_KEY, sizeof(UCACHE));
}


static int			/* 1: userno 在 pool 名單上 */
pertain_pal(pool, max, userno)	/* 參考 pal.c:belong_pal() */
  int *pool;
  int max;
  int userno;
{
  int datum, mid;

  while (max > 0)
  {
    datum = pool[mid = max >> 1];
    if (userno == datum)
    {
      return 1;
    }
    if (userno > datum)
    {
      pool += (++mid);
      max -= mid;
    }
    else
    {
      max = mid;
    }
  }
  return 0;
}


static int			/* 1: 對方設我為壞人 */
is_hisbad(up, userno)		/* 參考 pal.c:is_obad() */
  UTMP *up;
  int userno;
{
#ifdef HAVE_BADPAL
  return pertain_pal(up->pal_spool, up->pal_max, -userno);
#else
  return 0;
#endif
}


static int			/* 1:可看見 0:不可看見 */
can_seen(up, userno, ulevel)	/* 參考 bmw.c:can_see() */
  UTMP *up;
  int userno;
  usint ulevel;
{
  usint urufo;

  urufo = up->ufo;

  if (!(ulevel & PERM_SEECLOAK) && ((urufo & UFO_CLOAK) || is_hisbad(up, userno)))
    return 0;

#ifdef HAVE_SUPERCLOAK
  if (urufo & UFO_SUPERCLOAK)
    return 0;
#endif

  return 1;
}


/* itoc.030711: 加上檢查使用者帳號的部分，以免有人亂踹 */
static int
allow_userid(ap, userid)
  Agent *ap;
  char *userid;
{
  int ch, rc;
  char *str;

  rc = 0;
  ch = strlen(userid);
  if (ch >= 2 && ch <= IDLEN && is_alpha(*userid))
  {
    rc = 1;
    str = userid;
    while (ch = *(++str))
    {
      if (!is_alnum(ch))
      {
	rc = 0;
	break;
      }
    }
  }

  return rc;
}


/* ----------------------------------------------------- */
/* board：shm 部份須與 cache.c 相容			 */
/* ----------------------------------------------------- */

static BCACHE *bshm;

static void
init_bshm()
{
  /* itoc.030727: 在開啟 bbsd 之前，應該就要執行過 account，
     所以 bshm 應該已設定好 */

  if (bshm)
    return;

  bshm = shm_new(BRDSHM_KEY, sizeof(BCACHE));

  if (bshm->uptime <= 0)	/* bshm 未設定完成 */
    exit(0);
}


#ifdef HAVE_MODERATED_BOARD
static int			/* !=0:是板好  0:不在名單中 */
is_brdgood(userno, bpal)	/* 參考 pal.c:is_bgood() */
  int userno;
  BPAL *bpal;
{
  return pertain_pal(bpal->pal_spool, bpal->pal_max, userno);
}


static int			/* !=0:是板壞  0:不在名單中 */
is_brdbad(userno, bpal)		/* 參考 pal.c:is_bbad() */
  int userno;
  BPAL *bpal;
{
#ifdef HAVE_BADPAL
  return pertain_pal(bpal->pal_spool, bpal->pal_max, -userno);
#else
  return 0;
#endif
}
#endif


static int
Ben_Perm(brd, uno, uid, ulevel)	/* 參考 board.c:Ben_Perm() */
  BRD *brd;
  int uno;
  char *uid;
  usint ulevel;
{
  usint readlevel, postlevel, bits;
  char *blist, *bname;

#ifdef HAVE_MODERATED_BOARD
  BPAL *bpal;
  int ftype;			/* 0:一般ID 1:板好 2:板壞 */

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

  if (!brd)
    return 0;
  bname = brd->brdname;
  if (!*bname)
    return 0;

  readlevel = brd->readlevel;

#ifdef HAVE_MODERATED_BOARD
  bpal = bshm->pcache + (brd - bshm->bcache);
  ftype = is_brdgood(uno, bpal) ? 1 : is_brdbad(uno, bpal) ? 2 : 0;

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
  if ((ulevel & PERM_BM) && blist[0] > ' ' && str_has(blist, uid, strlen(uid)))
    bits = BRD_L_BIT | BRD_R_BIT | BRD_W_BIT | BRD_X_BIT | BRD_M_BIT;

  /* itoc.030515: 看板總管重新判斷 */
  else if (ulevel & PERM_ALLBOARD)
    bits = BRD_L_BIT | BRD_R_BIT | BRD_W_BIT | BRD_X_BIT;

  return bits;
}


static BRD *
brd_get(bname)
  char *bname;
{
  BRD *bhdr, *tail;

  bhdr = bshm->bcache;
  tail = bhdr + bshm->number;
  do
  {
    if (!strcmp(bname, bhdr->brdname))
      return bhdr;
  } while (++bhdr < tail);
  return NULL;
}


static int		/* 回傳PermBits方便做多次判斷 */
ben_perm(ap, brdname)
  Agent *ap;
  char *brdname;
{
  BRD *brd;

  if (acct_fetch(ap) && (brd = brd_get(brdname)))
    return Ben_Perm(brd, ap->userno, ap->userid, ap->userlevel);
  return 0;
}


static BRD *
allow_brdname(ap, brdname)
  Agent *ap;
  char *brdname;
{
  BRD *bhdr;

  if (bhdr = brd_get(brdname))
  {
    /* 若 readlevel == 0，表示 guest 可讀，無需 acct_fetch() */
    if (!bhdr->readlevel)
      return bhdr;

    if (acct_fetch(ap) && (Ben_Perm(bhdr, ap->userno, ap->userid, ap->userlevel) & BRD_R_BIT))
      return bhdr;
  }
  return NULL;
}


/* ----------------------------------------------------- */
/* movie：shm 部份須與 cache.c 相容			 */
/* ----------------------------------------------------- */

static FCACHE *fshm;


static void
init_fshm()
{
  fshm = shm_new(FILMSHM_KEY, sizeof(FCACHE));
}


static void
out_film(fpw, tag)
  FILE *fpw;
  int tag;
{
  int i, *shot, len;
  char *film, buf[FILM_SIZ];

  shot = fshm->shot;
  for (i = 0; !(*shot) && i < 5; ++i)
    sleep(1);

  if (!(*shot))
    return;		/* 若 5 秒以後還沒換好片，可能是沒跑 camera，直接離開 */

  film = fshm->film;
  if (tag)
  {
    len = shot[tag];
    film += len;
    len = shot[tag + 1] - len;
  }
  else
    len = shot[1];

  memcpy(buf, film, len);
  buf[len] = '\0';

  out_style(fpw);
  fputs("<table width=760 cellspacing=0 cellpadding=0 border=0>\n"
    "<tr>\n"
    "<td colspan=4><pre><font class=A03740>", fpw);
  ansi_html(fpw, buf);
  fputs("</font></pre></td>\n</table>\n", fpw);
}


/* ----------------------------------------------------- */
/* command dispatch (GET)				 */
/* ----------------------------------------------------- */

  /* --------------------------------------------------- */
  /* 通用清單						 */
  /* --------------------------------------------------- */

static void
list_neck(fpw, start, total, title)
  FILE *fpw;
  int start, total;
  char *title;
{
  fputs("<br>\n"
    "<table cellspacing=0 cellpadding=1 border=0 width=760>\n"
    "<tr bgcolor=" HCOLOR_NECK ">\n  <td width=15%", fpw);
  if (start != 1)
  {
    fprintf(fpw, " align=center><a href=?%d>上%d個</a",
      (start > HTML_TALL ? start - HTML_TALL : 1), HTML_TALL);
  }
  fputs("></td>\n  <td width=15%", fpw);

  start += HTML_TALL;
  if (start <= total)
  {
    fprintf(fpw, " align=center><a href=?%d>下%d個</a",
      start, HTML_TALL);
  }
  fputs("></td>\n  <td width=40% align=center>", fpw);
  fprintf(fpw, title, total);
  fprintf(fpw, "</td>\n"
    "  <td width=15%% align=center><a href=?1>前%d個</a></td>\n"
    "  <td width=15%% align=center><a href=?0>末%d個</a></td>\n"
    "</tr></table><br>\n", HTML_TALL, HTML_TALL);
}


static void
cmdlist_list(ap, title, list_tie, list_item)
  Agent *ap;
  char *title;
  void (*list_tie) (FILE *);
  void (*list_item) (FILE *, void *, int);
{
  int i, start, end, total;
  char *number;
  FILE *fpw;

  if (!arg_analyze(1, '?', ap->urlp, &number, NULL, NULL, NULL))
    start = 1;
  else
    start = atoi(number);
  total = ap->total_item;
  if (start <= 0 || start > total)	/* 超過範圍的話，直接到最後一頁 */
    start = total > HTML_TALL ? total - HTML_TALL + 1 : 1;

  fpw = ap->fpw;
  list_neck(fpw, start, total, title);
  fputs("<table cellspacing=0 cellpadding=4 border=0>\n<tr bgcolor=" HCOLOR_TIE ">\n", fpw);
  list_tie(fpw);

  end = start + HTML_TALL - 1;
  if (end > total)
    end = total;
  for (i = start - 1; i < end; i++)
  {
    fputs("<tr onmouseover=mOver(this); onmouseout=mOut(this);>\n", fpw);
    list_item(fpw, ap->myitem[i], i + 1);
  }

  fputs("</table>\n", fpw);
  list_neck(fpw, start, total, title);
}


  /* --------------------------------------------------- */
  /* 使用者名單						 */
  /* --------------------------------------------------- */

static int
userid_cmp(a, b)
  UTMP **a, **b;
{
  return str_cmp((*a)->userid, (*b)->userid);
}


static void
init_myusr(ap)
  Agent *ap;
{
  int num, userno;
  UTMP *uentp, *uceil;
  usint ulevel;

  acct_fetch(ap);
  uentp = ushm->uslot;
  uceil = (void *)uentp + ushm->offset;
  userno = ap->userno;
  ulevel = ap->userlevel;
  num = 0;

  do
  {
    if (!uentp->pid || !uentp->userno || !can_seen(uentp, userno, ulevel))
      continue;

    ap->myitem[num] = uentp;
    num++;
  } while (++uentp <= uceil);

  if (num > 1)
    qsort(ap->myitem, num, sizeof(UTMP *), userid_cmp);

  ap->total_item = num;
}


static void
userlist_tie(fpw)
  FILE *fpw;
{
  fputs("  <td width=40>編號</td>\n"
    "  <td width=100>網友代號</td>\n"
    "  <td width=210>網友暱稱</td>\n"
    "  <td width=230>客途故鄉</td>\n"
    "  <td width=100>網友動態</td>\n"
    "</tr>\n", fpw);
}


static void
userlist_item(fpw, up, n)
  FILE *fpw;
  UTMP *up;
  int n;
{
  fprintf(fpw, "  <td>%d</td>\n"
    "  <td><a href=/query?%s>%s</a></td>\n"
    "  <td>%s</td>\n"
    "  <td>%s</td>\n"
    "  <td>%s</td>\n"
    "</tr>\n",
    n,
    up->userid, up->userid,
    str_html(up->username, UNLEN),
    up->from,
    ModeTypeTable[up->mode]);
}


static int
cmd_userlist(ap)
  Agent *ap;
{
  init_myusr(ap);
  out_head(ap, "使用者名單");

  cmdlist_list(ap, "目前站上有 %d 個人", userlist_tie, userlist_item);

  return HS_END;
}


  /* --------------------------------------------------- */
  /* 看板列表						 */
  /* --------------------------------------------------- */

static int
brdtitle_cmp(a, b)
  BRD **a, **b;
{
  /* itoc.010413: 分類/板名交叉比對 */
  int k = strcmp((*a)->class, (*b)->class);
  return k ? k : str_cmp((*a)->brdname, (*b)->brdname);
}


static void
init_mybrd(ap)
  Agent *ap;
{
  int num, uno;
  char *uid;
  usint ulevel;
  BRD *bhdr, *tail;

  acct_fetch(ap);
  uno = ap->userno;
  uid = ap->userid;
  ulevel = ap->userlevel;
  bhdr = bshm->bcache;
  tail = bhdr + bshm->number;
  num = 0;

  do
  {
    if (Ben_Perm(bhdr, uno, uid, ulevel) & BRD_R_BIT)
    {
      ap->myitem[num] = bhdr;
      num++;
    }
  } while (++bhdr < tail);

  if (num > 1)
    qsort(ap->myitem, num, sizeof(BRD *), brdtitle_cmp);

  ap->total_item = num;
}


static void
boardlist_tie(fpw)
  FILE *fpw;
{
  fputs("  <td width=40>編號</td>\n"
    "  <td width=80>看板</td>\n"
    "  <td width=40>類別</td>\n"
    "  <td width=25>轉</td>\n"
    "  <td width=350>中文敘述</td>\n"
    "  <td width=75>板主</td>\n"
    "</tr>\n", fpw);
}


static void
boardlist_item(fpw, brd, n)
  FILE *fpw;
  BRD *brd;
  int n;
{
  fprintf(fpw, "  <td>%d</td>\n"
    "  <td><a href=/brd?%s>%s</a></td>\n"
    "  <td>%s</td>\n"
    "  <td>%s</td>\n"
    "  <td>%s</td>\n"
    "  <td>%.13s</td>\n"
    "</tr>\n",
    n,
    brd->brdname, brd->brdname,
    brd->class,
    (brd->battr & BRD_NOTRAN) ? ICON_NOTRAN_BRD : ICON_TRAN_BRD,
    str_html(brd->title, 33),
    brd->BM);
}


static int
cmd_boardlist(ap)
  Agent *ap;
{
  init_mybrd(ap);
  out_head(ap, "看板列表");

  cmdlist_list(ap, "目前站上有 %d 個板", boardlist_tie, boardlist_item);

  return HS_END;
}


  /* --------------------------------------------------- */
  /* 我的最愛						 */
  /* --------------------------------------------------- */

static void
init_myfavor(ap)
  Agent *ap;
{
  int num, uno;
  usint ulevel;
  char *uid, fpath[64];
  BRD *bhdr;
  FILE *fp;
  MF mf;

  uno = ap->userno;
  uid = ap->userid;
  ulevel = ap->userlevel;
  num = 0;
  usr_fpath(fpath, ap->userid, "MF/" FN_MF);

  if (fp = fopen(fpath, "r"))
  {
    while (fread(&mf, sizeof(MF), 1, fp) == 1)
    {
      /* 只支援第一層的看板 */
      if ((mf.mftype & MF_BOARD) &&
	(bhdr = brd_get(mf.xname)) &&
	(Ben_Perm(bhdr, uno, uid, ulevel) & BRD_R_BIT))
      {
	ap->myitem[num] = bhdr;
	num++;
      }
    }
    fclose(fp);
  }
  ap->total_item = num;
}


static int
cmd_favorlist(ap)
  Agent *ap;
{
  out_head(ap, "我的最愛");
  if (!acct_fetch(ap))
    return HS_ERR_LOGIN;

  init_myfavor(ap);

  cmdlist_list(ap, "我的最愛", boardlist_tie, boardlist_item);

  return HS_END;
}


  /* --------------------------------------------------- */
  /* 分類看板列表					 */
  /* --------------------------------------------------- */

static void
class_neck(fpw)
  FILE *fpw;
{
  fputs("<br>\n"
    "<table cellspacing=0 cellpadding=1 border=0 width=760>\n"
    "<tr bgcolor=" HCOLOR_NECK ">\n"
    "  <td width=50%></td>\n"
    "  <td width=50% align=center><a href=/class>回最上層</a></td>\n"
    "</tr></table><br>\n", fpw);
}


static int
cmd_class(ap)
  Agent *ap;
{
  int fd, i, userno;
  usint ulevel;
  char folder[64], *xname, *userid;
  BRD *brd;
  HDR hdr;
  FILE *fpw = out_head(ap, "分類看板");

  if (!arg_analyze(1, '?', ap->urlp, &xname, NULL, NULL, NULL))
    xname = CLASS_INIFILE;

  if (strlen(xname) > 12)
    return HS_ERROR;

  sprintf(folder, "gem/@/@%s", xname);
  if ((fd = open(folder, O_RDONLY)) < 0)
    return HS_ERR_CLASS;

  acct_fetch(ap);
  userno = ap->userno;
  userid = ap->userid;
  ulevel = ap->userlevel;

  class_neck(fpw);
  fputs("<table cellspacing=0 cellpadding=4 border=0>\n<tr bgcolor=" HCOLOR_TIE ">\n", fpw);
  boardlist_tie(fpw);
  i = 1;
  while (read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
  {
    fputs("<tr onmouseover=mOver(this); onmouseout=mOut(this);>\n", fpw);
    if (hdr.xmode & GEM_BOARD)	/* 看板 */
    {
      if ((brd = brd_get(hdr.xname)) &&
	Ben_Perm(brd, userno, userid, ulevel) & BRD_R_BIT)
      {
	boardlist_item(fpw, brd, i);
      }
      else
	continue;
    }
    else if ((hdr.xmode & GEM_FOLDER) && *hdr.xname == '@')	/* 分類 */
    {
      fprintf(fpw, "  <td>%d</td>\n"
	"  <td><a href=/class?%s>%s/</a></td>\n"
	"  <td>分類</td>\n"
	"  <td>□</td>\n"
	"  <td colspan=2>%s</td>\n</tr>\n",
	i,
	hdr.xname + 1, hdr.xname + 1,
	str_html(hdr.title + 21, 52));
    }
    else			/* 其他類別就不秀了 */
    {
      continue;
    }

    i++;
  }
  close(fd);
  fputs("</table>\n", fpw);
  class_neck(fpw);
  return HS_END;
}


  /* --------------------------------------------------- */
  /* 文章列表						 */
  /* --------------------------------------------------- */

static void
postlist_list(fpw, folder, brdname, start, total)
  FILE *fpw;
  char *folder, *brdname;
  int start, total;
{
  HDR hdr;
  char owner[80], *ptr1, *ptr2;
  int fd, xmode;

  fputs("<table cellspacing=0 cellpadding=4 border=0>\n<tr bgcolor=" HCOLOR_TIE ">\n"
    "  <td width=15>標</td>\n"
    "  <td width=15>刪</td>\n"
    "  <td width=50>編號</td>\n"
    "  <td width=10>m</td>\n"

#ifdef HAVE_SCORE
    "  <td width=10>&nbsp;</td>\n"
#endif

    "  <td width=50>日期</td>\n"
    "  <td width=100>作者</td>\n"
    "  <td width=400>標題</td>\n"
    "</tr>\n", fpw);

  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    int i, end;

    /* 秀出看板的第 start 篇開始的 HTML_TALL 篇 */
    i = start;
    end = i + HTML_TALL;

    lseek(fd, (off_t) (sizeof(HDR) * (i - 1)), SEEK_SET);

    while (i < end && read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
    {
      strcpy(owner, hdr.owner);
      if (ptr1 = strchr(owner, '.'))	/* 站外作者 */
	*(ptr1 + 1) = '\0';
      if (ptr2 = strchr(owner, '@'))	/* 站外作者 */
	*ptr2 = '\0';

      fputs("<tr onmouseover=mOver(this); onmouseout=mOut(this);>\n", fpw);
      if (brdname)
      {
	fprintf(fpw, "  <td><a href=/mpost?%s&%d&%d><img src=/img?mark.gif border=0></a></td>\n"
	  "  <td><a href=/dpost?%s&%d&%d><img src=/img?del.gif border=0></a></td>\n",
	  brdname, i, hdr.chrono, brdname, i, hdr.chrono);
      }
      else
      {
	fprintf(fpw, "  <td><a href=/mmail?%d&%d><img src=/img?mark.gif border=0></a></td>\n"
	  "  <td><a href=/dmail?%d&%d><img src=/img?del.gif border=0></a></td>\n",
	  i, hdr.chrono, i, hdr.chrono);
      }

      xmode = hdr.xmode;

      fprintf(fpw, "  <td>%d</td>\n  <td>%s</td>\n  <td>",
	xmode & POST_BOTTOM ? -1 : i, xmode & POST_MARKED ? "m" : "");

#ifdef HAVE_SCORE
      if (xmode & POST_SCORE)
	fprintf(fpw, "<font color='%s'>%d</font>", hdr.score >= 0 ? "red" : "green", abs(hdr.score));
#endif

      fprintf(fpw, "</td>\n  <td>%s</td>\n  <td><a href=%s%s>%s</a></td>\n",
	hdr.date + 3, (ptr1 || ptr2) ? "mailto:" : "/query?", hdr.owner, owner);

      if (brdname)
	fprintf(fpw, "  <td><a href=/bmore?%s&%d>", brdname, i);
      else
	fprintf(fpw, "  <td><a href=/mmore?%d&>", i);

      fprintf(fpw, "%s</td>\n</tr>\n", str_html(hdr.title, 50));

      i++;
    }
    close(fd);
  }

  fputs("</table>\n", fpw);
}


static void
postlist_neck(fpw, start, total, brdname)
  FILE *fpw;
  int start, total;
  char *brdname;
{
  fputs("<br>\n"
    "<table cellspacing=0 cellpadding=1 border=0 width=760>\n"
    "<tr bgcolor=" HCOLOR_NECK ">\n  <td width=20%", fpw);

  if (start > HTML_TALL)
  {
    fprintf(fpw, " align=center><a href=?%s&%d>上%d篇</a",
      brdname, start - HTML_TALL, HTML_TALL);
  }
  fputs("></td>\n  <td width=20%", fpw);

  start += HTML_TALL;
  if (start <= total)
  {
    fprintf(fpw, " align=center><a href=?%s&%d>下%d篇</a",
      brdname, start, HTML_TALL);
  }

  fprintf(fpw, "></td>\n  <td width=20%% align=center><a href=/dopost?%s target=_blank>發表文章</a></td>\n"
    "  <td width=20%% align=center><a href=/gem?%s>精華區</a></td>\n"
    "  <td width=20%% align=center><a href=/brdlist>看板列表</a>&nbsp;"
    "<a href=/rss?%s><img border=0 src=/img?xml.gif alt=\"RSS 訂閱\這個看板\"></a></td>\n"
    "</tr></table><br>\n",
    brdname, brdname, brdname);
}


static int
cmd_postlist(ap)
  Agent *ap;
{
  int start, total;
  char folder[64], *brdname, *number;
  FILE *fpw = out_head(ap, "文章列表");

  if (!arg_analyze(2, '?', ap->urlp, &brdname, &number, NULL, NULL))
  {
    if (brdname)
      number = "0";
    else
      return HS_ERROR;
  }

  if (!allow_brdname(ap, brdname))
    return HS_ERR_BOARD;

  brd_fpath(folder, brdname, FN_DIR);

  start = atoi(number);
  total = rec_num(folder, sizeof(HDR));
  if (start <= 0 || start > total)	/* 超過範圍的話，直接到最後一頁 */
    start = (total - 1) / HTML_TALL * HTML_TALL + 1;

  postlist_neck(fpw, start, total, brdname);

  postlist_list(fpw, folder, brdname, start, total);

  postlist_neck(fpw, start, total, brdname);
  return HS_END;
}


  /* --------------------------------------------------- */
  /* 信箱列表						 */
  /* --------------------------------------------------- */

static void
mboxlist_neck(fpw, start, total)
  FILE *fpw;
  int start, total;
{
  fputs("<br>\n"
    "<table cellspacing=0 cellpadding=1 border=0 width=760>\n"
    "<tr bgcolor=" HCOLOR_NECK ">\n  <td width=33% ", fpw);

  if (start > HTML_TALL)
  {
    fprintf(fpw, "align=center><a href=?%d>上%d篇</a",
      start - HTML_TALL, HTML_TALL);
  }
  fputs("></td>\n  <td width=33%", fpw);

  start += HTML_TALL;
  if (start <= total)
  {
    fprintf(fpw, " align=center><a href=?%d>下%d篇</a",
      start, HTML_TALL);
  }

  fputs("></td>\n  <td width=34% align=center><a href=/domail target=_blank>發送信件</a></td>\n"
    "</tr></table><br>\n", fpw);
}


static int
cmd_mboxlist(ap)
  Agent *ap;
{
  int start, total;
  char folder[64], *number;
  FILE *fpw = out_head(ap, "信箱列表");

  if (!acct_fetch(ap))
    return HS_ERR_LOGIN;

  if (!arg_analyze(1, '?', ap->urlp, &number, NULL, NULL, NULL))
    number = "0";

  usr_fpath(folder, ap->userid, FN_DIR);

  start = atoi(number);
  total = rec_num(folder, sizeof(HDR));
  if (start <= 0 || start > total)	/* 超過範圍的話，直接到最後一頁 */
    start = (total - 1) / HTML_TALL * HTML_TALL + 1;

  mboxlist_neck(fpw, start, total);

  postlist_list(fpw, folder, NULL, start, total);

  mboxlist_neck(fpw, start, total);
  return HS_END;
}


  /* --------------------------------------------------- */
  /* 精華區列表						 */
  /* --------------------------------------------------- */

static void
gemlist_neck(fpw, brdname)
  FILE *fpw;
  char *brdname;
{
  fprintf(fpw, "<br>\n"
    "<table cellspacing=0 cellpadding=1 border=0 width=760>\n"
    "<tr bgcolor=" HCOLOR_NECK ">\n"
    "  <td width=50%% align=center><a href=/brd?%s>回到看板</a></td>\n"
    "  <td width=50%% align=center><a href=/brdlist>看板列表</a></td>\n"
    "</tr></table><br>\n",
    brdname);
}


static int
cmd_gemlist(ap)
  Agent *ap;
{
  int fd, i;
  char folder[64], *brdname, *xname;
  HDR hdr;
  FILE *fpw = out_head(ap, "精華區");

  if (!arg_analyze(2, '?', ap->urlp, &brdname, &xname, NULL, NULL))
  {
    if (brdname)
      xname = FN_DIR;
    else
      return HS_ERROR;
  }

  if ((*xname != 'F' || strlen(xname) != 8) && strcmp(xname, FN_DIR))
    return HS_ERROR;

  if (!allow_brdname(ap, brdname))
    return HS_ERR_BOARD;

  gemlist_neck(fpw, brdname);

  fputs("<table cellspacing=0 cellpadding=4 border=0>\n"
    "<tr bgcolor=" HCOLOR_TIE ">\n"
    "  <td width=50>編號</td>\n"
    "  <td width=400>標題</td>\n"
    "</tr>\n", fpw);

  if (*xname == '.')
    sprintf(folder, "gem/brd/%s/%s", brdname, FN_DIR);
  else /* if (*xname == 'F') */
    sprintf(folder, "gem/brd/%s/%c/%s", brdname, xname[7], xname);

  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    i = 1;
    while (read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
    {
      fprintf(fpw, "<tr onmouseover=mOver(this); onmouseout=mOut(this);>\n"
	"  <td>%d</td>\n", i);
      if (hdr.xmode & GEM_RESTRICT)
      {
	fputs("  <td>◇ 資料保密</td>\n</tr>\n", fpw);
      }
      else if (hdr.xname[0] == 'A')	/* 文章 */
      {
	fprintf(fpw, "  <td><a href=/gmore?%s&%s&%d>◇ %s</a></td>\n</tr>\n",
	  brdname, xname, i, str_html(hdr.title, TTLEN));
      }
      else if (hdr.xname[0] == 'F')	/* 卷宗 */
      {
	fprintf(fpw, "  <td><a href=/gem?%s&%s>◆ %s</a></td>\n</tr>\n",
	  brdname, hdr.xname, str_html(hdr.title, TTLEN));
      }
      else				/* 其他類別就不秀了 */
      {
	fputs("  <td>◇ 其他資料</td>\n</tr>\n", fpw);
      }

      i++;
    }
    close(fd);
  }

  fputs("</table>\n", fpw);

  gemlist_neck(fpw, brdname);
  return HS_END;
}


  /* --------------------------------------------------- */
  /* 閱讀看板文章					 */
  /* --------------------------------------------------- */

static void
more_neck(fpw, pos, total, brdname, xname)
  FILE *fpw;
  int pos, total;
  char *brdname, *xname;
{
  fputs("<br>\n"
    "<table cellspacing=0 cellpadding=1 border=0 width=760>\n"
    "<tr bgcolor=" HCOLOR_NECK ">\n  <td width=20%", fpw);

  if (pos > 1)
  {
    fputs(" align=center><a href=?", fpw);
    if (brdname)
      fprintf(fpw, "%s&", brdname);
    if (xname)
      fprintf(fpw, "%s&", xname);
    fprintf(fpw, "%d>上一篇</a", pos - 1);
  }
  fputs("></td>\n  <td width=20%", fpw);

  if (pos < total)
  {
    fputs(" align=center><a href=?", fpw);
    if (brdname)
      fprintf(fpw, "%s&", brdname);
    if (xname)
      fprintf(fpw, "%s&", xname);
    fprintf(fpw, "%d>下一篇</a", pos + 1);
  }

  if (xname)
    fprintf(fpw, "></td>\n  <td width=60%% align=center><a href=/gem?%s&%s>回到卷宗</a", brdname, xname);
  else if (brdname)
  {
    fprintf(fpw, "></td>\n  <td width=20%% align=center><a href=/bmost?%s&%d target=_blank>同標題</a></td>\n"
      "  <td width=20%% align=center><a href=/dopost?%s target=_blank>發表文章</a></td>\n"
      "  <td width=20%% align=center><a href=/brd?%s>文章列表</a",
      brdname, pos,
      brdname, brdname);
  }
  else
    fputs("></td>\n  <td width=60% align=center><a href=/mbox>信箱列表</a", fpw);

  fputs("></td>\n</tr></table><br>\n", fpw);
}


static int
more_item(fpw, folder, pos, brdname)
  FILE *fpw;
  char *folder;
  int pos;
  char *brdname;
{
  int fd, total;
  HDR hdr;
  char fpath[64];

  total = rec_num(folder, sizeof(HDR));
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    int find;

    lseek(fd, (off_t) (sizeof(HDR) * (pos - 1)), SEEK_SET);
    find = read(fd, &hdr, sizeof(HDR)) == sizeof(HDR);
    close(fd);

    if (find)
    {
      more_neck(fpw, pos, total, brdname, NULL);

#ifdef HAVE_REFUSEMARK
      if (!(hdr.xmode & POST_RESTRICT))
      {
#endif
	hdr_fpath(fpath, folder, &hdr);
	out_article(fpw, fpath);
#ifdef HAVE_REFUSEMARK
      }
      else
	out_mesg(fpw, "這是加密的文章，您無法閱\讀");
#endif

      more_neck(fpw, pos, total, brdname, NULL);
      return HS_END;
    }
  }

  return HS_ERR_MORE;
}


static int
cmd_brdmore(ap)
  Agent *ap;
{
  int pos;
  char folder[64], *brdname, *number;
  FILE *fpw = out_head(ap, "閱\讀看板文章");

  if (!arg_analyze(2, '?', ap->urlp, &brdname, &number, NULL, NULL))
    return HS_ERROR;

  if (!allow_brdname(ap, brdname))
    return HS_ERR_BOARD;

  brd_fpath(folder, brdname, FN_DIR);

  if ((pos = atoi(number)) <= 0)
    pos = 1;

  return more_item(fpw, folder, pos, brdname);
}


  /* --------------------------------------------------- */
  /* 閱讀信箱文章					 */
  /* --------------------------------------------------- */

static int
cmd_mboxmore(ap)
  Agent *ap;
{
  int pos;
  char folder[64], *number;
  FILE *fpw = out_head(ap, "閱\讀信箱文章");

  if (!arg_analyze(1, '?', ap->urlp, &number, NULL, NULL, NULL))
    return HS_ERROR;

  if (!acct_fetch(ap))
    return HS_ERR_LOGIN;

  usr_fpath(folder, ap->userid, FN_DIR);

  if ((pos = atoi(number)) <= 0)
    pos = 1;

  return more_item(fpw, folder, pos, NULL);
}


  /* --------------------------------------------------- */
  /* 閱讀看板同標題文章					 */
  /* --------------------------------------------------- */

static void
do_brdmost(fpw, folder, title)
  FILE *fpw;
  char *folder, *title;
{
  int fd;
  char fpath[64];
  FILE *fp;
  HDR hdr;

  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    while (read(fd, &hdr, sizeof(HDR)) == sizeof(HDR))
    {
#ifdef HAVE_REFUSEMARK
      if (hdr.xmode & POST_RESTRICT)
	continue;
#endif

      if (!strcmp(str_ttl(hdr.title), title))
      {
	hdr_fpath(fpath, folder, &hdr);
	if (fp = fopen(fpath, "r"))
	{
	  txt2htm(fpw, fp);
	  fclose(fp);
	}
      }
    }
    close(fd);
  }
}


static void
brdmost_neck(fpw)
  FILE *fpw;
{
  fputs("<br>\n"
    "<table cellspacing=0 cellpadding=1 border=0 width=760>\n"
    "<tr bgcolor=" HCOLOR_NECK ">\n"
    "  <td align=center>同標題閱\讀</td>\n"
    "</tr></table><br>\n", fpw);
}


static int
cmd_brdmost(ap)
  Agent *ap;
{
  int fd, pos;
  char folder[64], *brdname, *number;
  HDR hdr;
  FILE *fpw = out_head(ap, "閱\讀看板同標題文章");

  if (!arg_analyze(2, '?', ap->urlp, &brdname, &number, NULL, NULL))
    return HS_ERROR;

  if (!allow_brdname(ap, brdname))
    return HS_ERR_BOARD;

  brd_fpath(folder, brdname, FN_DIR);

  if ((pos = atoi(number)) <= 0)
    pos = 1;

  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    int find;

    lseek(fd, (off_t) (sizeof(HDR) * (pos - 1)), SEEK_SET);
    find = read(fd, &hdr, sizeof(HDR)) == sizeof(HDR);
    close(fd);

    if (find)
    {
      brdmost_neck(fpw);
      out_style(fpw);
      do_brdmost(fpw, folder, str_ttl(hdr.title));
      brdmost_neck(fpw);
      return HS_END;
    }
  }
  return HS_ERR_MORE;
}


  /* --------------------------------------------------- */
  /* 閱讀精華區文章					 */
  /* --------------------------------------------------- */

static int
cmd_gemmore(ap)
  Agent *ap;
{
  int fd, pos, total;
  char *brdname, *xname, *number, folder[64];
  HDR hdr;
  FILE *fpw = out_head(ap, "閱\讀精華區文章");

  if (!arg_analyze(3, '?', ap->urlp, &brdname, &xname, &number, NULL))
    return HS_ERROR;

  if (*xname != 'F' && strlen(xname) != 8 && strcmp(xname, FN_DIR))
    return HS_ERROR;

  if (!allow_brdname(ap, brdname))
    return HS_ERR_BOARD;

  if (*xname == '.')
    gem_fpath(folder, brdname, FN_DIR);
  else /* if (*xname == 'F') */
    sprintf(folder, "gem/brd/%s/%c/%s", brdname, xname[7], xname);

  if ((pos = atoi(number)) <= 0)
    pos = 1;
  total = rec_num(folder, sizeof(HDR));

  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    int find;

    lseek(fd, (off_t) (sizeof(HDR) * (pos - 1)), SEEK_SET);
    find = read(fd, &hdr, sizeof(HDR)) == sizeof(HDR);
    close(fd);

    if (find)
    {
      more_neck(fpw, pos, total, brdname, xname);
      if (hdr.xname[0] == 'A')
      {
	if (!(hdr.xmode & GEM_RESTRICT))
	{
	  sprintf(folder, "gem/brd/%s/%c/%s", brdname, hdr.xname[7], hdr.xname);
	  out_article(fpw, folder);
	}
	else
	  out_mesg(fpw, "此為保密精華區，您無法閱\讀");
      }
      else
	out_mesg(fpw, "這是卷宗或唯讀資料，您必須由精華區列表來讀取");
      more_neck(fpw, pos, total, brdname, xname);
      return HS_END;
    }
  }

  return HS_ERR_MORE;
}


  /* --------------------------------------------------- */
  /* 發表文章						 */
  /* --------------------------------------------------- */

static int
cmd_dopost(ap)
  Agent *ap;
{
  char *brdname;
  FILE *fpw = out_head(ap, "發表文章");

  if (!arg_analyze(1, '?', ap->urlp, &brdname, NULL, NULL, NULL))
    return HS_ERROR;

  if (!(ben_perm(ap, brdname) & BRD_W_BIT))
    return HS_ERR_BOARD;

  fputs("<form method=post onsubmit=\"if(t.value.length==0 || c.value.length==0) {alert('標題、內容均不可為空白'); return false;} return true;\">\n"
    "  <input type=hidden name=dopost>\n"
    "  請輸入標題：<br>\n", fpw);
  fprintf(fpw,
    "  <input type=hidden name=b value=%s>\n"
    "  <input type=text name=t size=%d maxlength=%d><br><br>\n"
    "  請輸入內容：<br>\n"
    "  <textarea name=c rows=10 cols=%d></textarea><br><br>\n"
    "  <input type=hidden name=end>\r\n"
    "  <input type=submit value=送出文章> "
    "  <input type=reset value=重新填寫>"
    "</form>\n",
    brdname,
    TTLEN, TTLEN,
    SCR_WIDTH);

  return HS_END;
}


  /* --------------------------------------------------- */
  /* 發送信件						 */
  /* --------------------------------------------------- */

static int
cmd_domail(ap)
  Agent *ap;
{
  char *userid;
  FILE *fpw = out_head(ap, "發送信件");

  if (!acct_fetch(ap))
    return HS_ERR_LOGIN;

  if (!arg_analyze(1, '?', ap->urlp, &userid, NULL, NULL, NULL))
    userid = "";

  fputs("<form method=post onsubmit=\"if(u.value.length==0 || t.value.length==0 || c.value.length==0) {alert('收信人、標題、內容均不可為空白'); return false;} return true;\">\n"
    "  <input type=hidden name=domail>\n"
    "  請輸入收信人ＩＤ：<br>\n", fpw);
  fprintf(fpw,
    "  <input type=text name=u size=%d maxlength=%d value=%s><br><br>\n"
    "  請輸入標題：<br>\n"
    "  <input type=text name=t size=%d maxlength=%d><br><br>\n"
    "  請輸入內容：<br>\n"
    "  <textarea name=c rows=10 cols=%d></textarea><br><br>\n"
    "  <input type=hidden name=end>\r\n"
    "  <input type=submit value=送出信件> "
    "  <input type=reset value=重新填寫>"
    "</form>\n",
    IDLEN, IDLEN, userid,
    TTLEN, TTLEN,
    SCR_WIDTH);

  return HS_END;
}


  /* --------------------------------------------------- */
  /* 標記/刪除 文章					 */
  /* --------------------------------------------------- */

static void outgo_post();

static void
move_post(userid, hdr, folder, by_bm)	/* 將 hdr 從 folder 搬到別的板 */
  char *userid;
  HDR *hdr;
  char *folder;
  int by_bm;
{
  HDR post;
  int xmode;
  char fpath[64], fnew[64], *board;

  xmode = hdr->xmode;
  hdr_fpath(fpath, folder, hdr);

  if (!(xmode & POST_BOTTOM))	/* 置底文被砍不用 move_post */
  {
#ifdef HAVE_REFUSEMARK
    board = by_bm && !(xmode & POST_RESTRICT) ? BN_DELETED : BN_JUNK;	/* 加密文章丟去 junk */
#else
    board = by_bm ? BN_DELETED : BN_JUNK;
#endif

    brd_fpath(fnew, board, FN_DIR);
    hdr_stamp(fnew, HDR_LINK | 'A', &post, fpath);

    /* 直接複製 trailing data：owner(含)以下所有欄位 */
    memcpy(post.owner, hdr->owner, sizeof(HDR) -
      (sizeof(post.chrono) + sizeof(post.xmode) + sizeof(post.xid) + sizeof(post.xname)));

    if (by_bm)
      sprintf(post.title, "%-13s%.59s", userid, hdr->title);

    rec_bot(fnew, &post, sizeof(HDR));
    brd_get(board)->btime = -1;
  }
  unlink(fpath);
}


static int
op_shell(op_func, ap, title, msg)
  int (*op_func) (Agent *, char *, char *);
  Agent *ap;
  char *title, *msg;
{
  int ret = op_func(ap, title, msg);
  if (ret != HS_END)
    out_head(ap, title + 1);
  return ret;
}


static int
post_op(ap, title, msg)
  Agent *ap;
  char *title, *msg;
{
  int pos;
  time_t chrono;
  HDR hdr;
  usint bits;
  char *brdname, *number, *stamp, folder[64];
  FILE *fpw;

  if (!arg_analyze(3, '?', ap->urlp, &brdname, &number, &stamp, NULL) ||
    (pos = atoi(number) - 1) < 0 ||
    (chrono = atoi(stamp)) < 0)
    return HS_ERROR;

  if (bits = ben_perm(ap, brdname))
  {
    brd_fpath(folder, brdname, FN_DIR);
    if (rec_get(folder, &hdr, sizeof(HDR), pos) ||
      (hdr.chrono != chrono))
      return HS_ERR_MORE;

    if (*title == 'm')
    {
      if (!(bits & BRD_X_BIT))
	return HS_ERR_PERM;

      hdr.xmode ^= POST_MARKED;
      rec_put(folder, &hdr, sizeof(HDR), pos, NULL);
    }
    else /* if (*title == 'd') */
    {
      if (!(hdr.xmode & POST_MARKED))
      {
	if (!(bits & BRD_X_BIT) &&
	  (!(bits & BRD_W_BIT) || strcmp(ap->userid, hdr.owner)))
	  return HS_ERR_PERM;

	rec_del(folder, sizeof(HDR), pos, NULL);
	move_post(ap->userid, &hdr, folder, bits & BRD_X_BIT);
	brd_get(brdname)->btime = -1;
	/* 連線砍信 */
	if ((hdr.xmode & POST_OUTGO) &&		/* 外轉信件 */
	  hdr.chrono > (time(0) - 7 * 86400))	/* 7 天之內有效 */
	{
	  hdr.chrono = -1;
	  outgo_post(&hdr, brdname);
	}
      }
    }

    sprintf(folder, "/brd?%s&%d", brdname, (pos - 1) / HTML_TALL * HTML_TALL + 1);
    fpw = out_http(ap, HS_OK | HS_REFRESH, folder);
    out_title(fpw, title + 1);
    out_mesg(fpw, msg);
    fprintf(fpw, "<a href=%s>回文章列表</a>\n", folder);

    return HS_END;
  }
  return HS_ERR_BOARD;
}


static int
cmd_markpost(ap)
  Agent *ap;
{
  return op_shell(post_op, ap, "m標記文章", "已執行(取消)標記指令");
}


static int
cmd_delpost(ap)
  Agent *ap;
{
  return op_shell(post_op, ap, "d刪除文章", "已執行刪除指令，若未刪除表示此文章被標記了");
}


static int
cmd_predelpost(ap)
  Agent *ap;
{
  char *brdname, *number, *stamp;
  FILE *fpw = out_head(ap, "確認刪除文章");

  if (!arg_analyze(3, '?', ap->urlp, &brdname, &number, &stamp, NULL))
    return HS_ERROR;

  out_mesg(fpw, "若確定要刪除此篇文章，請再次點選以下連結；若要取消刪除，請按 [上一頁]");
  fprintf(fpw, "<a href=/delpost?%s&%s&%s>刪除 [%s] 板第 %s 篇文章</a><br>\n",
    brdname, number, stamp, brdname, number);

  return HS_END;
}


  /* --------------------------------------------------- */
  /* 標記/刪除 信件					 */
  /* --------------------------------------------------- */

static int
mail_op(ap, title, msg)
  Agent *ap;
  char *title, *msg;
{
  int pos;
  time_t chrono;
  HDR hdr;
  char *number, *stamp, folder[64], fpath[64];

  if (!arg_analyze(2, '?', ap->urlp, &number, &stamp, NULL, NULL) ||
    (pos = atoi(number) - 1) < 0 ||
    (chrono = atoi(stamp)) < 0)
    return HS_ERROR;

  if (acct_fetch(ap))
  {
    usr_fpath(folder, ap->userid, FN_DIR);
    if (rec_get(folder, &hdr, sizeof(HDR), pos) ||
      (hdr.chrono != chrono))
      return HS_ERR_MAIL;

    if (*title == 'm')
    {
      hdr.xmode ^= POST_MARKED;
      rec_put(folder, &hdr, sizeof(HDR), pos, NULL);
    }
    else /* if (*title == 'd') */
    {
      if (!(hdr.xmode & POST_MARKED))
      {
	rec_del(folder, sizeof(HDR), pos, NULL);
	hdr_fpath(fpath, folder, &hdr);
	unlink(fpath);
      }
    }

    sprintf(folder, "/mbox?%d", (pos - 1) / HTML_TALL * HTML_TALL + 1);
    out_title(out_http(ap, HS_OK | HS_REFRESH, folder), title + 1);
    out_mesg(ap->fpw, msg);
    fprintf(ap->fpw, "<a href=%s>回信箱列表</a>\n", folder);
    return HS_END;
  }
  return HS_ERR_LOGIN;
}


static int
cmd_markmail(ap)
  Agent *ap;
{
  return op_shell(mail_op, ap, "m標記信件", "已執行(取消)標記指令");
}


static int
cmd_delmail(ap)
  Agent *ap;
{
  return op_shell(mail_op, ap, "d刪除信件", "已執行刪除指令，若未刪除表示此信件被標記了");
}


static int
cmd_predelmail(ap)
  Agent *ap;
{
  char *number, *stamp;
  FILE *fpw = out_head(ap, "確認刪除信件");

  if (!arg_analyze(2, '?', ap->urlp, &number, &stamp, NULL, NULL))
    return HS_ERROR;

  out_mesg(fpw, "若確定要刪除此篇信件，請再次點選以下連結；若要取消刪除，請按 [上一頁]");
  fprintf(fpw, "<a href=/delmail?%s&%s>刪除信箱第 %s 篇信件</a><br>\n",
    number, stamp, number);

  return HS_END;
}


  /* --------------------------------------------------- */
  /* 查詢使用者						 */
  /* --------------------------------------------------- */

static int
cmd_query(ap)
  Agent *ap;
{
  int fd;
  ACCT acct;
  char fpath[64], *userid;
  FILE *fpw = out_head(ap, "查詢使用者");

  if (!arg_analyze(1, '?', ap->urlp, &userid, NULL, NULL, NULL))
    return HS_ERROR;

  if (!allow_userid(ap, userid))
    return HS_ERR_USER;

  usr_fpath(fpath, userid, FN_ACCT);
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    read(fd, &acct, sizeof(ACCT));
    close(fd);

    fprintf(fpw, "<pre>\n"
      "<a target=_blank href=domail?%s>%s (%s)</a><br>\n"
      "%s通過認證，共上站 %d 次，發表過 %d 篇文章<br>\n"
      "最近(%s)從[%s]上站<br>\n"
      "</pre>\n",
      acct.userid, acct.userid,
      str_html(acct.username, UNLEN),
      (acct.userlevel & PERM_VALID) ? "已" : "未", acct.numlogins, acct.numposts,
      Btime(&(acct.lastlogin)), acct.lasthost);

    usr_fpath(fpath, acct.userid, FN_PLANS);
    out_article(fpw, fpath);
    return HS_END;
  }

  return HS_ERR_USER;
}


  /* --------------------------------------------------- */
  /* 顯示圖片						 */
  /* --------------------------------------------------- */

static int
valid_path(str)
  char *str;
{
  int ch;

  if (!*str)
    return 0;

  while (ch = *str++)
  {
    if (!is_alnum(ch) && ch != '.' && ch != '-' && ch != '_')
      return 0;
  }
  return 1;
}


static int
cmd_image(ap)
  Agent *ap;
{
  FILE *fpw;
  struct stat st;
  char *fname, *ptr, fpath[64];

  if (!arg_analyze(1, '?', ap->urlp, &fname, NULL, NULL, NULL))
    return HS_NOTFOUND;

  if (!valid_path(fname) || !(ptr = strchr(fname, '.')))
    return HS_NOTFOUND;

  /* 支援格式 */
  if (!str_cmp(ptr, ".html"))
    ptr = "text/html";
  else if (!str_cmp(ptr, ".gif"))
    ptr = "image/gif";
  else if (!str_cmp(ptr, ".jpg"))
    ptr = "image/jpeg";
  else if (!str_cmp(ptr, ".css"))
    ptr = "text/css";
  else if (!str_cmp(ptr, ".png"))
    ptr = "image/png";
  else
    return HS_NOTFOUND;

  sprintf(fpath, "run/html/%.40s", fname);
  if (stat(fpath, &st))
    return HS_NOTFOUND;

  if (ap->modified[0] && !strcmp(Gtime(&st.st_mtime), ap->modified))	/* 沒有變更不需要傳輸 */
    return HS_NOTMOIDIFY;

  fpw = out_http(ap, HS_OK, ptr);
  fprintf(fpw, "Content-Length: %ld\r\n", st.st_size);
  fprintf(fpw, "Last-Modified: %s\r\n\r\n", Gtime(&st.st_mtime));
  f_suck(fpw, fpath);

  return HS_OK;
}


  /* --------------------------------------------------- */
  /* RSS						 */
  /* --------------------------------------------------- */

static int
cmd_rss(ap)
  Agent *ap;
{
  time_t blast;
  char folder[64], *brdname, *ptr;
  BRD *brd;
  HDR hdr;
  FILE *fpw;
  int fd;

  if (!arg_analyze(1, '?', ap->urlp, &brdname, NULL, NULL, NULL))
    return HS_BADREQUEST;

  if (!(brd = brd_get(brdname)))
    return HS_NOTFOUND;

  if (brd->readlevel)		/* 只有公開板才提供 rss */
    return HS_FORBIDDEN;

  blast = brd->blast;

  if (ap->modified[0] && !strcmp(Gtime(&blast), ap->modified))	/* 沒有變更不需要傳輸 */
    return HS_NOTMOIDIFY;

  fpw = out_http(ap, HS_OK, "application/xml");
  fprintf(fpw, "Last-Modified: %s\r\n\r\n", Gtime(&blast));

  /* xml header */
  fputs("<?xml version=\"1.0\" encoding=\"" MYCHARSET "\" ?>\n"
    "<rss version=\"2.0\">\n"
    "<channel>\n", fpw);
  ptr = Gtime(&blast);
  ptr[4] = '\0';
  fprintf(fpw, "<title>" BBSNAME "-%s板</title>\n"
#if BHTTP_PORT == 80
    "<link>http://" MYHOSTNAME "/brd?%s</link>\n"
#else
    "<link>http://" MYHOSTNAME ":%d/brd?%s</link>\n"
#endif
    "<description>%s</description>\n"
    "<language>zh-tw</language>\n"
    "<lastBuildDate>%s ",
    brdname,
#if BHTTP_PORT != 80
    BHTTP_PORT,
#endif
    brdname,
    str_html(brd->title, TTLEN), ptr);
  ptr += 5;
  if (*ptr == '0')
    ptr++;
  fprintf(fpw, "%s</lastBuildDate>\n<image>\n"
    "<title>" BBSNAME "</title>"
#if BHTTP_PORT == 80
    "<link>http://" MYHOSTNAME "</link>\n"
    "<url>http://" MYHOSTNAME "/img?rss.gif</url>\n"
#else
    "<link>http://" MYHOSTNAME ":%d</link>\n"
    "<url>http://" MYHOSTNAME ":%d/img?rss.gif</url>\n"
#endif
    "</image>\n",
#if BHTTP_PORT == 80
    ptr);
#else
    ptr, BHTTP_PORT, BHTTP_PORT);
#endif

  /* rss item */
  brd_fpath(folder, brdname, FN_DIR);
  if ((fd = open(folder, O_RDONLY)) >= 0)
  {
    int fsize;
    struct stat st;

    if (!fstat(fd, &st) && (fsize = st.st_size) >= sizeof(HDR))
    {
      int i, end;

      /* 只列出最後二十篇 */
      if (fsize > 20 * sizeof(HDR))
	end = fsize - 20 * sizeof(HDR);
      else
	end = 0;
      i = fsize / sizeof(HDR);

      while ((fsize -= sizeof(HDR)) >= end)
      {
	lseek(fd, fsize, SEEK_SET);
	read(fd, &hdr, sizeof(HDR));

	ptr = Gtime(&hdr.chrono);
	ptr[4] = '\0';
	fprintf(fpw, "<!-- %d --><item><title>%s</title>"
#if BHTTP_PORT == 80
	  "<link>http://" MYHOSTNAME "/bmore?%s&amp;%d</link>"
#else
	  "<link>http://" MYHOSTNAME ":%d/bmore?%s&amp;%d</link>"
#endif
	  "<author>%s</author>"
	  "<pubDate>%s ",
	  hdr.chrono, str_html(hdr.title, TTLEN),
#if BHTTP_PORT != 80
	  BHTTP_PORT,
#endif
	  brdname, i,
	  hdr.owner,
	  ptr);
	ptr += 5;
	if (*ptr == '0')
	  ptr++;
	fprintf(fpw, "%s</pubDate></item>\n", ptr);

	i--;
      }
    }
    close(fd);
  }
  fputs("</channel>\n</rss>\n", fpw);

  return HS_OK;
}


  /* --------------------------------------------------- */
  /* Robot Exclusion					 */
  /* --------------------------------------------------- */

#ifdef ROBOT_EXCLUSION
static int
cmd_robots(ap)
  Agent *ap;
{
  FILE *fpw = out_http(ap, HS_OK, NULL);

  fprintf(fpw, "Content-Length: 28\r\n");	/* robots.txt 的長度 */
  fprintf(fpw, "Last-Modified: Sat, 01 Jan 2000 00:02:21 GMT\r\n\r\n");	/* 隨便給個時間 */

  fprintf(fpw, "User-agent: *\r\nDisallow: /\r\n");
      
  return HS_OK;        
}
#endif


  /* --------------------------------------------------- */
  /* 首頁						 */
  /* --------------------------------------------------- */

static void
mainpage_neck(fpw, userid, logined)
  FILE *fpw;
  char *userid;
  int logined;
{
  fprintf(fpw, "<br>\n"
    "<table cellspacing=0 cellpadding=1 border=0 width=760>\n"
    "<tr bgcolor=" HCOLOR_NECK ">\n"
    "  <td width=100%% align=center>%s%s歡迎光臨</td>\n"
    "</tr></table><br>\n",
    logined ? userid : "",
    logined ? "，" : "");
}


static int
cmd_mainpage(ap)
  Agent *ap;
{
  int logined;
  FILE *fpw = out_head(ap, "");

  logined = acct_fetch(ap);
  mainpage_neck(fpw, ap->userid, logined);

  out_film(fpw, (ap->uptime % 3) + FILM_OPENING0);
  /* 登入 */
  if (!logined)
  {
    /* 開頭畫面 */
    fputs("<form method=post>\n"
      "  <input type=hidden name=login>\n"
      "  帳號 <input type=text name=u size=12 maxlength=12> "
      "  密碼 <input type=password name=p size=12 maxlength=8> "
      "  <input type=submit value=登入> "
      "  <input type=reset value=清除>"
      "</form>\n", fpw);
  }

  mainpage_neck(fpw, ap->userid, logined);
  return HS_END;
}


  /* --------------------------------------------------- */
  /* 指令集						 */
  /* --------------------------------------------------- */

static Command cmd_table_get[] =
{
  cmd_userlist,    "usrlist",   7,
  cmd_boardlist,   "brdlist",   7,
  cmd_favorlist,   "fvrlist",   7,
  cmd_class,       "class",     5,

  cmd_postlist,    "brd",       3,
  cmd_gemlist,     "gem",       3,
  cmd_mboxlist,    "mbox",      4,

  cmd_brdmore,     "bmore",     5,
  cmd_brdmost,     "bmost",     5,
  cmd_gemmore,     "gmore",     5,
  cmd_mboxmore,    "mmore",     5,

  cmd_dopost,      "dopost",    6,
  cmd_domail,      "domail",    6,

  cmd_delpost,     "delpost",   7,
  cmd_predelpost,  "dpost",     5,
  cmd_delmail,     "delmail",   7,
  cmd_predelmail,  "dmail",     5,
  cmd_markpost,    "mpost",     5,
  cmd_markmail,    "mmail",     5,

  cmd_query,       "query",     5,

  cmd_image,       "img",       3,
  cmd_rss,         "rss",       3,

#ifdef ROBOT_EXCLUSION
  cmd_robots,      "robots.txt",9,
#endif

  cmd_mainpage,    "\0",        1,

  NULL,            NULL,        0
};


/* ----------------------------------------------------- */
/* command dispatch (POST)				 */
/* ----------------------------------------------------- */

static char *
getfromhost(pip)
  void *pip;
{
  struct hostent *hp;

  if (hp = gethostbyaddr((char *)pip, 4, AF_INET))
    return hp->h_name;
  return inet_ntoa(*(struct in_addr *) pip);
}


  /* --------------------------------------------------- */
  /* 使用者登入						 */
  /* --------------------------------------------------- */

static int
cmd_login(ap)
  Agent *ap;
{
  char *userid, *passwd;
  char fpath[64];
  ACCT acct;

  /* u=userid&p=passwd */
  if (!arg_analyze(2, 0, ap->urlp, &userid, &passwd, NULL, NULL))
    return HS_ERROR;

  userid += 2;			/* skip "u=" */
  passwd += 2;			/* skip "p=" */

  if (*userid && *passwd && strlen(userid) <= IDLEN && strlen(passwd) <= PSWDLEN)
  {
    usr_fpath(fpath, userid, FN_ACCT);
    if (!rec_get(fpath, &acct, sizeof(ACCT), 0) &&
      !(acct.userlevel & (PERM_DENYLOGIN | PERM_PURGE)) &&
      !chkpasswd(acct.passwd, passwd))	/* 登入成功 */
    {
      /* itoc.040308: 產生 Cookie */
      sprintf(ap->cookie, "%s&p=%s", userid, acct.passwd);
      ap->setcookie = 1;
    }
  }

  return cmd_mainpage(ap);
}


  /* --------------------------------------------------- */
  /* 發表新文章						 */
  /* --------------------------------------------------- */

static void
outgo_post(hdr, board)
  HDR *hdr;
  char *board;
{
  bntp_t bntp;

  memset(&bntp, 0, sizeof(bntp_t));
  bntp.chrono = hdr->chrono;
  strcpy(bntp.board, board);
  strcpy(bntp.xname, hdr->xname);
  strcpy(bntp.owner, hdr->owner);
  strcpy(bntp.nick, hdr->nick);
  strcpy(bntp.title, hdr->title);
  rec_add("innd/out.bntp", &bntp, sizeof(bntp_t));
}


static int
cmd_addpost(ap)
  Agent *ap;
{
  char *brdname, *title, *content, *end;
  char folder[64], fpath[64];
  HDR hdr;
  BRD *brd;
  FILE *fp;
  FILE *fpw = out_head(ap, "文章發表");

  if (!acct_fetch(ap))
    return HS_ERR_LOGIN;

  /* b=brdname&t=title&c=content&end= */
  if (arg_analyze(4, 0, ap->urlp, &brdname, &title, &content, &end))
  {
    brdname += 2;		/* skip "b=" */
    title += 2;			/* skip "t=" */
    content += 2;		/* skip "c=" */

    if (*brdname && *title && *content)
    {
      if ((brd = brd_get(brdname)) &&
	(Ben_Perm(brd, ap->userno, ap->userid, ap->userlevel) & BRD_W_BIT))
      {
	brd_fpath(folder, brdname, FN_DIR);

	fp = fdopen(hdr_stamp(folder, 'A', &hdr, fpath), "w");
	fprintf(fp, "%s %s (%s) %s %s\n",
	  STR_AUTHOR1, ap->userid, ap->username,
	  STR_POST2, brdname);
	str_ncpy(hdr.title, title, sizeof(hdr.title));
	fprintf(fp, "標題: %s\n時間: %s\n\n", hdr.title, Now());
	fprintf(fp, "%s\n", content);
	fprintf(fp, EDIT_BANNER, ap->userid, getfromhost(&(ap->ip_addr)));
	fclose(fp);

	hdr.xmode = (brd->battr & BRD_NOTRAN) ? 0 : POST_OUTGO;
	strcpy(hdr.owner, ap->userid);
	strcpy(hdr.nick, ap->username);
	rec_bot(folder, &hdr, sizeof(HDR));

	brd->btime = -1;
	if (hdr.xmode & POST_OUTGO)
	  outgo_post(&hdr, brdname);

	out_reload(fpw, "您的文章發表成功\");
	return HS_OK;
      }
      return HS_ERR_BOARD;
    }
  }

  out_reload(fpw, "您的文章發表失敗");
  return HS_OK;
}


  /* --------------------------------------------------- */
  /* 發送新信件						 */
  /* --------------------------------------------------- */

static int
cmd_addmail(ap)
  Agent *ap;
{
  char *userid, *title, *content, *end;
  char folder[64], fpath[64];
  HDR hdr;
  FILE *fp;
  FILE *fpw = out_head(ap, "信件發送");

  /* u=userid&t=title&c=content&end= */
  if (arg_analyze(4, 0, ap->urlp, &userid, &title, &content, &end))
  {
    userid += 2;		/* skip "u=" */
    title += 2;			/* skip "t=" */
    content += 2;		/* skip "c=" */

    if (*userid && *title && *content && allow_userid(ap, userid))
    {
      usr_fpath(fpath, userid, FN_ACCT);
      if (dashf(fpath) && acct_fetch(ap))
      {
	usr_fpath(folder, userid, FN_DIR);

	fp = fdopen(hdr_stamp(folder, 0, &hdr, fpath), "w");
	fprintf(fp, "%s %s (%s)\n",
	  STR_AUTHOR1, ap->userid, ap->username);
	str_ncpy(hdr.title, title, sizeof(hdr.title));
	fprintf(fp, "標題: %s\n時間: %s\n\n", hdr.title, Now());
	fprintf(fp, "%s\n", content);
	fprintf(fp, EDIT_BANNER, ap->userid, getfromhost(&(ap->ip_addr)));
	fclose(fp);

	strcpy(hdr.owner, ap->userid);
	strcpy(hdr.nick, ap->username);
	rec_add(folder, &hdr, sizeof(HDR));

	out_reload(fpw, "您的信件發送成功\");
	return HS_OK;
      }
    }
  }

  out_reload(fpw, "您的信件發送失敗，也許\是因為您尚未登入或是查無此使用者");
  return HS_OK;
}


  /* --------------------------------------------------- */
  /* 指令集						 */
  /* --------------------------------------------------- */

static Command cmd_table_post[] =
{
  cmd_login,    "login=&",  7,
  cmd_addpost,  "dopost=&", 8,
  cmd_addmail,  "domail=&", 8,

  NULL,         NULL,       0
};


/* ----------------------------------------------------- */
/* close a connection & release its resource		 */
/* ----------------------------------------------------- */

static void
agent_fire(ap)
  Agent *ap;
{
  int csock;

  csock = ap->sock;
  if (csock > 0)
  {
    fcntl(csock, F_SETFL, M_NONBLOCK);
    shutdown(csock, 2);

    /* fclose(ap->fpw); */
    close(csock);
  }

  if (ap->data)
    free(ap->data);
}


/* ----------------------------------------------------- */
/* receive request from client				 */
/* ----------------------------------------------------- */

static int		/* >=0:mode -1:結束 */
do_cmd(ap, str, end, mode)
  Agent *ap;
  uschar *str, *end;		/* command line 的開頭和結尾 */
  int mode;
{
  int code;
  char *ptr;

  if (!(mode & (AM_GET | AM_POST)))
  {
    if (!str_ncmp(str, "GET ", 4))		/* str 格式為 GET /index.htm HTTP/1.0 */
    {
      mode ^= AM_GET;
      str += 4;

      if (*str != '/')
      {
	out_error(ap, HS_BADREQUEST);
	return -1;
      }

      if (ptr = strchr(str, ' '))
      {
	*ptr = '\0';
	str_ncpy(ap->url, str + 1, sizeof(ap->url));
      }
      else
      {
	*ap->url = '\0';
      }
    }
    else if (!str_ncmp(str, "POST ", 5))	/* str 格式為 POST /dopost?sysop HTTP/1.0 */
    {
      mode ^= AM_POST;
    }
  }
  else
  {
    if (*str)		/* 不是空行：檔頭 */
    {
      /* 分析 Cookie */
      if (!str_ncmp(str, "Cookie: user=", 13))
      {
	str_ncpy(ap->cookie, str + 13, LEN_COOKIE);
      }
      else if (!str_ncmp(str, "Cookie: ", 8))	/* waynesan.081018: 修正多 cookie 的狀況 */
      {
	char *user;
	if (user = strstr(str, "user="))
	  str_ncpy(ap->cookie, user + 5, LEN_COOKIE);
      }

      /* 分析 If-Modified-Since */
      if ((mode & AM_GET) && !str_ncmp(str, "If-Modified-Since: ", 19))	/* str 格式為 If-Modified-Since: Sat, 29 Oct 1994 19:43:31 GMT */
	str_ncpy(ap->modified, str + 19, sizeof(ap->modified));
    }
    else		/* 空行 */
    {
      Command *cmd;
      char *url;

      if (mode & AM_GET)
      {
	cmd = cmd_table_get;
	url = ap->url;
      }
      else /* if (mode & AM_POST) */
      {
	cmd = cmd_table_post;
	/* 在 AM_POST 時，空行的下一行是 POST 的內容 */
	for (url = end + 1; *url == '\r' || *url == '\n'; url++)	/* 找下一行 */
	  ;
      }

      for (; ptr = cmd->cmd; cmd++)
      {
	if (!str_ncmp(url, ptr, cmd->len))
	  break;
      }

      /* waynesan.081018: 如果在 command_table 裡面找不到，那麼送 404 Not Found */
      if (!ptr)
      {
	out_error(ap, HS_NOTFOUND);
	return -1;
      }

      ap->urlp = url + cmd->len;

      code = (*cmd->func) (ap);
      if (code != HS_OK)
      {
	if (code != HS_END)
	  out_error(ap, code);
	if (code < HS_OK)
	  out_tail(ap->fpw);
      }
      return -1;
    }
  }

  return mode;
}


static int
agent_recv(ap)
  Agent *ap;
{
  int cc, mode, size, used;
  uschar *data, *head;

  used = ap->used;
  data = ap->data;

  if (used > 0)
  {
    /* check the available space */

    size = ap->size;
    cc = size - used;

    if (cc < TCP_RCVSIZ + 3)
    {
      if (size < MAX_DATA_SIZE)
      {
	size += TCP_RCVSIZ + (size >> 2);

	if (data = (uschar *) realloc(data, size))
	{
	  ap->data = data;
	  ap->size = size;
	}
	else
	{
#ifdef LOG_VERBOSE
	  fprintf(flog, "ERROR\trealloc: %d\n", size);
#endif
	  return 0;
	}
      }
      else
      {
#ifdef LOG_VERBOSE
	fprintf(flog, "WARN\tdata too long\n");
#endif
	return 0;
      }
    }
  }

  head = data + used;
  cc = recv(ap->sock, head, TCP_RCVSIZ, 0);

  if (cc <= 0)
  {
    cc = errno;
    if (cc != EWOULDBLOCK)
    {
#ifdef LOG_VERBOSE
      fprintf(flog, "RECV\t%s\n", strerror(cc));
#endif
      return 0;
    }

    /* would block, so leave it to do later */

    return -1;
  }

  head[cc] = '\0';
  ap->used = (used += cc);

  /* itoc.050807: recv() 一次還讀不完的，一定是 cmd_dopost 或 cmd_domail，這二者的結束都有 &end= */
  if (used >= TCP_RCVSIZ)
  {
    /* 多 -2 是因為有些瀏覽器會自動補上 \r\n */
    if (!strstr(head + cc - strlen("&end=") - 2, "&end="))	/* 還沒讀完，繼續讀 */
      return 1;
  }

  mode = 0;
  head = data;

  while (cc = *head)
  {
    if (cc == '\n')
    {
      data++;
    }
    else if (cc == '\r')
    {
      *head = '\0';

      if ((mode = do_cmd(ap, data, head, mode)) < 0)
      {
	fflush(ap->fpw);	/* do_cmd() 回傳 -1 表示結束，就 fflush 所有結果 */
	return 0;
      }

      data = head + 1;
    }
    head++;
  }

  return 0;
}


/* ----------------------------------------------------- */
/* accept a new connection				 */
/* ----------------------------------------------------- */

static int
agent_accept(ipaddr)
  unsigned int *ipaddr;
{
  int csock;
  int value;
  struct sockaddr_in csin;

  for (;;)
  {
    value = sizeof(csin);
    csock = accept(0, (struct sockaddr *) & csin, &value);
    /* if (csock > 0) */
    if (csock >= 0)		/* Thor.000126: more proper */
      break;

    csock = errno;
    if (csock != EINTR)
    {
#ifdef LOG_VERBOSE
      fprintf(flog, "ACCEPT\t%s\n", strerror(csock));
#endif
      return -1;
    }

    while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
  }

  value = 1;
  /* Thor.000511: 註解: don't delay send to coalesce(聯合) packets */
  setsockopt(csock, IPPROTO_TCP, TCP_NODELAY, (char *)&value, sizeof(value));

  *ipaddr = csin.sin_addr.s_addr;

  return csock;
}


/* ----------------------------------------------------- */
/* signal routines					 */
/* ----------------------------------------------------- */

#ifdef  SERVER_USAGE
static void
servo_usage()
{
  struct rusage ru;

  if (getrusage(RUSAGE_SELF, &ru))
    return;

  fprintf(flog, "\n[Server Usage]\n\n"
    " user time: %.6f\n"
    " system time: %.6f\n"
    " maximum resident set size: %lu P\n"
    " integral resident set size: %lu\n"
    " page faults not requiring physical I/O: %ld\n"
    " page faults requiring physical I/O: %ld\n"
    " swaps: %ld\n"
    " block input operations: %ld\n"
    " block output operations: %ld\n"
    " messages sent: %ld\n"
    " messages received: %ld\n"
    " signals received: %ld\n"
    " voluntary context switches: %ld\n"
    " involuntary context switches: %ld\n",

    (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000.0,
    (double)ru.ru_stime.tv_sec + (double)ru.ru_stime.tv_usec / 1000000.0,
    ru.ru_maxrss,
    ru.ru_idrss,
    ru.ru_minflt,
    ru.ru_majflt,
    ru.ru_nswap,
    ru.ru_inblock,
    ru.ru_oublock,
    ru.ru_msgsnd,
    ru.ru_msgrcv,
    ru.ru_nsignals,
    ru.ru_nvcsw,
    ru.ru_nivcsw);

  fflush(flog);
}
#endif


static void
sig_term(sig)
  int sig;
{
  char buf[80];

  sprintf(buf, "sig: %d, errno: %d => %s", sig, errno, strerror(errno));
  logit("EXIT", buf);
  fclose(flog);
  exit(0);
}


static void
reaper()
{
  while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
}


static void
servo_signal()
{
  struct sigaction act;

  /* sigblock(sigmask(SIGPIPE)); *//* Thor.981206: 統一 POSIX 標準用法 */

  /* act.sa_mask = 0; *//* Thor.981105: 標準用法 */
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  act.sa_handler = sig_term;
  sigaction(SIGTERM, &act, NULL);	/* forced termination */
  sigaction(SIGSEGV, &act, NULL);	/* if rlimit violate */
  sigaction(SIGBUS, &act, NULL);

#if 1	/* Thor.990203: 抓 signal */
  sigaction(SIGURG, &act, NULL);
  sigaction(SIGXCPU, &act, NULL);
  sigaction(SIGXFSZ, &act, NULL);

#ifdef SOLARIS
  sigaction(SIGLOST, &act, NULL);
  sigaction(SIGPOLL, &act, NULL);
  sigaction(SIGPWR, &act, NULL);
#endif

#ifdef LINUX
  sigaction(SIGSYS, &act, NULL);
  /* sigaction(SIGEMT, &act, NULL); */
  /* itoc.010317: 我的 linux 沒有這個說 :p */
#endif

  sigaction(SIGFPE, &act, NULL);
  sigaction(SIGWINCH, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGQUIT, &act, NULL);
  sigaction(SIGILL, &act, NULL);
  sigaction(SIGTRAP, &act, NULL);
  sigaction(SIGABRT, &act, NULL);
  sigaction(SIGTSTP, &act, NULL);
  sigaction(SIGTTIN, &act, NULL);
  sigaction(SIGTTOU, &act, NULL);
  sigaction(SIGVTALRM, &act, NULL);
#endif

  sigaction(SIGHUP, &act, NULL);

  act.sa_handler = reaper;
  sigaction(SIGCHLD, &act, NULL);

#ifdef  SERVER_USAGE
  act.sa_handler = servo_usage;
  sigaction(SIGPROF, &act, NULL);
#endif

  /* Thor.981206: lkchu patch: 統一 POSIX 標準用法 */
  /* 在此借用 sigset_t act.sa_mask */
  sigaddset(&act.sa_mask, SIGPIPE);
  sigprocmask(SIG_BLOCK, &act.sa_mask, NULL);
}


/* ----------------------------------------------------- */
/* server core routines					 */
/* ----------------------------------------------------- */

static void
servo_daemon(inetd)
  int inetd;
{
  int fd, value;
  char buf[80];
  struct linger ld;
  struct sockaddr_in sin;

#ifdef HAVE_RLIMIT
  struct rlimit limit;
#endif

  /* More idiot speed-hacking --- the first time conversion makes the C     *
   * library open the files containing the locale definition and time zone. *
   * If this hasn't happened in the parent process, it happens in the       *
   * children, once per connection --- and it does add up.                  */

  time((time_t *) & value);
  gmtime((time_t *) & value);
  strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", localtime((time_t *) & value));

#ifdef HAVE_RLIMIT
  /* --------------------------------------------------- */
  /* adjust the resource limit				 */
  /* --------------------------------------------------- */

  getrlimit(RLIMIT_NOFILE, &limit);
  limit.rlim_cur = limit.rlim_max;
  setrlimit(RLIMIT_NOFILE, &limit);

  limit.rlim_cur = limit.rlim_max = 16 * 1024 * 1024;
  setrlimit(RLIMIT_FSIZE, &limit);

  limit.rlim_cur = limit.rlim_max = 16 * 1024 * 1024;
  setrlimit(RLIMIT_DATA, &limit);

#ifdef SOLARIS
#define RLIMIT_RSS RLIMIT_AS	/* Thor.981206: port for solaris 2.6 */
#endif

  setrlimit(RLIMIT_RSS, &limit);

  limit.rlim_cur = limit.rlim_max = 0;
  setrlimit(RLIMIT_CORE, &limit);
#endif

  /* --------------------------------------------------- */
  /* detach daemon process				 */
  /* --------------------------------------------------- */

  close(1);
  close(2);

  if (inetd)
    return;

  close(0);

  if (fork())
    exit(0);

  setsid();

  if (fork())
    exit(0);

  /* --------------------------------------------------- */
  /* setup socket					 */
  /* --------------------------------------------------- */

  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  value = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof(value));

  ld.l_onoff = ld.l_linger = 0;
  setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&ld, sizeof(ld));

  sin.sin_family = AF_INET;
  sin.sin_port = htons(BHTTP_PORT);
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  memset((char *)&sin.sin_zero, 0, sizeof(sin.sin_zero));

  if (bind(fd, (struct sockaddr *) & sin, sizeof(sin)) ||
    listen(fd, TCP_BACKLOG))
    exit(1);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int n, sock, cc;
  time_t uptime, tcheck, tfresh;
  Agent **FBI, *Scully, *Mulder, *agent;
  fd_set rset;
  struct timeval tv;

  cc = 0;

  while ((n = getopt(argc, argv, "i")) != -1)
  {
    switch (n)
    {
    case 'i':
      cc = 1;
      break;

    default:
      fprintf(stderr, "Usage: %s [options]\n"
	"\t-i  start from inetd with wait option\n",
	argv[0]);
      exit(0);
    }
  }

  servo_daemon(cc);

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(BBSHOME);

  init_bshm();
  init_ushm();
  init_fshm();

  guest_userno();

  servo_signal();

  log_open();
  dns_init();

  uptime = time(0);
  tcheck = uptime + BHTTP_PERIOD;
  tfresh = uptime + BHTTP_FRESH;

  Scully = Mulder = NULL;

  for (;;)
  {
    /* maintain : resource and garbage collection */

    uptime = time(0);
    if (tcheck < uptime)
    {
      /* ----------------------------------------------- */
      /* 將過久沒有動作的 agent 踢除			 */
      /* ----------------------------------------------- */

      tcheck = uptime - BHTTP_TIMEOUT;

      for (FBI = &Scully; agent = *FBI;)
      {
	if (agent->uptime < tcheck)
	{
	  agent_fire(agent);

	  *FBI = agent->anext;

	  agent->anext = Mulder;
	  Mulder = agent;
	}
	else
	{
	  FBI = &(agent->anext);
	}
      }

      /* ----------------------------------------------- */
      /* maintain server log				 */
      /* ----------------------------------------------- */

      if (tfresh < uptime)
      {
	tfresh = uptime + BHTTP_FRESH;
#ifdef SERVER_USAGE
	servo_usage();
#endif
	log_fresh();
      }
      else
      {
	fflush(flog);
      }

      tcheck = uptime + BHTTP_PERIOD;
    }

    /* ------------------------------------------------- */
    /* Set up the fdsets				 */
    /* ------------------------------------------------- */

    FD_ZERO(&rset);
    FD_SET(0, &rset);

    n = 0;
    for (agent = Scully; agent; agent = agent->anext)
    {
      sock = agent->sock;

      if (n < sock)
	n = sock;

      FD_SET(sock, &rset);
    }

    /* in order to maintain history, timeout every BHTTP_PERIOD seconds in case no connections */
    tv.tv_sec = BHTTP_PERIOD;
    tv.tv_usec = 0;
    if (select(n + 1, &rset, NULL, NULL, &tv) <= 0)
      continue;

    /* ------------------------------------------------- */
    /* serve active agents				 */
    /* ------------------------------------------------- */

    uptime = time(0);

    for (FBI = &Scully; agent = *FBI;)
    {
      sock = agent->sock;

      if (FD_ISSET(sock, &rset))
	cc = agent_recv(agent);
      else
	cc = -1;

      if (cc == 0)
      {
	agent_fire(agent);

	*FBI = agent->anext;

	agent->anext = Mulder;
	Mulder = agent;

	continue;
      }

      if (cc > 0)		/* 還有資料要 recv */
	agent->uptime = uptime;

      FBI = &(agent->anext);
    }

    /* ------------------------------------------------- */
    /* serve new connection				 */
    /* ------------------------------------------------- */

    /* Thor.000209: 考慮移前此部分, 免得卡在 accept() */
    if (FD_ISSET(0, &rset))
    {
      unsigned int ip_addr;

      sock = agent_accept(&ip_addr);
      if (sock > 0)
      {
	Agent *anext;

	if (agent = Mulder)
	{
	  anext = agent->anext;
	}
	else
	{
	  if (!(agent = (Agent *) malloc(sizeof(Agent))))
	  {
	    fcntl(sock, F_SETFL, M_NONBLOCK);
	    shutdown(sock, 2);
	    close(sock);

#ifdef LOG_VERBOSE
	    fprintf(flog, "ERROR\tNot enough space in main()\n");
#endif
	    continue;
	  }
	  anext = NULL;
	}

	/* variable initialization */

	memset(agent, 0, sizeof(Agent));

	agent->sock = sock;
	agent->tbegin = agent->uptime = uptime;

	agent->ip_addr = ip_addr;

	if (!(agent->data = (char *) malloc(MIN_DATA_SIZE)))
	{
	  agent_fire(agent);
#ifdef LOG_VERBOSE
	  fprintf(flog, "ERROR\tNot enough space in agent->data\n");
#endif
	  continue;
	}
	agent->size = MIN_DATA_SIZE;
	agent->used = 0;

	agent->fpw = fdopen(sock, "w");

	Mulder = anext;
	*FBI = agent;
      }
    }

    /* ------------------------------------------------- */
    /* tail of main loop				 */
    /* ------------------------------------------------- */
  }

  logit("EXIT", "shutdown");
  fclose(flog);

  exit(0);
}
