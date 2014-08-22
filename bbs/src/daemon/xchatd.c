/*-------------------------------------------------------*/
/* xchatd.c     ( NTHU CS MapleBBS Ver 3.00 )            */
/*-------------------------------------------------------*/
/* target : super KTV daemon for chat server             */
/* create : 95/03/29                                     */
/* update : 97/10/20                                     */
/*-------------------------------------------------------*/


#include "bbs.h"
#include "xchat.h"


#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/resource.h>


#define	SERVER_USAGE
#define WATCH_DOG
#undef	DEBUG			/* 程式除錯之用 */
#undef	MONITOR			/* 監督 chatroom 活動以解決糾紛 */
#undef	STAND_ALONE		/* 不搭配 BBS 獨立執行 */


#ifdef	DEBUG
#define	MONITOR
#endif

static int gline;

#ifdef  WATCH_DOG
#define MYDOG  gline = __LINE__
#else
#define MYDOG			/* NOOP */
#endif


#define CHAT_PIDFILE    "run/chat.pid"
#define CHAT_LOGFILE    "run/chat.log"
#define	CHAT_INTERVAL	(60 * 30)
#define SOCK_QLEN	3


/* name of the main room (always exists) */


#define	MAIN_NAME	"main"
#define	MAIN_TOPIC	"有緣千里來相會"


#define ROOM_LOCKED	1
#define ROOM_SECRET	2
#define ROOM_OPENTOPIC  4
#define ROOM_ALL	(NULL)


#define LOCKED(room)	(room->rflag & ROOM_LOCKED)
#define SECRET(room)	(room->rflag & ROOM_SECRET)
#define OPENTOPIC(room) (room->rflag & ROOM_OPENTOPIC)


#define RESTRICTED(usr)	(usr->uflag == 0)	/* guest */
#define CHATSYSOP(usr)	(usr->uflag & PERM_ALLCHAT)
#define	PERM_ROOMOP	PERM_CHAT	/* Thor: 借 PERM_CHAT 為 PERM_ROOMOP */
#define	PERM_CHATOP	PERM_DENYCHAT	/* Thor: 借 PERM_DENYCHAT 為 PERM_CHATOP */
/* #define ROOMOP(usr)  (usr->uflag & (PERM_ROOMOP | PERM_ALLCHAT)) */
/* Thor.980603: PERM_ALLCHAT 改為 default 沒有 roomop, 但可以自己取得 chatop */
#define ROOMOP(usr)	(usr->uflag & (PERM_ROOMOP | PERM_CHATOP))
#define CLOAK(usr)	(usr->uflag & PERM_CLOAK)


/* ----------------------------------------------------- */
/* ChatRoom data structure                               */
/* ----------------------------------------------------- */


typedef struct ChatRoom ChatRoom;
typedef struct ChatUser ChatUser;
typedef struct UserList UserList;
typedef struct ChatCmd ChatCmd;
typedef struct ChatAction ChatAction;


struct ChatUser
{
  ChatUser *unext;
  ChatRoom *room;
  UserList *ignore;
  int sock;			/* user socket */
  int userno;
  int uflag;
  int clitype;			/* Xshadow: client type. 1 for common client,
				 * 0 for bbs only client */
  time_t tbegin;
  time_t uptime;
  int sno;
  int xdata;
  int retry;

  int isize;			/* current size of ibuf */
  char ibuf[80];		/* buffer for non-blocking receiving */
  char userid[IDLEN + 1];	/* real userid */
  char chatid[9];		/* chat id */
  char rhost[30];		/* host address */
};


struct ChatRoom
{
  ChatRoom *next, *prev;
  UserList *invite;
  char name[IDLEN + 1];
  char topic[48];		/* Let the room op to define room topic */
  int rflag;			/* ROOM_LOCKED, ROOM_SECRET, ROOM_OPENTOPIC */
  int occupants;		/* number of users in room */
};


struct UserList
{
  UserList *next;
  int userno;
  char userid[0];
};


struct ChatCmd
{
  char *cmdstr;
  void (*cmdfunc) ();
  int exact;
};


static ChatRoom mainroom, *roompool;
static ChatUser *mainuser, *userpool;
static fd_set mainfset;
static int totaluser;		/* current number of connections */
static struct timeval zerotv;	/* timeval for selecting */
static int common_client_command;


#ifdef STAND_ALONE
static int userno_inc = 0;	/* userno auto-incrementer */
#endif


static char msg_not_op[] = "◆ 您不是這間聊天室的 Op";
static char msg_no_such_id[] = "◆ 目前沒有人使用 [%s] 這個聊天代號";
static char msg_not_here[] = "◆ [%s] 不在這間聊天室";


#define	FUZZY_USER	((ChatUser *) -1)


/* ----------------------------------------------------- */
/* operation log and debug information                   */
/* ----------------------------------------------------- */


static FILE *flog;


static void
logit(key, msg)
  char *key;
  char *msg;
{
  time_t now;
  struct tm *p;

  time(&now);
  p = localtime(&now);
  fprintf(flog, "%02d/%02d %02d:%02d:%02d %-13s%s\n",
    p->tm_mon + 1, p->tm_mday,
    p->tm_hour, p->tm_min, p->tm_sec, key, msg);
}


static inline void
log_init()
{
  FILE *fp;

  /* --------------------------------------------------- */
  /* log daemon's PID					 */
  /* --------------------------------------------------- */

  if (fp = fopen(CHAT_PIDFILE, "w"))
  {
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
  }

  flog = fopen(CHAT_LOGFILE, "a");
  logit("START", "chat daemon");
}


#ifdef	DEBUG
static char chatbuf[256];	/* general purpose buffer */


static void
debug_list(list)
  UserList *list;
{
  char buf[80];
  int i = 0;

  if (!list)
  {
    logit("DEBUG_L", "NULL");
    return;
  }
  while (list)
  {
    sprintf(buf, "%d) list: %p userno: %d next: %p", i++, list, list->userno, list->next);
    logit("DEBUG_L", buf);

    list = list->next;
  }
  logit("DEBUG_L", "end");
}


static void
debug_user()
{
  ChatUser *user;
  int i;
  char buf[80];

  sprintf(buf, "mainuser: %p userpool: %p", mainuser, userpool);
  logit("DEBUG_U", buf);
  for (i = 0, user = mainuser; user; user = user->unext)
  {
    /* MYDOG; */
    sprintf(buf, "%d) %p %-6d %s %s", ++i, user, user->userno, user->userid, user->chatid);
    logit("DEBUG_U", buf);
  }
}


static void
debug_room()
{
  ChatRoom *room;
  int i;
  char buf[80];

  i = 0;
  room = &mainroom;

  sprintf(buf, "mainroom: %p roompool: %p", mainroom, roompool);
  logit("DEBUG_R", buf);
  do
  {
    MYDOG;
    sprintf(buf, "%d) %p %s %d", ++i, room, room->name, room->occupants);
    logit("DEBUG_R", buf);
  } while (room = room->next);
}


static void
log_user(cu)
  ChatUser *cu;
{
  static int log_num;

  if (cu)
  {
    if (log_num > 100 && log_num < 150)
    {
      sprintf(chatbuf, "%d: %p <%d>", log_num, cu, gline);
      logit("travese user ", chatbuf);
    }
    else if (log_num == 100)
    {
      sprintf(chatbuf, "BOOM !! at line %d", gline);
      logit("travese user ", chatbuf);
    }
    log_num++;
  }
  else
    log_num = 0;
}
#endif				/* DEBUG */


/* ----------------------------------------------------- */
/* string routines                                       */
/* ----------------------------------------------------- */


static int
valid_chatid(id)
  char *id;
{
  int ch, len;

  for (len = 0; ch = *id; id++)
  { /* Thor.980921: 空白為不合理chatid, 怕getnext判斷錯誤等等 */
    if (ch == '/' || ch == '*' || ch == ':' || ch ==' ')
      return 0;
    if (++len > 8)
      return 0;
  }
  return len;
}


/* itoc.註解: 由於改採 MUD-like 的部分 match 即可 */
/* 所以 MUD-like 的 action 盡量不要用英文縮寫，並不要有重覆的 */

static int		/* 0: fit */
str_belong(s1, s2)	/* itoc.010321: 讓 mud-like 指令部分 match 即可，和 mud 一樣 */
  uschar *s1;		/* ChatAction 裡的小寫 verb */
  uschar *s2;		/* user input command 大小寫均可 */
{
  int c1, c2;
  int num = 0;

  for (;;)
  {
    c1 = *s1;
    c2 = *s2;

    if (c2 >= 'A' && c2 <= 'Z')
      c2 |= 0x20;	/* 換小寫 */

    if (num >= 2)	/* 至少要有二字元相同 */
    {
      if (!c1 || !c2)	/* 完全 match 或部分 match 皆可 (s1包含s2 或 s2包含s1均算) */
        return 0;
    }

    if (c1 > c2)	/* itoc.010927: 不同的回傳值 */
      return 1;
    else if (c1 < c2)
      return -1;

    s1++;
    s2++;
    num++;
  }
}


/* ----------------------------------------------------- */
/* match strings' similarity case-insensitively          */
/* ----------------------------------------------------- */
/* str_match(keyword, string)				 */
/* ----------------------------------------------------- */
/* 0 : equal            ("foo", "foo")                   */
/* -1 : mismatch        ("abc", "xyz")                   */
/* ow : similar         ("goo", "good")                  */
/* ----------------------------------------------------- */


static int
str_match(s1, s2)
  uschar *s1;		/* lower-case (sub)string */
  uschar *s2;
{
  int c1, c2;

  for (;;)
  {
    c1 = *s1;
    c2 = *s2;

    if (!c1)
      return c2;

    if (c2 >= 'A' && c2 <= 'Z')
      c2 |= 0x20;	/* 換小寫 */

    if (c1 != c2)
      return -1;

    s1++;
    s2++;
  }
}


/* ----------------------------------------------------- */
/* search user/room by its ID                            */
/* ----------------------------------------------------- */


static ChatUser *
cuser_by_userid(userid)
  char *userid;
{
  ChatUser *cu;
  char buf[80]; /* Thor.980727: 一次最長才80 */

  str_lower(buf, userid);
  for (cu = mainuser; cu; cu = cu->unext)
  {
    if (!cu->userno)
      continue;
    if (!str_cmp(buf, cu->userid))
      break;
  }
  return cu;
}


static ChatUser *
cuser_by_chatid(chatid)
  char *chatid;
{
  ChatUser *cu;
  char buf[80]; /* Thor.980727: 一次最長才80 */

  str_lower(buf, chatid);

  for (cu = mainuser; cu; cu = cu->unext)
  {
    if (!cu->userno)
      continue;
    if (!str_cmp(buf, cu->chatid))
      break;
  }
  return cu;
}


static ChatUser *
fuzzy_cuser_by_chatid(chatid)
  char *chatid;
{
  ChatUser *cu, *xuser;
  int mode;
  char buf[80]; /* Thor.980727: 一次最長才80 */

  str_lower(buf, chatid);
  xuser = NULL;

  for (cu = mainuser; cu; cu = cu->unext)
  {
    if (!cu->userno)
      continue;

    mode = str_match(buf, cu->chatid);
    if (mode == 0)
      return cu;

    if (mode > 0)
    {
      if (xuser)
	return FUZZY_USER;	/* 符合者大於 2 人 */

      xuser = cu;
    }
  }
  return xuser;
}


static ChatRoom *
croom_by_roomid(roomid)
  char *roomid;
{
  ChatRoom *room;
  char buf[80]; /* Thor.980727: 一次最長才80 */

  str_lower(buf, roomid);
  room = &mainroom;
  do
  {
    if (!str_cmp(buf, room->name))
      break;
  } while (room = room->next);
  return room;
}


/* ----------------------------------------------------- */
/* UserList routines                                     */
/* ----------------------------------------------------- */


static void
list_free(list)
  UserList **list;
{
  UserList *user, *next;

  for (user = *list, *list = NULL; user; user = next)
  {
    next = user->next;
    free(user);
  }
}


static void
list_add(list, user)
  UserList **list;
  ChatUser *user;
{
  UserList *node;
  char *userid;
  int len;

  len = strlen(userid = user->userid) + 1;
  if (node = (UserList *) malloc(sizeof(UserList) + len))
  {
    node->next = *list;
    node->userno = user->userno;
    memcpy(node->userid, userid, len);
    *list = node;
  }
}


static int
list_delete(list, userid)
  UserList **list;
  char *userid;
{
  UserList *node;
  char buf[80]; /* Thor.980727: 輸入一次最長才 80 */

  str_lower(buf, userid);

  while (node = *list)
  {
    if (!str_cmp(buf, node->userid))
    {
      *list = node->next;
      free(node);
      return 1;
    }
    list = &node->next;
  }

  return 0;
}


