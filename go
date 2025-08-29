#!/bin/bash
cd /home/gmazzini/gm
killall -9 wsjtx 2>/dev/null 
killall -9 jt9 2>/dev/null
killall -9 cqrun 2>/dev/null 
sleep 5
while true; do
  pid=$(pgrep -x wsjtx)
  if [ -z "$pid" ]; then
    killall -9 wsjtx 2>/dev/null 
    killall -9 jt9 2>/dev/null
    killall -9 cqrun 2>/dev/null
    sleep 3
    wsjtx &
    sleep 10
    ./cqrun &
    sleep 5
  else
    sleep 10
    runtime=$(ps -o etimes= -p "$pid")
    if [ "$runtime" -gt 28800 ]; then
       whois -h 127.0.0.1 -p 4343 set 55Jwb4VQtwK1 exit
       sleep 5
       killall -9 wsjtx 2>/dev/null 
       killall -9 jt9 2>/dev/null
       killall -9 cqrun 2>/dev/null
       sleep 3
    fi
  fi
done
