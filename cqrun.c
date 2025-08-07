// @2025- by GM IK4LZH cqrun in C

#include "cqfunc.c"
#define FILE_LOG "/home/gmazzini/.local/share/WSJT-X/wsjtx_log.adi"
#define FILE_ESC "/home/gmazzini/gm/cqlinux/wsjtx_black.txt"

#define CQRATE 2
#define PORT 7777
#define BUF_SIZE 1024
#define MAX_RXED 1000
int level=0; // bit 0 (1 run/0 test)

struct rxed {
  uint32_t ttime;
  time_t time;
  int32_t snr;
  double dt;
  uint32_t df;
  char mode[8];
  char msg[40];
  uint8_t LowConf;
  uint64_t freq;
};

void* th_enabletx(void* arg){
  sleep(6);
  emulate(XK_Alt_L,XK_n,2,wbase);
  emulate(XK_Alt_L,XK_6,2,wbase);
  pthread_exit(NULL);
}

int main() {
  int sock,i,j,k,m,jscore,cqed,inlog,inblack;
  struct sockaddr_in addr,sender_addr;
  socklen_t addr_len=sizeof(addr);
  char buffer[BUF_SIZE],out[BUF_SIZE],version[16],mygrid[16],aux[16],call[16],mode[8];
  char *p,*q;
  uint8_t bb,decoding,bdec,enabletx,jcq,cqrate;
  uint32_t type,xx,nrxed,len;
  uint64_t lastfreq;
  double aaa,topscore;
  struct rxed *rxed;
  time_t now;
  struct tm tm;
  pthread_t thread;
  FILE *fp;

  fp=fopen(FILE_LOG,"r");
  if(fp==NULL)return 0;
  for(;;){
    if(fgets(buffer,BUF_SIZE,fp)==NULL)break;
    extract(call,buffer,"call"); if(*call=='\0')continue;
    extract(mode,buffer,"submode"); if(*mode=='\0')extract(mode,buffer,"mode"); if(*mode=='\0')continue;
    extract(aux,buffer,"freq"); if(*aux=='\0')continue;
    sprintf(out,"%s_%s_%d",call,mode,atoi(aux));
    inslog(out);
  }
  fclose(fp);
  fp=fopen(FILE_ESC,"r");
  if(fp==NULL)return 0;
  for(;;){
    if(fgets(buffer,BUF_SIZE,fp)==NULL)break;
    j=strlen(buffer);
    if(j<3)continue;
    if(buffer[j-1]=='\n')buffer[j-1]='\0';
    insesc(buffer);
  }
  fclose(fp);
  rxed=(struct rxed *)malloc(MAX_RXED*sizeof(struct rxed));
  if(rxed==NULL)return 0;
  for(i=0;i<MAX_RXED;i++)rxed[i].msg[0]='\0';
  winid();
  printf("wbase:%lu wlog:%lu\n",wbase,wlog);
  sock=socket(AF_INET,SOCK_DGRAM,0);
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=inet_addr("127.0.0.1");
  addr.sin_port=htons(PORT);
  bind(sock,(struct sockaddr*)&addr,sizeof(addr));
  nrxed=0;
  decoding=0;
  cqrate=CQRATE;
  jcq=0;

  for(;;){
    len=recvfrom(sock,buffer,BUF_SIZE,0,(struct sockaddr *)&sender_addr,&addr_len);

    if((level&1) && winlog()){
      printf("Log\n");
      emulate(XK_Return,XK_Return,1,wlog);
    }

    p=buffer;
    Ru32(&xx,&p); if(xx!=0xadbccbda)continue;
    Ru32(&xx,&p);
    Ru32(&type,&p);
    if(type==2){
      Rs(out,&p);
      Rb(&bb,&p);
      Ru32(&rxed[nrxed].ttime,&p);
      now=time(NULL); tm=*gmtime(&now); tm.tm_hour=0; tm.tm_min=0; tm.tm_sec=0;
      rxed[nrxed].time=timegm(&tm)+rxed[nrxed].ttime/1000;
      Ru32((uint32_t *)&rxed[nrxed].snr,&p);
      Rf(&rxed[nrxed].dt,&p);
      Ru32(&rxed[nrxed].df,&p);
      Rs(rxed[nrxed].mode,&p);
      Rs(rxed[nrxed].msg,&p);
      Rb(&rxed[nrxed].LowConf,&p);
      rxed[nrxed].freq=lastfreq;
      if(++nrxed==MAX_RXED)nrxed=0;
    }
    else if(type==0){
      Rs(out,&p);
      Ru32(&xx,&p);
      Rs(version,&p);
      Rs(out,&p);
    }
    else if(type==12){
      Rs(out,&p);
      Rs(out,&p);
      extract(call,out,"call"); if(*call=='\0')goto go12;
      extract(mode,buffer,"submode"); if(*mode=='\0')extract(mode,buffer,"mode"); if(*mode=='\0')goto go12;
      extract(aux,out,"freq"); if(*aux=='\0')goto go12;
      sprintf(out,"%s_%s_%d",call,mode,atoi(aux));
      inslog(out);
      go12:
    }
    else if(type==1){
      printf("%d[%d]>",type,len);
      Rs(out,&p);
      Ru64(&lastfreq,&p); printf(" Freq:" PRIu64,lastfreq);
      Rs(out,&p); printf(" Mode:%s",out);
      Rs(out,&p); printf(" Dx:%s",out);
      Rs(out,&p); printf(" Rep:%s",out);
      Rs(out,&p);
      Rb(&enabletx,&p); printf(" TxEnable:%1x",enabletx);
      Rb(&bb,&p); printf(" Tx:%1x",bb);
      Rb(&bdec,&p);
      Ru32(&xx,&p); printf(" RxF:%lu",xx);
      Ru32(&xx,&p); printf(" TxF:%lu",xx);
      Rs(out,&p);
      Rs(mygrid,&p);
      Rs(out,&p); printf(" DxGrid:%s",out);
      Rb(&bb,&p);
      Rs(out,&p);
      Rb(&bb,&p);
      Ru8(&bb,&p);
      Rs(out,&p);
      Rs(out,&p);
      Rs(out,&p);
      Rs(out,&p); trim(out);  printf(" Msg:%s",out);
      printf("\n");
      if(bdec==0 && decoding==1){
        jscore=-1; topscore=1e37;
        cqed=0; inlog=0; inblack=0;
        now=time(NULL);
        for(i=0;i<MAX_RXED;i++)if(strncmp(rxed[i].msg,"CQ ",3)==0){
          cqed++;
          m=strlen(rxed[i].msg);
          for(k=0,j=m-1;j>0;j--){
            if(rxed[i].msg[j]==' ')k++;
            if(k==2)break;
          }
          if(j==0)continue;
          sprintf(call,"%.*s",m-j-6,rxed[i].msg+j+1);
          sprintf(out,"%s_%s_%d",call,rxed[i].mode,rxed[i].freq/1000000);
          if(checklog(out)){inlog++; continue;}
          if(checkesc(call)){inblack++; continue;}
          out[4]='\0';
          aaa=now-rxed[i].time+1000.0/(30.0+rxed[i].snr)+100000/(distlocator(out,mygrid)+0.1);;
printf("# %s %lf\n",call,aaa);
          if(aaa<topscore){topscore=aaa; jscore=i;}
        }
printf("## nrxed:%d cqed:%d inlog:%d inblack:%d\n",nrxed,cqed,inlog,inblack);

if(jscore>=50){
q=out;
Wu32(0xadbccbda,&q);
Wu32(2,&q);
Wu32(4,&q);
Ws("GM1",&q);
Wu32(rxed[jscore].ttime,&q);
Wu32(rxed[jscore].snr,&q);
Wf(rxed[jscore].dt,&q);
Wu32(rxed[jscore].df,&q);
Ws(rxed[jscore].mode,&q);
Ws(rxed[jscore].msg,&q);
Wb(rxed[jscore].LowConf,&q);
Wu8(0x00,&q);
sendto(sock,out,q-out,0,(struct sockaddr*)&sender_addr,sizeof(addr));

}

        printf(">>> jscore:%d topscored:%lf [%s]%d %lu\n",jscore,topscore,rxed[jscore].msg,rxed[jscore].snr,now-rxed[jscore].time);
      }
      decoding=bdec;
      if((level&1) && (!enabletx)){
        pthread_create(&thread,NULL,th_enabletx,NULL);
        pthread_detach(thread);
      }
    }
  }
}
