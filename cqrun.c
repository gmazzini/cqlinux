// @2025- by GM IK4LZH cqrun in C

#include "cqfunc.c"
#define FILE_LOG "/home/gmazzini/.local/share/WSJT-X/wsjtx_log.adi"
#define FILE_ESC "/home/gmazzini/gm/cqlinux/wsjtx_black.txt"
#define FILE_INFO "/home/gmazzini/gm/info.txt"

#define CQRATE 2
#define PORT 7777
#define MAX_RXED 1000
int level=3; // bit 0 (1 run/0 test) bit 1 (1 print/0 noprint)
int jcq=0;
int txenablelock=0;
int logginglock=0;
struct rxed {
  uint32_t ttime;
  time_t time;
  int32_t snr;
  double dt;
  uint32_t df;
  char mode[8];
  char msg[40];
  uint8_t LowConf;
  char modeS[8];
  uint64_t freqS;
  uint8_t eoS;
} *rxed;
uint32_t nrxed;
int sock;
struct sockaddr_in addr,sender_addr;
socklen_t addr_len=sizeof(addr);
char mygrid[16],lastmode[8];
uint8_t lasteo;
uint64_t lastfreq;

void* th_enabletx();
void* th_logging();
void sigint_handler();
char *mytime();

int main() {
  int i,j;
  char buffer[BUF_SIZE],out[BUF_SIZE],version[16],aux[16],call[16],mode[8],stime[16];
  char *p;
  uint8_t bb,bdec,enabletx,transmitting;
  uint32_t type,xx,TPeriod;
  time_t rawtime;
  struct tm tm,*ptm;
  pthread_t thread,thread2;
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
  if(level&2)printf("wbase:%lu wlog:%lu\n",wbase,wlog);
  sock=socket(AF_INET,SOCK_DGRAM,0);
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=inet_addr("127.0.0.1");
  addr.sin_port=htons(PORT);
  bind(sock,(struct sockaddr*)&addr,sizeof(addr));
  signal(34,sigint_handler);
  nrxed=0;
  lasteo=2;
  for(;;){
    recvfrom(sock,buffer,BUF_SIZE,0,(struct sockaddr *)&sender_addr,&addr_len);
    
    if((level&1) && winlog() && (!logginglock)){
      pthread_create(&thread2,NULL,th_logging,NULL);
      pthread_detach(thread2);
    }
    
    p=buffer;
    Ru32(&xx,&p); if(xx!=0xadbccbda)continue;
    Ru32(&xx,&p);
    Ru32(&type,&p);

    // Decode
    if(type==2){
      Rs(out,&p);
      Rb(&bb,&p);
      Ru32(&rxed[nrxed].ttime,&p);
      time(&rawtime); tm=*gmtime(&rawtime); tm.tm_hour=0; tm.tm_min=0; tm.tm_sec=0;
      rxed[nrxed].time=timegm(&tm)+rxed[nrxed].ttime/1000;
      Ru32((uint32_t *)&rxed[nrxed].snr,&p);
      Rf(&rxed[nrxed].dt,&p);
      Ru32(&rxed[nrxed].df,&p);
      Rs(rxed[nrxed].mode,&p);
      Rs(rxed[nrxed].msg,&p);
      Rb(&rxed[nrxed].LowConf,&p);
      rxed[nrxed].freqS=lastfreq;
      strcpy(rxed[nrxed].modeS,lastmode);
      rxed[nrxed].eoS=lasteo;
      if(++nrxed==MAX_RXED)nrxed=0;
    }

    // Heartbeat
    else if(type==0){
      Rs(out,&p);
      Ru32(&xx,&p);
      Rs(version,&p);
      Rs(out,&p);
      if(level&2)printf("%s Heartbeat: %s\n",mytime(),version);
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
      if(level&2)printf("%s Inslog:%s\n",mytime(),out);
      go12:
    }

    // Status
    else if(type==1){
      Rs(out,&p);
      Ru64(&lastfreq,&p);
      Rs(lastmode,&p);
      Rs(out,&p);
      Rs(out,&p);
      Rs(out,&p);
      Rb(&enabletx,&p);
      Rb(&transmitting,&p);
      Rb(&bdec,&p);
      Ru32(&xx,&p);
      Ru32(&xx,&p);
      Rs(out,&p);
      Rs(mygrid,&p);
      Rs(out,&p);
      Rb(&bb,&p);
      Rs(out,&p);
      Rb(&bb,&p);
      Ru8(&bb,&p);
      Ru32(&xx,&p);
      Ru32(&xx,&p);
      Rs(out,&p);
      Rs(out,&p);      
      if(transmitting){
        TPeriod=0;
        if(strcmp(lastmode,"FT4")==0)TPeriod=7500;
        if(strcmp(lastmode,"FT8")==0)TPeriod=15000;
        xx=(uint32_t)(ms_since_midnight_utc()/TPeriod);
        lasteo=xx&1;
      }
      if((level&1) && (!enabletx) && (!txenablelock) && (!logginglock)){
        pthread_create(&thread,NULL,th_enabletx,NULL);
        pthread_detach(thread);
      }
    }
  }
}

