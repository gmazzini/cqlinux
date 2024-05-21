#|/bin/bash
killall -9 wsjtx
sleep 2
php cqlinux/cqsetup.php
sleep 2
php cqlinux/cqrun.php