static int
list_belong(list, userno)
  UserList *list;
  int userno;
{
  while (list)
  {
    if (userno == list->userno)
      return 1;
    list = list->next;
  }
  return 0;
}


/* ------------------------------------------------------ */
/* non-blocking socket routines : send message to users   */
/* ------------------------------------------------------ */


static void
do_send(nfds, wset, msg)
  int nfds;
  fd_set *wset;
  char *msg;
{
  int len, sr;

#if 1
  /* Thor: for future reservation bug */
  zerotv.tv_sec = 0;
  zerotv.tv_usec = 0;
#endif

  sr = select(nfds + 1, NULL, wset, NULL, &zerotv);

  if (sr > 0)
  {
    len = strlen(msg) + 1;
    do
    {
      if (FD_ISSET(nfds, wset))
      {
	send(nfds, msg, len, 0);
	if (--sr <= 0)
	  return;
      }
    } while (--nfds > 0);
  }
}


static void
send_to_room(room, msg, userno, number)
  ChatRoom *room;
  char *msg;
  int userno;
  int number;
{
  ChatUser *cu;
  fd_set wset;
  int sock, max;
  int clitype;			/* 分為 bbs client 及 common client 兩次處理 */
  char *str, buf[256];

  for (clitype = (number == MSG_MESSAGE || !number) ? 0 : 1;
    clitype < 2; clitype++)
  {
    FD_ZERO(&wset);
    max = -1;

    for (cu = mainuser; cu; cu = cu->unext)
    {
      if (cu->userno && (cu->clitype == clitype) &&
	(room == ROOM_ALL || room == cu->room) &&
	(!userno || !list_belong(cu->ignore, userno)))
      {
	sock = cu->sock;

	FD_SET(sock, &wset);

	if (max < sock)
	  max = sock;
      }
    }

    if (max <= 0)
      continue;

    if (clitype)
    {
      str = buf;

      if (*msg)
	sprintf(str, "%3d %s", number, msg);
      else
	sprintf(str, "%3d", number);
    }
    else
    {
      str = msg;
    }

    do_send(max, &wset, str);
  }
}


static void
send_to_user(user, msg, userno, number)
  ChatUser *user;
  char *msg;
  int userno;
  int number;
{
  int sock;

#if 0
  if (!user->userno || (!user->clitype && number && number != MSG_MESSAGE))
#endif
  /* Thor.980911: 如果查user->userno則在login_user的error message會無法送回 */
  if (!user->clitype && number != MSG_MESSAGE)
    return;

  if ((sock = user->sock) <= 0)
    return;

  if (!userno || !list_belong(user->ignore, userno))
  {
    fd_set wset;
    char buf[256];

    FD_ZERO(&wset);
    FD_SET(sock, &wset);

    if (user->clitype)
    {
      if (*msg)
	sprintf(buf, "%3d %s", number, msg);
      else
	sprintf(buf, "%3d", number);
      msg = buf;
    }

    do_send(sock, &wset, msg);
  }
}


/* ----------------------------------------------------- */


static void
room_changed(room)
  ChatRoom *room;
{
  if (room)
  {
    char buf[256];

    sprintf(buf, "= %s %d %d %s",
      room->name, room->occupants, room->rflag, room->topic);
    send_to_room(ROOM_ALL, buf, 0, MSG_ROOMNOTIFY);
  }
}


static void
user_changed(cu)
  ChatUser *cu;
{
  if (cu)
  {
    ChatRoom *room;
    char buf[256];

    room = cu->room;
    sprintf(buf, "= %s %s %s %s%s",
      cu->userid, cu->chatid, room->name, cu->rhost,
      ROOMOP(cu) ? " Op" : "");
    send_to_room(room, buf, 0, MSG_USERNOTIFY);
  }
}


static void
exit_room(user, mode, msg)
  ChatUser *user;
  int mode;
  char *msg;
{
  ChatRoom *room;
  char buf[128];

  if (!(room = user->room))
    return;

  user->room = NULL;
  /* user->uflag &= ~(PERM_ROOMOP | PERM_ALLCHAT); */
  user->uflag &= ~PERM_ROOMOP;
  /* Thor.980601: 離開房間時只清 room op, 不清 sysop, chatroom 因天生具有 */

  if (--room->occupants > 0)
  {
    char *chatid;

    chatid = user->chatid;
    switch (mode)
    {
    case EXIT_LOGOUT:

      sprintf(buf, "◆ %s 離開了 ... %.50s", chatid, (msg && *msg) ? msg : "");
      break;

    case EXIT_LOSTCONN:

      sprintf(buf, "◆ %s 成了斷線的風箏囉", chatid);
      break;

    case EXIT_KICK:

      sprintf(buf, "◆ 哈哈！%s 被踢出去了", chatid);
      break;
    }

    if (!CLOAK(user))
      send_to_room(room, buf, 0, MSG_MESSAGE);

    sprintf(buf, "- %s", user->userid);
    send_to_room(room, buf, 0, MSG_USERNOTIFY);
    room_changed(room);
  }
  else if (room != &mainroom)
  {
    ChatRoom *next;

    fprintf(flog, "room-\t[%d] %s\n", user->sno, room->name);
    sprintf(buf, "- %s", room->name);

    room->prev->next = next = room->next;
    if (next)
      next->prev = room->prev;

    list_free(&room->invite);

    /* free(room); */

    /* 回收 */
    room->next = roompool;
    roompool = room;

    send_to_room(ROOM_ALL, buf, 0, MSG_ROOMNOTIFY);
  }
}


/* ----------------------------------------------------- */
/* chat commands                                         */
/* ----------------------------------------------------- */


#ifndef STAND_ALONE
/* ----------------------------------------------------- */
/* BBS server side routines                              */
/* ----------------------------------------------------- */


/* static */
int
acct_load(acct, userid)
  ACCT *acct;
  char *userid;
{
  int fd;

  usr_fpath((char *) acct, userid, FN_ACCT);
  fd = open((char *) acct, O_RDONLY);
  if (fd >= 0)
  {
    read(fd, acct, sizeof(ACCT));
    close(fd);
  }
  return fd;
}


static void
chat_query(cu, msg)
  ChatUser *cu;
  char *msg;
{
  FILE *fp;
  ACCT acct;
  char buf[256];

  /* Thor.980617: 可先查是否為空字串 */
  if (*msg && acct_load(&acct, msg) >= 0)
  {
    sprintf(buf, "%s(%s) 共上站 %d 次，文章 %d 篇",
      acct.userid, acct.username, acct.numlogins, acct.numposts);
    send_to_user(cu, buf, 0, MSG_MESSAGE);

    sprintf(buf, "最近(%s)從(%s)上站", Btime(&acct.lastlogin),
      (acct.lasthost[0] ? acct.lasthost : "外太空"));
    send_to_user(cu, buf, 0, MSG_MESSAGE);

    usr_fpath(buf, acct.userid, FN_PLANS);
    if (fp = fopen(buf, "r"))
    {
      int i;

      i = 0;
      while (fgets(buf, sizeof(buf), fp))
      {
	buf[strlen(buf) - 1] = 0;
	send_to_user(cu, buf, 0, MSG_MESSAGE);
	if (++i >= MAXQUERYLINES)
	  break;
      }
      fclose(fp);
    }
  }
  else
  {
    sprintf(buf, msg_no_such_id, msg);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
  }
}
#endif


static void
chat_clear(cu, msg)
  ChatUser *cu;
  char *msg;
{
  if (cu->clitype)
    send_to_user(cu, "", 0, MSG_CLRSCR);
  else
    send_to_user(cu, "/c", 0, MSG_MESSAGE);
}


static void
chat_date(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char buf[128];

  sprintf(buf, "◆ 標準時間: %s", Now());
  send_to_user(cu, buf, 0, MSG_MESSAGE);
}


static void
chat_topic(cu, msg)
  ChatUser *cu;
  char *msg;
{
  ChatRoom *room;
  char *topic, buf[128];

  room = cu->room;

  if (!ROOMOP(cu) && !OPENTOPIC(room))
  {
    send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
    return;
  }

  if (*msg == '\0')
  {
    send_to_user(cu, "※ 請指定話題", 0, MSG_MESSAGE);
    return;
  }

  topic = room->topic;
  str_ncpy(topic, msg, sizeof(room->topic));

  if (cu->clitype)
  {
    send_to_room(room, topic, 0, MSG_TOPIC);
  }
  else
  {
    sprintf(buf, "/t%s", topic);
    send_to_room(room, buf, 0, MSG_MESSAGE);
  }

  room_changed(room);

  if (!CLOAK(cu))
  {
    sprintf(buf, "◆ %s 將話題改為 \033[1;32m%s\033[m", cu->chatid, topic);
    send_to_room(room, buf, 0, MSG_MESSAGE);
  }
}


static void
chat_version(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char buf[80];

  sprintf(buf, "[Version] MapleBBS-3.10-20040726-PACK.itoc + Xchat-%d.%d", 
    XCHAT_VERSION_MAJOR, XCHAT_VERSION_MINOR);
  send_to_user(cu, buf, 0, MSG_MESSAGE);
}


static void
chat_nick(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char *chatid, *str, buf[128];
  ChatUser *xuser;

  chatid = nextword(&msg);
  chatid[8] = '\0';
  if (!valid_chatid(chatid))
  {
    send_to_user(cu, "※ 這個聊天代號是不正確的", 0, MSG_MESSAGE);
    return;
  }

  xuser = cuser_by_chatid(chatid);
  if (xuser != NULL && xuser != cu)
  {
    send_to_user(cu, "※ 已經有人捷足先登囉", 0, MSG_MESSAGE);
    return;
  }

  /* itoc.010528: 不可以用別人的 id 做為聊天代號 */
  usr_fpath(buf, chatid, NULL);
  if (dashd(buf) && str_cmp(chatid, cu->userid))
  {
    send_to_user(cu, "※ 抱歉這個代號有人註冊為 id，所以您不能當成聊天代號", 0, MSG_MESSAGE);
    return;
  }

  str = cu->chatid;

  if (!CLOAK(cu))
  {
    sprintf(buf, "※ %s 將聊天代號改為 \033[1;33m%s\033[m", str, chatid);
    send_to_room(cu->room, buf, cu->userno, MSG_MESSAGE);
  }

  strcpy(str, chatid);

  user_changed(cu);

  if (cu->clitype)
  {
    send_to_user(cu, chatid, 0, MSG_NICK);
  }
  else
  {
    sprintf(buf, "/n%s", chatid);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
  }
}


static void
chat_list_rooms(cuser, msg)
  ChatUser *cuser;
  char *msg;
{
  ChatRoom *cr, *room;
  char buf[128];
  int mode;

  if (RESTRICTED(cuser))
  {
    send_to_user(cuser, "※ 您沒有權限列出現有的聊天室", 0, MSG_MESSAGE);
    return;
  }

  mode = common_client_command;

  if (mode)
    send_to_user(cuser, "", 0, MSG_ROOMLISTSTART);
  else
    send_to_user(cuser, "\033[7m 談天室名稱  │人數│話題        \033[m", 0,
      MSG_MESSAGE);

  room = cuser->room;
  cr = &mainroom;

  do
  {
    if ((cr == room) || !SECRET(cr) || CHATSYSOP(cuser))
    {
      if (mode)
      {
	sprintf(buf, "%s %d %d %s",
	  cr->name, cr->occupants, cr->rflag, cr->topic);
	send_to_user(cuser, buf, 0, MSG_ROOMLIST);
      }
      else
      {
	sprintf(buf, " %-12s│%4d│%s", cr->name, cr->occupants, cr->topic);
	if (LOCKED(cr))
	  strcat(buf, " [鎖住]");
	if (SECRET(cr))
	  strcat(buf, " [秘密]");
	if (OPENTOPIC(cr))
	  strcat(buf, " [話題]");
	send_to_user(cuser, buf, 0, MSG_MESSAGE);
      }
    }
  } while (cr = cr->next);

  if (mode)
    send_to_user(cuser, "", 0, MSG_ROOMLISTEND);
}


