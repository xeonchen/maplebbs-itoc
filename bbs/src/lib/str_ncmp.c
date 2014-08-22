int
str_ncmp(s1, s2, n)
  char *s1, *s2;
  int n;
{
  int c1, c2;

  while (n--)
  {
    c1 = *s1++;
    if (c1 >= 'A' && c1 <= 'Z')
      c1 |= 0x20;

    c2 = *s2++;
    if (c2 >= 'A' && c2 <= 'Z')
      c2 |= 0x20;

    if (c1 -= c2)
      return (c1);

    if (!c2)
      break;
  }

  return 0;
}
