#include "dao.h"


time_t
chrono32(str)
  char *str;		/* M0123456 */
{
  time_t chrono;
  int ch;

  chrono = 0;
  while (ch = *++str)
  {
    ch -= (ch > '9') ? 'A' - 10 : '0';
    chrono = (chrono << 5) + ch;
  }
  return chrono;
}
