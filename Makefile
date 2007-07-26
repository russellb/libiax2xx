#
# Copyright (C) 2006, Russell Bryant <russell@russellbryant.net>
#
# LibIAX2xx Makefile
#

include Makefile.rules

CXX?=g++
AR?=ar
ARFLAGS?=-rv
RANLIB?=ranlib
CXXFLAGS+=-pipe -Iinclude -D_REENTRANT -D_GNU_SOURCE -g

DARWIN:=$(shell uname -a | grep -i darwin)
ifneq ($(DARWIN),)
POLLCOMPAT:=src/poll.o
CXXFLAGS+=-DPOLL_COMPAT
CFLAGS+=$(CXXFLAGS)
endif

LIBIAX2PP_OBJS:=$(sort src/iax2_dialog.o src/iax2_peer.o src/iax2_frame.o src/iax2_client.o src/iax2_server.o src/iax2_event.o src/iax2_command.o src/time.o src/iax2_lag.o $(POLLCOMPAT))

APPS:=test_server test_client test_iax2_dialog_timer iaxpacket

TEST_IAX2_DIALOG_TIMER_OBJS:=src/test_iax2_dialog_timer.o
TEST_IAX2_DIALOG_TIMER_LIBS:=-lpthread

TEST_SERVER_OBJS:=src/test_server.o
TEST_SERVER_LIBS:=-lpthread

TEST_CLIENT_OBJS:=src/test_client.o
TEST_CLIENT_LIBS:=-lpthread

IAXPACKET_OBJS:=src/iaxpacket.o
IAXPACKET_LIBS:=-lpthread

all: libiax2xx.a $(APPS)

$(eval $(call ast_make_a_o,libiax2xx.a,$(LIBIAX2PP_OBJS)))

$(eval $(call ast_make_o_cxx,src/iax2_client.o,src/iax2_client.cpp include/iax2/iax2_client.h include/iax2/iax2_frame.h include/iax2/iax2_dialog.h include/iax2/iax2_peer.h))

$(eval $(call ast_make_o_cxx,src/iax2_server.o,src/iax2_server.cpp include/iax2/iax2_server.h include/iax2/iax2_frame.h include/iax2/iax2_dialog.h include/iax2/iax2_peer.h))

$(eval $(call ast_make_o_cxx,src/iax2_frame.o,src/iax2_frame.cpp include/iax2/iax2_frame.h))

$(eval $(call ast_make_o_cxx,src/iax2_dialog.o,src/iax2_dialog.cpp include/iax2/iax2_dialog.h))

$(eval $(call ast_make_o_cxx,src/iax2_lag.o,src/iax2_lag.cpp include/iax2/iax2_lag.h include/iax2/iax2_dialog.h))

$(eval $(call ast_make_o_cxx,src/iax2_command.o,src/iax2_command.cpp include/iax2/iax2_command.h))

$(eval $(call ast_make_o_cxx,src/iax2_event.o,src/iax2_event.cpp include/iax2/iax2_event.h))

$(eval $(call ast_make_o_cxx,src/iax2_peer.o,src/iax2_peer.cpp include/iax2/iax2_peer.h))

$(eval $(call ast_make_o_cxx,src/test_server.o,src/test_server.cpp include/iax2/iax2_server.h include/iax2/iax2_event.h))

$(eval $(call ast_make_o_cxx,src/test_client.o,src/test_client.cpp include/iax2/iax2_client.h include/iax2/iax2_event.h))

$(eval $(call ast_make_o_cxx,src/iaxpacket.o,src/iaxpacket.cpp include/iax2/iax2_client.h include/iax2/iax2_event.h))

$(eval $(call ast_make_o_cxx,src/test_iax2_dialog_timer.o,src/test_iax2_dialog_timer.cpp include/iax2/iax2_peer.h))

$(eval $(call ast_make_o_cxx,src/time.o,src/time.cpp include/iax2/time.h))

$(eval $(call ast_make_o_c,src/poll.o,src/poll.c include/poll-compat.h))

test_iax2_dialog_timer: LIBS+=$(TEST_IAX2_DIALOG_TIMER_LIBS)

$(eval $(call ast_make_final,test_iax2_dialog_timer,$(TEST_IAX2_DIALOG_TIMER_OBJS) libiax2xx.a))

test_server: LIBS+=$(TEST_SERVER_LIBS)

$(eval $(call ast_make_final,test_server,$(TEST_SERVER_OBJS) libiax2xx.a))

test_client: LIBS+=$(TEST_CLIENT_LIBS)

iaxpacket: LIBS+=$(IAXPACKET_LIBS)

$(eval $(call ast_make_final,test_client,$(TEST_CLIENT_OBJS) libiax2xx.a))

$(eval $(call ast_make_final,iaxpacket,$(IAXPACKET_OBJS) libiax2xx.a))

clean:
	rm -f src/*.o libiax2xx.a $(APPS)

distclean: clean

.PHONY: all clean distclean