void cqselection(char *selcall,int *jsel,FILE *fp){
  int cqed,inlog,inblack,inmodifier,badmode,badfreq,badeo,i,m,j,nk,k[4];
  int vchecklog,vcheckesc,vmodifier,vbadmode,vbadfreq,vbadeo;
  double topscore,score,ptime,psnr,pdist;;
  time_t rawtime;
  char call[16],grid[8],out[BUF_SIZE],modifier[8];
  uint16_t times;
  
  *jsel=-1; topscore=0; cqed=0; inlog=0; inblack=0; inmodifier=0; badmode=0; badfreq=0; badeo=0;
  time(&rawtime);
  for(i=0;i<MAX_RXED;i++)if(strncmp(rxed[i].msg,"CQ ",3)==0){
    cqed++;
    m=strlen(rxed[i].msg);
    nk=0;
    for(j=0;j<=m;j++){
      if(rxed[i].msg[j]==' ' || rxed[i].msg[j]=='\0')k[nk++]=j;
      if(nk==4)break;
    }
    if(nk<3)continue;
    *modifier='\0';
    sprintf(call,"%.*s",k[1]-k[0]-1,rxed[i].msg+k[0]+1);
    if(onlychar(call)){
      strcpy(modifier,call);
      sprintf(call,"%.*s",k[2]-k[1]-1,rxed[i].msg+k[1]+1);
      sprintf(grid,"%.*s",k[3]-k[2]-1,rxed[i].msg+k[2]+1);
    }
    else sprintf(grid,"%.*s",k[2]-k[1]-1,rxed[i].msg+k[1]+1);
    sprintf(out,"%s_%s_%d",call,rxed[i].modeS,(int)(rxed[i].freqS/1000000));
    ptime=rawtime-rxed[i].time;
    psnr=30.0+rxed[i].snr;
    pdist=distlocator(grid,mygrid)+1;
    times=timesused(call);
    score=psnr*pdist/ptime/(1+times);
    vchecklog=checklog(out);
    vcheckesc=checkesc(call);
    vmodifier=0;
    if(*modifier!='\0'){
      vmodifier=1;
      if(strcmp(modifier,"EU")==0)vmodifier=0;
      if(strcmp(modifier,"DX")==0 && pdist>1500)vmodifier=0;
    }
    vbadmode=(strcmp(rxed[i].modeS,lastmode)==0)?0:1;
    vbadfreq=((int)(rxed[i].freqS/1000000)==(int)(lastfreq/1000000))?0:1;
    vbadeo=(rxed[i].eoS==lasteo)?0:1;
    if(fp!=NULL)fprintf(fp,"%d,%s,%.0lf,%.0lf,%.0lf,%d,%.0lf,%d,%d,%d,%d,%d,%d\n",i,call,ptime,psnr,pdist,times,score,vchecklog,vcheckesc,vmodifier,vbadmode,vbadfreq,vbadeo);
    if(vchecklog)inlog++;
    if(vcheckesc)inblack++;
    if(vmodifier)inmodifier++;
    if(vbadmode)badmode++;
    if(vbadfreq)badfreq++;
    if(vbadeo)badeo++;
    if(vchecklog || vcheckesc || vmodifier || vbadmode || vbadfreq || vbadeo)continue;
    if(score>topscore){
      topscore=score;
      *jsel=i;
      strcpy(selcall,call);
    }
  }
  if(fp!=NULL)fprintf(fp,"# Selection cqed:%d inlog:%d inblack:%d inmodifier:%d badmode:%d badfreq:%d badeo:%d\n",cqed,inlog,inblack,inmodifier,badmode,badfreq,badeo);
}

