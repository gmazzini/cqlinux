<?php
// $ff="/home/gmazzini/.local/share/WSJT-X/ALL.TXT";
$ff="ALL.TXT";

$start="20240504_000000";
$done=array();
$cq=array();
$fp=fopen($ff,"r");
for(;;){
  if(feof($fp))break;
  $aux=fgets($fp);
  $zz=preg_split('/\s+/',$aux);
  if($zz[0]<$start)continue;
 // echo $aux;
  if(@$zz[2]=="Tx" && (@$zz[9]=="73"||@$zz[9]=="RR73")){
    $bb=explode(".",$zz[1]);
    $cc=$zz[7]."_".$bb[0].$zz[3];
    $done[$cc]=$zz[0];
  }
  if(@$zz[2]=="Rx" && $zz[7]=="CQ"){
    $bb=explode(".",$zz[1]);
    $ma=(in_array($zz[8],array("DX","NA","AS","OC","ZL","VK")))?1:0;
    $cc=$zz[8+$ma]."_".$bb[0].$zz[3];
    $cq[$cc]=$zz[0]."_".$zz[4]."_".$zz[6]."_".$zz[9+$ma];
  }
}
fclose($fp);

foreach($done as $k => $v){
  unset($cq[$k]);
}
$ff="240504_034200";
$vff=mktime(substr($ff,7,2),substr($ff,9,2),substr($ff,11,2),substr($ff,2,2),substr($ff,4,2),substr($ff,0,2));
foreach($cq as $k => $v){
  $aux=explode("_",$v);
  $ff=$aux[0]."_".$aux[1];
  echo $ff."\n";
  $aff=mktime(substr($ff,7,2),substr($ff,9,2),substr($ff,11,2),substr($ff,2,2),substr($ff,4,2),substr($ff,0,2));
  $sel[$k]=($vff-$aff)+1000/(30+$aux[2]);
}


print_r($done);
asort($cq);
print_r($cq);
arsort($sel);
print_r($sel);
?>
