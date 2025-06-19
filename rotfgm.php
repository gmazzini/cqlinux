<?php
$aa=sprintf("%03d",$argv[1]);
$socket=socket_create(AF_INET,SOCK_STREAM,SOL_TCP);
socket_connect($socket,"10.200.0.6",10001);
socket_write($socket,chr(2)."AG".$aa."\r",7);
socket_close($socket);
echo "Tartget $aa\n";

for (;;){
  $socket=socket_create(AF_INET,SOCK_STREAM,SOL_TCP);
  socket_connect($socket,"10.200.0.6",10001);
  socket_write($socket,chr(2)."A?"."\r",4);
  sleep(1);
  $aux=trim(socket_read($socket,100,PHP_NORMAL_READ));
  $ee=explode(",",$aux);
  if(isset($ee[2]))$pos=(int)$ee[2];
  sleep(1);
  socket_close($socket);
  if(isset($ee[2])&&$ee[1]=="?"){
    echo "$aux $pos\n";
    if(abs($pos-(int)$aa)<5)break;
  }
}

?>
