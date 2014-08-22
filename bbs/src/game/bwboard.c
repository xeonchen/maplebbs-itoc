/*-------------------------------------------------------*/
/* bwboard.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : B/W & Chinese Chess Board			 */
/* create : 02/08/05					 */
/* update :   /  /  					 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_GAME


/*-------------------------------------------------------*/
/* 所有棋盤公用的參數					 */
/*-------------------------------------------------------*/


extern char lastcmd[MAXLASTCMD][80];


enum
{
  DISCONNECT = -2, LEAVE = -1, NOTHING = 0
};


enum
{
  Binit = 0, Bupdate = 1, Ballow = 2
};


enum
{
  Empty = 0, Black = 1, White = 2, Red = 2
};


static int msgline;		/* Where to display message now */
static int cfd;			/* socket number */
static int myColor;		/* my chess color */
static int Choose;		/* -3:黑白棋 -2:五子棋 -1:圍棋 0:軍棋 1:暗棋 */
static int dark_choose;		/* 暗棋是否已翻開第一子 */

static int (**rule) ();

static int bwRow, bwCol;
static char Board[19][19];


static char *ruleStrSet[] = {"黑白棋", "五子棋", "圍棋", "軍棋", "暗棋"};
static char *ruleStr;

static KeyFunc *mapTalk, *mapTurn;

static int cmdCol, cmdPos;
static char talkBuf[42] = "T";
static char *cmdBuf = &talkBuf[1];
static KeyFunc Talk[];


static void bw_printmsg();
static void ch_printmsg();
static void do_init();


static int
do_send(buf)
  char *buf;
{
  int len;

  len = strlen(buf) + 1;	/* with trail 0 */
  return (send(cfd, buf, len, 0) == len);
}


#ifdef EVERY_BIFF
static void
check_biff()
{
  /* Thor.980805: 有人在旁邊按enter才需要check biff */
  static int old_biff;
  int biff;
  char *msg = "◆ 噹! 郵差來按鈴了!";

  biff = cutmp->status & STATUS_BIFF;
  if (biff && !old_biff)
    (Choose < 0) ? bw_printmsg(msg) : ch_printmsg(1, msg);
  old_biff = biff;
}
#endif


static int
fTAB()
{
  mapTalk = mapTalk ? NULL : Talk;
  return NOTHING;
}


static int
fNoOp()
{
  return NOTHING;
}


static int
fCtrlD()
{
  /* send Q.....\0 cmd */
  if (!do_send("Q"))
    return DISCONNECT;
  return LEAVE;
}


static int
ftkCtrlC()
{
  *cmdBuf = '\0';
  cmdCol = 0;
  move(b_lines - 2, 35);
  clrtoeol();
  return NOTHING;
}


static int
ftkCtrlH()
{
  if (cmdCol)
  {
    int ch = cmdCol--;
    memcpy(&cmdBuf[cmdCol], &cmdBuf[ch], sizeof(talkBuf) - ch - 1);
    move(b_lines - 2, cmdCol + 35);
    outs(&cmdBuf[cmdCol]);
    clrtoeol();
  }
  return NOTHING;
}


static int
ftkEnter()
{
  char msg[80];

#ifdef EVERY_BIFF
  check_biff();
#endif

  if (*cmdBuf)
  {
    for (cmdPos = MAXLASTCMD - 1; cmdPos; cmdPos--)
      strcpy(lastcmd[cmdPos], lastcmd[cmdPos - 1]);
    strcpy(lastcmd[0], cmdBuf);

    if (!do_send(talkBuf))
      return DISCONNECT;

    sprintf(msg, "\033[1;36m☆%s\033[m", cmdBuf);
    (Choose < 0) ? bw_printmsg(msg) : ch_printmsg(1, msg);

    *cmdBuf = '\0';
    cmdCol = 0;
    cmdPos = -1;
    move(b_lines - 2, 35);
    clrtoeol();
  }
  return NOTHING;
}


static int
ftkLEFT()
{
  if (cmdCol)
    --cmdCol;
  return NOTHING;
}


static int
ftkRIGHT()
{
  if (cmdBuf[cmdCol])
    ++cmdCol;
  return NOTHING;
}


static int
ftkUP()
{
  cmdPos++;
  cmdPos %= MAXLASTCMD;
  str_ncpy(cmdBuf, lastcmd[cmdPos], 41);
  move(b_lines - 2, 35);
  outs(cmdBuf);
  clrtoeol();
  cmdCol = strlen(cmdBuf);
  return NOTHING;
}


static int
ftkDOWN()
{
  cmdPos += MAXLASTCMD - 2;
  return ftkUP();
}


static int
ftkDefault(ch)
  int ch;
{
  if (isprint2(ch))
  {
    if (cmdCol < 40)
    {
      if (cmdBuf[cmdCol])
      {				/* insert */
	int i;
	for (i = cmdCol; cmdBuf[i] && i < 39; i++);
	cmdBuf[i + 1] = '\0';
	for (; i > cmdCol; i--)
	  cmdBuf[i] = cmdBuf[i - 1];
      }
      else
      {				/* append */
	cmdBuf[cmdCol + 1] = '\0';
      }
      cmdBuf[cmdCol] = ch;
      move(b_lines - 2, cmdCol + 35);
      outs(&cmdBuf[cmdCol++]);
    }
  }
  return NOTHING;
}


static KeyFunc Talk[] =
{
  Ctrl('C'), ftkCtrlC,
  Ctrl('D'), fCtrlD,
  Ctrl('H'), ftkCtrlH,
  '\n', ftkEnter,
  KEY_LEFT, ftkLEFT,
  KEY_RIGHT, ftkRIGHT,
  KEY_UP, ftkUP,
  KEY_DOWN, ftkDOWN,
  KEY_TAB, fTAB,
  0, ftkDefault
};


/*-------------------------------------------------------*/
/* target : 戰績記錄程式				 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


static void
play_count(userid, total, win)	/* 回傳 userid 的總場數、勝場數 */
  char *userid;
  int *total, *win;
{
  char fpath[64];
  int fd;
  int grade[32];	/* 保留 32 個 int */

  *total = 0;
  *win = 0;

  usr_fpath(fpath, userid, "bwboard");
  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    read(fd, grade, sizeof(grade));
    close(fd);

    /* 依序儲存 黑白總,黑白勝; 五子總,五子勝; 圍總,圍勝; 軍總,軍勝; 暗總,暗勝 */
    fd = (Choose + 3) << 1;
    *total = grade[fd];
    *win = grade[fd + 1];
  }
}


static void
play_add(win)			/* 我的總/勝場數 +1 */
  int win;
{
  char fpath[64];
  int fd, kind;
  int grade[32];	/* 保留 32 個 int */

  usr_fpath(fpath, cuser.userid, "bwboard");
  if ((fd = open(fpath, O_RDWR | O_CREAT, 0600)) >= 0)
  {
    if (read(fd, grade, sizeof(grade)) != sizeof(grade))
      memset(grade, 0, sizeof(grade));

    /* 依序儲存 黑白總,黑白勝; 五子總,五子勝; 圍總,圍勝; 軍總,軍勝; 暗總,暗勝 */
    kind = (Choose + 3) << 1;
    grade[kind]++;
    if (win)
      grade[kind + 1]++;

    lseek(fd, (off_t) 0, SEEK_SET);
    write(fd, grade, sizeof(grade));
    close(fd);
  }
}


