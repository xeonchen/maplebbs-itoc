/*-------------------------------------------------------*/
/* edit.c	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : simple ANSI/Chinese editor			 */
/* create : 95/03/29					 */
/* update : 95/12/15					 */
/*-------------------------------------------------------*/


#include "bbs.h"


/* #define	VE_WIDTH	(ANSILINELEN - 1) */
/* Thor.990330: 為防止引言後, ">"要變色, 一行會超過ANSILINELEN, 故多留空間 */
/* itoc.010317.註解: 那麼實際一列可以放下的有效字數為 VE_WIDTH - 3 */
#define	VE_WIDTH	(ANSILINELEN - 11)


typedef struct textline
{
  struct textline *prev;
  struct textline *next;
  int len;
  uschar data[ANSILINELEN];
}	textline;


static textline *vx_ini;	/* first line */
static textline *vx_cur;	/* current line */
static textline *vx_top;	/* top line in current window */


static int ve_lno;		/* current line number */
static int ve_row;		/* cursor position */
static int ve_col;


static int ve_mode;		/* operation mode */


#ifdef HAVE_MULTI_BYTE
static int zhc;			/* 是否有 UFO_ZHC */
#endif


#ifdef HAVE_ANONYMOUS
char anonymousid[IDLEN + 1];	/* itoc.010717: 自定匿名 ID */
#endif


#define	VE_INSERT	0x01
#define	VE_ANSI		0x02
#define	VE_FOOTER	0x04
#define	VE_REDRAW	0x08

#ifdef EVERY_BIFF
#define VE_BIFF		0x10
#endif /* Thor.980805: 郵差到處來按鈴 */


#define	FN_BAK		"bak"


/* ----------------------------------------------------- */
/* 記憶體管理與編輯處理					 */
/* ----------------------------------------------------- */


#ifdef	DEBUG_VEDIT
static void
ve_abort(i)
  int i;
{
  char msg[40];

  sprintf(msg, "嚴重內傷 %d", i);
  blog("VEDIT", msg);
}

#else

#define	ve_abort(n)	;
#endif


static void
ve_position(cur, top)
  textline *cur;
  textline *top;
{
  int row;

  row = cur->len;
  if (ve_col > row)
    ve_col = row;
#ifdef HAVE_MULTI_BYTE
  else if (zhc && ve_col < row && IS_ZHC_LO(cur->data, ve_col))	/* hightman.060504: 漢字整字調節 */
    ve_col++;
#endif

  row = 0;
  while (cur != top)
  {
    row++;
    cur = cur->prev;
  }
  ve_row = row;

  ve_mode |= VE_REDRAW;
}


static inline void
ve_pageup()
{
  textline *cur, *top, *tmp;
  int lno, n;

  cur = vx_cur;
  top = vx_top;
  lno = ve_lno;
  for (n = PAGE_SCROLL; n > 0; n--)
  {
    if (!(tmp = cur->prev))
      break;

    cur = tmp;
    lno--;

    if (tmp = top->prev)
      top = tmp;
  }

  vx_cur = cur;
  vx_top = top;
  ve_lno = lno;

  ve_position(cur, top);
}


static inline void
ve_forward(n)
  int n;
{
  textline *cur, *top, *tmp;
  int lno;

  cur = vx_cur;
  top = vx_top;
  lno = ve_lno;
  while (n--)
  {
    if (!(tmp = cur->next))
      break;

    lno++;
    cur = tmp;

    if (tmp = top->next)
      top = tmp;
  }

  vx_cur = cur;
  vx_top = top;
  ve_lno = lno;

  ve_position(cur, top);
}


static inline char *
ve_strim(s)
  char *s;
{
  while (*s == ' ')
    s++;
  return s;
}


static textline *
ve_alloc()
{
  textline *p;

  if (p = (textline *) malloc(sizeof(textline)))
  {
    p->prev = NULL;
    p->next = NULL;
    p->len = 0;
    p->data[0] = '\0';
    return p;
  }

  ve_abort(13);			/* 記憶體用光了 */
  abort_bbs();

  return NULL;
}


/* ----------------------------------------------------- */
/* Thor: ansi 座標轉換  for color 編輯模式		 */
/* ----------------------------------------------------- */


static int
ansi2n(ansix, line)
  int ansix;
  textline *line;
{
  uschar *data, *tmp;
  int ch;

  data = tmp = line->data;

  while (ch = *tmp)
  {
    if (ch == KEY_ESC)
    {
      for (;;)
      {
	ch = *++tmp;
	if (ch >= 'a' && ch <= 'z' /* isalpha(ch) */ )
	{
	  tmp++;
	  break;
	}
	if (!ch)
	  break;
      }
      continue;
    }
    if (ansix <= 0)
      break;
    tmp++;
    ansix--;
  }
  return tmp - data;
}


static int
n2ansi(nx, line)
  int nx;
  textline *line;
{
  uschar *tmp, *nxp;
  int ansix;
  int ch;

  tmp = nxp = line->data;
  nxp += nx;
  ansix = 0;

  while (ch = *tmp)
  {
    if (ch == KEY_ESC)
    {
      for (;;)
      {
	ch = *++tmp;
	if (ch >= 'a' && ch <= 'z' /* isalpha(ch) */ )
	{
	  tmp++;
	  break;
	}
	if (!ch)
	  break;
      }
      continue;
    }
    if (tmp >= nxp)
      break;
    tmp++;
    ansix++;
  }
  return ansix;
}


/* ----------------------------------------------------- */
/* delete_line deletes 'line' from the list,		 */
/* and maintains the vx_ini pointers.			 */
/* ----------------------------------------------------- */


static void
delete_line(line)
  textline *line;
{
  textline *p = line->prev;
  textline *n = line->next;

  if (p || n)
  {
    if (n)
      n->prev = p;

    if (p)
      p->next = n;
    else
      vx_ini = n;

    free(line);
  }
  else
  {
    line->data[0] = line->len = 0;
  }
}


/* ----------------------------------------------------- */
/* split 'line' right before the character pos		 */
/* ----------------------------------------------------- */


static void
ve_split(line, pos)
  textline *line;
  int pos;
{
  int len = line->len - pos;

  if (len >= 0)
  {
    textline *p, *n;
    uschar *ptr;

    line->len = pos;
    p = ve_alloc();
    p->len = len;
    strcpy(p->data, (ptr = line->data + pos));
    *ptr = '\0';

    /* --------------------------------------------------- */
    /* append p after line in list. keep up with last line */
    /* --------------------------------------------------- */

    if (p->next = n = line->next)
      n->prev = p;
    line->next = p;
    p->prev = line;

    if (line == vx_cur && pos <= ve_col)
    {
      vx_cur = p;
      ve_col -= pos;
      ve_row++;
      ve_lno++;
    }
    ve_mode |= VE_REDRAW;
  }
}


