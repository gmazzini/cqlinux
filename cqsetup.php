<?php

$fpw=fopen("x4.txt","wt");
$gmwin=trim(shell_exec("xdotool search --onlyvisible --name 'K1JT'"));
fprintf($fpw,"gmwin=%s\n",$gmwin);
shell_exec("import -silent -window $gmwin x1.tif");
shell_exec("convert x1.tif -colorspace Gray -sharpen 0x1.0 x2.tif");
shell_exec("tesseract x2.tif x3 --psm 6 hocr");
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
fprintf($fpw,"gmenable='%d %d'\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

// Log
for($i=0;$i<$can;$i++)if($an[$i]["label"]=="Log")break;
if($i==$can){echo "Log not found\n"; exit(0);}
fprintf($fpw,"gmlog='%d %d'\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));
$gmlog=sprintf("%d %d",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

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
array_multisort($txv,$txi);
for($i=0;$i<$ctx;$i++)fprintf($fpw,"gmtx%d='%d %d'\n",$i+1,floor(($an[$txi[$i]]["x1"]+$an[$txi[$i]]["x2"])/2),floor(($an[$txi[$i]]["y1"]+$an[$txi[$i]]["y2"])/2));

// Call
for($i=0;$i<$can;$i++)if($an[$i]["label"]=="Call")break;
if($i==$can){echo "Call not found\n"; exit(0);}
fprintf($fpw,"gmcall='%d %d'\n",$an[$i]["x1"],2*$an[$i]["y2"]-$an[$i]["y1"]);

printf("xdotool mousemove --window $gmwin $gmlog click 1");

shell_exec("xdotool mousemove --window $gmwin $gmlog click 1");
exit(0);
sleep(2);
$gmlogwin=trim(shell_exec("xdotool search --onlyvisible --name 'Log'"));
fprintf($fpw,"gmlogwin=%s\n",$gmlogwin);
shell_exec("import -silent -window $gmlogwin x5.tif");
shell_exec("convert x5.tif -colorspace Gray -sharpen 0x1.0 x6.tif");
shell_exec("tesseract x6.tif x7 --psm 6 hocr");
$can=0;
$fp=fopen("x7.hocr","r");
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

// LogOK
for($i=0;$i<$can;$i++)if($an[$i]["label"]=="OK")break;
if($i==$can){echo "Log OK not found\n"; exit(0);}
fprintf($fpw,"gmlokok='%d %d'\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

// LogCANCEL
for($i=0;$i<$can;$i++)if($an[$i]["label"]=="Cancel")break;
if($i==$can){echo "Log Cancel not found\n"; exit(0);}
fprintf($fpw,"gmlocancel='%d %d'\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

// xdotool mousemove --window $gmlogwin $gmlogcancel click 1


fclose($fpw);
?>