/*-------------------------------------------------------*/
/* target : Black & White Chess Board 黑白棋/五子棋/圍棋 */
/* create : 99/02/20					 */
/* update : 02/08/05					 */
/* author : thor.bbs@bbs.cs.nthu.edu.tw			 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


enum
{
  Deny = 3
};


static KeyFunc myRound[], yourRound[];

static char *bw_icon[] = {"┼", "●", "○", "  "};	/* Empty, Black, White, Deny */


/* 19 * 19, standard chess board */
static char Allow[19][19];
static int numWhite, numBlack;
static int rulerRow, rulerCol;

static int yourPass, myPass;
static int yourStep, myStep;

static int GameOver;			/* end game */


#if 0

  % rules util include
  0. Board initialize
     (happen before start, and clear board action)
  1. Board update by "color" down a "chess pos"(without update screen)
     (screen by a main redraw routine, after call this)
     return value, represent win side to end(for both turn)
     (happen when state change)
  2. Board allow step checking by "color"(for my turn)
     return value, represent

  % possible step
     (happen when state change)
  if Game is over, won''t allow any step

#endif


/* numWhite, numBlack maintained by rule, 0&1 */
/* Board[19][19] Allow[19][19] maintained by rule */


static inline void
countBWnum()
{
  int i, j;

  numWhite = numBlack = 0;
  for (i = 0; i < 19; i++)
  {
    for (j = 0; j < 19; j++)
    {
      if (Board[i][j] == White)
	numWhite++;
      else if (Board[i][j] == Black)
	numBlack++;
    }
  }
}


  /*-----------------------------------------------------*/
  /* 黑白棋 8 x 8					 */
  /*-----------------------------------------------------*/

static int
othInit()
{
  int i, j;
  for (i = 0; i < 19; i++)
    for (j = 0; j < 19; j++)
      Board[i][j] = i < 8 && j < 8 ? Empty : Deny;
  Board[3][3] = Board[4][4] = Black;
  Board[3][4] = Board[4][3] = White;
  numWhite = numBlack = 2;
  rulerRow = rulerCol = 8;

  return 0;
}


static inline int
othEatable(Color, row, col, rowstep, colstep)
  int Color, row, col, rowstep, colstep;
{
  int eat = 0;
  do
  {
    row += rowstep;
    col += colstep;
    /* check range */
    if (row < 0 || row >= 8 || col < 0 || col >= 8)
      return 0;
    if (Board[row][col] == Color)
      return eat;
    eat = 1;
  } while (Board[row][col] == Deny - Color);
  return 0;
}


static int othAllow();


static int
othUpdate(Color, row, col)
  int Color, row, col;
{
  int i, j, p, q;
  int winside = Empty;

  Board[row][col] = Color;
  for (i = -1; i <= 1; i++)
  {
    for (j = -1; j <= 1; j++)
    {
      if (i != 0 || j != 0)
      {
	if (othEatable(Color, row, col, i, j))
	{
	  p = row + i;
	  q = col + j;
	  for (;;)
	  {
	    if (Board[p][q] == Color)
	      break;
	    Board[p][q] = Color;
	    p += i;
	    q += j;
	  }
	}
      }
    }
  }

  /* count numWhite & numBlack */
  countBWnum();

  /* Thor.990329.註解: 下滿時 */
  {
    int my = myColor;		/* Thor.990331: 暫存myColor */
    int allowBlack, allowWhite;
    myColor = Black;
    allowBlack = othAllow();
    myColor = White;
    allowWhite = othAllow();
    myColor = my;
    if (allowBlack == 0 && allowWhite == 0)
    {
      if (numWhite > numBlack)
	winside = White;
      else if (numWhite < numBlack)
	winside = Black;
      else
	winside = Deny;
    }
  }

  return winside;
}


static void
do_othAllow(i, j, num)
  int i, j;
  int *num;
{
  int p, q;

  Allow[i][j] = 0;
  if (Board[i][j] == Empty)
  {
    for (p = -1; p <= 1; p++)
    {
      for (q = -1; q <= 1; q++)
      {
	if (p != 0 || q != 0)
	{
	  if (othEatable(myColor, i, j, p, q))
	  {
	    Allow[i][j] = 1;
	    (*num)++;
	    return;
	  }
	}
      }
    }
  }
}


static int
othAllow()
{
  int i, j, num;

  num = 0;
  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      do_othAllow(i, j, &num);
    }
  }

  return num;
}


static int (*othRule[]) () =
{
  othInit, othUpdate, othAllow
};


  /*-----------------------------------------------------*/
  /* 五子棋 15 x 15					 */
  /*-----------------------------------------------------*/

static int
fivInit()
{
  int i, j;
  for (i = 0; i < 19; i++)
  {
    for (j = 0; j < 19; j++)
      Board[i][j] = i < 15 && j < 15 ? Empty : Deny;
  }
  numWhite = numBlack = 0;
  rulerRow = rulerCol = 15;
  return 0;
}


static int
fivCount(Color, row, col, rowstep, colstep)
  int Color, row, col, rowstep, colstep;
{
  int count = 0;
  for (;;)
  {
    row += rowstep;
    col += colstep;
    /* check range */
    if (row < 0 || row >= 15 || col < 0 || col >= 15)
      return count;

    if (Board[row][col] != Color)
      return count;
    count++;
  }
}


