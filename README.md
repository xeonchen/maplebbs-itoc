註: 為保持目錄架構完整使程式順利執行, 故在裡面的空目錄都放了 `.gitkeep` 空檔案, `git clone`下來並複製到 BBS 家目錄後, 若欲刪除之, 建議再自行使用以下指令：

```
$ find /home/bbs -name ".gitkeep" -exec rm {} ";"
```

# 快速安裝手冊

本文件是寫給要安裝 itoc 所維護的 bbs 程式版本所使用的，並不適用其他 bbs 版本。


# 一、安裝作業系統

本程式已在 FreeBSD 及 Linux 測試過可以正常運作，其他系統則沒有試過，所以不清楚。

安裝作業系統時就像平常安裝一樣，沒什麼特別要注意的，唯一要提醒您的是，請安裝 sed awk make gcc 等程式，因為 bbs 會用到。

# 二、下載 BBS 程式

在 http://processor.tfcis.org/~itoc 可以找到最新的程式版本，應該長得像 MapleBBS-3.10-20yymmdd-PACK.tgz 這樣的檔名。

# 三、建立 BBS 帳號

以 root 身分登入。

    -root- # mkdir /home/bbs
    -root- # vipw

如果您是 FreeBSD 的話，在最後一行加上

    bbs:*:9999:99::0:0:BBS Administrator:/home/bbs:/bin/tcsh

如果您是 Linux 的話，在最後一行加上

    bbs:x:9999:999:BBS Administrator:/home/bbs:/bin/bash

(當然您也可以用 pw 或 useradd,adduser 的指令來完成相同的動作)

    -root- # joe /etc/group

(假設編輯器是 joe，如果不是的話，請自行改變)
如果您是 FreeBSD 的話，在最後一行加上

    bbs:*:99:bbs

如果您是 Linux 的話，在最後一行加上

    bbs:*:999:bbs

(當然您也可以用 pw 或 groupadd 的指令來完成相同的動作)

    -root- # passwd bbs

輸入 bbs 的密碼

    -root- # tar xvfz /tmp/MapleBBS-3.10-20yymmdd-PACK.tgz -C /home/

假設您把 BBS 程式檔案放在 /tmp/

    -root- # chown -R bbs:bbs /home/bbs


# 四、安裝 BBS

以 bbs 身分登入。

    -bbs- % joe /home/bbs/src/include/config.h

修改 `HOST_ALIASES`，把您所有的 fqdn 都加進去

    #define HOST_ALIASES    {MYHOSTNAME, MYIPADDR, \
        "wolf.twbbs.org", "wolf.twbbs.org.tw", \
        NULL}

如果您是 Linux 的話，改 BBSGID 為 999

    #define BBSGID          99                      /* Linux 請設為 999 */

如果您是 FreeBSD 的話
那麼 BBSGID 維持是 99

    -bbs- % joe /home/bbs/src/include/dns.h

如果您沒有 relay server 可幫您的 BBS 寄信的話，那麼請跳過這一步，但您將可能無法對外寄信到某些站台。

如果您有 relay server 可幫您的 BBS 寄信的話，請將 `HAVE_RELAY_SERVER` 的 `#undef` 改成 `#define`，並改 `RELAY_SERVER` 的定義值。

    #define HAVE_RELAY_SERVER       /* 採用 relay server 來外寄信件 */

    #ifdef HAVE_RELAY_SERVER
    #define RELAY_SERVER    "mail.tnfsh.tn.edu.tw"  /* outbound mail server */
    #endif

例如在交通大學的站可以使用 "smtp.nctu.edu.tw"，而使用 HiNet ADSL 的站可以使用 "msa.hinet.net"。

    -bbs- % joe /home/bbs/bin/install.sh

修改 schoolname bbsname ... msg_bmw 等數項，例如改成以下這樣
(請參考 install.sh 前面的註解，對名稱有些限制)

    schoolname="交大電子"
    bbsname="蘋果樂園"
    bbsname2="AppleBBS"
    sysopnick="站長大大"
    myipaddr="140.113.55.66"
    myhostname="nctu5566.dorm3.nctu.edu.tw"
    msg_bmw="水球"

