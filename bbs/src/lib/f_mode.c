#include <sys/stat.h>


int
f_mode(fpath)
  char *fpath;
{
  struct stat st;

  if (stat(fpath, &st))
    return 0;

  return st.st_mode;
}