static int
fivUpdate(Color, row, col)
  int Color, row, col;
{
#if 0
  int cnt[4], n3, n4, n5, nL, i;
#endif

  int winside = Empty;
  Board[row][col] = Color;
  if (Color == Black)
    numBlack++;
  else if (Color == White)
    numWhite++;


  /* 黑棋（先著者）有下列三著禁著(又稱禁手)點：三三(雙活三)、四四、長連。 */
  /* 在連五之前形成禁著者，裁定為禁著負。                                 */
  /* 白棋沒有禁著點，長連或者三三也都可以勝。                             */

#if 0
  cnt[0] = fivCount(Color, row, col, -1, -1) + fivCount(Color, row, col, +1, +1) + 1;
  cnt[1] = fivCount(Color, row, col, -1, 0) + fivCount(Color, row, col, +1, 0) + 1;
  cnt[2] = fivCount(Color, row, col, 0, -1) + fivCount(Color, row, col, 0, +1) + 1;
  cnt[3] = fivCount(Color, row, col, -1, +1) + fivCount(Color, row, col, +1, -1) + 1;

  n3 = 0;			/* 雙活三 */
  n4 = 0;			/* 雙四 */
  n5 = 0;			/* 五 */
  nL = 0;			/* 長連 */

  for (i = 0; i < 4; i++)
  {
    if (cnt[i] == (3 | (LIVE_SIDE + LIVE_SIDE)))
      n3++;
    if ((cnt[i] % LIVE_SIDE) == 4)
      n4++;
    if ((cnt[i] % LIVE_SIDE) == 5)
      n5++;
    if ((cnt[i] % LIVE_SIDE) > 5)
      nL++;
  }

  if (n5 > 0)
    winside = Color;
  else
  {
    if (Color == Black)
    {
      if (n3 >= 2)
      {
	bw_printmsg("◆ 黑方雙三禁著");
	winside = White;
      }
      if (n4 >= 2)
      {
	bw_printmsg("◆ 黑方雙四禁著");
	winside = White;
      }
      if (nL > 0)
      {
	bw_printmsg("◆ 黑方長連禁著");
	winside = White;
      }
    }
    else
    {
      if (nL > 0)
	winside = Color;
    }
  }
#endif

#if 0   /* Thor.990415: 上面那段又寫錯了, 留待有心人士再慢慢寫吧 :p */

    (一)●  ●  ●  ●
              ↑
            再放進去就算四四
                ↓
    (二)●●●      ●●●(中間空三格)

    (三)●  ●●○
          ↑放這裡也算
              ●
                ●
                  ●

    (四)●●  ●    ●●
                ↑再放就是四四


    長連的話,只要五子以上就算,不管死活

#endif

#if 1
  if (fivCount(Color, row, col, -1, -1) + fivCount(Color, row, col, +1, +1) >= 4 ||
    fivCount(Color, row, col, -1, 0) + fivCount(Color, row, col, +1, 0) >= 4 ||
    fivCount(Color, row, col, 0, -1) + fivCount(Color, row, col, 0, +1) >= 4 ||
    fivCount(Color, row, col, -1, +1) + fivCount(Color, row, col, +1, -1) >= 4)
    winside = Color;
#endif

  return winside;
}


static int
fivAllow()
{
  int i, j, num = 0;
  for (i = 0; i < 19; i++)
  {
    for (j = 0; j < 19; j++)
      num += Allow[i][j] = (Board[i][j] == Empty);
  }
  return num;
}


static int (*fivRule[]) () =
{
  fivInit, fivUpdate, fivAllow
};


  /*-----------------------------------------------------*/
  /* 圍棋 19 x 19					 */
  /*-----------------------------------------------------*/

static int
blkInit()
{
  memset(Board, 0, sizeof(Board));
  numWhite = numBlack = 0;
  rulerRow = rulerCol = 18;
  return 0;
}


/* borrow Allow for traversal, and return region */
/* a recursive procedure, clear Allow before call it */
/* with row,col range check, return false if out */
static int
blkLive(Color, row, col)
  int Color, row, col;
{
  if (row < 0 || row >= 19 || col < 0 || col >= 19)
    return 0;
  if (Board[row][col] == Empty)
    return 1;
  if (Board[row][col] != Color)
    return 0;
  if (Allow[row][col])
    return 0;
  Allow[row][col] = 1;
  return blkLive(Color, row - 1, col) |
    blkLive(Color, row + 1, col) |
    blkLive(Color, row, col - 1) |
    blkLive(Color, row, col + 1);
}


static inline void
blkClear()
{
  int i, j;

  for (i = 0; i < 19; i++)
  {
    for (j = 0; j < 19; j++)
    {
      if (Allow[i][j])
	Board[i][j] = Empty;
    }
  }
}


static int
blkUpdate(Color, row, col)
  int Color, row, col;
{
  Board[row][col] = Color;

  memset(Allow, 0, sizeof(Allow));
  if (!blkLive(Deny - Color, row - 1, col))
    blkClear();

  memset(Allow, 0, sizeof(Allow));
  if (!blkLive(Deny - Color, row + 1, col))
    blkClear();

  memset(Allow, 0, sizeof(Allow));
  if (!blkLive(Deny - Color, row, col - 1))
    blkClear();

  memset(Allow, 0, sizeof(Allow));
  if (!blkLive(Deny - Color, row, col + 1))
    blkClear();

  /* check for suiside */
  memset(Allow, 0, sizeof(Allow));
  if (!blkLive(Color, row, col))
    blkClear();

  /* count numWhite & numBlack */
  countBWnum();

  return Empty;		/* Please check win side by your own */
}


static int (*blkRule[]) () =
{
  blkInit, blkUpdate, fivAllow	/* borrow fivAllow as blkAllow */
};


  /*-----------------------------------------------------*/
  /* board util						 */
  /*-----------------------------------------------------*/

#if 0				/* screen */

  [maple BWboard]
  xxxx vs yyyy
  ++++++++ talkline(you color, yellow) (40 chars)
  ++++++++ talkline(my color, cryn)
  ++++++++
  ++++++++
  ++++++++

  one line for simple help, press key to......
  one line for nth turn, myColor, num, pass < -youcolor, num, pass(35), input talk
  two line for write msg

#endif


static char *
bw_brdline(row)
  int row;
{
  static char buf[80] = "\033[30;43m";
  static char rTxtY[] = " A B C D E F G H I J K L M N O P Q R S";
  static char rTxtX[] = " 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9";
  char txt[3];
  char *ptr = buf + 8, *t;
  int i;

  for (i = 0; i < 19; i++)
  {
    t = bw_icon[Board[row][i]];
    if (t == bw_icon[Empty])
    {
      if (row == 0)
      {
	if (i == 0)
	  t = "╔";
	else if (i >= 18 || Board[row][i + 1] == Deny)
	  t = "╗";
	else
	  t = "╤";
      }
      else if (row >= 18 || Board[row + 1][i] == Deny)
      {
	if (i == 0)
	  t = "╚";
	else if (i >= 18 || Board[row][i + 1] == Deny)
	  t = "╝";
	else
	  t = "╧";
      }
      else
      {
	if (i == 0)
	  t = "╟";
	else if (i >= 18 || Board[row][i + 1] == Deny)
	  t = "╢";
      }
    }
    if (t != bw_icon[Black] && t != bw_icon[White])
    {
      if (row == rulerRow && i < rulerCol)
      {
	str_ncpy(txt, rTxtX + 2 * i, 3);
	t = txt;
      }
      else if (i == rulerCol && row < rulerRow)
      {
	str_ncpy(txt, rTxtY + 2 * row, 3);
	t = txt;
      }
    }
    strcpy(ptr, t);
    ptr += 2;
  }
  strcpy(ptr, "\033[m");
  return buf;
}


static char *
bw_coorstr(row, col)
  int row;
  int col;
{
  static char coor[10];
  sprintf(coor, "(%c,%d)", row + 'A', col + 1);
  return coor;
}


static void
bw_printmsg(msg)
  char *msg;
{
  int line;

  line = msgline;
  move(line, 0);
  outs(bw_brdline(line - 1));
  outs(msg);
  clrtoeol();
  if (++line == b_lines - 3)	/* stop line */
    line = 1;
  move(line, 0);
  outs(bw_brdline(line - 1));
  outs("→");
  clrtoeol();
  msgline = line;
}


