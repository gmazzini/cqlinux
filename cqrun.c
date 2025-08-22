#define RELEASE "cqrun @IK4LZH @GM 2020-2025 Rel 3.3"
#include "cqfunc.c"
#define FILE_LOG "/home/gmazzini/.local/share/WSJT-X/wsjtx_log.adi"
#define FILE_ESC "/home/gmazzini/gm/cqlinux/wsjtx_black.txt"

#define CQRATE 2
#define PORT 7777
#define MAX_RXED 1000
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
int sock;
struct sockaddr_in addr,sender_addr;
socklen_t addr_len=sizeof(addr);
char mygrid[16],lastmode[8],version[16];
uint8_t lasteo,enabletx;
uint32_t nrxed,rxdf,txdf;
uint64_t lastfreq;
time_t tstart;
void cqselection(char *,int *,char *);
void* th_enabletx();
void* th_logging();
void sigint_handler();

#include "cqwhois.c"

int main() {
  int i,j;
  char buffer[BUF_SIZE],out[BUF_SIZE],aux[16],call[16],mode[8];
  char *p;
  uint8_t bb,bdec,transmitting;
  uint32_t type,xx,TPeriod;
  time_t rawtime;
  struct tm tm;
  pthread_t thread,thread2,thread3;
  FILE *fp;

  time(&tstart);
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
  if(wlog==0){
    sleep(3);
    emulate(XK_Alt_L,XK_Q,2,wbase);
    sleep(2);
    winid();
    emulate(XK_Escape,XK_Escape,1,wlog);
    sleep(1);
  }
  printf("# %s\n# wbase:%lu wlog:%lu\n",RELEASE,wbase,wlog);
  sock=socket(AF_INET,SOCK_DGRAM,0);
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=inet_addr("127.0.0.1");
  addr.sin_port=htons(PORT);
  bind(sock,(struct sockaddr*)&addr,sizeof(addr));
  pthread_create(&thread3,NULL,whois_server_thread,NULL);
  pthread_detach(thread3);

  nrxed=0;
  lasteo=2;
  for(;;){  
    if(winlog() && (!logginglock)){
      pthread_create(&thread2,NULL,th_logging,NULL);
      pthread_detach(thread2);
      continue;
    }
    recvfrom(sock,buffer,BUF_SIZE,0,(struct sockaddr *)&sender_addr,&addr_len);
    
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
      time(&heartbeat);
    }

    // Logged ADIF 
    else if(type==12){
      Rs(out,&p);
      Rs(out,&p);
      extract(call,out,"call"); if(*call=='\0')goto go12;
      extract(mode,out,"submode"); if(*mode=='\0')extract(mode,out,"mode"); if(*mode=='\0')goto go12;
      extract(aux,out,"freq"); if(*aux=='\0')goto go12;
      sprintf(out,"%s_%s_%d",call,mode,atoi(aux));
      inslog(out);
      printf("%s Inslog:%s\n",mytime(),out);
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
      Ru32(&rxdf,&p);
      Ru32(&txdf,&p);
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
      if((!enabletx) && (!winlog()) && (!txenablelock) && (!logginglock)){
        pthread_create(&thread,NULL,th_enabletx,NULL);
        pthread_detach(thread);
      }
    }
  }
}

void cqselection(char *selcall,int *jsel,char *ttt){
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
    if(ttt!=NULL)sprintf(ttt+strlen(ttt),"%d,%s,%.0lf,%.0lf,%.0lf,%d,%.0lf,%d,%d,%d,%d,%d,%d\n",i,call,ptime,psnr,pdist,times,score,vchecklog,vcheckesc,vmodifier,vbadmode,vbadfreq,vbadeo);
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
  if(ttt!=NULL)sprintf(ttt+strlen(ttt),"# Selection cqed:%d inlog:%d inblack:%d inmodifier:%d badmode:%d badfreq:%d badeo:%d\n",cqed,inlog,inblack,inmodifier,badmode,badfreq,badeo);
}

void* th_enabletx(){
  int jsel;
  char out[BUF_SIZE],selcall[16],*q;
  static time_t last=0;

  if(time(NULL)-last<2)return NULL;
  txenablelock=1;
  printf("%s EnableTx in %d\n",mytime(),jcq);
  sleep(15);
  if(jcq==CQRATE-1){
    cqselection(selcall,&jsel,NULL);
    if(jsel>=0)printf("%s Selected %s\n",mytime(),rxed[jsel].msg);
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
  time(&last);
  printf("%s EnableTx out %d\n",mytime(),jcq);
  pthread_exit(NULL);
}

void* th_logging(){
  static time_t last=0;
  if(time(NULL)-last<2)return NULL;
  logginglock=1;
  printf("%s Logging in\n",mytime());
  sleep(3);
  emulate(XK_Return,XK_Return,1,wlog);
  sleep(3);
  logginglock=0;
  time(&last);
  printf("%s Logging out\n",mytime());
  pthread_exit(NULL);
}

