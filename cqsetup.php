<?php
echo "CQSETUP by IK4LZH v 1.4\n";
file_put_contents("x8.txt","ERROR");
$fpw=fopen("x4.php","wt");
fprintf($fpw,"<?php\n");
$gmwin=trim(shell_exec("xdotool search --onlyvisible --name 'K1JT'"));
fprintf($fpw,"\$gmwin=%s;\n",$gmwin);
shell_exec("import -silent -window $gmwin x1.tif");
// shell_exec("convert x1.tif -colorspace Gray -sharpen 0x1.0 x2.tif");
shell_exec("convert x1.tif -units pixelsperinch -density 300 -depth 8 x2.tif");
// shell_exec("tesseract x2.tif x3 --psm 6 hocr");
shell_exec("tesseract x2.tif x3 --psm 1 hocr");
$can=0;
$fp=fopen("x3.hocr","r");
for(;;){
  if(feof($fp))break;
  $aux=fgets($fp);
  if(stripos($aux,"<span")===false)continue;
  $c1=stripos($aux,"title='");
  if($c1===false)continue;
  $c2=stripos($aux,"'",$c1+7);
  if($c2===false)continue;
  $o1=substr($aux,$c1+7,$c2-$c1-7);
  $o2=explode(" ",$o1);
  $an[$can]["x1"]=$o2[1];
  $an[$can]["y1"]=$o2[2];
  $an[$can]["x2"]=$o2[3];
  $an[$can]["y2"]=substr($o2[4],0,-1);
  $an[$can]["conf"]=$o2[6];
  $c1=stripos($aux,">",$c2+1);
  if($c1===false)continue;
  $c2=stripos($aux,"<",$c1+1);
  if($c2===false)continue;
  $o2=substr($aux,$c1+1,$c2-$c1-1);
  $an[$can]["label"]=$o2;
  $can++;
}
fclose($fp);

