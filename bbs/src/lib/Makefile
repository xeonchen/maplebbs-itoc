# ------------------------------------------------------ #
#  lib/Makefile		( NTHU CS MapleBBS Ver 3.10 )	 #
# ------------------------------------------------------ #
#  author : opus.bbs@bbs.cs.nthu.edu.tw		 	 #
#  target : Makefile for MapleBBS library routines	 #
#  create : 95/03/29				 	 #
#  update : 95/12/15					 #
# ------------------------------------------------------ #


CC	= gcc
RANLIB	= ranlib
CPROTO	= cproto -E"gcc -pipe -E" -I../include # -s -v
CFLAGS	= -O2 -pipe -fomit-frame-pointer -Wunused -I../include


# ------------------------------------------------------ #
# 下列的 make rules 不需修改				 #
# ------------------------------------------------------ #


SRC =	\
	is_alnum.c is_alpha.c not_addr.c \
	\
	dns.c dns_aton.c dns_name.c dns_open.c dns_smtp.c \
	\
	str_ansi.c str_cat.c str_cmp.c str_decode.c str_folder.c str_fpath.c \
	str_from.c str_has.c str_hash.c str_lower.c str_lowest.c str_ncmp.c \
	str_ncpy.c str_passwd.c str_stamp.c str_str.c str_sub.c str_tail.c \
	str_time.c str_trim.c str_ttl.c \
	\
	archiv32.c chrono32.c hash32.c radix32.c \
	\
	f_cat.c f_cp.c f_img.c f_ln.c f_lock.c f_map.c f_mode.c f_mv.c f_new.c \
	f_path.c f_rm.c f_suck.c mak_dirs.c \
	\
	rec_add.c rec_bot.c rec_num.c rec_del.c rec_get.c rec_ins.c rec_put.c \
	rec_ref.c rec_sync.c \
	\
	hdr_fpath.c hdr_stamp.c \
	\
	shm.c \
	\
	splay.c \
	\
	acl_has.c rfc2047.c xwrite.c \
	\
	dl_lib.c

OBJ =	\
	is_alnum.o is_alpha.o not_addr.o \
	\
	dns.o dns_aton.o dns_name.o dns_open.o dns_smtp.o \
	\
	str_ansi.o str_cat.o str_cmp.o str_decode.o str_folder.o str_fpath.o \
	str_from.o str_has.o str_hash.o str_lower.o str_lowest.o str_ncmp.o \
	str_ncpy.o str_passwd.o str_stamp.o str_str.o str_sub.o str_tail.o \
	str_time.o str_trim.o str_ttl.o \
	\
	archiv32.o chrono32.o hash32.o radix32.o \
	\
	f_cat.o f_cp.o f_img.o f_ln.o f_lock.o f_map.o f_mode.o f_mv.o f_new.o \
	f_path.o f_rm.o f_suck.o mak_dirs.o \
	\
	rec_add.o rec_bot.o rec_num.o rec_del.o rec_get.o rec_ins.o rec_put.o \
	rec_ref.o rec_sync.o \
	\
	hdr_fpath.o hdr_stamp.o \
	\
	shm.o \
	\
	splay.o \
	\
	acl_has.o rfc2047.o xwrite.o \
	\
	dl_lib.o


.c.o:   ;   $(CC) $(CFLAGS) -c $*.c


all:	libdao.a

dao.p:	$(SRC)
	$(CPROTO) -o $@ $?

libdao.a:	$(OBJ)
	ar rv $@ $?
	$(RANLIB) $@

clean:
	rm -f libdao.a *.o *~
