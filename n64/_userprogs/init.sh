#!/bin/sh
cd /
echo Booting...
mount -t devtmpfs none /dev
mkdir /dev/pts
mount -t devpts none /dev/pts
mount -t proc none /proc
mount -t sysfs none /sys
mount -t tmpfs none /tmp -o size=200%
echo nameserver 8.8.8.8 > /etc/resolv.conf # redirects to /tmp/resolv.conf
#exec </dev/tty1 >/dev/tty1 2>&1
#echo hello stdout
#echo hello tty1 > /dev/tty1
#echo hello ttyE0 > /dev/ttyE0 &
#echo hello console > /dev/console
#echo hi cat | cat

#( while :; do echo ttyE0~~ > /dev/ttyE0; sleep 2; done )
#while :; do read s </dev/ttyE0; echo $s | sh >/dev/ttyE0 2>&1; done

#echo launching sh on ttyE0
#/bin/sh -i </dev/ttyE0 >/dev/ttyE0 2>&1
#echo ttyE0 shell done

sysctl -w vm.swappiness=100 vm.panic_on_oom=1

echo prepareing swap
losetup -o 33554432 /dev/loop0 /dev/cart0
mkswap /dev/loop0
swapon /dev/loop0
free

echo starting syslogd
syslogd

#echo starting dropbear
#dropbear -R >/dev/console 2>&1 &

echo starting telnetd
telnetd

#echo starting pppd
#pppd noauth nodetach defaultroute debug /dev/ttyE0
# note: "pppd" must be with "nodetach", or pppd take ttyE0 and pppd's die cause uart_port->state(uart_state)->port(tty_port).tty(tty_struct*) null, cause kernel crash in __uart_start->uart_tx_stopped by hit some key!
#(sleep 30;pkill -HUP pppd) & pppd nodetach noauth defaultroute debug /dev/ttyE0 2>/dev/tty1
#echo pppd died

echo starting shell on ttyE0
#setsid cttyhack sh -i </dev/ttyE0 >/dev/ttyE0 2>&1 &
/shonusb.sh

echo starting joytty
jscal -s 2,1,2,-2,-2,8658944,9941751,1,1,1,1,6972137,7254791 /dev/input/js0
joytty /dev/input/js0 /dev/tty1 &

echo mounting SDp1 at /mnt
mount -r /dev/mmcblk0p1 /mnt

echo timeslipping to current using DS1337 RTC at i2c-0:68
echo ds1337 0x68 > /sys/bus/i2c/devices/i2c-0/new_device
hwclock -s

echo welcome!
#login root </dev/ttyE0 >/dev/ttyE0 2>&1 &
while :; do setsid sh -c 'exec sh -i </dev/tty1 >/dev/tty1 2>&1'; done
