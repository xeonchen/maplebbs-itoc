# ------------------------------------------------------- #
#  util/Makefile	( NTHU CS MapleBBS Ver 3.10 )	  #
# ------------------------------------------------------- #
#  target : Makefile					  #
#  create : 95/03/29				 	  #
#  update : 95/12/15				 	  #
# ------------------------------------------------------- #


# ------------------------------------------------------ #
# 下列的 make rules 不需修改				 #
# ------------------------------------------------------ #


CC =	gcc

EXE =	bguard bhttpd bmtad bnntpd bpop3d gemd xchatd


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
	@$(MAKE) CFLAGS="-DLINUX -O2 -pipe -I../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../lib -ldao -lcrypt -lresolv" $(EXE)

solaris:
	@$(MAKE) CFLAGS="-DSOLARIS -O2 -pipe -I../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../lib -ldao -lsocket -lresolv -lnsl -L/usr/ucblib -lucb -R/usr/ucblib" $(EXE)

sol-x86:
	@$(MAKE) CFLAGS="-DSOLARIS -O2 -I../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../lib -ldao -lsocket -lresolv -lnsl" $(EXE)

freebsd:
	@$(MAKE) CFLAGS="-DBSD44 -O2 -pipe -I../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../lib -ldao -lcrypt" $(EXE)

bsd:
	@$(MAKE) CFLAGS="-DBSD44 -O2 -pipe -I../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../lib -ldao" $(EXE)

cygwin:
	@$(MAKE) CFLAGS="-DLINUX -DCYGWIN -O2 -pipe -I../include -fomit-frame-pointer -Wunused" LDFLAGS="-s -L../lib -ldao -lcrypt -lresolv -lcygipc" $(EXE)


.c:	;  $(CC) -o $@ $@.c $(CFLAGS) $(LDFLAGS)


install: $(EXE)
	install -m 0700 $? $(HOME)/bin

update:
	-csh -c "kill `tail -1 $(HOME)/run/bguard.pid | awk '{print $$1}'`";exit 0
	-csh -c "kill `tail -1 $(HOME)/run/bhttp.pid | awk '{print $$1}'`";exit 0
	-csh -c "kill `tail -1 $(HOME)/run/bmta.pid | awk '{print $$1}'`";exit 0
	-csh -c "kill `tail -1 $(HOME)/run/bnntp.pid | awk '{print $$1}'`";exit 0
	-csh -c "kill `tail -1 $(HOME)/run/pop3.pid | awk '{print $$1}'`";exit 0
	-csh -c "kill `tail -1 $(HOME)/run/gemd.pid | awk '{print $$1}'`";exit 0
	-csh -c "kill `tail -1 $(HOME)/run/chat.pid | awk '{print $$1}'`";exit 0

clean:
	rm -f $(EXE) *.exe *.o *~
