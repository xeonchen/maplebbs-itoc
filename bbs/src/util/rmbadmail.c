/*-------------------------------------------------------*/
/* util/rmbadmail.c      ( YZU WindTop 2000)             */
/*-------------------------------------------------------*/
/* author : visor.bbs@bbs.yzu.edu.tw                     */
/* target : 刪除[信箱|看板]未在 .DIR 裡的信件            */
/* create : 00/08/30                                     */
/* update : 01/02/16                                     */
/*-------------------------------------------------------*/
/* syntax : rmbadmail                                    */
/*-------------------------------------------------------*/


#undef  FAKE_IO
#include "bbs.h"


static int reserve, r_size, ulink, u_size;


static void
reaper(fpath, lowid)
  char *fpath;
  char *lowid;
{
  int fd, size, check;
  char buf[256], *fname, folder[128], *ptr;
  DIR *dirp;
  struct dirent *de;
  HDR *head, *tail, *base;
  struct stat st;
  time_t now;

  now = time(0) - 60;

  printf("> processing account %-20s ", lowid);

  sprintf(buf, "%s/.DIR", fpath);
  
  if ((fd = open(buf, O_RDONLY)) >= 0)
  {
    fstat(fd, &st);
    size = st.st_size / sizeof(HDR);
    
    if (size <= 0)
    {
      base = NULL;
    }
    else
    {
      base = (HDR *) malloc(sizeof(HDR) * size);
      tail = base + size;
      read(fd, base, sizeof(HDR) * size);
    }
    
    close(fd);
  }
  else
  {
    size = 0;
    base = NULL;
  }
  
  printf("total mail : %d\n", size);
  sprintf(folder, "%s/@", fpath);

  if (!(dirp = opendir(folder)))
  {
    if (base)
      free(base);
    return;
  }
  
  ptr = strchr(folder, '@') + 1;
  *ptr++ = '/';

  while (de = readdir(dirp))
  {
    check = 0;
    fname = de->d_name;
    if (fname[0] > ' ' && fname[0] != '.')
    {
      if (base)
      {
	for (head = base; head < tail; head++)
	{
	  if (!strcmp(head->xname, fname))
	  {
	    check = 1;
	    break;
	  }
	}
      }
      if (!check)
      {
	strcpy(ptr, fname);
	if (!(!stat(folder, &st) && (st.st_atime > now)))
	{
	  u_size += st.st_size;
	  ulink++;
	  printf("file is not in HDR : %s : unlink !\n", fname);
	  printf("--> unlinking %s\n", folder);

#ifndef FAKE_IO
	  unlink(folder);
#endif
	}
	else
	{
	  r_size += st.st_size;
	  reserve++;
	  printf("file is not in HDR : %s : reserve !\n", fname);
	}
      }
    }
  }
  closedir(dirp);
  free(base);
}


static void
expire(fpath, lowid)
  char *fpath;
  char *lowid;
{
  int fd, size, check;
  char buf[256], *fname, folder[128], *ptr, *str;
  DIR *dirp;
  struct dirent *de;
  HDR *head, *tail, *base;
  struct stat st;
  time_t now;

  now = time(0) - 60;

  printf("> processing board %-20s ", lowid);

  sprintf(buf, "%s/.DIR", fpath);
  if ((fd = open(buf, O_RDONLY)) >= 0)
  {
    fstat(fd, &st);
    size = st.st_size / sizeof(HDR);
    if (size <= 0)
    {
      base = NULL;
    }
    else
    {
      base = (HDR *) malloc(sizeof(HDR) * size);
      tail = base + size;
      read(fd, base, sizeof(HDR) * size);
    }
    close(fd);
  }
  else
  {
    base = NULL;
    size = 0;
  }
  printf("total article : %d\n", size);
  sprintf(folder, "%s/@", fpath);

  str = strchr(folder, '@');
  ptr = str + 1;
  *ptr++ = '/';
  *str = '0';

  while (1)
  {
    if ((dirp = opendir(folder)))
    {
      while (de = readdir(dirp))
      {
	check = 0;
	fname = de->d_name;
	if (fname[0] > ' ' && fname[0] != '.')
	{
	  if (base)
	  {
	    for (head = base; head < tail; head++)
	    {
	      if (!strcmp(head->xname, fname))
	      {
		check = 1;
		break;
	      }
	    }
	  }
	  if (!check)
	  {
	    strcpy(ptr, fname);
	    if (!(!stat(folder, &st) && (st.st_atime > now)))
	    {
	      u_size += st.st_size;
	      ulink++;
	      printf("file is not in HDR : %s : unlink !\n", fname);
	      printf("--> unlinking %s\n", folder);

#ifndef FAKE_IO
	      unlink(folder);
#endif
	    }
	    else
	    {
	      r_size += st.st_size;
	      reserve++;
	      printf("file is not in HDR : %s : reserve !\n", fname);
	    }
	  }
	}
      }
      closedir(dirp);
    }
    if (++(*str) == ('9' + 1))
      *str = 'A';
    if ((*str) == 'W')
      break;
  }
  free(base);
}


static void
traverse(fpath, mode)
  char *fpath;
  int mode;
{
  DIR *dirp;
  struct dirent *de;
  char *fname, *str;

  if (!(dirp = opendir(fpath)))
  {
    return;
  }
  for (str = fpath; *str; str++);
  *str++ = '/';

  while (de = readdir(dirp))
  {
    fname = de->d_name;
    if (fname[0] > ' ' && fname[0] != '.')
    {
      strcpy(str, fname);
      if (mode == 1)
	reaper(fpath, fname);
      else if (mode == 2)
	expire(fpath, fname);
    }
  }
  closedir(dirp);
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int ch;
  char *fname, fpath[256], buf[32];

  chdir(BBSHOME);

  if (argc > 1 && !strncmp(argv[1], "-a", 2))
  {
    if (argc > 2)
    {
      str_lower(buf, argv[2]);
      sprintf(fpath, "usr/%c/%s", *buf, buf);
      if (!access(fpath, 0))
	reaper(fpath, buf);
      else
	printf("error open account %s\n", buf);
    }
    else
    {
      strcpy(fname = fpath, "usr/@");
      fname = (char *) strchr(fname, '@');
      for (ch = 'a'; ch <= 'z'; ch++)
      {
	fname[0] = ch;
	fname[1] = '\0';
	traverse(fpath, 1);
      }
      for (ch = '0'; ch <= '9'; ch++)
      {
	fname[0] = ch;
	fname[1] = '\0';
	traverse(fpath, 1);
      }
    }
  }
  else if (argc > 1 && !strncmp(argv[1], "-b", 2))
  {
    if (argc > 2)
    {
      strcpy(buf, argv[2]);
      sprintf(fpath, "brd/%s", buf);
      if (!access(fpath, 0))
	expire(fpath, buf);
      else
	printf("error open board %s\n", buf);
    }
    else
    {
      strcpy(fpath, "brd");
      traverse(fpath, 2);
    }
  }
  else
  {
    printf("syntax : rmbadmail [-a|-b] [account|board]\n");
  }
  printf("total unlink  %10d  unlink  size : %10d\n", ulink, u_size);
  printf("total reserve %10d  reserve size : %10d\n", reserve, r_size);
  return 0;
}
