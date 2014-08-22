/*-------------------------------------------------------*/
/* classtable.c	( YZU WindTopBBS Ver 3.02 )		 */
/*-------------------------------------------------------*/
/* target : 功課表					 */
/* create :   /  /                                       */
/* update : 02/07/12                                     */
/* author :						 */
/* modify : itoc.bbs@bbs.ee.nctu.edu.tw			 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_CLASSTABLE

/* ----------------------------------------------------- */
/* classtable.c 中運用的資料結構			 */
/* ----------------------------------------------------- */


#define MAX_WEEKDAY	6	/* 一星期有 6 天 */
#define	MAX_DAYCLASS	16	/* 一天有 16 節 */


typedef struct
{
  char name[9];		/* 課名 */
  char teacher[9];	/* 教師 */
  char class[5];	/* 教室 */
  char objid[7];	/* 課號 */
}   CLASS;


typedef struct
{
  CLASS table[MAX_WEEKDAY][MAX_DAYCLASS];	/* 一星期 MAX_WEEKDAY * MAX_DAYCLASS 堂課 */
}   CLASS_TABLE;


typedef struct
{
  char c_class[5];	/* 第幾節 */
  char c_start[6];	/* 上課時間 */
  char c_break[6];	/* 下課時間 */
}  CLOCK;


static CLOCK class_time[MAX_DAYCLASS] = 	/* 課堂時間 */
{
  {" 一 ", "06:00", "06:50"}, 
  {" 二 ", "07:00", "07:50"}, 
  {" 三 ", "08:00", "08:50"}, 
  {" 四 ", "09:00", "09:50"}, 
  {" 五 ", "10:10", "11:00"}, 
  {" 六 ", "11:10", "12:00"}, 
  {" 七 ", "12:30", "13:20"}, 
  {" 八 ", "13:30", "14:20"}, 
  {" 九 ", "14:30", "15:20"}, 
  {" 十 ", "15:40", "16:30"}, 
  {"十一", "16:40", "17:30"}, 
  {"十二", "17:40", "18:30"}, 
  {"十三", "18:30", "19:20"}, 
  {"十四", "19:30", "20:20"}, 
  {"十五", "20:30", "21:20"}, 
  {"十六", "21:30", "22:20"}
};


/* ----------------------------------------------------- */
/* CLASS 處理函數					 */
/* ----------------------------------------------------- */


static void
class_show(x, y, class)
  int x, y;
  CLASS *class;
{
  move(x, y);
  prints("課名：%s", class->name);
  move(x + 1, y);
  prints("教師：%s", class->teacher);
  move(x + 2, y);
  prints("教室：%s", class->class);
  move(x + 3, y);
  prints("課號：%s", class->objid);
}


static void
class_edit(class)
  CLASS *class;
{
  int echo;

  echo = *(class->name) ? GCARRY : DOECHO;
  vget(4, 0, "課名：", class->name, sizeof(class->name), echo);
  vget(5, 0, "教師：", class->teacher, sizeof(class->teacher), echo);
  vget(6, 0, "教室：", class->class, sizeof(class->class), echo);
  vget(7, 0, "課號：", class->objid, sizeof(class->objid), echo);
}


static int			/* 1:正確 0:錯誤 */
class_number(day, class)	/* 傳回星期幾第幾節 */
  int *day;
  int *class;
{
  char ans[5];

  move(2, 0);
  outs("503 表示星期五第三節");
  *day = vget(3, 0, "上課時間：", ans, 4, DOECHO) - '1';	/* 503 表示星期五第三節 */
  *class = atoi(ans + 1) - 1;
  if (*day > MAX_WEEKDAY - 1 || *day < 0 || *class > MAX_DAYCLASS - 1 || *class < 0)
    return 0;

  return 1;
}


/* ----------------------------------------------------- */
/* CLASS_TABLE 處理函數					 */
/* ----------------------------------------------------- */