static void
chat_do_user_list(cu, msg, theroom)
  ChatUser *cu;
  char *msg;
  ChatRoom *theroom;
{
  ChatRoom *myroom, *room;
  ChatUser *user;
  int start, stop, curr, mode; /* , uflag; */
  char buf[128];

  curr = 0; /* Thor.980619: initialize curr */
  start = atoi(nextword(&msg));
  stop = atoi(nextword(&msg));

  mode = common_client_command;

  if (mode)
    send_to_user(cu, "", 0, MSG_USERLISTSTART);
  else
    send_to_user(cu, "\033[7m 聊天代號│使用者代號  │聊天室 \033[m", 0,
      MSG_MESSAGE);

  myroom = cu->room;

  /* Thor.980717: 需要先排除 cu->userno == 0 的狀況嗎? */
  for (user = mainuser; user; user = user->unext)
  {
#if 0	/* Thor.980717: 既然 cu 都空了那還進來幹麼? */
    if (!cu->userno)
      continue;
#endif

    if (!user->userno) 
      continue;

    room = user->room;
    if ((theroom != ROOM_ALL) && (theroom != room))
      continue;

    /* Thor.980717: viewer check */
    if ((myroom != room) && (RESTRICTED(cu) || (room && SECRET(room) && !CHATSYSOP(cu))))
      continue;

    /* Thor.980717: viewee check */
    if (CLOAK(user) && (user != cu) && !CHATSYSOP(cu))
      continue;

    curr++;
    if (start && curr < start)
      continue;
    else if (stop && (curr > stop))
      break;

    if (mode)
    {
      if (!room)
	continue;		/* Xshadow: 還沒進入任何房間的就不列出 */

      sprintf(buf, "%s %s %s %s",
	user->chatid, user->userid, room->name, user->rhost);

      /* Thor.980603: PERM_ALLCHAT 改為 default 沒有 roomop, 但可以自己取得 */
      /* if (uflag & (PERM_ROOMOP | PERM_ALLCHAT)) */
      if (ROOMOP(user))
	strcat(buf, " Op");
    }
    else
    {
      sprintf(buf, " %-8s│%-12s│%s",
	user->chatid, user->userid, room ? room->name : "[在門口徘徊]");
      /* Thor.980603: PERM_ALLCHAT 改為 default 沒有 roomop, 但可以自己取得 */
      /* if (uflag & (PERM_ROOMOP | PERM_ALLCHAT)) */
      /* if (uflag & (PERM_ROOMOP | PERM_CHATOP)) */
      if (ROOMOP(user))  /* Thor.980602: 統一用法 */
	strcat(buf, " [Op]");
    }

    send_to_user(cu, buf, 0, mode ? MSG_USERLIST : MSG_MESSAGE);
  }

  if (mode)
    send_to_user(cu, "", 0, MSG_USERLISTEND);
}


static void
chat_list_by_room(cu, msg)
  ChatUser *cu;
  char *msg;
{
  ChatRoom *whichroom;
  char *roomstr, buf[128];

  roomstr = nextword(&msg);
  if (!*roomstr)
  {
    whichroom = cu->room;
  }
  else
  {
    if (!(whichroom = croom_by_roomid(roomstr)))
    {
      sprintf(buf, "※ 沒有 [%s] 這個聊天室", roomstr);
      send_to_user(cu, buf, 0, MSG_MESSAGE);
      return;
    }

    if (whichroom != cu->room && SECRET(whichroom) && !CHATSYSOP(cu))
    {
      send_to_user(cu, "※ 無法列出在秘密聊天室的使用者", 0, MSG_MESSAGE);
      return;
    }
  }
  chat_do_user_list(cu, msg, whichroom);
}


static void
chat_list_users(cu, msg)
  ChatUser *cu;
  char *msg;
{
  chat_do_user_list(cu, msg, ROOM_ALL);
}


static void
chat_chatroom(cu, msg)
  ChatUser *cu;
  char *msg;
{
  if (common_client_command)
    send_to_user(cu, "聊天室", 0, MSG_CHATROOM);
}


static void
chat_map_chatids(cu, whichroom)
  ChatUser *cu;			/* Thor: 還沒有作不同間的 */
  ChatRoom *whichroom;
{
  int c;
  ChatRoom *myroom, *room;
  ChatUser *user;
  char buf[128];

  myroom = cu->room;

  send_to_user(cu,
    "\033[7m 聊天代號 使用者代號  │ 聊天代號 使用者代號  │ 聊天代號 使用者代號 \033[m", 0, MSG_MESSAGE);

  for (c = 0, user = mainuser; user; user = user->unext)
  {
    if (!cu->userno)
      continue;

    room = user->room;
    if (whichroom != ROOM_ALL && whichroom != room)
      continue;

    if (myroom != room)
    {
      if (RESTRICTED(cu) ||	/* Thor: 要先check room 是不是空的 */
	(room && SECRET(room) && !CHATSYSOP(cu)))
	continue;
    }

    if (CLOAK(user) && (user != cu) && !CHATSYSOP(cu))	/* Thor:隱身術 */
      continue;

    sprintf(buf + (c * 24), " %-8s%c%-12s%s",
      user->chatid, ROOMOP(user) ? '*' : ' ',
      user->userid, (c < 2 ? "│" : "  "));

    if (++c == 3)
    {
      send_to_user(cu, buf, 0, MSG_MESSAGE);
      c = 0;
    }
  }

  if (c > 0)
    send_to_user(cu, buf, 0, MSG_MESSAGE);
}


static void
chat_map_chatids_thisroom(cu, msg)
  ChatUser *cu;
  char *msg;
{
  chat_map_chatids(cu, cu->room);
}


static void
chat_setroom(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char *modestr;
  ChatRoom *room;
  char *chatid;
  int sign, flag;
  char *fstr, buf[128];

  if (!ROOMOP(cu))
  {
    send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
    return;
  }

  modestr = nextword(&msg);
  sign = 1;
  if (*modestr == '+')
  {
    modestr++;
  }
  else if (*modestr == '-')
  {
    modestr++;
    sign = 0;
  }

  if (*modestr == '\0')
  {
    send_to_user(cu,
      "※ 請指定狀態: {[+(設定)][-(取消)]}{[L(鎖住)][s(秘密)][t(開放話題)}", 0, MSG_MESSAGE);
    return;
  }

  room = cu->room;
  chatid = cu->chatid;

  while (*modestr)
  {
    flag = 0;
    switch (*modestr)
    {
    case 'l':
    case 'L':
      flag = ROOM_LOCKED;
      fstr = "鎖住";
      break;

    case 's':
    case 'S':
      flag = ROOM_SECRET;
      fstr = "秘密";
      break;

    case 't':
    case 'T':
      flag = ROOM_OPENTOPIC;
      fstr = "開放話題";
      break;

    default:
      sprintf(buf, "※ 狀態錯誤：[%c]", *modestr);
      send_to_user(cu, buf, 0, MSG_MESSAGE);
    }

    /* Thor: check room 是不是空的, 應該不是空的 */

    if (flag && (room->rflag & flag) != sign * flag)
    {
      room->rflag ^= flag;

      if (!CLOAK(cu))
      {
	sprintf(buf, "※ 本聊天室被 %s %s [%s] 狀態",
	  chatid, sign ? "設定為" : "取消", fstr);
	send_to_room(room, buf, 0, MSG_MESSAGE);
      }
    }
    modestr++;
  }

  /* Thor.980602: 不准 Main room 鎖起 or 秘密，否則離開的就進不來，要看也看不到。
     想要踢人也踢不進 main room，不會很奇怪嗎？ */

  if (!str_cmp(MAIN_NAME, room->name))
  {
    if (room->rflag & (ROOM_LOCKED | ROOM_SECRET))
    {
      send_to_room(room, "※ 但天使施了『復原』的魔法", 0, MSG_MESSAGE);
      room->rflag &= ~(ROOM_LOCKED | ROOM_SECRET);
    }
  }

  room_changed(room);
}


static char *chat_msg[] =
{
  "[//]help", "MUD-like 社交動詞",
  "[/h]elp op", "談天室管理員專用指令",
  "[/a]ct <msg>", "做一個動作",
  "[/b]ye [msg]", "道別",
  "[/c]lear  [/d]ate", "清除螢幕  目前時間",
  "[/i]gnore [user]", "忽略使用者",
  "[/j]oin <room>", "建立或加入談天室",
  "[/l]ist [start [stop]]", "列出談天室使用者",
  "[/m]sg <id|user> <msg>", "跟 <id> 說悄悄話",
  "[/n]ick <id>", "將談天代號換成 <id>",
  "[/p]ager", "切換呼叫器",
  "[/q]uery <user>", "查詢網友",
  "[/qui]t [msg]", "道別",  
  "[/r]oom", "列出一般談天室",
  "[/t]ape", "開關錄音機",
  "[/u]nignore <user>", "取消忽略",
  "[/w]ho", "列出本談天室使用者",
  "[/w]hoin <room>", "列出談天室<room> 的使用者",
  NULL
};


static char *room_msg[] =
{
  "[/f]lag [+-][lst]", "設定鎖定、秘密、開放話題",
  "[/i]nvite <id>", "邀請 <id> 加入談天室",
  "[/kick] <id>", "將 <id> 踢出談天室",
  "[/o]p [<id>]", "將 Op 的權力轉移給 <id>",
  "[/topic] <text>", "換個話題",
  "[/w]all", "廣播 (站長專用)",
  NULL
};


static void
chat_help(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char **table, *str, buf[128];

  if (!str_cmp("op", nextword(&msg)))
  {
    send_to_user(cu, "談天室管理員專用指令", 0, MSG_MESSAGE);
    table = room_msg;
  }
  else
  {
    table = chat_msg;
  }

  while (str = *table++)
  {
    sprintf(buf, "  %-20s- %s", str, *table++);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
  }
}


static void
chat_private(cu, msg)
  ChatUser *cu;
  char *msg;
{
  ChatUser *xuser;
  char *recipient, buf[128];

  recipient = nextword(&msg);
  xuser = (ChatUser *) fuzzy_cuser_by_chatid(recipient);
  if (xuser == NULL)		/* Thor.980724: 用 userid也可傳悄悄話 */
  {
    xuser = cuser_by_userid(recipient);
  }

  if (xuser == NULL)
  {
    sprintf(buf, msg_no_such_id, recipient);
  }
  else if (xuser == FUZZY_USER)
  {				/* ambiguous */
    strcpy(buf, "※ 請指明聊天代號");
  }
  else if (*msg)
  {
    int userno;

    userno = cu->userno;

    sprintf(buf, "\033[1m*%s*\033[m %.50s", cu->chatid, msg);
    send_to_user(xuser, buf, userno, MSG_MESSAGE);

    if (xuser->clitype)		/* Xshadow: 如果對方是用 client 上來的 */
    {
      sprintf(buf, "%s %s %.50s", cu->userid, cu->chatid, msg);
      send_to_user(xuser, buf, userno, MSG_PRIVMSG);
    }

    if (cu->clitype)
    {
      sprintf(buf, "%s %s %.50s", xuser->userid, xuser->chatid, msg);
      send_to_user(cu, buf, 0, MSG_MYPRIVMSG);
    }

    sprintf(buf, "%s> %.50s", xuser->chatid, msg);
  }
  else
  {
    sprintf(buf, "※ 您想對 %s 說什麼話呢？", xuser->chatid);
  }

  send_to_user(cu, buf, 0, MSG_MESSAGE);
}


static void
chat_cloak(cu, msg)
  ChatUser *cu;
  char *msg;
{
  if (CHATSYSOP(cu))
  {
    char buf[128];

    cu->uflag ^= PERM_CLOAK;
    sprintf(buf, "◆ %s", CLOAK(cu) ? MSG_CLOAKED : MSG_UNCLOAK);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
  }
}


/* ----------------------------------------------------- */


static void
arrive_room(cuser, room)
  ChatUser *cuser;
  ChatRoom *room;
{
  char *rname, buf[256];

  /* Xshadow: 不必送給自己, 反正換房間就會重新 build user list */

  sprintf(buf, "+ %s %s %s %s",
    cuser->userid, cuser->chatid, room->name, cuser->rhost);
  if (ROOMOP(cuser))
    strcat(buf, " Op");
  send_to_room(room, buf, 0, MSG_USERNOTIFY);

  room->occupants++;
  room_changed(room);

  cuser->room = room;
  rname = room->name;

  if (cuser->clitype)
  {
    send_to_user(cuser, rname, 0, MSG_ROOM);
    send_to_user(cuser, room->topic, 0, MSG_TOPIC);
  }
  else
  {
    sprintf(buf, "/r%s", rname);
    send_to_user(cuser, buf, 0, MSG_MESSAGE);
    sprintf(buf, "/t%s", room->topic);
    send_to_user(cuser, buf, 0, MSG_MESSAGE);
  }

  sprintf(buf, "※ \033[32;1m%s\033[m 進入 \033[33;1m[%s]\033[m 包廂",
    cuser->chatid, rname);

  if (!CLOAK(cuser))
    send_to_room(room, buf, cuser->userno, MSG_MESSAGE);
  else
    send_to_user(cuser, buf, 0, MSG_MESSAGE);
}


