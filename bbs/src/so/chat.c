/*-------------------------------------------------------*/
/* chat.c	( NTHU CS MapleBBS Ver 2.36 )		 */
/*-------------------------------------------------------*/
/* target : chat client for xchatd			 */
/* create : 95/03/29					 */
/* update : 95/12/15					 */
/*-------------------------------------------------------*/


#include "bbs.h"


#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


static char chatroom[IDLEN];	/* Chat-Room Name */
static int chatline;		/* Where to display message now */
static char chatopic[48];
static FILE *frec;


#define	stop_line	(b_lines - 2)


extern char *bmode();


static void
chat_topic()
{
  move(0, 0);
  prints("[1;37;46m %s«Ç¡G%-14s[45m ¸ÜÃD¡G%-48s[m",
    (frec ? "¿ý­µ" : "½Í¤Ñ"), chatroom, chatopic);
}


static void
printchatline(msg)
  char *msg;
{
  int line;

  line = chatline;
  move(line, 0);
  outs(msg);
  outc('\n');

  if (frec)
    fprintf(frec, "%s\n", msg);

  if (++line == stop_line)
    line = 2;
  move(line, 0);
  outs("¡÷");
  clrtoeol();
  chatline = line;
}


static void
chat_record()
{
  FILE *fp;
  time_t now;
  char buf[80];

  time(&now);

  if (fp = frec)
  {
    fprintf(fp, "%s\nµ²§ô¡G%s\n", msg_seperator, Btime(&now));
    fclose(fp);
    frec = NULL;
    printchatline("¡» ¿ý­µ§¹²¦¡I");
  }
  else
  {
#ifdef EVERY_Z
    /* Thor.980602: ¥Ñ©ó tbf_ask() »Ý°ÝÀÉ¦W¡A¦¹®É·|¥Î¨ì igetch()¡A
       ¬°¤F¨¾¤î I_OTHERDATA ³y¦¨·í¦í¡A¦b¦¹¥Î every_Z() ªº¤è¦¡¡A
       ¥ý«O¦s vio_fd¡A«Ý°Ý§¹«á¦AÁÙ­ì */

    vio_save();		/* Thor.980602: ¼È¦s vio_fd */
#endif

    usr_fpath(buf, cuser.userid, tbf_ask());

#ifdef EVERY_Z
    vio_restore();	/* Thor.980602: ÁÙ­ì vio_fd */
#endif

    move(b_lines, 0);
    clrtoeol();

    fp = fopen(buf, "a");
    if (fp)
    {
      fprintf(fp, "¥DÃD: %s\n¥]´[: %s\n¿ý­µ: %s (%s)\n¶}©l: %s\n%s\n",
	chatopic, chatroom, cuser.userid, cuser.username,
	Btime(&now), msg_seperator);
      printchatline("¡» ¶}©l¿ý­µÅo¡I");
      frec = fp;
    }
    else
    {
      printchatline("¡» ¿ý­µ¾÷¬G»Ù¤F¡A½Ð³qª¾¯¸ªøºû­×");
    }
  }
  bell();
  chat_topic();
}


static void
chat_clear()
{
  int line;

  for (line = 2; line < stop_line; line++)
  {
    move(line, 0);
    clrtoeol();
  }
  chatline = stop_line - 1;
  printchatline("");
}


static void
print_chatid(chatid)
  char *chatid;
{
  move(b_lines - 1, 0);
  outs(chatid);
  outc(':');
}


static inline int
chat_send(fd, buf)
  int fd;
  char *buf;
{
  int len;

  len = strlen(buf);
  return (send(fd, buf, len, 0) == len);
}


