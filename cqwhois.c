int whois_readline(int fd,char *buf,int max){
  char c;
  int i;
  ssize_t n;

  if(buf==NULL || max<2)return -1;

  i=0;
  for(;;){
    n=read(fd,&c,1);
    if(n<=0){
      if(i==0)return -1;
      break;
    }
    if(c=='\n')break;
    if(c=='\r')continue;
    if(i<max-1)buf[i++]=c;
  }

  buf[i]='\0';
  return 0;
}

void whois_write(int fd,char *s){
  if(s==NULL)return;
  write(fd,s,strlen(s));
}

void whois_send_wsjtx_mode(char *mode){
  char myout[BUF_SIZE],*q;

  q=myout;
  Wu32(0xadbccbda,&q);
  Wu32(2,&q);
  Wu32(15,&q);
  Ws("GM1",&q);
  Ws(mode,&q);
  Wu32(0xffffffff,&q);
  Ws("",&q);
  Wb(0,&q);
  Wu32(0xffffffff,&q);
  Wu32(0xffffffff,&q);
  Ws("",&q);
  Ws("",&q);
  Wb(1,&q);
  sendto(sock,myout,q-myout,0,(struct sockaddr*)&sender_addr,sizeof(sender_addr));
}

int whois_wsjtx_key(KeySym mod,KeySym key,char *out,int client_fd,char *name){
  winid();

  if(wbase==0){
    sprintf(out,"%s: WSJT-X main window not found\n",name);
    whois_write(client_fd,out);
    return 0;
  }

  emulate(mod,key,2,wbase);

  sprintf(out,"%s: sent\n",name);
  whois_write(client_fd,out);
  return 1;
}

void whois_help(int fd,char *out){
  sprintf(out,
    "Usage:\n"
    "  KEY help\n"
    "  KEY version\n"
    "  KEY heartbeat\n"
    "  KEY status\n"
    "  KEY rxed\n"
    "  KEY cqed\n"
    "  KEY freefreq\n"
    "  KEY used\n"
    "  KEY logged\n"
    "  KEY escluded\n"
    "  KEY excluded\n"
    "  KEY read N\n"
    "  set KEY odd\n"
    "  set KEY even\n"
    "  set KEY ft8\n"
    "  set KEY ft4\n"
    "  set KEY txup\n"
    "  set KEY txdw\n"
    "  set KEY txdown\n"
    "  set KEY exit\n");
  whois_write(fd,out);
}