static int
enter_room(cuser, rname, msg)
  ChatUser *cuser;
  char *rname;
  char *msg;
{
  ChatRoom *room;
  int create;
  char buf[256];

  create = 0;
  room = croom_by_roomid(rname);

  if (room == NULL)
  {
    /* new room */

#ifdef DEBUG
    logit(cuser->userid, "create new room");
    debug_room();
#endif

    if (room = roompool)
    {
      roompool = room->next;
    }
    else
    {
      room = (ChatRoom *) malloc(sizeof(ChatRoom));
    }

    if (room == NULL)
    {
      send_to_user(cuser, "※ 無法再新闢包廂了", 0, MSG_MESSAGE);
      return 0;
    }

    memset(room, 0, sizeof(ChatRoom));
    str_ncpy(room->name, rname, sizeof(room->name));
    strcpy(room->topic, "這是一個新天地");

    sprintf(buf, "+ %s 1 0 %s", room->name, room->topic);
    send_to_room(ROOM_ALL, buf, 0, MSG_ROOMNOTIFY);

    if (mainroom.next)
      mainroom.next->prev = room;
    room->next = mainroom.next;

    mainroom.next = room;
    room->prev = &mainroom;

#ifdef DEBUG
    logit(cuser->userid, "create room succeed");
    debug_room();
#endif

    create = 1;
    fprintf(flog, "room+\t[%d] %s\n", cuser->sno, rname);
  }
  else
  {
    if (cuser->room == room)
    {
      sprintf(buf, "※ 您本來就在 [%s] 聊天室囉 :)", rname);
      send_to_user(cuser, buf, 0, MSG_MESSAGE);
      return 0;
    }

    if (!CHATSYSOP(cuser) && LOCKED(room) &&
      !list_belong(room->invite, cuser->userno))
    {
      send_to_user(cuser, "※ 內有惡犬，非請莫入", 0, MSG_MESSAGE);
      return 0;
    }
  }

  exit_room(cuser, EXIT_LOGOUT, msg);
  arrive_room(cuser, room);

  if (create)
    cuser->uflag |= PERM_ROOMOP;

  return 0;
}


static void
cuser_free(cuser)
  ChatUser *cuser;
{
  int sock;

  sock = cuser->sock;
  shutdown(sock, 2);
  close(sock);

  FD_CLR(sock, &mainfset);

  list_free(&cuser->ignore);
  totaluser--;

  if (cuser->room)
  {
    exit_room(cuser, EXIT_LOSTCONN, NULL);
  }

  fprintf(flog, "BYE\t[%d] T%d X%d\n",
    cuser->sno, time(0) - cuser->tbegin, cuser->xdata);
}


static void
print_user_counts(cuser)
  ChatUser *cuser;
{
  ChatRoom *room;
  int num, userc, suserc, roomc, number;
  char buf[256];

  userc = suserc = roomc = 0;

  room = &mainroom;
  do
  {
    num = room->occupants;
    if (SECRET(room))
    {
      suserc += num;
      if (CHATSYSOP(cuser))
	roomc++;
    }
    else
    {
      userc += num;
      roomc++;
    }
  } while (room = room->next);

  number = (cuser->clitype) ? MSG_MOTD : MSG_MESSAGE;

  sprintf(buf,
    "⊙ 歡迎光臨【聊天室】，目前開了 \033[1;31m%d\033[m 間包廂", roomc);
  send_to_user(cuser, buf, 0, number);

  sprintf(buf, "⊙ 共有 \033[1;36m%d\033[m 人來擺\龍門陣", userc);
  if (suserc)
    sprintf(buf + strlen(buf), " [%d 人在秘密聊天室]", suserc);

  send_to_user(cuser, buf, 0, number);
}


static int
login_user(cu, msg)
  ChatUser *cu;
  char *msg;
{
  int utent;

  char *userid;
  char *chatid, *passwd;
  ChatUser *xuser;
  int level;
  /* struct hostent *hp; */

#ifndef STAND_ALONE
  ACCT acct;
#endif

  /* Xshadow.0915: common client support : /-! userid chatid password */
  /* client/server 版本依據 userid 抓 .PASSWDS 判斷 userlevel */

  userid = nextword(&msg);
  chatid = nextword(&msg);

#ifdef	DEBUG
  logit("ENTER", userid);
#endif

#ifndef STAND_ALONE
  /* Thor.980730: parse space before passwd */

  passwd = msg;

  /* Thor.980813: 跳過一空格即可, 因為反正如果chatid有空格, 密碼也不對 */
  /* 就算密碼對, 也不會怎麼樣:p */
  /* 可是如果密碼第一個字是空格, 那跳太多空格會進不來... */
  /* Thor.980910: 由於 nextword修改為後接空格填0, 傳入值則直接後移至0後,
                  所以不需作此動作 */
#if 0
  if (*passwd == ' ')
    passwd++;
#endif

  /* Thor.980729: load acct */

  if (!*userid || (acct_load(&acct, userid) < 0))
  {

#ifdef	DEBUG
    logit("noexist", userid);
#endif

    if (cu->clitype)
      send_to_user(cu, "錯誤的使用者代號", 0, ERR_LOGIN_NOSUCHUSER);
    else
      send_to_user(cu, CHAT_LOGIN_INVALID, 0, MSG_MESSAGE);

    return -1;
  }

  /* Thor.980813: 改用真實 password check, for C/S bbs */

  /* Thor.990214: 注意 daolib 中 非 0 代表失敗 */
  /* if (!chkpasswd(acct.passwd, passwd)) */
  if (chkpasswd(acct.passwd, passwd))
  {

#ifdef	DEBUG
    logit("fake", userid);
#endif

    if (cu->clitype)
      send_to_user(cu, "密碼錯誤", 0, ERR_LOGIN_PASSERROR);
    else
      send_to_user(cu, CHAT_LOGIN_INVALID, 0, MSG_MESSAGE);

    return -1;
  }

  level = acct.userlevel;
  utent = acct.userno;

#else				/* STAND_ALONE */
  level = 1;
  utent = ++userno_inc;
#endif				/* STAND_ALONE */

  /* Thor.980819: for client/server bbs */

#ifdef DEBUG
  log_user(NULL);
#endif

  for (xuser = mainuser; xuser; xuser = xuser->unext)
  {

#ifdef DEBUG
    log_user(xuser);
#endif

    if (xuser->userno == utent)
    {

#ifdef	DEBUG
      logit("enter", "bogus");
#endif

      if (cu->clitype)
	send_to_user(cu, "請勿派遣分身進入聊天室！", 0,
	  ERR_LOGIN_USERONLINE);
      else
	send_to_user(cu, CHAT_LOGIN_BOGUS, 0, MSG_MESSAGE);
      return -1;		/* Thor: 或是0等它自己了斷? */
    }
  }


#ifndef STAND_ALONE
  /* Thor.980629: 暫時借用 invalid_chatid 濾除 沒有PERM_CHAT的人 */
               
  if (!valid_chatid(chatid) || !(level & PERM_CHAT) || (level & PERM_DENYCHAT))
  { /* Thor.981012: 徹底一些, 連 denychat也BAN掉, 免得 client作怪 */

#ifdef	DEBUG
    logit("enter", chatid);
#endif

    if (cu->clitype)
      send_to_user(cu, "不合法的聊天室代號！", 0, ERR_LOGIN_NICKERROR);
    else
      send_to_user(cu, CHAT_LOGIN_INVALID, 0, MSG_MESSAGE);
    return 0;
  }
#endif

#ifdef	DEBUG
  debug_user();
#endif

  if (cuser_by_chatid(chatid) != NULL)
  {
    /* chatid in use */

#ifdef	DEBUG
    logit("enter", "duplicate");
#endif

    if (cu->clitype)
      send_to_user(cu, "這個代號已經有人使用", 0, ERR_LOGIN_NICKINUSE);
    else
      send_to_user(cu, CHAT_LOGIN_EXISTS, 0, MSG_MESSAGE);
    return 0;
  }

#ifdef DEBUG			/* CHATSYSOP 一進來就隱身 */
  cu->uflag = level & ~(PERM_ROOMOP | PERM_CHATOP | (CHATSYSOP(cu) ? 0 : PERM_CLOAK));
#else
  cu->uflag = level & ~(PERM_ROOMOP | PERM_CHATOP | PERM_CLOAK);
#endif

  /* Thor: 進來先清空 ROOMOP (同PERM_CHAT) */

  strcpy(cu->userid, userid);
  str_ncpy(cu->chatid, chatid, sizeof(cu->chatid));
  /* Thor.980921: str_ncpy與一般 strncpy有所不同, 特別注意 */

  fprintf(flog, "ENTER\t[%d] %s\n", cu->sno, userid);

  /* Xshadow: 取得 client 的來源 */

  dns_name(cu->rhost, cu->ibuf);
  str_ncpy(cu->rhost, cu->ibuf, sizeof(cu->rhost));
#if 0
  hp = gethostbyaddr(cu->rhost, sizeof(struct in_addr), AF_INET);
  str_ncpy(cu->rhost, hp ? hp->h_name : inet_ntoa((struct in_addr *) cu->rhost), sizeof(cu->rhost));
#endif

  cu->userno = utent;

  if (cu->clitype)
    send_to_user(cu, "順利", 0, MSG_LOGINOK);
  else
    send_to_user(cu, CHAT_LOGIN_OK, 0, MSG_MESSAGE);

  arrive_room(cu, &mainroom);

  send_to_user(cu, "", 0, MSG_MOTDSTART);
  print_user_counts(cu);
  send_to_user(cu, "", 0, MSG_MOTDEND);

#ifdef	DEBUG
  logit("enter", "OK");
#endif

  return 0;
}


static void
chat_act(cu, msg)
  ChatUser *cu;
  char *msg;
{
  if (*msg)
  {
    char buf[256];

    sprintf(buf, "%s \033[36m%s\033[m", cu->chatid, msg);
    send_to_room(cu->room, buf, cu->userno, MSG_MESSAGE);
  }
}


static void
chat_ignore(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char *str, buf[256];

  if (RESTRICTED(cu))
  {
    str = "※ 您沒有 ignore 別人的權利";
  }
  else
  {
    char *ignoree;

    str = buf;
    ignoree = nextword(&msg);
    if (*ignoree)
    {
      ChatUser *xuser;

      xuser = cuser_by_userid(ignoree);

      if (xuser == NULL)
      {
	sprintf(str, msg_no_such_id, ignoree);
      }
      else if (xuser == cu || CHATSYSOP(xuser) ||
	(ROOMOP(xuser) && (xuser->room == cu->room)))
      {
	sprintf(str, "◆ 不可以 ignore [%s]", ignoree);
      }
      else
      {
	if (list_belong(cu->ignore, xuser->userno))
	{
	  sprintf(str, "※ %s 已經被凍結了", xuser->chatid);
	}
	else
	{
	  list_add(&(cu->ignore), xuser);
	  sprintf(str, "◆ 將 [%s] 打入冷宮了 :p", xuser->chatid);
	}
      }
    }
    else
    {
      UserList *list;

      if (list = cu->ignore)
      {
	int len;
	char userid[16];

	send_to_user(cu, "◆ 這些人被打入冷宮了：", 0, MSG_MESSAGE);
	len = 0;
	do
	{
	  sprintf(userid, "%-13s", list->userid);
	  strcpy(str + len, userid);
	  len += 13;
	  if (len >= 78)
	  {
	    send_to_user(cu, str, 0, MSG_MESSAGE);
	    len = 0;
	  }
	} while (list = list->next);

	if (len == 0)
	  return;
      }
      else
      {
	str = "◆ 您目前並沒有 ignore 任何人";
      }
    }
  }

  send_to_user(cu, str, 0, MSG_MESSAGE);
}


static void
chat_unignore(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char *ignoree, *str, buf[80];

  ignoree = nextword(&msg);

  if (*ignoree)
  {
    sprintf(str = buf, (list_delete(&(cu->ignore), ignoree)) ?
      "◆ [%s] 不再被您冷落了" :
      "◆ 您並未 ignore [%s] 這號人物", ignoree);
  }
  else
  {
    str = "◆ 請指明 user ID";
  }
  send_to_user(cu, str, 0, MSG_MESSAGE);
}


static void
chat_join(cu, msg)
  ChatUser *cu;
  char *msg;
{
  if (RESTRICTED(cu))
  {
    send_to_user(cu, "※ 您沒有加入其他聊天室的權限", 0, MSG_MESSAGE);
  }
  else
  {
    char *roomid = nextword(&msg);

    if (*roomid)
      enter_room(cu, roomid, msg);
    else
      send_to_user(cu, "※ 請指定聊天室", 0, MSG_MESSAGE);
  }
}


static void
chat_kick(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char *twit, buf[80];
  ChatUser *xuser;
  ChatRoom *room;

  if (!ROOMOP(cu))
  {
    send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
    return;
  }

  twit = nextword(&msg);
  xuser = cuser_by_chatid(twit);

  if (xuser == NULL)
  {                       /* Thor.980604: 用 userid也嘛通 */
    xuser = cuser_by_userid(twit);
  }
               
  if (xuser == NULL)
  {
    sprintf(buf, msg_no_such_id, twit);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
    return;
  }

  room = cu->room;
  if (room != xuser->room || CLOAK(xuser))
  {
    sprintf(buf, msg_not_here, twit);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
    return;
  }

  if (CHATSYSOP(xuser))
  {
    sprintf(buf, "◆ 不可以 kick [%s]", twit);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
    return;
  }

  exit_room(xuser, EXIT_KICK, (char *) NULL);

  if (room == &mainroom)
    xuser->uptime = 0;		/* logout_user(xuser); */
  else
    enter_room(xuser, MAIN_NAME, (char *) NULL);  
    /* Thor.980602: 其實踢就踢,不要show出xxx離開了的訊息比較好 */
}


