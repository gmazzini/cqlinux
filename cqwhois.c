void *whois_server_thread(){
  int server_fd,client_fd,opt,i,j,e,jsel,occ,to;
  struct sockaddr_in addr;
  char buf[200],selcall[16],*out,*ll,*token,myout[BUF_SIZE],*q;
  ssize_t n;
  uint8_t busy[3200];
  time_t rawtime;
  struct timespec ts;
  
  ts.tv_sec=0; ts.tv_nsec=50*1000000L;
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
      time(&rawtime);
      sprintf(out,"Release: %s\nCompiled on: %s %s\nWSJTX Version: %s\nStarted: %lu sec ago\n",RELEASE,__DATE__,__TIME__,version,(unsigned long)(rawtime-tstart)); 
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
      sprintf(out,"# nrxed=%" PRIu32 "\n",nrxed);
      write(client_fd,out,strlen(out));
    }
    else if(strcmp(ll,"cqed")==0){
      cqselection(selcall,&jsel,out);
      if(jsel>=0)sprintf(out+strlen(out),"# Selected jsel:%d call:%s\n",jsel,selcall);
      write(client_fd,out,strlen(out));
    }
    else if(strcmp(ll,"status")==0){
      sprintf(out,"lastfreq=%" PRIu64 " lastmode=%s enabletx=%d lasteo=%d rxdf=%" PRIu32 " txdf=%" PRIu32 "\n",lastfreq,lastmode,enabletx,lasteo,rxdf,txdf);
      write(client_fd,out,strlen(out));
    }
    else if(strcmp(ll,"freefreq")==0){
      occ=0;
      time(&rawtime);
      for(j=0;j<3200;j++)busy[j]=0;
      if(strcmp(lastmode,"FT4")==0)occ=100;
      else if(strcmp(lastmode,"FT8")==0)occ=50;
      for(i=0;i<MAX_RXED;i++){
        if(strcmp(rxed[i].modeS,lastmode)!=0)continue;
        if((int)(rxed[i].freqS/1000000)!=(int)(lastfreq/1000000))continue;
        if(rawtime-rxed[i].time>300)continue;
        for(e=rxed[i].df+occ,j=rxed[i].df;j<e;j++)busy[j]=1;
      }
      busy[199]=1;
      busy[3000]=1;
      for(j=200;j<=3000;j++){
        if(busy[j-1]==1 && busy[j]==0)e=j;
        else if(busy[j-1]==0 && busy[j]==1 && j-e>=occ){sprintf(out,"%d-%d\n",e,j-1); write(client_fd,out,strlen(out)); }
      }
    }
    else if(strncmp(ll,"read",4)==0){
      token=strtok(ll," "); if(token==NULL)goto go21;
      token=strtok(NULL," "); if(token==NULL)goto go21;
      i=atoi(token);
      switch(i){
        case 1: sprintf(out,"%d\n# nlog\n",nlog); break;
        case 2: sprintf(out,"%d\n# nesc\n",nesc); break;
        case 3: sprintf(out,"%d\n# nused\n",nused); break;
        case 4: sprintf(out,"%" PRIu64 "\n# heartbeat\n",(uint64_t)heartbeat); break;
        case 5: sprintf(out,"%d\n# lasteo\n",lasteo); break;
        case 6: sprintf(out,"%d\n# enabletx\n",enabletx); break;
        case 7: sprintf(out,"%" PRIu32 "\n# nrxed\n",(uint32_t)nrxed); break;
        case 8: sprintf(out,"%" PRIu32 "\n# rxdf\n",(uint32_t)rxdf); break;
        case 9: sprintf(out,"%" PRIu32 "\n# txdf\n",(uint32_t)txdf); break;
        case 10: sprintf(out,"%" PRIu64 "\n# lastfreq\n",(uint64_t)lastfreq); break;
        case 11: sprintf(out,"%" PRIu64 "\n# tstart\n",(uint64_t)tstart); break;
        case 12: sprintf(out,"%" PRIu64 "\n# tlastlogged\n",(uint64_t)tlastlogged); break;
        default: sprintf(out,"# tbd\n"); break;
      }
      write(client_fd,out,strlen(out));
      go21:
    }
    else if(strncmp(ll,"set",3)==0){
      token=strtok(ll," "); if(token==NULL)goto go20;
      token=strtok(NULL," "); if(token==NULL)goto go20;
      if(strcmp(token,KEY)==0){
        token=strtok(NULL," "); if(token==NULL)goto go20;
        sprintf(out,"set: %s\n",token); write(client_fd,out,strlen(out));
        if(strcmp(token,"odd")==0){
          emulate(XK_Control_L,XK_E,2,wbase);
        }
        else if(strcmp(token,"even")==0){
          emulate(XK_Shift_L,XK_E,2,wbase);
        }
        else if(strcmp(token,"ft8")==0){
          q=myout;
          Wu32(0xadbccbda,&q); Wu32(2,&q); Wu32(15,&q); 
          Ws("GM1",&q); Ws("FT8",&q); Wu32(0xffffffff,&q); Ws("",&q); Wb(0,&q); 
          Wu32(0xffffffff,&q); Wu32(0xffffffff,&q); Ws("",&q); Ws("",&q); Wb(1,&q);
          sendto(sock,myout,q-myout,0,(struct sockaddr*)&sender_addr,sizeof(addr));
        }
        else if(strcmp(token,"ft4")==0){
          q=myout;
          Wu32(0xadbccbda,&q); Wu32(2,&q); Wu32(15,&q); 
          Ws("GM1",&q); Ws("FT4",&q); Wu32(0xffffffff,&q); Ws("",&q); Wb(0,&q); 
          Wu32(0xffffffff,&q); Wu32(0xffffffff,&q); Ws("",&q); Ws("",&q); Wb(1,&q);
          sendto(sock,myout,q-myout,0,(struct sockaddr*)&sender_addr,sizeof(addr));
        }
        else if(token[0]>='0' && token[0]<='9'){
          occ=0;
          if(strcmp(lastmode,"FT4")==0)occ=90;
          else if(strcmp(lastmode,"FT8")==0)occ=60;
          if(occ==0)goto go20;
          i=atoi(token);
          if(i>(int)txdf){
            e=(i-txdf)/occ; to=txdf+e*occ;
            for(j=0;j<e;j++){emulate(XK_Shift_L,XK_F12,2,wbase); nanosleep(&ts,NULL);}
            sprintf(out,"new txdf=%d\n",to); write(client_fd,out,strlen(out));
          }
          else {
            e=(txdf-i)/occ; to=txdf-e*occ;
            for(j=0;j<e;j++){emulate(XK_Shift_L,XK_F11,2,wbase); nanosleep(&ts,NULL);}
            sprintf(out,"new txdf=%d\n",to); write(client_fd,out,strlen(out));
          }
        }
      }
      go20:
    }
    else {
      sprintf(out,"Unknown\n");
      write(client_fd,out,strlen(out));
    }
  
    close(client_fd);
  }
  close(server_fd);
  return NULL;
}
