int
str_hash(str, seed)
  char *str;
  int seed;
{
  int cc;

  while (cc = *str++)
  {
    seed = (seed << 5) - seed + cc;	/* 31 * seed + cc */
  }
  return (seed & 0x7fffffff);
}
