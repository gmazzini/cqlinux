#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
#include <inttypes.h>
#include <signal.h>

#define MAX_WINDOWS 1000
#define MAX_LOG 10000
#define MAX_ESC 2000
#define MAX_USED 2000
#define BUF_SIZE 1024
#define LOG_KEY_SIZE 40
#define ESC_KEY_SIZE 24

Window wbase,wlog;
struct used {
  char call[16];
  uint16_t times;
} *used=NULL;
char **vlog=NULL,**vesc=NULL;
uint16_t nlog=0,nesc=0,nused=0;
time_t heartbeat=0;
pthread_mutex_t xlock=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t loglock=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t esclock=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t usedlock=PTHREAD_MUTEX_INITIALIZER;

void Rsx(char *b,int max,char **q){
  uint32_t x,y;
  char *p=*q;

  x=((uint32_t)(uint8_t)p[0]<<24)|((uint32_t)(uint8_t)p[1]<<16)|((uint32_t)(uint8_t)p[2]<<8)|((uint32_t)(uint8_t)p[3]);
  if(x==0xffffffff)x=0;
  y=x;
  if(max<1)max=1;
  if(y>=(uint32_t)max)y=max-1;
  snprintf(b,max,"%.*s",(int)y,p+4);
  *q+=4+x;
}

void Rs(char *b,char **q){
  Rsx(b,BUF_SIZE,q);
}

void Ws(char *b,char **q){
  char *p=*q;
  uint32_t len=(uint32_t)strlen((char *)b);
  p[0]=(uint8_t)(len>>24);
  p[1]=(uint8_t)(len>>16);
  p[2]=(uint8_t)(len>>8);
  p[3]=(uint8_t)len;
  memcpy(p+4,b,len);
  *q+=4+len;
}

void Ru32(uint32_t *b,char **q){
  char *p=*q;
  *b=((uint32_t)(uint8_t)p[0]<<24)|((uint32_t)(uint8_t)p[1]<<16)|((uint32_t)(uint8_t)p[2]<<8)|((uint32_t)(uint8_t)p[3]);
  *q+=4;
}

void Wu32(uint32_t b, char **q){
  char *p=*q;
  p[0]=(uint8_t)(b>>24);
  p[1]=(uint8_t)(b>>16);
  p[2]=(uint8_t)(b>>8);
  p[3]=(uint8_t)b;
  *q+=4;
}

void Ru64(uint64_t *b,char **q){
  char *p=*q;
  *b=((uint64_t)(uint8_t)p[0]<<56)|((uint64_t)(uint8_t)p[1]<<48)|((uint64_t)(uint8_t)p[2]<<40)|((uint64_t)(uint8_t)p[3]<<32)|((uint64_t)(uint8_t)p[4]<<24)|((uint64_t)(uint8_t)p[5]<<16)|((uint64_t)(uint8_t)p[6]<<8)|((uint64_t)(uint8_t)p[7]);
  *q+=8;
}

void Wu64(uint64_t b,char **q){
  char *p=*q;
  p[0]=(uint8_t)(b>>56);
  p[1]=(uint8_t)(b>>48);
  p[2]=(uint8_t)(b>>40);
  p[3]=(uint8_t)(b>>32);
  p[4]=(uint8_t)(b>>24);
  p[5]=(uint8_t)(b>>16);
  p[6]=(uint8_t)(b>>8);
  p[7]=(uint8_t)b;
  *q+=8;
}

void Rb(uint8_t *b,char **q){
  char *p=*q;
  *b=p[0]&1;
  *q+=1;
}

void Wb(uint8_t b,char **q){
  char *p=*q;
  p[0]=b?1:0;
  *q+=1;
}

void Ru8(uint8_t *b,char **q){
  char *p=*q;
  *b=(uint8_t)p[0];
  *q+=1;
}

void Wu8(uint8_t b,char **q){
  char *p=*q;
  p[0]=b;
  *q+=1;
}

