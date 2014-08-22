#!/bin/sh
# ¨Ï¥Î¦¹ script «e¡A½Ð¥ý½T»{¤w¸g­×§ï¤F¤U­±ªº¬ÛÃö³]©w

# ½Ð¦b³oÃä­×§ï¦¨¦Û¤vªº³]©w
#
# ¥H¤U¦WºÙ³]©w¤¤¬Ò¤£¯à¦³ &`"\ µ¥²Å¸¹
#
#   (1) ¤£¯à¦³ & ²Å¸¹¡A¦]¬°³o¬O­I´º°õ¦æªº«O¯d²Å¸¹
#   (2) ¤£¯à¦³ ` ²Å¸¹
#   (3) °£¤F«e«áªº "" ¥H¥~¡A¤£¯à¦A¦³¨ä¥Lªº " ²Å¸¹
#   (4) ¤£¯à¦³ \ ²Å¸¹¡A¦]¬° Big5 ½s½Xªº°ÝÃD¡A¥H¤U³o¨Ç¦r¤]¤£¯à¨Ï¥Î
#       ¢\ £\ ¤\ ¥\ ¦\ §\ ¨\ ©\ ª\ «\ ¬\ ­\ ®\ ¯\ °\ ±\ ²\ ³\ ´\ µ\
#       ¶\ ·\ ¸\ ¹\ º\ »\ ¼\ ½\ ¾\ ¿\ À\ Á\ Â\ Ã\ Ä\ Å\ Æ\ Ç\ È\ É\
#       Ê\ Ë\ Ì\ Í\ Î\ Ï\ Ð\ Ñ\ Ò\ Ó\ Ô\ Õ\ Ö\ ×\ Ø\ Ù\ Ú\ Û\ Ü\ Ý\
#       Þ\ ß\ à\ á\ â\ ã\ ä\ å\ æ\ ç\ è\ é\ ê\ ë\ ì\ í\ î\ ï\ ð\ ñ\
#       ò\ ó\ ô\ õ\ ö\ ÷\ ø\ ù\
#
# ¦pªG±z§Æ±æ¯¸¦Wµ¥³]©w¤º¦³ &`"\ µ¥²Å¸¹ªº¸Ü¡A½Ð¥ýÀH«Kµ¹­Ó³]©w¡A
# ¨Æ«á¦A¥h§ï src/include/config.h §Y¥i


schoolname="¥x«n¤@¤¤"
bbsname="»P«n¦@»R"
bbsname2="WolfBBS"
sysopnick="¯T¤Hªø¦Ñ"
myipaddr="210.70.137.5"
myhostname="bbs.tnfsh.tn.edu.tw"
msg_bmw="¤ô²y"

# ½Ð­×§ï±zªº§@·~¨t²Î
# sun linux solaris sol-x86 freebsd bsd

ostype="freebsd"

echo "±z©Ò³]©wªº SCHOOLNAME ¬O $schoolname"
echo "±z©Ò³]©wªº BBSNAME    ¬O $bbsname"
echo "±z©Ò³]©wªº BBSNAME2   ¬O $bbsname2"
echo "±z©Ò³]©wªº SYSOPNICK  ¬O $sysopnick"
echo "±z©Ò³]©wªº MYIPADDR   ¬O $myipaddr"
echo "±z©Ò³]©wªº MYHOSTNAME ¬O $myhostname"
echo "±z©Ò³]©wªº MSG_BMW    ¬O $msg_bmw"
echo "­Y±z¦³¦h­Ó FQDN (Eg: twbbs)¡A«hÁÙ»Ý¤â°Ê­×§ï src/include/config.h ªº HOST_ALIASES"

echo "±zªº§@·~¨t²Î          ¬O $ostype"

echo "±z¥i¥H¦b http://processor.tfcis.org/~itoc §ä¨ì³Ì·sªºµ{¦¡¤Î¦w¸Ë¤å¥ó"

# ¦^ BBSHOME
cd
echo "[1;36m¶i¦æÂà´«¤¤ [0;5m...[m"


# ´«¦WºÙ ip addr

filelist_1="etc/valid src/include/config.h"

for i in $filelist_1
do
  cat $i | sed 's/¥x«n¤@¤¤/'"$schoolname"'/g' > $i.sed
  mv -f $i.sed $i

  cat $i | sed 's/»P«n¦@»R/'"$bbsname"'/g' > $i.sed
  mv -f $i.sed $i

  cat $i | sed 's/WolfBBS/'"$bbsname2"'/g' > $i.sed
  mv -f $i.sed $i

  cat $i | sed 's/¯T¤Hªø¦Ñ/'"$sysopnick"'/g' > $i.sed
  mv -f $i.sed $i

  cat $i | sed 's/210.70.137.5/'"$myipaddr"'/g' > $i.sed
  mv -f $i.sed $i

  cat $i | sed 's/bbs.tnfsh.tn.edu.tw/'"$myhostname"'/g' > $i.sed 
  mv -f $i.sed $i
done


# ´« ¤ô²y

filelist_2="etc/tip \
src/include/config.h src/include/global.h src/include/modes.h \
src/include/struct.h src/include/theme.h src/include/ufo.h src/maple/CHANGE \
src/maple/acct.c src/maple/bbsd.c src/maple/bmw.c src/maple/menu.c \
src/maple/pal.c src/maple/post.c src/maple/talk.c src/maple/visio.c \
src/maple/xover.c"

for i in $filelist_2
do
  cat $i | sed 's/¤ô²y/'"$msg_bmw"'/g' > $i.sed
  mv -f $i.sed $i
done


# ¦w¸Ë Maple 3.10
echo "[1;36m¦w¸Ë BBS ¤¤ [0;5m...[m"
cd src
make clean $ostype install

# ±Ò°Ê
# °²³]¶}¦b port 9987
cd
bin/bbsd 9987
bin/camera
bin/account

# telnet ´ú¸Õ
telnet 0 9987
