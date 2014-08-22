/*-------------------------------------------------------*/
/* race.c         ( NTHU CS MapleBBS Ver 3.10 )          */
/*-------------------------------------------------------*/
/* target : ÁÉ°¨³õ¹CÀ¸                                   */
/* create : 98/12/17                                     */
/* update : 01/04/21                                     */
/* author : SugarII (u861838@Oz.nthu.edu.tw)             */
/* recast : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/*-------------------------------------------------------*/


#include "bbs.h"


#ifdef HAVE_GAME


static int pace[5];		/* ¤­¤Ç°¨¶]¤F¦h»· */


static void
out_song()
{
  static int count = 0;  

  /* ±i«B£»¤@¨¥ÃøºÉ */
  uschar *msg[13] = 
  {
    "©pµ¹§Ú¤@³õÀ¸  ©p¬ÝµÛ§Ú¤J°g",
    "³Q©p±q¤ß¸Ì­é¸¨ªº·P±¡  µhªº¤£ª¾«ç»ò±Ë¥h",
    "¿ð¿ð¤£¯à¬Û«H³o·PÄ±  ¹³¦Û¤v©M¦Û¤v¤ÀÂ÷",
    "¦Ó«H»}¥¹¥¹ªº·R±¡  ¦b¨º¸Ì",
    "§Ú¤@¨¥ÃøºÉ  §Ô¤£¦í¶Ë¤ß",
    "¿Å¶q¤£¥X·R©Î¤£·R¤§¶¡ªº¶ZÂ÷",
    "©p»¡©pªº¤ß  ¤£¦A·Å¼ö¦p©õ",
    "±q¨º¸Ì¶}©l  ±q¨º¸Ì¥¢¥h",
    "§Ú¤@¨¥ÃøºÉ  §Ô¤£¦í¶Ë¤ß",
    "¿Å¶q¤£¥X·R©Î¤£·R¤§¶¡ªº¶ZÂ÷",
    "ÁôÁô¬ù¬ù¤¤  ©ú¥Õ©pªº¨M©w",
    "¤£´±«j±j©p  ¥u¦n¬°Ãø¦Û¤v",
    "§Ú¬°Ãø§Ú¦Û¤v  §Ú¬°Ãø§Ú¦Û¤v"
  };
  move(b_lines - 2, 0);
  prints("\033[1;3%dm%s\033[m  Äw½XÁÙ¦³ %d ¤¸", time(0) % 7, msg[count], cuser.money);
  clrtoeol();
  if (++count == 13)
    count = 0;
}


static int			/* -1: ÁÙ¨S¤À¥X³Ó­t 0~4:Ä¹ªº¨º¤Ç°¨ */
race_path(run, j, step)
  int run, j, step;
{
  int i;

  if (!step)
  {
    return -1;
  }
  else if (step < 0)
  {
    if (j + step < 0)
      j = -step;
    move(run + 9, (j + step) * 2 + 8);
    clrtoeol();
    pace[run] += step * 100;
    if (pace[run] < 1)
      pace[run] = 1;
    return -1;
  }

  /* step > 0 */
  move(run + 9, j * 2 + 8);
  for (i = 0; i < step; i++)
    outs("¡½");

  if (pace[run] + step * 100 > 3000)
    return run;
  return -1;
}