/* ----------------------------------------------------- */
/* connects 'line' and the next line. returns true if:	 */
/* 1) lines were joined and one was deleted		 */
/* 2) lines could not be joined		 		 */
/* 3) next line is empty				 */
/* ----------------------------------------------------- */
/* returns false if:					 */
/* 1) Some of the joined line wrapped			 */
/* ----------------------------------------------------- */


static int
ve_join(line)
  textline *line;
{
  textline *n;
  uschar *data, *s;
  int sum, len;

  if (!(n = line->next))
    return 1;

  if (!*ve_strim(data = n->data))
    return 1;

  len = line->len;
  sum = len + n->len;
  if (sum < VE_WIDTH)
  {
    strcpy(line->data + len, data);
    line->len = sum;
    delete_line(n);
    return 1;
  }

  s = data - len + VE_WIDTH - 1;
  while (*s == ' ' && s != data)
    s--;
  while (*s != ' ' && s != data)
    s--;
  if (s == data)
    return 1;

  ve_split(n, (s - data) + 1);
  if (len + n->len >= VE_WIDTH)
  {
    ve_abort(0);
    return 1;
  }

  ve_join(line);
  n = line->next;
  len = n->len;
  if (len >= 1 && len < VE_WIDTH - 1)
  {
    s = n->data + len - 1;
    if (*s != ' ')
    {
      *s++ = ' ';
      *s = '\0';
      n->len = len + 2;
    }
  }
  return 0;
}


static void
join_up(line)
  textline *line;
{
  while (!ve_join(line))
  {
    line = line->next;
    if (line == NULL)
    {
      ve_abort(2);
      abort_bbs();
    }
  }
}


/* ----------------------------------------------------- */
/* character insert / detete				 */
/* ----------------------------------------------------- */


static void
ve_char(ch)
  int ch;
{
  textline *p;
  int col, len, mode;
  uschar *data;

  p = vx_cur;
  len = p->len;
  col = ve_col;

  if (col > len)
  {
    ve_abort(1);
    return;
  }

  data = p->data;
  mode = ve_mode;

  /* --------------------------------------------------- */
  /* overwrite						 */
  /* --------------------------------------------------- */

  if ((col < len) && !(mode & VE_INSERT))
  {
    data[col++] = ch;

    /* Thor: ansi 編輯, 可以 overwrite, 不蓋到 ansi code */

    if (mode & VE_ANSI)
      col = ansi2n(n2ansi(col, p), p);

    ve_col = col;
    return;
  }

  /* --------------------------------------------------- */
  /* insert / append					 */
  /* --------------------------------------------------- */

  for (mode = len; mode >= col; mode--)
  {
    data[mode + 1] = data[mode];
  }
  data[col++] = ch;
  ve_col = col;
  p->len = ++len;

  if (len >= VE_WIDTH - 2)
  {
    /* Thor.980727: 修正 editor buffer overrun 問題, 見後 */

    ve_split(p, VE_WIDTH - 3);

#if 0
    uschar *str = data + len;

    while (*--str == ' ')
    {
      if (str == data)
	break;
    }

    ve_split(p, (str - data) + 1);
#endif



#if 0
 作者  yvb (yvb)                                            看板  SYSOP
 標題  關於 editor...
 時間  Sun Jun 28 11:28:02 1998
───────────────────────────────────────

    post 及 mail 等的這個 editor, 如果你一直往後面打字,
    最多可以打到一列 157 字吧... 再打就會被迫換行.

    不過如果你堅持仍要在後面加字... 也就是再移回剛才那行
    繼續打字... 那系統仍會讓你在該列繼續加一個字母, 並且
    換出新的一行來...

    重覆這個步驟, 到第 170 字時, 你就會被斷線了...
    呵... 夠變態吧 :P

    其實, 從另一個觀點來說, 若這不是系統特意這樣子設計,
    那就表示這裏似乎潛藏著可藉由 buffer overrun 的方式,
    可能達成入侵系統的危機...
--
※ 來源: 月光森林 ◆ From: bamboo.Dorm6.NCTU.edu.tw

#endif

  }
}


static void
delete_char(cur, col)
  textline *cur;
  int col;
{
  uschar *dst, *src;

  cur->len--;
  dst = cur->data + col;
  for (;;)
  {
    src = dst + 1;
    if (!(*dst = *src))
      break;
    dst = src;
  }
}


static void
ve_string(str)
  uschar *str;
{
  int ch;

  while (ch = *str)
  {
    if (isprint2(ch) || ch == KEY_ESC)
    {
      ve_char(ch);
    }
    else if (ch == '\t')
    {
      do
      {
	ve_char(' ');
      } while (ve_col & TAB_WIDTH);
    }
    else if (ch == '\n')
    {
      ve_split(vx_cur, ve_col);
    }
    str++;
  }
}


static void
ve_ansi()
{
  int fg, bg, mode;
  char ans[4], buf[16], *apos, *color, *tmp;
  static char t[] = "BRGYLPCW";

  mode = ve_mode | VE_REDRAW;
  color = str_ransi;

  if (mode & VE_ANSI)
  {
    move(b_lines - 1, 55);
    outs("\033[1;33;40mB\033[41mR\033[42mG\033[43mY\033[44mL\033[45mP\033[46mC\033[47mW\033[m");
    if (fg = vget(b_lines, 0, "請輸入  亮度/前景/背景[正常白字黑底][0wb]：",
	apos = ans, 4, LCECHO))
    {
      color = buf;
      strcpy(color, "\033[");
      if (isdigit(fg))
      {
	sprintf(color, "%s%c", color, *(apos++));
	if (*apos)
	  strcat(color, ";");
      }
      if (*apos)
      {
	if (tmp = strchr(t, toupper(*(apos++))))
	  fg = tmp - t + 30;
	else
	  fg = 37;
	sprintf(color, "%s%d", color, fg);
      }
      if (*apos)
      {
	if (tmp = strchr(t, toupper(*(apos++))))
	  bg = tmp - t + 40;
	else
	  bg = 40;
	sprintf(color, "%s;%d", color, bg);
      }
      strcat(color, "m");
    }
  }

  ve_mode = mode | VE_INSERT;
  ve_string(color);
  ve_mode = mode;
}


static textline *
ve_line(this, str)
  textline *this;
  uschar *str;
{
  int cc, len;
  uschar *data;
  textline *line;

  do
  {
    line = ve_alloc();
    data = line->data;
    len = 0;

    for (;;)
    {
      cc = *str;

      if (cc == '\n')
	cc = 0;
      if (cc == 0)
	break;

      str++;

      if (cc == '\t')
      {
	do
	{
	  *data++ = ' ';
	  len++;
	} while ((len & TAB_WIDTH) && (len < VE_WIDTH));
      }
      else if (cc < ' ' && cc != KEY_ESC)
      {
	continue;
      }
      else
      {
	*data++ = cc;
	len++;
      }
      if (len >= VE_WIDTH)
	break;
    }

    *data = '\0';
    line->len = len;
    line->prev = this;
    this = this->next = line;

  } while (cc);

  return this;
}