void Rf(double *b,char **q){
  char *p=*q;
  uint64_t bb;

  bb=((uint64_t)(uint8_t)p[0]<<56)|((uint64_t)(uint8_t)p[1]<<48)|((uint64_t)(uint8_t)p[2]<<40)|((uint64_t)(uint8_t)p[3]<<32)|((uint64_t)(uint8_t)p[4]<<24)|((uint64_t)(uint8_t)p[5]<<16)|((uint64_t)(uint8_t)p[6]<<8)|((uint64_t)(uint8_t)p[7]);
  memcpy(b,&bb,sizeof(double));
  *q+=8;
}

void Wf(double v,char **q){
  char *p=*q;
  uint64_t b;

  memcpy(&b,&v,sizeof(uint64_t));
  p[0]=(uint8_t)(b>>56);
  p[1]=(uint8_t)(b>>48);
  p[2]=(uint8_t)(b>>40);
  p[3]=(uint8_t)(b>>32);
  p[4]=(uint8_t)(b>>24);
  p[5]=(uint8_t)(b>>16);
  p[6]=(uint8_t)(b>>8);
  p[7]=(uint8_t)b;
  *q+=8;
}

void trim(char *p){
  int i;

  i=strlen(p)-1;
  while(i>=0){
    if(p[i]==' ' || p[i]=='\n' || p[i]=='\r' || p[i]=='\t')p[i]='\0';
    else break;
    i--;
  }
}

void upper(char *p){
  for(;*p;p++)if(*p>='a' && *p<='z')*p=*p-'a'+'A';
}

int validlocator(char *p){
  if(strlen(p)<4)return 0;
  upper(p);
  if(p[0]<'A' || p[0]>'R')return 0;
  if(p[1]<'A' || p[1]>'R')return 0;
  if(p[2]<'0' || p[2]>'9')return 0;
  if(p[3]<'0' || p[3]>'9')return 0;
  return 1;
}

int validcall(char *p){
  char *q;
  int len,ndigit,nalpha;

  upper(p);
  len=strlen(p);
  if(len<3 || len>15)return 0;
  ndigit=0;
  nalpha=0;
  for(q=p;*q!='\0';q++){
    if(*q>='0' && *q<='9')ndigit++;
    else if(*q>='A' && *q<='Z')nalpha++;
    else if(*q=='/'){}
    else return 0;
  }
  if(ndigit==0 || nalpha==0)return 0;
  return 1;
}

void winid(){
  Window queue[MAX_WINDOWS],root,current,root_ret,parent_ret,*children;
  int front=0,rear=0;
  unsigned int nchildren,i;
  char *name;
  Display *dpy;
  XWindowAttributes attr;
  uint32_t wdim=0,ww;

  pthread_mutex_lock(&xlock);
  wbase=0;
  wlog=0;
  dpy=XOpenDisplay(":0");
  if(!dpy){
    pthread_mutex_unlock(&xlock);
    return;
  }
  root=DefaultRootWindow(dpy);
  queue[rear++]=root;
  while(front<rear && rear<MAX_WINDOWS){
    current=queue[front++];
    if(!XGetWindowAttributes(dpy,current,&attr))continue;
    name=NULL;
    XFetchName(dpy,current,&name);
    if(name){
      if(strstr(name,"WSJT-X")){
        if(strstr(name,"Log"))wlog=current;
        ww=attr.width*attr.height;
        if(ww>wdim){wdim=ww; wbase=current;}
      }
      XFree(name);
    }
    children=NULL;
    nchildren=0;
    if(XQueryTree(dpy,current,&root_ret,&parent_ret,&children,&nchildren)){
      for(i=0;i<nchildren && rear<MAX_WINDOWS;i++)queue[rear++] = children[i];
      if(children)XFree(children);
    }
  }
  XCloseDisplay(dpy);
  pthread_mutex_unlock(&xlock);
}

int winlog(){
  Display *dpy;
  XWindowAttributes attrs;
  int r;

  if(wlog==0)return 0;
  pthread_mutex_lock(&xlock);
  dpy=XOpenDisplay(":0");
  if(!dpy){
    pthread_mutex_unlock(&xlock);
    return 0;
  }
  r=XGetWindowAttributes(dpy,wlog,&attrs);
  XCloseDisplay(dpy);
  pthread_mutex_unlock(&xlock);
  if(!r)return 0;
  return (attrs.map_state == IsViewable);
}

