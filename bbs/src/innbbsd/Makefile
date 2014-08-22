# ------------------------------------------------------ #
#  innbbsd/Makefile     ( NTHU CS MapleBBS Ver 3.10 )    #
# ------------------------------------------------------ #
#  target : Makefile for 轉信程式 innbbsd                #
#  create : 95/03/29                                     #
#  update : 01/08/26                                     #
#  author : skhuang                                      #
# ------------------------------------------------------ #


# ------------------------------------------------------ #
# 下列的 make rules 不需修改				 #
# ------------------------------------------------------ #


CC	= gcc

INNOBJS	= channel.o bbslib.o convcode.o inntobbs.o rec_article.o nocem.o history.o

LNKOBJS	= bbslink.o bbslib.o convcode.o inntobbs.o rec_article.o nocem.o history.o

INNBBSD	= innbbsd bbslink

EXE	= $(INNBBSD)


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
	@$(MAKE) CFLAGS="-O2 -pipe -I../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../lib -ldao" $(EXE)

solaris:
	@$(MAKE) CFLAGS="-DSOLARIS -O2 -pipe -I../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../lib -ldao -lsocket -lnsl -L/usr/ucblib -lucb" $(EXE)

sol-x86:
	@$(MAKE) CFLAGS="-DSOLARIS -O2 -I../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../lib -ldao -lsocket -lnsl" $(EXE)

freebsd:
	@$(MAKE) CFLAGS="-O2 -pipe -I../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../lib -ldao" $(EXE)

bsd:
	@$(MAKE) CFLAGS="-O2 -pipe -I../include -fomit-frame-pointer -Wunused -Wno-unused-result -Wno-parentheses" LDFLAGS="-s -L../lib -ldao" $(EXE)

cygwin:
	@$(MAKE) CFLAGS="-DCYGWIN -O2 -pipe -I../include -fomit-frame-pointer -Wunused" LDFLAGS="-s -L../lib -ldao -lcygipc" $(EXE)



bbslink: $(LNKOBJS)
	$(CC) -o bbslink $(LNKOBJS) $(OPT) $(LDFLAGS)

innbbsd: $(INNOBJS)
	$(CC) -o innbbsd $(INNOBJS) $(OPT) $(LDFLAGS)


install: $(INNBBSD)
	install -m 0700 $? $(HOME)/innd

update:
	-csh -c "kill `tail -1 $(HOME)/run/innbbsd.pid | awk '{print $$1}'`";exit 0

clean:
	rm -f $(EXE) *.exe *.o *~
