#define RELEASE "cqrun @IK4LZH @GM 2020-2026 Rel 3.10"
#include "cqfunc.c"
#define FILE_LOG "/home/gmazzini/.local/share/WSJT-X/wsjtx_log.adi"
#define FILE_ESC "/home/gmazzini/gm2/cqlinux/wsjtx_black.txt"

#define CQRATE 2
#define PORT 7777
#define MAX_RXED 1000
#define CQ_MAX_AGE 600
int jcq=0;
time_t txenablelock=0;
time_t logginglock=0;
time_t laststatus=0;
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
time_t tstart,tlastlogged=0;
pthread_mutex_t rxlock=PTHREAD_MUTEX_INITIALIZER;
void cqselection(char *,int *,char *);
void* th_enabletx();
void* th_logging();
void sigint_handler();
int locked(time_t,int);
void unlock_old(void);
int wsjtx_alive(void);
int status_alive(void);
int cached_winlog(void);
void refresh_windows(void);
int send_reply(struct rxed *);
void append_diag(char *,char *);

#include "cqwhois.c"

int locked(time_t t,int sec){
  if(t==0)return 0;
  if(time(NULL)-t>sec)return 0;
  return 1;
}

void unlock_old(void){
  if(txenablelock && time(NULL)-txenablelock>40){
    printf("%s Unlock old txenable\n",mytime());
    txenablelock=0;
  }
  if(logginglock && time(NULL)-logginglock>20){
    printf("%s Unlock old logging\n",mytime());
    logginglock=0;
  }
}

int wsjtx_alive(void){
  if(heartbeat==0)return 0;
  if(time(NULL)-heartbeat>30)return 0;
  return 1;
}

int status_alive(void){
  if(laststatus==0)return 0;
  if(time(NULL)-laststatus>10)return 0;
  return 1;
}

int cached_winlog(void){
  static time_t last=0;
  static int value=0;
  time_t now;

  now=time(NULL);
  if(now==last)return value;
  last=now;
  value=winlog();
  return value;
}

void refresh_windows(void){
  static time_t last=0;

  if(time(NULL)-last<10)return;
  last=time(NULL);
  if(wbase==0 || wlog==0 || !wsjtx_alive())winid();
}

void append_diag(char *dst,char *src){
  if(dst==NULL)return;
  if(strlen(dst)<BUF_SIZE-160)snprintf(dst+strlen(dst),BUF_SIZE-strlen(dst),"%s",src);
}

int send_reply(struct rxed *r){
  char out[BUF_SIZE],*q;
  struct sockaddr_in to_addr;

  if(r==NULL)return 0;
  to_addr=sender_addr;
  q=out;
  Wu32(0xadbccbda,&q);
  Wu32(2,&q);
  Wu32(4,&q);
  Ws("GM1",&q);
  Wu32(r->ttime,&q);
  Wu32((uint32_t)r->snr,&q);
  Wf(r->dt,&q);
  Wu32(r->df,&q);
  Ws(r->mode,&q);
  Ws(r->msg,&q);
  Wb(r->LowConf,&q);
  Wu8(0x00,&q);
  sendto(sock,out,q-out,0,(struct sockaddr*)&to_addr,sizeof(to_addr));
  return 1;
}

