# ------------------------------------------------------- #
#  src/Makefile	( NTHU CS MapleBBS Ver 3.10 )	          #
# ------------------------------------------------------- #
#  target : Makefile for ALL				  #
#  create : 00/02/12                                      #
#  update :   /  /                                        #
# ------------------------------------------------------- #


# 支援的 OS-type
# sun linux solaris sol-x86 freebsd bsd cygwin

# 需要 compile 的目錄
# lib daemon innbbsd maple so game pip util util/backup util/tran util/uno


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
	@cd lib; make
	@cd daemon; make sun
	@cd innbbsd; make sun
	@cd maple; make sun
	@cd so; make sun
	@cd game; make sun
	@cd pip; make sun
	@cd util; make sun
	@cd util/backup; make sun
	@cd util/tran; make sun
	@cd util/uno; make sun

linux:
	@cd lib; make
	@cd daemon; make linux
	@cd innbbsd; make linux
	@cd maple; make linux
	@cd so; make linux
	@cd game; make linux
	@cd pip; make linux
	@cd util; make linux
	@cd util/backup; make linux
	@cd util/tran; make linux
	@cd util/uno; make linux

solaris:
	@cd lib; make
	@cd daemon; make solaris
	@cd innbbsd; make solaris
	@cd maple; make solaris
	@cd so; make solaris
	@cd game; make solaris
	@cd pip; make solaris
	@cd util; make solaris
	@cd util/backup; make solaris
	@cd util/tran; make solaris
	@cd util/uno; make solaris

sol-x86:
	@cd lib; make
	@cd daemon; make sol-x86
	@cd innbbsd; make sol-x86
	@cd maple; make sol-x86
	@cd so; make sol-x86
	@cd game; make sol-x86
	@cd pip; make sol-x86
	@cd util; make sol-x86
	@cd util/backup; make sol-x86
	@cd util/tran; make sol-x86
	@cd util/uno; make sol-x86

freebsd:
	@cd lib; make
	@cd daemon; make freebsd
	@cd innbbsd; make freebsd
	@cd maple; make freebsd
	@cd so; make freebsd
	@cd game; make freebsd
	@cd pip; make freebsd
	@cd util; make freebsd
	@cd util/backup; make freebsd
	@cd util/tran; make freebsd
	@cd util/uno; make freebsd

bsd:
	@cd lib; make
	@cd daemon; make bsd
	@cd innbbsd; make bsd
	@cd maple; make bsd
	@cd so; make bsd
	@cd game; make bsd
	@cd pip; make bsd
	@cd util; make bsd
	@cd util/backup; make bsd
	@cd util/tran; make bsd
	@cd util/uno; make bsd

cygwin:
	@cd lib; make
	@cd daemon; make cygwin
	@cd innbbsd; make cygwin
	@cd maple; make cygwin
	@cd so; make cygwin
	@cd game; make cygwin
	@cd pip; make cygwin
	@cd util; make cygwin
	@cd util/backup; make cygwin
	@cd util/tran; make cygwin
	@cd util/uno; make cygwin

install:
	@cd daemon; make install
	@cd innbbsd; make install
	@cd maple; make install
	@cd so; make install
	@cd game; make install
	@cd pip; make install
	@cd util; make install
	@cd util/backup; make install
	@cd util/tran; make install
	@cd util/uno; make install

update:
	@cd daemon; make update
	@cd innbbsd; make update
	@cd maple; make update

clean:
	@cd lib; make clean
	@cd daemon; make clean
	@cd innbbsd; make clean
	@cd maple; make clean
	@cd so; make clean
	@cd game; make clean
	@cd pip; make clean
	@cd util; make clean
	@cd util/backup; make clean
	@cd util/tran; make clean
	@cd util/uno; make clean
