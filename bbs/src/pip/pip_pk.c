/* ----------------------------------------------------- */
/* pip_pk.c	( NTHU CS MapleBBS Ver 3.10 )      	 */
/* ----------------------------------------------------- */
/* target : PK 對戰選單                                  */
/* create : 02/02/17                                     */
/* update :   /  /		  			 */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 */
/* ----------------------------------------------------- */


#include "bbs.h"

#ifdef HAVE_GAME

#include "pip.h"


#if 0

  0. 玩法是進入電子雞遊戲以後選 PK，然後一人選 1)蓄勢待發，
     另一人選 2)下挑戰書輸入前者的 ID 即可開始對戰。

  1. PK 的時候用的值是 ptmp-> 裡面的，所以對戰完 d. 並不會改變。
  2. 為增加趣味，對戰的攻擊方式較 pip_fight.c 為多樣。
  3. 對戰時不可吃補品。 (可改)
  4. 對戰時 skillXYZ 沒有特別效用。 (可改)

  5. pip_pk_skill() 也是偷懶做法，想改的人再自己從 pip_fight.c 抄過來。

#endif

/*-------------------------------------------------------*/
/* pip PK cache                                          */
/*-------------------------------------------------------*/


static PCACHE *pshm;
static PTMP *cp;		/* 我的小雞 */
static PTMP *ep;		/* 對手的小雞 */


static void
pip_pkshm_init()
{
  pshm = shm_new(PIPSHM_KEY, sizeof(PCACHE));
}


static int
pip_ptmp_new(pp)
  PTMP *pp;
{
  PTMP *pentp, *ptail;

  /* --------------------------------------------------- */
  /* semaphore : critical section                        */
  /* --------------------------------------------------- */

#ifdef	HAVE_SEM
  sem_lock(BSEM_ENTER);
#endif

  pentp = pshm->pslot;
  ptail = pentp + MAX_PIPPK_USER;

  do
  {
    if (!pentp->inuse)		/* 找到一個空位子，cp-> 指向 */
    {
      memcpy(pentp, pp, sizeof(PTMP));
      cp = pentp;

#ifdef	HAVE_SEM
      sem_lock(BSEM_LEAVE);
#endif

      return 1;
    }
  } while (++pentp < ptail);

  /* Thor:告訴user有人登先一步了 */

#ifdef	HAVE_SEM
  sem_lock(BSEM_LEAVE);
#endif

  return 0;
}


static int
pip_ptmp_setup()
{
  PTMP ptmp;

  memset(&ptmp, 0, sizeof(PTMP));

  /* 基本屬性 */
  strcpy(ptmp.name, d.name);
  strcpy(ptmp.userid, cuser.userid);

  ptmp.sex = d.sex;
  ptmp.level = d.level;

  /* 血補滿 */
  ptmp.hp = d.maxhp;
  ptmp.mp = d.maxmp;
  ptmp.vp = d.maxvp;
  ptmp.sp = d.maxsp;
  ptmp.maxhp = d.maxhp;
  ptmp.maxmp = d.maxmp;
  ptmp.maxvp = d.maxvp;
  ptmp.maxsp = d.maxsp;

  /* 各種攻擊能力 */
  ptmp.combat = d.attack + (d.resist >> 2);	/* 物理身段: 決定「肉搏、防禦」的強度 */
  ptmp.magic = d.immune + (d.mskill >> 2);	/* 魔法造詣: 決定「法術-各系」的強度 */
  ptmp.speed = d.speed + (d.hskill >> 2);	/* 敏捷技巧: 決定「技能-護身、技能-輕功、技能-劍法」的強度 */
  ptmp.spirit = d.brave + (d.etchics >> 2);	/* 內力強度: 決定「技能-心法、技能-拳法、技能-刀法」的強度 */
  ptmp.charm = d.charm + (d.art >> 2);		/* 動感魅力: 決定「魅惑、召喚」的強度 */
  ptmp.oral = d.speech + (d.manners >> 2);	/* 口若懸河: 決定「說服、煽動」的強度 */
  ptmp.cook = d.cook + (d.homework >> 2);	/* 美味烹調: 決定「技能-暗器、法術-治療」 */

  if (!pip_ptmp_new(&ptmp))
  {
    vmsg("抱歉，PK 場客滿了喔");
    return 0;
  }

  return 1;
}


static void
pip_ptmp_free()
{
  if (!cp || !cp->inuse)
    return;

#ifdef  HAVE_SEM
  sem_lock(BSEM_ENTER);
#endif

  cp->inuse = 0;

#ifdef  HAVE_SEM
  sem_lock(BSEM_LEAVE);
#endif
}


