#!/bin/bash
killall -9 cqrun
sleep 5
while true; do
  if ! pgrep -x cqrun >/dev/null; then
    killall -9 wsjtx
    killall -9 jt9
    killall -9 cqrun
    sleep 2
    wsjtx &
    sleep 8
    /home/gmazzini/gm/cqrun &
    sleep 5
  else
    sleep 10
    start_epoch=$(whois -h 127.0.0.1 -p 4343 read 11 | head -n 1)
    now_epoch=$(date +%s)
    delta=$(( now_epoch - start_epoch ))
    if [ "$delta" -gt 28800 ]; then
       killall -9 cqrun
       sleep 5
    fi
  fi
done