int main() {
  int i,j,n;
  char buffer[BUF_SIZE],out[BUF_SIZE],aux[16],call[16],mode[8];
  char *p;
  uint8_t bb,bdec,transmitting;
  uint32_t type,xx,TPeriod;
  time_t rawtime;
  struct tm tm;
  pthread_t thread,thread2,thread3;
  FILE *fp;
  struct timeval tv;

  time(&tstart);
  fp=fopen(FILE_LOG,"r");
  if(fp!=NULL){
    for(;;){
      if(fgets(buffer,BUF_SIZE,fp)==NULL)break;
      extractx(call,sizeof(call),buffer,"call"); if(*call=='\0')continue;
      extractx(mode,sizeof(mode),buffer,"submode"); if(*mode=='\0')extractx(mode,sizeof(mode),buffer,"mode"); if(*mode=='\0')continue;
      extractx(aux,sizeof(aux),buffer,"freq"); if(*aux=='\0')continue;
      upper(call); upper(mode);
      snprintf(out,sizeof(out),"%s_%s_%d",call,mode,atoi(aux));
      inslog(out);
    }
    fclose(fp);
  }
  else printf("%s No log file\n",mytime());
  fp=fopen(FILE_ESC,"r");
  if(fp!=NULL){
    for(;;){
      if(fgets(buffer,BUF_SIZE,fp)==NULL)break;
      trim(buffer);
      j=strlen(buffer);
      if(j<3)continue;
      upper(buffer);
      insesc(buffer);
    }
    fclose(fp);
  }
  else printf("%s No black file\n",mytime());
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
  if(sock<0){
    printf("socket error\n");
    return 1;
  }
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=inet_addr("127.0.0.1");
  addr.sin_port=htons(PORT);
  if(bind(sock,(struct sockaddr*)&addr,sizeof(addr))<0){
    printf("bind error\n");
    return 1;
  }
  tv.tv_sec=1;
  tv.tv_usec=0;
  setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
  if(pthread_create(&thread3,NULL,whois_server_thread,NULL)==0)pthread_detach(thread3);

  nrxed=0;
  lasteo=2;
  for(;;){
    unlock_old();
    refresh_windows();
    if(cached_winlog() && (!locked(logginglock,20))){
      logginglock=time(NULL);
      if(pthread_create(&thread2,NULL,th_logging,NULL)!=0)logginglock=0;
      else pthread_detach(thread2);
      continue;
    }
    addr_len=sizeof(sender_addr);
    n=recvfrom(sock,buffer,BUF_SIZE,0,(struct sockaddr *)&sender_addr,&addr_len);
    if(n<0)continue;
    if(n<12)continue;
    
    p=buffer;
    Ru32(&xx,&p); if(xx!=0xadbccbda)continue;
    Ru32(&xx,&p);
    Ru32(&type,&p);

    // Decode
    if(type==2){
      pthread_mutex_lock(&rxlock);
      Rsx(out,sizeof(out),&p);
      Rb(&bb,&p);
      Ru32(&rxed[nrxed].ttime,&p);
      time(&rawtime); tm=*gmtime(&rawtime); tm.tm_hour=0; tm.tm_min=0; tm.tm_sec=0;
      rxed[nrxed].time=timegm(&tm)+rxed[nrxed].ttime/1000;
      Ru32((uint32_t *)&rxed[nrxed].snr,&p);
      Rf(&rxed[nrxed].dt,&p);
      Ru32(&rxed[nrxed].df,&p);
      Rsx(rxed[nrxed].mode,sizeof(rxed[nrxed].mode),&p);
      Rsx(rxed[nrxed].msg,sizeof(rxed[nrxed].msg),&p);
      Rb(&rxed[nrxed].LowConf,&p);
      rxed[nrxed].freqS=lastfreq;
      snprintf(rxed[nrxed].modeS,sizeof(rxed[nrxed].modeS),"%s",lastmode);
      rxed[nrxed].eoS=lasteo;
      if(++nrxed==MAX_RXED)nrxed=0;
      pthread_mutex_unlock(&rxlock);
    }

    // Heartbeat
    else if(type==0){
      Rsx(out,sizeof(out),&p);
      Ru32(&xx,&p);
      Rsx(version,sizeof(version),&p);
      Rsx(out,sizeof(out),&p);
      time(&heartbeat);
    }

    // Logged ADIF 
    else if(type==12){
      Rsx(out,sizeof(out),&p);
      Rsx(out,sizeof(out),&p);
      extractx(call,sizeof(call),out,"call"); if(*call=='\0')goto go12;
      extractx(mode,sizeof(mode),out,"submode"); if(*mode=='\0')extractx(mode,sizeof(mode),out,"mode"); if(*mode=='\0')goto go12;
      extractx(aux,sizeof(aux),out,"freq"); if(*aux=='\0')goto go12;
      upper(call); upper(mode);
      snprintf(out,sizeof(out),"%s_%s_%d",call,mode,atoi(aux));
      inslog(out);
      printf("%s Inslog:%s\n",mytime(),out);
      time(&tlastlogged);
      go12:
    }

    // Status
    else if(type==1){
      Rsx(out,sizeof(out),&p);
      Ru64(&lastfreq,&p);
      Rsx(lastmode,sizeof(lastmode),&p);
      upper(lastmode);
      Rsx(out,sizeof(out),&p);
      Rsx(out,sizeof(out),&p);
      Rsx(out,sizeof(out),&p);
      Rb(&enabletx,&p);
      Rb(&transmitting,&p);
      Rb(&bdec,&p);
      Ru32(&rxdf,&p);
      Ru32(&txdf,&p);
      Rsx(out,sizeof(out),&p);
      Rsx(mygrid,sizeof(mygrid),&p);
      upper(mygrid);
      Rsx(out,sizeof(out),&p);
      Rb(&bb,&p);
      Rsx(out,sizeof(out),&p);
      Rb(&bb,&p);
      Ru8(&bb,&p);
      Ru32(&xx,&p);
      Ru32(&xx,&p);
      Rsx(out,sizeof(out),&p);
      Rsx(out,sizeof(out),&p);
      time(&laststatus);
      if(transmitting){
        TPeriod=0;
        if(strcmp(lastmode,"FT4")==0)TPeriod=7500;
        if(strcmp(lastmode,"FT8")==0)TPeriod=15000;
        if(TPeriod){
          xx=(uint32_t)(ms_since_midnight_utc()/TPeriod);
          lasteo=xx&1;
        }
      }
      if((!enabletx) && wsjtx_alive() && status_alive() && (!cached_winlog()) && (!locked(txenablelock,40)) && (!locked(logginglock,20))){
        txenablelock=time(NULL);
        if(pthread_create(&thread,NULL,th_enabletx,NULL)!=0)txenablelock=0;
        else pthread_detach(thread);
      }
    }
  }
}