static void
bw_draw()
{
  int i, myNum, yourNum;
  for (i = 0; i < 19; i++)
  {
    move(1 + i, 0);
    outs(bw_brdline(i));
  }
  myNum = (myColor == Black) ? numBlack : numWhite;
  yourNum = (myColor == Black) ? numWhite : numBlack;

  move(b_lines - 2, 0);
  prints("第%d回 %s%d子%d讓 (%s) %s%d子%d讓",
    BMIN(myStep, yourStep) + 1, bw_icon[Deny - myColor], myNum, myPass,
    (mapTurn == myRound ? "←" : "→"), bw_icon[myColor], yourNum, yourPass);
  /* Thor.990219: 特別注意, 在此因顏色關係, 故用子的icon, 剛好與原本相反 */
  /* nth turn,myColor,num,pass <- youcolor, num,pass */
}


static void
bw_init()
{
  yourPass = myPass = yourStep = myStep = 0;

  /* 黑白棋/五子棋/圍棋為黑子先行 */
  if (myColor == Black)
  {
    (*rule[Ballow]) ();
    mapTurn = myRound;
  }
  else
  {
    mapTurn = yourRound;
  }

  move(b_lines - 1, 0);
  outs(COLOR1 " 對奕模式 " COLOR2 " (Enter)落子 (TAB)切換棋盤/交談 (^P)讓手 (^C)重玩 (^D)離開          \033[m");

  bw_draw();
}


static inline void
bw_overgame()
{
  if (GameOver == Black)
    bw_printmsg("\033[1;32m◆ 黑方獲勝\033[m");
  else if (GameOver == White)
    bw_printmsg("\033[1;32m◆ 白方獲勝\033[m");
  else if (GameOver == Deny)
    bw_printmsg("\033[1;32m◆ 雙方平手\033[m");

  play_add(myColor == GameOver);
}


#if 0	/* communication protocol */

  Ctrl('O'):

  enter BWboard mode, (pass to another)
  first hand specify rule set(pass Rule later)
  then start

  clear chess board, C.....\ 0
  talk line by line, T.....\ 0
  specify down pos, Dxxx \ 0, y = xxx / 19, x = xxx % 19
  pass one turn, P.....\ 0
  leave BWboard mode, Q.....\ 0

#endif


static inline int
bw_recv()
{
  static char buf[512];
  static int bufstart = 0;
  int cc, len;
  char *bptr, *str;
  char msg[80];
  int i;

  bptr = buf;
  cc = bufstart;
  len = sizeof(buf) - cc - 1;

  if ((len = recv(cfd, bptr + cc, len, 0)) <= 0)
    return DISCONNECT;

  cc += len;

  for (;;)
  {
    len = strlen(bptr);

    if (len >= cc)
    {				/* wait for trailing data */
      memcpy(buf, bptr, len);
      bufstart = len;
      break;
    }
    str = bptr + 1;
    switch (*bptr)
    {
      /* clear chess board, C.....\0 */
    case 'C':
      do_init();
      break;

      /* talk line by line, T.....\0 */
    case 'T':
      sprintf(msg, "\033[1;33m★%s\033[m", str);
      bw_printmsg(msg);
      break;

      /* specify down pos, Dxxx\0 , y = xxx / 19, x = xxx % 19 */
    case 'D':
      yourStep++;
      /* get pos */
      i = atoi(str);
      sprintf(msg, "◆ 對方落子 %s", bw_coorstr(i / 19, i % 19));
      /* update board */
      GameOver = (*rule[Bupdate]) (Deny - myColor, i / 19, i % 19);

      mapTurn = myRound;

      bw_draw();

      bw_printmsg(msg);

      if (GameOver)
      {
	bw_overgame();
	memset(Allow, 0, sizeof(Allow));
      }
      else
      {
	if ((*rule[Ballow]) () <= 0)
	  bw_printmsg("◆ 您走投無路了");
      }
      break;

      /* pass one turn, P.....\0 */
    case 'P':
      yourPass++;
      yourStep++;

      mapTurn = myRound;
      bw_draw();
      bw_printmsg("◆ 對方讓手");
      if (GameOver)
      {
	memset(Allow, 0, sizeof(Allow));
      }
      else
      {
	if ((*rule[Ballow]) () <= 0)
	  bw_printmsg("◆ 您走投無路了");	/* Thor.990329: ending game? */
      }
      break;

      /* leave BWboard mode, Q.....\0 */
    case 'Q':
      return LEAVE;
    }

    cc -= ++len;
    if (cc <= 0)
    {
      bufstart = 0;
      break;
    }
    bptr += len;
  }

  return NOTHING;
}


static int
ftnCtrlC()
{
  if (!do_send("C"))
    return DISCONNECT;
  do_init();
  return NOTHING;
}


static int
ftnUP()
{
  if (bwRow)
    bwRow--;
  return NOTHING;
}


static int
ftnDOWN()
{
  if (bwRow < 18)
    if (Board[bwRow + 1][bwCol] != Deny)
      bwRow++;
  return NOTHING;
}


static int
ftnLEFT()
{
  if (bwCol)
    bwCol--;
  return NOTHING;
}


static int
ftnRIGHT()
{
  if (bwCol < 18)
    if (Board[bwRow][bwCol + 1] != Deny)
      bwCol++;
  return NOTHING;
}


static int
ftnPass()
{
  /* Thor.990220: for chat mode to enter ^P pass */
  if (mapTurn == myRound)
  {
    myPass++;
    myStep++;
    if (!do_send("P"))
      return DISCONNECT;
    mapTurn = yourRound;
    bw_draw();
    bw_printmsg("◆ 我方讓手");
  }
  return NOTHING;
}


static int
ftnEnter()
{
  char msg[80];
  char buf[20];

  if (!Allow[bwRow][bwCol])
    return NOTHING;

  sprintf(msg, "◆ 我方落子 %s", bw_coorstr(bwRow, bwCol));

  myStep++;
  sprintf(buf, "D%d", bwRow * 19 + bwCol);

  if (!do_send(buf))
    return DISCONNECT;

  /* update board */
  GameOver = (*rule[Bupdate]) (myColor, bwRow, bwCol);

  mapTurn = yourRound;

  bw_draw();

  bw_printmsg(msg);

  if (GameOver)
    bw_overgame();

  return NOTHING;
}


static KeyFunc yourRound[] =
{
  Ctrl('C'), ftnCtrlC,
  Ctrl('D'), fCtrlD,
  KEY_LEFT, ftnLEFT,
  KEY_RIGHT, ftnRIGHT,
  KEY_UP, ftnUP,
  KEY_DOWN, ftnDOWN,
  KEY_TAB, fTAB,
  0, fNoOp
};


static KeyFunc myRound[] =
{
  Ctrl('C'), ftnCtrlC,
  ' ', ftnEnter,
  '\n', ftnEnter,
  Ctrl('P'), ftnPass,
  Ctrl('D'), fCtrlD,
  KEY_LEFT, ftnLEFT,
  KEY_RIGHT, ftnRIGHT,
  KEY_UP, ftnUP,
  KEY_DOWN, ftnDOWN,
  KEY_TAB, fTAB,
  0, fNoOp
};


