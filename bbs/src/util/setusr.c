/*-------------------------------------------------------*/
/* util/setusr.c	( NTHU CS MapleBBS Ver 3.10 )    */
/*-------------------------------------------------------*/
/* target : 設定使用者資料                               */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/* create : 01/07/16                                     */
/* update :                                              */
/*-------------------------------------------------------*/


#if 0
  設定 itoc 的 姓名為王大明 暱稱為大明 銀幣有1000 金幣有500
  setusr -r 王大明 -n 大明 -m 1000 -g 500 itoc

  設定 itoc 的權限
  setusr -p 11100...1101 itoc   (權限有 32 = NUMPERMS 種)
            ^^^^^ 32 個 0 和 1

  設定 itoc 的習慣
  setusr -f 11100...1101 itoc   (習慣有 27 = NUMUFOS 種)
            ^^^^^ 27 個 0 和 1

  該使用者必須不在線上才有效
#endif


#include "bbs.h"


#define MAXUSIES	9	/* 共有 9 種可以改的 */

static void
usage(msg)
  char *msg;
{
  int i, len;
  char buf[80];
  char *usies[MAXUSIES] =
  {
    "r realname", "n username", "m money", "g gold", "# userno", 
    "e email", "j 1/0(justify)", "p userlevel", "f ufo"
  };


  printf("Usage: %s [-%s] [-%s] [-%s] ... [-%s] UserID\n", 
    msg, usies[0], usies[1], usies[2], usies[MAXUSIES - 1]);
  len = strlen(msg);
  sprintf(buf, "%%%ds-%%s\n", len + MAXUSIES);
  for (i = 0; i < MAXUSIES; i++)
    printf(buf, "", usies[i]);
}


static usint
bitcfg(len, str)	/* config bits */
  int len;		/* 該欄位的長度 */
  char *str;		/* optarg */
{
  int i;
  char c;
  usint bits;

  if (strlen(str) != len)	/* 直接給個數字 */
  {
    bits = (usint) atoi(str);
  }
  else
  {
    bits = 0;
    for (i = 0; i < len; i++)
    {
      c = str[i];

      if (c != '0' && c != '1')
      {
	printf("bit 一定要是 0 或 1\n");
	exit(1);
      }

      bits <<= 1;
      bits |= c - '0';
    }
  }

  return bits;
}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int c;
  char *userid, fpath[64];
  ACCT acct;

  if (argc < 4 || argc % 2)	/* argc 要是偶數 */
  {
    usage(argv[0]);
    exit(1);
  }

  chdir(BBSHOME);

  userid = argv[argc - 1];
  usr_fpath(fpath, userid, FN_ACCT);
  if (rec_get(fpath, &acct, sizeof(ACCT), 0) < 0)
  {
    printf("%s: read error (maybe no such id?)\n", userid);
    exit(1);
  }

  while ((c = getopt(argc, argv, "r:n:m:g:#:e:j:p:f:")) != -1)
  {
    switch (c)
    {
    case 'r':		/* realname */
      strcpy(acct.realname, optarg);
      break;

    case 'n':		/* username */
      strcpy(acct.username, optarg);
      break;

    case 'm':		/* money */
      acct.money = atoi(optarg);
      break;

    case 'g':		/* gold */
      acct.gold = atoi(optarg);
      break;

    case '#':		/* userno */ 
      acct.userno = atoi(optarg);
      break;

    case 'e':		/* email */
      strcpy(acct.email, optarg);
      break;

    case 'j':		/* justify */
      if (atoi(optarg)) /* 認證通過 */
	acct.userlevel |= PERM_VALID;
      else
	acct.userlevel &= ~PERM_VALID;
      time(&acct.tvalid);
      break;

    case 'p':		/* userlevel */
      acct.userlevel = bitcfg(NUMPERMS, optarg);
      break;

    case 'f':		/* ufo */
      acct.ufo = bitcfg(NUMUFOS, optarg);
      break;

    default:
      usage(argv[0]);
      exit(0);
    }
  }

  if (rec_put(fpath, &acct, sizeof(ACCT), 0, NULL) < 0)
    printf("%s: write error\n", userid);
}