static inline int
chat_recv(fd, chatid)
  int fd;
  char *chatid;
{
  static char buf[512];
  static int bufstart = 0;
  int cc, len;
  char *bptr, *str;

  bptr = buf;
  cc = bufstart;
  len = sizeof(buf) - cc - 1;
  if ((len = recv(fd, bptr + cc, len, 0)) <= 0)
    return -1;
  cc += len;

  for (;;)
  {
    len = strlen(bptr);

    if (len >= cc)
    {				/* wait for trailing data */
      memcpy(buf, bptr, len);
      bufstart = len;
      break;
    }
    if (*bptr == '/')
    {
      str = bptr + 1;
      fd = *str++;

      if (fd == 'c')
      {
	chat_clear();
      }
      else if (fd == 'n')
      {
	str_ncpy(chatid, str, 9);
         
	/* Thor.980819: ¶¶«K´«¤@¤U mateid ¦n¤F... */
	str_ncpy(cutmp->mateid, str, sizeof(cutmp->mateid));

	print_chatid(chatid);
	clrtoeol();
      }
      else if (fd == 'r')
      {
	str_ncpy(chatroom, str, sizeof(chatroom));
	chat_topic();
      }
      else if (fd == 't')
      {
	str_ncpy(chatopic, str, sizeof(chatopic));
	chat_topic();
      }
    }
    else
    {
      printchatline(bptr);
    }

    cc -= ++len;
    if (cc <= 0)
    {
      bufstart = 0;
      break;
    }
    bptr += len;
  }

  return 0;
}


static void
chat_pager(arg)
  char *arg;
{
  cuser.ufo ^= UFO_PAGER;
  cutmp->ufo ^= UFO_PAGER;
  /* Thor.980805: ¸Ñ¨Mufo ¦P¨B°ÝÃD */

  sprintf(arg, "¡» ±zªº©I¥s¾¹¤w¸g%s¤F!",
    cuser.ufo & UFO_PAGER ? "Ãö³¬" : "¥´¶}");
  printchatline(arg);
}


#if 0
/* Thor.980727: ©M /flag ½Äkey */
static void
chat_write(arg)
  char *arg;
{
  int uno;
  UTMP *up;
  char *str;
  CallMsg cmsg;

  strtok(arg, STR_SPACE);
  if ((str = strtok(NULL, STR_SPACE)) && (uno = acct_userno(str)) > 0)
  {
    cmsg.recver = uno;		/* ¥ý°O¤U userno §@¬° check */
    if (up = utmp_find(uno))
    {
      if (can_override(up))
      {
	if (str = strtok(NULL, "\n"))	/* Thor.980725:§ì¾ã¥y¸Ü */
	{			/* Thor.980724: ±q my_write §ï¹L¨Ó */
	  int len;
	  char buf[80];
	  extern char fpmsg[];
	  /* Thor.980722: msg file¥[¤W¦Û¤v»¡ªº¸Ü */

	  sprintf(fpmsg + 4, "%s-", cuser.userid);
	  /* Thor.980722: ­É¥Î len·í¤@¤Ufd :p */
	  len = open(fpmsg, O_WRONLY | O_CREAT | O_APPEND, 0600);
	  sprintf(buf, "µ¹%s¡G%s\n", up->userid, str);
	  write(len, buf, strlen(buf));
	  close(len);

	  sprintf(buf, "%s(%s", cuser.userid, cuser.username);
	  len = strlen(str);
	  buf[71 - len] = '\0';
	  sprintf(cmsg.msg, "\033[1;33;46m¡¹ %s) \033[37;45m %s \033[m", buf, str);

	  cmsg.caller = cutmp;
	  cmsg.sender = cuser.userno;

	  if (do_write(up, &cmsg))
	    printchatline("¡» ¹ï¤è¤w¸gÂ÷¥h");
	}
	else
	{
	  printchatline("¡» §O¥u¯w²´¡A»¡¨Ç¸Ü§a¡I");
	}
      }
      else
      {
	printchatline("¡» ¹ï¤è§â¦Õ¦·Ý³¦í»¡¡G¡y§Ú¨SÅ¥¨ì¡K¡K§Ú¨SÅ¥¨ì¡K¡K¡z");
      }
    }
    else
    {
      printchatline("¡» ¹ï¤è¤£¦b¯¸¤W");
    }
  }
  else
  {
    printchatline(err_uid);
  }
}


