<?php
$target=(int)$argv[1];
$fp=fsockopen("10.0.0.41",10001,$errno,$errstr,5);
$mytarget=sprintf("%03d",$target);
fwrite($fp,"M$mytarget\r");
sleep(2);
fwrite($fp,"C\r");
$aux=fgets($fp,128);
$pos=strpos($aux,"AZ=");
$myold=intval(substr($aux,$pos+3,3));
sleep(10);
for(;;){
  fwrite($fp,"C\r");
  $aux=fgets($fp,128);
  $pos=strpos($aux,"AZ=");
  $myaz=intval(substr($aux,$pos+3,3));
  echo "A=$myaz O=$myold T=$mytarget\n";
  if(dist($myaz,$mytarget)<10)break;
  if(dist($myaz,$myold)<10){
    echo "Retry\n";
    fwrite($fp,"S\r");
    sleep(2);
    fwrite($fp,"M$mytarget\r");
    sleep(1);
  }
  $myold=$myaz;
  sleep(10);
}
fclose($fp);
echo "end\n";

function dist($f1,$f2){
  $vmax=max($f1,$f2);
  $vmin=min($f1,$f2);
  $aux=360-$vmax+$vmin;
  if($aux>180)$aux=360-$aux;
  return $aux;
}
?>
