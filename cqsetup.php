<?php

$gmwin=trim(shell_exec("xdotool search --onlyvisible --name 'K1JT'"));
echo "gmwin=$gmwin\n";
shell_exec("import -silent -window $gmwin x1.tif");
shell_exec("convert x1.tif -set colorspace Gray -separate -average x2.tif");
shell_exec("tesseract x2.tif x3 hocr");
$an=array();
$can=0;
$fp=fopen("x3.hocr","r");
for(;;){
  if(feof($fp))break;
  $aux=fgets($fp);
  if(strpos($aux,"<span")===false)continue;
  $c1=strpos($aux,"title='");
  if($c1===false)continue;
  $c2=strpos($aux,"'",$c1+7);
  if($c2===false)continue;
  $o1=substr($aux,$c1+7,$c2-$c1-7);
  $o2=explode(" ",$o1);
  $an[$can]["x1"]=$o2[1];
  $an[$can]["y1"]=$o2[2];
  $an[$can]["x2"]=$o2[3];
  $an[$can]["y2"]=substr($o2[4],0,-1);
  $an[$can]["conf"]=$o2[6];
  $c1=strpos($aux,">",$c2+1);
  if($c1===false)continue;
  $c2=strpos($aux,"<",$c1+1);
  if($c2===false)continue;
  $o2=substr($aux,$c1+1,$c2-$c1-1);
  $an[$can]["label"]=$o2;
  $can++;
}
fclose($fp);

// Enable
for($i=0;$i<$can;$i++)if($an[$i]["label"]=="Enable")break;
if($i==$can){echo "Enable not found\n"; exit(0);}
$yb=$an[$i]["y2"];
printf("gmenable='%d %d'\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

// Log
for($i=0;$i<$can;$i++)if($an[$i]["label"]=="Log")break;
if($i==$can){echo "Log not found\n"; exit(0);}
printf("gmenable='%d %d'\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

// Tx
$con=0;
$itop=0; $top=0;
for($i=0;$i<$can;$i++)if($an[$i]["y1"]>$yb && $an[$i]["label"]=="Tx")$on[$con++]=$an[$i]["x1"];
for($i=0;$i<$con;$i++){
  $oc=0;
  for($j=0;$j<$con;$j++)if($on[$i]>$on[$j]-5 && $on[$i]<$on[$j]+5)$oc++;
  if($oc>$top){$top=$oc; $itop=$i;}
}
$ref=$on[$itop];
$ctx=0;
for($i=0;$i<$can;$i++)if($an[$i]["x1"]>$ref-5 && $an[$i]["x1"]<$ref+5 && $an[$i]["label"]<>"Now"){$txi[$ctx]=$i; $txv[$ctx]=$an[$i]["y1"]; $ctx++; }
if($ctx<6){echo "Tx only $ctx\n"; exit(0);}
array_multisort($tvx,$txi);
for($i=0;$i<$ctx;$i++)printf("gmtx%d='%d %d'\n",$i,floor(($an[$txi[$i]]["x1"]+$an[$txi[$i]]["x2"])/2),floor(($an[$txi[$i]]["y1"]+$an[$txi[$i]]["y2"])/2));






?>