/* ----------------------------------------------------- */
/* 顯示簽名檔						 */
/* ----------------------------------------------------- */


static void
show_sign()		/* itoc.000319: 顯示簽名檔的內容 */
{
  int fd, len, i;
  char fpath[64], buf[10], ch, *str;

  clear();

  sprintf(buf, "%s.0", FN_SIGN);
  usr_fpath(fpath, cuser.userid, buf);	/* itoc.020123: 各個簽名檔檔案分開 */
  len = strlen(fpath) - 1;

  for (ch = '1'; ch <= '3'; ch++)	/* 三個簽名檔 */
  {
    fpath[len] = ch;

    fd = open(fpath, O_RDONLY);
    if (fd >= 0)
    {
      mgets(-1);
      move((ch - '1') * (MAXSIGLINES + 1), 0);
      prints("\033[1;36m【 簽名檔 %c 】\033[m\n", ch);

      for (i = 1; i <= MAXSIGLINES; i++)
      {
	if (!(str = mgets(fd)))
	  break;
	prints("%s\n", str);
      }
    }
  }
}


/* ----------------------------------------------------- */
/* 符號輸入工具 Input Tools				 */
/* ----------------------------------------------------- */


#ifdef INPUT_TOOLS
static void
input_tools()   /* itoc.000319: 符號輸入工具 */
{
  char msg1[] = {"1.括符方塊  2.線條表框  3.數學符號 (PgDn/N:下一頁)？[Q] "};
  char msg2[] = {"4.圖案數字  5.希臘字母  6.注音標點 (PgUp/P:上一頁)？[Q] "};

  char ansi[6][100] =
  {
    {	/* 1.括符方塊 */
      "▁▂▃▄▅▆▇█◢◣"
      "▏▎▍▌▋▊▉◥◤□"
      "（）｛｝〔〕【】《》"
      "〈〉「」『』︻︼︽︾"
      "︵︶︷︸︹︺﹁﹂﹃﹄"
    }, 

    {	/* 2.線條表框 */
      "╓╥╖╙╨╜─│═∥"
      "┌┬┐├┼┤└┴┘╳"
      "╔╦╗╠╬╣╚╩╝﹏\"
      "╒╕╞╡╘╛╭╮╰╯"
      "▁▔▏▕╱╲←→↑↓"
    }, 

    {	/* 3.數學符號 */
      "＋－×÷±＝＜＞≦≧"
      "≠≡～∵∴∩∪∞π√"
      "ΣΠ∫∮∠⊥∟⊿Δ°"
      "＃＆＊※＠＄￥￠￡％"
      "℃℉㎜㎝㎞㎡㎎㎏㏄§"
    }, 

    {	/* 4.圖案數字 */
      "○●△▲◎☆◇◆□■"
      "▽▼㊣⊕⊙十卄卅♂♀"
      "０１２３４５６７８９"
      "ⅠⅡⅢⅣⅤⅥⅦⅧⅨⅩ"
      "けげこごさざしじすず"
    }, 

    {	/* 5.希臘字母 */
      "ΑΒΓΔΕΖΗΘΙΚ"
      "ΛΜΝΞΟΠΡΣΤΥ"
      "ΦΧΨΩα\βγδεζ"
      "ηθικλμνξοπ"
      "ρστυφχψω∕﹨"
    }, 

    {	/* 6.注音標點 */
      "ㄅㄆㄇㄈㄉㄊㄋㄌㄍㄎ"
      "ㄏㄐㄑㄒㄓㄔㄕㄖㄗㄘ"
      "ㄙㄚㄛㄜㄝㄞㄟㄠㄡㄢ"
      "ㄣㄤㄥㄦㄧㄨㄩˊˇˋ"
      "˙，。、！？：；「」"
    }
  };

  char buf[80], tmp[6];
  char *ptr, *str;
  int ch, page;

  /* 選分類 */
  ch = KEY_PGUP;
  do
  {
    outz("內碼輸入工具：");
    outs((ch == KEY_PGUP || ch == 'P') ? msg1 : msg2);
    ch = vkey();
  } while (ch == KEY_PGUP || ch == 'P' || ch == KEY_PGDN || ch == 'N');

  if (ch < '1' || ch > '6')
    return;

  ptr = ansi[ch - '1'];
  page = 0;

  for (;;)
  {
    buf[0] = '\0';
    str = ptr + page * 20;	/* 每 page 有十個中文字，每個中文字是 2 char */

    for (ch = 0; ch < 10; ch++)
    {
      sprintf(tmp, "%d.%2.2s ", ch, str + ch * 2);	/* 每個中文字是 2 char */
      strcat(buf, tmp);
    }
    strcat(buf, "(PgUp/P:上  PgDn/N:下)[Q] ");
    outz(buf);
    ch = vkey();

    if (ch == KEY_PGUP || ch == 'P')
    {
      if (page)
	page--;
    }
    else if (ch == KEY_PGDN || ch == 'N')
    {
      if (page != 4)
	page++;
    }
    else if (ch < '0' || ch > '9')
    {
      break;
    }
    else
    {
      str += (ch - '0') * 2;
      ve_char(*str);		/* 印出二個 char 中文字 */
      ve_char(*++str);
      break;
    }
  }
}
#endif


/* ----------------------------------------------------- */
/* 暫存檔 TBF (Temporary Buffer File) routines		 */
/* ----------------------------------------------------- */


char *
tbf_ask()
{
  static char fn_tbf[] = "buf.1";
  int ch;

  do
  {
    ch = vget(b_lines, 0, "請選擇暫存檔(1-5)：", fn_tbf + 4, 2, GCARRY);
  } while (ch < '1' || ch > '5');
  return fn_tbf;
}


FILE *
tbf_open()
{
  int ans;
  char fpath[64], op[4];
  struct stat st;

  usr_fpath(fpath, cuser.userid, tbf_ask());
  ans = 'a';

  if (!stat(fpath, &st))
  {
    ans = vans("暫存檔已有資料 (A)附加 (W)覆寫 (Q)取消？[A] ");
    if (ans == 'q')
      return NULL;

    if (ans != 'w')
    {
      /* itoc.030208: 檢查暫存檔大小 */
      if (st.st_size >= 100000)		/* 100KB 應該夠了 */
      {
	zmsg("暫存檔太大，無法附加");
	return NULL;
      }

      ans = 'a';
    }
  }

  op[0] = ans;
  op[1] = '\0';

  return fopen(fpath, op);
}


static textline *
ve_load(this, fd)
  textline *this;
  int fd;
{
  uschar *str;
  textline *next;

  next = this->next;

  mgets(-1);
  while (str = mgets(fd))
  {
    this = ve_line(this, str);
  }

  this->next = next;
  if (next)
    next->prev = this;

  return this;
}