static PTMP *
pip_ptmp_get(userid, inuse)
  char *userid;
  int inuse;		/* 1:找「蓄勢待發」的人來挑戰  2:找「下挑戰書」的挑戰者回應 */
{
  PTMP *pentp, *ptail;

  pentp = pshm->pslot;
  ptail = pentp + MAX_PIPPK_USER;
  do
  {
    if (pentp->inuse == inuse && !str_cmp(pentp->userid, userid))
      return pentp;
  } while (++pentp < ptail);

  return NULL;
}


static void
pip_ptmp_show()
{
  int max;
  PTMP *pentp, *ptail;

  clrfromto(7, 16);
  move(10, 5);

  max = 0;
  pentp = pshm->pslot;
  ptail = pentp + MAX_PIPPK_USER;
  do
  {
    if (pentp->inuse)
    {
      max++;
      prints("\033[1;3%dm%-20s\033[m", 2 + pentp->inuse, pentp->userid);

      if (max % 4 == 0)
        move(10 + max % 4, 5);
    }
  } while (++pentp < ptail);

  move(8, 5);
  prints("\033[1;31m戰鬥中  \033[33m蓄勢待發  \033[34m挑戰者等待回應\033[m"
    "  目前場子裡有 \033[1;36m%d/%d\033[m 隻雞", max, MAX_PIPPK_USER);
}


/*-------------------------------------------------------*/
/* 對戰主函式                                            */
/*-------------------------------------------------------*/


  /*-----------------------------------------------------*/
  /* 輪流控制                                            */
  /*-----------------------------------------------------*/


static void
pip_pk_turn()	/* 換對方 */
{
  cp->done = 1;
  ep->done = 0;
}


  /*-----------------------------------------------------*/
  /* 畫面顯示                                            */
  /*-----------------------------------------------------*/


static void
pip_pk_showfoot()
{
  out_cmd("", COLOR1 " 戰鬥命令 " COLOR2 " [1]肉搏 [2]技能 [3]魅惑 [4]召喚 [5]說服 [6]煽動 [Q]認輸            \033[m");
}


