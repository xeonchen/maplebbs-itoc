#include <string.h>

void
str_folder(fpath, folder, fname)
  char *fpath;
  char *folder;
  char *fname;
{
  int ch;
  char *token;

  while (ch = *folder++)
  {
    *fpath++ = ch;
    if (ch == '/')
      token = fpath;
  }
  if (*token != '.')
    token -= 2;
  strcpy(token, fname);
}