static inline void
tbf_read()
{
  int fd;
  char fpath[80];

  usr_fpath(fpath, cuser.userid, tbf_ask());

  fd = open(fpath, O_RDONLY);
  if (fd >= 0)
  {
    ve_load(vx_cur, fd);
    close(fd);
  }
}


static inline void
tbf_write()
{
  FILE *fp;
  textline *p;
  uschar *data;

  if (fp = tbf_open())
  {
    for (p = vx_ini; p;)
    {
      data = p->data;
      p = p->next;
      if (p || *data)
	fprintf(fp, "%s\n", data);
    }
    fclose(fp);
  }
}


static inline void
tbf_erase()
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, tbf_ask());
  unlink(fpath);
}


/* ----------------------------------------------------- */
/* 編輯器自動備份					 */
/* ----------------------------------------------------- */


void
ve_backup()
{
  textline *p, *n;

  if (p = vx_ini)
  {
    FILE *fp;
    char bakfile[64];

    vx_ini = NULL;
    usr_fpath(bakfile, cuser.userid, FN_BAK);
    if (fp = fopen(bakfile, "w"))
    {
      do
      {
	n = p->next;
	fprintf(fp, "%s\n", p->data);
	free(p);
      } while (p = n);
      fclose(fp);
    }
  }
}


void
ve_recover()
{
  char fpbak[80], fpath[80];
  int ch;

  usr_fpath(fpbak, cuser.userid, FN_BAK);
  if (dashf(fpbak))
  {
    ch = vans("您有一篇文章尚未完成，(M)寄到信箱 (S)寫入暫存檔 (Q)算了？[M] ");
    if (ch == 's')
    {
      usr_fpath(fpath, cuser.userid, tbf_ask());
      rename(fpbak, fpath);
      return;
    }
    else if (ch != 'q')
    {
      mail_self(fpbak, cuser.userid, "未完成的文章", 0);
    }
    unlink(fpbak);
  }
}


/* ----------------------------------------------------- */
/* 引用文章						 */
/* ----------------------------------------------------- */


static int
is_quoted(str)
  char *str;			/* "--\n", "-- \n", "--", "-- " */
{
  if (*str == '-')
  {
    if (*++str == '-')
    {
      if (*++str == ' ')
	str++;
      if (*str == '\n')
	str++;
      if (!*str)
	return 1;
    }
  }
  return 0;
}


static inline int
quote_line(str, qlimit)
  char *str;
  int qlimit;			/* 允許幾層引言？ */
{
  int qlevel = 0;
  int ch;

  while ((ch = *str) == QUOTE_CHAR1 || ch == QUOTE_CHAR2)
  {
    if (*(++str) == ' ')
      str++;
    if (qlevel++ >= qlimit)
      return 0;
  }
  while ((ch = *str) == ' ' || ch == '\t')
    str++;
  if (qlevel >= qlimit)
  {
    if (!memcmp(str, "※ ", 3) || !memcmp(str, "==>", 3) ||
      strstr(str, ") 提到:\n"))
      return 0;
  }
  return (*str != '\n');
}


static void
ve_quote(this)
  textline *this;
{
  int fd, op;
  FILE *fp;
  textline *next;
  char *str, buf[ANSILINELEN];
  static char msg[] = "選擇簽名檔 (1/2/3 0=不加 r=亂數)[0]：";

  next = this->next;

  /* --------------------------------------------------- */
  /* 引言						 */
  /* --------------------------------------------------- */

  if (*quote_file)
  {
    op = vans("是否引用原文 Y)引用 N)不引用 A)引用全文 R)重貼全文 1-9)引用層數？[Y] ");

    if (op != 'n')
    {
      if (fp = fopen(quote_file, "r"))
      {
	str = buf;

	if ((op >= '1') && (op <= '9'))
	  op -= '1';
	else if ((op != 'a') && (op != 'r'))
	  op = 1;		/* default : 2 level */

	if (op != 'a')		/* 去掉 header */
	{
	  if (*quote_nick)
	    sprintf(buf + 128, " (%s)", quote_nick);
	  else
	    buf[128] = '\0';
	  sprintf(str, "※ 引述《%s%s》之銘言：", quote_user, buf + 128);
	  this = ve_line(this, str);

	  while (fgets(str, ANSILINELEN, fp) && *str != '\n');

	  if (curredit & EDIT_LIST)	/* 去掉 mail list 之 header */
	  {
	    while (fgets(str, ANSILINELEN, fp) && (!memcmp(str, "※ ", 3)));
	  }
	}

	if (op == 'r')
	{
	  op = 'a';
	}
	else
	{
	  *str++ = QUOTE_CHAR1;
	  *str++ = ' ';
	}

	if (op == 'a')
	{
	  while (fgets(str, ANSILINELEN - 2, fp))	/* 留空間給 "> " */
	    this = ve_line(this, buf);
	}
	else
	{
	  while (fgets(str, ANSILINELEN - 2, fp))	/* 留空間給 "> " */
	  {
	    if (is_quoted(str))	/* "--\n" */
	      break;
	    if (quote_line(str, op))
	      this = ve_line(this, buf);
	  }
	}
	fclose(fp);
      }
    }
    *quote_file = '\0';
  }

  this = ve_line(this, "");

  /* --------------------------------------------------- */
  /* 簽名檔						 */
  /* --------------------------------------------------- */

#ifdef HAVE_ANONYMOUS
  if (!(currbattr & BRD_ANONYMOUS) && !(cuser.ufo & UFO_NOSIGN))	/* 在匿名板中無論是否匿名，均不使用簽名檔 */
#else
  if (!(cuser.ufo & UFO_NOSIGN))					/* itoc.000320: 不使用簽名檔 */
#endif
  {    
    if (cuser.ufo & UFO_SHOWSIGN)	/* itoc.000319: 顯示簽名檔的內容 */
      show_sign();

    msg[33] = op = cuser.signature + '0';
    if (fd = vget(b_lines, 0, msg, buf, 3, DOECHO))
    {
      if (op != fd && ((fd >= '0' && fd <= '3') || fd == 'r'))
      {
	cuser.signature = fd - '0';
	op = fd;
      }
    }

    if (op == 'r')
      op = (time(0) % 3) + '1';

    if (op != '0')
    {
      char fpath[64];

      sprintf(buf, "%s.%c", FN_SIGN, op);
      usr_fpath(fpath, cuser.userid, buf);	/* itoc.020123: 各個簽名檔檔案分開 */

      if ((fd = open(fpath, O_RDONLY)) >= 0)
      {
	op = 0;
	mgets(-1);
	while ((str = mgets(fd)) && (op < MAXSIGLINES))
	{
	  if (!op)
	    this = ve_line(this, "--");

	  this = ve_line(this, str);
	  op++;
	}
	close(fd);
      }
    }
  }

  this->next = next;
  if (next)
    next->prev = this;
}


/* ----------------------------------------------------- */
/* 審查 user 引言的使用					 */
/* ----------------------------------------------------- */


