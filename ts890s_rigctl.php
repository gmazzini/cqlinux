<?php
include "login.php";
$m["1"]="LSB"; $m["2"]="USB"; $m["3"]="CW"; $m["4"]="FM"; $m["5"]="AM"; $m["6"]="FSK";
$m["7"]="CWR"; $m["9"]="FSKR"; $m["A"]="PSK"; $m["B"]="PSKR"; $m["C"]="LSBD"; $m["D"]="USBD";
$m["E"]="FMD"; $m["F"]="AMD";
$sock1=socket_create(AF_INET,SOCK_STREAM,SOL_TCP);
socket_bind($sock1,"0.0.0.0",6789);
socket_listen($sock1,5);
$last=0;
for(;;){
  if(time()-$last>60){
    for(;;){
      for(;;){
        if($last>0)socket_close($sock2);
echo "NN\n";
        $last=time();
        $sock2=socket_create(AF_INET,SOCK_STREAM,0);
        socket_connect($sock2,"10.0.0.10",60000);
        $aux=trim(xx($sock2,"##CN;",1));
        if($aux=="##CN1;")break;
        sleep(1);
      }
      $aux=trim(xx($sock2,"##ID00".sprintf("%d%d%s%s;",strlen($ts890s_login),strlen($ts890s_passwd),$ts890s_login,$ts890s_passwd),1));
      if($aux=="##ID1;")break;
    }
  }
  $msg=socket_accept($sock1);
  $buf=trim(socket_read($msg,1024));
  $i=0;
  $split=0;
  for(;;){
    $c=substr($buf,$i,1);
    $i++;
    if($i>strlen($buf))break;
    switch($c){
      case "i":
        $aux=xx($sock2,($split)?"FB;":"FA;",1);
        $mm=substr($aux,2,-1)."\n";
        socket_write($msg,$mm,strlen($mm));
        break;
      case "f":
        $aux=xx($sock2,"FA;",1);
        $mm=substr($aux,2,-1)."\n";
        socket_write($msg,$mm,strlen($mm));
        break;
      case "m":
        $aux=xx($sock2,"OM0;",1);
        if(strlen($aux)>3)$mm=$m[substr($aux,3,1)]."\n0\n";
        else $mm="NONE\n0\n";
        socket_write($msg,$mm,strlen($mm));
        break;
      case "s":
        $aux=xx($sock2,"TB;",1);
        $mm=substr($aux,2,1)."\n0\n";
        $split=(int)$mm;
        socket_write($msg,$mm,strlen($mm));
        break;
      case "F":
        $aux=substr($buf,$i);
        $aux=explode(" ",$aux);
        xx($sock2,"FA".sprintf("%011d;\n",$aux[1]),0);
        break;
    }
  }
  socket_close($msg);
}
function xx($ss,$mm,$an){
  @ socket_write($ss,$mm,strlen($mm));
  if($an){
    @ $rr=socket_read($ss,1024);
    return $rr;
  }
}
?>
