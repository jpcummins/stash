require 'mkmf'
$CFLAGS = "-Wall -pedantic -std=c99 -O3"
create_makefile('stash/stash')
