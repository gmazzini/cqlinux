<?php
include "x4.php";
$g1="JN54";
$called[0]="IK4LZH"; $called[1]="IK4LZH"; $called[2]="IK4LZH";
$calledv=0;
$aux=explode(" ",$gmenable); $aax=$aux[0]-5; $aay=$aux[1]-5;
for(;;){
  shell_exec("import -silent -window $gmwin x10.tif");
  shell_exec("convert x10.tif -crop '10x10+$aax+$aay' +repage x11.tif");
  $aaout=trim(shell_exec("convert x11.tif -colorspace RGB -format '%[fx:mean.r] %[fx:mean.g] %[fx:mean.b]' info:"));
  $aux=explode(" ",$aaout); $aared=$aux[0]; $aagreen=$aux[1]; $aablu=$aux[2];
  if($aared<0.5 && $aagreen>0.1 && $aablu>0.1){
    echo "STATUS: Enable Off\n";
    $aaout=shell_exec("xdotool windowfocus $gmlogwin 2>&1");
    if(strpos($aaout,"Bad")===false){
      sleep(6);
      shell_exec("xdotool windowfocus --sync $gmlogwin mousemove --sync --window $gmlogwin $gmlogok click 1");
      echo "SET: Log Ok\n";
      sleep(6);      
    }
 
    // Read actual activity
    unset($done); $done=array();
    unset($cq); $cq=array();
    unset($sel); $sel=array();
    $fp=fopen("/home/gmazzini/.local/share/WSJT-X/ALL.TXT","r");
    for(;;){
      if(feof($fp))break;
      $aux=fgets($fp);
      $zz=preg_split('/\s+/',$aux);
      if(@$zz[2]=="Tx" && (@$zz[9]=="73"||@$zz[9]=="RR73")){
        $bb=explode(".",$zz[1]);
        $cc=$zz[7]."_".$bb[0].$zz[3];
        $done[$cc]=$zz[0];
      }
      if(@$zz[2]=="Rx" && $zz[7]=="CQ"){
        $bb=explode(".",$zz[1]);
        $ma=(strlen($zz[8])<3)?1:0;
        $cc=$zz[8+$ma]."_".$bb[0].$zz[3];
        $cq[$cc]=$zz[0]."_".$zz[4]."_".$zz[6]."_".$zz[9+$ma];
      }
    }
    fclose($fp);
    
    // Processing exclusions
    foreach($done as $k => $v)unset($cq[$k]);
    $black=file("cqlinux/wsjtx_black.txt",FILE_IGNORE_NEW_LINES);
    foreach($cq as $k => $v){
      $aux=explode("_",$k);
      if(in_array($aux[0],$black))unset($cq[$k]);
      else if(in_array($aux[0],$called))unset($cq[$k]);
    }
    
    // Scoring
    if(count($cq)>0){
      $ff=strftime("%y%m%d_%H%M%S");
      $vff=mktime(substr($ff,7,2),substr($ff,9,2),substr($ff,11,2),substr($ff,2,2),substr($ff,4,2),substr($ff,0,2));
      $x1lat=(ord(substr($g1,1,1))-65)*10+(int)substr($g1,3,1)+1/48-90;
      $x1lon=-((ord(substr($g1,0,1))-65)*20+(int)substr($g1,2,1)*2+1/24-180);
      $lat1=(float)$x1lat*M_PI/180;
      $lon1=(float)$x1lon*M_PI/180;
      foreach($cq as $k => $v){
        $aux=explode("_",$v);
        $ff=$aux[0]."_".$aux[1];
        $aff=mktime(substr($ff,7,2),substr($ff,9,2),substr($ff,11,2),substr($ff,2,2),substr($ff,4,2),substr($ff,0,2));
        $g2=$aux[4];
        $x2lat=(ord(substr($g2,1,1))-65)*10+(int)substr($g2,3,1)+1/48-90;
        $x2lon=-((ord(substr($g2,0,1))-65)*20+(int)substr($g2,2,1)*2+1/24-180);
        $lat2=(float)$x2lat*M_PI/180;
        $lon2=(float)$x2lon*M_PI/180;
        $a=pow(sin(($lat1-$lat2)/2),2)+cos($lat1)*cos($lat2)*pow(sin(($lon1-$lon2)/2),2);
        $dist=6371*2*atan2(sqrt($a),sqrt(1-$a));
        $sel[$k]=($vff-$aff)+1000/(30+$aux[2])+100000/($dist+0.1);
      }
    }
    
    // Selection & click
    $top=""; $topv=100000;
    foreach($sel as $k => $v){
      if($v<300 && $v<$topv){
        $top=$k;
        $topv=$v;
      }
    }
    if($topv<300){
      printf("SET: %s %d %s\n",$top,$topv,$cq[$top]);
      $aux=explode("_",$top);
      $call=$aux[0];
      $called[$calledv++]=$call;
      if($calledv>=3)$calledv=0;
      shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmcall click --repeat 5 1 key Delete key Delete key Delete key Delete key Delete key Delete key Delete key Delete key Delete key Delete key Delete type '$call'");
      sleep(1);
      $aux=explode("_",$cq[$top]);
      if($aux[2]<0)$rx=sprintf("\%d",$aux[2]);
      else $rx=sprintf("%d",$aux[2]);
      $tx=$aux[3];
      shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmreport click --repeat 5 1 key Delete key Delete key Delete key Delete key type '$rx'");
      sleep(1);
      shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmrx click --repeat 5 1 key Delete key Delete key Delete key Delete key type '$tx'");
      sleep(1);
      shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmtx1 click 1");
      echo "SET: Tx1\n";
    }
    else {
      shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmtx6 click 1");
      echo "SET: Tx6\n";
    }
    sleep(2);
    shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmenable click 1");
    echo "SET: Enable On\n";
  }
  sleep(3);
}

?>