void *whois_server_thread(){
  int server_fd,client_fd,opt,i,j,e,jsel,occ,from,to2,is_set;
  struct sockaddr_in addr;
  struct timeval tv;
  char buf[200],work[200],selcall[16],*out,*cmd,*arg,*token;
  uint8_t busy[3200];
  time_t rawtime;

  out=(char *)malloc(60000*sizeof(char));
  if(out==NULL)return NULL;

  for(;;){
    server_fd=socket(AF_INET,SOCK_STREAM,0);
    if(server_fd<0){
      sleep(1);
      continue;
    }

    opt=1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEPORT,&opt,sizeof(opt));
#endif

    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(4343);

    if(bind(server_fd,(struct sockaddr *)&addr,sizeof(addr))<0){
      close(server_fd);
      sleep(1);
      continue;
    }

    if(listen(server_fd,5)<0){
      close(server_fd);
      sleep(1);
      continue;
    }

    for(;;){
      client_fd=accept(server_fd,NULL,NULL);
      if(client_fd<0)continue;

      tv.tv_sec=5;
      tv.tv_usec=0;
      setsockopt(client_fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

      if(whois_readline(client_fd,buf,sizeof(buf))<0){
        close(client_fd);
        continue;
      }

      snprintf(work,sizeof(work),"%s",buf);

      cmd=NULL;
      arg=NULL;
      is_set=0;

      token=strtok(work," \t");
      if(token==NULL){
        sprintf(out,"Forbidden\n");
        whois_write(client_fd,out);
        close(client_fd);
        continue;
      }

      if(strcmp(token,"set")==0){
        is_set=1;
        token=strtok(NULL," \t");
        if(token==NULL || strcmp(token,KEY)!=0){
          sprintf(out,"Forbidden\n");
          whois_write(client_fd,out);
          close(client_fd);
          continue;
        }
        cmd=strtok(NULL," \t");
        arg=strtok(NULL," \t");
      }
      else {
        if(strcmp(token,KEY)!=0){
          sprintf(out,"Forbidden\n");
          whois_write(client_fd,out);
          close(client_fd);
          continue;
        }
        cmd=strtok(NULL," \t");
        arg=strtok(NULL," \t");
      }

      if(cmd==NULL){
        sprintf(out,"Unknown\n");
        whois_write(client_fd,out);
        close(client_fd);
        continue;
      }

      if(strcmp(cmd,"help")==0){
        whois_help(client_fd,out);
      }

      else if(strcmp(cmd,"version")==0){
        time(&rawtime);
        sprintf(out,"Release: %s\nCompiled on: %s %s\nWSJTX Version: %s\nStarted: %lu sec ago\n",RELEASE,__DATE__,__TIME__,version,(unsigned long)(rawtime-tstart));
        whois_write(client_fd,out);
      }

      else if(strcmp(cmd,"heartbeat")==0){
        sprintf(out,"Heartbeat: %s\n",mytime());
        whois_write(client_fd,out);
      }

      else if(strcmp(cmd,"used")==0){
        pthread_mutex_lock(&usedlock);
        for(i=0;i<nused;i++){
          sprintf(out,"%d,%s,%d\n",i,used[i].call,used[i].times);
          whois_write(client_fd,out);
        }
        pthread_mutex_unlock(&usedlock);
      }

      else if(strcmp(cmd,"escluded")==0 || strcmp(cmd,"excluded")==0){
        pthread_mutex_lock(&esclock);
        for(i=0;i<nesc;i++){
          sprintf(out,"%d,%s\n",i,vesc[i]);
          whois_write(client_fd,out);
        }
        pthread_mutex_unlock(&esclock);
      }

      else if(strcmp(cmd,"logged")==0){
        pthread_mutex_lock(&loglock);
        for(i=0;i<nlog;i++){
          sprintf(out,"%d,%s\n",i,vlog[i]);
          whois_write(client_fd,out);
        }
        pthread_mutex_unlock(&loglock);
      }

      else if(strcmp(cmd,"rxed")==0){
        pthread_mutex_lock(&rxlock);
        for(i=0;i<MAX_RXED;i++){
          if(rxed[i].msg[0]=='\0')continue;
          sprintf(out,"%d,%" PRIu32 ",%ld,%d,%3.1f,%" PRIu32 ",%s,%s,%d,%s,%" PRIu64 ",%d\n",i,rxed[i].ttime,rxed[i].time,rxed[i].snr,rxed[i].dt,rxed[i].df,rxed[i].mode,rxed[i].msg,rxed[i].LowConf,rxed[i].modeS,rxed[i].freqS,rxed[i].eoS);
          whois_write(client_fd,out);
        }
        sprintf(out,"# nrxed=%" PRIu32 "\n",nrxed);
        pthread_mutex_unlock(&rxlock);
        whois_write(client_fd,out);
      }

      else if(strcmp(cmd,"cqed")==0){
        out[0]='\0';
        cqselection(selcall,&jsel,out);
        if(jsel>=0)sprintf(out+strlen(out),"# Selected jsel:%d call:%s\n",jsel,selcall);
        whois_write(client_fd,out);
      }

      else if(strcmp(cmd,"status")==0){
        sprintf(out,"lastfreq=%" PRIu64 " lastmode=%s enabletx=%d lasteo=%d rxdf=%" PRIu32 " txdf=%" PRIu32 "\n",lastfreq,lastmode,enabletx,lasteo,rxdf,txdf);
        whois_write(client_fd,out);
      }

      else if(strcmp(cmd,"freefreq")==0){
        occ=0;
        time(&rawtime);
        for(j=0;j<3200;j++)busy[j]=0;

        if(strcmp(lastmode,"FT4")==0)occ=100;
        else if(strcmp(lastmode,"FT8")==0)occ=50;

        if(occ>0){
          pthread_mutex_lock(&rxlock);
          for(i=0;i<MAX_RXED;i++){
            if(strcmp(rxed[i].modeS,lastmode)!=0)continue;
            if((int)(rxed[i].freqS/1000000)!=(int)(lastfreq/1000000))continue;
            if(rawtime-rxed[i].time>300)continue;

            from=(int)rxed[i].df;
            to2=from+occ;

            if(from<0)from=0;
            if(from>3199)from=3199;
            if(to2<0)to2=0;
            if(to2>3199)to2=3199;

            for(j=from;j<to2;j++)busy[j]=1;
          }
          pthread_mutex_unlock(&rxlock);

          busy[199]=1;
          busy[3000]=1;

          e=200;
          for(j=200;j<=3000;j++){
            if(busy[j-1]==1 && busy[j]==0)e=j;
            else if(busy[j-1]==0 && busy[j]==1 && j-e>=occ){
              sprintf(out,"%d-%d\n",e,j-1);
              whois_write(client_fd,out);
            }
          }
        }
      }

      else if(strcmp(cmd,"read")==0){
        if(arg==NULL){
          sprintf(out,"Unknown\n");
          whois_write(client_fd,out);
        }
        else {
          i=atoi(arg);
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
          whois_write(client_fd,out);
        }
      }

      else if(is_set){
        sprintf(out,"set: %s\n",cmd);
        whois_write(client_fd,out);

        if(strcmp(cmd,"odd")==0){
          winid();
          if(wbase==0){
            sprintf(out,"odd: WSJT-X main window not found\n");
            whois_write(client_fd,out);
          }
          else emulate(XK_Control_L,XK_E,2,wbase);
        }

        else if(strcmp(cmd,"even")==0){
          winid();
          if(wbase==0){
            sprintf(out,"even: WSJT-X main window not found\n");
            whois_write(client_fd,out);
          }
          else emulate(XK_Shift_L,XK_E,2,wbase);
        }

        else if(strcmp(cmd,"ft8")==0){
          whois_send_wsjtx_mode("FT8");
        }

        else if(strcmp(cmd,"ft4")==0){
          whois_send_wsjtx_mode("FT4");
        }

        else if(strcmp(cmd,"txup")==0){
          whois_wsjtx_key(XK_Shift_L,XK_F12,out,client_fd,"txup");
        }

        else if(strcmp(cmd,"txdw")==0 || strcmp(cmd,"txdown")==0){
          whois_wsjtx_key(XK_Shift_L,XK_F11,out,client_fd,"txdw");
        }

        else if(strcmp(cmd,"exit")==0){
          winid();
          if(wbase==0){
            sprintf(out,"exit: WSJT-X main window not found\n");
            whois_write(client_fd,out);
          }
          else {
            emulate(XK_Alt_L,XK_F4,2,wbase);
            sleep(2);
          }
          exit(0);
        }

        else {
          sprintf(out,"Unknown set command\n");
          whois_write(client_fd,out);
        }
      }

      else {
        sprintf(out,"Unknown\n");
        whois_write(client_fd,out);
      }

      close(client_fd);
    }

    close(server_fd);
    sleep(1);
  }

  return NULL;
}