static void
pip_pk_showing()
{
  int pic;
  char inbuf1[20], inbuf2[20], inbuf3[20], inbuf4[20];
  
  clear();
  move(0, 0);

  prints("\033[1;41m  " BBSNAME PIPNAME " ～\033[32m%s\033[37m%-13s                                            \033[m\n", 
    cp->sex == 1 ? "♂" : (cp->sex == 2 ? "♀" : "？"), cp->name);

  /* 螢幕上方秀出我的小雞資料 */

  sprintf(inbuf1, "%d%s/%d%s", cp->hp > 1000 ? cp->hp / 1000 : cp->hp,
    cp->hp > 1000 ? "K" : "", cp->maxhp > 1000 ? cp->maxhp / 1000 : cp->maxhp,
    cp->maxhp > 1000 ? "K" : "");
  sprintf(inbuf2, "%d%s/%d%s", cp->mp > 1000 ? cp->mp / 1000 : cp->mp,
    cp->mp > 1000 ? "K" : "", cp->maxmp > 1000 ? cp->maxmp / 1000 : cp->maxmp,
    cp->maxmp > 1000 ? "K" : "");
  sprintf(inbuf3, "%d%s/%d%s", cp->vp > 1000 ? cp->vp / 1000 : cp->vp,
    cp->vp > 1000 ? "K" : "", cp->maxvp > 1000 ? cp->maxvp / 1000 : cp->maxvp,
    cp->maxvp > 1000 ? "K" : "");
  sprintf(inbuf4, "%d%s/%d%s", cp->sp > 1000 ? cp->sp / 1000 : cp->sp,
    cp->sp > 1000 ? "K" : "", cp->maxsp > 1000 ? cp->maxsp / 1000 : cp->maxsp,
    cp->maxsp > 1000 ? "K" : "");

  outs("\033[1;31m┌──────────────────────────────────────┐\033[m\n");
  prints("\033[1;31m│\033[33m生  命:\033[37m%-12s\033[33m法  力:\033[37m%-12s\033[33m移動力:\033[37m%-12s\033[33m內  力:\033[37m%-12s\033[31m│\033[m\n", inbuf1, inbuf2, inbuf3, inbuf4);
  prints("\033[1;31m│\033[33m攻  擊:\033[37m%-12d\033[33m魔  法:\033[37m%-12d\033[33m敏  捷:\033[37m%-12d\033[33m武  術:\033[37m%-12d\033[31m│\033[m\n", cp->combat, cp->magic, cp->speed, cp->spirit);
  prints("\033[1;31m│\033[33m魅  力:\033[37m%-12d\033[33m口  才:\033[37m%-12d\033[33m烹  調:\033[37m%-12d\033[33m等  級:\033[37m%-12d\033[31m│\033[m\n", cp->charm, cp->oral, cp->cook, cp->level);
  outs("\033[1;31m└──────────────────────────────────────┘\033[m");

  /* 螢幕中間 7~16 列 秀出怪物的圖檔 */
  pic = 101 + 100 * (rand() % 5) + rand() % 3;    /* 101~501 102~502 103~503 十五選一 */
  show_badman_pic(pic);

  /* 螢幕下方秀出對方的小雞資料 */

  sprintf(inbuf1, "%d%s/%d%s", ep->hp > 1000 ? ep->hp / 1000 : ep->hp,
    ep->hp > 1000 ? "K" : "", ep->maxhp > 1000 ? ep->maxhp / 1000 : ep->maxhp,
    ep->maxhp > 1000 ? "K" : "");
  sprintf(inbuf2, "%d%s/%d%s", ep->mp > 1000 ? ep->mp / 1000 : ep->mp,
    ep->mp > 1000 ? "K" : "", ep->maxmp > 1000 ? ep->maxmp / 1000 : ep->maxmp,
    ep->maxmp > 1000 ? "K" : "");
  sprintf(inbuf3, "%d%s/%d%s", ep->vp > 1000 ? ep->vp / 1000 : ep->vp,
    ep->vp > 1000 ? "K" : "", ep->maxvp > 1000 ? ep->maxvp / 1000 : ep->maxvp,
    ep->maxvp > 1000 ? "K" : "");
  sprintf(inbuf4, "%d%s/%d%s", ep->sp > 1000 ? ep->sp / 1000 : ep->sp,
    ep->sp > 1000 ? "K" : "", ep->maxsp > 1000 ? ep->maxsp / 1000 : ep->maxsp,
    ep->maxsp > 1000 ? "K" : "");

  move(18, 0);
  outs("\033[1;34m┌──────────────────────────────────────┐\033[m\n");
  prints("\033[1;34m│\033[32m姓  名:\033[37m%-12s\033[32mＩ  Ｄ:\033[37m%-12s\033[32m性  別:\033[37m%-12s\033[32m等  級:\033[37m%-12d\033[34m│\033[m\n", ep->name, ep->userid, ep->sex == 1 ? "♂" : (ep->sex == 2 ? "♀" : "？"), ep->level);
  prints("\033[1;34m│\033[32m生  命:\033[37m%-12s\033[32m法  力:\033[37m%-12s\033[32m移動力:\033[37m%-12s\033[32m內  力:\033[37m%-12s\033[34m│\033[m\n", inbuf1, inbuf2, inbuf3, inbuf4);
  outs("\033[1;34m└──────────────────────────────────────┘\033[m\n");

  pip_pk_showfoot(); 
}


static void
pip_pk_ending()
{
  clrfromto(7, 16);
  move(8, 0);

  if (cp->hp > 0)
  {
    outs("           \033[1;31m┌──────────────────────┐\033[m\n");
    prints("           \033[1;31m│ \033[37m武術大會的小雞\033[33m%-13s                \033[31m│\033[m\n", cp->name);
    prints("           \033[1;31m│ \033[37m打敗了強勁的對手\033[32m%-13s              \033[31m│\033[m\n", ep->name);
    outs("           \033[1;31m│ \033[37m勇敢和經驗都上升了不少                     \033[31m│\033[m\n");
    outs("           \033[1;31m└──────────────────────┘\033[m");
    vmsg("您打敗了一個強硬的傢伙");
    d.exp += ep->level;
  }
  else
  {
    outs("           \033[1;31m┌──────────────────────┐\033[m\n");
    prints("           \033[1;31m│ \033[37m武術大會的小雞\033[33m%-13s                \033[31m│\033[m\n", cp->name);
    prints("           \033[1;31m│ \033[37m被\033[32m%-13s\033[37m對手打得落花流水            \033[31m│\033[m\n", ep->name);
    outs("           \033[1;31m│ \033[37m決定回家好好再鍛練                         \033[31m│\033[m\n");
    outs("           \033[1;31m└──────────────────────┘\033[m");
    vmsg("被打敗的您心中相當不是味道");
    d.exp -= cp->level;
  }
}


  /*-----------------------------------------------------*/
  /* 行動函式                                            */
  /*-----------------------------------------------------*/