static void
chat_makeop(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char *newop, buf[80];
  ChatUser *xuser;
  ChatRoom *room;

  /* Thor.980603: PERM_ALLCHAT 改為 default 沒有 roomop, 但可以自己取得 */

  newop = nextword(&msg);

  room = cu->room;

  if (!*newop && CHATSYSOP(cu))
  {
    /* Thor.980603: PERM_ALLCHAT 改為 default 沒有 roomop, 但可以自己取得 */
    cu->uflag ^= PERM_CHATOP;

    user_changed(cu);
    if (!CLOAK(cu))
    {
      sprintf(buf,ROOMOP(cu) ? "※ 天使 將 Op 權力授予 %s"
                             : "※ 天使 將 %s 的 Op 權力收回", cu->chatid);
      send_to_room(room, buf, 0, MSG_MESSAGE);
    }
    
    return;
  }

  /* if (!ROOMOP(cu)) */
  if (!(cu->uflag & PERM_ROOMOP)) /* Thor.980603: chat room總管不能轉移 Op 權力 */
  {
    send_to_user(cu, "◆ 您不能轉移 Op 的權力" /* msg_not_op */, 0, MSG_MESSAGE);
    return;
  }

  xuser = cuser_by_chatid(newop);

#if 0
  if (xuser == NULL)
  {                       /* Thor.980604: 用 userid 嘛也通 */
    xuser = cuser_by_userid(newop);
  }
#endif

  if (xuser == NULL)
  {
    sprintf(buf, msg_no_such_id, newop);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
    return;
  }

  if (cu == xuser)
  {
    send_to_user(cu, "※ 您早就已經是 Op 了啊", 0, MSG_MESSAGE);
    return;
  }

  /* room = cu->room; */

  if (room != xuser->room || CLOAK(xuser))
  {
    sprintf(buf, msg_not_here, xuser->chatid);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
    return;
  }

  cu->uflag &= ~PERM_ROOMOP;
  xuser->uflag |= PERM_ROOMOP;

  user_changed(cu);
  user_changed(xuser);

  if (!CLOAK(cu))
  {
    sprintf(buf, "※ %s 將 Op 權力轉移給 %s",
      cu->chatid, xuser->chatid);
    send_to_room(room, buf, 0, MSG_MESSAGE);
  }
}


static void
chat_invite(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char *invitee, buf[80];
  ChatUser *xuser;
  ChatRoom *room;
  UserList **list;

  if (!ROOMOP(cu))
  {
    send_to_user(cu, msg_not_op, 0, MSG_MESSAGE);
    return;
  }

  invitee = nextword(&msg);
  xuser = cuser_by_chatid(invitee);

#if 0
  if (xuser == NULL)
  {                       /* Thor.980604: 用 userid 嘛也通 */
    xuser = cuser_by_userid(invitee);
  }
#endif
               
  if (xuser == NULL)
  {
    sprintf(buf, msg_no_such_id, invitee);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
    return;
  }

  room = cu->room;		/* Thor: 是否要 check room 是否 NULL ? */
  list = &(room->invite);

  if (list_belong(*list, xuser->userno))
  {
    sprintf(buf, "※ %s 已經接受過邀請了", xuser->chatid);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
    return;
  }
  list_add(list, xuser);

  sprintf(buf, "※ %s 邀請您到 [%s] 聊天室",
    cu->chatid, room->name);
  send_to_user(xuser, buf, 0, MSG_MESSAGE);
  sprintf(buf, "※ %s 收到您的邀請了", xuser->chatid);
  send_to_user(cu, buf, 0, MSG_MESSAGE);
}


static void
chat_broadcast(cu, msg)
  ChatUser *cu;
  char *msg;
{
  char buf[80];

  if (!CHATSYSOP(cu))
  {
    send_to_user(cu, "※ 您沒有在聊天室廣播的權力!", 0, MSG_MESSAGE);
    return;
  }

  if (*msg == '\0')
  {
    send_to_user(cu, "※ 請指定廣播內容", 0, MSG_MESSAGE);
    return;
  }

  sprintf(buf, "\033[1m※ " BBSNAME "談天室廣播中 [%s].....\033[m",
    cu->chatid);
  send_to_room(ROOM_ALL, buf, 0, MSG_MESSAGE);
  sprintf(buf, "◆ %s", msg);
  send_to_room(ROOM_ALL, buf, 0, MSG_MESSAGE);
}


static void
chat_bye(cu, msg)
  ChatUser *cu;
  char *msg;
{
  exit_room(cu, EXIT_LOGOUT, msg);
  cu->uptime = 0;
  /* logout_user(cu); */
}


/* --------------------------------------------- */
/* MUD-like social commands : action             */
/* --------------------------------------------- */


#if 0	/* itoc.010816: 重新翻修一些不太適當的 action 敘述 */
  1. 注意按字母排列。
  2. 請愛用全形標點符號。
  3. 三類 action 不能有重覆。
  4. 由於 action 採用「部分比對」，故最好不要有指令包含另一指令所有關鍵字的狀況。
     （例如 fire/fireball，kiss/kissbye，no/nod，tea/tear/tease，drive/drivel，love/lover）
     （有這樣的情形也不會怎麼樣，只是使用者容易搞混）
  5. 由於 action 部分比對至少 2 bytes，故不要用 //1 //2 這類只有一個字的 action。
  6. 由於 action 採用部分比對，故指令不必用縮寫。
  7. 統一 action message 最後不要加句點。
  8. 修正錯字。（是 adore，不是 aodre 啊 :p）
  9. 減少重覆的字眼。（不要老是「死去活來」啊 :p）
#endif


struct ChatAction
{
  char *verb;			/* 動詞 */
  char *chinese;		/* 中文翻譯 */
  char *part1_msg;		/* 介詞 */
  char *part2_msg;		/* 動作 */
};


static ChatAction *
action_fit(action, actnum, cmd)		/* 找看看是哪個 ChatAction */
  ChatAction *action;
  int actnum;
  char *cmd;
{
  ChatAction *pos, *locus, *mid;	/* locus:左指標 mid:中指標 pos:右指標 */
  int cmp;

  /* itoc.010927: 由於 ChatAction 都是按字母排序的，所以可以用 binary search */
  /* itoc.010928.註解:由於是 binary search 所以雖然 recline 排在 recycle 前面
    但是打 //rec 時卻可能出現 //recycle 的效果，判定優先次序端賴 binary 的順序 */

  locus = action;
  pos = action + actnum - 1;		/* 最後一個是 NULL，但不可能被檢查到 */

  while (1)
  {
    if (pos <= locus + 1)
      break;

    mid = locus + ((pos - locus) >> 1);

    if (!(cmp = str_belong(mid->verb, cmd)))	/* itoc.010321: MUD-like match */
      return mid;
    else if (cmp < 0)
      locus = mid;
    else
      pos = mid;
  }

  /* 特例: 如果右指標停留在 1，要檢查第 0 個 */
  if (pos == action + 1)
  {
    if (!str_belong(action->verb, cmd))		/* itoc.010321: MUD-like match */
      return action;
  }

  return NULL;
}


/* itoc.010805.註解:  //adore sysop   itoc 對 sysop 的景仰有如滔滔江水，連綿不絕… */

#define ACTNUM_PARTY	110

static ChatAction party_data[ACTNUM_PARTY] =
{
  {
    "adore", "景仰", "對", "的景仰有如滔滔江水，連綿不絕…"
  },
  {
    "aluba", "阿魯巴", "把", "架上柱子阿到死！"
  },
  {
    "aruba", "阿魯巴", "把", "架上柱子阿到死！"
  },
  {
    "bark", "吠叫", "汪汪！對", "大聲吠叫"
  },
  {
    "bite", "啃咬", "把", "咬得死去活來"
  },
  {
    "blade", "一刀", "一刀把", "送上西天"
  },
  {
    "bless", "祝福", "祝福", "心想事成"
  },
  {
    "blink", "眨眼", "對著", "眨眨眼，不知暗示著什麼"
  },
  {
    "board", "主機板", "把", "抓去跪主機板"
  },
  {
    "bokan", "氣功\", "雙掌微合，蓄勢待發……突然間，電光乍現，對", "使出了Ｂｏ--Ｋａｎ"
  },
  {
    "bow", "鞠躬", "畢躬畢敬的向", "鞠躬"
  },
  {
    "box", "幕之內", "開始輪擺\式移位，對", "作肝臟攻擊"
  },
  {
    "bye", "掰掰", "向", "說掰掰"
  },
  {
    "cake", "丟蛋糕", "拿出一個蛋糕，往", "的臉上砸去"
  },
  {
    "call", "呼喚", "大聲的呼喚，啊～",	"啊～人在哪裡啊啊～啊"
  },
  {
    "caress", "輕撫", "輕輕的撫摸著", ""
  },
  {
    "clap", "鼓掌", "向", "熱烈鼓掌"
  },
  {
    "claw", "抓抓", "從貓咪樂園借了隻貓爪，把",	"抓得昏天暗地"
  },
  {
    "clock", "切鬧鐘", "切掉", "的鬧鐘，快起床啦"
  },
  {
    "cola", "灌可樂", "對", "灌了一加侖的可樂"
  },
  {
    "comfort", "安慰", "溫言安慰", ""
  },
  {
    "congratulate", "恭喜", "從背後拿出了拉炮，呯！呯！恭喜", ""
  },
  {
    "cowhide", "鞭打","拿鞭子對", "狠狠地抽打"
  },
  {
    "cpr", "口對口", "對著", "做口對口人工呼吸"
  },
  {
    "crime", "道德", "說：", "的道德指數不夠，滿臉戾氣"
  },
  {
    "cringe", "乞憐", "向", "卑躬屈膝，搖尾乞憐"
  },
  {
    "cry", "大哭", "向", "嚎啕大哭"
  },
  {
    "curtsy", "中古禮", "優雅地對著", "行中古世紀的屈膝禮。"
  },
  {
    "dance", "跳舞", "拉了", "的手翩翩起舞"
  },
  {
    "destroy", "毀滅", "祭起了『極大毀滅咒文』，轟向", ""
  },
  {
    "dogleg", "狗腿", "對", "阿諛奉承，大大狗腿了一番"
  },
  {
    "drivel", "流口水",	"對著",	"流口水"
  },
  {
    "envy", "羨慕", "向", "流露出羨慕的眼光"
  },
  {
    "evening", "晚安", "對", "說『晚安』"
  },
  {
    "eye", "送秋波", "對", "頻送秋波"
  },
  {
    "fire", "銬問", "拿著火紅的鐵棒走向", ""
  },
  {
    "forgive", "原諒", "接受", "的道歉，原諒了他"
  },
  {
    "french", "法式吻",	"把舌頭伸到", "喉嚨裡～～～哇！一個浪漫的法國式深吻"
  },
  {
    "fuzzy", "飛鳥", "派出飛鳥一號向", "衝過去"
  },
  {
    "gag", "縫嘴巴", "把", " 的嘴巴用針縫起來"
  },
  {
    "giggle", "傻笑", "對著", "傻傻的呆笑"
  },
  {
    "glare", "瞪人", "冷冷地瞪著", ""
  },
  {
    "glue", "補心", "用快乾把", "的心黏了起來"
  },
  {
    "goodbye", "告別", "淚\眼汪汪的向",	"告別"
  },
  {
    "grin", "奸笑", "對", "露出邪惡的笑容"
  },
  {
    "growl", "咆哮", "對", "咆哮不已"
  },
  {
    "hand", "握手", "跟", "握手"
  },
  {
    "hide", "躲", "躲在", "背後"
  },
  {
    "hospital", "送醫院", "把", "送進醫院"
  },
  {
    "hrk", "昇龍拳", "沉穩了身形，匯聚了內勁，對", "使出了一記Ｈｏ--Ｒｙｕ--Ｋａｎ"
  },
  {
    "hug", "熱擁", "熱情的擁抱", ""
  },
  {
    "hypnoze", "催眠", "拿著掛錶晃呀晃的，對", "展開催眠"
  },
  {
    "jab", "捅人", "用力地捅著", "，似乎對他很是不滿"
  },
  {
    "judo", "柔道", "抓住了", "的衣襟，轉身……啊，是一記過肩摔"
  },
  {
    "kick", "踢人", "把", "踢得痛哭流涕"
  },
  {
    "kill", "砍人", "把", "亂刀砍死～～"
  },
  {
    "kiss", "輕吻", "輕吻", "的臉頰"
  },
  {
    "laugh", "嘲笑", "大聲嘲笑", ""
  },
  {
    "levis", "給我", "說：給我", "！其餘免談！"
  },
  {
    "lick", "舔", "狂舔", ""
  },
  {
    "listen", "聽", "叫", "閉嘴仔細聽"
  },
  {
    "lobster", "壓制", "施展逆蝦形固定，把", "壓制在地板上"
  },
  {
    "love", "表白", "對", "深情的表白"
  },
  {
    "mail", "打包", "把", "打包遞送到大陸"
  },
  {
    "marry", "求婚", "捧著九百九十九朵玫瑰向", "求婚"
  },
  {
    "morning", "早安", "對", "說『早安』"
  },
  {
    "noon", "午安", "對", "說『午安』"
  },
  {
    "nod", "點頭", "向", "點頭稱是"
  },
  {
    "nudge", "頂肚子", "用手肘頂", "的肥肚子"
  },
  {
    "pad", "拍肩膀", "輕拍", "的肩膀"
  },
  {
    "pan", "平底鍋", "從背後拿出了平底鍋，把", "敲昏了"
  },
  {
    "pat", "拍頭", "拍拍", "的頭"
  },
  {
    "pettish", "撒嬌", "跟", "嗲聲嗲氣地撒嬌"
  },
  {
    "pili", "霹靂", "使出 君子風 天地根	般若懺 三式合一打向", "～～"
  },
  {
    "pinch", "擰人", "用力的把", "擰得黑青"
  },
  {
    "poke", "戳弄", "戳了戳", "想要引起他的注意"
  },
  {
    "puding", "灌布丁",	"對", "灌了一卡車布丁"
  },
  {
    "roll", "打滾", "放出多爾袞的音樂，", "在地上滾來滾去"
  },
  {
    "protect", "保護", "誓死保護著", ""
  },
  {
    "pull", "拉", "死命地拉住",	"不放"
  },
  {
    "punch", "揍人", "狠狠揍了", "一頓"
  },
  {
    "rascal", "耍賴", "跟", "耍賴"
  },
  {
    "recline", "入懷", "鑽到", "的懷裡睡著了……"
  },
  {
    "recycle", "回收桶", "把", "丟到資源回收桶"
  },
  {
    "respond", "負責", "安慰", "說：『不要哭，我會負責的』"
  },
  {
    "scratch", "磨爪", "撿起", "身邊的石子磨磨自己的利爪"
  },
  {
    "sex", "性騷擾", "對", "性騷擾"
  },
  {
    "shit", "雪特", "對", "罵了一聲『雪特』"
  },
  {
    "shrug", "聳肩", "無奈地向", "聳了聳肩膀"
  },
  {
    "sigh", "歎氣", "對", "歎了一口氣"
  },
  {
    "slap", "打耳光", "啪啪的巴了", "一頓耳光"
  },
  {
    "smooch", "擁吻", "擁吻著",	""
  },
  {
    "snicker", "竊笑", "嘿嘿嘿地對", "竊笑"
  },
  {
    "sniff", "不屑", "對", "嗤之以鼻"
  },
  {
    "sorry", "對不起", "向", "說對不起！我對不起大家，我對不起國家社會"
  },
  {
    "spank", "打屁屁", "用巴掌打", "的臀部"
  },
  {
    "squeeze", "緊擁", "緊緊地擁抱著", ""
  },
  {
    "thank", "感謝", "向", "感謝得五體投地"
  },
  {
    "throw", "丟擲", "拿了腳下一塊大石頭朝", "那丟了過去"
  },
  {
    "tickle", "搔癢", "咕嘰咕嘰，搔", "的癢"
  },
  {
    "wait", "等一下", "叫", "等一下哦！"
  },
  {
    "wake", "搖醒", "輕輕地把",	"搖醒"
  },
  {
    "wave", "揮手", "對著", "揮揮手，表示告別之意"
  },
  {
    "welcome", "歡迎", "歡迎", "進來八卦一下"
  },
  {
    "what", "什麼", "說：『", "哩公瞎密哇隴聽某?？?﹖?』"
  },
  {
    "whip", "鞭笞", "手上拿著蠟燭，用鞭子痛打",	""
  },
  {
    "wiggle", "扭屁股",	"對著",	"扭屁股"
  },
  {
    "wink", "眨眼", "對", "神秘的眨眨眼睛"
  },
  {
    "zap", "猛攻", "對", "瘋狂的攻擊"
  },
  {
    NULL, NULL, NULL, NULL
  }
};


