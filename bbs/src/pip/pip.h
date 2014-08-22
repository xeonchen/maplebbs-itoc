/* ----------------------------------------------------- */
/* pip.h     ( NTHU CS MapleBBS Ver 3.10 )               */
/* ----------------------------------------------------- */
/* target : 小雞 data structure                          */
/* create : 01/08/16                                     */
/* update :   /  /                                       */
/* author : itoc.bbs@bbs.tnfsh.tn.edu.tw                 */
/* ----------------------------------------------------- */


#ifndef	_PIP_H_
#define	_PIP_H_


#if 0	/* 版權宣告 */

  根據我的考證，電子雞的最前身是由 [天長地久 dsyan] 所撰寫，
  接著在 [雲鳥天堂 fennet] 手中做了一些變動，
  後來在 [星空下的夜裡 chiyuan] 大改版，形成了星空戰鬥雞。

  其後 [風之塔 visor] 將這程式 port 到 WindTopBBS 來使用，
  目前 [與南共舞 itoc] 在以這版程式為基礎下，做了大幅度的改版。

  各檔案已經被我完整地改過了，在某些方面做了一些最佳化，
  包括程式的重新撰寫、新 idea 的加入等等，比較重要的變動說明於下：

  1) 將一個近萬行的程式模組化，拆散到各 *.c 中，以後在做修改時能更方便。
  2) 將各程式以 indent 排版，力求系統維護者閱讀程式的便利。
  3) struct 做變動，加入一些新的欄位。
  4) 戰鬥/修行加減屬性公式之變動。
  5) 撰寫怪獸產生器的程式。
  6) 撰寫地圖產生器的程式。
  7) 重新創造新的技能架構。
  8) 新增一些亂數事件，像是學到技能或是遇到站長等。
  9) 新增任務架構，升級要解任務。
 10) 改善武器系統，讓每個人的武器多樣化。
 11) 統一所有的用字及畫面處理。
 12) 加入大量的註解。
 13) 大幅度減少不必要的重繪。
 14) 其他..
  
  希望這些努力，能給您帶來許多便利，如果有什麼意見，也歡迎來信指教。

        台南一中 與南共舞  itoc.bbs@bbs.tnfsh.tn.edu.tw  2001.08.16

#endif

/* include 檔均命名為 pipxxx.h   C 檔均命名為 pip_xxx.c */
#include "pipglobal.h"
#include "pipstruct.h"


#define PIP_PICHOME	"etc/game/pip/"


#define LEARN_LEVEL	((d.happy+d.satisfy)/100)	/* 學習效果與快樂及滿足成正比 */


/* itoc.010801: 秀出最後二列的指令列 */
#define	out_cmd(cmd_1,cmd_2)	{ move(b_lines - 1, 0); clrtoeol(); outs(cmd_1); \
				move(b_lines, 0); clrtoeol(); outs(cmd_2); }

#endif		/* _PIP_H_ */