如果您是使用 Linux 的話，還要改

    ostype="linux"

    -bbs- % /home/bbs/bin/install.sh

您需要等待一段時間來完成編譯

    -bbs- % rm -f /home/bbs/bin/install.sh

sed 用過一次以後就沒用了，那就跟它說聲再見吧

    -bbs- % crontab /home/bbs/doc/crontab

把 doc/crontab 的內容加入 crontab

#  五、設定 BBS 環境 -- （Ａ）如果有 inetd

如果沒有 /etc/inetd.conf 這檔案，請跳到五（Ｂ），通常 FreeBSD 應該有 inetd 才對。

以 root 身分登入。

    -root- # joe /etc/inetd.conf

刪除原本的二行 (前面加上 # 即可)

    #telnet stream  tcp     nowait  root    /usr/libexec/telnetd    telnetd
    #telnet stream  tcp6    nowait  root    /usr/libexec/telnetd    telnetd

加入以下數行

    #
    # MapleBBS
    #
    telnet  stream  tcp     wait    bbs     /home/bbs/bin/bbsd      bbsd -i
    smtp    stream  tcp     wait    bbs     /home/bbs/bin/bmtad     bmtad -i
    gopher  stream  tcp     wait    bbs     /home/bbs/bin/gemd      gemd -i
    finger  stream  tcp     wait    bbs     /home/bbs/bin/bguard    bguard -i
    pop3    stream  tcp     wait    bbs     /home/bbs/bin/bpop3d    bpop3d -i
    nntp    stream  tcp     wait    bbs     /home/bbs/bin/bnntpd    bnntpd -i
    http    stream  tcp     wait    bbs     /home/bbs/bin/bhttpd    bhttpd -i
    xchat   stream  tcp     wait    bbs     /home/bbs/bin/xchatd    xchatd -i
    bbsnntp stream  tcp     wait    bbs     /home/bbs/innd/innbbsd  innbbsd -i

.

    -root- # joe /etc/rc.local

加入以下數行 (這檔案有可能原本是沒有任何文字的開新檔案)

    #!/bin/sh
    #
    # MapleBBS
    #
    su bbs -c '/home/bbs/bin/camera'
    su bbs -c '/home/bbs/bin/account'

# 五、設定 BBS 環境 -- （Ｂ）如果有 xinetd

如果沒有 /etc/xinetd.d/ 這目錄，請跳到五（Ｃ），通常 Linux 應該有 xinetd 才對。

以 root 身分登入。

    -root- # joe /etc/xinetd.d/telnet

將這檔案改成此內容 (這檔案有可能原本是沒有任何文字的開新檔案)

    service telnet
    {
            disable         = no
            flags           = REUSE
            socket_type     = stream
            wait            = yes
            user            = bbs
            server          = /home/bbs/bin/bbsd
            server_args     = -i
    }

.

    -root- # joe /etc/xinetd.d/smtp

將這檔案改成此內容 (這檔案有可能原本是沒有任何文字的開新檔案)

    service smtp
    {
            disable         = no
            socket_type     = stream
            wait            = yes
            user            = bbs
            server          = /home/bbs/bin/bmtad
            server_args     = -i
    }

.

    -root- # joe /etc/xinetd.d/gopher

將這檔案改成此內容 (這檔案有可能原本是沒有任何文字的開新檔案)

    service gopher
    {
            disable         = no
            flags           = REUSE
            socket_type     = stream
            wait            = yes
            user            = bbs
            server          = /home/bbs/bin/gemd
            server_args     = -i
    }

.

    -root- # joe /etc/xinetd.d/finger

將這檔案改成此內容 (這檔案有可能原本是沒有任何文字的開新檔案)

    service finger
    {
            disable         = no
            socket_type     = stream
            wait            = yes
            user            = bbs
            server          = /home/bbs/bin/bguard
            server_args     = -i
    }

.

    -root- # joe /etc/xinetd.d/pop3

將這檔案改成此內容 (這檔案有可能原本是沒有任何文字的開新檔案)

    service pop3
    {
        disable         = no
        socket_type     = stream
        wait            = yes
        user            = bbs
        server          = /home/bbs/bin/bpop3d
        server_args     = -i
    }

.

    -root- # joe /etc/xinetd.d/nntp

將這檔案改成此內容 (這檔案有可能原本是沒有任何文字的開新檔案)

    service nntp
    {
            disable         = no
            flags           = REUSE
            socket_type     = stream
            wait            = yes
            user            = bbs
            server          = /home/bbs/bin/bnntpd
            server_args     = -i
    }

.

    -root- # joe /etc/xinetd.d/http

將這檔案改成此內容 (這檔案有可能原本是沒有任何文字的開新檔案)

    service http
    {
            disable         = no
            flags           = REUSE
            socket_type     = stream
            wait            = yes
            user            = bbs
            server          = /home/bbs/bin/bhttpd
            server_args     = -i
    }

.

    -root- # joe /etc/xinetd.d/xchat

將這檔案改成此內容 (這檔案有可能原本是沒有任何文字的開新檔案)

    service xchat
    {
            disable         = no
            flags           = REUSE
            socket_type     = stream
            wait            = yes
            user            = bbs
            server          = /home/bbs/bin/xchatd
            server_args     = -i
    }

.

    -root- # joe /etc/xinetd.d/bbsnntp

將這檔案改成此內容 (這檔案有可能原本是沒有任何文字的開新檔案)

    service bbsnntp
    {
            disable         = no
            flags           = REUSE
            socket_type     = stream
            wait            = yes
            user            = bbs
            server          = /home/bbs/innd/innbbsd
            server_args     = -i
    }

.

    -root- # joe /etc/rc.d/rc.local

加入以下數行 (這檔案有可能原本是沒有任何文字的開新檔案)

    #!/bin/sh
    #
    # MapleBBS
    #
    su bbs -c '/home/bbs/bin/camera'
    su bbs -c '/home/bbs/bin/account'

# 五、設定 BBS 環境 -- （Ｃ）如果沒有 inetd/xinetd

沒 inetd 也沒 xinetd，改用 standalone 啟動。

以 root 身分登入。

    -root- # joe /etc/rc.local

加入以下數行 (這檔案有可能原本是沒有任何文字的開新檔案)

    #!/bin/sh
    #
    # MapleBBS
    #
    /home/bbs/bin/bbsd
    /home/bbs/bin/bmtad
    /home/bbs/bin/gemd
    /home/bbs/bin/bguard
    /home/bbs/bin/bpop3d
    /home/bbs/bin/bnntpd
    /home/bbs/bin/xchatd
    /home/bbs/innd/innbbsd

    su bbs -c '/home/bbs/bin/camera'
    su bbs -c '/home/bbs/bin/account'


# 六、其他設定

以 root 身分登入。

    -root- # joe /etc/services

加入以下數行

    xchat           3838/tcp
    xchat           3838/udp
    bbsnntp         7777/tcp   usenet       #Network News Transfer Protocol
    bbsnntp         7777/udp   usenet       #Network News Transfer Protocol

.

    -root- # joe /etc/login.conf

修改 md5 為 des 編碼，Linux 請跳過此步驟

    default:\
        :passwd_format=des:\

.

    -root- # joe /etc/rc.conf

把 YES 改成 NO，Linux 請跳過此步驟

    sendmail_enable="NO"

.

    -root- # reboot

重開機吧

# 七、享用您自己的 BBS

您的 BBS 應該已經架好了，試著 telnet 看看，那就這樣好好享用吧。

# --

交大電子 杜宇軒

itoc.bbs@bbs.tnfsh.tn.edu.tw

WWW: http://processor.tfcis.org/~itoc