static int
party_action(cu, cmd, party)
  ChatUser *cu;
  char *cmd;
  char *party;
{
  ChatAction *cap;
  char buf[256];

  if ((cap = action_fit(party_data, ACTNUM_PARTY, cmd)))
  {
    if (*party == '\0')
    {
      party = "大家";
    }
    else
    {
      ChatUser *xuser;

      xuser = fuzzy_cuser_by_chatid(party);
      if (xuser == NULL)
      {			/* Thor.980724: 用 userid也嘛通 */
	xuser = cuser_by_userid(party);
      }

      if (xuser == NULL)
      {
	sprintf(buf, msg_no_such_id, party);
	send_to_user(cu, buf, 0, MSG_MESSAGE);
	return 0;
      }
      else if (xuser == FUZZY_USER)
      {
	send_to_user(cu, "※ 請指明聊天代號", 0, MSG_MESSAGE);
	return 0;
      }
      else if (cu->room != xuser->room || CLOAK(xuser))
      {
	sprintf(buf, msg_not_here, party);
	send_to_user(cu, buf, 0, MSG_MESSAGE);
	return 0;
      }
      else
      {
	party = xuser->chatid;
      }
    }
    sprintf(buf, "\033[1;32m%s \033[31m%s\033[33m %s \033[31m%s\033[m",
      cu->chatid, cap->part1_msg, party, cap->part2_msg);
    send_to_room(cu->room, buf, cu->userno, MSG_MESSAGE);
    return 0;			/* Thor: cu->room 是否為 NULL? */
  }
  return 1;
}


/* --------------------------------------------- */
/* MUD-like social commands : speak              */
/* --------------------------------------------- */


/* itoc.010805.註解:  //ask 大家今天過得好嗎？	 itoc 問大家今天過得好嗎？*/

#define ACTNUM_SPEAK	29

static ChatAction speak_data[ACTNUM_SPEAK] =
{
  {
    "ask", "詢問", "問", NULL
  },
  {
    "broadcast", "廣播", "廣播", NULL
  },
  {
    "chant", "歌頌", "高聲歌頌", NULL
  },
  {
    "cheer", "喝采", "喝采", NULL
  },
  {
    "chuckle", "輕笑", "輕笑", NULL
  },
  {
    "curse", "詛咒", "暗幹", NULL
  },
  {
    "demand", "要求", "要求", NULL
  },
  {
    "fuck", "公幹", "公幹", NULL
  },
  {
    "groan", "呻吟", "呻吟", NULL
  },
  {
    "grumble", "發牢騷", "發牢騷", NULL
  },
  {
    "guitar", "彈唱", "邊彈著吉他，邊唱著", NULL
  },
  {
    "hum", "喃喃", "喃喃自語", NULL
  },
  {
    "moan", "怨嘆", "怨嘆", NULL
  },
  {
    "notice", "強調", "強調", NULL
  },
  {
    "order", "命令", "命令", NULL
  },
  {
    "ponder", "沈思", "沈思", NULL
  },
  {
    "pout", "噘嘴", "噘著嘴說",	NULL
  },
  {
    "pray", "祈禱", "祈禱", NULL
  },
  {
    "request", "懇求", "懇求", NULL
  },
  {
    "shout", "大罵", "大罵", NULL
  },
  {
    "sing", "唱歌", "唱歌", NULL
  },
  {
    "smile", "微笑", "微笑", NULL
  },
  {
    "smirk", "假笑", "假笑", NULL
  },
  {
    "swear", "發誓", "發誓", NULL
  },
  {
    "tease", "嘲笑", "嘲笑", NULL
  },
  {
    "whimper", "嗚咽", "嗚咽的說", NULL
  },
  {
    "yawn", "哈欠", "邊打哈欠邊說", NULL
  },
  {
    "yell", "大喊", "大喊", NULL
  },
  {
    NULL, NULL, NULL, NULL
  }
};


static int
speak_action(cu, cmd, msg)
  ChatUser *cu;
  char *cmd;
  char *msg;
{
  ChatAction *cap;
  char buf[256];

  if ((cap = action_fit(speak_data, ACTNUM_SPEAK, cmd)))
  {
    sprintf(buf, "\033[1;32m%s \033[31m%s：\033[33m %s\033[m",
      cu->chatid, cap->part1_msg, msg);
    send_to_room(cu->room, buf, cu->userno, MSG_MESSAGE);
    return 0;
  }
  return 1;
}


/* ----------------------------------------------------- */
/* MUD-like social commands : condition			 */
/* ----------------------------------------------------- */


/* itoc.010805.註解:  //agree	itoc 深表同意 */

#define ACTNUM_CONDITION	73

static ChatAction condition_data[ACTNUM_CONDITION] =
{
  {
    "agree", "同意", "深表同意", NULL
  },
  {
    "aha", "靈光", "苦思良久，忽然靈光一現，不禁呀哈的一聲", NULL
  },
  {
    "akimbo", "插腰", "又氣又無奈的兩手插腰", NULL
  },
  {
    "alas", "哎呀", "哎呀呀～", NULL
  },
  {
    "applaud", "拍手", "啪啪啪啪啪……啪啪", NULL
  },
  {
    "avert", "害羞", "害羞地轉開視線", NULL
  },
  {
    "ayo", "唉呦喂", "唉呦喂～", NULL
  },
  {
    "back", "坐回來", "回來坐正繼續奮戰", NULL
  },
  {
    "blood", "在血中", "倒在血泊之中", NULL
  },
  {
    "blush", "臉紅", "臉都紅了", NULL
  },
  {
    "broke", "心碎", "的心破碎成一片一片的", NULL
  },
  {
    "bug", "臭蟲", "發現這系統有Ｂｕｇ～", NULL
  },
  {
    "careles", "沒人理", "嗚～～都沒有人理我 ：～", NULL
  },
  {
    "chew", "嗑瓜子", "很悠閒的嗑起瓜子來了", NULL
  },
  {
    "climb", "爬山", "自己慢慢爬上山來……", NULL
  },
  {
    "cold", "感冒", "感冒了，媽媽不讓我出去玩 ：（", NULL
  },
  {
    "cough", "咳嗽", "咳了幾聲", NULL
  },
  {
    "crash", "當機", "嗚…" BBSNAME "當機了", NULL
  },
  {
    "die", "暴斃", "當場暴斃", NULL
  },
  {
    "dive", "潛水", "跳到水裡躲起來", NULL
  },
  {
    "faint", "昏倒", "當場昏倒", NULL
  },
  {
    "fart", "放屁", "全是在放屁，胡扯一通！", NULL
  },
  {
    "flop", "香蕉皮", "踩到香蕉皮…滑倒！", NULL
  },
  {
    "fly", "飄飄然", "飄飄然地，好似飛了起來", NULL
  },
  {
    "frown", "蹙眉", "蹙眉，不知為了什麼", NULL
  },
  {
    "gold", "拿金牌", "唱著：『金ㄍㄠˊ金ㄍㄠˊ出國比賽，得冠軍，拿金牌，光榮倒鄧來！』", NULL
  },
  {
    "gulu", "肚子餓", "的肚子發出咕嚕咕嚕～的聲音", NULL
  },
  {
    "haha", "哈哈", "哇哈哈哈…大笑了起來", NULL
  },
  {
    "happy", "高興", "高興得在地上打滾", NULL
  },
  {
    "hiccup", "打嗝", "打嗝個不停", NULL
  },
  {
    "hoho", "呵呵", "呵呵呵笑個不停", NULL
  },
  {
    "hypnzed", "被催眠", "眼神呆滯，被催眠了……ｚＺｚzzz", NULL
  },
  {
    "idle", "呆住", "瞬間呆住了", NULL
  },
  {
    "jacky", "痞子", "痞子般的晃來晃去", NULL
  },
  {
    "jealous", "吃醋", "氣鼓鼓地喝了一缸醋", NULL
  },
  {
    "jump", "跳樓", "跳樓自殺",	NULL
  },
  {
    "luck", "幸運", "哇！福氣啦！", NULL
  },
  {
    "macarn", "一種舞",	"開始跳起了ＭａＣａＲｅＮａ～～～～", NULL
  },
  {
    "miou", "喵喵", "喵喵口苗口苗～～～～～", NULL
  },
  {
    "money", "賺錢", "埋首研究怎樣賺大錢", NULL
  },
  {
    "mouth", "扁嘴", "扁嘴中！", NULL
  },
  {
    "mutter", "低咕", "低聲咕噥著某些事。", NULL
  },
  {
    "nani", "怎麼會", "：奈ㄝ啊捏??", NULL
  },
  {
    "nose", "流鼻血", "流鼻血",	NULL
  },
  {
    "puke", "嘔吐", "嘔吐中", NULL
  },
  {
    "rest", "休息", "休息中，請勿打擾",	NULL
  },
  {
    "reverse", "翻肚", "翻肚", NULL
  },
  {
    "room", "開房間", "r-o-O-m-r-O-Ｏ-Mmm-rRＲ........", NULL
  },
  {
    "scream", "尖叫", "大聲尖叫！ 啊~~~~~~~", NULL
  },
  {
    "shake", "搖頭", "搖了搖頭", NULL
  },
  {
    "sleep", "睡著", "趴在鍵盤上睡著了，口水流進鍵盤，造成當機！", NULL
  },
  {
    "snore", "打鼾中", "打鼾中…", NULL
  },
  {
    "sob", "賤胚", "Ｓｏｎ Ｏｆ Ｂｉｔｃｈ！！", NULL
  },
  {
    "stare", "凝視", "靜靜地凝視著天空", NULL
  },
  {
    "stretch", "疲倦", "伸伸懶腰又打了個呵欠很疲倦似的。", NULL
  },
  {
    "story", "講古", "開始講古了", NULL
  },
  {
    "strut", "搖擺\走",	"大搖大擺\地走", NULL
  },
  {
    "suicide", "自殺", "自殺", NULL
  },
  {
    "sweat", "流汗", "揮汗如雨！", NULL
  },
  {
    "tear", "流淚\", "痛哭流涕中.....",	NULL
  },
  {
    "think", "思考", "歪著頭想了一下", NULL
  },
  {
    "tongue", "吐舌", "吐了吐舌頭", NULL
  },
  {
    "wall", "撞牆", "跑去撞牆",	NULL
  },
  {
    "wawa", "哇哇", "哇哇哇~~~~~!!!!!  ~~~>_<~~~", NULL
  },
  {
    "wc", "洗手間", "企洗手間一下 :>", NULL
  },
  {
    "whine", "肚子餓", "肚子餓!	:(", NULL
  },
  {
    "whistle", "吹口哨", "吹口哨", NULL
  },
  {
    "wolf", "狼嚎", "ㄠㄨㄠㄨ…ㄠㄨㄠㄨ…", NULL
  },
  {
    "www", "汪汪", "汪汪汪！", NULL
  },
  {
    "ya", "ㄛ耶", "噢～ＹＡ！ *^_^*", NULL
  },
  {
    "zzz", "打呼", "呼嚕～ZZzZzｚＺZZzzZzzzZZ", NULL
  },
  {
    NULL, NULL, NULL, NULL
  }
};


