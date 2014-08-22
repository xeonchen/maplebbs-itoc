# ------------------------------------------------------ #
#  Makefile	( NTHU CS MapleBBS Ver 2.36 )		 #
# ------------------------------------------------------ #
#  author : opus.bbs@bbs.cs.nthu.edu.tw		 	 #
#  target : Makefile for MapleBBS main programs		 #
#  create : 95/03/29				 	 #
#  update : 95/12/15					 #
# ------------------------------------------------------ #


# ------------------------------------------------------ #
# 下列的 make rules 不需修改				 #
# ------------------------------------------------------ #


CC =	gcc

SO =	bar.so bingo.so bj.so bwboard.so chessmj.so dice.so dragon.so fantan.so \
	gp.so guessnum.so km.so liteon.so marie.so mine.so nine.so pushbox.so \
	race.so recall.so reversi.so seven.so tetris.so


.SUFFIXES:
.SUFFIXES: .c .o .so

.c.o:	;  $(CC) $(CFLAGS) -c $*.c
.o.so:	;  ld -s -G $*.o -o $*.so -L../lib -ldao


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


install: $(SO)
	install -m 0700 $? $(HOME)/bin
	
clean:
	rm -f *.so *.o *~