static int
quote_check()
{
  textline *p;
  char *str;
  int post_line;
  int quot_line;
  int in_quote;

  post_line = quot_line = in_quote = 0;
  for (p = vx_ini; p; p = p->next)
  {
    str = p->data;

    /* itoc.030305: 為維護網路禮儀，簽名檔也算引言 :p */

    if (in_quote)		/* 簽名檔 */
    {
      quot_line++;
    }
    else if (is_quoted(str))	/* 簽名檔開始 */
    {
      in_quote = 1;
      quot_line++;
    }
    else if (str[0] == QUOTE_CHAR1 && str[1] == ' ')	/* 引言 */
    {
      quot_line++;
    }
    else			/* 一般內文 */
    {
      /* 空白行不算 post_line */
      while (*str == ' ')
	str++;
      if (*str)
	post_line++;
    }
  }

  if ((quot_line >> 2) <= post_line)   /* 文章行數要多於引言行數四分之一 */
    return 0;

  if (HAS_PERM(PERM_ALLADMIN))
    return (vans("引言過多 (E)繼續編輯 (W)強制寫入？[E] ") != 'w');

  vmsg("引言太多，請按 Ctrl+Y 來刪除不必要之引言");
  return 1;
}


/* ----------------------------------------------------- */
/* 審查 user 發表文章字數/注音文的使用			 */
/* ----------------------------------------------------- */


int wordsnum;		/* itoc.010408: 算文章字數 */


#ifdef ANTI_PHONETIC
static int
words_check()
{
  textline *p; 
  uschar *str, *pend;
  int phonetic;		/* 注音文數目 */

  wordsnum = phonetic = 0;

  for (p = vx_ini; p; p = p->next)
  {
    if (is_quoted(str = p->data))	/* 簽名檔開始 */
      break;

    if (!(str[0] == QUOTE_CHAR1 && str[1] == ' ') && strncmp(str, "※ ", 3)) /* 非引用他人文章 */
    {
      wordsnum += p->len;

      pend = str + p->len;
      while (str < pend)
      {
	if (str[0] >= 0x81 && str[0] < 0xFE && str[1] >= 0x40 && str[1] <= 0xFE && str[1] != 0x7F)	/* 中文字 BIG5+ */
	{
	  if (str[0] == 0xA3 && str[1] >= 0x74 && str[1] <= 0xBA)	/* 注音文 */
	    phonetic++;
	  str++;	/* 中文字雙位元，要多加一次 */
	}
	str++;
      }

    }
  }
  return phonetic;
}

#else

static void
words_check()
{
  textline *p; 
  char *str;

  wordsnum = 0;

  for (p = vx_ini; p; p = p->next)
  {
    if (is_quoted(str = p->data))	/* 簽名檔開始 */
      break;

    if (!(str[0] == QUOTE_CHAR1 && str[1] == ' ') && strncmp(str, "※ ", 3)) /* 非引用他人文章 */
      wordsnum += p->len;
  }
}
#endif


/* ----------------------------------------------------- */
/* 檔案處理：讀檔、存檔、標題、簽名檔			 */
/* ----------------------------------------------------- */


void
ve_header(fp)
  FILE *fp;
{
  time_t now;
  char *title;

  title = ve_title;
  title[72] = '\0';
  time(&now);

  if (curredit & EDIT_MAIL)
  {
    fprintf(fp, "%s %s (%s)\n", str_author1, cuser.userid, cuser.username);
  }
  else
  {   
#ifdef HAVE_ANONYMOUS
    if (currbattr & BRD_ANONYMOUS && !(curredit & EDIT_RESTRICT))
    {
      if (!vget(b_lines, 0, "請輸入您想用的ID，也可直接按[Enter]，或是按[r]用真名：", anonymousid, IDLEN, DOECHO))
      {											/* 留 1 byte 加 "." */
	strcpy(anonymousid, STR_ANONYMOUS);
	curredit |= EDIT_ANONYMOUS;
      }
      else if (strcmp(anonymousid, "r"))
      {
	strcat(anonymousid, ".");		/* 若是自定ID，要在後面加個 . 表示不同 */
	curredit |= EDIT_ANONYMOUS;
      }
    }
#endif

    if (!(currbattr & BRD_NOSTAT) && !(curredit & EDIT_RESTRICT))	/* 不計文章篇數 及 加密存檔 不列入統計篇數 */
    {
      /* 產生統計資料 */

      POSTLOG postlog;

#ifdef HAVE_ANONYMOUS
      /* Thor.980909: anonymous post mode */
      if (curredit & EDIT_ANONYMOUS)
	strcpy(postlog.author, anonymousid);
      else
#endif
	strcpy(postlog.author, cuser.userid);

      strcpy(postlog.board, currboard);
      str_ncpy(postlog.title, str_ttl(title), sizeof(postlog.title));
      postlog.date = now;
      postlog.number = 1;

      rec_add(FN_RUN_POST, &postlog, sizeof(POSTLOG));
    }

#ifdef HAVE_ANONYMOUS
    /* Thor.980909: anonymous post mode */
    if (curredit & EDIT_ANONYMOUS)
    {
      fprintf(fp, "%s %s (%s) %s %s\n",
	str_author1, anonymousid, STR_ANONYMOUS,
	curredit & EDIT_OUTGO ? str_post1 : str_post2, currboard);
    }
    else
#endif

    {
      fprintf(fp, "%s %s (%s) %s %s\n", 
	str_author1, cuser.userid, cuser.username,
	curredit & EDIT_OUTGO ? str_post1 : str_post2, currboard);
    }
  }
  fprintf(fp, "標題: %s\n時間: %s\n\n", title, Btime(&now));
}


void
ve_banner(fp, modify)		/* 加上來源等訊息 */
  FILE *fp;
  int modify;			/* 1:修改 0:原文 */
{
  /* itoc: 建議 banner 不要超過三行，過長的站簽可能會造成某些使用者的反感 */

  if (!modify)
  {
    fprintf(fp, EDIT_BANNER, 
#ifdef HAVE_ANONYMOUS
      (curredit & EDIT_ANONYMOUS) ? STR_ANONYMOUS : 
#endif
      cuser.userid, 
#ifdef HAVE_ANONYMOUS
      (curredit & EDIT_ANONYMOUS) ? "雲與山的彼端 ^O^||" : 
#endif
      fromhost);
  }
  else
  {
    fprintf(fp, MODIFY_BANNER, cuser.userid, Now());
  }
}


