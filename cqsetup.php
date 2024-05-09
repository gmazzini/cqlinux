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
  $c2=strpos($aux,"'",$c1+1);
  $o1=substr($aux,$c1+7,$c2-$c1);
  echo "$o1\n";
}
fclose($fp);
?>
