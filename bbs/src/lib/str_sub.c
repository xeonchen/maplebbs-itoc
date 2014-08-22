#include "dao.h"


char * 
str_sub(str, tag) 
  char *str; 
  char *tag;		/* non-empty lowest case pattern */ 
{ 
  int cc, c1, c2;
  char *p1, *p2;
  int in_chi = 0;	/* 1: 前一碼是中文字 */
  int in_chii;		/* 1: 前一碼是中文字 */

  cc = *tag++;
 
  while (c1 = *str)
  {
    if (in_chi)
    {
      in_chi ^= 1;
    }
    else
    {
      if (c1 & 0x80)
	in_chi ^= 1;
      else if (c1 >= 'A' && c1 <= 'Z')
	c1 |= 0x20;

      if (c1 == cc)
      {
	p1 = str;
	p2 = tag;
	in_chii = in_chi;

	do
	{
	  c2 = *p2;
 	  if (!c2)
	    return str;
 
	  p2++;
	  c1 = *++p1;
	  if (in_chii || c1 & 0x80)
	    in_chii ^= 1;
	  else if (c1 >= 'A' && c1 <= 'Z')
	    c1 |= 0x20;
	} while (c1 == c2);
      }
    }
 
    str++;
  }

  return NULL;
}
