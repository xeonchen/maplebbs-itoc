int
str_cmp(s1, s2)
  char *s1, *s2;
{
  int c1, c2, diff;

  do
  {
    c1 = *s1++;
    c2 = *s2++;
    if (c1 >= 'A' && c1 <= 'Z')
      c1 |= 0x20;
    if (c2 >= 'A' && c2 <= 'Z')
      c2 |= 0x20;
    if (diff = c1 - c2)
      return (diff);
  } while (c1);
  return 0;
}
