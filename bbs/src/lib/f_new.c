/* ----------------------------------------------------- */
/* exclusively create file [*.n]			 */
/* ----------------------------------------------------- */


#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>


FILE *
f_new(fold, fnew)
  char *fold;
  char *fnew;
{
  int fd, try;
  extern int errno;

  try = 0;
  str_cat(fnew, fold, ".n");

  for (;;)
  {
    fd = open(fnew, O_WRONLY | O_CREAT | O_EXCL, 0600);

    if (fd >= 0)
      return fdopen(fd, "w");

    if (errno != EEXIST)
      break;

    if (!try++)
    {
      struct stat st;

      if (stat(fnew, &st))
	break;
      if (st.st_mtime < time(NULL) - 20 * 60)	/* 假設 20 分鐘內應該處理完 */
	unlink(fnew);
    }
    else
    {
      if (try > 24)		/* 等待 120 秒鐘 */
	break;
      sleep(5);
    }
  }
  return NULL;
}
