VERSION:=$(shell git log -1 --date=short --format="%ad %h")
UID:=$(shell id -u)
CXXFLAGS+= -pthread -std=c++17 -O3 -gdwarf -Wall -I/usr/local/include
LDFLAGS+= -fvisibility=hidden
DEFINES=-DGIT_VERSION="\"$(VERSION)\""
PREFIX?=/usr
SSL?=/usr
SQLITE3?=/usr/local
LIB_DYNAMIC=no

$(shell if [ ! -d obj ] ; then mkdir obj; fi)

SRCS=src/cryfic-main.cpp src/cryfic.cpp src/cryfic-db.cpp src/cryfic-logging.cpp \
     src/cryfic-hash.cpp src/cryfic-opts.cpp src/dsForm.cpp

INCS=src/cryfic.hpp src/cryfic-db.hpp src/cryfic-logging.hpp src/cryfic-opts.hpp \
	 src/cryfic-hash.hpp src/dsForm.hpp src/cryfic-threads.hpp

OBJS=obj/cryfic-main.o obj/cryfic.o obj/cryfic-db.o obj/cryfic-logging.o \
     obj/cryfic-hash.o obj/cryfic-opts.o obj/dsForm.o

# TARGETS=cryfic libcryfic.so
TARGETS=cryfic

CFLAGS+=-I$(SSL)/include
LIBS=-lpthread

ifeq ($(LIB_DYNAMIC),yes)
LIBS+=-ldl
DEFINES+=-DLIB_DYNAMIC
else
LIBS+=-lssl -lcrypto -lsqlite3
endif

ifeq ($(UID),0)
INSTALL_OPTS=-a -o root -g root
endif

all: $(TARGETS)

clean:
	rm -f $(TARGETS) $(OBJS)

install: $(TARGETS)
	./install-sh $(INSTALL_OPTS) -m 0444 -t $(PREFIX)/include cryfic.h
	./install-sh $(INSTALL_OPTS) -m 0544 -t $(PREFIX)/lib libcryfic.so
	./install-sh $(INSTALL_OPTS) -m 0655 -t $(PREFIX)/bin cryfic

deinstall:
	rm -f $(PREFIX)/include/cryfic.h
	rm -f $(PREFIX)/lib/libcryfic.so
	rm -f $(PREFIX)/bin/cryfic

# Targets
cryfic: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ -L/usr/lib -L/usr/local/lib -L$(SSL)/lib -L${SQLITE3}/lib $(LIBS)

libcryfic.so: $(filter-out obj/cryfic-main.o,$(OBJS))
	$(CXX) $(LDFLAGS) -shared -o $@ $^ -L$(SSL)/lib $(LIBS)

# Objects
obj/cryfic-main.o: src/cryfic-main.cpp $(INCS)
	$(CXX) -c $(CXXFLAGS) $(DEFINES) -o $@ $<

obj/cryfic.o: src/cryfic.cpp $(INCS)
	$(CXX) -c $(CXXFLAGS) $(DEFINES) -o $@ $<

obj/cryfic-logging.o: src/cryfic-logging.cpp $(INCS)
	$(CXX) -c $(CXXFLAGS) $(DEFINES) -o $@ $<

obj/cryfic-db.o: src/cryfic-db.cpp $(INCS)
	$(CXX) -c $(CXXFLAGS) $(DEFINES) -o $@ $<

obj/cryfic-hash.o: src/cryfic-hash.cpp $(INCS)
	$(CXX) -c $(CXXFLAGS) $(DEFINES) -o $@ $<

obj/cryfic-opts.o: src/cryfic-opts.cpp $(INCS)
	$(CXX) -c $(CXXFLAGS) $(DEFINES) -o $@ $<

obj/dsForm.o: src/dsForm.cpp $(INCS)
	$(CXX) -c $(CXXFLAGS) $(DEFINES) -o $@ $<
