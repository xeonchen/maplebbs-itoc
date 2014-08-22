/* a alternative str_str() using KMP algorithm. */
/* English case independent and BIG5 Chinese supported */

#include <stdlib.h>


void
str_expand(dst, src)	/* 將 char 轉為 short，並將英文變小寫 */
  char *dst, *src;
{
  int ch;
  int in_chi = 0;	/* 1: 前一碼是中文字 */

  do
  {
    ch = *src++;

    if (in_chi || ch & 0x80)
    {
      in_chi ^= 1;
    }
    else
    {    
      if (ch >= 'A' && ch <= 'Z')
        ch |= 0x20;
      *dst++ = 0;
    }
    *dst++ = ch;
  } while (ch);
}


void
str_str_kmp_tbl(pat, tbl)
  const short *pat;
  int *tbl;
{
  register short c;
  register int i, j;

  tbl[0] = -1;
  for (j = 1; c = pat[j]; j++)
  {
    i = tbl[j - 1];
    while (i >= 0 && c != pat[i + 1])
      i = tbl[i];
    tbl[j] = (c == pat[i + 1]) ? i + 1 : -1;
  }
}


const int
str_str_kmp(str, pat, tbl)
  const short *str;
  const short *pat;
  const int *tbl;
{
  register const short *i;
  register int j;

  for (i = str, j = 0; *i && pat[j];)
  {
    if (*i == pat[j])
    {
      j++;
    }
    else if (j)
    {
      j = tbl[j - 1] + 1;
      continue;		/* 不需要 i++ */
    }
    i++;
  }

  /* match */
  if (!pat[j])
    return 1;

  return 0;
}


#undef	TEST

#ifdef TEST
static void
try_match(str, key)
  char *str, *key;
{
  short a[256], b[256];		/* 假設 256 已足夠 */
  int tbl[256];

  str_expand(a, str);
  str_expand(b, key);

  str_str_kmp_tbl(key, tbl);
  printf("「%s」 %s包括 「%s」\n", 
    str, str_str_kmp(a, b, tbl) ? "" : "不", key);
}


int
main()
{
  try_match("好的電影", "犒");
  try_match("好的電影", "N");
  try_match("好的電影", "n");
  try_match("好的電影", "好的");

  try_match("x好的x電影", "漩電");
  try_match("x好的x電影", "的x");
  try_match("x好的x電影", "的X");
  try_match("x好的X電影", "的x");

  try_match("abx好的x電影", "x電");

  return 0;
}
#endif
