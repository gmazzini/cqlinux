<?php
include "login.php";
shell_exec("sudo etherwake e0:da:dc:6:4a:12");
sleep(3);
$ss=socket_create(AF_INET,SOCK_STREAM,0);
socket_connect($ss,"10.0.0.10",60000);
xx($ss,"##CN;",1);
xx($ss,"##ID00".sprintf("%d%d%s%s;",strlen($ts890s_login),strlen($ts890s_passwd),$ts890s_login,$ts890s_passwd),1);
xx($ss,"PS1;",0);
socket_close($ss);
function xx($ss,$mm,$an){
  @ socket_write($ss,$mm,strlen($mm));
  if($an){
    @ $rr=socket_read($ss,1024);
    return $rr;
  }
}
?>
