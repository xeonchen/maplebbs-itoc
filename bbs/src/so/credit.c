/*-------------------------------------------------------*/
/* credit.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 記帳本，記錄生活中的收入支出		 */
/* create : 99/12/18                                     */
/* update : 02/01/26					 */
/* author : wildcat@wd.twbbs.org			 */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_CREDIT

/* ----------------------------------------------------- */
/* credit.c 中運用的資料結構                             */
/* ----------------------------------------------------- */

typedef struct
{
  int year;			/* 年 */
  char month;			/* 月 */
  char day;			/* 日 */

  char flag;			/* 支出/收入 */
  int money;			/* 金額 */
  char useway;			/* 類別(食衣住行育樂) */
  char desc[112];		/* 說明 */		/* 這太長了，保留給其他欄位使用 */
}      CREDIT;


#define CREDIT_OUT	0x1	/* 支出 */
#define CREDIT_IN	0x2	/* 收入 */

#define CREDIT_OTHER	0	/* 其他 */
#define CREDIT_EAT	1	/* 食 */
#define CREDIT_WEAR	2	/* 衣 */
#define CREDIT_LIVE	3	/* 住 */
#define CREDIT_MOVE	4	/* 行 */
#define CREDIT_EDU	5	/* 育 */
#define CREDIT_PLAY	6	/* 樂 */

static char fpath[64];		/* FN_CREDIT 檔案路徑 */


static void
credit_head()
{
  vs_head("記帳手札", str_site);
  prints(NECKER_CREDIT, d_cols, "");
}


static void
credit_body(page)
  int page;
{
  CREDIT credit;
  char *way[] = {"其他", "[食]", "[衣]", "[住]", "[行]", "[育]", "[樂]"};
  int fd;

  move(1, 65);
  prints("第 %2d 頁", page + 1);

  move(3, 0);
  clrtobot();

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    int pos, n;

    pos = page * XO_TALL;	/* 一頁有 XO_TALL 筆 */
    n = XO_TALL;

    while (n)
    {
      lseek(fd, (off_t) (sizeof(CREDIT) * pos), SEEK_SET);
      if (read(fd, &credit, sizeof(CREDIT)) == sizeof(CREDIT))
      {
	n--;
	pos++;
	prints("%6d %04d/%02d/%02d %s %8d %4s %.*s\n", 
	  pos, credit.year, credit.month, credit.day, 
	  credit.flag == CREDIT_OUT ? "\033[1;32m支出\033[m" : "\033[1;31m收入\033[m",
	  credit.money, 
	  credit.flag == CREDIT_OUT ? way[credit.useway] : "    ",
	  d_cols + 46, credit.desc);
      }
      else
      {
        break;
      }
    }

    close(fd);
  }
}


static int
credit_add()
{
  CREDIT credit;
  char buf[80];

  move(3, 0);
  clrtobot();

  memset(&credit, 0, sizeof(CREDIT));

  if (vget(5, 0, "收支 (1)收入 (2)支出 [2] ", buf, 3, DOECHO) == '1')
    credit.flag = CREDIT_IN;
  else
    credit.flag = CREDIT_OUT;
    
  vget(6, 0, "時間 (年份) ", buf, 5, DOECHO);
  credit.year = atoi(buf);

  vget(7, 0, "時間 (月份) ", buf, 3, DOECHO);
  credit.month = atoi(buf);

  vget(8, 0, "時間 (日期) ", buf, 3, DOECHO);
  credit.day = atoi(buf);

  vget(9, 0, "金錢 (元) ", buf, 9, DOECHO);
  credit.money = atoi(buf);

  if (credit.flag == CREDIT_OUT)	/* 支出才有記錄用途 */
  {
    int useway;

    useway = vget(10, 0, "用途 0)其他 1)食 2)衣 3)住 4)行 5)育 6)樂 [0] ", buf, 3, DOECHO) - '0';
    if (useway > 6 || useway < 0)
      useway = 0;
    credit.useway = useway;
  }

  vget(11, 0, "說明：", credit.desc, 51, DOECHO);

  rec_add(fpath, &credit, sizeof(CREDIT));
  return 1;
}


static int
credit_delete()
{
  int pos;
  char buf[4];

  vget(b_lines, 0, "要刪除第幾筆資料：", buf, 4, DOECHO);
  pos = atoi(buf);
  
  if (rec_num(fpath, sizeof(CREDIT)) < pos)
  {
    vmsg("您搞錯囉，沒有這筆資料");
    return 0;
  }

  rec_del(fpath, sizeof(CREDIT), pos - 1, NULL);
  return 1;
}


static int
credit_count()
{
  CREDIT *credit;
  struct stat st;
  int fd;
  int way[7], moneyin, moneyout;

  if ((fd = open(fpath, O_RDONLY)) >= 0 && !fstat(fd, &st) && st.st_size > 0)
  {
    memset(way, 0, sizeof(way));
    moneyin = 0;

    mgets(-1);
    while (credit = mread(fd, sizeof(CREDIT)))
    {
      if (credit->flag == CREDIT_OUT)	/* 支出才有記錄用途 */
	way[credit->useway] += credit->money;
      else
        moneyin += credit->money;
    }
    close(fd);

    moneyout = 0;
    for (fd = 0; fd <= 6; fd++)
      moneyout += way[fd];

    move(3, 0);
    clrtobot();

    move(7, 0);
    prints("      \033[1;31m總收入  %12d 元\033[m\n", moneyin);
    prints("      \033[1;32m總支出  %12d 元\033[m\n\n", moneyout);

    prints("花在  \033[1;36m [食]   %12d 元    \033[32m [衣]   %12d 元\033[m\n", way[CREDIT_EAT], way[CREDIT_WEAR]);
    prints("      \033[1;31m [住]   %12d 元    \033[33m [行]   %12d 元\033[m\n", way[CREDIT_LIVE], way[CREDIT_MOVE]);
    prints("      \033[1;35m [育]   %12d 元    \033[37m [樂]   %12d 元\033[m\n", way[CREDIT_EDU], way[CREDIT_PLAY]);
    prints("      \033[1;34m 其他   %12d 元\033[m", way[CREDIT_OTHER]);

    vmsg(NULL);
    return 1;
  }

  vmsg("您沒有記帳記錄");
  return 0;
}


int
main_credit()
{
  int page, redraw;
  char buf[3];

  credit_head();

  usr_fpath(fpath, cuser.userid, FN_CREDIT);
  page = 0;
  redraw = 1;

  for (;;)
  {
    if (redraw)
      credit_body(page);

    switch (vans("記帳手札 C)換頁 1)新增 2)刪除 3)全刪 4)總計 Q)離開 [Q] "))
    {
    case 'c':
      vget(b_lines, 0, "跳到第幾頁：", buf, 3, DOECHO);
      redraw = atoi(buf) - 1;
      
      if (page != redraw && redraw >= 0 && 
        redraw <= (rec_num(fpath, sizeof(CREDIT)) - 1) / XO_TALL)
      {
        page = redraw;
        redraw = 1;
      }
      else
      {
        redraw = 0;
      }
      break;

    case '1':
      redraw = credit_add();
      break;

    case '2':
      redraw = credit_delete();
      break;

    case '3':
      if (vans(MSG_SURE_NY) == 'y')
      {
        unlink(fpath);
        return 0;
      }
      break;

    case '4':
      redraw = credit_count();
      break;

    default:
      return 0;
    }
  }
}
#endif				/* HAVE_CREDIT */