static int
printuserent(uentp)
  user_info *uentp;
{
  static char uline[80];
  static int cnt;
  char pline[30];
  int cloak;

  if (!uentp)
  {
    if (cnt)
      printchatline(uline);
    memset(uline, 0, 80);
    return cnt = 0;
  }
  cloak = uentp->ufo & UFO_CLOAK;
  if (cloak && !HAS_PERM(PERM_SEECLOAK))
    return 0;

  sprintf(pline, " %-13s%c%-10s", uentp->userid,
    cloak ? '#' : ' ', bmode(uentp, 1));
  if (cnt < 2)
    strcat(pline, "¢x");
  strcat(uline, pline);
  if (++cnt == 3)
  {
    printchatline(uline);
    memset(uline, 0, 80);
    cnt = 0;
  }
  return 0;
}


static void
chat_users()
{				/* ¦]¬°¤H¼Æ°Ê»³¤W¦Ê¡A·N¸q¤£¤j */
  printchatline("");
  printchatline("¡i " BBSNAME "¹C«È¦Cªí ¡j");
  printchatline(MSG_CHAT_ULIST);

  if (apply_ulist(printuserent) == -1)
    printchatline("ªÅµL¤@¤H");

  printuserent(NULL);
}
#endif


struct chat_command
{
  char *cmdname;		/* Char-room command length */
  void (*cmdfunc) ();		/* Pointer to function */
};


struct chat_command chat_cmdtbl[] = 
{
  {"pager", chat_pager},
  {"tape", chat_record},

#if 0
  /* Thor.980727: ©M /flag ½Äkey */
  {"fire", chat_write},

  {"users", chat_users},
#endif

  {NULL, NULL}
};


static inline int
chat_cmd_match(buf, str)
  char *buf;
  char *str;
{
  int c1, c2;

  for (;;)
  {
    c1 = *str++;
    if (!c1)
      break;

    c2 = *buf++;
    if (!c2 || c2 == ' ' || c2 == '\n')
      break;

    if (c2 >= 'A' && c2 <= 'Z')
      c2 |= 0x20;

    if (c1 != c2)
      return 0;
  }

  return 1;
}


static inline int
chat_cmd(fd, buf)
  int fd;
  char *buf;
{
  struct chat_command *cmd;
  char *key;

  buf++;
  for (cmd = chat_cmdtbl; key = cmd->cmdname; cmd++)
  {
    if (chat_cmd_match(buf, key))
    {
      cmd->cmdfunc(buf);
      return '/';
    }
  }

  return 0;
}


extern char lastcmd[MAXLASTCMD][80];

#define CHAT_YPOS	10


