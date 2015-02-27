# we use GLib 2.22.2 version (not modifyed but only 2.22.2)
# to use GLib 2.22.2 you can make it from source code
# http://ftp.gnome.org/pub/gnome/sources/glib/2.22/glib-2.22.2.tar.bz2
# tar -xf ...
# ./configure --prefix = /path-to-GLib.2.22.2
# make
# make install
# and then make soft link named "glib"
# ln -s /path-to-GLib.2.22.2 glib
#

# GLib finder
this=.
elecard_glib := $(shell [ -d $(this)/glib/include/glib-2.0 ] && echo yes || echo no)

ifeq ($(elecard_glib), yes)
glib-cflags := -I$(this)/glib/include/glib-2.0 -I$(this)/glib/lib/glib-2.0/include
glib-libflags := -L$(this)/glib/lib  -lglib-2.0 -lgthread-2.0 -lgmodule-2.0
else
glib_found := $(shell ( pkg-config --cflags glib-2.0 > /dev/null ) && echo yes || echo no)
ifeq ($(glib_found), yes)
glib-cflags := `pkg-config --cflags glib-2.0`
glib-libflags := `pkg-config --libs gmodule-2.0 gthread-2.0`
endif
endif


all:
	cc -fPIC -shared -I./ $(glib-cflags) $(glib-libflags) -g -O0 -lm -o billing.so billing.c

clean:
	[ -f billing.so ] && rm billing.so || true
	[ -f test ] && rm test || true

tests:
	cc -fPIC -I./ $(glib-cflags) $(glib-libflags) -g -O0 -o test test.c -lm
