#include <fcntl.h>


int
f_mv(src, dst)
  char *src, *dst;
{
  int ret;

  if (ret = rename(src, dst))
  {
    ret = f_cp(src, dst, O_TRUNC);
    if (!ret)
      unlink(src);
  }
  return ret;
}
