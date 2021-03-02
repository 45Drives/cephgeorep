TARGET = dist/from_source/cephfssyncd
LIBS = -lboost_system -lboost_filesystem -lpthread -ltbb
CC = g++
CFLAGS = -std=c++17 -g -O2 -Wall

SOURCE_FILES := $(wildcard src/*.cpp)
OBJECT_FILES := $(patsubst src/%.cpp, build/%.o, $(SOURCE_FILES))

ifeq ($(PREFIX),)
	PREFIX := /opt/45drives/cephgeorep
endif

.PHONY: default all static clean clean-build clean-target install uninstall

default: $(TARGET)
all: default
static: LIBS = -static -lboost_system -lboost_filesystem -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
static: CFLAGS = -DNO_PARALLEL_SORT -std=c++11 -g -O2 -Wall
static: default

.PRECIOUS: $(TARGET) $(OBJECTS)

$(OBJECT_FILES): build/%.o : src/%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(patsubst build/%.o, src/%.cpp, $@) -o $@

$(TARGET): $(OBJECT_FILES)
	mkdir -p dist/from_source
	$(CC) $(OBJECT_FILES) -Wall $(LIBS) -o $@

clean: clean-build clean-target

clean-target:
	-rm -rf dist/from_source

clean-build:
	-rm -rf build

install: all inst-man-pages inst-config inst-completion
	mkdir -p $(DESTDIR)$(PREFIX)
	mkdir -p $(DESTDIR)/lib/systemd/system
	mkdir -p $(DESTDIR)/usr/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)
	install -m 755 s3wrap.sh $(DESTDIR)$(PREFIX)
	cp cephfssyncd.service $(DESTDIR)/lib/systemd/system/cephfssyncd.service
	ln -sf $(PREFIX)/$(notdir $(TARGET)) $(DESTDIR)/usr/bin/$(notdir $(TARGET))
ifneq ($(PACKAGING),1)
	-systemctl daemon-reload
endif

uninstall: rm-man-pages rm-completion
	-systemctl disable --now cephfssyncd
	-rm -f $(DESTDIR)$(PREFIX)/$(notdir $(TARGET))
	-rm -f $(DESTDIR)$(PREFIX)/s3wrap.sh
	-rm -f $(DESTDIR)/usr/lib/systemd/system/cephfssyncd.service
	-rm -f $(DESTDIR)/usr/bin/$(notdir $(TARGET))
ifneq ($(PACKAGING),1)
	systemctl daemon-reload
endif

inst-man-pages:
	mkdir -p $(DESTDIR)/usr/share/man/man8
	gzip -kf doc/man/cephgeorep.8
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

inst-completion:
	mkdir -p $(DESTDIR)/usr/share/bash-completion/completions
	cp doc/completion/cephfssyncd.bash-completion $(DESTDIR)/usr/share/bash-completion/completions/cephfssyncd

rm-completion:
	-rm -f $(DESTDIR)/usr/share/bash-completion/completions/cephfssyncd