void cqselection(char *selcall,int *jsel,char *ttt){
  int cqed,inlog,inblack,inmodifier,badmode,badfreq,badeo,badgrid,badcall,old,lowconf,i,m,j,nk,k[4];
  int vchecklog,vcheckesc,vmodifier,vbadmode,vbadfreq,vbadeo,vbadgrid,vbadcall,vold,vlowconf;
  double topscore,score,ptime,psnr,pdist;
  time_t rawtime;
  char call[16],grid[8],out[BUF_SIZE],modifier[16],line[192];
  uint16_t times;
  
  *jsel=-1; topscore=0; cqed=0; inlog=0; inblack=0; inmodifier=0; badmode=0; badfreq=0; badeo=0; badgrid=0; badcall=0; old=0; lowconf=0;
  time(&rawtime);
  pthread_mutex_lock(&rxlock);
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
    snprintf(call,sizeof(call),"%.*s",k[1]-k[0]-1,rxed[i].msg+k[0]+1);
    upper(call);
    if(onlychar(call)){
      snprintf(modifier,sizeof(modifier),"%s",call);
      snprintf(call,sizeof(call),"%.*s",k[2]-k[1]-1,rxed[i].msg+k[1]+1);
      snprintf(grid,sizeof(grid),"%.*s",k[3]-k[2]-1,rxed[i].msg+k[2]+1);
    }
    else snprintf(grid,sizeof(grid),"%.*s",k[2]-k[1]-1,rxed[i].msg+k[1]+1);
    upper(call); upper(grid);
    snprintf(out,sizeof(out),"%s_%s_%d",call,rxed[i].modeS,(int)(rxed[i].freqS/1000000));
    ptime=rawtime-rxed[i].time;
    vold=(ptime>CQ_MAX_AGE)?1:0;
    if(ptime<1)ptime=1;
    psnr=30.0+rxed[i].snr;
    if(psnr<1)psnr=1;
    vbadgrid=validlocator(grid)?0:1;
    vbadcall=validcall(call)?0:1;
    pdist=distlocator(grid,mygrid)+1;
    if(pdist<1)pdist=1;
    times=timesused(call);
    score=psnr*pdist/ptime/(1+times);
    vlowconf=rxed[i].LowConf?1:0;
    if(vlowconf)score=score/2;
    vchecklog=checklog(out);
    vcheckesc=checkesc(call);
    vmodifier=0;
    if(*modifier!='\0'){
      vmodifier=1;
      if(strcmp(modifier,"EU")==0)vmodifier=0;
      else if(strcmp(modifier,"DX")==0 && pdist>1500)vmodifier=0;
      else if(strcmp(modifier,"WW")==0)vmodifier=0;
    }
    vbadmode=(strcmp(rxed[i].modeS,lastmode)==0)?0:1;
    vbadfreq=((int)(rxed[i].freqS/1000000)==(int)(lastfreq/1000000))?0:1;
    vbadeo=(rxed[i].eoS==lasteo)?0:1;
    if(ttt!=NULL){
      snprintf(line,sizeof(line),"%d,%s,%.0lf,%.0lf,%.0lf,%d,%.0lf,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",i,call,ptime,psnr,pdist,times,score,vchecklog,vcheckesc,vmodifier,vbadmode,vbadfreq,vbadeo,vbadgrid,vbadcall,vold,vlowconf);
      append_diag(ttt,line);
    }
    if(vchecklog)inlog++;
    if(vcheckesc)inblack++;
    if(vmodifier)inmodifier++;
    if(vbadmode)badmode++;
    if(vbadfreq)badfreq++;
    if(vbadeo)badeo++;
    if(vbadgrid)badgrid++;
    if(vbadcall)badcall++;
    if(vold)old++;
    if(vlowconf)lowconf++;
    if(vchecklog || vcheckesc || vmodifier || vbadmode || vbadfreq || vbadeo || vbadgrid || vbadcall || vold)continue;
    if(score>topscore){
      topscore=score;
      *jsel=i;
      snprintf(selcall,16,"%s",call);
    }
  }
  pthread_mutex_unlock(&rxlock);
  if(ttt!=NULL){
    snprintf(line,sizeof(line),"# Selection cqed:%d inlog:%d inblack:%d inmodifier:%d badmode:%d badfreq:%d badeo:%d badgrid:%d badcall:%d old:%d lowconf:%d\n",cqed,inlog,inblack,inmodifier,badmode,badfreq,badeo,badgrid,badcall,old,lowconf);
    append_diag(ttt,line);
  }
}

void* th_enabletx(){
  int jsel;
  char selcall[16];
  static time_t last=0;
  struct rxed r;

  if(time(NULL)-last<2){
    txenablelock=0;
    return NULL;
  }
  printf("%s EnableTx in %d\n",mytime(),jcq);
  sleep(15);
  if(jcq==CQRATE-1){
    cqselection(selcall,&jsel,NULL);
    if(jsel>=0){
      pthread_mutex_lock(&rxlock);
      r=rxed[jsel];
      pthread_mutex_unlock(&rxlock);
      printf("%s Selected %s\n",mytime(),r.msg);
      addused(selcall);
      send_reply(&r);
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

  if(time(NULL)-last<8){
    logginglock=0;
    return NULL;
  }
  printf("%s Logging in\n",mytime());
  sleep(3);
  emulate(XK_Return,XK_Return,1,wlog);
  sleep(3);
  if(cached_winlog())emulate(XK_Escape,XK_Escape,1,wlog);
  logginglock=0;
  time(&last);
  printf("%s Logging out\n",mytime());
  pthread_exit(NULL);
}