void* th_enabletx(){
  int jsel;
  char out[BUF_SIZE],selcall[16],*q,stime[16];
  time_t rawtime;
  struct tm *ptm;

  txenablelock=1;
  if(level&2)printf("%s EnableTx in\n",mytime());
  sleep(16);
  if(jcq==CQRATE-1){
    cqselection(selcall,&jsel,NULL);
    if(level&2 && jsel>=0)printf("%s Selected %s\n",mytime(),rxed[jsel].msg);
    if(jsel>=0){
      addused(selcall);
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
  emulate(XK_Alt_L,XK_n,2,wbase);
  if(jcq!=CQRATE-1)emulate(XK_Alt_L,XK_6,2,wbase);
  if(++jcq==CQRATE)jcq=0;
  txenablelock=0;
  if(level&2)printf("%s EnableTx out\n",mytime());
  pthread_exit(NULL);
}

void* th_logging(){
  char stime[16];
  time_t rawtime;
  struct tm *ptm;
  logginglock=1;
  if(level&2)printf("%s Logging in\n",mytime());
  sleep(4);
  emulate(XK_Return,XK_Return,1,wlog);
  logginglock=0;
  if(level&2)printf("%s Logging out\n",mytime());
  pthread_exit(NULL);
}

void sigint_handler(){
  FILE *fp;
  uint32_t i;
  int jsel;
  char selcall[16];
  fp=fopen(FILE_INFO,"w");
  if(fp==NULL)return;
  fprintf(fp,">> CQED\n");
  cqselection(selcall,&jsel,fp);
  if(jsel>=0)fprintf(fp,"# Selected jsel:%d call:%s\n",jsel,selcall);
  fprintf(fp,"<< CQED\n");
  fprintf(fp,">> RXED\n");
  for(i=0;i<MAX_RXED;i++){
    if(rxed[i].msg[0]=='\0')continue;
    fprintf(fp,"%d,",i);
    fprintf(fp,"%" PRIu32 ",%ld,%d,%3.1f,%" PRIu32 ",%s,%s,%d,",rxed[i].ttime,rxed[i].time,rxed[i].snr,rxed[i].dt,rxed[i].df,rxed[i].mode,rxed[i].msg,rxed[i].LowConf);
    fprintf(fp,"%s,%" PRIu64 ",%d\n",rxed[i].modeS,rxed[i].freqS,rxed[i].eoS);
  }
  fprintf(fp,"<< RXED\n");
  fprintf(fp,">> LOG\n");
  for(i=0;i<nlog;i++)fprintf(fp,"%d,%s\n",i,vlog[i]);
  fprintf(fp,"<< LOG\n");
  fprintf(fp,">> ESC\n");
  for(i=0;i<nesc;i++)fprintf(fp,"%d,%s\n",i,vesc[i]);
  fprintf(fp,"<< ESC\n");
  fprintf(fp,">> USED\n");
  for(i=0;i<nused;i++)fprintf(fp,"%d,%s,%d\n",i,used[i].call,used[i].times);
  fprintf(fp,"<< USED\n");
  fclose(fp);
}

char *mytime(void){
  static char stime[16];
  time_t rawtime;
  struct tm *ptm;
  time(&rawtime); 
  ptm=gmtime(&rawtime); 
  strftime(stime,16,"%H%M%S",ptm);
  return stime;
}
