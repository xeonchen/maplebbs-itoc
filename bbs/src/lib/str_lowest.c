void
str_lowest(dst, src)
  char *dst, *src;
{
  int ch;
  int in_chi = 0;	/* 1: 前一碼是中文字 */

  do
  {
    ch = *src++;
    if (in_chi || ch & 0x80)
      in_chi ^= 1;
    else if (ch >= 'A' && ch <= 'Z')
      ch |= 0x20;
    *dst++ = ch;
  } while (ch);
}
