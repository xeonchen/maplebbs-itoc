#include <string.h>
#include <sys/stat.h>
#include <dirent.h>


static int
rm_dir(fpath)
  char *fpath;
{
  struct stat st;
  DIR *dirp;
  struct dirent *de;
  char buf[256], *fname;

  if (!(dirp = opendir(fpath)))
    return -1;

  for (fname = buf; *fname = *fpath; fname++, fpath++)
    ;

  *fname++ = '/';

  while (de = readdir(dirp))
  {
    fpath = de->d_name;

    /* skip ./ ¤Î ../ */
    if (!*fpath || (*fpath == '.' && (fpath[1] == '\0' || (fpath[1] == '.' && fpath[2] == '\0'))))
      continue;

    strcpy(fname, fpath);
    if (!lstat(buf, &st))
    {
      if (S_ISDIR(st.st_mode))
	rm_dir(buf);
      else
	unlink(buf);
    }
  }
  closedir(dirp);

  *--fname = '\0';
  return rmdir(buf);
}


int
f_rm(fpath)
  char *fpath;
{
  struct stat st;

  if (stat(fpath, &st))
    return -1;

  if (!S_ISDIR(st.st_mode))
    return unlink(fpath);

  return rm_dir(fpath);
}
