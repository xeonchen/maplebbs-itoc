# ------------------------------------------------------- #
#  util/uno/Makefile 	( NTHU CS MapleBBS Ver 3.10 )	  #
# ------------------------------------------------------- #
#  target : Makefile for 寄信、統計、備份、系統維護工具	  #
#  create : 01/03/02				 	  #
#  update :   /  /  				 	  #
# ------------------------------------------------------- #


# ------------------------------------------------------ #
# 下列的 make rules 不需修改				 #
# ------------------------------------------------------ #


CC =	gcc

EXE =	collect_uno conflict_uno fix_uno


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
	@$(MAKE) CFLAGS="-O2 -pipe -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses -I../../include" LDFLAGS="-s -L../../lib -ldao" $(EXE)

linux:
	@$(MAKE) CFLAGS="-O2 -pipe -I../../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../../lib -ldao" $(EXE)

solaris:
	@$(MAKE) CFLAGS="-O2 -pipe -I../../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../../lib -ldao -lsocket -lnsl -L/usr/ucblib -lucb" $(EXE)

sol-x86:
	@$(MAKE) CFLAGS="-O2 -I../../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../../lib -ldao -lsocket -lnsl" $(EXE)

freebsd:
	@$(MAKE) CFLAGS="-O2 -pipe -I../../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../../lib -ldao" $(EXE)

bsd:
	@$(MAKE) CFLAGS="-O2 -pipe -I../../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../../lib -ldao" $(EXE)

cygwin:
	@$(MAKE) CFLAGS="-O2 -pipe -I../../include -fomit-frame-pointer -Wunused" LDFLAGS="-s -L../../lib -ldao" $(EXE)


.c:	;  $(CC) -o $@ $@.c $(CFLAGS) $(LDFLAGS)


install: $(EXE)
# 不複製到 bin/ 下
#	install -m 0700 $? $(HOME)/bin
	@echo "ok!"

clean:
	rm -f $(EXE) *.exe *.o *~