static void
pip_pk_combat()		/* 肉搏 */
{
  int injure;

  injure = cp->combat - (ep->combat >> 2);
  if (injure > 0)
  {
    ep->hp -= injure;
    show_fight_pic(1);
    vmsg("您造成了對方的傷害");
  }
  else
  {
    show_fight_pic(2);
    vmsg("您的攻擊在對方眼裡簡直是搔癢");
  }
  pip_pk_turn();  
}


static void 
pip_pk_skill()		/* 技能: 武功/魔法 */
{
  /* itoc.020217: 懶得寫像 pip_fight.c 裡面那種的了，有興趣的人自己抄著改 :p */

  /* 技能不扣點，但效果比較差(只有其他的對半) */

  int ch, class;
  int injure[5] = {125, 200, 300, 450, 750};

  out_cmd(COLOR1 " 武功\選單 " COLOR2 " [1]護身 [2]輕功\ [3]心法 [4]拳法 [5]劍法 [6]刀法 [7]暗器 [Q]放棄    \033[m",
    COLOR1 " 法術選單 " COLOR2 " [A]治療 [B]雷系 [C]冰系 [D]炎系 [E]土系 [F]風系 [G]特殊 [Q]放棄    \033[m");

  for (;;)
  {
    ch = vkey();

    if (ch == 'q')
    {
      pip_pk_showfoot();
      return;
    }

    else if (ch == '1')		/* 護身 */
    {
      class = rand() % 10;
      if (class == 0)		/* 10% 的機率反彈攻擊，可再度攻擊 */
      {
        ep->hp -= cp->speed >> 3;
        vmsg("對手的攻擊反彈回他的身上");
      }
      else if (class <= 2)	/* 20% 的機率使對方物理攻擊永遠降低，可再度攻擊 */
      {
        ep->combat = ep->combat * 4 / 5;
        vmsg("對方的手扭到了，看來短時間內不會恢復");
      }
      else			/* 70% 的機率什麼都沒做 */
      {
        vmsg("您採取防禦措施");
        break;
      }
      pip_pk_showfoot();
      return;
    }

    else if (ch == '2')		/* 輕功 */
    {
      class = cp->speed >> 9;
      if (class > 4)
        class = 4;

      cp->vp += injure[class];	/* 補 vp 也借用 injure[] */      
      if (cp->vp > cp->maxvp)
        cp->vp = cp->maxvp;

      vmsg("精神飽滿，準備再戰");
      break;
    }

    else if (ch == '3')		/* 心法 */
    {
      class = cp->spirit >> 9;
      if (class > 4)
        class = 4;

      cp->sp += injure[class];	/* 補 sp 也借用 injure[] */      
      if (cp->sp > cp->maxsp)
        cp->sp = cp->maxsp;

      vmsg("活力充沛，準備再戰");
      break;
    }

    else if (ch == '4')		/* 拳法 */
    {
      class = cp->spirit >> 9;
      if (class > 4)
        class = 4;

      ep->hp -= injure[class] * (75 + rand() % 50) / 100;
      vmsg("全身精力集中於掌上，奮力一擊");
      break;
    }

    else if (ch == '5')		/* 劍法 */
    {
      class = cp->speed >> 9;
      if (class > 4)
        class = 4;

      ep->hp -= injure[class] * (50 + rand() % 100) / 100;
      vmsg("快劍斬亂麻，神劍闖江湖");
      break;
    }

    else if (ch == '6')		/* 刀法 */
    {
      class = cp->spirit >> 9;
      if (class > 4)
        class = 4;

      ep->hp -= injure[class] * (30 + rand() % 140) / 100;
      vmsg("全身精力集中於掌上，奮力一擊");
      break;
    }

    else if (ch == '7')		/* 暗器 */
    {
      class = cp->cook >> 9;
      if (class > 4)
        class = 4;

      ep->hp -= injure[class] * (80 + rand() % 40) / 100;
      vmsg("您的菜餚中下了毒藥，對手不注意就吃了下去");
      break;
    }

    else if (ch == 'a')		/* 治療 */
    {
      class = cp->cook >> 10;	/* 補 hp 的門檻比較高 */
      if (class > 4)
        class = 4;

      cp->hp += injure[class];	/* 補 hp 也借用 injure[] */      
      if (cp->hp > cp->maxhp)
        cp->hp = cp->maxhp;

      vmsg("充電以後，再度出發");
      break;
    }

    else if (ch >= 'b' && ch <= 'f')		/* 各系法術 */
    {
      char buf[64];
      char name[5][3] = {"雷", "冰", "炎", "土", "風"};

      class = cp->magic >> 9;
      if (class > 4)
        class = 4;

      ep->hp -= injure[class] * (50 + rand() % 100) / 100;
      sprintf(buf, "您施展了%s系法術，威力十足", name[ch - 'b']);
      vmsg(buf);
      break;
    }

    else if (ch == 'g')		/* 特殊 */
    {
      class = cp->magic >> 9;
      if (class > 4)
        class = 4;

      cp->mp += injure[class];	/* 補 mp 也借用 injure[] */      
      if (cp->mp > cp->maxmp)
        cp->mp = cp->maxmp;

      vmsg("能量充填，魔力再現");
      break;
    }
  }

  pip_pk_turn();
}


