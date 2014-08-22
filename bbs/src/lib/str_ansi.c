/* ----------------------------------------------------- */
/* ¥h°£ ANSI ±±¨î½X					 */
/* ----------------------------------------------------- */


void
str_ansi(dst, str, max)		/* strip ANSI code */
  char *dst, *str;
  int max;
{
  int ch, ansi;
  char *tail;

  for (ansi = 0, tail = dst + max - 1; ch = *str; str++)
  {
    if (ch == '\n')
    {
      break;
    }
    else if (ch == '\033')
    {
      ansi = 1;
    }
    else if (ansi)
    {
      if ((ch < '0' || ch > '9') && ch != ';' && ch != '[')
	ansi = 0;
    }
    else
    {
      *dst++ = ch;
      if (dst >= tail)
	break;
    }
  }
  *dst = '\0';
}