static void
table_file(fpath, table)	/* 把 table 寫入 FN_CLASSTBL_LOG */
  char *fpath;
  CLASS_TABLE *table;
{
  int i, j;
  FILE *fp;

  fp = fopen(fpath, "w");

  fprintf(fp, "           星期一    星期二    星期三    星期四    星期五    星期六\n");
  for (i = 0; i < MAX_DAYCLASS; i++)
  {
    fprintf(fp, "第%s節  ", class_time[i].c_class);
    for (j = 0; j < MAX_WEEKDAY; j++)
      fprintf(fp, "%-8.8s  ", table->table[j][i].name);

    fprintf(fp, "\n  %s   ", class_time[i].c_start);
    for (j = 0; j < MAX_WEEKDAY; j++)
      fprintf(fp, "%-8.8s  ", table->table[j][i].teacher);

    fprintf(fp, "\n   ↓     ");
    for (j = 0; j < MAX_WEEKDAY; j++)
      fprintf(fp, "%-8.8s  ", table->table[j][i].class);

    fprintf(fp, "\n  %s   ", class_time[i].c_break);
    for (j = 0; j < MAX_WEEKDAY; j++)
      fprintf(fp, "%-8.8s  ", table->table[j][i].objid);

    fprintf(fp, "\n\n");
  }
  fclose(fp);
}


static void
table_show(table)
  CLASS_TABLE *table;
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, FN_CLASSTBL_LOG);
  table_file(fpath, table);
  more(fpath, NULL);
}


static void
table_mail(table)
  CLASS_TABLE *table;
{
  char fpath[64];

  usr_fpath(fpath, cuser.userid, FN_CLASSTBL_LOG);
  table_file(fpath, table);
  mail_self(fpath, cuser.userid, "個人功\課表", MAIL_READ);
}


static void
table_edit(table)
  CLASS_TABLE *table;
{
  int i, j;

  vs_bar("編輯個人功\課表");

  if (class_number(&i, &j))
  {
    class_edit(&(table->table[i][j]));
    class_show(10, 0, &(table->table[i][j]));
  }
}


static void
table_del(table)
  CLASS_TABLE *table;
{
  int i, j;

  vs_bar("刪除個人功\課表");

  if (!class_number(&i, &j))
    return;

  class_show(10, 0, &(table->table[i][j]));

  if (vans(msg_sure_ny) == 'y')
    memset(&(table->table[i][j]), 0, sizeof(CLASS));
}


static void
table_copy(table)
  CLASS_TABLE *table;
{
  int i, j, x, y;

  vs_bar("個人功\課表");

  move(9, 0);
  outs("來源：");
  if (!class_number(&i, &j))
    return;

  class_show(10, 0, &(table->table[i][j]));

  move(9, 39);
  outs("目的：");
  if (!class_number(&x, &y))
    return;

  class_show(10, 39, &(table->table[x][y]));

  if (vans(msg_sure_ny) == 'y')
    memcpy(&(table->table[x][y]), &(table->table[i][j]), sizeof(CLASS));
}


int
main_classtable()
{
  char fpath[64];
  CLASS_TABLE newtable, oldtable, *ptr;

  usr_fpath(fpath, cuser.userid, FN_CLASSTBL);
  ptr = &newtable;

  if (rec_get(fpath, ptr, sizeof(CLASS_TABLE), 0))
    memset(ptr, 0, sizeof(CLASS_TABLE));
  memcpy(&oldtable, ptr, sizeof(CLASS_TABLE));

  for (;;)
  {
    switch (vans("課表系統 (E/C/D)編輯/複製/刪除 P)印出 K)全砍 S)存檔 M)信箱 Q)離開 [Q] "))
    {
    case 'e':
      table_edit(ptr);
      break;
    case 'd':
      table_del(ptr);
      break;
    case 'c':
      table_copy(ptr);
      break;

    case 'p':
      table_show(ptr);
      break;
    case 'm':
      table_mail(ptr);
      break;

    case 's':
      rec_put(fpath, ptr, sizeof(CLASS_TABLE), 0, NULL);
      memcpy(&oldtable, ptr, sizeof(CLASS_TABLE));
      vmsg("儲存完成");
      break;
    case 'k':
      if (vans(msg_sure_ny) == 'y')
      {
	unlink(fpath);
	memset(ptr, 0, sizeof(CLASS_TABLE));
	memset(&oldtable, 0, sizeof(CLASS_TABLE));
      }
      break;

    default:
      goto end_loop;
    }
  }

end_loop:

  /* 檢查新舊是否一樣，若不一樣要問是否存檔 */
  if (memcmp(&oldtable, ptr, sizeof(CLASS_TABLE)))
  {
    if (vans("是否儲存(Y/N)？[Y] ") != 'n')
      rec_put(fpath, ptr, sizeof(CLASS_TABLE), 0, NULL);
  }

  return 0;  
}
#endif	/* HAVE_CLASSTABLE */
