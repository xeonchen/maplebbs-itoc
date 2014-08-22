#include <string.h>


void
setdirpath(fpath, direct, fname)
  char *fpath, *direct, *fname;
{
  int ch;
  char *target;

  while (ch = *direct)
  {
    *fpath++ = ch;
    if (ch == '/')
      target = fpath;
    direct++;
  }

  strcpy(target, fname);
}

#if 0
int
is_fname(str)
  char *str;
{
  int ch;

  ch = *str;
  if (ch == '/')
    return 0;

  do
  {
    if (!is_alnum(ch) && !strchr("-._/+@", ch))
      return 0;
  } while (ch = *++str);
  return 1;
}


/* ----------------------------------------------------- */
/* transform to real path & security check		 */
/* ----------------------------------------------------- */


int
is_fpath(path)
  char *path;
{
  int ch, level;
  char *source, *target;

  level = 0;
  source = target = path;


  for (;;)
  {
    ch = *source;

    if (ch == '/')
    {
      int next;

      next = source[1];

      if (next == '/')
      {
	return 0;		/* [//] */
      }
      else if (next == '.')
      {
	next = source[2];

	if (next == '/')
	  return 0;		/* [/./] */

	if (next == '.' && source[3] == '/')
	{
	  /* -------------------------- */
	  /* abc/xyz/../def ==> abc/def */
	  /* -------------------------- */

	  for (;;)
	  {
	    if (target <= path)
	      return 0;

	    target--;
	    if (*target == '/')
	      break;
	  }

	  source += 3;
	  continue;
	}
      }

      level++;
    }

    *target = ch;

    if (ch == 0)
      return level;

    target++;
    source++;
  }
}
#endif