/*-------------------------------------------------------*/
/* target : Chinese Chess Board 軍棋/暗棋		 */
/* create : 99/12/14					 */
/* update : 02/08/05					 */
/* author : weichung.bbs@bbs.ntit.edu.tw		 */
/* modify : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


enum
{
  Cover = 1
};


static KeyFunc myTurn[], yourTurn[];

static int sideline;
static int Totalch;		/* 加速用 :p */
static int Focus;
static int youreat_index;
static int myeat_index;

static char MyEat[16], YourEat[16];
static char Appear[14];

static char *ch_icon[] = 
{
  "  ", "●", 		/* Empty, Cover */
  "帥", "仕", "相", "硨", "傌", "炮", "兵", 
  "將", "士", "象", "車", "馬", "包", "卒"
};


  /*-----------------------------------------------------*/
  /* 軍棋 10 x 9					 */
  /*-----------------------------------------------------*/

static int
armyInit()
{
  int i, j;

  for (i = 0; i < 10; i++)
  {
    for (j = 0; j < 9; j++)
      Board[i][j] = Empty;
  }

  Board[0][4] = 2;			/* 帥 */
  Board[0][3] = Board[0][5] = 3;	/* 仕 */
  Board[0][2] = Board[0][6] = 4;	/* 相 */
  Board[0][1] = Board[0][7] = 6;	/* 傌 */
  Board[0][0] = Board[0][8] = 5;	/* 硨 */
  Board[2][1] = Board[2][7] = 7;	/* 炮 */
  Board[3][0] = Board[3][2] = Board[3][4] = Board[3][6] = Board[3][8] = 8;	/* 兵 */

  Board[9][4] = 9;			/* 將 */
  Board[9][3] = Board[9][5] = 10;	/* 士 */
  Board[9][2] = Board[9][6] = 11;	/* 象 */
  Board[9][1] = Board[9][7] = 13;	/* 馬 */
  Board[9][0] = Board[9][8] = 12;	/* 車 */
  Board[7][1] = Board[7][7] = 14;	/* 包 */
  Board[6][0] = Board[6][2] = Board[6][4] = Board[6][6] = Board[6][8] = 15;	/* 卒 */

  memset(MyEat, Empty, sizeof(MyEat));
  memset(YourEat, Empty, sizeof(YourEat));

  sideline = 19;
  return 0;
}      


static int (*armyRule[]) () =
{
  armyInit
};


  /*-----------------------------------------------------*/
  /* 暗棋 4 x 8						 */
  /*-----------------------------------------------------*/

static int
darkInit()
{
  int i, j;

  for (i = 0; i < 4; i++)
    for (j = 0; j < 8; j++)
      Board[i][j] = Cover;

  memset(Appear, Empty, sizeof(Appear));
  memset(MyEat, Empty, sizeof(MyEat));
  memset(YourEat, Empty, sizeof(YourEat));

  sideline = 9;
  return 0;
}      


static int (*darkRule[]) () =
{
  darkInit
};


  /*-----------------------------------------------------*/
  /* board util						 */
  /*-----------------------------------------------------*/


static void
ch_printmsg(type, msg)
  int type;
  char *msg;
{
  char buf[80];

  switch (type)
  {
  case 1:			/* for move uncover eat */
    move(msgline + 9, 37);
    outs(msg);
    clrtoeol();
    if (++msgline >= 11)
      msgline = 1;
    move(msgline + 9, 37);
    outs("→");
    clrtoeol();
    break;

  case 2:			/* for select */
    sprintf(buf, "\033[1;33m◆您選取了 %s(%d, %c)\033[m", 
      ch_icon[Focus / 256], bwCol, bwRow + 'A');
    move(1, 37);
    outs(buf);
    clrtoeol();
    break;

  case 3:			/* for disable select */
    move(1, 37);
    clrtoeol();
    break;
  }
}


static void
ch_printeat()
{
  int i;
  /* my:4, your:7 */

  if (myeat_index)
  {
    for (i = 0; i < myeat_index; i++)
    {
      move(4, 37 + i * 2);
      outs(ch_icon[MyEat[i]]);
    }
  }

  if (youreat_index)
  {
    for (i = 0; i < youreat_index; i++)
    {
      move(7, 37 + i * 2);
      outs(ch_icon[YourEat[i]]);
    }
  }
}


static void
ch_overgame(win)
  int win;
{
  char buf[80];

  sprintf(buf, "%s方獲勝！請按 Ctrl-C 重玩", win == myColor ? "我" : "對");
  ch_printmsg(1, buf);

  play_add(win == myColor);
}


static char *
ch_brdline(row)
  int row;
{
  char *t, *str, ch;
  static char buf[80];
  static char river[] = "│楚      河          漢      界│";
  static char side[] = " A B C D E F G H I J";
  int i;

  if (row > 8 && Choose)	/* 暗棋的 row 最多到 8 */
    return NULL;

  if (row == 9)			/* 軍棋的 row 9 是 楚河漢界 */
    return river;

  str = buf;

  /* 畫格子及格子上的棋子 */

  for (i = 0; i < 17; i++)
  {
    if (!Choose && (row % 2 || i % 2))
      ch = Empty;
    else
      ch = Board[row / 2][i / 2];

    if (Choose || (ch == Empty))
    {
      if (row == 0)					/* ┌─┬─┐ */
      {
	if (i == 0)
	  t = "┌";
	else if (i == 16)
	  t = "┐";
	else if (i % 2)
	  t = "─";
	else
	  t = "┬";
      }
      else if (row == 18 || (row == 8 && Choose))	/* └─┴─┘ */
      {
	if (i == 0)
	  t = "└";
	else if (i == 16)
	  t = "┘";
	else if (i % 2)
	  t = "─";
	else
	  t = "┴";
      }
      else if (row % 2)					/* │  │  │ */
      {
	if (i % 2)
	  t = ch_icon[ch];
	else
	  t = "│";
      }
      else						/* ├─┼─┤ */
      {
	if (i == 0)
	  t = "├";
	else if (i == 16)
	  t = "┤";
	else if (i % 2)
	  t = "─";
	else
	  t = "┼";
      }
    }
    else
    {
      t = ch_icon[ch];
    }
    strcpy(str, t);		/* color */
    str += 2;
  }				/* end for loop */

  if ((Choose && (row % 2 == 1)) || (!Choose && (row % 2 == 0)))
  {
    i = row / 2;
    strncpy(str, side + i * 2, 2);
  }

  return buf;
}


static void
ch_draw()
{
  int i;

  for (i = 0; i < sideline; i++)
  {
    move(1 + i, 0);
    outs(ch_brdline(i));
  }
  move(3, 37);
  outs("☆我方所吃之子");
  move(6, 37);
  outs("★對方所吃之子");
  move(9, 37);
  outs("====================================");

  move(sideline + 1, 0);
  if (Choose)
    outs("  Ｏ  １  ２  ３  ４  ５  ６  ７");
  else
    outs("Ｏ  １  ２  ３  ４  ５  ６  ７  ８");

  move(b_lines - 2, 0);
  prints("我是 \033[47;%s子\033[m [輪到%s]", 
    dark_choose ? (myColor == Black ? "30m黑" : "31m紅") : "30m白", 
    mapTurn == myTurn ? "我了" : "對方");
}


