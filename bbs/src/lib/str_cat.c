void
str_cat(dst, s1, s2)
  char *dst;
  char *s1;
  char *s2;
{
  while (*dst = *s1)
  {
    s1++;
    dst++;
  }

  while (*dst++ = *s2++)
    ;
}
