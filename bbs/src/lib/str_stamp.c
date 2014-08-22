/* ------------------------------------------ */
/* mail / post 時，依據時間建立檔案，加上郵戳 */
/* ------------------------------------------ */
/* Input: fpath = directory;		      */
/* Output: fpath = full path;		      */
/* ------------------------------------------ */


#include <stdio.h>
#include <time.h>


void
str_stamp(str, chrono)
  char *str;
  time_t *chrono;
{
  struct tm *ptime;

  ptime = localtime(chrono);
  /* Thor.990329: y2k */
  sprintf(str, "%02d/%02d/%02d",
    ptime->tm_year % 100, ptime->tm_mon + 1, ptime->tm_mday);
}
