void *whois_server_thread(void *arg){
  int client_fd,opt;
  struct sockaddr_in addr;
  char buf[200];
  ssize_t n;
  uint8_t a[4],cidr,nfound;
  int i,j,len;
  uint16_t b[4];
  uint32_t ip4,q;
  uint64_t ip6;
  struct v4 *aiv4;
  struct v6 *aiv6;

  server_fd=socket(AF_INET,SOCK_STREAM,0);
  opt=1;
  setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=INADDR_ANY;
  addr.sin_port=htons(43);
  bind(server_fd,(struct sockaddr *)&addr,sizeof(addr));
  listen(server_fd,5);
  while(!interrupted){
    client_fd=accept(server_fd,NULL,NULL);
    if(client_fd<0)continue;
    n=read(client_fd,buf,199);
    if(n>0){
      pthread_mutex_lock(&lock);
      buf[n]='\0';
      if(strncmp(buf,"stat",4)==0){
        sprintf(buf,"%s Tstart\n",mydata(tstart)); write(client_fd,buf,strlen(buf));
        sprintf(buf,"%s Trx\n",mydata(trx)); write(client_fd,buf,strlen(buf));
        sprintf(buf,"%s Tnew\n",mydata(tnew)); write(client_fd,buf,strlen(buf));
        sprintf(buf,"%10lu Nrestart\n",restart); write(client_fd,buf,strlen(buf));
        sprintf(buf,"%10lu Nelm v4\n%10lu Ncollision v4 [%6.4f%%]\n%10lu Nrx v4\n%10lu Nnew v4\n",nv4-1,coll4,100.0*(coll4+0.0)/(rxv4+0.01),rxv4,newv4); write(client_fd,buf,strlen(buf));
        sprintf(buf,"%10lu Nelm v6\n%10lu Ncollision v6 [%6.4f%%]\n%10lu Nrx v6\n%10lu Nnew v6\n",nv6-1,coll6,100.0*(coll6+0.0)/(rxv6+0.01),rxv6,newv6); write(client_fd,buf,strlen(buf));
      }
      else {
        query++;
        len=n;
        nfound=0;
        for(i=0;i<len;i++)if(buf[i]=='.')break;
        if(i<len){
          for(j=0;j<4;j++)a[j]=0;
          for(i=-1,j=0;j<4;j++)for(a[j]=0,i++;i<len;i++)if((buf[i]!='.'&&j<3) || (buf[i]!='\0'&&j==3))a[j]=a[j]*10+dd[buf[i]]; else break;
          for(ip4=0,j=0;j<4;j++){ip4<<=8; ip4|=a[j];}
          for(cidr=24;cidr>=8;cidr--){
            q=hv4(ip4,cidr);
            if(v4i[q]!=0){
              aiv4=v4+v4i[q];
              sprintf(buf,"%u %lu %s\n",cidr,aiv4->asn,mydata(aiv4->ts)); write(client_fd,buf,strlen(buf));
              nfound++;
            }
          }
        }
        else {
          for(j=0;j<4;j++)b[j]=0;
          for(i=-1,j=0;j<4;j++){
            for(i++;i<len;i++)if(buf[i]!=':' && buf[i]!='\0')b[j]=b[j]*16+dd[buf[i]]; else break;
            if(buf[i]=='\0' || buf[i+1]==':')break;
          }
          for(ip6=0,j=0;j<4;j++){ip6<<=16; ip6|=b[j];}
          for(cidr=48;cidr>=16;cidr--){
            q=hv6(ip6,cidr);
            if(v6i[q]!=0){
              aiv6=v6+v6i[q];
              sprintf(buf,"%u %lu %s\n",cidr,aiv6->asn,mydata(aiv6->ts)); write(client_fd,buf,strlen(buf));
              nfound++;
            }
          }
        }
        sprintf(buf,"--\n%u match found\n%lu v4 elm\n%lu v6 elm\n%lu query\n",nfound,nv4-1,nv6-1,query); write(client_fd,buf,strlen(buf));
      }
      pthread_mutex_unlock(&lock);
    }    
    close(client_fd);
  }
  close(server_fd);
  return NULL;
}
