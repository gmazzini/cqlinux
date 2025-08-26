#!/bin/bash
while true; do
  if ! pgrep -x cqrun >/dev/null; then
    killall -9 wsjtx
    killall -9 jt9
    killall -9 cqrun
    sleep 2
    wsjtx &
    sleep 8
    /home/gmazzini/gm/cqrun &
  else
    sleep 10
  fi
done
