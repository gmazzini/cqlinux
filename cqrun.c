// @2025- by GM IK4LZH cqrun in C

#include "cqfunc.c"
#define FILE_LOG "/home/gmazzini/.local/share/WSJT-X/wsjtx_log.adi"
#define FILE_ESC "/home/gmazzini/gm/cqlinux/wsjtx_black.txt"

#define CQRATE 2
#define PORT 7777
#define MAX_RXED 1000
int level=0; // bit 0 (1 run/0 test)
int jcq=0;
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
} *rxed;
uint32_t nrxed;
int sock;
struct sockaddr_in addr,sender_addr;
socklen_t addr_len=sizeof(addr);
char mygrid[16];

void* th_enabletx(void* arg);
int main() {
  int i,j;
  char buffer[BUF_SIZE],out[BUF_SIZE],version[16],aux[16],call[16],mode[8],lastmode[8];
  char *p;
  uint8_t bb,bdec,enabletx;
  uint32_t type,xx,len;
  uint64_t lastfreq;
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

    // decode
    if(type==2){
      Rs(out,&p);
      Rb(&bb,&p);
      Ru32(&rxed[nrxed].ttime,&p);
      now=time(NULL); tm=*gmtime(&now); tm.tm_hour=0; tm.tm_min=0; tm.tm_sec=0;
      rxed[nrxed].time=timegm(&tm)+rxed[nrxed].ttime/1000;
      Ru32((uint32_t *)&rxed[nrxed].snr,&p);
      Rf(&rxed[nrxed].dt,&p);
      Ru32(&rxed[nrxed].df,&p);
      Rs(out,&p); strcpy(rxed[nrxed].mode,lastmode);
      Rs(rxed[nrxed].msg,&p);
      Rb(&rxed[nrxed].LowConf,&p);
      rxed[nrxed].freq=lastfreq;
      if(++nrxed==MAX_RXED)nrxed=0;
    }

    // Heartbeat
    else if(type==0){
      Rs(out,&p);
      Ru32(&xx,&p);
      Rs(version,&p);
      Rs(out,&p);
    }

    // Logged ADIF 
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

    // Status
    else if(type==1){
      printf("%d[%d]>",type,len);
      Rs(out,&p);
      Ru64(&lastfreq,&p); printf(" Freq:%" PRIu64,lastfreq);
      Rs(lastmode,&p); printf(" Mode:%s",out);
      Rs(out,&p); printf(" Dx:%s",out);
      Rs(out,&p); printf(" Rep:%s",out);
      Rs(out,&p);
      Rb(&enabletx,&p); printf(" TxEnable:%1x",enabletx);
      Rb(&bb,&p); printf(" Tx:%1x",bb);
      Rb(&bdec,&p);
      Ru32(&xx,&p); printf(" RxF:%" PRIu32,xx);
      Ru32(&xx,&p); printf(" TxF:%" PRIu32,xx);
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
      if((level&1) && (!enabletx)){
        pthread_create(&thread,NULL,th_enabletx,NULL);
        pthread_detach(thread);
      }
    }
  }
}

void* th_enabletx(void* arg){
  int jsel,cqed,inlog,inblack,i,j,k,sp1,m;
  double topscore,score;
  time_t now;
  char out[BUF_SIZE],call[16],*q;
  
  if(jcq==0){
    jsel=-1; topscore=1e37; cqed=0; inlog=0; inblack=0;
    now=time(NULL);
    for(i=0;i<MAX_RXED;i++)if(strncmp(rxed[i].msg,"CQ ",3)==0){
      cqed++;
      m=strlen(rxed[i].msg);
      for(k=0,j=m-1;j>0;j--){
        if(rxed[i].msg[j]==' ')k++;
        if(k==1)sp1=j;
        if(k==2)break;
      }
      if(j==0)continue;
      sprintf(call,"%.*s",m-j-6,rxed[i].msg+j+1);
      sprintf(out,"%s_%s_%d",call,rxed[i].mode,(int)(rxed[i].freq/1000000));
printf("@ %s\n",out);
      if(checklog(out)){inlog++; continue;}
      if(checkesc(call)){inblack++; continue;}
      sprintf(out,"%.*s",4,rxed[i].msg+sp1+1);
      score=now-rxed[i].time+1000.0/(30.0+rxed[i].snr)+100000/(distlocator(out,mygrid)+0.1);;
printf("# %s %lf\n",call,score);
      if(score<topscore){topscore=score; jsel=i;}
    }
    printf("## nrxed:%d cqed:%d inlog:%d inblack:%d\n",nrxed,cqed,inlog,inblack);
    printf("## jsel:%d topscored:%lf [%s]%d %lu\n",jsel,topscore,rxed[jsel].msg,rxed[jsel].snr,now-rxed[jsel].time);
    if(jsel>=50){  // fake
      q=out;
      Wu32(0xadbccbda,&q);
      Wu32(2,&q);
      Wu32(4,&q);
      Ws("GM1",&q);
      Wu32(rxed[jsel].ttime,&q);
      Wu32(rxed[jsel].snr,&q);
      Wf(rxed[jsel].dt,&q);
      Wu32(rxed[jsel].df,&q);
      Ws(rxed[jsel].mode,&q);
      Ws(rxed[jsel].msg,&q);
      Wb(rxed[jsel].LowConf,&q);
      Wu8(0x00,&q);
      sendto(sock,out,q-out,0,(struct sockaddr*)&sender_addr,sizeof(addr));
    }    
  }
  sleep(6);
  emulate(XK_Alt_L,XK_n,2,wbase);
  if(jcq>0)emulate(XK_Alt_L,XK_6,2,wbase);
  if(++jcq>=CQRATE)jcq=0;
  pthread_exit(NULL);
}
