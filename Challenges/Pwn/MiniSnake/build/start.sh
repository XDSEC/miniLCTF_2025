#!/bin/sh

echo $FLAG > /flag
export FLAG=
export TERM=xterm
chroot --userspec=ctf:ctf / socat TCP-LISTEN:9999,reuseaddr,fork EXEC:'/minisnake',pty