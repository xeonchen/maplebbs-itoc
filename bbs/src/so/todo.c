/*-------------------------------------------------------*/
/* todo.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 行事曆					 */
/* create : 00/09/13					 */
/* update : 03/08/24					 */
/* author : DavidYu.bbs@ptt2.twbbs.org			 */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_CALENDAR

#define HEADER_COLOR		"\033[1;44m"
#define CALENDAR_COLOR	 	"\033[30;47m"
#define CALENDAR_TODAY	 	"\033[30;42m"


/*-------------------------------------------------------*/
/* 日期處理函式						 */
/*-------------------------------------------------------*/


static int 		/* 這個月有幾天 */
month_day(y, m)
  int y, m;
{
  static int day[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  return ((m == 2) && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) ? 29 : day[m - 1];
}


static time_t
make_time(year, month, day)
  int year, month, day;
{
  struct tm ptime;

  ptime.tm_sec = 0;
  ptime.tm_min = 0;
  ptime.tm_hour = 0;
  ptime.tm_mday = day;
  ptime.tm_mon = month - 1;
  ptime.tm_year = year - 1900;
  ptime.tm_isdst = 0;
#ifndef CYGWIN
  ptime.tm_zone = "GMT";
  ptime.tm_gmtoff = 0;
#endif

  return mktime(&ptime);
}


static time_t
parse_date(date, year, month, day)
  char *date;				/* 傳入 date 格式為 2003/08/25 */
  int *year, *month, *day;		/* 傳出 time_t year month day */
{
  char *yy, *mm, *dd;

  yy = strtok(date, "/");
  mm = strtok(NULL, "/");
  dd = strtok(NULL, "");
  if (!yy || !mm || !dd)
    return 0;

  *year = atoi(yy);
  *month = atoi(mm);
  *day = atoi(dd);
  if (*year < 1 || *month < 1 || *month > 12 || *day < 1 || *day > 31)
    return 0;

  return make_time(*year, *month, *day);
}


/*-------------------------------------------------------*/
/* 事件處理函式						 */
/*-------------------------------------------------------*/


typedef struct EVENT
{
  time_t chrono;		/* 絕對時間 */
  int year, month, day;		/* 日期: 年/月/日 */
  char content[40];		/* 敘述 */
  struct EVENT *next;
}       EVENT;


static void 
event_insert(head, insert)
  EVENT *head, *insert;
{
  EVENT *p, *next;

  for (p = head; (next = p->next) && (next->chrono < insert->chrono); p = next)
    ;
  insert->next = p->next;
  p->next = insert;
}


static void 
event_free(head)
  EVENT *head;
{
  EVENT *n;

  while (head)
  {
    n = head->next;
    free(head);
    head = n;
  }
}


static EVENT *
event_read(today)
  time_t today;
{
  FILE *fp;
  char buf[80];
  EVENT head;

  head.next = NULL;

  usr_fpath(buf, cuser.userid, FN_TODO);
  if (fp = fopen(buf, "r"))
  {
    while (fgets(buf, sizeof(buf), fp))
    {
      time_t chrono;
      int year, month, day;
      char *date, *content;
      EVENT *t;

      if (buf[0] == '#')
	continue;

      date = strtok(buf, " \n");
      content = strtok(NULL, "\n");
      if (!date || !content)
	continue;

      if ((chrono = parse_date(date, &year, &month, &day)) && (chrono >= today))
      {
	t = (EVENT *) malloc(sizeof(EVENT));

	t->chrono = chrono;
	t->year = year;
	t->month = month;
	t->day = day;
	for (; *content == ' '; content++)
	  ;
	str_ncpy(t->content, content, sizeof(t->content));

	event_insert(&head, t);
      }
    }
    fclose(fp);
  }

  return head.next;
}


/*-------------------------------------------------------*/
/* 月曆產生器						 */
/*-------------------------------------------------------*/


static char **
AllocCalBuffer(line, len)
  int line, len;
{
  int i;
  char **p;

  p = malloc(sizeof(char *) * line);
  p[0] = malloc(sizeof(char) * line * len);
  p[0][0] = '\0';
  for (i = 1; i < line; i++)
  {
    p[i] = p[i - 1] + len;
    p[i][0] = '\0';
  }
  return p;
}


static int 
GenerateCalendar(calendar, y, m, tm_mon, tm_mday)	/* 產生月曆 */
  char **calendar;
  int y, m;			/* 要產生幾年幾月的月曆 */
  int tm_mon, tm_mday;		/* 今天是幾月幾日 */
{
  static char week_str[7][3] = {"日", "一", "二", "三", "四", "五", "六"};
  static char month_color[12][8] = 
  {
    "\033[1;32m", "\033[1;33m", "\033[1;35m", "\033[1;36m",
    "\033[1;32m", "\033[1;33m", "\033[1;35m", "\033[1;36m",
    "\033[1;32m", "\033[1;33m", "\033[1;35m", "\033[1;36m"
  };
  static char *month_str[12] = 
  {
    "一月  ", "二月  ", "三月  ", "四月  ", "五月  ", "六月  ",
    "七月  ", "八月  ", "九月  ", "十月  ", "十一月", "十二月"
  };

  char *p;
  int monthday, wday;
  int i, line;
  time_t first_day;
  struct tm *ptime;

  line = 0;
  first_day = make_time(y, m, 1);
  ptime = localtime(&first_day);

  /* week day banner */
  p = calendar[line];
  p += sprintf(p, "    %s ", HEADER_COLOR);
  for (i = 0; i < 7; i++)
    p += sprintf(p, "%s ", week_str[i]);
  p += sprintf(p, "\033[m");

  /* indent for first line */
  p = calendar[++line];
  p += sprintf(p, "    %s ", CALENDAR_COLOR);
  for (i = 0, wday = ptime->tm_wday; i < wday; i++)
    p += sprintf(p, "   ");

  monthday = month_day(y, m);
  for (i = 1; i <= monthday; i++, wday = (wday + 1) % 7)
  {
    if (m == tm_mon && i == tm_mday)
      p += sprintf(p, "%s%2d%s", CALENDAR_TODAY, i, CALENDAR_COLOR);
    else
      p += sprintf(p, "%2d", i);

    if (wday == 6)
    {
      p += sprintf(p, " \033[m");
      p = calendar[++line];

      if (line >= 2 && line <= 4)	/* show month */
	p += sprintf(p, "%s%2.2s\033[m  %s ", month_color[m - 1], month_str[m - 1] + (line - 2) * 2, CALENDAR_COLOR);
      else if (i < monthday)
	p += sprintf(p, "    %s ", CALENDAR_COLOR);
    }
    else
    {
      *p++ = ' ';
    }
  }

  /* fill up the last line */
  if (wday)
  {
    for (wday = 7 - wday; wday; wday--)
      p += sprintf(p, "   ");
    p += sprintf(p, "\033[m");
  }

  return line + 1;
}


/*-------------------------------------------------------*/
/* 主程式						 */
/*-------------------------------------------------------*/


int 
main_todo()
{
  char **calendar, fpath[64];
  time_t now, today;
  struct tm *ptime, ntime;
  int i, y, m;
  int lines;			/* 目前有幾行事件 */
  EVENT *head, *e;

  /* initialize date */
  time(&now);
  ptime = localtime(&now);
  ntime = *ptime;
  y = ntime.tm_year + 1900;
  m = ntime.tm_mon + 1;
  today = now - (now % 86400);

  /* read event */
  head = e = event_read(today);

  /* generate calendar */
  lines = 0;

  calendar = AllocCalBuffer(21, 128);	/* 三個月最多要 21 列 */
  for (i = 0; i < 3; i++)		/* 每次秀出三個月的行事曆 */
  {
    lines += GenerateCalendar(calendar + lines, y, m, ntime.tm_mon + 1, ntime.tm_mday) + 1;
    if (m == 12)
    {
      y++;
      m = 1;
    }
    else
    {
      m++;
    }
  }

  /* output */
  vs_bar("行事曆");

  today /= 86400;

  for (i = 0; i < 21; i++)
  {
    outs(calendar[i]);
    if (i >= 2)
    {
      if (e)
      {
	prints("%*s\033[1;37m(%3d)\033[m %02d/%02d %s",
	  (i % 7) ? 5 : 31, "", e->chrono / 86400 - today, e->month, e->day, e->content);
	e = e->next;
      }
    }
    else if (i == 0)
    {
      prints("    \033[1;37m現在是 %d/%02d/%02d %2d:%02d:%02d%cm\033[m",
	ntime.tm_year + 1900, ntime.tm_mon + 1, ntime.tm_mday,
	(ntime.tm_hour == 0 || ntime.tm_hour == 12) ? 12 : ntime.tm_hour % 12, 
	ntime.tm_min, ntime.tm_sec,
	ntime.tm_hour >= 12 ? 'p' : 'a');
    }
    outc('\n');
  }
  event_free(head);

  /* edit */
  switch (vans("行事曆 (D)刪除 (E)修改 (Q)取消？[Q] "))
  {
  case 'e':
    more("etc/todo.welcome", NULL);
    usr_fpath(fpath, cuser.userid, FN_TODO);
    vedit(fpath, 0);
    break;

  case 'd':
    usr_fpath(fpath, cuser.userid, FN_TODO);
    unlink(fpath);
    break;
  }

  return 0;
}
#endif	/* HAVE_CALENDAR */