static void
pip_pk_charm()		/* 魅惑: 耗 hp */
{
  int class;
  char buf[80];
  char name[5][9] = {"凡夫俗子", "色情狂", "小鬼", "龍神", "站長"};
  int injure[5] = {250, 400, 600, 900, 1500};
  int needhp[5] = {350, 600, 900, 1500, 3200};

  class = cp->charm >> 8;	/* 相對於以下三者，其所需門檻比較低就可以施展強大魅惑術，但耗的是 hp */
  if (class > 4)
    class = 4;

  if (cp->hp >= needhp[class])
  {
    cp->hp -= needhp[class];
    ep->hp -= injure[class] * (75 + rand() % 50) / 100;
    sprintf(buf, "一群%s在您的指使之下，拼命攻擊對方", name[class]);
    vmsg(buf);
    pip_pk_turn();
  }
  else
  {
    vmsg("您全身都是傷口，還想魅惑誰");
    pip_pk_showfoot();  
  }
}


static void
pip_pk_summon()		/* 召喚: 耗 mp */
{
  int class;
  char buf[80];
  char name[5][9] = {"史萊姆", "虎頭蜂", "猛虎", "遠古巨龍", "死神撒旦"};
  int injure[5] = {250, 400, 600, 900, 1500};
  int needmp[5] = {350, 600, 900, 1500, 3200};

  class = cp->charm >> 9;
  if (class > 4)
    class = 4;

  if (cp->mp >= needmp[class])
  {
    cp->mp -= needmp[class];
    ep->hp -= injure[class] * (75 + rand() % 50) / 100;
    sprintf(buf, "您召喚出%s，重重地給了對方一擊", name[class]);
    vmsg(buf);
    pip_pk_turn();
  }
  else
  {
    vmsg("您感到全身疲憊，什麼也召喚不出來");
    pip_pk_showfoot();  
  }
}


static void
pip_pk_convince()	/* 說服: 耗 vp */
{
  int class;
  char buf[80];
  char name[5][9] = {"面具怪人", "骷髏\頭怪", "炸蛋超人", "東海龍王", "齊天大聖"};
  int injure[5] = {250, 400, 600, 900, 1500};
  int needvp[5] = {350, 600, 900, 1500, 3200};

  class = cp->oral >> 9;
  if (class > 4)
    class = 4;

  if (cp->vp >= needvp[class])
  {
    cp->vp -= needvp[class];
    ep->hp -= injure[class] * (75 + rand() % 50) / 100;
    sprintf(buf, "您成功\地說服%s來助您一臂之力", name[class]);
    vmsg(buf);
    pip_pk_turn();
  }
  else
  {
    vmsg("遠方傳來一陣聲音：想說服我，再等一百年吧");
    pip_pk_showfoot();  
  }
}


static void
pip_pk_incite()		/* 煽動: 耗 sp */
{
  int class;
  char buf[80];
  char name[5][9] = {"巨蠅怪", "布耶魯", "地獄犬", "噴火龍", "熾天使"};
  int injure[5] = {250, 400, 600, 900, 1500};
  int needsp[5] = {350, 600, 900, 1500, 3200};

  class = cp->oral >> 9;
  if (class > 4)
    class = 4;

  if (cp->sp >= needsp[class])
  {
    cp->sp -= needsp[class];
    ep->hp -= injure[class] * (40 + rand() % 120) / 100;	/* 變異性較前面三者為高 */
    sprintf(buf, "您勇敢地煽動%s整個族群來對付敵人", name[class]);
    vmsg(buf);
    pip_pk_turn();
  }
  else
  {
    vmsg("眾人不為所動，您的計畫失敗了");
    pip_pk_showfoot();  
  }
}


