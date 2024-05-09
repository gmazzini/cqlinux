<?php

$gmwin=trim(shell_exec("xdotool search --onlyvisible --name 'K1JT'"));
echo $gmwin."\n";
shell_exec("import -silent -window $gmwin x1.tif");
shell_exec("convert x1.tif -set colorspace Gray -separate -average x2.tif");
shell_exec("tesseract x2.tif x3 hocr");
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
  echo "$o1\n";
  $c1=strpos($aux,">",$c2+1);
  if($c1===false)continue;
  $c2=strpos($aux,"<",$c1+1);
  if($c2===false)continue;
  $o2=substr($aux,$c1+1,$c2-$c1-1);
  echo "$o2\n";
}
fclose($fp);
?>
