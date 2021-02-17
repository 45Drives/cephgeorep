TARGET = cephfssyncd
LIBS = -g -lboost_system -lboost_filesystem -lpthread
CC = g++
CFLAGS = -g -std=gnu++11 -Wall

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard src/*.cpp))
HEADERS = $(wildcard src/*.hpp)

ifeq ($(PREFIX),)
	PREFIX := /opt/45drives/cephgeorep
endif

.PHONY: default all static clean clean-build clean-target install uninstall

default: $(TARGET)
all: default
static: LIBS = -g -static -l:libboost_system.a -l:libboost_filesystem.a -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
static: default

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean: clean-build clean-target

clean-target:
	-rm -f $(TARGET)

clean-build:
	-rm -f src/*.o

install: all inst-man-pages inst-config
	mkdir -p $(DESTDIR)$(PREFIX)
	mkdir -p $(DESTDIR)/lib/systemd/system
	mkdir -p $(DESTDIR)/usr/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)
	install -m 755 s3wrap.sh $(DESTDIR)$(PREFIX)
	cp cephfssyncd.service $(DESTDIR)/lib/systemd/system/cephfssyncd.service
	ln -sf $(PREFIX)/$(TARGET) $(DESTDIR)/usr/bin/$(TARGET)
	-systemctl daemon-reload

uninstall: rm-man-pages
	-systemctl disable --now cephfssyncd
	-rm -f $(DESTDIR)$(PREFIX)/$(TARGET)
	-rm -f $(DESTDIR)$(PREFIX)/s3wrap.sh
	-rm -f $(DESTDIR)/usr/lib/systemd/system/cephfssyncd.service
	-rm -f $(DESTDIR)/usr/bin/$(TARGET)
	systemctl daemon-reload

inst-man-pages:
	mkdir -p $(DESTDIR)/usr/share/man/man8
	gzip -k doc/man/cephgeorep.8
	mv doc/man/cephgeorep.8.gz $(DESTDIR)/usr/share/man/man8/
	ln -sf $(DESTDIR)/usr/share/man/man8/cephgeorep.8.gz $(DESTDIR)/usr/share/man/man8/cephfssyncd.8.gz
	ln -sf $(DESTDIR)/usr/share/man/man8/cephgeorep.8.gz $(DESTDIR)/usr/share/man/man8/s3wrap.sh.8.gz

rm-man-pages:
	-rm -f $(DESTDIR)/usr/share/man/man8/s3wrap.sh.8.gz
	-rm -f $(DESTDIR)/usr/share/man/man8/cephfssyncd.8.gz
	-rm -f $(DESTDIR)/usr/share/man/man8/cephgeorep.8.gz

inst-config:
	mkdir -p $(DESTDIR)/etc
	cp -n doc/cephfssyncd.conf.template $(DESTDIR)/etc/cephfssyncd.conf
