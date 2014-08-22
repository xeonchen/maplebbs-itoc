/* ----------------------------------------------------  */
/* E-mail address format				 */
/* ----------------------------------------------------  */
/* 1. user@domain					 */
/* 2. <user@domain>					 */
/* 3. user@domain (nick)				 */
/* 4. user@domain ("nick")				 */
/* 5. nick <user@domain>				 */
/* 6. "nick" <user@domain>				 */
/* ----------------------------------------------------  */


#include "dao.h"
#include <string.h>


int
str_from(from, addr, nick)
  char *from, *addr, *nick;
{
  char *str, *ptr, *langle;
  int cc;

  *nick = 0;

  langle = ptr = NULL;

  for (str = from; cc = *str; str++)
  {
    if (cc == '<')
      langle = str;
    else if (cc == '@')
      ptr = str;
  }

  if (ptr == NULL)
  {
    strcpy(addr, from);
    return -1;
  }

  if (langle && langle < ptr && str[-1] == '>')
  {
    /* case 2/5/6 : name <mail_addr> */

    str[-1] = 0;
    if (langle > from)
    {
      ptr = langle - 2;
      if (*from == '"')
      {
	from++;
	if (*ptr == '"')
	  ptr--;
      }
      if (*from == '(')
      {
	from++;
	if (*ptr == ')')
	  ptr--;
      }
      ptr[1] = '\0';
      strcpy(nick, from);
      str_decode(nick);
    }

    from = langle + 1;
  }
  else
  {
    /* case 1/3/4 */

    if (*--str == ')')
    {
      if (str[-1] == '"')
	str--;
      *str = 0;

      if (ptr = (char *) strchr(from, '('))
      {
	ptr[-1] = 0;
	if (*++ptr == '"')
	  ptr++;

	strcpy(nick, ptr);
	str_decode(nick);
      }
    }
  }

  strcpy(addr, from);
  return 0;
}
