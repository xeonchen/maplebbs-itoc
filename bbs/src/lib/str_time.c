#include <stdio.h>
#include <time.h>


static char datemsg[40];


char *
Atime(clock)	/* Thor.990125: 假裝 ARPANET 時間格式 */
  time_t *clock;
{
  /* ARPANET format: Thu, 11 Feb 1999 06:00:37 +0800 (CST) */
  /* strftime(datemsg, 40, "%a, %d %b %Y %T %Z", localtime(clock)); */
  /* Thor.990125: time zone的傳回值不知和ARPANET格式是否一樣,先硬給,同sendmail*/
  strftime(datemsg, 40, "%a, %d %b %Y %T +0800 (CST)", localtime(clock));
  return (datemsg);
}


char *
Btime(clock)	/* BBS 時間格式 */
  time_t *clock;
{
  struct tm *t = localtime(clock);

  sprintf(datemsg, "%d/%02d/%02d %.3s %02d:%02d:%02d",
    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
    "SunMonTueWedThuFriSat" + (t->tm_wday * 3),
    t->tm_hour, t->tm_min, t->tm_sec);
  return (datemsg);
}


char *
Now()
{
  time_t now;

  time(&now);
  return Btime(&now);
}