static void
ch_init()
{
  Totalch = youreat_index = myeat_index = Focus = 0;

  /* 暗棋的持子顏色未定 */
  dark_choose = (Choose == 1) ? 0 : 1;

  /* 軍棋/暗棋為黑子先行 (暗棋: 真正的持子顏色會在第一次翻子時決定) */
  mapTurn = (myColor == Black) ? myTurn : yourTurn;

  move(b_lines - 1, 0);
  outs(COLOR1 " 對奕模式 " COLOR2 " (Enter)翻子/移動 (TAB)切換棋盤/交談 (^S)認輸 (^C)重玩 (^D)離開     \033[m");

  ch_draw();
}


static inline int
ch_recv()
{
  static char buf[256];
  char msg[80];
  int len, ch, row, col, ch2, row2, col2;

  len = sizeof(buf) + 1;
  if ((len = recv(cfd, buf, len, 0)) <= 0)
    return DISCONNECT;

  switch (*buf)
  {
    /* clear chess board, C.....\0 */
  case 'C':
    do_init();
    break;

  case 'D':
    ch = atoi(buf + 1);
    row = (ch / 16) % 16;
    col = ch % 16;
    ch = ch / 256;

    Board[row][col] = ch;
    Appear[ch - 2]++;
    Totalch++;
    mapTurn = myTurn;
    sprintf(msg, "\033[1;32m△對方翻開 %s(%d, %c)\033[m", ch_icon[ch], col, row + 'A');
    ch_printmsg(1, msg);
    ch_draw();
    break;

  case 'T':
    sprintf(msg, "\033[1;33m★%s\033[m", buf + 1);
    ch_printmsg(1, msg);
    break;

  case 'E':
    ch = atoi(strtok(buf + 1, ":"));
    row = (ch / 16) % 16;
    col = ch % 16;
    ch = ch / 256;
    ch2 = atoi(strtok(NULL, ":"));
    row2 = (ch2 / 16) % 16;
    col2 = ch2 % 16;
    ch2 = ch2 / 256;

    Board[row][col] = Empty;
    Board[row2][col2] = ch;
    YourEat[youreat_index++] = ch2;
    mapTurn = myTurn;
    sprintf(msg, "\033[1;32m▽對方移動 %s(%d, %c) 吃 %s(%d, %c)\033[m",
      ch_icon[ch], col, row + 'A',
      ch_icon[ch2], col2, row2 + 'A');
    ch_printmsg(1, msg);
    ch_draw();
    ch_printeat();

    if (Choose)
    {
      if (youreat_index == 16)
	ch_overgame((myColor == Black) ? Red : Black);
    }
    else
    {
      if (ch2 == 2 || ch2 == 9)
	ch_overgame((myColor == Black) ? Red : Black);
    }
    break;

  case 'M':
    ch = atoi(strtok(buf + 1, ":"));
    row = (ch / 16) % 16;
    col = ch % 16;
    ch = ch / 256;
    ch2 = atoi(strtok(NULL, ":"));
    row2 = (ch2 / 16) % 16;
    col2 = ch2 % 16;
    ch2 = ch2 / 256;

    Board[row][col] = Empty;
    Board[row2][col2] = ch;
    mapTurn = myTurn;
    sprintf(msg, "\033[1;37m▽對方移動 %s(%d, %c) 至 (%d, %c)\033[m",
      ch_icon[ch], col, row + 'A', col2, row2 + 'A');
    ch_printmsg(1, msg);
    ch_draw();
    break;

  case 'F':
    dark_choose = 1;
    myColor = (atoi(buf + 1) == Black) ? Red : Black;
    ch_draw();
    break;

  case 'S':
    ch_overgame(myColor);
    break;

  case 'Q':
    return LEAVE;
  }

  return NOTHING;
}


static int 
ch_rand()
{
  int rd, i;
  char *index[] = {"1", "2", "2", "2", "2", "2", "5", "1", "2", "2", "2", "2", "2", "5"};

  if (Totalch == 31)		/* 避免剩最後一個時還要 random */
  {
    for (i = 0; i < 14; i++)
    {
      if (Appear[i] < atoi(index[i]))
      {
	i += 2;
	return i;
      }
    }
  }

  for (;;)
  {
    rd = rnd(16);

    if (rd < 2)
      continue;
    if (Appear[rd - 2] < atoi(index[rd - 2]))
    {
      Appear[rd - 2] += 1;
      Totalch++;
      break;
    }
    else
      continue;
  }
  return rd;
}


static int
ch_count(row, col)	/* 回傳包/炮與待吃物中間有幾一子 */
  int row, col;
{
  int count, start, end;

  if (bwRow != row && bwCol != col)	/* 必須在同一行或同一列跳 */
    return -1;

  count = 0;
  if (bwRow != row)
  {
    if (bwRow > row)
    {
      start = row + 1;
      end = bwRow;
    }
    else
    {
      start = bwRow + 1;
      end = row;
    }
    for (; start < end; start++)
    {
      if (Board[start][bwCol] != Empty)
	count++;
    }
  }
  else
  {
    if (bwCol > col)
    {
      start = col + 1;
      end = bwCol;
    }
    else
    {
      start = bwCol + 1;
      end = col;
    }
    for (; start < end; start++)
    {
      if (Board[bwRow][start] != Empty)
	count++;
    }
  }

  return count;
}


static int		/* 0:相同顏色 1:不同顏色 */
ch_check(ch)
  char ch;
{
  if ((myColor == Red && ch < 9) ||
    (myColor == Black && ch > 8))
    return 0;

  return 1;
}


