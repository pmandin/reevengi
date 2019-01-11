#!/bin/sh

#aclocal
#aclocal -I m4
aclocal -I m4 -I /usr/local/share/aclocal
automake --add-missing --copy
autoheader
autoconf
