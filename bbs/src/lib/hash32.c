int
hash32(str)
  unsigned char *str;
{
  int xo, cc;

  xo = 1048583;			/* a big prime number */
  while (cc = *str++)
  {
    xo = (xo << 5) - xo + cc;	/* 31 * xo + cc */
  }
  return (xo & 0x7fffffff);
}