static int
ch_Mv0()			/* for 軍棋 */
{
  int mych, yourch, way;	/* way: 0:NOTHING  1:move  2:eat */
  int row, col, Rdis, Cdis;
  char buf[80];

  row = (Focus / 16) % 16;
  col = Focus % 16;
  mych = Focus / 256;
  yourch = Board[bwRow][bwCol];

  Rdis = abs(row - bwRow);	/* displayment */
  Cdis = abs(col - bwCol);
  way = NOTHING;

  switch (mych)
  {
  case 2:	/* 帥將 */
  case 9:
    if (((bwCol >= 3 && bwCol <= 5) && (bwRow <= 2 || bwRow >= 7) && (Rdis + Cdis == 1)) ||	/* 限制在九宮格中，走一格 */
      (bwCol == col && abs(mych - yourch) == 7 && !ch_count(row, col)))				/* 王見王 */
    {
      if (yourch == Empty)
	way = 1;
      else if (ch_check(yourch))
	way = 2;
    }
    break;

  case 3:	/* 仕士 */
  case 10:
    if ((bwCol >= 3 && bwCol <= 5) && (bwRow <= 2 || bwRow >= 7) &&	/* 限制在九宮格中 */
      (Rdis == 1 && Cdis == 1))						/* 走一斜 */
    {
      if (yourch == Empty)
	way = 1;
      else if (ch_check(yourch))
	way = 2;
    }
    break;

  case 4:	/* 相象 */
  case 11:
    if (((bwRow <= 4 && myColor == Red) || (bwRow >= 5 && myColor == Black)) &&	/* 限制不能過河 */
      (Rdis == 2 && Cdis == 2) &&					/* 走一田 */
      (Board[(bwRow + row) / 2][(bwCol + col) / 2] == Empty))		/* 不能拐象腳 */
    {
      if (yourch == Empty)
	way = 1;
      else if (ch_check(yourch))
	way = 2;
    }
    break;

  case 5:	/* 硨車 */
  case 12:
    if (!ch_count(row, col) && (row == bwRow || col == bwCol))	/* 走一線 */
    {
      if (yourch == Empty)
	way = 1;
      else if (ch_check(yourch))
	way = 2;
    }
    break;

  case 6:	/* 傌馬 */
  case 13:
    if ((Rdis == 2 && Cdis == 1 && Board[(bwRow + row) / 2][col] == Empty) ||	/* 走一拐 */
      (Rdis == 1 && Cdis == 2 && Board[row][(bwCol + col) / 2] == Empty))	/* 不能拐馬腳 */
    {
      if (yourch == Empty)
	way = 1;
      else if (ch_check(yourch))
	way = 2;
    }
    break;

  case 7:	/* 炮包 */
  case 14:
    if (row == bwRow || col == bwCol)				/* 走一線 */
    {
      Rdis = ch_count(row, col);	/* 借用 Rdis */
      if (Rdis == 0 && yourch == Empty)
	way = 1;
      else if (Rdis == 1 && yourch != Empty && ch_check(yourch))
	way = 2;
    }
    break;

  case 8:	/* 兵卒 */
  case 15:
    if (Rdis + Cdis != 1)	/* 走一格 */
      break;

    if (myColor == Red)
    {
      if ((bwRow < row) || 		/* 不能走回頭步 */
	(row <= 4 && col != bwCol))	/* 在國內只能走直的 */
	break;
    }
    else
    {
      if ((bwRow > row) ||		/* 不能走回頭步 */
	(row >= 5 && col != bwCol))	/* 在國內只能走直的 */
	break;
    }

    if (yourch == Empty)
      way = 1;
    else if (ch_check(yourch))
      way = 2;
    break;
  }		/* end switch */

  if (way == 1)
  {
    sprintf(buf, "M%d:%d", Focus, bwRow * 16 + bwCol);
    if (!do_send(buf))
      return DISCONNECT;
    sprintf(buf, "\033[1;36m▼我方移動 %s(%d, %c) 至 (%d, %c)\033[m",
      ch_icon[mych], col, row + 'A', bwCol, bwRow + 'A');
  }
  else if (way == 2)
  {
    sprintf(buf, "E%d:%d", Focus, yourch * 256 + bwRow * 16 + bwCol);
    if (!do_send(buf))
      return DISCONNECT;
    sprintf(buf, "\033[1;32m▼我方移動 %s(%d, %c) 吃 %s(%d, %c)\033[m",
      ch_icon[mych], col, col + 'A', ch_icon[yourch], bwCol, bwRow + 'A');
    MyEat[myeat_index++] = yourch;    
    ch_printeat();
  }

  if (way)
  {
    Board[bwRow][bwCol] = mych;
    Board[row][col] = Empty;
    mapTurn = yourTurn;
    Focus = Empty;
    ch_printmsg(1, buf);
    ch_draw();

    if (yourch == 2 || yourch == 9)
      ch_overgame(myColor);
  }

  return NOTHING;
}


static int
ch_Mv1()			/* for 暗棋 */
{
  int row, col;
  char mych, yourch;
  char buf[80];

  row = (Focus / 16) % 16;
  col = Focus % 16;
  mych = Focus / 256;
  yourch = Board[bwRow][bwCol];

  if (yourch == Empty)	/* 移進空地 */
  {
    if (abs(bwRow - row) + abs(bwCol - col) != 1)	/* 要在隔壁才能移過去 */
      return NOTHING;

    sprintf(buf, "M%d:%d", Focus, bwRow * 16 + bwCol);
    if (!do_send(buf))
      return DISCONNECT;
    sprintf(buf, "\033[1;36m▼我方移動 %s(%d, %c) 至 (%d, %c)\033[m",
      ch_icon[mych], col, row + 'A', bwCol, bwRow + 'A');
  }
  else			/* 移進有子的地 */
  {
    if (!ch_check(yourch))		/* 不同色才可以吃 */
      return NOTHING;

    if (mych == 7 || mych == 14)	/* 包/炮用跳的才能吃 */
    {
      if (ch_count(row, col) != 1)
	return NOTHING;
    }
    else
    {
      if (abs(bwRow - row) + abs(bwCol - col) != 1)	/* 一般子要在隔壁才能吃過去 */
	return NOTHING;

      if (myColor == Black)
      {
	if (mych != 15 || yourch != 2)		/* 卒可以吃帥 */
	{
	  if (mych - 7 > yourch)		/* 小不能吃大 */
	    return NOTHING;
	  if (mych == 9 && yourch == 8)	/* 將不能吃兵 */
	    return NOTHING;
	}
      }
      else
      {
	if (mych != 8 || yourch != 9)		/* 兵可以吃將 */
	{
	  if (mych + 7 > yourch)		/* 小不能吃大 */
	    return NOTHING;
	  if (mych == 2 && yourch == 15)	/* 帥不能吃兵 */
	    return NOTHING;
	}
      }
    }

    sprintf(buf, "E%d:%d", Focus, yourch * 256 + bwRow * 16 + bwCol);
    if (!do_send(buf))
      return DISCONNECT;
    MyEat[myeat_index++] = yourch;
    sprintf(buf, "\033[1;32m▼我方移動 %s(%d, %c) 吃 %s(%d, %c)\033[m",
      ch_icon[mych], col, row + 'A', ch_icon[yourch], bwCol, bwRow + 'A');
    ch_printeat();
  }

  Board[bwRow][bwCol] = mych;
  Board[row][col] = Empty;
  mapTurn = yourTurn;
  Focus = Empty;

  ch_printmsg(1, buf);
  ch_draw();

  if (myeat_index == 16)
    ch_overgame(myColor);

  return NOTHING;
}


static int
chCtrlS()
{
  if (!do_send("S"))
    return DISCONNECT;
  ch_overgame((myColor == Black) ? Red : Black);
  return NOTHING;
}


static int
chLEFT()
{
  if (bwCol > 0)
    bwCol--;
  return NOTHING;
}


static int
chRIGHT()
{
  if (bwCol < 8 - Choose)
    bwCol++;
  return NOTHING;
}


static int
chUP()
{
  if (bwRow > 0)
    bwRow--;
  return NOTHING;
}


static int
chDOWN()
{
  if (bwRow < 9 - Choose * 6)
    bwRow++;
  return NOTHING;
}