static int
condition_action(cu, cmd)
  ChatUser *cu;
  char *cmd;
{
  ChatAction *cap;
  char buf[256];

  if ((cap = action_fit(condition_data, ACTNUM_CONDITION, cmd)))
  {
    sprintf(buf, "\033[1;32m%s \033[31m%s\033[m",
      cu->chatid, cap->part1_msg);
    send_to_room(cu->room, buf, cu->userno, MSG_MESSAGE);
    return 1;
  }
  return 0;
}


/* --------------------------------------------- */
/* MUD-like social commands : help               */
/* --------------------------------------------- */


static char *dscrb[] =
{
  "\033[1;37m【 Verb + Nick：   動詞 + 對方名字 】\033[36m  例：//kick piggy\033[m",
  "\033[1;37m【 Verb + Message：動詞 + 要說的話 】\033[36m  例：//sing 天天天藍\033[m",
  "\033[1;37m【 Verb：動詞 】   ↑↓：舊話重提\033[m", NULL
};



static ChatAction *catbl[] =
{
  party_data, speak_data, condition_data, NULL
};


static void
chat_partyinfo(cu, msg)
  ChatUser *cu;
  char *msg;
{
  if (common_client_command)
  {
    send_to_user(cu, "3 動作  交談  狀態", 0, MSG_PARTYINFO);
  }
}


static void
chat_party(cu, msg)
  ChatUser *cu;
  char *msg;
{
  int kind, i;
  ChatAction *cap;
  char buf[80];

  if (!common_client_command)
    return;

  kind = atoi(nextword(&msg));
  if (kind < 0 || kind > 2)
    return;

  sprintf(buf, "%d\t%s", kind, kind == 2 ? "I" : "");

  /* Xshadow: 只有 condition 才是 immediate mode */
  send_to_user(cu, buf, 0, MSG_PARTYLISTSTART);

  cap = catbl[kind];
  for (i = 0; cap[i].verb; i++)
  {
    sprintf(buf, "%-10s %-20s", cap[i].verb, cap[i].chinese);
    send_to_user(cu, buf, 0, MSG_PARTYLIST);
  }

  sprintf(buf, "%d", kind);
  send_to_user(cu, buf, 0, MSG_PARTYLISTEND);
}


#define	MAX_VERB_LEN	8
#define VERB_NO		10


static void
view_action_verb(cu, cmd)	/* Thor.980726: 新加動詞分類顯示 */
  ChatUser *cu;
  int cmd;
{
  int i;
  char *p, *q, *data, *expn, buf[256];
  ChatAction *cap;

  send_to_user(cu, "/c", 0, MSG_CLRSCR);

  data = buf;

  if (cmd < '1' || cmd > '3')
  {				/* Thor.980726: 寫得不好, 想辦法改進... */
    for (i = 0; p = dscrb[i]; i++)
    {
      sprintf(data, "  [//]help %d          - MUD-like 社交動詞   第 %d 類", i + 1, i + 1);
      send_to_user(cu, data, 0, MSG_MESSAGE);
      send_to_user(cu, p, 0, MSG_MESSAGE);
      send_to_user(cu, " ", 0, MSG_MESSAGE);	/* Thor.980726: 換行 */
    }
  }
  else
  {
    i = cmd - '1';

    send_to_user(cu, dscrb[i], 0, MSG_MESSAGE);

    expn = buf + 100;		/* Thor.980726: 應該不會overlap吧? */

    *data = '\0';
    *expn = '\0';

    cap = catbl[i];

    for (i = 0; p = cap[i].verb; i++)
    {
      q = cap[i].chinese;

      strcat(data, p);
      strcat(expn, q);

      if (((i + 1) % VERB_NO) == 0)
      {
	send_to_user(cu, data, 0, MSG_MESSAGE);
	send_to_user(cu, expn, 0, MSG_MESSAGE);	/* Thor.980726: 顯示中文註解 */
	*data = '\0';
	*expn = '\0';
      }
      else
      {
	strncat(data, "        ", MAX_VERB_LEN - strlen(p));
	strncat(expn, "        ", MAX_VERB_LEN - strlen(q));
      }
    }

    if (i % VERB_NO)
    {
      send_to_user(cu, data, 0, MSG_MESSAGE);
      send_to_user(cu, expn, 0, MSG_MESSAGE);	/* Thor.980726: 顯示中文註解 */
    }
  }
  /* send_to_user(cu, " ", 0); *//* Thor.980726: 換行, 需要 " " 嗎? */
}


/* ----------------------------------------------------- */
/* chat user service routines                            */
/* ----------------------------------------------------- */


static ChatCmd chatcmdlist[] =
{
  "act", chat_act, 0,
  "bye", chat_bye, 0,
  "chatroom", chat_chatroom, 1,		/* Xshadow: for common client */
  "clear", chat_clear, 0,
  "cloak", chat_cloak, 2,
  "date", chat_date, 0,
  "flags", chat_setroom, 0,
  "help", chat_help, 0,
  "ignore", chat_ignore, 1,
  "invite", chat_invite, 0,
  "join", chat_join, 0,
  "kick", chat_kick, 1,
  "msg", chat_private, 0,
  "nick", chat_nick, 0,
  "operator", chat_makeop, 0,
  "party", chat_party, 1,		/* Xshadow: party data for common client */
  "partyinfo", chat_partyinfo, 1,	/* Xshadow: party info for common client */

#ifndef STAND_ALONE
  "query", chat_query, 0,
#endif

  "quit", chat_bye, 0,

  "room", chat_list_rooms, 0,
  "unignore", chat_unignore, 1,
  "whoin", chat_list_by_room, 1,
  "wall", chat_broadcast, 2,

  "who", chat_map_chatids_thisroom, 0,
  "list", chat_list_users, 0,
  "topic", chat_topic, 1,
  "version", chat_version, 1,

  NULL, NULL, 0
};


/* Thor: 0 不用 exact, 1 要 exactly equal, 2 秘密指令 */


static int
command_execute(cu)
  ChatUser *cu;
{
  char *cmd, *msg, buf[128];
  /* Thor.981108: lkchu patch: chatid + msg 只用 80 bytes 不夠, 改為 128 */
  ChatCmd *cmdrec;
  int match, ch;

  msg = cu->ibuf;
  match = *msg;

  /* Validation routine */

  if (cu->room == NULL)
  {
    /* MUST give special /! or /-! command if not in the room yet */

    if (match == '/' && ((ch = msg[1]) == '!' || (ch == '-' && msg[2] == '!')))
    {
      if (ch == '-')
	fprintf(flog, "cli\t[%d] S%d\n", cu->sno, cu->sock);

      cu->clitype = (ch == '-') ? 1 : 0;
      return (login_user(cu, msg + 2 + cu->clitype));
    }
    else
    {
      return -1;
    }
  }

  /* If not a /-command, it goes to the room. */

  if (match != '/')
  {
    if (match)
    {
      if (cu->room && !CLOAK(cu))	/* 隱身的人也不能說話哦 */
      {
	char chatid[16];

	sprintf(chatid, "%s:", cu->chatid);
	sprintf(buf, "%-10s%s", chatid, msg);
	send_to_room(cu->room, buf, cu->userno, MSG_MESSAGE);
      }
    }
    return 0;
  }

  msg++;
  cmd = nextword(&msg);
  match = 0;

  if (*cmd == '/')
  {
    cmd++;
    /* if (!*cmd || !str_cmp("help", cmd)) */
    if (!*cmd || str_match(cmd, "help") >= 0)	/* itoc.010321: 部分 match 就算 */
    {
      cmd = nextword(&msg);	/* Thor.980726: 動詞分類 */
      view_action_verb(cu, *cmd);
      match = 1;
    }
    else if (!party_action(cu, cmd, msg))
      match = 1;
    else if (!speak_action(cu, cmd, msg))
      match = 1;
    else
      match = condition_action(cu, cmd);
  }
  else
  {
    char *str;

    common_client_command = 0;
    if (*cmd == '-')
    {
      if (cu->clitype)
      {
	cmd++;			/* Xshadow: 指令從下一個字元才開始 */
	common_client_command = 1;
      }
      else
      {
	/* 不是 common client 但送出 common client 指令 -> 假裝沒看到 */
      }
    }

    str_lower(buf, cmd);

    for (cmdrec = chatcmdlist; str = cmdrec->cmdstr; cmdrec++)
    {
      switch (cmdrec->exact)
      {
      case 1:			/* exactly equal */
	match = !str_cmp(str, buf);
	break;

      case 2:			/* Thor: secret command */
	if (CHATSYSOP(cu))
	  match = !str_cmp(str, buf);
	break;

      default:			/* not necessary equal */
	match = str_match(buf, str) >= 0;
	break;
      }

      if (match)
      {
	cmdrec->cmdfunc(cu, msg);
	break;
      }
    }
  }

  if (!match)
  {
    sprintf(buf, "◆ 指令錯誤：/%s", cmd);
    send_to_user(cu, buf, 0, MSG_MESSAGE);
  }

  return 0;
}


/* ----------------------------------------------------- */
/* serve chat_user's connection                          */
/* ----------------------------------------------------- */


static int
cuser_serve(cu)
  ChatUser *cu;
{
  int ch, len, isize;
  char *str, *cmd, buf[256];

  str = buf;
  len = recv(cu->sock, str, sizeof(buf) - 1, 0);
  if (len < 0)
  {
    ch = errno;

    exit_room(cu, EXIT_LOSTCONN, NULL);
    logit("recv", strerror(ch));
    return -1;
  }

  if (len == 0)
  {
    if (++cu->retry > 100)
      return -1;
    return 0;
  }

#if 0
  /* Xshadow: 將送達的資料忠實紀錄下來 */
  memcpy(logbuf, buf, sizeof(buf));
  for (ch = 0; ch < sizeof(buf); ch++)
  {
    if (!logbuf[ch])
      logbuf[ch] = '$';
  }

  logbuf[len + 1] = '\0';
  logit("recv: ", logbuf);
#endif

#if 0
  logit(cu->userid, str);
#endif

  cu->xdata += len;

  isize = cu->isize;
  cmd = cu->ibuf + isize;
  while (len--)
  {
    ch = *str++;

    if (ch == '\r' || !ch)
      continue;

    if (ch == '\n')
    {
      *cmd = '\0';

      if (command_execute(cu) < 0)
	return -1;

      isize = 0;
      cmd = cu->ibuf;

      continue;
    }

    if (isize < SCR_WIDTH)
    {
      *cmd++ = ch;
      isize++;
    }
  }
  cu->isize = isize;
  return 1;
}


