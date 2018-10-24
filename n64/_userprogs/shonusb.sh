#!/bin/sh
setsid sh -c 'exec sh -i </dev/ttyE0 >/dev/ttyE0 2>&1' &
