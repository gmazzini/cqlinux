void *whois_server_thread(){
  int server_fd,client_fd,opt;
  struct sockaddr_in addr;
  char buf[200];
  ssize_t n;

  server_fd=socket(AF_INET,SOCK_STREAM,0);
  setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=INADDR_ANY;
  addr.sin_port=htons(4343);
  bind(server_fd,(struct sockaddr *)&addr,sizeof(addr));
  listen(server_fd,5);
  for(;;){
    client_fd=accept(server_fd,NULL,NULL);
    if(client_fd<0)continue;
    n=read(client_fd,buf,199);
    if(n>0){
      buf[n]='\0';
      sprintf(buf,"ciao\n"); write(client_fd,buf,strlen(buf));
    }    
    close(client_fd);
  }
  close(server_fd);
  return NULL;
}
