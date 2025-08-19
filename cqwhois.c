void *whois_server_thread(){
  int server_fd,client_fd,opt;
  struct sockaddr_in addr;
  char buf[200],selcall[16],*out,*ll;
  ssize_t n;
  int i;

  out=(char *)malloc(60000*sizeof(char));
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
    buf[n-2]='\0';
    if(n==0){close(client_fd); continue;}
    ll=buf;
    if(strcmp(ll,"version")==0){
      sprintf(out,"Version: %s\n",version); 
      write(client_fd,out,strlen(out));
    }
    else if(strcmp(ll,"heartbeat")==0){
      sprintf(out,"Heartbeat: %s\n",mytime()); 
      write(client_fd,out,strlen(out));
    }
    else if(strcmp(ll,"used")==0){
      for(i=0;i<nused;i++){
        sprintf(out,"%d,%s,%d\n",i,used[i].call,used[i].times);
        write(client_fd,out,strlen(out));
      }
    }
    else if(strcmp(ll,"escluded")==0){
      for(i=0;i<nesc;i++){
        sprintf(out,"%d,%s\n",i,vesc[i]);
        write(client_fd,out,strlen(out));
      }
    }
    else if(strcmp(ll,"logged")==0){
      for(i=0;i<nlog;i++){
        sprintf(out,"%d,%s\n",i,vlog[i]);
        write(client_fd,out,strlen(out));
      }
    }
    else if(strcmp(ll,"rxed")==0){
      for(i=0;i<MAX_RXED;i++){
        if(rxed[i].msg[0]=='\0')continue;
        sprintf(out,"%d,%" PRIu32 ",%ld,%d,%3.1f,%" PRIu32 ",%s,%s,%d,%s,%" PRIu64 ",%d\n",i,rxed[i].ttime,rxed[i].time,rxed[i].snr,rxed[i].dt,rxed[i].df,rxed[i].mode,rxed[i].msg,rxed[i].LowConf,rxed[i].modeS,rxed[i].freqS,rxed[i].eoS);
        write(client_fd,out,strlen(out));
      }
    }
    else if(strcmp(ll,"cqed")==0){
      cqselection(selcall,&jsel,out);
      if(jsel>=0)sprintf(out+strlen(out),"# Selected jsel:%d call:%s\n",jsel,selcall);
       write(client_fd,out,strlen(out));
    }
    else {
      sprintf(out,"Unknow\n");
      write(client_fd,out,strlen(out));
    }
  
    close(client_fd);
  }
  close(server_fd);
  return NULL;
}
