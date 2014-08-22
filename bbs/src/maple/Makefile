# ------------------------------------------------------ #
#  maple/Makefile	( NTHU CS MapleBBS Ver 3.10 )    #
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

SRC =	acct.c bbsd.c bmw.c board.c cache.c edit.c favor.c \
	gem.c mail.c menu.c more.c pal.c post.c talk.c ulist.c \
	user.c visio.c window.c xover.c xpost.c

OBJ =	acct.o bbsd.o bmw.o board.o cache.o edit.o favor.o \
	gem.o mail.o menu.o more.o pal.o post.o talk.o ulist.o \
	user.o visio.o window.o xover.o xpost.o

EXE =	bbsd


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
	@$(MAKE) CFLAGS="-O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" LDFLAGS="-s -L../lib -ldao" $(EXE)

linux:
	@$(MAKE) CFLAGS="-DLINUX -O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" LDFLAGS="-s -L../lib -ldao -lcrypt -lresolv -ldl -rdynamic" $(EXE)

solaris:
	@$(MAKE) CFLAGS="-DSOLARIS -DSYSV -O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" LDFLAGS="-s -L../lib -ldao -ltermcap -lsocket -lnsl -lresolv -lelf -ldl" $(EXE)

sol-x86:
	@$(MAKE) CFLAGS="-DSOLARIS -DSYSV -O2 -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" LDFLAGS="-s -L../lib -ldao -ltermcap -lsocket -lnsl -lresolv -lelf -ldl" $(EXE)

freebsd:
	@$(MAKE) CFLAGS="-DBSD44 -O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" LDFLAGS="-s -L../lib -ldao -lcrypt -Wl,--export-dynamic" $(EXE)

bsd:
	@$(MAKE) CFLAGS="-DBSD44 -O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../include" LDFLAGS="-s -L../lib -ldao" $(EXE)

cygwin:
	@$(MAKE) CFLAGS="-DLINUX -DCYGWIN -O2 -pipe -fomit-frame-pointer -Wunused -I../include" LDFLAGS="-s -L../lib -ldao -lcrypt -lresolv -lcygipc" $(EXE)


bbsd: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)


install: $(EXE)
	install -m 0700 $? $(HOME)/bin

update:
	-csh -c "kill `tail -1 $(HOME)/run/bbs.pid | awk '{print $$1}'`";exit 0

clean:
	rm -f $(EXE) *.exe *.o *~
