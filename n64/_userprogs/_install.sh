#!/bin/bash

set -x

SRC=$(dirname "$0")
DST=$SRC/../target

ln $SRC/init.sh $DST/init
ln $SRC/ppp.sh $DST/
ln $SRC/shonusb.sh $DST/
ln $SRC/unmute.sh $DST/

ln $SRC/joytty $DST/usr/bin/
ln $SRC/jstest2 $DST/usr/bin/
#ln $SRC/tiocsti $DST/usr/bin/
ln $SRC/oviplay $DST/usr/bin/