static int
chEnter()
{
  char buf[40], ch;

  ch = Board[bwRow][bwCol];
  if (ch == Cover)	/* 暗棋翻子 */
  {
    ch = ch_rand();
    Board[bwRow][bwCol] = ch;
    sprintf(buf, "D%d", ch * 256 + bwRow * 16 + bwCol);
    if (!do_send(buf))
      return DISCONNECT;

    if (!dark_choose)	/* 暗棋第一次翻子決定顏色 */
    {
      dark_choose = 1;
      myColor = (ch < 9) ? Red : Black;
      sprintf(buf, "F%d", myColor);
      if (!do_send(buf))
	return DISCONNECT;
    }

    mapTurn = yourTurn;
    Focus = Empty;
    sprintf(buf, "\033[1;32m▲我方翻開 %s(%d, %c)\033[m",
      ch_icon[ch], bwCol, bwRow + 'A');
    ch_printmsg(1, buf);
    ch_draw();
  }
  else
  {
    if (Focus)		/* 軍/暗棋移動 */
    {
      if (Focus == ch * 256 + bwRow * 16 + bwCol)	/* 再選一遍表示取消選取 */
      {
	Focus = 0;
	ch_printmsg(3, NULL);
      }
      else
	return Choose ? ch_Mv1() : ch_Mv0();
    }
    else		/* 軍/暗棋選取 */
    {
      if (ch != Empty && !ch_check(ch))
      {
	Focus = ch * 256 + bwRow * 16 + bwCol;
	ch_printmsg(2, NULL);
      }
    }
  }
  return NOTHING;
}


static KeyFunc yourTurn[] =
{
  Ctrl('C'), ftnCtrlC,
  Ctrl('D'), fCtrlD,
  KEY_LEFT, chLEFT,
  KEY_RIGHT, chRIGHT,
  KEY_UP, chUP,
  KEY_DOWN, chDOWN,
  KEY_TAB, fTAB,
  0, fNoOp
};


static KeyFunc myTurn[] =
{
  Ctrl('C'), ftnCtrlC,
  Ctrl('D'), fCtrlD,
  Ctrl('S'), chCtrlS,
  ' ', chEnter,
  '\n', chEnter,
  KEY_LEFT, chLEFT,
  KEY_RIGHT, chRIGHT,
  KEY_UP, chUP,
  KEY_DOWN, chDOWN,
  KEY_TAB, fTAB,
  0, fNoOp
};


/*-------------------------------------------------------*/
/* 所有棋盤公用主程式					 */
/*-------------------------------------------------------*/


/* rule set */
static int (**ruleSet[]) () =
{
  othRule, fivRule, blkRule, armyRule, darkRule
};


static void
do_init()
{
  char *t, *mateid, msg[160], buf[80];
  int i, myTotal, yourTotal, myWin, yourWin;

  /* Initialize state */
  (*rule[Binit]) ();
  mapTalk = NULL;	/* 一開始進入是在棋盤上 */

  bwRow = bwCol = 0;
  msgline = 1;

  cmdCol = 0;
  *cmdBuf = 0;
  cmdPos = -1;

  /* Initialize screen */
  clear();

  t = cuser.userid;
  mateid = cutmp->mateid;

  /* 取得戰績 */
  play_count(t, &myTotal, &myWin);
  play_count(mateid, &yourTotal, &yourWin);

  sprintf(buf, "☆%s(%d戰%d勝) vs ★%s(%d戰%d勝) \033[m", 
    t, myTotal, myWin, mateid, yourTotal, yourWin);

  sprintf(msg, "\033[1;33;44m【 對奕%s 】", ruleStr);
  i = 80 - strlen(buf) + 3 - strlen(msg) + 10;
  t = str_tail(msg);
  for (; i; i--)
    *t++ = ' ';
  strcpy(t, buf);
  outs(msg);

  (Choose < 0) ? bw_init() : ch_init();
}


static int
main_board(sock, later)
  int sock, later;
{
  screenline sl[T_LINES];
  char c;
  int ch;
  KeyFunc *k;

  vs_save(sl);
  cfd = sock;

  if (!later)
  {
    /* ask for which rule set */
    /* assume: peer won't send char until setup */
    c = vans("想下哪種棋 (1)黑白棋 (2)五子棋 (3)圍棋 (4)軍棋 (5)暗棋 (Q)取消？[Q] ");
    if (c >= '1' && c <= '5')
    {
      c -= '1';
    }
    else
    {
      c = -1;
      vs_restore(sl);	/* lkchu.990428: 把 foot restore 回來 */
    }

    /* transmit rule set */
    if (send(cfd, &c, 1, 0) != 1)
      return DISCONNECT;

    /* 啟動遊戲者為黑子 */
    myColor = Black;
  }
  else
  {
    /* prompt for waiting rule set */
    outz("★ 對方要求進入對奕模式選擇中，請稍候 \033[5m...\033[m");
    refresh();
    /* receive rule set */
    if (recv(cfd, &c, 1, 0) != 1)
      return DISCONNECT;

    vs_restore(sl);		/* lkchu.990428: 把 foot restore 回來 */

    /* 被啟動遊戲者為紅(白)子 */
    myColor = White;		/* White == Red */
  }

  if (c < 0)
    return LEAVE;
  rule = ruleSet[c];
  ruleStr = ruleStrSet[c];

  Choose = c - 3;		/* -3:黑白棋 -2:五子棋 -1:圍棋 0:軍棋 1:暗棋 */

  /* initialize all */
  do_init();

  for (;;)
  {
    if (mapTalk)
    {
      move(b_lines - 2, cmdCol + 35);
      k = mapTalk;
    }
    else
    {
      if (Choose < 0)
	move(bwRow + 1, bwCol * 2 + 1);
      else if (Choose == 0)
	move(bwRow * 2 + 1, bwCol * 4 + 1);
      else
	move(bwRow * 2 + 2, bwCol * 4 + 3);

      k = mapTurn;
    }

    ch = vkey();
    if (ch == I_OTHERDATA)
    {				/* incoming */
      ch = (Choose < 0) ? bw_recv() : ch_recv();
      if (ch >= NOTHING)	/* -1 for exit bwboard, -2 for exit talk */
	continue;
      vs_restore(sl);
      return ch;
    }

#ifdef EVERY_Z
    /* Thor: Chat 中按 ctrl-z */
    else if (ch == Ctrl('Z'))
    {
      char buf[IDLEN + 1];
      screenline slt[T_LINES];

      /* Thor.980731: 暫存 mateid, 因為出去時可能會用掉 mateid */
      strcpy(buf, cutmp->mateid);

      vio_save();	/* Thor.980727: 暫存 vio_fd */
      vs_save(slt);
      every_Z(0);
      vs_restore(slt);
      vio_restore();	/* Thor.980727: 還原 vio_fd */

      /* Thor.980731: 還原 mateid, 因為出去時可能會用掉 mateid */
      strcpy(cutmp->mateid, buf);
      continue;
    }
#endif

    for (;; k++)
    {
      if (!k->key || ch == k->key)
	break;
    }

    /* -1 for exit bwboard, -2 for exit talk */
    if ((ch = k->key ? (*k->func) () : (*k->func) (ch)) >= NOTHING)
      continue;
    vs_restore(sl);
    return ch;
  }
}

#include <stdarg.h>

int
vaBWboard(pvar)
  va_list pvar;
{
  int sock, later;
  sock = va_arg(pvar, int);
  later = va_arg(pvar, int);
  return main_board(sock, later);
}
#endif		/* HAVE_GAME */
