TARGET = cephfssyncd
LIBS =  -static -l:libboost_system.a -l:libboost_filesystem.a -lpthread
CC = g++
CFLAGS = -std=gnu++11 -Wall

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard src/*.cpp))
HEADERS = $(wildcard src/*.hpp)

ifeq ($(PREFIX),)
	PREFIX := /opt/45drives/cephgeorep
endif

.PHONY: default all clean clean-build clean-target install uninstall

default: $(TARGET)
all: default

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

install: all
	mkdir -p $(DESTDIR)$(PREFIX)
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)
	install -m 755 s3wrap.sh $(DESTDIR)$(PREFIX)
	cp cephfssyncd.service $(DESTDIR)/usr/lib/systemd/system/cephfssyncd.service
	systemctl daemon-reload
	ln -sf $(PREFIX)/$(TARGET) $(DESTDIR)/usr/local/bin
	ln -sf $(PREFIX)/s3wrap.sh $(DESTDIR)/usr/local/bin

uninstall:
	-systemctl disable --now cephfssyncd
	-rm -f $(DESTDIR)$(PREFIX)/$(TARGET)
	-rm -f $(DESTDIR)$(PREFIX)/s3wrap.sh
	-rm -f $(DESTDIR)/usr/lib/systemd/system/cephfssyncd.service
	systemctl daemon-reload