int
t_chat()
{
  ACCT acct;
  int ch, cfd, cmdpos, cmdcol;
  char *ptr, buf[80], chatid[9];
  struct sockaddr_in sin;
#if     defined(__OpenBSD__)
  struct hostent *h;
#endif
  
#ifdef CHAT_SECURE
  extern char passbuf[];
#endif

#ifdef EVERY_Z
  /* Thor.980725: ¬° talk & chat ¥i¥Î ^z §@·Ç³Æ */
  if (vio_holdon())
  {
    vmsg("±zÁ¿¸ÜÁ¿¤@¥bÁÙ¨SÁ¿§¹­C");
    return -1;
  }
#endif

#if     defined(__OpenBSD__)

  if (!(h = gethostbyname(str_host)))
    return -1;

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(CHAT_PORT);
  memcpy(&sin.sin_addr, h->h_addr, h->h_length);
            
#else

  sin.sin_family = AF_INET;
  sin.sin_port = htons(CHAT_PORT);
  /* sin.sin_addr.s_addr = INADDR_LOOPBACK; */
  /* sin.sin_addr.s_addr = INADDR_ANY; */
  /* for FreeBSD 4.x */  
  sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  memset(sin.sin_zero, 0, sizeof(sin.sin_zero));

#endif

  cfd = socket(AF_INET, SOCK_STREAM, 0);
  if (cfd < 0)
    return -1;

  if (connect(cfd, (struct sockaddr *) & sin, sizeof sin))
  {
    close(cfd);
    blog("CHAT ", "connect");
    return -1;
  }

  for (;;)
  {
    ch = vget(b_lines, 0, "½Ð¿é¤J²á¤Ñ¥N¸¹¡G", chatid, 9, DOECHO);
    if (ch == '/')
    {
      continue;
    }
    else if (!ch)
    {
      /* str_ncpy(chatid, cuser.userid, sizeof(chatid)); */
      close(cfd);	/* itoc.010322: ¤j³¡¤À³£¬O»~«ö¨ì Talk->Chat §ï¦¨¹w³]¬°Â÷¶} */
      return 0;
    }
    else
    {
      /* itoc.010528: ¤£¥i¥H¥Î§O¤Hªº id °µ¬°²á¤Ñ¥N¸¹ */
      if (acct_load(&acct, chatid) >= 0 && acct.userno != cuser.userno)
      {
	vmsg("©êºp³o­Ó¥N¸¹¦³¤Hµù¥U¬° id¡A©Ò¥H±z¤£¯à·í¦¨²á¤Ñ¥N¸¹");
	continue;
      }
      /* Thor.980911: chatid¤¤¤£¥i¥HªÅ¥Õ, ¨¾¤î parse¿ù»~ */
      for(ch = 0; ch < 8; ch++)
      {
        if (chatid[ch] == ' ')
          break;
        else if (!chatid[ch]) /* Thor.980921: ¦pªG0ªº¸Ü´Nµ²§ô */
          ch = 8;
      }
      if (ch < 8) 
        continue;
    }

#ifdef CHAT_SECURE	/* Thor.980729: secured chat room */

#if 0
    §@ªÌ  opus (¤H¥Í¦³¨ý¬O²MÅw)                                ¯¸¤º sysopplan
    ¼ÐÃD  Re: Ãö©ó chatroom
    ®É¶¡  Wed Jul 30 03:14:56 1997
    ¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w¢w

    > passwd ¬O³sÄòªº«DªÅ¥Õ¦r¤¸¶Ü??

    ¤T­Ó°Ñ¼Æ userid + chatid + passwd ¤¤, userid/chatid ¬Ò¤£§t space, 
    ¦Ó passwd ¥i¥]§t space, ©Ò¥H¥²¶·±N¥¦Â\¦b²Ä¤T­Ó¦ì¸m¥H«K token-parsing¡C

    > ¥t¥~, ACCT ¤¤ªº passwd ¬OPASSLEN¦Û°Ê¸ÉªÅ¥ÕÁÙ¬O­n¤â°Ê¥[?
    > ·|¦Û°Ê¸Éº¡¶Ü?

    Unix ªº crypt ³Ì¦h¥u¨ú«e 14 ­Ó¦r, ©Ò¥H¤£¥[¥ç¥i¡C
    ³o­Ó¦a¤è °Ñ·Ó user login ªº¦a¤è¼g´N¦n¤F¡C

    --
    ¡° Origin: ·¬¾ôÅæ¯¸(bbs.cs.nthu.edu.tw) From: thcs-8.cs.nthu.edu.tw
#endif

    /* Thor.980819: ®³±¼userno */
    /* Thor.980730: passwd §ï¬°³Ì«áªº°Ñ¼Æ */
    /* Thor.980813: passwd §ï¬°¯u¥¿ªº password */
    /* Thor.980813: xchatd¤¤, chatid ¦Û°Ê¸õ¹LªÅ¥Õ, ©Ò¥H¦³ªÅ¥Õ·| invalid login */
#if 0
    sprintf(buf, "/! %d %s %s %s\n",
      cuser.userno, cuser.userid, chatid, cuser.passwd);
      cuser.userno, cuser.userid, chatid, passbuf);
#endif
    sprintf(buf, "/! %s %s %s\n",
      cuser.userid, chatid, passbuf);

#else
    sprintf(buf, "/! %d %d %s %s\n",
      cuser.userno, cuser.userlevel, cuser.userid, chatid);
#endif

    chat_send(cfd, buf);
    if (recv(cfd, buf, 3, 0) != 3)
      return 0;

    if (!strcmp(buf, CHAT_LOGIN_OK))
      break;
    else if (!strcmp(buf, CHAT_LOGIN_EXISTS))
      ptr = "³o­Ó¥N¸¹¤w¸g¦³¤H¥Î¤F";
    else if (!strcmp(buf, CHAT_LOGIN_INVALID))
      ptr = "³o­Ó¥N¸¹¬O¿ù»~ªº";
    else if (!strcmp(buf, CHAT_LOGIN_BOGUS))
    {				/* Thor: ¸T¤î¬Û¦P¤G¤H¶i¤J */
      close(cfd);
      vmsg("½Ð¤Å¬£»º¡u¤À¨­¡v¶i¤J½Í¤Ñ«Ç");
      return 0;
    }
    move(b_lines - 1, 0);
    outs(ptr);
    clrtoeol();
    bell();
  }

  clear();
  move(1, 0);
  outs(msg_seperator);
  move(stop_line, 0);
  outs(msg_seperator);
  print_chatid(chatid);
  memset(ptr = buf, 0, sizeof(buf));
  chatline = 2;
  cmdcol = 0;
  cmdpos = -1;

  add_io(cfd, 60);

  strcpy(cutmp->mateid, chatid);

  for (;;)
  {
    move(b_lines - 1, cmdcol + CHAT_YPOS);
    ch = vkey();

    if (ch == I_OTHERDATA)
    {				/* incoming */
      if (chat_recv(cfd, chatid) == -1)
	break;
      continue;
    }

    if (isprint2(ch))
    {
      if (cmdcol < 68)
      {
	if (ptr[cmdcol])
	{			/* insert */
	  int i;

	  for (i = cmdcol; ptr[i] && i < 68; i++);
	  ptr[i + 1] = '\0';
	  for (; i > cmdcol; i--)
	    ptr[i] = ptr[i - 1];
	}
	else
	{			/* append */
	  ptr[cmdcol + 1] = '\0';
	}
	ptr[cmdcol] = ch;
	move(b_lines - 1, cmdcol + CHAT_YPOS);
	outs(&ptr[cmdcol++]);
      }
      continue;
    }

    if (ch == '\n')
    {
#ifdef EVERY_BIFF
      /* Thor.980805: ¦³¤H¦b®ÇÃä«öenter¤~»Ý­ncheck biff */
      static int old_biff;
      int biff = HAS_STATUS(STATUS_BIFF);
      if (biff && !old_biff)
        printchatline("¡» ¾´! ¶l®t¨Ó«ö¹a¤F!");
      old_biff = biff;
#endif
      if (ch = *ptr)
      {
	if (ch == '/')
	  ch = chat_cmd(cfd, ptr);

        /* Thor.980602: ¦³­Ó­nª`·Nªº¤p¦a¤è, ­ì¥»¦pªG¬O¡y/¡z, 
                        ·|¨q¥X /helpªºµe­±,  
                        ²{¦b¥´ /, ·|ÅÜ¦¨ /p ¤Á´« pager */

        /* Thor.980925: «O¯d ptr ³Ì­ì©l¼Ë, ¤£¥[ /n */
	for (cmdpos = MAXLASTCMD - 1; cmdpos; cmdpos--)
	  strcpy(lastcmd[cmdpos], lastcmd[cmdpos - 1]);
	strcpy(lastcmd[0], ptr);

	if (ch != '/')
	{
	  strcat(ptr, "\n");
	  if (!chat_send(cfd, ptr))
	    break;
	}
	if (*ptr == '/' && ptr[1] == 'b')
	  break;

#if 0
	for (cmdpos = MAXLASTCMD - 1; cmdpos; cmdpos--)
	  strcpy(lastcmd[cmdpos], lastcmd[cmdpos - 1]);
	strcpy(lastcmd[0], ptr);
#endif

	*ptr = '\0';
	cmdcol = 0;
	cmdpos = -1;
	move(b_lines - 1, CHAT_YPOS);
	clrtoeol();
      }
      continue;
    }

    if (ch == KEY_BKSP)
    {
      if (cmdcol)
      {
	ch = cmdcol;
	cmdcol--;
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: §PÂ_²{¦b§R°£ªº¦ì¸m¬O§_¬°º~¦rªº«á¥b¬q¡A­Y¬O§R¤G¦r¤¸ */
	if ((cuser.ufo & UFO_ZHC) && cmdcol && IS_ZHC_LO(ptr, cmdcol))
	  cmdcol--;
#endif
	strcpy(ptr + cmdcol, ptr + ch);
	move(b_lines - 1, cmdcol + CHAT_YPOS);
	outs(ptr + cmdcol);
	clrtoeol();
      }
      continue;
    }

    if (ch == KEY_DEL)
    {
      if (ptr[cmdcol])
      {
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: §PÂ_²{¦b§R°£ªº¦ì¸m¬O§_¬°º~¦rªº«e¥b¬q¡A­Y¬O§R¤G¦r¤¸ */
	if ((cuser.ufo & UFO_ZHC) && ptr[cmdcol + 1] && IS_ZHC_HI(ptr[cmdcol]))
	  ch = 2;
	else
#endif
	  ch = 1;
	strcpy(ptr + cmdcol, ptr + cmdcol + ch);
	move(b_lines - 1, cmdcol + CHAT_YPOS);
	outs(ptr + cmdcol);
	clrtoeol();
      }
      continue;
    }

    if (ch == Ctrl('D'))
    {
      chat_send(cfd, "/b\n");	/* /bye Â÷¶} */
      break;
    }

    if (ch == Ctrl('C'))	/* itoc.µù¸Ñ: ²M°£ input ¾ã¦æ */
    {
      *ptr = '\0';
      cmdcol = 0;
      move(b_lines - 1, CHAT_YPOS);
      clrtoeol();
      continue;
    }

    if (ch == KEY_HOME || ch == Ctrl('A'))
    {
      cmdcol = 0;
      continue;
    }

    if (ch == KEY_END || ch == Ctrl('E'))
    {
      cmdcol = strlen(ptr);
      continue;
    }
    
    if (ch == KEY_LEFT)
    {
      if (cmdcol)
      {
	cmdcol--;
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: ¥ª²¾®É¸I¨ìº~¦r²¾Âù®æ */
	if ((cuser.ufo & UFO_ZHC) && cmdcol && IS_ZHC_LO(ptr, cmdcol))
	  cmdcol--;
#endif
      }
      continue;
    }

    if (ch == KEY_RIGHT)
    {
      if (ptr[cmdcol])
      {
	cmdcol++;
#ifdef HAVE_MULTI_BYTE
	/* hightman.060504: ¥k²¾®É¸I¨ìº~¦r²¾Âù®æ */
	if ((cuser.ufo & UFO_ZHC) && ptr[cmdcol] && IS_ZHC_HI(ptr[cmdcol - 1]))
	  cmdcol++;
#endif
      }
      continue;
    }

#ifdef EVERY_Z
    /* Thor: Chat ¤¤«ö ctrl-z */
    if (ch == Ctrl('Z'))
    {
      char buf[IDLEN + 1];
      screenline slt[T_LINES];

      /* Thor.980731: ¼È¦s mateid, ¦]¬°¥X¥h®É¥i¯à·|¥Î±¼ mateid */
      strcpy(buf, cutmp->mateid);

      vio_save();	/* Thor.980727: ¼È¦s vio_fd */
      vs_save(slt);
      every_Z(0);
      vs_restore(slt);
      vio_restore();	/* Thor.980727: ÁÙ­ì vio_fd */

      /* Thor.980731: ÁÙ­ì mateid, ¦]¬°¥X¥h®É¥i¯à·|¥Î±¼ mateid */
      strcpy(cutmp->mateid, buf);
      continue;
    }
#endif

    if (ch == KEY_DOWN)
    {
      cmdpos += MAXLASTCMD - 2;
      ch = KEY_UP;
    }

    if (ch == KEY_UP)
    {
      cmdpos++;
      cmdpos %= MAXLASTCMD;
      strcpy(ptr, lastcmd[cmdpos]);
      move(b_lines - 1, CHAT_YPOS);
      outs(ptr);
      clrtoeol();
      cmdcol = strlen(ptr);
    }
  }

  if (frec)
    chat_record();

  close(cfd);
  add_io(0, 60);
  cutmp->mateid[0] = '\0';
  return 0;
}