// Enable
for($i=0;$i<$can;$i++)if(stripos($an[$i]["label"],"enable")!==false)break;
if($i==$can){echo "Enable not found\n"; exit(0);}
$yb=$an[$i]["y2"];
fprintf($fpw,"\$gmenable='%d %d';\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

// Log
for($i=0;$i<$can;$i++)if(stripos($an[$i]["label"],"log")!==false)break;
if($i==$can){echo "Log not found\n"; exit(0);}
fprintf($fpw,"\$gmlog='%d %d';\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));
$gmlog=sprintf("%d %d",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

// Tx
$con=0;
$itop=0; $top=0;
for($i=0;$i<$can;$i++)if($an[$i]["y1"]>$yb && stripos($an[$i]["label"],"tx")!==false)$on[$con++]=$an[$i]["x1"];
for($i=0;$i<$con;$i++){
  $oc=0;
  for($j=0;$j<$con;$j++)if($on[$i]>$on[$j]-5 && $on[$i]<$on[$j]+5)$oc++;
  if($oc>$top){$top=$oc; $itop=$i;}
}
$ref=$on[$itop];
$ctx=0;
for($i=0;$i<$can;$i++)if($an[$i]["x1"]>$ref-5 && $an[$i]["x1"]<$ref+5 && stripos($an[$i]["label"],"now")===false){$txi[$ctx]=$i; $txv[$ctx]=$an[$i]["y1"]; $ctx++; }
if($ctx<6){echo "Tx only $ctx\n"; exit(0);}
array_multisort($txv,$txi);
for($i=0;$i<$ctx;$i++)fprintf($fpw,"\$gmtx%d='%d %d';\n",$i+1,floor(($an[$txi[$i]]["x1"]+$an[$txi[$i]]["x2"])/2),floor(($an[$txi[$i]]["y1"]+$an[$txi[$i]]["y2"])/2));

// Call
for($i=0;$i<$can;$i++)if(stripos($an[$i]["label"],"call")!==false && $an[$i]["y1"]>$yb)break;
if($i==$can){echo "Call not found\n"; exit(0);}
fprintf($fpw,"\$gmcall='%d %d';\n",2*$an[$i]["x1"]-$an[$i]["x2"],4*$an[$i]["y2"]-3*$an[$i]["y1"]);

// Generate
for($i=0;$i<$can;$i++)if(stripos($an[$i]["label"],"generate")!==false && $an[$i]["y1"]>$yb)break;
if($i==$can){echo "Generate not found\n"; exit(0);}
fprintf($fpw,"\$gmgenerate='%d %d';\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

// Report
for($i=0;$i<$can;$i++)if(stripos($an[$i]["label"],"report")!==false && $an[$i]["y1"]>$yb)break;
if($i==$can){echo "Report not found\n"; exit(0);}
fprintf($fpw,"\$gmreport='%d %d';\n",$an[$i]["x2"],floor(($an[$i]["y1"]+$an[$i]["y2"])/2));
$gmrxx=$an[$i]["x1"];

// Rx Tx
for($i=0;$i<$can;$i++)if(stripos($an[$i]["label"],"hz")!==false && $an[$i]["y1"]>$yb)break;
if($i==$can){echo "Rx or Tx not found\n"; exit(0);}
for($j=0;$j<$can;$j++)if(stripos($an[$j]["label"],"hz")!==false && $an[$j]["y1"]>$an[$i]["y1"])break;
for($w=0;$w<$can;$w++)if(stripos($an[$w]["label"],"hz")!==false && $an[$w]["y1"]<$an[$i]["y1"])break;
if($j==$can && $w==$can){echo "Rx or Tx not found\n"; exit(0);}
if($j==$can)$aa=$w;
else $aa=$j;
if($an[$i]["y1"]<$an[$aa]["y1"]){$gmrxi=$aa; $gmtxi=$i; }
else {$gmrxi=$i; $gmtxi=$aa; }
fprintf($fpw,"\$gmrx='%d %d';\n",$gmrxx,floor(($an[$gmrxi]["y1"]+$an[$gmrxi]["y2"])/2));
fprintf($fpw,"\$gmtx='%d %d';\n",$gmrxx,floor(($an[$gmtxi]["y1"]+$an[$gmtxi]["y2"])/2));

// Even
// for($i=0;$i<$can;$i++)if(stripos($an[$i]["label"],"even")!==false && $an[$i]["y1"]>$yb)break;
// if($i==$can){echo "Even not found\n"; exit(0);}
// fprintf($fpw,"\$gmeven='%d %d';\n",$an[$i]["x1"]-floor(($an[$i]["x2"]-$an[$i]["x1"])/strlen($an[$i]["label"])*4),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

shell_exec("xdotool windowfocus --sync $gmwin mousemove --sync --window $gmwin $gmlog click 1");
sleep(2);
$gmlogwin=trim(shell_exec("xdotool search --onlyvisible --name 'Log'"));
fprintf($fpw,"\$gmlogwin=%s;\n",$gmlogwin);
shell_exec("import -silent -window $gmlogwin x5.tif");
shell_exec("convert x5.tif -colorspace Gray -sharpen 0x1.0 x6.tif");
shell_exec("tesseract x6.tif x7 --psm 6 hocr");
$can=0;
$fp=fopen("x7.hocr","r");
for(;;){
  if(feof($fp))break;
  $aux=fgets($fp);
  if(stripos($aux,"<span")===false)continue;
  $c1=stripos($aux,"title='");
  if($c1===false)continue;
  $c2=stripos($aux,"'",$c1+7);
  if($c2===false)continue;
  $o1=substr($aux,$c1+7,$c2-$c1-7);
  $o2=explode(" ",$o1);
  $an[$can]["x1"]=$o2[1];
  $an[$can]["y1"]=$o2[2];
  $an[$can]["x2"]=$o2[3];
  $an[$can]["y2"]=substr($o2[4],0,-1);
  $an[$can]["conf"]=$o2[6];
  $c1=stripos($aux,">",$c2+1);
  if($c1===false)continue;
  $c2=stripos($aux,"<",$c1+1);
  if($c2===false)continue;
  $o2=substr($aux,$c1+1,$c2-$c1-1);
  $an[$can]["label"]=$o2;
  $can++;
}
fclose($fp);

// LogOK
for($i=0;$i<$can;$i++)if(stripos($an[$i]["label"],"ok")!==false)break;
if($i==$can){echo "Log OK not found\n"; exit(0);}
for($j=0;$j<$can;$j++)if(stripos($an[$j]["label"],"ok")!==false && $an[$j]["y1"]>$an[$i]["y1"])break;
if($j<$can)$i=$j;
fprintf($fpw,"\$gmlogok='%d %d';\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

// LogCANCEL
for($i=0;$i<$can;$i++)if(stripos($an[$i]["label"],"cancel")!==false)break;
if($i==$can){echo "Log Cancel not found\n"; exit(0);}
fprintf($fpw,"\$gmlogcancel='%d %d';\n",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));
$gmlogcancel=sprintf("%d %d",floor(($an[$i]["x1"]+$an[$i]["x2"])/2),floor(($an[$i]["y1"]+$an[$i]["y2"])/2));

shell_exec("xdotool windowfocus --sync $gmlogwin mousemove --sync --window $gmlogwin $gmlogcancel click 1");

fprintf($fpw,"?>\n");
fclose($fpw);
file_put_contents("x8.txt","OK");
echo "All setups completed\n";
?>
