#!/bin/sh

cd `dirname $0`

echo '1..1'

make 2>&1 > /dev/null

perl ./test.pl > test.pl.out 2> /dev/null
./test    > test.c.out 2> /dev/null

diff test.pl.out test.c.out

if [ $? -eq 0 ]; then
	echo 'ok 1'
else
	echo 'not ok 1'
fi
