/*
 * str_ncpy() - similar to strncpy(3) but terminates string always with '\0'
 * if n != 0, and doesn't do padding
 */


void
str_ncpy(dst, src, n)
  char *dst;
  char *src;
  int n;
{
  char *end;

  end = dst + n - 1;

  do
  {
    n = (dst >= end) ? 0 : *src++;
    *dst++ = n;
  } while (n);
}