/* ----------------------------------------------------- */
/* chatroom server core routines                         */
/* ----------------------------------------------------- */


static int
/* start_daemon(mode) 
  int mode; */
servo_daemon(inetd)
  int inetd;
{
  int fd, value;
  char buf[80];
  struct sockaddr_in sin;
  struct linger ld;
#ifdef HAVE_RLIMIT
  struct rlimit limit;
#endif

  /*
   * More idiot speed-hacking --- the first time conversion makes the C
   * library open the files containing the locale definition and time zone.
   * If this hasn't happened in the parent process, it happens in the
   * children, once per connection --- and it does add up.
   */

  time((time_t *) &value);
  gmtime((time_t *) &value);
  strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", localtime((time_t *) &value));

  /* --------------------------------------------------- */
  /* speed-hacking DNS resolve                           */
  /* --------------------------------------------------- */

  dns_init();
#if 0
  gethostname(buf, sizeof(buf));
  gethostbyname(buf);
#endif

#ifdef HAVE_RLIMIT
  /* --------------------------------------------------- */
  /* adjust the resource limit                           */
  /* --------------------------------------------------- */

  getrlimit(RLIMIT_NOFILE, &limit);
  limit.rlim_cur = limit.rlim_max;
  setrlimit(RLIMIT_NOFILE, &limit);

  limit.rlim_cur = limit.rlim_max = 4 * 1024 * 1024;
  setrlimit(RLIMIT_DATA, &limit);

#ifdef SOLARIS
#define RLIMIT_RSS RLIMIT_AS	/* Thor.981206: port for solaris 2.6 */
#endif  

  setrlimit(RLIMIT_RSS, &limit);

  limit.rlim_cur = limit.rlim_max = 0;
  setrlimit(RLIMIT_CORE, &limit);

#if 0
  limit.rlim_cur = limit.rlim_max = 60 * 20;
  setrlimit(RLIMIT_CPU, &limit);
#endif
#endif

  /* --------------------------------------------------- */
  /* detach daemon process                               */
  /* --------------------------------------------------- */

  close(2);
  close(1);

  /* if (mode > 1) */
  if (inetd)
    return 0;

  close(0);

  if (fork())
    exit(0);

  setsid();

  if (fork())
    exit(0);

  /* --------------------------------------------------- */
  /* bind the service port				 */
  /* --------------------------------------------------- */

  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  /*
   * timeout 方面, 將 socket 改成 O_NDELAY (no delay, non-blocking),
   * 如果能順利送出資料就送出, 不能送出就算了, 不再等待 TCP_TIMEOUT 時間。
   * (default 是 120 秒, 並且有 3-way handshaking 機制, 有可能一等再等)。
   */

#if 1
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NDELAY);
#endif

  value = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &value, sizeof(value));

  value = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &value, sizeof(value));

  ld.l_onoff = ld.l_linger = 0;
  setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld));

  sin.sin_family = AF_INET;
  sin.sin_port = htons(CHAT_PORT);
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(sin.sin_zero, 0, sizeof(sin.sin_zero));

  if ((bind(fd, (struct sockaddr *) & sin, sizeof(sin)) < 0) ||
    (listen(fd, SOCK_QLEN) < 0))
    exit(1);

  return fd;
}


#ifdef	SERVER_USAGE
static void
server_usage()
{
  struct rusage ru;

  if (getrusage(RUSAGE_SELF, &ru))
    return;

  fprintf(flog, "\n[Server Usage]\n\n"
    "user time: %.6f\n"
    "system time: %.6f\n"
    "maximum resident set size: %lu P\n"
    "integral resident set size: %lu\n"
    "page faults not requiring physical I/O: %ld\n"
    "page faults requiring physical I/O: %ld\n"
    "swaps: %ld\n"
    "block input operations: %ld\n"
    "block output operations: %ld\n"
    "messages sent: %ld\n"
    "messages received: %ld\n"
    "signals received: %ld\n"
    "voluntary context switches: %ld\n"
    "involuntary context switches: %ld\n"
    "gline: %d\n\n",

    (double) ru.ru_utime.tv_sec + (double) ru.ru_utime.tv_usec / 1000000.0,
    (double) ru.ru_stime.tv_sec + (double) ru.ru_stime.tv_usec / 1000000.0,
    ru.ru_maxrss,
    ru.ru_idrss,
    ru.ru_minflt,
    ru.ru_majflt,
    ru.ru_nswap,
    ru.ru_inblock,
    ru.ru_oublock,
    ru.ru_msgsnd,
    ru.ru_msgrcv,
    ru.ru_nsignals,
    ru.ru_nvcsw,
    ru.ru_nivcsw,
    gline);

  fflush(flog);
}
#endif


static void
reaper()
{
  while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0)
    ;
}


static void
sig_trap(sig)
  int sig;
{
  char buf[80];

  sprintf(buf, "signal [%d] at line %d (errno: %d)", sig, gline, errno);
  logit("EXIT", buf);
  fclose(flog);
  exit(1);
}


static void
sig_over()
{
  int fd;

  server_usage();
  logit("OVER", "");
  fclose(flog);
  for (fd = 0; fd < 64; fd++)
    close(fd);
  execl("bin/xchatd", NULL);
}


static void
main_signals()
{
  struct sigaction act;

  /* sigblock(sigmask(SIGPIPE)); */
  /* Thor.981206: 統一 POSIX 標準用法  */

  /* act.sa_mask = 0; */ /* Thor.981105: 標準用法 */
  sigemptyset(&act.sa_mask);      
  act.sa_flags = 0;

  act.sa_handler = sig_trap;
  sigaction(SIGBUS, &act, NULL);
  sigaction(SIGSEGV, &act, NULL);
  sigaction(SIGTERM, &act, NULL);

  act.sa_handler = sig_over;
  sigaction(SIGXCPU, &act, NULL);

  act.sa_handler = reaper;
  sigaction(SIGCHLD, &act, NULL);

#ifdef  SERVER_USAGE
  act.sa_handler = server_usage;
  sigaction(SIGPROF, &act, NULL);
#endif

  /* Thor.981206: lkchu patch: 統一 POSIX 標準用法  */
  /* 在此借用 sigset_t act.sa_mask */
  sigaddset(&act.sa_mask, SIGPIPE);
  sigprocmask(SIG_BLOCK, &act.sa_mask, NULL);

}


int
main(argc, argv)
  int argc;
  char *argv[];
{
  int sock, nfds, maxfds, servo_sno;
  ChatUser *cu,/* *userpool,*/ **FBI;
  time_t uptime, tcheck;
  fd_set rset, xset;
  static struct timeval tv = {CHAT_INTERVAL, 0};
  struct timeval tv_tmp; /* Thor.981206: for future reservation bug */

  sock = 0;

  while ((nfds = getopt(argc, argv, "hid")) != -1)
  {
    switch (nfds)
    {
    case 'i':
      sock = 1;
      break;

    case 'd':
      break;

    case 'h':
    default:

      fprintf(stderr, "Usage: %s [options]\n"
        "\t-i  start from inetd with wait option\n"
        "\t-d  debug mode\n"
        "\t-h  help\n",
        argv[0]);
      exit(0);
    }       
  }

  servo_daemon(sock); 
  /* start_daemon(argc); */

  setgid(BBSGID);
  setuid(BBSUID);
  chdir(BBSHOME);
  umask(077);

  log_init();

  main_signals();

  /* --------------------------------------------------- */
  /* init variable : rooms & users			 */
  /* --------------------------------------------------- */

  userpool = NULL;
  strcpy(mainroom.name, MAIN_NAME);
  strcpy(mainroom.topic, MAIN_TOPIC);

  /* --------------------------------------------------- */
  /* main loop						 */
  /* --------------------------------------------------- */

  tcheck = 0;
  servo_sno = 0;

  for (;;)
  {
    uptime = time(0);
    if (tcheck < uptime)
    {
      nfds = maxfds = 0;
      FD_ZERO(&mainfset);
      FD_SET(0, &mainfset);

      tcheck = uptime - CHAT_INTERVAL;

      for (FBI = &mainuser; cu = *FBI;)
      {
	if (cu->uptime < tcheck)
	{
	  cuser_free(cu);

	  *FBI = cu->unext;

	  cu->unext = userpool;
	  userpool = cu;
	}
	else
	{
	  nfds++;
	  sock = cu->sock;
	  FD_SET(sock, &mainfset);
	  if (maxfds < sock)
	    maxfds = sock;

	  FBI = &(cu->unext);
	}
      }

      totaluser = nfds;
      fprintf(flog, "MAINTAIN %d user (%d)\n", nfds, maxfds++);
      fflush(flog);

      tcheck = uptime + CHAT_INTERVAL;
    }

    /* ------------------------------------------------- */
    /* Set up the fdsets				 */
    /* ------------------------------------------------- */

    rset = mainfset;
    xset = mainfset;

    /* Thor.981206: for future reservation bug */   
    tv_tmp = tv;
    nfds = select(maxfds, &rset, NULL, &xset, &tv_tmp);

#if 0
    {
      char buf[32];
      static int xxx;

      if ((++xxx & 8191) == 0)
      {
	sprintf(buf, "%d/%d", nfds, maxfds);
	logit("MAIN", buf);
      }
    }
#endif

    if (nfds == 0)
    {
      continue;
    }

    if (nfds < 0)
    {
      sock = errno;
      if (sock != EINTR)
      {
	logit("select", strerror(sock));
      }
      continue;
    }

    /* ------------------------------------------------- */
    /* serve active agents				 */
    /* ------------------------------------------------- */

    uptime = time(0);

    for (FBI = &mainuser; cu = *FBI;)
    {
      sock = cu->sock;

      if (FD_ISSET(sock, &rset))
      {
	static int xxx, xno;

	nfds = cuser_serve(cu);

	if ((++xxx & 511) == 0)
	{
	  int sno;

	  sno = cu->sno;
	  fprintf(flog, "rset\t[%d] S%d R%d %d\n", sno, sock, nfds, xxx);
	  if (sno == xno)
	    nfds = -1;
	  else
	    xno = sno;
	}
      }
      else if (FD_ISSET(sock, &xset))
      {
	nfds = -1;
      }
      else
      {
	nfds = 0;
      }

      if (nfds < 0 || cu->uptime <= 0)	/* free this client */
      {
	cuser_free(cu);

	*FBI = cu->unext;

	cu->unext = userpool;
	userpool = cu;

	continue;
      }

      if (nfds > 0)
      {
	cu->uptime = uptime;
      }

      FBI = &(cu->unext);
    }

    /* ------------------------------------------------- */
    /* accept new connection				 */
    /* ------------------------------------------------- */

    if (FD_ISSET(0, &rset))
    {

      {
	static int yyy;

	if ((++yyy & 2047) == 0)
	  fprintf(flog, "conn\t%d\n", yyy);
      }

      for (;;)
      {
	int value;
	struct sockaddr_in sin;

	value = sizeof(sin);
	sock = accept(0, (struct sockaddr *) &sin, &value);
	if (sock > 0)
	{
	  if (cu = userpool)
	  {
	    userpool = cu->unext;
	  }
	  else
	  {
	    cu = (ChatUser *) malloc(sizeof(ChatUser));
	  }

	  *FBI = cu;

	  /* variable initialization */

	  memset(cu, 0, sizeof(ChatUser));
	  cu->sock = sock;
	  cu->tbegin = uptime;
	  cu->uptime = uptime;
	  cu->sno = ++servo_sno;
	  cu->xdata = 0;
	  cu->retry = 0;
	  memcpy(cu->rhost, &sin.sin_addr, sizeof(struct in_addr));

	  totaluser++;

	  FD_SET(sock, &mainfset);
	  if (sock >= maxfds)
	    maxfds = sock + 1;

	  {
	    int value;

	    value = 1;
	    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
	      (char *) &value, sizeof(value));
	  }

#if 1
	  fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NDELAY);
#endif

	  fprintf(flog, "CONN\t[%d] %d %s\n",
	    servo_sno, sock, Btime(&cu->tbegin));
	  break;
	}

	nfds = errno;
	if (nfds != EINTR)
	{
	  logit("accept", strerror(nfds));
	  break;
	}

#if 0
	while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
#endif
      }
    }

    /* ------------------------------------------------- */
    /* tail of main loop				 */
    /* ------------------------------------------------- */

  }
}