int
main_race()
{
  int money[5];			/* ¤­¤Ç°¨ªº©ãª÷ */
  int speed[5];			/* ¤­¤Ç°¨ªº³t«× */
  int stop[5];			/* ¤­¤Ç°¨ªº¼È°± */
  int bomb;			/* ¬O§_¨Ï¥Î¬µ¼u */
  int run;			/* ¥Ø«e¦b­pºâªº¨º¤Ç */
  int win;			/* ­þ¤Ç°¨Ä¹¤F */
  int flag;			/* ¨Æ¥óµo¥Í¦¸¼Æ */
  int i, j, ch;
  char buf[60];
  char *racename[5] = {"¨ª¨ß", "ªº¿c", "¤ö¶À", "­¸¹q", "¦½¦å"};

  if (HAS_STATUS(STATUS_COINLOCK))
  {
    vmsg(msg_coinlock);
    return XEASY;
  }

  while (1)
  {
    vs_bar("ÁÉ°¨³õ");
    out_song();
    bomb = 0;
    win = -1;
    flag = 0;
    for (i = 0; i < 5; i++)
    {
      pace[i] = 1;
      stop[i] = money[i] = 0;
      speed[i] = 100;
    }
    move(5, 0);
    outs("  \033[1m°¨¦W¡G\033[m");
    for (i = 0; i < 5; i++)
      prints("     %d. \033[1;3%dm%s\033[m", i + 1, i + 1, racename[i]);
    outs("\n  \033[1m³t«×¡G\033[m\n  \033[1m½äª÷¡G\033[m\n\n");

    for (i = 0; i < 5; i++)
      prints("%d.\033[1;3%dm%s\033[mùø\n", i + 1, i + 1, racename[i]);
    outs("¢w¢w¢wùö¢w¢w¢r¢w¢w¢r¢w¢w¢r¢w¢w¢r¢w¢w¢r¢w¢w¢r¢w¢w¢r¢w¢w¢r¢w¢w¢r¢w¢wù÷");

    while (1)
    {
      /* ¨M©w¦U¤Ç°¨½äª` */
      ch = vget(2, 0, "±z­n©ã­þ¤Ç°¨(1-5)¡H[S]¶}©l [Q]Â÷¶}¡G", buf, 3, DOECHO);
      i = buf[0];
      if (!ch || i == 's')
      {
	if (money[0] || money[1] || money[2] || money[3] || money[4])
	  break;
	addmoney(money[0] + money[1] + money[2] + money[3] + money[4]);	/* ÁÙ¿ú */
	goto abort_game;
      }
      else if (i < '1' || i > '5')
      {
	addmoney(money[0] + money[1] + money[2] + money[3] + money[4]);	/* ÁÙ¿ú */
	goto abort_game;
      }

      ch = vget(3, 0, "­n©ã¦h¤Ö½äª÷¡H", buf, 6, DOECHO);
      j = atoi(buf);
      if (!ch)
      {
	if (money[0] || money[1] || money[2] || money[3] || money[4])
	  break;
	addmoney(money[0] + money[1] + money[2] + money[3] + money[4]);	/* ÁÙ¿ú */
	goto abort_game;
      }
      if (j < 1 || j > cuser.money)
      {
	addmoney(money[0] + money[1] + money[2] + money[3] + money[4]);	/* ÁÙ¿ú */
	goto abort_game;
      }

      money[i - '1'] += j;
      cuser.money -= j;

      move(7, 15);
      clrtoeol();
      outs("\033[1m");
      for (i = 0; i < 5; i++)
	prints("     \033[3%dm%7d", i + 1, money[i]);
      outs("\033[m");
      out_song();
    }

    /* ¶}©l¹CÀ¸ */
    move(3, 0);
    clrtoeol();
    move(2, 0);
    clrtoeol();
    outs("-== ½Ð«ö \033[1;36mk\033[m ¬°±z¿ïªº«l¾s¥[ªo¡A«ö \033[1;36mz\033[m ¥i¥á¥X¬µ¼u(¥u¦³¤@¦¸¾÷·|) ==-");

    while (win < 0)
    {
      move(6, 15);
      clrtoeol();
      outs("\033[1m");
      for (i = 0; i < 5; i++)
      {
	if (stop[i] < 1)
	  speed[i] += rnd(20) - (speed[i] + 170) / 30;
	if (speed[i] < 0)
	  speed[i] = 0;
	prints("     \033[3%dm%7d", i + 1, speed[i]);
      }
      outs("\033[m");

      do
      {
	ch = igetch();
      } while (ch != 'k' && (ch != 'z' || bomb));

      run = rnd(5);		/* ¿ï¾Ü¨Æ¥óµo¥Í¹ï¶H */
      flag %= 5;		/* ¦C¦L¨Æ¥ó©ó¿Ã¹õ¤W */
      move(15 + flag, 0);
      clrtoeol();

      if (ch == 'z')		/* ¥á¬µ¼u */
      {
	stop[run] = 3;
	prints("\033[1m«OÄÖ²y¯{¨ì\033[3%dm%s\033[37m°±¤î«e¶i¤T¦¸¡A³t«× = 0\033[m",
	  run + 1, racename[run]);
	speed[run] = 0;
	flag++;
	bomb = 1;
      }
      else if (rnd(12) == 0)	/* ¯S®í¨Æ¥ó */
      {
	prints("\033[1;3%dm%s\033[36m", run + 1, racename[run]);

	switch (rnd(14))
	{
	case 0:
	  outs("ªA¤U«Â¦Ó­è¡A³t«× x1.5\033[m");
	  speed[run] *= 1.5;
	  break;
	case 1:
	  outs("¨Ï¥XºµªºÃzµo¤O¡A«e¶i¤­®æ\033[m");
	  win = race_path(run, pace[run] / 100, 5);
	  pace[run] += 500;
	  break;
	case 2:
	  outs("½ò¨ì¦a¹p¡A³t«×´î¥b\033[m");
	  speed[run] /= 2;
	  break;
	case 3:
	  outs("½ò¨ì­»¿¼¥Ö·Æ­Ë¡A¼È°±¤G¦¸\033[m");
	  stop[run] += 2;
	  break;
	case 4:
	  outs("½Ð¯«¤W¨­¡A¼È°±¥|¦¸¡A³t«×¥[­¿\033[m");
	  stop[run] += 4;
	  speed[run] *= 2;
	  break;
	case 5:
	  outs("°Û¥X¤jÅ]ªk©G¡A¨Ï¨ä¥L¤H¼È°±¤T¦¸\033[m");
	  for (i = 0; i < 5 && i != run; i++)
	    stop[i] += 3;
	  break;
	case 6:
	  outs("Å¥¨£ badboy ªº¥[ªoÁn¡A³t«× +100\033[m");
	  speed[run] += 100;
	  break;
	case 7:
	  outs("¨Ï¥X¿ûÅKÅÜ¨­¡A«e¶i¤T®æ¡A³t«× +30\033[m");
	  win = race_path(run, pace[run] / 100, 3);
	  speed[run] += 30;
	  break;
	case 8:
	  outs("°I¯«¤W¨­³t«×´î¥b¡A®ÇÃä¼È°±¤G¦¸\033[m");
	  speed[run] /= 2;
	  if (run > 0)
	    stop[run - 1] += 2;
	  if (run < 4)
	    stop[run + 1] += 2;
	  break;
	case 9:
	  outs("³Q¶A©G¡A¦^¨ì°_ÂI\033[m");
	  win = race_path(run, pace[run] / 100, -30);
	  break;
	case 10:
	  if (pace[0] + pace[1] + pace[2] + pace[3] + pace[4] > 6000)
	  {
	    outs("\033[5m¨Ï¥X³Í¤ÆÂÈ³³¡A©Ò¦³¤H¦^¨ì°_ÂI\033[m");
	    for (i = 0; i < 5; i++)
	      win = race_path(i, pace[i] / 100, -30);
	  }
	  else
	  {
	    outs("¨Ï¥X¥øÃZ¼u¸õ¡A³t«× x1.3¡A¨ä¥L¤H´î¥b\033[m");
	    for (i = 0; i < 5 && i != run; i++)
	      speed[i] /= 2;
	    speed[run] *= 1.3;
	  }
	  break;
	case 11:
	  if (money[run])
	  {
	    outs("¾ß¨ì«Ü¦h¿ú¡A¼È°±¤@¦¸\033[m");
	    addmoney(money[run]);
	    out_song();
	    stop[run]++;
	  }
	  else
	  {
	    outs("¾ã¤Ç°¨²n°_¨Ó¤F¡A³t«× +50\033[m");
	    speed[run] += 50;
	  }
	  break;
	case 12:
	  j = rnd(5);
	  prints("·R¤W¤F[3%dm%s[36m¡A³t«×¸ò¨e¤@¼Ë", j + 1, racename[j]);
	  speed[run] = speed[j];
	  break;
	case 13:
	  if (money[run] > 0)
	  {
	    outs("ªº½äª÷ x1.5¡AÁÈ°Õ¡I\033[m");
	    money[run] *= 1.5;
	    move(7, 15);
	    clrtoeol();
	    outs("\033[1m");
	    for (i = 0; i < 5; i++)
	      prints("     \033[3%dm%7d ", i + 1, money[i]);
	    outs("\033[m");
	  }
	  else
	  {
	    outs("¾c¤l±¼¤F¡A°h«á¤T®æ\033[m");
	    race_path(run, pace[run] / 100, -3);
	  }
	  break;
	}
      }
      else		/* ©¹«e¶] */
      {
	if (stop[run])
	{
	  prints("\033[1;3%dm%s\033[37m ª¦¤£°_¨Ó\033[m", run + 1, racename[run]);
	  stop[run]--;
	}
	else
	{
	  prints("\033[1;3%dm%s\033[37m ©é©R©b¶]\033[m", run + 1, racename[run]);
	  i = pace[run] / 100;
	  win = race_path(run, i, (pace[run] + speed[run]) / 100 - i);
	  pace[run] += speed[run];
	}
      }
      flag++;
    }
    move(b_lines - 1, 0);
    prints("\033[1;35m¡¹ \033[37m¹CÀ¸µ²§ô \033[35m¡¹ \033[37mÀò³Óªº¬O\033[3%dm %s \033[m",
      win + 1, racename[win]);
    if (money[win])
    {
      money[win] += money[win] * (pace[win] - (pace[0] + pace[1] + pace[2] + pace[3] + pace[4]) / 5) / 500;
      sprintf(buf, "®¥³ß±z©ã¤¤¤F¡AÀò±o¼úª÷ %d ¤¸", money[win]);
      addmoney(money[win]);
    }
    else
    {
      strcpy(buf, "©êºp...±z¨S©ã¤¤£¬~~~");
    }
    vmsg(buf);
  }

abort_game:
  return 0;
}
#endif				/* HAVE_GAME */
