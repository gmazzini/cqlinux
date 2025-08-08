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
#include <math.h>
#include <pthread.h>
#include <inttypes.h>

#define MAX_WINDOWS 1000
#define MAX_LOG 10000
#define MAX_ESC 2000
#define MAX_USED 2000
#define BUF_SIZE 1024

Window wbase,wlog;
struct used {
  char call[16];
  uint16_t times;
} *used=NULL;
char **vlog=NULL,**vesc=NULL;
uint16_t nlog=0,nesc=0,nused=0;

void Rs(char *b,char **q){
  uint32_t x;
  char *p=*q;
  x=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|((uint32_t)p[3]);
  if(x==0xffffffff)x=0;
  sprintf(b,"%.*s",x,p+4);
  *q+=4+x;
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
  *b=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|((uint32_t)p[3]);
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
  *b=((uint64_t)p[0]<<56)|((uint64_t)p[1]<<48)|((uint64_t)p[2]<<40)|((uint64_t)p[3]<<32)|((uint64_t)p[4]<<24)|((uint64_t)p[5]<<16)|((uint64_t)p[6]<<8)|((uint64_t)p[7]);
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
  *b=p[0];
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
  bb=((uint64_t)p[0]<<56)|((uint64_t)p[1]<<48)|((uint64_t)p[2]<<40)|((uint64_t)p[3]<<32)|((uint64_t)p[4]<<24)|((uint64_t)p[5]<<16)|((uint64_t)p[6]<<8)|((uint64_t)p[7]);
  *b=*(double *)(&bb);
  *q+=8;
}

void Wf(double v,char **q){
  char *p=*q;
  uint64_t b;
  b=*(uint64_t *)&v;
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
  int i,len=strlen(p);
  for(i=len-1;i>0;i--)if(p[i]==' ')p[i]='\0'; else break;
}

void winid(){
  Window queue[MAX_WINDOWS],root,current,root_ret,parent_ret,*children;
  int front=0,rear=0;
  unsigned int nchildren,i;
  char *name;
  Display *dpy;
  XWindowAttributes attr;
  uint32_t wdim=0,ww;

  dpy=XOpenDisplay(":0");
  if(!dpy)return;
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
}

int winlog(){
  Display *dpy;
  dpy=XOpenDisplay(":0");
  XWindowAttributes attrs;
  XGetWindowAttributes(dpy,wlog,&attrs);
  XCloseDisplay(dpy);
  return (attrs.map_state == IsViewable);
}

void emulate(KeySym k1,KeySym k2,int keys,Window win){
  Display *dpy;
  KeyCode kk1,kk2;
  dpy=XOpenDisplay(":0");
  if(!dpy)return;
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
}

double distlocator(char *loc1,char *loc2){
  double lon1,lon2,lat1,lat2,dlat,dlon,a;
  if(strlen(loc1)<4 || strlen(loc2)<4)return -1;
  lon1=(loc1[0]-'A')*20.0+(loc1[2]-'0')*2.0-180.0+1.0;
  lat1=(loc1[1]-'A')*10.0+(loc1[3]-'0')*1.0-90.0+0.5;
  lon2=(loc2[0]-'A')*20.0+(loc2[2]-'0')*2.0-180.0+1.0;
  lat2=(loc2[1]-'A')*10.0+(loc2[3]-'0')*1.0-90.0+0.5;
  dlat=(lat2-lat1)*M_PI/180.0;
  dlon=(lon2-lon1)*M_PI/180.0;
  lat1*=M_PI/180.0;
  lat2*=M_PI/180.0;
  a=sin(dlat/2)*sin(dlat/2)+cos(lat1)*cos(lat2)*sin(dlon/2)*sin(dlon/2);
  return 6371.0*2*atan2(sqrt(a),sqrt(1-a));
}

void extract(char *dst,char *src,char *look){
  char *ll,*le,ss[32];
  uint8_t lenlook,len;

  lenlook=strlen(look);
  *ss='<';
  strcpy(ss+1,look);
  *(ss+1+lenlook)=':';
  *(ss+1+lenlook+1)='\0';
  ll=strstr(src,ss);
  if(ll==NULL){ *dst='\0'; return; }
  le=strchr(ll,'>');
  if(le==NULL){ *dst='\0'; return; }
  len=atoi(ll+1+lenlook+1);
  sprintf(dst,"%.*s",len,le+1);
}

void inslog(char *p){
  int pos,start,end,found,a,i;
  if(vlog==NULL){
     vlog=(char **)malloc(MAX_LOG*sizeof(char *));
    if(vlog==NULL)exit(1);
    for(i=0;i<MAX_LOG;i++){
      vlog[i]=(char *)malloc(24*sizeof(char));
      if(vlog[i]==NULL)exit(1);
    }
  }
  start=0;
  end=nlog-1;
  found=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(vlog[pos],p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  if(!found){
    pos=start;
    for(i=nlog;i>pos;i--)strcpy(vlog[i],vlog[i-1]);
    nlog++;
    if(nlog>=MAX_LOG)nlog=MAX_LOG;
    strcpy(vlog[pos],p);
  }
}

int checklog(char *p){
  int pos,start,end,found,a;
  start=0;
  end=nlog-1;
  found=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(vlog[pos],p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  return found;
}

void insesc(char *p){
  int pos,start,end,found,a,i;
  if(vesc==NULL){
     vesc=(char **)malloc(MAX_ESC*sizeof(char *));
    if(vesc==NULL)exit(1);
    for(i=0;i<MAX_ESC;i++){
      vesc[i]=(char *)malloc(16*sizeof(char));
      if(vesc[i]==NULL)exit(1);
    }
  }
  start=0;
  end=nesc-1;
  found=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(vesc[pos],p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  if(!found){
    pos=start;
    for(i=nesc;i>pos;i--)strcpy(vesc[i],vesc[i-1]);
    nesc++;
    if(nesc>=MAX_ESC)nesc=MAX_ESC;
    strcpy(vesc[pos],p);
  }
}

int checkesc(char *p){
  int pos,start,end,found,a;
  start=0;
  end=nesc-1;
  found=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(vesc[pos],p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  return found;
}

int onlychar(char *p){
  char *q;
  int j=0;
  for(q=p;*q!='\0';q++)if(*q<'A' || *q>'Z')j++;
  return (j==0)?1:0;
}

void addused(char *p){
  int pos,start,end,found,a,i;
  if(used==NULL){
     used=(struct used *)malloc(MAX_USED*sizeof(struct used));
    if(used==NULL)exit(1);
  }
  start=0;
  end=nused-1;
  found=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(used[pos].call,p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  if(!found){
    pos=start;
    for(i=nused;i>pos;i--)memcpy(used+i,used+i-1,sizeof(struct used));
    nused++;
    if(nused>=MAX_USED)nused=MAX_USED;
    strcpy(used[pos].call,p);
    used[pos].times=1;
  }
}

uint16_t timesused(char *p){
  int pos,start,end,found,a;
  start=0;
  end=nused-1;
  found=0;
  while(start<=end){
    pos=start+(end-start)/2;
    a=strcmp(used[pos].call,p);
    if(a==0){found=1; break;}
    else if(a<0)start=pos+1;
    else end=pos-1;
  }
  return (found==1)?used[pos].times:0;
}
