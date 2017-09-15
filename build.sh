libtoolize --quiet --force
aclocal
autoconf
autoheader
automake --add-missing
autoconf
./configure
