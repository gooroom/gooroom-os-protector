#
# Makefile for Loader of Gooroom OS Protector.
#
all: gop-helper

CPPFLAGS+=$(shell dpkg-buildflags --get CPPFLAGS)
CXXFLAGS+=$(shell dpkg-buildflags --get CXXFLAGS) $(CPPFLAGS)
LDFLAGS+=$(shell dpkg-buildflags --get LDFLAGS)

clean: 
	rm -f *.o
	rm -f gop-helper

gop-helper: gop-helper.c
	gcc $(CXXFLAGS) $(LDFLAGS) -o gop-helper gop-helper.c
