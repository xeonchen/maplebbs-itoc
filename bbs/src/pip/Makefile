# ------------------------------------------------------ #
#  Makefile	( NTHU CS MapleBBS Ver 3.10 )		 #
# ------------------------------------------------------ #
#  author : itoc.bbs@bbs.tnfsh.tn.edu.tw		 #
#  target : Makefile for PIP GAME main programs		 #
#  create : 01/07/24				 	 #
#  update :   /  /					 #
# ------------------------------------------------------ #


# ------------------------------------------------------ #
# 下列的 make rules 不需修改				 #
# ------------------------------------------------------ #


CC =	gcc

OBJ =	pip_basic.o pip_ending.o pip_fight.o pip_item.o pip_job.o \
	pip_menu.o pip_pk.o pip_play.o pip_prac.o pip_quest.o pip_race.o \
	pip_royal.o pip_stuff.o pip_visio.o pip_weapon.o pip.o


SO =	pip.so


all: 
	@echo "Please enter 'make sys-type', "
	@echo " make sun     : for Sun-OS 4.x and maybe some BSD systems, cc or gcc" 
	@echo " make linux   : for Linux"
	@echo " make solaris : for Sun-OS 5.x gcc" 
	@echo " make sol-x86 : for Solaris 7 x86"
	@echo " make freebsd : for BSD 4.4 systems"
	@echo " make bsd     : for BSD systems, cc or gcc, if not in the above lists"
	@echo " make cygwin  : for Microsoft Windows and Cygwin gcc"

sun:
	@$(MAKE) CFLAGS="-O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" $(SO)

linux:
	@$(MAKE) CFLAGS="-DLINUX -O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" $(SO)

solaris:
	@$(MAKE) CFLAGS="-DSOLARIS -DSYSV -O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" $(SO)

sol-x86:
	@$(MAKE) CFLAGS="-DSOLARIS -DSYSV -O2 -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" $(SO)

freebsd:
	@$(MAKE) CFLAGS="-DBSD44 -O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" $(SO)

bsd:
	@$(MAKE) CFLAGS="-DBSD44 -O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" $(SO)

cygwin:
	@cd ../maple; make cygwin


pip.so: $(OBJ)
	ld -s -G $(OBJ) -o pip.so -L../lib -ldao


install: $(SO)
	install -m 0700 $? $(HOME)/bin
	
clean:
	rm -f $(SO) *.o *~
