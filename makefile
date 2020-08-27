TARGET = cephfssyncd
LIBS =  -l:libboost_system.a -l:libboost_filesystem.a
CC = g++
CFLAGS = -std=gnu++11 -Wall

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard src/*.cpp))
HEADERS = $(wildcard src/*.hpp)

ifeq ($(PREFIX),)
	PREFIX := /usr/local
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
	install -m 755 cephfssyncd $(DESTDIR)$(PREFIX)/bin
	install -m 755 s3wrap.sh $(DESTDIR)$(PREFIX)/bin
	cp cephfssyncd.service /usr/lib/systemd/system/cephfssyncd.service
	systemctl daemon-reload

uninstall:
	systemctl dsiable --now cephfssyncd
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)$(PREFIX)/bin/s3wrap.sh
	rm -f /usr/lib/systemd/system/cephfssyncd.service
	systemctl daemon-reload
