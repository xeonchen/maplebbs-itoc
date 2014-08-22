#include "dao.h"


char * 
str_str(str, tag) 
  char *str; 
  char *tag;                  /* non-empty lower case pattern */ 
{ 
  int cc, c1, c2;
  char *p1, *p2;

  cc = *tag++;
 
  while (c1 = *str)
  {
    if (c1 >= 'A' && c1 <= 'Z')
      c1 |= 0x20;

    if (c1 == cc)
    {
      p1 = str;
      p2 = tag;

      do
      {
        c2 = *p2;
        if (!c2)
          return str;
 
        p2++;
        c1 = *++p1;
        if (c1 >= 'A' && c1 <= 'Z')
          c1 |= 0x20;
      } while (c1 == c2);
    }
 
    str++;
  }

  return NULL;
}
