/*-------------------------------------------------------*/
/* calendar.c	( NTHU CS MapleBBS Ver 3.10 )		 */
/*-------------------------------------------------------*/
/* target : 萬年曆					 */
/* create : 02/08/31					 */
/* update :   /  /  					 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/*-------------------------------------------------------*/


#include "bbs.h"

#ifdef HAVE_CALENDAR

/* $NetBSD: cal.c,v 1.10 1998/07/28 19:26:09 mycroft Exp $	 */

/*
 * Copyright (c) 1989, 1993, 1994 The Regents of the University of
 * California.  All rights reserved.
 * 
 * This code is derived from software contributed to Berkeley by Kim Letkeman.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * University of California, Berkeley and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#define TM_YEAR_BASE		1900

#define	THURSDAY		4	/* for reformation */
#define	SATURDAY 		6	/* 1 Jan 1 was a Saturday */

#define	FIRST_MISSING_DAY 	639799	/* 3 Sep 1752 */
#define	NUMBER_MISSING_DAYS 	11	/* 11 day correction */

#define	MAXDAYS			42	/* max slots in a month array */
#define	SPACE			-1	/* used in day array */


static int days_in_month[2][13] = 
{
  {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

static int sep1752[MAXDAYS] = 
{
  SPACE, SPACE, 1, 2, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30,
  SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
  SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
  SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
};

static int empty[MAXDAYS] = 
{
  SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
  SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
  SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
  SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
  SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
  SPACE, SPACE, SPACE, SPACE, SPACE, SPACE, SPACE,
};

static char *month_names[12] = 
{
  "一月", "二月", "三月", "四月", "五月", "六月",
  "七月", "八月", "九月", "十月", "十一月", "十二月"
};

static char *day_headings = "日 一 二 三 四 五 六";

/* leap year -- account for gregorian reformation in 1752 */
#define	leap_year(yr)			((yr) <= 1752 ? !((yr) % 4) : (!((yr) % 4) && ((yr) % 100)) || !((yr) % 400))

/* number of centuries since 1700, not inclusive */
#define	centuries_since_1700(yr)	((yr) > 1700 ? (yr) / 100 - 17 : 0)

/* number of centuries since 1700 whose modulo of 400 is 0 */
#define	quad_centuries_since_1700(yr)	((yr) > 1600 ? ((yr) - 1600) / 400 : 0)

/* number of leap years between year 1 and this year, not inclusive */
#define	leap_years_since_year_1(yr)	((yr) / 4 - centuries_since_1700(yr) + quad_centuries_since_1700(yr))

#define	DAY_LEN		3	/* 3 spaces per day */
#define	WEEK_LEN	20	/* 7 * 3 - one space at the end */
#define	HEAD_SEP	2	/* spaces between day headings */


/* 
 * day_in_year -- return the 1 based day number within the year 
 */
static int
day_in_year(day, month, year)
  int day, month, year;
{
  int i, leap;

  leap = leap_year(year);
  for (i = 1; i < month; i++)
    day += days_in_month[leap][i];
  return (day);
}


/*
 * day_in_week return the 0 based day number for any date from 1 Jan. 1 to 31
 * Dec. 9999.  Assumes the Gregorian reformation eliminates 3 Sep. 1752
 * through 13 Sep. 1752.  Returns Thursday for all missing days.
 */
static int
day_in_week(day, month, year)
  int day, month, year;
{
  int temp;

  temp = (year - 1) * 365 + leap_years_since_year_1(year - 1) + day_in_year(day, month, year);
  if (temp < FIRST_MISSING_DAY)
    return ((temp - 1 + SATURDAY) % 7);
  if (temp >= (FIRST_MISSING_DAY + NUMBER_MISSING_DAYS))
    return (((temp - 1 + SATURDAY) - NUMBER_MISSING_DAYS) % 7);
  return (THURSDAY);
}


/*
 * day_array -- Fill in an array of 42 integers with a calendar.  Assume for
 * a moment that you took the (maximum) 6 rows in a calendar and stretched
 * them out end to end.  You would have 42 numbers or spaces.  This routine
 * builds that array for any month from Jan. 1 through Dec. 9999.
 */
static void
day_array(month, year, days)
  int month, year;
  int *days;
{
  int day, dw, dm;

  if (month == 9 && year == 1752)
  {
    memmove(days, sep1752, MAXDAYS * sizeof(int));
    return;
  }
  memmove(days, empty, MAXDAYS * sizeof(int));
  dm = days_in_month[leap_year(year)][month];
  dw = day_in_week(1, month, year);
  day = 1;
  while (dm--)
    days[dw++] = day++;
}


static char *
ascii_day(p, day)
  char *p;
  int day;
{
  static char *aday[] = 
  {
    "",
    " 1", " 2", " 3", " 4", " 5", " 6", " 7",
    " 8", " 9", "10", "11", "12", "13", "14",
    "15", "16", "17", "18", "19", "20", "21",
    "22", "23", "24", "25", "26", "27", "28",
    "29", "30", "31",
  };

  if (day == SPACE)
  {
    memset(p, ' ', DAY_LEN);
    p += DAY_LEN;
  }
  else
  {
    *p++ = aday[day][0];
    *p++ = aday[day][1];
    *p++ = ' ';
  }

  return p;
}


static void
monthly(year, month)
  int year, month;
{
  int col, row, len, days[MAXDAYS];
  char *p, buf[80];

  day_array(month, year, days);
  len = snprintf(buf, sizeof(buf), "%s %d", month_names[month - 1], year);

  vs_bar("萬年月曆");
  move(2, 5);
  outs("若未輸入月份可查詢年曆");
  move(4, 6);
  prints("\033[1;35m%*s%s", (WEEK_LEN - len) / 2, "", buf);
  move(6, 6);
  prints("\033[1;36m%s\033[m", day_headings);

  for (row = 0; row < 6; row++)
  {
    for (col = 0, p = buf; col < 7; col++)
    {
      if (col == 0)		/* 星期日 */
      {
	move(7 + row, 6);
	strcpy(p, "\033[1;31m");
	p += 7;
      }
      else if (col == 1)	/* 星期一～五 */
      {
	strcpy(p, "\033[37m");
	p += 5;
      }
      else if (col == 6)	/* 星期六 */
      {
	strcpy(p, "\033[32m");
	p += 5;
      }
      p = ascii_day(p, days[row * 7 + col]);
    }
    strcpy(p, "\033[m\n");
    outs(buf);
  }

  vmsg(NULL);
}


static void
center(fp, str, len, separate)
  FILE *fp;
  char *str;
  int len;
  int separate;
{
  len -= strlen(str);
  fprintf(fp, "%*s%s%*s", len / 2, "", str, len / 2 + len % 2, "");
  if (separate)
    fprintf(fp, "%*s", separate, "");
}


static void
yearly(fpath, year)
  char *fpath;
  int year;
{
  int col, *dp, i, month, row, which_cal;
  int days[12][MAXDAYS];
  char *p, buf[80];
  FILE *fp;

  /* 年曆會超過一頁，用 more() 的 */

  if (fp = fopen(fpath, "w"))
  {
    sprintf(buf, "%d", year);
    center(fp, buf, WEEK_LEN * 3 + HEAD_SEP * 2, 0);
    fprintf(fp, "\n");
    for (i = 0; i < 12; i++)
      day_array(i + 1, year, days[i]);
    memset(buf, ' ', sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    for (month = 0; month < 12; month += 3)
    {
      center(fp, month_names[month], WEEK_LEN, HEAD_SEP);
      center(fp, month_names[month + 1], WEEK_LEN, HEAD_SEP);
      center(fp, month_names[month + 2], WEEK_LEN, 0);
      fprintf(fp, "\n%s%*s%s%*s%s\n", 
	day_headings, HEAD_SEP, "", day_headings, HEAD_SEP, "", day_headings);
      for (row = 0; row < 6; row++)
      {
        for (which_cal = 0; which_cal < 3; which_cal++)
        {
	  p = buf + which_cal * (WEEK_LEN + 2);
	  dp = &days[month + which_cal][row * 7];
	  for (col = 0; col < 7; col++)
	    p = ascii_day(p, *dp++);
        }
        *p = '\0';
        fprintf(fp, "%s\n", buf);
      }
    }
    fprintf(fp, "\n");
    fclose(fp);

    more(fpath, NULL);
    unlink(fpath);
  }
}


int
main_calendar()
{
  int year, month;
  time_t now;
  struct tm *ptime;
  char fpath[64], ans[5];

  time(&now);
  ptime = localtime(&now);
  year = ptime->tm_year + 1900;
  month = ptime->tm_mon + 1;
  sprintf(fpath, "tmp/%s.calendar", cuser.userid);

  for (;;)
  {
    if (month)		/* 月曆 */
      monthly(year, month);
    else		/* 年曆 */
      yearly(fpath, year);

    if (!vget(b_lines, 0, "請輸入要查詢的年份：", ans, 5, DOECHO))
      return 0;
    year = atoi(ans);
    if (year < 1 || year > 9999)
      return 0;

    if (!vget(b_lines, 0, "請輸入要查詢的月份：", ans, 3, DOECHO))
    {
      month = 0;
    }
    else
    {
      month = atoi(ans);
      if (month < 1 || month > 12)
	return 0;
    }
  }
}
#endif	/* HAVE_CALENDAR */