static int
ve_filer(fpath, ve_op)
  char *fpath;
  int ve_op;	/* 1: 有 header  0,2: 無 header  -1: 不能儲存 */
{
  int ans;
  FILE *fp;
  textline *p, *v;
  char buf[80], *msg;

#ifdef POPUP_ANSWER
  char **menu;
#  ifdef HAVE_REFUSEMARK
  char *menu1[] = {"EE", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔", NULL};
  char *menu2[] = {"SE", "Save     存檔", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔", NULL};
  char *menu3[] = {"SE", "Save     存檔", "Local    存為站內檔", "XRefuse  加密存檔", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔", NULL};
  char *menu4[] = {"LE", "Local    存為站內檔", "Save     存檔", "XRefuse  加密存檔", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔", NULL};
#  else
  char *menu1[] = {"EE", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔", NULL};
  char *menu2[] = {"SE", "Save     存檔", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔", NULL};
  char *menu3[] = {"SE", "Save     存檔", "Local    存為站內檔", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔", NULL};
  char *menu4[] = {"LE", "Local    存為站內檔", "Save     存檔", "Abort    放棄", "Title    改標題", "Edit     繼續編輯", "Read     讀取暫存檔", "Write    寫入暫存檔", "Delete   刪除暫存檔", NULL};
#  endif
#else
#  ifdef HAVE_REFUSEMARK
  char *msg1 = "[E]繼續 (A)放棄 (R/W/D)讀寫刪暫存檔？";
  char *msg2 = "[S]存檔 (A)放棄 (T)改標題 (E)繼續 (R/W/D)讀寫刪暫存檔？";
  char *msg3 = "[S]存檔 (L)站內 (X)密封 (A)放棄 (T)改標題 (E)繼續 (R/W/D)讀寫刪暫存檔？";
  char *msg4 = "[L]站內 (S)存檔 (X)密封 (A)放棄 (T)改標題 (E)繼續 (R/W/D)讀寫刪暫存檔？";
#  else
  char *msg1 = "[E]繼續 (A)放棄 (R/W/D)讀寫刪暫存檔？";
  char *msg2 = "[S]存檔 (A)放棄 (T)改標題 (E)繼續 (R/W/D)讀寫刪暫存檔？";
  char *msg3 = "[S]存檔 (L)站內 (A)放棄 (T)改標題 (E)繼續 (R/W/D)讀寫刪暫存檔？";
  char *msg4 = "[L]站內 (S)存檔 (A)放棄 (T)改標題 (E)繼續 (R/W/D)讀寫刪暫存檔？";
#  endif
#endif

  ans = 0;

#ifdef POPUP_ANSWER
  if (ve_op < 0)			/* itoc.010301: 新增 ve_op = -1 不能儲存 */
    menu = menu1;
  else if (bbsmode != M_POST)		/* 寫信 */
    menu = menu2;
  else if (curredit & EDIT_OUTGO)	/* 轉信板發文 */
    menu = menu3;
  else
    menu = menu4;

  switch (pans(3, 20, "存檔選項", menu))
#else
  if (ve_op < 0)			/* itoc.010301: 新增 ve_op = -1 不能儲存 */
    msg = msg1;
  else if (bbsmode != M_POST)		/* 寫信 */
    msg = msg2;
  else if (curredit & EDIT_OUTGO)	/* 轉信板發文 */
    msg = msg3;
  else
    msg = msg4;

  switch (vans(msg))
#endif

  {
  case 's':
    if (ve_op < 0)		/* itoc.010301: 不能儲存 */
      return VE_FOOTER;
    /* Thor.990111: 不轉信則不外流 */
    if (HAS_PERM(PERM_INTERNET) && !(currbattr & BRD_NOTRAN))
      curredit |= EDIT_OUTGO;
    break;

  case 'a':
    ans = -1;
    break;

  case 'l':
    if (ve_op < 0)		/* itoc.010301: 不能儲存 */
      return VE_FOOTER;
    curredit &= ~EDIT_OUTGO;
    break;

#ifdef HAVE_REFUSEMARK
  case 'x':
   if (ve_op < 0)		/* itoc.010301: 不能儲存 */
     return VE_FOOTER;
   curredit |= EDIT_RESTRICT;
   curredit &= ~EDIT_OUTGO;	/* 加密必是 local save */
   break;
#endif

  case 'r':
    tbf_read();
    return VE_REDRAW;

  case 'e':
    return VE_FOOTER;

  case 'w':
    tbf_write();
    return VE_FOOTER;

  case 'd':
    tbf_erase();
    return VE_FOOTER;

  case 't':
    if (ve_op > 0)		/* itoc.010301: 不能儲存 */
    {
      strcpy(buf, ve_title);
      if (!vget(b_lines, 0, "標題：", ve_title, TTLEN + 1, GCARRY))
	strcpy(ve_title, buf);
    }
    return VE_FOOTER;

  default:
    if (ve_op < 0)		 /* itoc.010301: 不能儲存 */
      return VE_FOOTER;    
  }

  if (!ans)
  {
    if (ve_op == 1 && !(curredit & EDIT_MAIL) && quote_check())
      return VE_FOOTER;

#ifdef ANTI_PHONETIC
    if (words_check() > 2)
    {
      vmsg("請勿使用注音文");
      return VE_FOOTER;
    }
#endif

    if (!*fpath)
    {
      usr_fpath(fpath, cuser.userid, fn_note);
    }

    if ((fp = fopen(fpath, "w")) == NULL)
    {
      ve_abort(5);
      abort_bbs();
    }

#ifndef ANTI_PHONETIC
    words_check();	/* itoc.010408: 算文章字數 */
#endif

    if (ve_op == 1)
      ve_header(fp);
  }

  if (p = vx_ini)
  {
    vx_ini = NULL;

    do
    {
      v = p->next;
      if (!ans)
      {
	msg = p->data;
	str_trim(msg);
	fprintf(fp, "%s\n", msg);
      }
      free(p);
    } while (p = v);
  }

  if (!ans)
  {
    if (bbsmode == M_POST || bbsmode == M_SMAIL)
      ve_banner(fp, 0);
    fclose(fp);
  }

  return ans;
}


/* ----------------------------------------------------- */
/* 螢幕處理：輔助訊息、顯示編輯內容			 */
/* ----------------------------------------------------- */


static void
ve_outs(text)
  uschar *text;
{
  int ch;
  uschar *tail;

  tail = text + SCR_WIDTH;
  while (ch = *text)
  {
    switch (ch)
    {
    case KEY_ESC:
      ch = '*';
      break;
    }
    outc(ch);

    if (++text >= tail)
      break;
  }
}


int
ve_subject(row, topic, dft)
  int row;
  char *topic;
  char *dft;
{
  char *title;

  title = ve_title;

  if (topic)
  {
    sprintf(title, "Re: %s", str_ttl(topic));
    title[TTLEN] = '\0';
  }
  else
  {
    if (dft)
      strcpy(title, dft);
    else
      *title = '\0';
  }

  return vget(row, 0, "標題：", title, TTLEN + 1, GCARRY);
}


/* ----------------------------------------------------- */
/* 編輯處理：主程式、鍵盤處理				 */
/* ----------------------------------------------------- */

/* ----------------------------------------------------- */
/* vedit 回傳 -1:取消編輯 0:完成編輯			 */
/* ----------------------------------------------------- */
/* ve_op:						 */
/*  0 => 純粹編輯檔案					 */
/* -1 => 編輯但不能儲存，用在編輯作者不是自己的文章	 */
/*  1 => 引文、加簽名檔，並加上檔頭，用在發表文章/站內信 */
/*  2 => 引文、加簽名檔，不加上檔頭，用在寄站外信	 */
/* ----------------------------------------------------- */
/* 若 ve_op 是 1 或 2 時，進入 vedit 前還得指定 curredit */
/* 和 quote_file					 */
/* ----------------------------------------------------- */

int		/* -1:取消編輯 0:完成編輯 */
vedit(fpath, ve_op)
  char *fpath;
  int ve_op;	/* 0:純粹編輯檔案  -1:編輯但不能儲存  1:quote/header  2:quote */
{
  textline *vln, *tmp;
  int cc, col, mode, margin, pos;

  /* --------------------------------------------------- */
  /* 初始設定：載入檔案、引用文章、設定編輯模式		 */
  /* --------------------------------------------------- */

  tmp = vln = ve_alloc();

  if (*fpath)
  {
    cc = open(fpath, O_RDONLY);
    if (cc >= 0)
    {
      vln = ve_load(vln, cc);
    }
    else
    {
      cc = open(fpath, O_WRONLY | O_CREAT, 0600);
      if (cc < 0)
      {
	ve_abort(4);
	abort_bbs();
      }
    }
    close(cc);
  }

  /* if (ve_op) */
  if (ve_op > 0)	/* itoc.010301: 新增 ve_op = -1 時不能儲存 */
  {
    ve_quote(vln);
  }

  if (vln = tmp->next)
  {
    free(tmp);
    vln->prev = NULL;
  }
  else
  {
    vln = tmp;
  }

  vx_cur = vx_top = vx_ini = vln;

  ve_col = ve_row = margin = 0;
  ve_lno = 1;
  ve_mode = VE_INSERT | VE_REDRAW | VE_FOOTER;

#ifdef HAVE_MULTI_BYTE
  zhc = (cuser.ufo & UFO_ZHC);
#endif

  /* --------------------------------------------------- */
  /* 主迴圈：螢幕顯示、鍵盤處理、檔案處理		 */
  /* --------------------------------------------------- */

  clear();

  for (;;)
  {
    vln = vx_cur;
    mode = ve_mode;
    col = ve_col;
    /* itoc.031123.註解: 如果超過 SCR_WIDTH，那麼頁面往右翻，並保留左頁的最後 4 字 */
    cc = (col <= SCR_WIDTH) ? 0 : (col / (SCR_WIDTH - 4)) * (SCR_WIDTH - 4);
    if (cc != margin)
    {
      mode |= VE_REDRAW;
      margin = cc;
    }

    if (mode & VE_REDRAW)
    {
      ve_mode = (mode ^= VE_REDRAW);

      tmp = vx_top;

      for (pos = 0;; pos++)
      {
	move(pos, 0);
	clrtoeol();
	if (pos == b_lines)
	  break;
	if (tmp)
	{
	  if (mode & VE_ANSI)
	    outx(tmp->data);
	  else if (tmp->len > margin)
	    ve_outs(tmp->data + margin);
	  tmp = tmp->next;
	}
	else
	{
	  outc('~');
	}
      }
#ifdef EVERY_BIFF
      if (!(mode & VE_BIFF))
      {
	if (HAS_STATUS(STATUS_BIFF))
	  ve_mode = mode |= VE_BIFF;
      }
#endif
    }
    else
    {
      move(ve_row, 0);
      if (mode & VE_ANSI)
	outx(vln->data);
      else if (vln->len > margin)
	ve_outs(vln->data + margin);
      clrtoeol();
    }

    /* ------------------------------------------------- */
    /* 顯示狀態、讀取鍵盤				 */
    /* ------------------------------------------------- */

    if (mode & VE_ANSI)		/* Thor: 作 ansi 編輯 */
      pos = n2ansi(col, vln);	/* Thor: ansi 不會用到cc */
    else			/* Thor: 不是ansi要作margin shift */
      pos = col - margin;

    if (mode & VE_FOOTER)
    {
      move(b_lines, 0);
      clrtoeol();

      if (cuser.ufo & UFO_VEDIT)
      {
	ve_mode = (mode ^= VE_FOOTER);
      }
      else
      {
	prints(FOOTER_VEDIT,
#ifdef EVERY_BIFF
	  mode & VE_BIFF ? "郵差來了" : "編輯文章",
#else
	  "編輯文章", 
#endif
	  mode & VE_INSERT ? "插入" : "取代",
	  mode & VE_ANSI ? "ANSI" : "一般",
	  ve_lno, 1 + (mode & VE_ANSI ? pos : col));
      }
    }

    move(ve_row, pos);

ve_key:

    cc = vkey();

    if (isprint2(cc))
    {
      ve_char(cc);
    }
    else
    {
      switch (cc)
      {
      case '\n':

	ve_split(vln, col);
	break;

      case KEY_TAB:

	do
	{
	  ve_char(' ');
	} while (ve_col & (TAB_STOP - 1));
	break;

      case KEY_INS:		/* Toggle insert/overwrite */

	ve_mode = mode ^ VE_INSERT;
	continue;

      case KEY_BKSP:		/* backspace */

	/* Thor: 在 ANSI 編輯模式下, 不可以按倒退, 不然會很可怕.... */

	if (mode & VE_ANSI)
	{	
#if 0
	  goto ve_key;	/* 按後退鍵就當沒按 */	  
#endif

	  /* itoc.010322: ANSI 編輯時按後退鍵回到非 ANSI 模式 */
  	  mode ^= VE_ANSI;
	  clear();
	  ve_mode = mode | VE_REDRAW;
	  continue;	  
	}

	if (col)
	{
	  delete_char(vln, --col);
#ifdef HAVE_MULTI_BYTE
	  /* hightman.060504: 判斷現在刪除的位置是否為漢字的後半段，若是刪二字元 */
	  if (zhc && col && IS_ZHC_LO(vln->data, col))
	    delete_char(vln, --col);
#endif
	  ve_col = col;
	  continue;
	}

	if (!(tmp = vln->prev))
	  goto ve_key;

	ve_row--;
	ve_lno--;
	vx_cur = tmp;
	ve_col = tmp->len;
	if (*ve_strim(vln->data))
	  join_up(tmp);
	else
	  delete_line(vln);
	ve_mode = mode | VE_REDRAW;
	break;

      case Ctrl('D'):
      case KEY_DEL:		/* delete current character */

	cc = vln->len;
	if (cc == col)
	{
	  join_up(vln);
	  ve_mode = mode | VE_REDRAW;
	}
	else
	{
	  if (cc == 0)
	    goto ve_key;
#ifdef HAVE_MULTI_BYTE
	  /* hightman.060504: 判斷現在刪除的位置是否為漢字的前半段，若是刪二字元 */
	  /* 注意原有的雙色字刪除後可能出問題，暫時不作另行處理 */
	  if (zhc && col < cc - 1 && IS_ZHC_HI(vln->data[col]))
	    delete_char(vln, col);
#endif
	  delete_char(vln, col);
	  if (mode & VE_ANSI)	/* Thor: 雖然增加 load, 不過edit 時會比較好看 */
	    ve_col = ansi2n(n2ansi(col, vln), vln);
	}
	continue;

      case KEY_LEFT:

	if (col)
	{
	  ve_col = (mode & VE_ANSI) ? ansi2n(pos - 1, vln) : col - 1;
#ifdef HAVE_MULTI_BYTE
	  /* hightman.060504: 左移時碰到漢字移雙格 */
	  if (zhc && ve_col && IS_ZHC_LO(vln->data, ve_col))
	    ve_col--;
#endif
	  continue;
	}

	if (!(tmp = vln->prev))
	  goto ve_key;

	ve_row--;
	ve_lno--;
	ve_col = tmp->len;
	vx_cur = tmp;
	break;

      case KEY_RIGHT:

	if (col < vln->len)
	{
	  ve_col = (mode & VE_ANSI) ? ansi2n(pos + 1, vln) : col + 1;
#ifdef HAVE_MULTI_BYTE
	  /* hightman.060504: 右移時碰到漢字移雙格 */
	  if (zhc && ve_col < vln->len && IS_ZHC_HI(vln->data[ve_col - 1]))
	    ve_col++;
#endif
	  continue;
	}

	if (!(tmp = vln->next))
	  goto ve_key;

	ve_row++;
	ve_lno++;
	ve_col = 0;
	vx_cur = tmp;
	break;

      case KEY_HOME:
      case Ctrl('A'):

	ve_col = 0;
	continue;

      case KEY_END:
      case Ctrl('E'):

	ve_col = vln->len;
	continue;

      case KEY_UP:
      case Ctrl('P'):

	if (!(tmp = vln->prev))
	  goto ve_key;

	ve_row--;
	ve_lno--;
	if (mode & VE_ANSI)
	{
	  ve_col = ansi2n(pos, tmp);
	}
	else
	{
	  cc = tmp->len;
	  if (col > cc)
	    ve_col = cc;
	}
	vx_cur = tmp;
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: 漢字整字調節 */
	if (zhc && ve_col < tmp->len && IS_ZHC_LO(tmp->data, ve_col))
	  ve_col++;
#endif
	break;

      case KEY_DOWN:
      case Ctrl('N'):

	if (!(tmp = vln->next))
	  goto ve_key;

	ve_row++;
	ve_lno++;
	if (mode & VE_ANSI)
	{
	  ve_col = ansi2n(pos, tmp);
	}
	else
	{
	  cc = tmp->len;
	  if (col > cc)
	    ve_col = cc;
	}
	vx_cur = tmp;
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: 漢字整字調節 */
	if (zhc && ve_col < tmp->len && IS_ZHC_LO(tmp->data, ve_col))
	  ve_col++;
#endif
	break;

      case KEY_PGUP:
      case Ctrl('B'):

	ve_pageup();
	continue;

      case KEY_PGDN:
      case Ctrl('F'):
      case Ctrl('T'):		/* tail of file */

	ve_forward(cc == Ctrl('T') ? -1 : PAGE_SCROLL);
	continue;

      case Ctrl('S'):		/* start of file */

	vx_cur = vx_top = vx_ini;
	ve_col = ve_row = 0;
	ve_lno = 1;
	ve_mode = mode | VE_REDRAW;
	continue;

      case Ctrl('V'):		/* Toggle ANSI color */

	mode ^= VE_ANSI;
	clear();
	ve_mode = mode | VE_REDRAW;
	continue;

      case Ctrl('X'):		/* Save and exit */

	/* cc = ve_filer(fpath, ve_op & 1); */
	cc = ve_filer(fpath, ve_op);	/* itoc.010301: 新增 ve_op = -1 時不能儲存 */
	if (cc <= 0)
	  return cc;
	ve_mode = mode | cc;
	continue;

      case Ctrl('Z'):

	cutmp->status |= STATUS_EDITHELP;
	xo_help("post");
	cutmp->status ^= STATUS_EDITHELP;
	ve_mode = mode | VE_REDRAW;
	continue;

      case Ctrl('C'):

	ve_ansi();
	break;

      case Ctrl('O'):		/* delete to end of file */

	/* vln->len = ve_col = cc = 0; */
	tmp = vln->next;
	vln->next = NULL;
	while (tmp)
	{
	  vln = tmp->next;
	  free(tmp);
	  tmp = vln;
	}
	ve_mode = mode | VE_REDRAW;
	continue;

      case Ctrl('Y'):		/* delete current line */

	vln->len = ve_col = 0;
	vln->data[0] = '\0'; /* Thor.981001: 將內容一併清除 */

      case Ctrl('K'):		/* delete to end of line */

	if (cc = vln->len)
	{
	  if (cc != col)
	  {
	    vln->len = col;
	    vln->data[col] = '\0';
	    continue;
	  }

	  join_up(vln);
	}
	else
	{
	  tmp = vln->next;
	  if (!tmp)
	  {
	    tmp = vln->prev;
	    if (!tmp)
	      break;

	    if (ve_row > 0)
	    {
	      ve_row--;
	      ve_lno--;
	    }
	  }
	  if (vln == vx_top)
	    vx_top = tmp;
	  delete_line(vln);
	  vx_cur = tmp;
	}

	ve_mode = mode | VE_REDRAW;
	break;

      case Ctrl('U'):

	ve_char(KEY_ESC);
	break;

#ifdef SHOW_USER_IN_TEXT
      case Ctrl('Q'):
	cc = vans("顯示使用者資料(1)id (2)暱稱？");
	if (cc >= '1' && cc <= '2')
	{
	  ve_char(KEY_ESC);
	  ve_char('*');
	  ve_char("sn"[cc - '1']);
	}
	ve_mode = mode | VE_FOOTER;
	break;
#endif

#ifdef INPUT_TOOLS
      case Ctrl('W'):

	input_tools();
	ve_mode = mode | VE_FOOTER;
	break;
#endif

      default:

	goto ve_key;
      }
    }

    /* ------------------------------------------------- */
    /* ve_row / ve_lno 調整				 */
    /* ------------------------------------------------- */

    cc = ve_row;
    if (cc < 0)
    {
      ve_row = 0;
      if (vln = vx_top->prev)
      {
	vx_top = vln;
	rscroll();
      }
      else
      {
	ve_abort(6);
      }
    }
    else if (cc >= b_lines)
    {
      ve_row = b_lines - 1;
      if (vln = vx_top->next)
      {
	vx_top = vln;
	scroll();
      }
      else
      {
	ve_abort(7);
      }
    }
  }
}
