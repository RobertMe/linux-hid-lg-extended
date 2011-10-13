prefix ?= /usr/local
bindir ?= $(prefix)/bin

default:
	gcc -lreadline lg-debug.c -o lg-debug

install:
	install -D -m 0755 lg-warn-battery $(DESTDIR)$(bindir)/lg-warn-battery
	install -D -m 0755 lg-bind $(DESTDIR)$(bindir)/lg-bind
	install -D -m 0700 lg-debug $(DESTDIR)$(bindir)/lg-debug

clean:
	rm -rf lg-debug
