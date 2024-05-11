<?php
$g1="JN54";
include "x4.php";
$black=("IK4LZH");

// Read actual activity
$done=array(); $cq=array();
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
$black=file("black.txt",FILE_IGNORE_NEW_LINES);
foreach($cq as $k => $v){
  $aux=explode("_",$k);
  if(in_array($aux[0],$black))unset($cq[$k]);
}

// Scoring
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
asort($sel);

$i=0;
foreach($sel as $k => $v){
  printf("%s %.0f %s\n",$k,$v,$cq[$k]);
  if(++$i>0)break;
}

$aux=explode("_",$k);
$call=$aux[0];
shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmcall click --repeat 5 1 key Delete key Delete key Delete key Delete key Delete key Delete key Delete key Delete key Delete key Delete key Delete type '$call'");
$aux=explode("_",$cq[$k]);
if($aux[2]<0)$rx=sprintf("\%d",$aux[2]);
else $rx=sprintf("%d",$aux[2]);
$tx=$aux[3];

shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmreport click --repeat 5 1 key Delete key Delete key Delete key Delete key type '$rx'");
shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmrx click --repeat 5 1 key Delete key Delete key Delete key Delete key type '$tx'");

?>
