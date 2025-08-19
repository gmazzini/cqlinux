void *whois_server_thread(){
  int server_fd,client_fd,opt;
  struct sockaddr_in addr;
  char buf[200],out[300],*ll;
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
    buf[n-1]='\0';
    if(n==0){close(client_fd); continue;}
    ll=buf;
    sprintf(out,">>Input: %s<<\n",ll); write(client_fd,out,strlen(out));
    if(strcmp(ll,"version")==0){sprintf(out,"Version: %s\n",version); write(client_fd,out,strlen(out));}
    if(strcmp(ll,"heartbeat")==0){sprintf(out,"Heartbeat: %s\n",mytime()); write(client_fd,out,strlen(out));}
    close(client_fd);
  }
  close(server_fd);
  return NULL;
}
