/*-------------------------------------------------------*/
/* guessnum.c   ( NTHU CS MapleBBS Ver 3.10 )            */
/*-------------------------------------------------------*/
/* author : thor.bbs@bbs.cs.nthu.edu.tw			 */
/* target : Guess Number tool dynamic link module        */
/* create : 99/02/16                                     */
/* update :   /  /                                       */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef HAVE_GAME


typedef char Num[4];

typedef struct
{
  Num n;
  int A, B;
}      His;

static int hisNum;
static His *hisList;

static int numNum;
static char *numSet;


static void 
AB(p, q, A, B)
  Num p, q;
  int *A, *B;
{				/* compare p and q, return ?A?B */
  int i, j;

  *A = *B = 0;
  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 4; j++)
    {
      if (p[i] == q[j])
      {
	if (i == j)
	  ++*A;
	else
	  ++*B;
      }
    }
  }
}


static int 
getth(o, a)
  int o;
  char *a;
{				/* return "o"th element index in a[], base 0 */
  int i = -1;

  o++;
  while (o)
  {
    if (a[++i])
      continue;
    else
      o--;
  }
  return i;
}


static void 
ord2Num(o, p)
  int o;
  Num p;
{				/* return "o"th filtered number */
  char digit[10];
  int i, j, k;

  memset(digit, 0, sizeof digit);

  for (j = 0, k = 10; j < 4; j++, k--)
  {
    i = o % k;
    o /= k;
    i = getth(i, digit);
    p[j] = i;
    digit[i] = 1;
  }
}


static int 
matchHis(n)
  Num n ;
{
  int i, A, B;

  for (i = 0; i < hisNum; i++)
  {
    AB(n, hisList[i].n, &A, &B);
    if (A != hisList[i].A || B != hisList[i].B)
      return 0;
  }
  return 1;
}


static void
finish(msg)
  char *msg;
{
  free(hisList);
  free(numSet);

  vmsg(msg);
}


static int
valid_guess(num)
  char *num;
{
  const char n0 = num[0];
  const char n1 = num[1];
  const char n2 = num[2];
  const char n3 = num[3];

  if (n0 >= '0' && n0 <= '9' && n1 >= '0' && n1 <= '9' &&
    n2 >= '0' && n2 <= '9' && n3 >= '0' && n3 <= '9' &&
    n0 != n1 && n0 != n2 && n0 != n3 && n1 != n2 && n1 != n3 && n2 != n3)
  {
    return 1;
  }
  return 0;
}


static int 
mainNum(fighting)
  int fighting;	/* Thor.990317: 對戰模式 */
{
  Num myNumber;

  if (vans("想好您的數字了嗎(Y/N)？[N] ") != 'y')
  {
    /* vmsg(MSG_QUITGAME); */	/* itoc.010312: 不要了 */
    return XEASY;
  }

  /* initialize variables */

  hisList = (His *) malloc(sizeof(His));	/* pseudo */
  numSet = (char *)malloc(10 * 9 * 8 * 7 * sizeof(char));

  hisNum = 0;
  numNum = 10 * 9 * 8 * 7;
  memset(numSet, 0, 10 * 9 * 8 * 7 * sizeof(char));

  /* Thor.990317:對戰模式 */
  vs_bar(fighting ? "猜數字大戰" : "傻瓜猜數字");

  if (fighting)
    ord2Num(rnd(numNum), myNumber);	/* Thor.990317:對戰模式 */

  /* while there is possibility */
  for (;;)
  {
    Num myGuess, yourGuess;
    int youA, youB, myA, myB;

    if (fighting)		/* Thor.990317:對戰模式 */
    {
      int i;
      char tmp[50];
      vget(b_lines - 3, 0, "您猜我的數字是[????]：", tmp, 5, DOECHO);
      if (!valid_guess(tmp))
	goto abort_game;

      for (i = 0; i < 4; i++)
	yourGuess[i] = tmp[i] - '0';
      AB(myNumber, yourGuess, &myA, &myB);
      move(b_lines - 2, 0);
      prints("我說 \033[1m%dA%dB \033[m", myA, myB);

      if (myA == 4)
      {
	/* you win  */
	finish("您贏了! 好崇拜 ^O^");
	return 0;
      }
    }

    /* pickup a candidate number */
    for (;;)
    {
      int i;
      /* pickup by random */
      if (numNum <= 0)
	goto foolme;
      i = rnd(numNum);
      i = getth(i, numSet);	/* i-th ordering num */
      numSet[i] = 1;
      numNum--;			/* filtered out */
      ord2Num(i, myGuess);	/* convert ordering num to Num */

      /* check history */
      if (matchHis(myGuess))
	break;
    }

    /* show the picked number */
    move(b_lines - 1, 0);
    prints("我猜您的數字是 \033[1;37m%d%d%d%d\033[m", myGuess[0], myGuess[1], myGuess[2], myGuess[3]);

    /* get ?A?B */
    for (;;)
    {
      char buf[5];
      /* get response */
      vget(b_lines, 0, "您的回答[?A?B]：", buf, 5, DOECHO);

      if (!buf[0])
      {
    abort_game:
	finish(MSG_QUITGAME);
	return 0;
      }
      if (isdigit(buf[0]) && (buf[1] | 0x20) == 'a'
	&& isdigit(buf[2]) && (buf[3] | 0x20) == 'b')
      {
	youA = buf[0] - '0';
	youB = buf[2] - '0';
	/* check legimate */
	if (youA >= 0 && youA <= 4
	  && youB >= 0 && youB <= 4
	  && youA + youB <= 4)
	{
	  /* if 4A, end the game */
	  if (youA == 4)
	  {
	    /* I win  */
	    finish("我贏了! 厲害吧 ^O^");
	    return 0;
	  }
	  else
	  {
	    break;
	  }
	}
      }
      /* err A B */
      zmsg("輸入格式有誤");
    }
    /* put in history */
    hisNum++;
    hisList = (His *) realloc(hisList, hisNum * sizeof(His));	/* assume must succeeded */
    memcpy(hisList[hisNum - 1].n, myGuess, sizeof(Num));
    hisList[hisNum - 1].A = youA;
    hisList[hisNum - 1].B = youB;

    move(hisNum + 2, 0);
    if (fighting)		/* Thor.990317: 對戰模式 */
      prints("第 \033[1;37m%d\033[m 次, 您猜 \033[1;36m%d%d%d%d\033[m, 我說 \033[1;33m%dA%dB\033[m; 我猜 \033[1;33m%d%d%d%d\033[m, 您說 \033[1;36m%dA%dB\033[m", hisNum, yourGuess[0], yourGuess[1], yourGuess[2], yourGuess[3], myA, myB, myGuess[0], myGuess[1], myGuess[2], myGuess[3], youA, youB);
    else
      prints("第 \033[1;37m%d\033[m 次, 我猜 \033[1;33m%d%d%d%d\033[m, 您說 \033[1;36m%dA%dB\033[m", hisNum, myGuess[0], myGuess[1], myGuess[2], myGuess[3], youA, youB);
  }

foolme:
  /* there is no posibility, show "you fool me" */
  finish("您騙我！不跟您玩了 ~~~>_<~~~");

  return 0;
}


int 
guessNum()
{
  return mainNum(0);
}


int 
fightNum()
{
  return mainNum(1);
}

#endif	/* HAVE_GAME */
