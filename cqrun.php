<?php
include "x4.php";
$aux=explode(" ",$gmenable); $aax=$aux[0]; $aay=$aux[1];
for(;;){
  shell_exec("import -silent -window $gmwin x10.tif");
  shell_exec("convert x10.tif -crop '10x10+$(($aax-5))+$(($aay-5))' +repage x11.tif");
  $aaout=trim(shell_exec("convert x11.tif -colorspace RGB -format '%[fx:mean.r] %[fx:mean.g] %[fx:mean.b]' info:"));
  $aux=explode(" ",$aaout); $aared=$aux[0]; $aagreen=$aux[1]; $aabu=$aux[2];
  if($aared<0.5 && $aagreen>0.1 && $aablu>0.1){
    $aaout=shell_exec("xdotool windowfocus $gmlogwin 2>&1");
    if(strpos($aaout,"Bad")===false){
      sleep(6);
      shell_exec("xdotool windowfocus --sync $gmlogwin mousemove --sync --window $gmlogwin $gmlogok click 1");
      sleep(6);      
    }
    shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmtx6 click 1");
    sleep(2);
    shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmenale click 1");
  }
  sleep(3);
}

?>