static void
pip_pk_man()		/* 輪到我下指令 */
{
  /* 秀出戰鬥主畫面 */
  pip_pk_showing();

  while (!cp->done)
  {
    switch (vkey())
    {
    case '1':		/* 肉搏 */
      pip_pk_combat();
      break;

    case '2':		/* 技能: 武功/魔法 */
      pip_pk_skill();
      break;

    case '3':		/* 魅惑 */
      pip_pk_charm();
      break;

    case '4':		/* 召喚 */
      pip_pk_summon();
      break;

    case '5':		/* 說服 */
      pip_pk_convince();
      break;

    case '6':		/* 煽動 */
      pip_pk_incite(); 
      break;

    case 'q':		/* 認輸 */
      cp->hp = 0;
      pip_pk_turn();
      break;
    }
  }
}


static void
pip_pk_wait()		/* 等待對方下指令 */
{
  int fd;
  struct timeval tv = {1, 100};

  /* 秀出戰鬥主畫面 */
  pip_pk_showing();

  outz("等待對方的攻擊 Q)認輸");
  refresh();

  while (!ep->done)
  {
    fd = 1;
    if (select(1, (fd_set *) &fd, NULL, NULL, &tv) > 0)
    {
      if (vkey() == 'q')
      {
	cp->done = 1;	/* 二人都強迫結束行動 */
	ep->done = 1;      
	cp->hp = 0;	/* 有事離開算輸 */
	break;
      }
    }
  }
}


/*-------------------------------------------------------*/
/* 對戰主選單                                            */
/*-------------------------------------------------------*/


int 
pip_pk_menu()
{
  int ch;
  char userid[IDLEN + 1];
  struct timeval tv = {1, 100};

  /* itoc.020327: 有個蠻大的問題是，如果對戰到一半，其中一人斷線了，
     另外一人會進入迴圈，只能按 Q 離開 */

  if (d.hp <= 0)
    return XEASY;

  pip_pkshm_init();

  pip_ptmp_show();

  ch = ians(b_lines, 0, "◎ 小雞對戰 1)蓄勢待發 2)下挑戰書 [Q]離開 ");
  if (ch == '1')
  {
    /* 設定 cp-> */
    if (!pip_ptmp_setup())
      return XEASY;

    cp->inuse = 1;

    outz("等候挑戰中 Q)離開");
    refresh();
    do
    {
      ch = 1;
      if (select(1, (fd_set *) &ch, NULL, NULL, &tv) > 0)
      {
	if (vkey() == 'q')
	{
	  pip_ptmp_free();
	  return XEASY;
	}
      }
    } while (!*cp->mateid);

    if (!(ep = pip_ptmp_get(cp->mateid, 2)))
    {
      pip_ptmp_free();
      return XEASY;
    }
    cp->done = 0;	/* 被挑戰者先行動，挑戰者就會通知他 */
    cp->inuse = -1;	/* 兩方都進入戰鬥 */
    ep->inuse = -1;
  }
  else if (ch == '2')
  {
    /* 決定 PK 對戰對象 ep-> */
    if (!vget(b_lines, 0, msg_uid, userid, IDLEN + 1, DOECHO) || 
      !str_cmp(cuser.userid, userid) ||
      !(ep = pip_ptmp_get(userid, 1)))
    {
      return XEASY;
    }

    /* 設定 cp-> */
    if (!pip_ptmp_setup())
      return XEASY;

    strcpy(cp->mateid, userid);
    strcpy(ep->mateid, cuser.userid);
    cp->done = 1;	/* 挑戰者後行動，就會主動通知被挑戰者 */
    cp->inuse = 2;
  }
  else
  {
    return XEASY;
  }

  ch = d.tired;

  for (;;)		/* 雙方攻防 */
  {  
    pip_pk_man();
    if (ep->hp <= 0 || cp->hp <= 0)
      break;          

    pip_pk_wait();
    if (cp->hp <= 0 || ep->hp <= 0)
      break;
  }

  /* 結果判斷 */
  pip_pk_ending();

  pip_ptmp_free();

  d.tired = ch;		/* 以免 PK 打太久，PK 結束後來因 tired 過高死掉了 */
  return 0;
}
#endif	/* HAVE_GAME */
