<?php

source x4
gmx=`echo $gmenable | awk -F" " '{print $1}'`
gmy=`echo $gmenable | awk -F" " '{print $2}'`

while true
do
  import -silent -window $gmwin x10.tif
  convert x10.tif -crop "10x10+$(($gmx-5))+$(($gmy-5))" +repage x11.tif
  aux=`convert x11.tif -colorspace RGB -format "%[fx:mean.r] %[fx:mean.g] %[fx:mean.b]" info:`
  gmred=`echo $aux | awk -F" " '{print $1}'`
  gmgreen=`echo $aux | awk -F" " '{print $2}'`
  gmblu=`echo $aux | awk -F" " '{print $3}'`
  if (( $(echo "$gmred<0.5"|bc -l) && $(echo "$gmgreen>0.1"|bc -l) && $(echo "$gmblu>0.1"|bc -l) )); then
    aux=`xdotool windowfocus $gmlogwin 2>&1 | grep Bad | wc -l`
    if (( ! $aux )); then
      sleep 6
      xdotool mousemove --window $gmlogwin $gmlogok click 1
      sleep 6 
    fi
    xdotool mousemove --window $gmwin $gmtx6 click 1
    sleep 2
    xdotool mousemove --window $gmwin $gmenable click 1
  fi
  sleep 3
done
  ?>