void emulate(KeySym k1,KeySym k2,int keys,Window win){
  Display *dpy;
  KeyCode kk1,kk2;

  if(win==0)return;
  pthread_mutex_lock(&xlock);
  dpy=XOpenDisplay(":0");
  if(!dpy){
    pthread_mutex_unlock(&xlock);
    return;
  }
  XRaiseWindow(dpy,win);
  XSetInputFocus(dpy,win,RevertToParent,CurrentTime);
  XFlush(dpy);
  usleep(200000);
  kk1=XKeysymToKeycode(dpy,k1);
  if(keys==2)kk2=XKeysymToKeycode(dpy,k2);
  XTestFakeKeyEvent(dpy,kk1,True,CurrentTime);
  if(keys==2){
    XTestFakeKeyEvent(dpy,kk2,True,CurrentTime);
    XTestFakeKeyEvent(dpy,kk2,False,CurrentTime);
  }
  XTestFakeKeyEvent(dpy,kk1,False,CurrentTime);
  XFlush(dpy);
  XCloseDisplay(dpy);
  pthread_mutex_unlock(&xlock);
}

double distlocator(char *loc1,char *loc2){
  double lon1,lon2,lat1,lat2,dlat,dlon,a;
  char l1[8],l2[8];

  snprintf(l1,sizeof(l1),"%s",loc1);
  snprintf(l2,sizeof(l2),"%s",loc2);
  if(!validlocator(l1) || !validlocator(l2))return -1;
  lon1=(l1[0]-'A')*20.0+(l1[2]-'0')*2.0-180.0+1.0;
  lat1=(l1[1]-'A')*10.0+(l1[3]-'0')*1.0-90.0+0.5;
  lon2=(l2[0]-'A')*20.0+(l2[2]-'0')*2.0-180.0+1.0;
  lat2=(l2[1]-'A')*10.0+(l2[3]-'0')*1.0-90.0+0.5;
  dlat=(lat2-lat1)*M_PI/180.0;
  dlon=(lon2-lon1)*M_PI/180.0;
  lat1*=M_PI/180.0;
  lat2*=M_PI/180.0;
  a=sin(dlat/2)*sin(dlat/2)+cos(lat1)*cos(lat2)*sin(dlon/2)*sin(dlon/2);
  return 6371.0*2*atan2(sqrt(a),sqrt(1-a));
}

void extractx(char *dst,int max,char *src,char *look){
  char *ll,*le,ss[32];
  int lenlook,len;

  if(max<1)return;
  dst[0]='\0';
  lenlook=strlen(look);
  if(lenlook>28)return;
  *ss='<';
  strcpy(ss+1,look);
  *(ss+1+lenlook)=':';
  *(ss+1+lenlook+1)='\0';
  ll=strstr(src,ss);
  if(ll==NULL)return;
  le=strchr(ll,'>');
  if(le==NULL)return;
  len=atoi(ll+1+lenlook+1);
  if(len<0)len=0;
  if(len>=max)len=max-1;
  snprintf(dst,max,"%.*s",len,le+1);
}

void extract(char *dst,char *src,char *look){
  extractx(dst,BUF_SIZE,src,look);
}

