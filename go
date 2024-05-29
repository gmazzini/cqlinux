#|/bin/bash
killall -9 wsjtx
killall -9 jt9
wsjtx &
sleep 10
php cqlinux/cqsetup.php
sleep 2
if grep -q OK "$File"; then
  php cqlinux/cqrun.php
fi