void inslog(char *p){
  int pos,start,end,found,a,i;

  pthread_mutex_lock(&loglock);
  if(vlog==NULL){
    vlog=(char **)malloc(MAX_LOG*sizeof(char *));
    if(vlog==NULL)exit(1);
    for(i=0;i<MAX_LOG;i++){
      vlog[i]=(char *)malloc(LOG_KEY_SIZE*sizeof(char));
      if(vlog[i]==NULL)exit(1);
    }
  }
  start=0;
  end=nlog-1;
  found=0;
  pos=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(vlog[pos],p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  if(!found && nlog<MAX_LOG){
    pos=start;
    for(i=nlog;i>pos;i--)snprintf(vlog[i],LOG_KEY_SIZE,"%s",vlog[i-1]);
    nlog++;
    snprintf(vlog[pos],LOG_KEY_SIZE,"%s",p);
  }
  pthread_mutex_unlock(&loglock);
}

int checklog(char *p){
  int pos,start,end,found,a;

  pthread_mutex_lock(&loglock);
  start=0;
  end=nlog-1;
  found=0;
  pos=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(vlog[pos],p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  pthread_mutex_unlock(&loglock);
  return found;
}

void insesc(char *p){
  int pos,start,end,found,a,i;

  pthread_mutex_lock(&esclock);
  if(vesc==NULL){
    vesc=(char **)malloc(MAX_ESC*sizeof(char *));
    if(vesc==NULL)exit(1);
    for(i=0;i<MAX_ESC;i++){
      vesc[i]=(char *)malloc(ESC_KEY_SIZE*sizeof(char));
      if(vesc[i]==NULL)exit(1);
    }
  }
  start=0;
  end=nesc-1;
  found=0;
  pos=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(vesc[pos],p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  if(!found && nesc<MAX_ESC){
    pos=start;
    for(i=nesc;i>pos;i--)snprintf(vesc[i],ESC_KEY_SIZE,"%s",vesc[i-1]);
    nesc++;
    snprintf(vesc[pos],ESC_KEY_SIZE,"%s",p);
  }
  pthread_mutex_unlock(&esclock);
}

int checkesc(char *p){
  int pos,start,end,found,a;

  pthread_mutex_lock(&esclock);
  start=0;
  end=nesc-1;
  found=0;
  pos=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(vesc[pos],p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  pthread_mutex_unlock(&esclock);
  return found;
}

int onlychar(char *p){
  char *q;
  int j,len;

  len=strlen(p);
  if(len<2 || len>4)return 0;
  j=0;
  for(q=p;*q!='\0';q++)if(*q<'A' || *q>'Z')j++;
  return (j==0)?1:0;
}

void addused(char *p){
  int pos,start,end,found,a,i;

  pthread_mutex_lock(&usedlock);
  if(used==NULL){
    used=(struct used *)malloc(MAX_USED*sizeof(struct used));
    if(used==NULL)exit(1);
  }
  start=0;
  end=nused-1;
  found=0;
  pos=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(used[pos].call,p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  if(!found && nused<MAX_USED){
    pos=start;
    for(i=nused;i>pos;i--)memcpy(used+i,used+i-1,sizeof(struct used));
    nused++;
    snprintf(used[pos].call,sizeof(used[pos].call),"%s",p);
    used[pos].times=1;
  }
  if(found && used[pos].times<65535)used[pos].times++;
  pthread_mutex_unlock(&usedlock);
}

uint16_t timesused(char *p){
  int pos,start,end,found,a;
  uint16_t r;

  pthread_mutex_lock(&usedlock);
  start=0;
  end=nused-1;
  found=0;
  pos=0;
  r=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(used[pos].call,p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  if(found==1)r=used[pos].times;
  pthread_mutex_unlock(&usedlock);
  return r;
}

uint64_t ms_since_midnight_utc(void){
  struct timeval tv;
  struct tm utc;
  uint64_t sec_midnight;
  gettimeofday(&tv,NULL);
  gmtime_r(&tv.tv_sec,&utc);
  sec_midnight=utc.tm_hour*3600LL+utc.tm_min*60LL+utc.tm_sec;
  return sec_midnight*1000LL+(tv.tv_usec/1000);
}

char *mytime(void){
  static char stime[16];
  time_t rawtime,dd;
  struct tm *ptm;
  time(&rawtime);
  dd=rawtime-heartbeat; if(dd>99)dd=99;
  ptm=gmtime(&rawtime); 
  strftime(stime,16,"%H%M%S",ptm);
  stime[6]='('; stime[9]=')'; stime[10]='\0';
  stime[7]=(int)(dd/10)+'0'; stime[8]=(int)(dd%10)+'0';
  return stime;
}
