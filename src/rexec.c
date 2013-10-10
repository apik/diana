/*
    This file is part of DIANA program (DIagram ANAlyser) $Revision: 2.36 $.
    Copyright (C) Mikhail Tentyukov <tentukov@physik.uni-bielefeld.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>

/*gethostbyname:*/
#include <netdb.h>
#include <arpa/inet.h>

#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "queue.h"
/*#include "hash.h"*/
#include "rproto.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"

#ifndef NOSTRINGS
char *strerror(int errnum);
#endif

#define SRVFILENAME "dianasrv"
#define DELTA_MAXTOPJOBS 128
#define JOBS_Q 1
#define SYNCH_JOBS_Q 2
#define FINISHED_JOBS_Q 3
#define FAILED_JOBS_Q 4
#define RUNNING_JOBS_Q 5
#define PIPELINE_Q 6
#define STICKY_PIPELINE_Q 7

/* moved to comdef.h: #define TOP_JOBS_ID 2147483646*/

#define REPORT_JOB_PERFORMED 1
#define REPORT_ALL_JOB_PERFORMED 2

/*Timeout to establish connecton with server, millisecs:*/
#define CONNECT_TIMEOUT 1000

/*Timeout to start new job, millisecs:*/
#define STARTJOB_TIMEOUT 1000

#define MAX_HIT_START_TIMEOUT 10

#define JOBSTABLESIZE 1223

long int g_connect_timeout=CONNECT_TIMEOUT;
long int g_startjob_timeout=STARTJOB_TIMEOUT;
unsigned int g_jobnamestablesize= JOBSTABLESIZE;
unsigned short int g_port=0;

char *g_srv_dir="./";

char *g_remote_srv_dir="./";

/*There are two files, g_jobinfovar and g_jobinfonam.
  (respectively, "1", and "2"). File 2 contains fields:
    No Name Cmdline
  Where Cmdline is a  structure:"XXXXxxx...XXxxx... ...\0

  File 1 contains space-separated fileds:
  X(1)        - HEX number, current placement               (1)
  XXXXXXXX(8) - retcode,                                    (2)
  XXXXXXXX(8) - IP (in HEX notation)                        (4)
  XXXXXXXX(8) - PID                                         (8)
  XXXXXXXX(8) - attributes ("type")                         (16)
  XX(2)       - HEX Timeout hits                            (32)
  XX(2)       - HEX fail hits                               (64)
  XX(2)       - HEX max fail hits,                          (128)
  XX(2)       - HEX errorlevel, or "-1", or "-2".           (256)
  XXXXXXXX(8) - address of a corresponding line in 2 file   (512)
  And, at the end  is EOL '\n'.

  The following structure contains the corresponding string (the field "filelds")
  and related information:
*/
struct jobinfo_struct
{
   int mask;/*bitmask - which fields must be printed, or -1, if all*/
   off_t offset;/*Offset from the beginning of the file 1*/
   int nfields; /*Number of filelds in "filelds"*/
   int *ofields; /*Offsets from the begin of a string to the field*/
   int *lfields;/* Lengths (without separators)*/
   int ltot; /*total length of "filelds", including separators*/
   char *filelds;/*fields;)*/
   /*To write 'i' field to descriptor 'd': write(d,filelds[ofields[i]],lfields[i])*/
   /* Test - must the field 'i' be printed? - (  (1<<i) & mask  )*/
};

static struct jobinfo_struct *l_jobinfo=NULL;

struct job_struct
{
   char *name;/*The name of the job*/
   char *args;/*The structure:"XX{XXxxx...XXxxx... ...}\0"*/
   char retcode[9];/*See the comment to c2pFIN, file rproto.h*/
   struct job_struct *stickJob;
   struct queue_base_struct *myQueueCell;/*Pointer to the queue cell*/
   struct timeval st;/*Starting time*/
   struct timeval ft;/*finishing time*/
   unsigned long int id;/*number of the job*/
   int handlerid;/*id of a handler which performs the job*/
   int stick2hndid;/*id of a handler on which the job must be performed*/
   /*Pid of a process for the job (if started, as reported by the server), or 0:*/
   pid_t pid;
   int hitTimeout,hitFail,
      placement;/*At the top of this file:
                  #define JOBS_Q 1
                  #define SYNCH_JOBS_Q 2
                  #define FINISHED_JOBS_Q 3
                  #define FAILED_JOBS_Q 4
                  #define RUNNING_JOBS_Q 5
                  #define PIPELINE_Q 6
                  #define STICKY_PIPELINE_Q 7
                 */
   int errlevel;/*see "type, ERRORLEVEL_JOB_T" below*/
   int maxnhits;/* see "type, RESTART_JOB_T" below*/
   long int type;/*& SYNC_JOB_T - must be performed after all previously started jobs finish
              & STICKY_JOB_T - must be performed at the same handler as job to stick was
              & FAIL_STICKY_JOB_T - fail if job to stick failed
              & ERRORLEVEL_JOB_T - 0--255 - success if errlevel <=; -1 (default) -
                success if finished, -2 - success if started
              & RESTART_JOB_T - number of job restarts if files, 0 if not set up
              */
};

struct rhandler_struct{
   int rsocket,wsocket;
   int id;
   char ip[16];/*"" for local server*/
   unsigned short int port;/* 0 for local server*/
   char passwd[9];/*"" for local server*/
   struct timeval st;/*Starting time*/
   struct queue_q1_struct thequeue;
   struct queue_base_struct *theJob;
   int n;
   int thenice;
   int status;/*-2 raw, -1 free, 0,1,2 working*/
};

struct localClient_struct{
   /*rawHandlers - non-connected handlers. They allocated by the procedure
   initHandlers() reading the current directory or allocating handlers for
   local servers in the beginning of
   mkRemoteJobs. They are used as a source for freeHandlers - handlers
   connected with the remote/local servers. When a new job is started, it
   is bound to one of these freeHandlers, which is moved to workingHandlers.
   When the job is finished, the corresponding handler is moved from
   workingHandlers back to freeHandlers:*/
   struct queue_base_struct rawHandlers, freeHandlers, workingHandlers;

   /*
      The task coming from the processor is placed into one of synchronous or
      asynchronous queue; then it is placed into one of sticky or non-sticky
      pipeline.

      Pipelines are asynchronous, that means that the program use
      a pipeline without any care of synchronization

      Stycky job must be performed on concrete handler.

      jobsQ is a queue of asynchronous jobs, jobsQn is its length;
      jobsSQ is a queue of synchronous jobs, jobsSQn is its length;
      jobsP is a pipeline of non-sticky jobs, jobsPn is its length;
      jobsSP is a pipeline of sticky jobs, jobsSPn is its length;

      finishedJobsQ is a queue of completed jobs (both successful and failed),
      finishedJobsQn is its length, failedJobsQn in the number of failed jobs in
      finishedJobsQ.
    */
   struct queue_base_struct jobsQ,jobsSQ,finishedJobsQ;
   struct queue_base_struct jobsP,jobsSP;
   unsigned long int jobsQn,jobsSQn,jobsPn,jobsSPn,finishedJobsQn,failedJobsQn;

   /*For future developing, not used yet:*/
#ifdef SKIP
   struct queue_q1_struct toProcessor;
#endif

   struct queue_base_struct **HandlersTable;
   unsigned long int topJobId,maxTopJobId;
   struct job_struct **allJobs;
   unsigned int topHndId;
   int rawHandlersN,freeHandlersN,workingHandlersN;
   struct timeval baseTime;
   int selret;
   char *buf;
   int maxd;
   int repeatmainloop;
   int keepCleintAlive;
   int killServers;
   int retcode;
   int reqs;
   HASH_TABLE nametable;
   int s,r,w;
   fd_set rfds,/*set of reading descriptors*/
          wfds;/*set of writting descriptors*/

};

void clear_jobinfo(void)
{
   if(l_jobinfo!=NULL){
      free(l_jobinfo->filelds);
      free(l_jobinfo->lfields);
      free(l_jobinfo->ofields);
      free_mem(&l_jobinfo);
   }/*if(jobinfo!=NULL)*/

   if(g_jobinfovar_d>-1)
      close(g_jobinfovar_d);
   g_jobinfovar_d=-1;

   if(g_jobinfonam_d>-1)
      close(g_jobinfonam_d);
   g_jobinfonam_d=-1;

   free_mem(&g_jobinfovar);
   free_mem(&g_jobinfonam);

}/*clear_jobinfo*/

/*Allocates, fills up and returnes struct jobinfo_struct *,
AND open files g_jobinfovar and g_jobinfonam:*/
static  struct jobinfo_struct *l_alloc_jobinfo(void)
{
struct jobinfo_struct *tmp=get_mem(1,sizeof(struct jobinfo_struct));
   /*Allocate maximum, then realloc:*/
   tmp->ofields=get_mem(MAX_STR_LEN,sizeof(int));
   tmp->lfields=get_mem(MAX_STR_LEN,sizeof(int));

   /*See comdef.h,
     JOBINFO_TEMPLATE "1 12345678 12345678 12345678 12345678 12 12 12 12 12345678\n"
    */
   tmp->filelds=new_str(JOBINFO_TEMPLATE);

   /*Parse tmp->filelds setting up all fields:*/
   tmp->nfields=0;
   tmp->ofields[0]=0;
   tmp->ltot=0;
   for(;(tmp->filelds)[tmp->ltot]!='\0';){
      while(  (tmp->filelds)[tmp->ltot] >' ' )(tmp->ltot)++;
      (tmp->lfields)[tmp->nfields]= (tmp->ltot) - (tmp->ofields)[tmp->nfields];
      (tmp->nfields)++;
      if( (tmp->filelds)[(tmp->ltot)++] != '\n')
         (tmp->ofields)[tmp->nfields]=(tmp->ltot);
   }/*for(;(tmp->filelds)[tmp->ltot]!='\0';)*/

   /*Realloc:*/
   if( (tmp->ofields=realloc(tmp->ofields,sizeof(int)*(tmp->nfields)))==NULL)
      halt(NOTMEMORY,NULL);
   if( (tmp->lfields=realloc(tmp->lfields,sizeof(int)*(tmp->nfields)))==NULL)
      halt(NOTMEMORY,NULL);

   /*Do not clear_jobinfo on fail here - will be invoked inl_new_jobinfo:*/
   if(g_jobinfovar_d>0)
         close(g_jobinfovar_d);
   if(  (g_jobinfovar_d=open(g_jobinfovar, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) <0 )
      return NULL;
   if(g_jobinfonam_d>0)
      close(g_jobinfonam_d);
   if(  (g_jobinfonam_d=open(g_jobinfonam, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) <0 )
      return NULL;

   return tmp;
}/*l_alloc_jobinfo*/

/*Converts IP in dot notations int HEX format:*/
char *ip2hex(char *ip, char *hexip)
{
char *b;
int d=0;
long int i;

   do{
      i=strtol(ip,&b,10);
      d++;
      if(ip==b)return NULL;
      switch(*b){
         case '.':
            if(d>3)return NULL;
            ip=b+1;
            break;
         case '\0':
            if (d!=4) return NULL;
            break;
         default:
            return NULL;
      }/*switch(*b)*/
      int2hex(hexip+(d-1)*2,i,2);
   }while(d!=4);
   return hexip;
}/*ip2hex*/

/*Updates a file 1 accordint to jobinfo->mask, or creates new record,
  if jobinfo->mask == ~0. Returns 0 in success, -1 on (unspecified)
  error, or -2 if the file 1 is corrupted:*/
static int l_update_jobinfo(struct jobinfo_struct *jobinfo)
{
int i,ibit=1;;
   /*Full string?*/
   if( jobinfo->mask == ~0 ){/*Write full string to the end of file:*/
      off_t fpos=lseek(g_jobinfovar_d,0,SEEK_END); /*If compiler fails here,
                                 try L_XTND instead of SEEK_END*/
      if(  fpos == (off_t)-1 )/*fail*/
         return -1;
      if( writexactly(g_jobinfovar_d, jobinfo->filelds, jobinfo->ltot) )
         return -1;
      return 0;
   }/*if( jobinfo->mask == -1 )*/
   for(i=0; i< jobinfo->nfields; i++){
      if(ibit & (jobinfo->mask)){/*Need to output:*/
         /*If compiler fails here, try L_SET instead of  SEEK_SET:*/
         if( lseek (g_jobinfovar_d,jobinfo->offset+jobinfo->ofields[i],SEEK_SET)
              == (off_t)-1
           )/*fail*/
              return -1;
         if( writexactly(g_jobinfovar_d,
                         jobinfo->filelds+jobinfo->ofields[i],
                         jobinfo->lfields[i] )
           )
              return -1;
      }/*if(ibit & mask)*/
      ibit <<= 1;
   }/*for(i=0; i< jobinfo->nfields; i++)*/
   return 0;
}/*l_update_jobinfo*/

/*auxiliary routine for the next procedure, see case 12,13,14 of
l_get_jobinfo_field:*/
static void l_getToRest(char *buf,int m,
                                  int nn/*from nn to the end*/,
                                  struct job_struct *theJob)
{
   char *args=theJob->args+2;/*now args points to the first arg*/
   int i,n=hex2int(theJob->args,2)-nn+1;/*n is the number of args must be processed*/
      *buf='\0';
      if(n<=0)/*Unsufficient number of args*/
         return ;

      for(nn--;nn>0;nn--)
         args+=(hex2int(args,2)+2);/*skip the arg*/

      for(;n>0;n--,buf++){
         if(  (m-=(i=hex2int(args,2)))<=0  )
            break;/*overflow*/
         s_letn(args+2,buf,i+1);
         args+=(i+2);
         buf+=i;
         *buf=' ';
      }/*for(;n>0;n--;buf++)*/
      *(buf-1)='\0';/*The cycle is performed at least once !*/
}/*l_getToRest*/

static char *l_get_jobinfo_field(char *buf,/*return buf*/
                                int n,/*index*/
                                int m,/*max width*/
                                struct job_struct *theJob,
                                struct localClient_struct *l)
{
   *buf='\0';
   switch(n){
      case 1:
      /*1) current placement:*/
         long2str(buf,theJob->placement);
         break;
      case 2:
         /*2) retcode:*/
         s_let(theJob->retcode,buf);
         break;
      case 3:
         /*3) IP (in dot notation):*/
         if( (theJob->handlerid > 0) ){
            if( *s_let(
               ((struct rhandler_struct*)(l->HandlersTable[theJob->handlerid]->msg))->ip,
               buf) =='\0')
                  s_let("localhost",buf);
         }/*if( (theJob->handlerid > 0) )*/
         if(*buf=='\0') /*Not defined yet*/
            s_let("  -- ",buf);
         break;
      case 4:
         /*4)  PID:*/
         if(theJob->pid<1)
            s_let("  -- ",buf+1);
         else{
            char tc,*ptr=buf+1;
            /*Problem converting PID to string: it may be long, not int!*/
            if (theJob->placement==RUNNING_JOBS_Q){
               *buf=' ';
               long2str(buf+1,theJob->pid);
               tc=' ';
            }else{
               *buf='(';
               long2str(ptr,theJob->pid);
               tc=')';
            }/*if (theJob->placement==RUNNING_JOBS_Q) ... else*/
            while(*ptr!= '\0')ptr++;
            *ptr++=tc;
            *ptr='\0';
         }/*if(theJob->pid<1) ... else*/
         break;
      case 5:
         /*5) - attributes ("type"):*/
         long2str(buf,theJob->type);
         break;
      case 6:
         /*6)  Timeout hits*/
         long2str(buf,theJob->hitTimeout);
         break;
      case 7:
         /*7) fail hits*/
         long2str(buf,theJob->hitFail);
         break;
      case 8:
         /*8)max fail hits*/
         long2str(buf,theJob->maxnhits);
         break;
      case 9:
         /*9) errorlevel, or "-1", or "-2":*/
         long2str(buf,theJob->errlevel);
         break;
      case 10:
         /*10) - name*/
         s_letn(theJob->name,buf,m+1);
         break;
      case 11:
         /*11) - number of cmdline args*/
         long2str(buf,hex2int(theJob->args,2));
         break;
      case 12:
         /*12) - all args started from second*/
         l_getToRest(buf,m,2,theJob);
         break;
      case 13:
         /*13) - all args started from third:*/
         l_getToRest(buf,m,3,theJob);
         break;
      case 14:
         /*14) - all args started from fourth:*/
         l_getToRest(buf,m,4,theJob);
         break;
      default:
         /*Expected to be <0; exact index of cmdline arg starting from 1*/
         if(n<0){
            char *args=theJob->args;
               if( (-n) > hex2int(args,2) )
                  break;
               for(args+=2,n++; n<0;n++)
                 args+=(hex2int(args,2)+2);
               if(  (n=hex2int(args,2)) < m )
                  s_letn(args+2,buf,n+1);
         }/*if(n<0)*/
   }/*switch(n)*/

   return buf;
}/*l_get_jobinfo_field*/

static char *l_getJobInfoLine(char *arg, int njob,struct localClient_struct *l)
{
char *b,*ptr,*resptr,*res=get_mem(MAX_STR_LEN,sizeof(char));
char *buf=get_mem(MAX_STR_LEN,sizeof(char));
struct job_struct *theJob;
int i,n,m,len,reswidth=MAX_STR_LEN-2;
   if( (njob>l->topJobId)||(njob<1) )
      goto getJobInfoLineFails;
   theJob=l->allJobs[njob];
   ptr=arg;
   resptr=res;
   while(*ptr!='\0'){
      n=strtol(ptr,&b,10);/*n may be negative!*/
      if( (*b) !='/')
         goto getJobInfoLineFails;
      ptr=b+1;

      m=strtol(ptr,&b,10);

      if( (*b) == '\0')
         ptr=b;
      else{
         if( (*b) !=':')
            goto getJobInfoLineFails;
         ptr=b+1;
      }/*if( (*b) == '\0') ... else*/
      len=s_len(l_get_jobinfo_field(buf,n,MAX_STR_LEN,theJob,l));

      n=1;/*Need not n , use it as a sign of 'm'*/
      if(m==0)/*The field is outputted as is, without padding:*/
         m=len;
      else if(m<0){
         m=-m;
         n=-1;
      }/*else if(m<0)*/

      if(m>=reswidth)
         goto getJobInfoLineFails;

      if(len>=m){
         s_letn(buf,resptr,m+1);
      }else if (n>0){/*right justification, left padding*/
         memset(resptr,' ',i=m-len);
         resptr+=i;
         s_let(buf,resptr);
      }else{/*left justification, right padding*/
         s_let(buf,resptr);
         resptr+=len;
         memset(resptr,' ',m-len);
      }/*if(len>=m)...else...else*/
      while(*resptr!='\0')resptr++;

      if(*ptr!='\0')
         *(resptr++)=' ';

      reswidth-=(m+1);
   }/*for(ptr=arg;*ptr!='\0';)*/
   free(buf);
      return res;
   getJobInfoLineFails:
      free(buf);
      free(res);
      return get_mem(1,sizeof(char));
}/*l_getJobInfoLine*/

/*Fills in all fields except 9) - address of a corresponding line in 2 file:*/
static void l_fillup_jobinfo(char *buf,
                            struct job_struct *theJob,
                            struct localClient_struct *l)
{
   if(l_jobinfo->mask& 1){
      /*0) X(1)        - HEX number, current placement:*/
      int2hex(
         l_jobinfo->filelds + l_jobinfo->ofields[0],
         theJob->placement,
         -(l_jobinfo->lfields[0])
      );
   }/*if(l_jobinfo->mask& 1)*/

   if(l_jobinfo->mask& 2){
      /*1) XXXXXXXX(8) - retcode:*/
      memcpy(l_jobinfo->filelds + l_jobinfo->ofields[1],
         theJob->retcode,
         l_jobinfo->lfields[1]);
   }/*if(l_jobinfo->mask& 2)*/

   if(l_jobinfo->mask& 4){
      /*2) XXXXXXXX(8) - IP (in HEX notation):*/
      if( (theJob->handlerid < 1) )
         memset(l_jobinfo->filelds + l_jobinfo->ofields[2],
             '0',
             l_jobinfo->lfields[2]);
      else{
         struct rhandler_struct *h=(struct rhandler_struct*)
                  (l->HandlersTable[theJob->handlerid]->msg);
         if(ip2hex(h->ip, buf) == NULL)/*Local server*/
            memset(l_jobinfo->filelds + l_jobinfo->ofields[2],
             '0',
             l_jobinfo->lfields[2]);
         else
            memcpy(l_jobinfo->filelds + l_jobinfo->ofields[2],
               buf,
               l_jobinfo->lfields[2]);
      }/*if( (theJob->handlerid < 1) ) ... else */
   }/*if(l_jobinfo->mask& 4)*/

   if(l_jobinfo->mask& 8){
      /*3) XXXXXXXX(8) - PID:*/
      int2hex(
         l_jobinfo->filelds + l_jobinfo->ofields[3],
         theJob->pid,
         -(l_jobinfo->lfields[3])
      );
   }/*if(l_jobinfo->mask& 8)*/

   if(l_jobinfo->mask& 16){
      /*4) XXXXXXXX(8) - attributes ("type"):*/
      int2hex(
         l_jobinfo->filelds + l_jobinfo->ofields[4],
         theJob->type,
         -(l_jobinfo->lfields[4])
      );
   }/*if(l_jobinfo->mask& 16)*/

   if(l_jobinfo->mask& 32){
      /*5) XX(2)       - HEX Timeout hits*/
      int2hex(
         l_jobinfo->filelds + l_jobinfo->ofields[5],
         theJob->hitTimeout,
         -(l_jobinfo->lfields[5])
      );
   }/*if(l_jobinfo->mask& 32)*/

   if(l_jobinfo->mask& 64){
      /*6) XX(2)       - HEX fail hits*/
      int2hex(
         l_jobinfo->filelds + l_jobinfo->ofields[6],
         theJob->hitFail,
         -(l_jobinfo->lfields[6])
      );
   }/*if(l_jobinfo->mask& 64)*/

   if(l_jobinfo->mask& 128){
      /*7) XX(2)       - HEX max fail hits*/
      int2hex(
         l_jobinfo->filelds + l_jobinfo->ofields[7],
         theJob->maxnhits,
         -(l_jobinfo->lfields[7])
      );
   }/*if(l_jobinfo->mask& 128)*/

   if(l_jobinfo->mask& 256){
      /*8) XX(2)       - HEX errorlevel, or "-1", or "-2":*/
      switch(theJob->errlevel){
         case -1: case -2:
            memset(l_jobinfo->filelds + l_jobinfo->ofields[8],
               '0',
               l_jobinfo->lfields[8]);
            *(l_jobinfo->filelds + l_jobinfo->ofields[8]+ l_jobinfo->lfields[8] -2)='-';
            int2hex(
                l_jobinfo->filelds + l_jobinfo->ofields[8]+ l_jobinfo->lfields[8] -1,
                -(theJob->errlevel),
                -1
            );
            break;
         default:
            int2hex(
               l_jobinfo->filelds + l_jobinfo->ofields[8],
               theJob->errlevel,
               -(l_jobinfo->lfields[8])
            );
      }/*switch(theJob->errlevel)*/
   }/*if(l_jobinfo->mask& 256)*/

}/*l_fillup_jobinfo*/

static int l_new_jobinfo(struct job_struct *theJob,struct localClient_struct *l)
{
off_t fpos;
char *ptr,*buf=get_mem(MAX_STR_LEN,1);
int flen;
   /*Here we trust both g_jobinfovar and g_jobinfonam != NULL*/
   if( l_jobinfo == NULL ){
      if(   (l_jobinfo=l_alloc_jobinfo())==NULL  )
         goto new_jobinfo_fail;
   }/*if( l_jobinfo == NULL )*/

   /*Now all files are opened and the structure is allocated*/

   /*Read the offset of a file 2:*/
   /*If compiler fails here, try L_INCR instead of SEEK_CUR:*/
   if(  (fpos=lseek (g_jobinfonam_d,0,SEEK_CUR)) == (off_t)-1 )/*fail*/
      goto new_jobinfo_fail;

   /*Build in buf the structure to put in file 2:*/
   ptr=long2str(buf, theJob->id);/*Put the order number*/
   while(*ptr != '\0')ptr++;

   *ptr++ = ' ';/*separator (' ')*/
   {/*block*/
      char *tmp=theJob->name;   /*Copy a name:*/
      for(;(*ptr=*tmp)!=0;ptr++,tmp++);
      *ptr++=' ';/*separator (' ')*/
      /*Copy args:*/
      for(tmp=theJob->args;(*ptr=*tmp)!=0;ptr++,tmp++);
      *ptr++='\n';
      *ptr='\0';
   }/*block*/
   flen=ptr-buf;
   if( writexactly(g_jobinfonam_d, buf, flen) )
      goto new_jobinfo_fail;

   /*fill up the structure:*/
   l_jobinfo->mask = ~0;/*all fields*/
   l_fillup_jobinfo(buf,theJob,l);
   /*All except 9) are ready*/

   /*9) XXXXXXXX(8) - address of a corresponding line in 2 file*/
      int2hex(
         l_jobinfo->filelds + l_jobinfo->ofields[9],
         fpos,
         -(l_jobinfo->lfields[9])
      );

   /*The structure is filled up*/

   /*ATT! l_jobinfo->offset is NOT filled! Since mask = -1, the string will be appended
     to the end of file!*/

   /*Need not buffer anymore:*/
   free(buf);
   /*Write it to the file and */
   return l_update_jobinfo(l_jobinfo);

   new_jobinfo_fail:
      free(buf);
      clear_jobinfo();
      return -1;
}/*l_new_jobinfo*/

void l_closeAllDescriptors(int startFrom)
{
register int maxfd=sysconf(_SC_OPEN_MAX);
     for(;startFrom<maxfd;startFrom++)
        close(startFrom);
}/*l_closeAllDescriptors*/

long int remaining_time(
                           struct timeval *sv,/* Start time*/
                           long int t/*initial timeout in milliseconds*/
                       )
{
struct timeval cv;
long int ret;
   if(t<0)return 0;
   gettimeofday(&cv, NULL);

   ret=
      (sv->tv_sec-cv.tv_sec+t / 1000)/*seconds remaining*/
      *1000/*millisecs come from  round seconds*/
      +
      (sv->tv_usec-cv.tv_usec)/1000+(t%1000);/*millisecs come from microsecs and t % 1000*/
   if(ret<0)return 0;

   return ret;
}/*remaining_time*/

/*The function may be used to adjust a timeout for select() call. Parameter sv must contain
a time returned gettimeofday() at start, the parameter t -s the timeout in milliseconds.
Based on these two parameters, the function evaluates remaining time, puts it into tv and
returns a pointer to it. If the timeout is already expired, it returns 0. If t<0,
the function returns NULL leaving tv untouched.*/
struct timeval *adjust_timeout(
                                 struct timeval *sv,/* Start time*/
                                 struct timeval *tv,/* Produced timeout*/
                                 long int t/*initial timeout in milliseconds*/
                              )
{
struct timeval cv;

   if(t<0)/*No timeout, infinity*/
      return NULL;

   gettimeofday(&cv, NULL);

   /*Subtract from timeout the difference between current time and start time:*/
   tv->tv_sec=sv->tv_sec-cv.tv_sec+t / 1000;
   tv->tv_usec=sv->tv_usec-cv.tv_usec+(t % 1000)*1000;

   if (tv->tv_usec <0){
      /*Normalize:*/
      if(  (t= (-tv->tv_usec) % 1000000)!=0 ){
         tv->tv_sec-=( (-tv->tv_usec)/1000000 + 1);
         tv->tv_usec = 1000000 - t;
      }else{/*if(  (t=tv->tv_usec % 1000000)!=0 )*/
         tv->tv_sec-= ( (-tv->tv_usec)/1000000 );
         tv->tv_usec = 0;
      }
   }/*if (tv->tv_usec <0)*/
   if(tv->tv_sec < 0)/*if tv->tv_sec < 0 , then the timeout is expired*/
      tv->tv_sec=tv->tv_usec=0;
   return tv;
}/*adjust_timeout*/

static void l_exitFromisServerOk(int fd, int thesocket)
{
   write(fd,"00000000",8);
   close(fd);
   close(thesocket);
   exit(0);
}/*l_exitFromisServerOk*/

/*The problem is that the TCP timeout may be extremely huge.*/
int query_server(
                  char *ip,
                  unsigned short int theport,
                  char *passwd,
                  int (*perform_query)(int)
                )
{
int ret=0;
int fd[2];
char buf[2];
pid_t childpid;
int thesocket;

   /*First, create a socket: */
   if (( thesocket= socket(AF_INET,SOCK_STREAM,0)) <1)
      return 0;

   /* Open a pipe:*/
   if (pipe(fd)!= 0) return 0;
   /* Fork to create the child process:*/
   if((childpid = fork()) == -1)
      return 0;
   if(childpid){/*The father - waits CONNECT_TIMEOUT while the child will
                        establish a connection:*/
      struct timeval sv,tv;
      fd_set ds;
      int continuepolling=1;
         close(fd[1]);/*Close up writable pipe*/
         /*Now may read from fd[0]*/
         gettimeofday(&sv, NULL);/*Store starting time*/
         do{
            FD_ZERO(&ds);
            FD_SET(fd[0],&ds);
               switch(select(fd[0]+1, &ds, NULL, NULL,
                                            adjust_timeout(&sv,&tv,2*CONNECT_TIMEOUT) ) ){
                     case 1:/*Child screams*/
                        {/*block*/
                         char buf[9];
                           ret=readHex(fd[0],buf,8,NULL,0);
                        }/*block*/
                        continuepolling=0;
                        break;
                     default:
                        if (errno == EINTR)/*Interrupted by a signal, just continue:*/
                           continue;
                        /*else - timeout, or something is wrong. Give it up!*/
                        continuepolling=0;
                        break;
               }/*switch*/
         }while(continuepolling);
         close(fd[0]);
         close(thesocket);
         /*Kill the child!*/
         kill(childpid,SIGKILL);
         waitpid(childpid,&continuepolling,0);
   }else{/*The child*/
      struct sockaddr_in address;
      char theretbuf[9];
      close(fd[0]);/*Close up readable pipe*/
      /*Now may write to fd[1]*/
      /**/

      /*
         Problems with inet_pton in some platforms, this is
         non-standart function!
         inet_pton(AF_INET,h->ip,&address.sin_addr);
      */

      /*Instead:*/

      memset(&address,'\0',sizeof(struct sockaddr_in));
      address.sin_family = AF_INET;
      address.sin_port = htons(theport);

      address.sin_addr.s_addr=inet_addr(ip);

      if (connect(thesocket,(struct sockaddr *)&address,sizeof(address)) != 0)
         l_exitFromisServerOk(fd[1],thesocket);

      /*Authorization:*/
      if(  writexactly(thesocket,passwd,s_len(passwd))  )
         l_exitFromisServerOk(fd[1],thesocket);
      if(  readexactly(thesocket,buf,2)  )
         l_exitFromisServerOk(fd[1],thesocket);
      if( hex2int(buf,2)!= s2cOK_i )
         l_exitFromisServerOk(fd[1],thesocket);
      /*Ok.*/

      write(fd[1],int2hex(theretbuf,perform_query(thesocket),8),8);

      close(fd[1]);
      close(thesocket);

      pause();/*Sleep waiting sigkill from the father*/

      exit(0);/*We wouldn't like to go further, if pause() breaks by a signal, eh?*/
   }/*if(childpid) ... else*/

   return ret;
}/*query_server*/

/*This simple function only checks is IP has a structure like x[xx].x[xx].x[xx].x[xx]
where x is a digit 0...9. Returns 0 if not, or >0:*/
int isIp(char *ip)
{
register int p,d;
      for(d=p=0;*ip!='\0';ip++)switch(*ip){
         case '0':case '1':case '2':case '3':case '4':case '5':
         case '6':case '7':case '8':case '9':
            if( (++d)>3 ) return 0;
            break;
         case '.':
            if( (++p)>3 ) return 0;
            if(d==0) return 0;
            d=0;
            break;
         default:
            return 0;
      }/*for(d=p=0;*ip!='\0';ip++)switch(*ip)*/
      return d;
}/*isIp*/

/*1 on success:*/
int queryTheServer(char *ip,  int (*perform_query)(char*,unsigned short int,char*) )
{
char *fn=new_str(g_srv_dir);
FILE *f=NULL;
unsigned short int theport;
int ret=0;
mysighandler_t oldPIPE;
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   oldPIPE=signal(SIGPIPE,SIG_IGN);

   if( !isIp(ip) ) goto pingSrvFails;
   fn=s_inc(fn,SRVFILENAME);
   fn=s_inc(fn,ip);
   if(  (f=fopen(fn,"r"))== NULL) goto pingSrvFails;
   /*We need not fn anymore, but we will use it as a buffer. Indeed, the lengt of
   it is at least of IP, so it is enough, if the length of SRVFILENAME >2.*/

   /*On the offchance:*/
   free(fn); fn=get_mem(17, sizeof(char));

   /*Read a port number:*/
   if(fgets(fn,9,f)==NULL)goto pingSrvFails;
   if(  sscanf(fn,"%hu",&theport )!=1  )goto pingSrvFails;

   /*Read the password:*/
   if(fgets(fn,16,f)==NULL)goto pingSrvFails;
   {/*block*/
   int tmp;
      if(  sscanf(fn,"%d",&tmp )!=1  )goto pingSrvFails;
      if(tmp<0)goto pingSrvFails;
      int2hex(fn,tmp,8);
   }/*block*/

   ret=perform_query(ip,theport,fn);

   pingSrvFails:
      free(fn);
      if(f!=NULL)
         fclose(f);
      /* And now, restore old signals:*/
      signal(SIGPIPE,oldPIPE);
      return ret;
}/*queryTheServer*/

/*ATT! The following function is NOT thread-safe, it uses the external buffer
  to get parameters/send the result!
*/
static char l_thesig[4];
static char l_pid[17];
static int l_q_sendAsignal(int thesocket)
{
int l;
      if(writexactly(thesocket,c2sSENDSIG,2))/*fail*/
         return 0;
      /*Send  2000 (HEX) - 8192(DEC) - startup & 8192 means "send a signal":*/
      if(writexactly(thesocket,"00002000",8))/*fail*/
         return 0;
      if(writexactly(thesocket,l_thesig,3))/*fail*/
         return 0;
      /*Now l_thesig will be used as a buffer, its contens will be lost!:*/
      /*send a width:*/
      if( writexactly(thesocket,int2hex(l_thesig,l=s_len(l_pid),2),2) )
         return 0;
      /*send a pid:*/
      if(writexactly(thesocket,l_pid,l))/*fail*/
         return 0;
      /*Read the result of sending kill, one byte:*/
      return readHex(thesocket,l_thesig,2,NULL,0);
}/*l_q_sendAsignal*/

int sendAsignal(char *ip,unsigned short int theport, char *passwd)
{
   return query_server(ip,theport,passwd, &l_q_sendAsignal);
}/*sendAsignal*/

/*1 if success to talk, otherwise, 0:*/
int sendSig(char *ip)
{
   return queryTheServer(ip,&sendAsignal);
}/*sendSig*/

/*The following function kills the server job forked out on ping request:*/
static int l_q_isServerOk(int thesocket)
{
   /*Kill it:*/
   writexactly(thesocket,c2sBYE,2);
   return 1;
}/*l_q_isServerOk*/

int isServerOk(char *ip,unsigned short int theport, char *passwd)
{
   return query_server(ip,theport,passwd, &l_q_isServerOk);
}/*isServerOk*/

static int l_q_killTheServer(int thesocket)
{
   writexactly(thesocket,c2sDIE,2);
   return 1;
}/*l_q_killTheServer*/

int kill_server(char *ip,unsigned short int theport, char *passwd)
{
   if(*ip == '\0')
      return 0;
   return query_server(ip,theport,passwd, &l_q_killTheServer);
}/*kill_server*/

/*1 if alive, otherwise, 0:*/
int pingSrv(char *ip)
{
   return queryTheServer(ip,&isServerOk);
}/*pingSrv*/

/*1 on success, if it can't connect with the server, 0:*/
int killSrv(char *ip)
{
   return queryTheServer(ip,&kill_server);
}/*killSrv*/

typedef void SWSUB1(int, int);
typedef void SWSUB2(void);
/*If argv!=NULL, the function forks and executes the command "cmd". If both
  fdsend!=NULL and fdreceive!= NULL it returns into these pointers the
  descriptors of  stdin and stdout of swallowed program. arg[] is
  a NULL-terminated array of cmd arguments.

  If argv==NULL, the function assumes that cmd is a pointer to some function
  void cmd(int*,int*)(if both fdsend!=NULL and fdreceive!= NULL) or
  void cmd(void) if fdsend==NULL or fdreceive==NULL.*/
pid_t run_cmd(
   int *fdsend,
   int *fdreceive,
   int ttymode,/*0 - nothing, &1 - reopen stdin &2 - reopen stdout &4 - daemonizeing
                 &8 - setsid() if not daemonizeing*/
   char *cmd,
   char *argv[]
   )
{/**/
int fdin[2], fdout[2], fdsig[2];
pid_t childpid;

   if(  (fdsend!=NULL)&&(fdreceive!=NULL)  ){
       if(  /* Open two pipes:*/
            (pipe(fdin)!=0)
          ||(pipe(fdout)!=0)
         )return(-1);
   }/*if(  (fdsend!=NULL)&&(fdreceive!=NULL)  )*/

   if(pipe(fdsig)!=0) return -1;/*This pipe will be used by a child to
                                  tell the father if fail.*/

   /* Fork to create the child process:*/
   if((childpid = fork()) == -1)
       return(-1);

  if(childpid == 0){/*Child.*/
     int i,maxfd=fdsig[1];

     close(fdsig[0]);

     for(i=3;i<maxfd;i++){
        if(   (i != fdin[0])
            &&(i != fdin[1])
            &&(i != fdout[0])
            &&(i != fdout[1])
            &&(i != fdsig[1])
          )
           close(i);
     }/*for(i=3;i<maxfd;i++)*/

     l_closeAllDescriptors(maxfd+1);

     if(  (fdsend!=NULL)&&(fdreceive!=NULL)  ){
           if(
              (close(fdin[1]) == -1 )/*Close up parent's input channel*/
              ||(close(fdout[0])== -1 )/* Close up parent's output channel*/
             )/*Fail!*/
                exit(1);
           if(ttymode & 1){/*Reopen stdin:*/
              if(
                   (close(0) == -1 )/* Use fdin as stdin :*/
                 ||(dup(fdin[0]) == -1 )

                )/*Fail!*/
                   exit(1);
              close(fdin[0]);
           }/*if(ttymode & 1)*/
           if(ttymode & 2){/*Reopen stdout:*/
              if(
                   (close(1)==-1)/* Use fdout as stdout:*/
                 ||( dup(fdout[1]) == -1 )
                )/*Fail!*/
                   exit(1);
                close(fdout[1]);
           }/*if(ttymode & 2)*/
     }/*if(  (fdsend!=NULL)&&(fdreceive!=NULL)  )*/

     if( ttymode & 4 ){/*Daemonize*/
        switch(childpid=fork()){
           case 0:/*grandchild*/
              if(   !( ttymode & 1 )   ){
                       /*stdin < /dev/null*/
                       close(0);/*close stdin*/
                       open("/dev/null",O_RDONLY); /* reopen stdin */
              }/*if(   !( ttymode & 1 )   )*/
              if(   !( ttymode & 2 )   ){
                       /*stdout > /dev/null*/
                       close(1);/*close stdin*/
                       open("/dev/null",O_WRONLY); /* reopen stdout */
              }/*if(   !( ttymode & 1 )   )*/
              /*stderr > /dev/null*/
              close(2);/*close stderr*/
              open("/dev/null",O_WRONLY); /* reopen stderr*/

              setsid(); /* Detach from the current process group and
                           obtain a new process group */
              if(argv!=NULL){/*Execute external command:*/
                 if(set_cloexec_flag (fdsig[1], 1)!=0)
                    close(fdsig[1]);/*To avoid blocking of the father*/
                 /*Execute external command:*/
                 execvp(cmd, argv);
              }else{/*Run some function:*/
                 /*We need not this pipe here:*/
                 close(fdsig[1]);
                 if(  (fdsend!=NULL)&&(fdreceive!=NULL)  )
                    ( (SWSUB1*)cmd)(fdout[1],fdin[0]);
                 else
                    ( (SWSUB2*)cmd)();
                 exit(2);/*Tha's all, the pipe is closed!*/
              }/*if(argv!=NULL)...else*/
              /*No break;*/
           case -1:
              /* Control can  reach this point only on error!*/
              writexactly(fdsig[1],"-1",2);/*Inform the father about the failure*/
              exit(2);
           default:/*Son of his father*/
              /*Send a grandchild pid to the grandfather:*/
              writeLong(fdsig[1],childpid);
              close(fdsig[1]);/*Close the descriptor - now it is opened only
                                by a grandchild*/
              exit(0);
        }/*switch(childpid=fork())*/
     }else{/*if( ttymode & 4 )*/

        if( ttymode & 8 ) /*become a session leader:*/
           setsid();

        if(argv!=NULL){/*Execute external command:*/
           if(set_cloexec_flag (fdsig[1], 1)!=0)
              close(fdsig[1]);/*To avoid blocking of the father*/
           /*Execute external command:*/
           execvp(cmd, argv);
        }else{/*Run some function:*/
           /*We need not this pipe here:*/
           close(fdsig[1]);
           if(  (fdsend!=NULL)&&(fdreceive!=NULL)  )
              ( (SWSUB1*)cmd)(fdout[1],fdin[0]);
           else
              ( (SWSUB2*)cmd)();
           exit(2);/*Tha's all, the pipe is closed!*/
        }/*if(argv!=NULL)*/
        /* Control can  reach this point only on error!*/
        writexactly(fdsig[1],"-1",2);
        exit(2);
     }/*if( ttymode & 4 )...else*/
  }else{/* The father*/
     char buf[2];

     close(fdsig[1]);

     if(  (fdsend!=NULL)&&(fdreceive!=NULL)  ){
        if(
		           (close(fdin[0])==-1)/* Close up output side of fdin*/
           ||(close(fdout[1])==-1)/*Close up input side of fdout*/
          ){/**/
            close(fdin[1]);
            close(fdout[0]);
            childpid = -childpid;/*Negative childpid indicates an error*/
        }else{/*Success*/
           *fdsend=fdin[1];
           *fdreceive=fdout[0];
        }
     }/*if(  (fdsend!=NULL)&&(fdreceive!=NULL)  )*/
     /*Now if childpid == -1 then something is wrong*/
     if(childpid <0){/*Negative childpid indicates an error*/
         kill(childpid,SIGHUP); /* warn the child process */
         kill(childpid,SIGKILL);/* kill the child process*/
     }else{
        if( ttymode & 4 ){/*Daemonize*/
           char buf[17];
           int l;
           /*Race condition may occur! Both grandchild and the child may write
             into the pipe simultaneously.*/
           /*Note here, for the size of an atomic IO operation, the  POSIX standard
             dictates 512 bytes. Linux PIPE_BUF is quite considerably, 4096.
             Anyway, in all systems we assume PIPE_BUF>20.*/
           *buf = '+';
           if(  (l=readHex(fdsig[0],buf,2,NULL,0))<1  ){
              childpid=-1;
              if(*buf == '-')/*Race condition?*/
                 if(  (l=readHex(fdsig[0],buf,2,NULL,0))>0  )
                    /*Race condition, need to read pid from the child:*/
                    readHex(fdsig[0],buf,l,NULL,0);/*read the pid of the grandchild*/
                    /*We need not the read pid since the grandchild already fails.*/
           }else/*in l we have a length of  grandchild pid.*/
              childpid=readHex(fdsig[0],buf,l,NULL,0);/*read the pid of the grandchild*/
           /*Here in childpid we have a grandchild pid, or -1 if it fails.*/
        }/*if( ttymode & 4 )*/
        if(childpid >0)/*Try to read the fail signal:*/
           if (read(fdsig[0],buf,2)==2)childpid=-1;/*Fail!*/
     }/*if(childpid <0)..else*/
  }/*The father*/

  close(fdsig[0]);
  /*Here can be ONLY the father*/
  if( ttymode & 4 )/*Daemonize*/
     wait(fdsig);/*Wait while the father of a grandchild dies*/

  return(childpid);
}/*run_cmd*/

/*The maximal peer queue length - if it is longer, then we will stop to listen swallowed:*/
#define MAX_PEER_Q_LEN 32768

#define READ_BUF_LEN 1024

size_t g_socket_buf_len=512;

size_t g_pipebuf_len=512;

#define CMD_LEN 5
struct cmdFromPeer_struct;
struct localMkchat_struct;
struct queue_q1_struct;

/*Formal handler*/
typedef int (*CMDHANDLER)(struct cmdFromPeer_struct *c,
                          void *l
                          );

/*Data for parsePeerCmd:*/
struct cmdFromPeer_struct{
   char *fromPeer;/*Buffer to read data from peer*/
   ssize_t fromPeerLen;/*the length of data read from peer*/
   ssize_t fromPeerAlloc;/*the length of allocated buffer*/
   int icmd;/*integer code of a command*/
   char cmd[CMD_LEN];/*a command/parameter buffer*/
   char *par;/*a pointer to  parameter buffer, may be allocated in the heap
                    (if the lenght required is > CMD_LEN) or pints to cmd*/
   char isalloc;/*1|0 depending in is par allocated, or it is pointing to cmd*/
   int ind;/*length of currently parsed portion of a command*/
   int parind;/*current position in a "par" buffer*/
   int maxparind;/*total space available for par*/
   long int datac;/*if >0, then datac bytes must bi copied from peer directly to outputQueue*/
   struct queue_q1_struct *outputQueue;/*output queue - only meaningful if datac >0*/
   CMDHANDLER handler;/*A command handler*/
};

struct localMkchat_struct{
   int fdsend, fdreceive,socksend,sockrec;/*descriptors*/

   fd_set rfds,/*set of reading descriptors*/
          wfds;/*set of writting descriptors*/
   struct timeval tv;/*timeout*/
   struct timeval sv;/* Start time*/
   char *buf;/*Just a buffer*/
   ssize_t bufAlloc;/*Length of allocated buffer*/
   ssize_t bufLen;/*Current length of the buffer*/
   pid_t childpid;/*PID of swallowed command*/
   struct queue_q1_struct
      peer_queue,/*Queue of messages adddressed to peer*/
      send_queue,/*Queue of messages adddressed to swallowed command*/
      stdout_queue,/*Queue of messages adddressed to stdout*/
      stderr_queue;/*Queue of messages adddressed to stderr*/

   int
      status,/*Returned status of a swallowed command*/
      selret,/*result of select()*/
      peer_listen,/*!0 - listen peer, 0 - not*/
      swallowed_listen,/*!0 - listen swallowed, 0 - not*/
      stdin_peer, /*!0- pass stdin to peer, 0 - not*/
      stdin_swallow,/*!0- pass stdin to swallowed, 0 - not*/
      stdout_swallow,/*!0- pass output of swallowed to stdout,  0 - not*/
      peer_swallow,/*!0- pass output of swallowed to peer, 0 - not*/
      repeatmainloop,/*Flag for leaving the main loop*/
      ret,/*Returned value for mkchat*/
      timeouthits,/*How many times it hits timeout*/
      ttymode,/*Flags for run_cmd*/
      maxd;/*first argument for select()*/
   struct cmdFromPeer_struct cmdFromPeer;/*Data for parsePeerCmd*/
   mysighandler_t oldPIPE;/*Stored sigpipe handler*/
};

/*+1 if is running,-1 if stopped, 0 if dead:*/
static int l_isalive(int hang,struct localMkchat_struct *l)
{
pid_t ret;
   if(hang)
      ret=waitpid(l->childpid, &l->status,0);
   else
      ret=waitpid(l->childpid, &l->status,WNOHANG|WUNTRACED);
   if(   ret==0  ){
      /*See rproto.h*/add_q1_queue(2,c2pWRK,&l->peer_queue);/*running*/
      return 1;
   }/*if(   (ret=waitpid(l->childpid, &l->status,WNOHANG|WUNTRACED))==0  )*/

   if(WIFSTOPPED(l->status)){/*Stopped!*/
      /*See rproto.h*/add_q1_queue(2,c2pSTOPPED,&l->peer_queue);/*stopped*/
      return -1;
   }/*if(WIFSTOPPED(l->status))*/
   if(ret==l->childpid){
      /*See rproto.h*/add_q1_queue(2,c2pFIN,&l->peer_queue);/*Died*/
      /*Report the status as hex number:*/
      /*
         HEX status is: 8 HEX digits:
            xx   : if normal exit, then the first two digits repersents exit() argument,
                   othervice 00
            xx   : 00 - notmal exit, 01 - non-caught signal, otherwise - ff.
            xxxx : - signal number ( if previous was 01), otherwise - 0000. If signal
                   is incompatible, then 0000
       */
      if(WIFEXITED(l->status)){/*Normal exitting, report exit status:*/
         add_q1_queue(2,int2hex(l->buf,WEXITSTATUS(l->status),2),&l->peer_queue);
         /* "00" since no signals, and "0000" - a signal number:*/
         add_q1_queue(6,"000000",&l->peer_queue);
      }else{/*Exit due to a signal?*/
         if(WIFSIGNALED(l->status)){/*A  signal which was not caught*/
            int signum;
            add_q1_queue(4,"0001",&l->peer_queue);
            signum=sig2int(WTERMSIG(l->status));
            add_q1_queue(4,int2hex(l->buf,signum,4),&l->peer_queue);
         }else{/*Nobody knows!*/
             add_q1_queue(8,"00ff0000",&l->peer_queue);
         }
      }

      return 0;
   }/*if(ret==l->childpid)*/
   /*The last possibility, may occur if a child is attached to the debugger, or
     when multiply l_isalive:*/
   /*See rproto.h*/add_q1_queue(2,c2pDIED,&l->peer_queue);/*Died, status is lost */
   return 0;
}/*l_isalive*/

static void l_flushpeerq(struct localMkchat_struct *l)
{
char *buf;
size_t len,mem=0;

   while( (len=get_q1_queue(&buf, &l->peer_queue))>0  ){
      if( writexactly(l->socksend, buf, len) )break;
      accept_q1_queue(len,&l->peer_queue);
      if(mem == len){
         break;
      }/*if(mem == len)*/
   }/*while( (len=get_q1_queue(&buf, &l->peer_queue))>0  )*/
}/*flushpeerq*/

/*Real handler:*/
int mkchat_h(struct cmdFromPeer_struct *c, void *localMkchat )
{
struct localMkchat_struct *l=(struct localMkchat_struct *)localMkchat;

   if(c->ind==2)switch(c->icmd){/*c->icmd is ready (c->ind was incremented)*/
      case p2cDATASW_i:/*Read a lenght of data to swallowed, 4*/
      case p2cDATASO_i:/*Read a lenght of data to stdout, 4*/
      case p2cDATASE_i:/*Read a lenght of data to stderr, 4*/
      case p2cFORK_i:/*Go to backgroud redirecting output to the file*/
         return 4;/*read 4 char - hex length of the filename*/
      case p2cSW2STDOUTstart_i:l->stdout_swallow=1;return 0;
      case p2cSW2STDOUTstop_i:l->stdout_swallow=0;return 0;
      case p2cSTDIN2PEERstart_i:l->stdin_peer=1;return 0;
      case p2cSTDIN2PEERstop_i:l->stdin_peer=0;return 0;
      case p2cSTDIN2SWstart_i:l->stdin_swallow=1;return 0;
      case p2cSTDIN2SWstop_i:l->stdin_swallow=0;return 0;

      case p2cSIG_i:/*Send a signal*/
         return 2;/* We will use some "abstract" short integers to index signals,
                     to be system-independent*/
      case p2cISALIVE_i:/*Is it alive?*/
         l_isalive(0,l);
         return 0;
      case p2cRUNANDDIE_i:
         l_isalive(1,l);
         l_flushpeerq(l);
         l->repeatmainloop=0;
         l->ret=1;
         return 0;
      case p2cRUNANDNEXT_i:
         l_isalive(1,l);
         l_flushpeerq(l);
         l->repeatmainloop=0;
         l->ret=0;
         return 0;
      case p2cDIE_i:/*Finish*/
         l->repeatmainloop=0;
         l->ret=1;
         return 0;
      case p2cNEXT_i:/*Go to the next task*/
         l->repeatmainloop=0;
         l->ret=0;
         return 0;
      case p2cOK_i:/*"ok", reserved*/return 0;
      case p2cNOK_i:/*"Fail", reserved*/return 0;
      case p2cREADSWstop_i:/*Do not read data from swallowed*/
         l->peer_swallow=0;return 0;
      case p2cREADSWstart_i:/*Read data from swallowed*/
         l->peer_swallow=1;return 0;
      default:/*Unknown command - just skip it:*/
         return 0;
   }else /*if(c->ind==2)switch(c->icmd)*/
   if(c->ind>2)switch(c->icmd){
      case p2cDATASW_i:/*Now in c->par we have hex number - the length of data to be sent to swallowed*/
         c->datac=hex2int(c->par,c->maxparind);
         c->outputQueue=&l->send_queue;
         return 0;
      case p2cDATASO_i:/*Now in c->par we have hex number - the length of data to be sent to stdout*/
         c->datac=hex2int(c->par,c->maxparind);
         c->outputQueue=&l->stdout_queue;
         return 0;
      case p2cDATASE_i:/*Now in c->par we have hex number - the length of data to be sent to stderr*/
         c->datac=hex2int(c->par,c->maxparind);
         c->outputQueue=&l->stderr_queue;
         return 0;
      case p2cFORK_i:/*Go to backgroud redirecting output to the file*/
         if(c->ind==6)/*Now in c->par we have hex number - the length of the filename*/
            return hex2int(c->par,c->maxparind);
         else{/*Now in c->par we have the file name*/
            int optfd=open(c->par,O_WRONLY|O_CREAT,S_IRWXU);
            pid_t lpid;
            if(optfd<0){
               /*See rproto.h*/add_q1_queue(2,c2pNOK,&l->peer_queue);/*Fail*/
               return 0;
            }/*if(optfd<0)*/
            if(  (lpid=fork())<0  )
               /*See rproto.h*/add_q1_queue(2,c2pNOK,&l->peer_queue);/*Fail*/
            else if (lpid!=0){/*The father*/
               close(optfd);
               /*See rproto.h*/add_q1_queue(2,c2pOK,&l->peer_queue);/*ok*/
               l->repeatmainloop=0;
               return 0;
            }
            /*Child!*/
            clear_q1_queue(&l->stderr_queue);
            clear_q1_queue(&l->stdout_queue);
            clear_q1_queue(&l->send_queue);
            clear_q1_queue(&l->peer_queue);

            if(c->isalloc)
               free(c->par);
            free_mem(&l->cmdFromPeer.fromPeer);
            while(1){
               ssize_t r=read(l->fdreceive,l->buf,l->bufAlloc);
               if(r<0)exit(0);
               write(optfd,l->buf,r);
            }/*while(1)*/
         }/*if(c->ind==6) ... else*/
      case p2cSIG_i:/*Send a signal*/
         /*Now in c->par we have hex number of a signal*/
         {/*block*/
            int s;
               if(   ( s=int2sig(hex2int(c->par,2)) )==0   )
                  /*See rproto.h*/add_q1_queue(2,c2pNOK,&l->peer_queue);/*No such a signal*/
               else{
                  kill(l->childpid,s);
                  /*See rproto.h*/add_q1_queue(2,c2pOK,&l->peer_queue);
               }
         }/*block*/
         return 0;

      default:/*Unknown command - just skip it:*/
         return 0;

   }/*if(c->ind>2)switch(c->icmd)*/
   return 0;
}/*mkchat_h*/

static void l_clearCmd(struct cmdFromPeer_struct *c)
{
   c->parind=0;
   c->ind=0;
   c->icmd=-1;
   if(c->isalloc){
      free(c->par);
      c->isalloc=0;
   }/*if(c->isalloc)*/
   c->par=c->cmd;
   c->maxparind=2;
}/*l_clearCmd*/

void parsePeerCmd(struct cmdFromPeer_struct *c,void *l)
{
int i;
   for(i=0; i<c->fromPeerLen; i++){
      if(c->datac>0){/*Copy to outputQueue*/
         long int rd=c->fromPeerLen-i;
            if(rd>c->datac)rd=c->datac;
            add_q1_queue(rd,c->fromPeer+i,c->outputQueue);
            c->datac-=rd;
            i+=rd;
      }/*if(c->datac>0)*/
      if(   (i<c->fromPeerLen)&&(c->fromPeer[i]>' ')  ){
         /*Copy from rawbuffer to proper one:*/
         c->par[c->parind++]=c->fromPeer[i];
         /*Now c->parind is incremented!*/
         if(    (c->ind)++ == 1 ){/*Time to convert a command*/
            if( (c->icmd=hex2int(c->cmd,2))<0  ){/*Not a HEX number!*/
              l_clearCmd(c);
              continue;
           }/*if( (c->icmd=hex2int(c->cmd,2))<0  )*/
         }/*if(    (c->ind)++ == 1 )*/
         /*Now c->ind is incremented!*/

         if(c->parind == c->maxparind){/*Time to invoke a handler*/
            /*Note, c->parind is incremented here!*/
            if(   (c->maxparind=(c->handler)(c,l))==0  ){
               /*Command is performed!*/
               l_clearCmd(c);
               continue;
            }else{/*if(   (c->maxparind=(c->handler)(c,l))==0  )*/
               /*Just allocate a new buffer*/
               /*Free, if used:*/
               if(c->isalloc){
                  free(c->par);
                  c->isalloc=0;
               }/*if(c->isalloc)*/
               /*And allocate a new one:*/
               if(c->maxparind>CMD_LEN){/*too long -- allocate in heap:*/
                  c->par=get_mem(c->maxparind,sizeof(char));
                  c->isalloc=1;
               }else/*CMD_LEN is enough*/
                  c->par=c->cmd;
               c->parind=0;/*reset the index*/
            }/*if(   (c->maxparind=(c->handler)(c,l))==0  ) ... else*/
         }/*if(c->parind == c->maxparind)*/
      }/*if(   (i<c->fromPeerLen)&&(c->fromPeer[i]>' ')  )*/
   }/*for(i=0; i<fromPeerLen; i++)*/
}/*parsePeerCmd*/

int mkchat(    int socksend,/* remote write descriptor*/
               int sockrec,/* remote read descriptor*/
               char *cmd, /*cmd to run*/
               char **args,/* arguments, ATTENTION! NULL terminated! */
               long int timeout,/*millisecs*/
               int startup/*bit flag of startup mode, see a few lines below*/
                )
{

   struct localMkchat_struct l;

   l.fdsend=-1;l.fdreceive=-1;
   l.selret=0;
   l.ttymode=0;
   l.peer_listen=1&startup;/*!0 - listen peer, 0 - not*/
   l.swallowed_listen=2&startup; /*!0 - listen swallowed, 0 - not*/
   l.stdin_peer=4&startup; /*!0- pass stdin to peer, 0 - not*/
   l.stdin_swallow=8&startup; /*!0- pass stdin to swallowed, 0 - not*/
   l.stdout_swallow=16&startup;/*!0- pass swallowed to stdout,  0 - not*/
   l.peer_swallow=32&startup;/*!0- pass swallowed to peer, 0 - not*/
   /*64&startup - swallow the child stdin and stdout, or not:*/
   if(64&startup)
      l.ttymode|=3;/*reopen stdin, stdout*/
   /*128&startup - setsid():*/
   if(128&startup)
      l.ttymode|=8;

   l.repeatmainloop=1;
   l.bufAlloc=READ_BUF_LEN-1;
   l.buf=get_mem(READ_BUF_LEN,sizeof(char));
   l.buf[l.bufAlloc]='\0';
   l.bufLen=0;
   l.ret=0;
   l.maxd=0;
   l.timeouthits=0;
   l.socksend=socksend;
   l.sockrec=sockrec;
   l.cmdFromPeer.fromPeerAlloc=READ_BUF_LEN-1;
   l.cmdFromPeer.fromPeer=get_mem(READ_BUF_LEN,sizeof(char));
   l.cmdFromPeer.fromPeer[l.cmdFromPeer.fromPeerAlloc]='\0';
   l.cmdFromPeer.fromPeerLen=0;
   l.cmdFromPeer.cmd[2]='\0';
   l.cmdFromPeer.icmd=-1;
   l.cmdFromPeer.isalloc=0;
   l.cmdFromPeer.ind=0;
   l.cmdFromPeer.par=l.cmdFromPeer.cmd;
   l.cmdFromPeer.parind=0;
   l.cmdFromPeer.maxparind=2;
   l.cmdFromPeer.datac=0;
   l.cmdFromPeer.outputQueue=NULL;
   l.cmdFromPeer.handler= &mkchat_h;

   /* Temporary ignore this signal:*/
   /* if compiler fails here, try to change the definition of
      mysighandler_t on the beginning of the file types.h*/
   l.oldPIPE=signal(SIGPIPE,SIG_IGN);
   /* Swallow a program  "cmd":*/
   if(64&startup)/*swallow the child stdin and stdout*/
      l.childpid= run_cmd(&l.fdsend,&l.fdreceive,l.ttymode,cmd,args);
      /*11 -  reopen stdin,stdout,setsid*/
   else/*Leave the child stdin and stdout untouched:*/
      l.childpid= run_cmd(NULL,NULL,l.ttymode,cmd,args);
   if ( l.childpid>0){
      /*Success. Command is running.*/
      writexactly(socksend,c2pWRK,2);/*confirmation*/

      writeLong(socksend,l.childpid);/*Send pid to a caller*/

      /* highest-numbered descriptor:*/
      l.maxd=socksend;
      if(l.maxd<sockrec)l.maxd=sockrec;
      if(l.maxd<l.fdsend)l.maxd=l.fdsend;
      if(l.maxd<l.fdreceive)l.maxd=l.fdreceive;
      /*Note, we assume stdin and stdout are smaller then other descriptors!*/
      l.maxd++;

      gettimeofday(&l.sv, NULL);/*Store start time*/
      init_q1_queue(&l.peer_queue);
      init_q1_queue(&l.send_queue);
      init_q1_queue(&l.stdout_queue);
      init_q1_queue(&l.stderr_queue);
      /*Start polling:*/
      do{

         /*Clear descriptors:*/
         FD_ZERO(&l.rfds);
         FD_ZERO(&l.wfds);

         /* First, we will listen sockrec and fdreceive:*/
         if(l.peer_listen)
            FD_SET(sockrec,  &l.rfds);
         if(l.swallowed_listen && (l.peer_queue.totlen<MAX_PEER_Q_LEN) &&(l.fdreceive>-1))
            FD_SET(l.fdreceive,&l.rfds);
         /*Optionally, stdin may be interesting:*/
         if(l.stdin_peer || l.stdin_swallow)
            FD_SET(0, &l.rfds);

         if( is_queue(&l.peer_queue) )/*Need to send something to peer:*/
            FD_SET(socksend, &l.wfds);

         if( is_queue(&l.send_queue) && (l.fdsend>-1) )/*Need to send something to swallowed routine:*/
            FD_SET(l.fdsend, &l.wfds);

         if( is_queue(&l.stdout_queue) )/*Need to send something to stdout:*/
            FD_SET(1, &l.wfds);

         if( is_queue(&l.stderr_queue) )/*Need to send something to stderr:*/
            FD_SET(2, &l.wfds);

         /*Wait for nonblocking:*/
         l.selret=select(l.maxd, &l.rfds, &l.wfds, NULL,
                          adjust_timeout(&l.sv,&l.tv,timeout) );

         if(l.selret>0){/*Something interesting*/

            if(  FD_ISSET(sockrec, &l.rfds) ){
               /*Message from peer*/
               if(
                   (l.cmdFromPeer.fromPeerLen=
                     read2b(sockrec,l.cmdFromPeer.fromPeer,l.cmdFromPeer.fromPeerAlloc)
                   )<1/*Is peer died?*/
                  ){
                  l.ret=1;/*Die!*/
                  l.repeatmainloop=0;
               }else
                  parsePeerCmd(&l.cmdFromPeer,&l);

               if (l.repeatmainloop==0)/*A halt command from peer*/
                  break;

            }/*if(  FD_ISSET(sockrec, &l.rfds) )*/

            if(  (l.fdreceive>-1)&&(FD_ISSET(l.fdreceive, &l.rfds)) ){
               /*Message from swallowed command*/
               if(
                  (l.bufLen=read2b(l.fdreceive,l.buf,l.bufAlloc))<1
                 ){
                    /*It closed stdout:*/
                   /*See rproto.h*/add_q1_queue(2,c2pSTDOUTCLOSED,&l.peer_queue);
                   l_isalive(0,&l);
                   l.swallowed_listen=0;
               }else{
                  if(l.stdout_swallow)/*copy to stdout*/
                     add_q1_queue( l.bufLen,l.buf,&l.stdout_queue);
                  if(l.peer_swallow){/*copy to peer queue*/
                     char buf[5];
                     /*"XXxxxx" means read data of xxxx length from swallowed,
                       xxxx is a hex length:*/
                     /*See rproto.h*/add_q1_queue(2,c2pDATASW,&l.peer_queue);
                     add_q1_queue(4,int2hex(buf,l.bufLen,4),&l.peer_queue);
                     /*And, send data itself:*/
                     add_q1_queue( l.bufLen,l.buf,&l.peer_queue);
                  }/*if(l.peer_swallow)*/
               }
            }/*if(  (l.fdreceive>-1)&&(FD_ISSET(l.fdreceive, &l.rfds)) )*/

            if(  FD_ISSET(socksend, &l.wfds) ){
               /*Peer is ready to read*/
               if( is_queue(&l.peer_queue) ){
                  char *buf;
                  ssize_t len;
                  len=get_q1_queue(&buf, &l.peer_queue);
                  if(len>g_socket_buf_len)
                     len=g_socket_buf_len;
                  len=writeFromb(socksend, buf, len);
                  if(len<0){/*Peer is died (?)*/
                     kill(l.childpid,SIGHUP); /* warn the child process */
                     kill(l.childpid,SIGKILL);/* kill the child process*/
                     l.ret=1;/*Die!*/
                     l.repeatmainloop=0;
                     clear_q1_queue(&l.peer_queue);
                     break;
                  }else/*if(len<0)*/
                     accept_q1_queue(len,&l.peer_queue);
               }/*if( is_queue(&peer_queue) )*/
            }/*if(  FD_ISSET(socksend, &l.wfds) )*/

            if(  (l.fdsend>-1)&&(FD_ISSET(l.fdsend, &l.wfds)) ){
               /*Swallowed command ready to read*/
               /*Not so easy: swallowed command may accept only one character!
                 But we trust the buffer length is >= atomic  PIPE_BUF>=512:*/
               if( is_queue(&l.send_queue) ){
                  char *buf;
                  ssize_t len;
                  len=get_q1_queue(&buf, &l.send_queue);
                  if(len>g_pipebuf_len)
                     len=g_pipebuf_len;
                  len=writeFromb(l.fdsend, buf, len);
                  if(len<0){/*Hm... Seems it is died!*/
                     /*It closes stdin*/
                     /*See rproto.h*/add_q1_queue(2,c2pSTDINCLOSED,&l.peer_queue);
                     l_isalive(0,&l);
                     clear_q1_queue(&l.send_queue);
                  }else
                     accept_q1_queue(len, &l.send_queue);
               }/*if( is_queue(l.send_queue) )*/
            }/*if(  (l.fdsend>-1)&&(FD_ISSET(l.fdsend, &l.wfds)) )*/

            if(  FD_ISSET(0, &l.rfds) ){
              /*Something interesting at stdin*/
               if(
                  (l.bufLen=read2b(0,l.buf,l.bufAlloc))<1
                 )l.stdin_peer=l.stdin_swallow=0;
               else{
                  if(l.stdin_peer){/*copy to peer queue*/
                     char buf[5];
                     /*"XXxxxx" means read data of xxxx length from stdin,
                        xxxx is a hex length:*/
                     /*See rproto.h*/add_q1_queue(2,c2pDATASI,&l.peer_queue);
                     /*Send a length:*/
                     add_q1_queue(4,int2hex(buf,l.bufLen,4),&l.peer_queue);
                     /*Send data:*/
                     add_q1_queue(l.bufLen,l.buf,&l.peer_queue);
                  }/*if(l.peer_swallow)*/

                  if(l.stdin_swallow){/*copy to swallowed queue*/
                     add_q1_queue(l.bufLen,l.buf,&l.send_queue);
                  }/*if(l.peer_swallow)*/

               }/*if()...else*/
            }/*if(  FD_ISSET(0), &l.rfds) )*/
            if(  FD_ISSET(1, &l.wfds) ){
              /*stdout can be written*/

               if( is_queue(&l.stdout_queue) ){
                  char *buf;
                  ssize_t len;
                  len=get_q1_queue(&buf, &l.stdout_queue);
                  if(len>g_pipebuf_len)
                     len=g_pipebuf_len;
                  len=writeFromb(1, buf, len);
                  if(len<0){/*Hm... It's closed!*/
                     clear_q1_queue(&l.stdout_queue);
                  }else
                     accept_q1_queue(len, &l.stdout_queue);
               }/*if( is_queue(l.stdout_queue) )*/

            }/*if(  FD_ISSET(1, &l.wfds) )*/

            if(  FD_ISSET(2, &l.wfds) ){
              /*stderr can be written*/

               if( is_queue(&l.stderr_queue) ){
                  char *buf;
                  ssize_t len;
                  len=get_q1_queue(&buf, &l.stderr_queue);
                  if(len>g_pipebuf_len)
                     len=g_pipebuf_len;
                  len=writeFromb(1, buf, len);
                  if(len<0){/*Hm... It's closed!*/
                     clear_q1_queue(&l.stderr_queue);
                  }else
                     accept_q1_queue(len, &l.stderr_queue);
               }/*if( is_queue(l.stderr_queue) )*/

            }/*if(  FD_ISSET(1, &l.wfds) )*/

         }else if(l.selret==0){/*Timeout*/
            /*Timeout;)*/
            if(l.timeouthits++ == 0){
               /*See rproto.h*/add_q1_queue(2,c2pTIMEOUT,&l.peer_queue);
            }/*if(l.timeouthits++ == 0)*/
            /*If peer is able to read, quit the function only after the queue is expired*/
            if(   !is_queue(&l.peer_queue)||(  !(FD_ISSET(socksend, &l.wfds)) )   ){
               /*That's all!*/
               l.repeatmainloop=0;
               l.ret=0;
            }/*if(   !is_queue(&l.peer_queue)||(  !(FD_ISSET(socksend, &l.wfds)) )   )*/
         }else{/*Error from select*/
            if (errno != EINTR){
               /*a "select" error*/
               /*See rproto.h*/writexactly(socksend,c2pSELERR, 2);
               /*Note, can't use add_q1_queue(...&l.peer_queue) here since
                select() is broken - we must exit right now!*/
                l.repeatmainloop=0;
                l.ret=0;
            }/*if (errno != EINTR)*/
            /*else - select was interrupted by a signal, nothing special...*/
         }

      }while(l.repeatmainloop);

      if(l.fdsend>-1)
         close(l.fdsend);
      if(l.fdreceive>-1)
         close(l.fdreceive);

      clear_q1_queue(&l.stderr_queue);
      clear_q1_queue(&l.stdout_queue);
      clear_q1_queue(&l.send_queue);
      clear_q1_queue(&l.peer_queue);

   }else/*if ( l.childpid>0)*/
       writexactly(socksend,c2pNOK, 2);/*Something is wrong!*/

   free_mem(&l.cmdFromPeer.fromPeer);
   free_mem(&l.buf);
   writexactly(socksend,c2pNEXT, 2);
   /* And now, restore old signals:*/
   signal(SIGPIPE,l.oldPIPE);
   return(l.ret);
}/*mkchat*/

#define MAX_PENDING_CONNECTIONS 3

/*
    ATTENTION! The following function uses non-reentrant gethostbyname() and
    inet_ntoa() functions! Not thread-safe, not re-entrant!

    ATTENTION! Some linux libc6 + ldap will chrash on gethostbyname(), when the program is
    linked statically!

    This function tries to find the current host IP, if buf="", otherwise
    it determines the IP of the named host:
*/
char *defip(char *buf, size_t len)
{
    struct hostent *hp;
    size_t i;
    char *ptr;

       if( (*buf == '\0')&&
           ( gethostname(buf,len)!=0 )
         )  return NULL;
       /*Now in buf is the name!*/

       if( (hp=gethostbyname(buf))==NULL ) /*Crash under Linux? Set "ldap" after "dns"
                                  in "hosts" entry in /etc/nsswitch.conf */
          return NULL;
       len--;/*What about he last '\0'?*/
       ptr=inet_ntoa(*(struct in_addr *)(hp->h_addr));
       /*Just copy - to be independent on "string.h":*/
       for(i=0;i<len; i++)
          if(  (buf[i] = ptr[i])=='\0'  )break;
       buf[i]='\0';

       return buf;

}/*defip*/

/*
  Sends a signal converted from thesigname to pid group pid.
  thesigname is of 3 bytes length, the first character is either '0' or '1'.
  If '0', then thesigname+1 is a HEX representation of a signal, if '1', it is a HEX
  representation if Diana-specific signal, see the file int2sig.c, procedure int2sig.
  The function returns:
          0 - Success.
          1 - An invalid signal was specified.
          2 - The pid or process group does not exist.
          3 - The  process  does  not have permission to send the signal
          4 - Unknown error.
*/
static int l_makeAkill(char *thesigname,pid_t pid)
{
   int thesig=hex2int(thesigname+1,2);
      if(*thesigname == '1')/*Diana-specific*/
         thesig=int2sig(thesig);
      /*else - just a number 'as is'*/
      if(kill(-pid,thesig)!=0)switch(errno){
         case EINVAL:return 1;/*An invalid signal was specified.*/
         case ESRCH:return 2;/*The pid or process group does not exist.*/
         case EPERM:return 3;/*The  process  does  not have permission to send the signal*/
         default: return 4;/*Unknown error*/
      }/*if(kill(pid,thesig)!=0)switch(errno)*/
      return 0;
}/*l_makeAkill*/

void go_to_chat(int wsock,int rsock)
{
int finish=0;
   do{
      char buf[9], **args;
      int i,n,N,startup;
      long int timeout;
      /*
        The sturtup protocol (No spaces!):
        12345678 12345678    12    { 12   123...}
        startup   timeout    n       N_i  arg[i]
                                   ^^^^^^^^^^^^^n times
        If startup > 32766) - just stop!
        if timeout == 0 then timeout is infinite.

        if (16384 & startup) - a ping query. Return s2cOK and try again.
        if (8192 & startup) - send a signal. Perform a query and quit.

      */
         /*startup:*/
         if(  (startup=readHex(rsock,buf,8, s2cNOK,2)) < 0 ){
            exit(60);
         }
         if(startup > 32766) break;/*A stop command*/

         if(16384 & startup){/*A "ping" command*/
            writexactly(wsock,s2cOK,2);
            continue;
         }/*if(16384 & startup)*/

         if (8192 & startup){
            char pidbuf[17];
            int l;
            pid_t pid;
            /*send a signal. Perform a query and quit.*/
            if( readexactly(rsock, buf, 3)<0 )
               exit(60);
            /*now in buf we have a signal*/

            /*Read a length of a pid:*/
            if( (l=readHex(rsock,pidbuf,2,NULL,0))<0 )
               exit(60);

            /*Read pid:*/
            if( (pid=readHex(rsock,pidbuf,l,NULL,0))<0 )
               exit(60);

            /*Send a signal and return the result to the client:*/
            writexactly(wsock,int2hex(pidbuf,l_makeAkill(buf,pid),2),2);
            break;/*quit the query - end the process.*/
         }/*if (8192 & startup)*/

         /*timeout*/
         if(  (timeout=readHex(rsock,buf,8, s2cNOK,2)) < 0 ){
            exit(60);
         }
         if(timeout==0)timeout=-1;

         /*Number of args:*/
         if(  (n=readHex(rsock,buf,2, s2cNOK,2)) < 0 ){
            exit(60);
         }

         /*n == number of args*/
         /*Allocate args:*/
         args=get_mem(n+1,sizeof(char *));/*+1 for trailing NULL*/
         for(i=0; i<n;i++){
            if(  (N=readHex(rsock,buf,2, s2cNOK,2)) < 0 ){
               exit(60);
            }
            args[i]=get_mem(N+1,sizeof(char));/*+1 for trailing '\0'*/
            if( readexactly(rsock, args[i], N)<0 ){
               exit(60);
            }
         }/*for(i=0; i<n;i++)*/
         finish=mkchat(wsock,rsock,args[0],args+1,timeout,startup);

         for(i=0; i<n;i++)
            free_mem(args+i);
         free_mem(&args);
   }while(!finish);
}/*go_to_chat(int rsock,int wsock)*/

static int l_clrbl1(FILE *portFile, char *filetounlink, char *ptrtoclear, int ret)
{
   if(portFile!=NULL)
      fclose(portFile);
   if(filetounlink!=NULL)
      unlink(filetounlink);
   if(ptrtoclear!=NULL)
      free(ptrtoclear);
   return ret;
}/*l_clrbl1*/

int runserver(int n, int thenice)
{
   int new_socket;
   int rndint=0;
   char *fn=new_str(g_srv_dir);

   {/*block 1*/
      int create_socket,addrlen;
      pid_t childpid;
      struct sockaddr_in address;
      FILE *portFile=NULL;
      char *ip;
      {/*block 1.1*/
         char buf[512];
         int i;

         /*Here we trust ourself: SRVFILENAME must not be too long!*/
         s_letn(SRVFILENAME,buf, 512);

         for(i=0; buf[i]!='\0';i++);
         /*Here i must be of the order of 10*/

         /*buf[i]=='\0'!:*/
         if(    (ip=defip(buf+i, 512-i))==NULL )return l_clrbl1(NULL,NULL,fn,1);

         /*Now in "buf" we have SRVFILENAME<>(our IP address)*/
         for(i=0; fn[i]!='\0';i++);
         if( fn[i-1]!='/' )fn=s_inc(fn,"/");
         fn=s_inc(fn,buf);
         /*Now in "fn" we have full path to our port file*/
      }/*block 1.1*/
      do{
         { mode_t oldmode=umask( ~(S_IRUSR|S_IWUSR) );
            portFile=fopen(fn,"a");
            umask(oldmode);
         }
         if(   portFile == NULL  )return l_clrbl1(NULL,NULL,fn,2);

         if( ftell(portFile)  ){
            /*Already in progress?*/
            fclose(portFile);
            portFile=NULL;
            if( pingSrv(ip) ){
               message(SERVERALREADYISRUNNING,ip);
               return l_clrbl1(NULL,NULL,fn,3);
            }else{
               unlink(fn);
            }
         }/*if( ftell(portFile)  )*/
      }while(portFile==NULL);

      if ((create_socket = socket(PF_INET,SOCK_STREAM,0)) <= 0)
         return l_clrbl1(portFile,fn,fn,4);

      address.sin_family = AF_INET;
      address.sin_addr.s_addr = htonl(INADDR_ANY);
      address.sin_port = htons(g_port);
      if (bind(create_socket,(struct sockaddr *)&address,sizeof(address)) != 0)
         return l_clrbl1(portFile,fn,fn,5);

      {/*block 2*/
         struct sockaddr_in sa;
         int sa_len;

         /*We will use a random integer as a password:*/
         /*Initialize random seed by timer:*/
         {struct timeval t; gettimeofday(&t,NULL); srand((unsigned int)t.tv_usec);}
         rndint=rand();

         /* We must put the length in a variable:*/
         sa_len = sizeof(sa);

         /* Ask getsockname to fill in this socket's local address:*/
         if (getsockname(create_socket, (struct sockaddr *)&sa, (socklen_t *)&sa_len) == -1)
            return l_clrbl1(portFile,fn,fn,6);
         /*In the prev. line I cast &sa to (struct sockaddr *) to avoid warning*/

         fprintf(portFile,"%d\n", (int) ntohs(sa.sin_port));
         /*Now store the "password":*/
         fprintf(portFile,"%d\n", rndint);
         /* Store number of allowed handlers:*/
         fprintf(portFile,"%d\n", n);
         /* Store the nice:*/
         fprintf(portFile,"%d\n", thenice);
         /*That's all!*/
         fclose(portFile);portFile=NULL;

      }/*block 2*/
      if( listen(create_socket,MAX_PENDING_CONNECTIONS)!=0 )
         return l_clrbl1(portFile,fn,fn,7);
      message(STARTSERVER,NULL);
      do{
         childpid=1;/*About continue*/
         addrlen = sizeof(struct sockaddr_in);
         new_socket = accept(create_socket,
                       (struct sockaddr *)&address,(socklen_t *)&addrlen);
         if (new_socket > 0){
            {/*block3*/
               char buf[9];
                /*Identification:*/
                if ( rndint!=readHex(new_socket,buf,8, s2cNOK,2) ){
                   close(new_socket);
                   continue;/*Autorisation fails*/
                }
                writexactly(new_socket,s2cOK,2);

               /*See rproto.h: c2sDIE means "Die!", c2sJOB means "Start a new job!":*/
               switch(  readHex(new_socket,buf,2, s2cNOK,2)  ){
                  case c2sDIE_i:
                     close(new_socket);
                     close(create_socket);
                     unlink(fn);
                     l_clrbl1(portFile,fn,fn,5);
                     return 0;
                  case c2sJOB_i:
                     writexactly(new_socket,s2cOK,2);
                     /*No break! */
                  case c2sSENDSIG_i:/*expect 00002000 as a startup - send a signal*/
                     break;
                  default:
                     writexactly(new_socket,s2cNOK,2);
                     /*No break!*/
                  case c2sBYE_i:
                  case -1:/*Message to the client was sent by readHex*/
                     /*Just continue the loop*/
                     close(new_socket);
                     continue;
               }/*switch(  readHex(new_socket,buf,1, s2cNOK,2)  )*/
            }/*block3*/
            switch (childpid=fork()){
               case -1: return l_clrbl1(portFile,fn,fn,8);
               case 0:/*Child*/
                  /*
                    The following code is not so efficient, but extremely portable:
                    to avoid zombies and taking care on all forked children,
                    the parent process should fork, and then wait right there
                    for the child process to terminate.  The child process then
                    forks again, giving us a child and a grandchild.  The child exits
                    immediately (and hence the parent waiting for it notices its death
                    and continues to work), and the grandchild does whatever the child
                    was originally supposed to. Since its parent died, it is inherited
                    by init, which will do whatever waiting is needed.
                   */
                  switch (fork()){
                  case 0:/*grandchild, just continue*/
                     break;
                  case -1:/*Fail - exit to prevent infinitely blocking of the father*/
                     exit(10);
                  default:
                     /*The father of a "grandchild", just exit*/
                     exit(0);
                  }/*switch (fork())*/

                  /*grandchild*/
                  /*Nothing to listen:*/
                  close(create_socket);
                  break;
               default:/*The father*/
                  /*Detach child's socket:*/
                  close(new_socket);
                  /*Wait for the child ("the father of a grandchild") will be finished:*/
                  wait(&new_socket);

#ifdef SKIP
                  if( (WIFEXITED(new_socket))&&(WEXITSTATUS(new_socket)  ){
                     /*Fail!!*/
                     /*So, what can I do?*/
                  }/*if( (WIFEXITED(new_socket))&&(WEXITSTATUS(new_socket)  )*/
#endif
                  /*And continue to listen:*/
                  break;
            }/*switch (childpid=fork())*/
         }else{/*if (new_socket > 0)*/
            if(errno != EINTR){
               /*Error, halt !*/
#ifndef NOSTRINGS
               halt(strerror(errno),NULL);
#else
               halt(SOCKETERROR,errno);
#endif
            }/*else - interrupted by a signal, just continue.*/
         }/*if (new_socket > 0) ... else */
      }while(childpid);

   }/*block 1*/

   /*The child!*/
   go_to_chat(new_socket,new_socket);

   close(new_socket);
   return 0;
}/*runserver*/

/************************ CLIENT *****************************************/

/*The following function does exit from the CHILD, one per iteration!:*/
static void l_exitFromCheckHandlers(int fd, struct rhandler_struct *h)
{
   write(fd,"0\n",2);
   close(fd);
   if(h!=NULL){
      close(h->rsocket);
      if(h->rsocket!=h->wsocket)
         close(h->wsocket);
   }/*if(h!=NULL)*/
   exit(0);
}/*l_exitFromCheckHandlers*/

/*The following procedure reads a port file into h. The only information used in h is
IP.*/
int resetRawHandler(struct rhandler_struct *h,struct localClient_struct *l)
{
char *fname=new_str(g_remote_srv_dir);
char *buf=get_mem(16,sizeof(char));
FILE *f=NULL;
int tmp, ret=-1;

   fname=s_inc(fname,SRVFILENAME);
   fname=s_inc(fname, h->ip);
   if( (f=fopen(fname,"r"))==NULL)goto failResetHandler;

   /*Read a port number:*/
   if(fgets(buf,16,f)==NULL)goto failResetHandler;
   if(  sscanf(buf,"%hu",&(h->port) )!=1  )goto failResetHandler;

   /*Read the password:*/
   if(fgets(buf,16,f)==NULL)goto failResetHandler;
   if(  sscanf(buf,"%d",&tmp )!=1  )goto failResetHandler;
   if(tmp<0)goto failResetHandler;
   int2hex(h->passwd,tmp,8);

   /*Read the number of allowed handlers:*/
   if(fgets(buf,16,f)==NULL)goto failResetHandler;
   /*We will not reset here possible number of handlers, so just ignore this information.*/

   /*Read here the nice:*/
   if(fgets(buf,16,f)==NULL)goto failResetHandler;
   if(  sscanf(buf,"%d",&(h->thenice) )!=1  )goto failResetHandler;

   ret=0;
   failResetHandler:
      if(f!=NULL)fclose(f);
      free(buf);
      free(fname);
      return ret;
}/*resetRawHandler*/

static int l_theval(struct queue_base_struct *theJobCell)
{
   return ((struct job_struct*)(theJobCell->msg))->id;
}/*l_theval*/

void shutdownAllHandlers(struct localClient_struct *l)
{
struct queue_base_struct *c=l->freeHandlers.next;

   while( c != &l->freeHandlers ){
      struct rhandler_struct *h=(struct rhandler_struct *)(c->msg);
         if( (h->wsocket)>0 ){
            /*HERE the only possibility to send something to the server!*/
            close(h->wsocket);
            if(  ( (h->rsocket)>0 )&&(h->rsocket!=h->wsocket)  )
               close(h->rsocket);
            (h->rsocket)=(h->wsocket)=-1;

         }/*if( (h->wsocket)>0 )*/

         {/*block*/
            struct queue_base_struct *tmp=c->next;/*go to the next handler*/
            /*Move the cell from "free" handlers to "raw":*/
            move_base_queue(c,&l->rawHandlers);
            c=tmp;
         }/*block*/
         l->rawHandlersN++;
         l->freeHandlersN--;
   }/*while( c != &l->freeHandlers )*/
}/*shutdownAllHandlers*/

/*The following procedure places the handler newH to the freeHandlers queue according to
  the value of 'thenice'. The more thenice, the less priority. The handler with highest
  priority will be placed to freeH->prev:*/
void l_add_to_freeHandlers(struct queue_base_struct *newH,struct queue_base_struct *freeH)
{
int n=((struct rhandler_struct *)(newH->msg))->thenice;

   /*Remove cell from working/raw handlers:*/
   newH->prev->next=newH->next;
   newH->next->prev=newH->prev;

   if(/*Iqueuq not empty?:*/
      (freeH->next!=freeH) &&
      /*Is newH the worst?*/
      ( ((struct rhandler_struct *)(freeH->next->msg))->thenice > n )
     )
      /*No, freeH->next is worse. So go to the opposite side...*/
      do
         freeH=freeH->prev;
      while( ((struct rhandler_struct *)(freeH->msg))->thenice < n );
      /*...until the worse (or equal) cell is not found.*/
      /*Note, this cycle will be finished sooner or later - at least reaching initial
        freeH->next*/
   /*Now newH must be placed to freeH->next. Add cell to 'freeH':*/
   newH->next=freeH->next;
   newH->next->prev=newH;
   freeH->next=newH;
   newH->prev=freeH;
}/*l_add_to_freeHandlers*/

void cleanJobsQueue(struct queue_base_struct *theroot,unsigned long int *n)
{
struct queue_base_struct *c=theroot->next;

   while( c != theroot ){
      struct job_struct *theJob=(struct job_struct *)(c->msg);
         free_mem(&theJob->name);
         free_mem(&theJob->args);
         c=c->next;
   }/*while( c != theroot )*/
   clear_base_queue(theroot);
   *n=0;
}/*cleanJobsQueue*/

static void l_fillinTheFields(int themask,
                              char *buf,
                              struct job_struct *theJob,
                              struct localClient_struct *l)
{
   l_jobinfo->mask = themask;
   l_jobinfo->offset=(theJob->id-1)*(l_jobinfo->ltot);
   l_fillup_jobinfo(buf,theJob,l);
   if( l_update_jobinfo(l_jobinfo) )
      clear_jobinfo();
}/*l_fillinTheFields*/

void startNewJob(
                  struct queue_base_struct *job,
                  struct queue_base_struct *hnd,
                  struct localClient_struct *l
               )
{
   struct rhandler_struct *h=(struct rhandler_struct *)(hnd->msg);
   unlink_cell(hnd);
   link_cell(hnd,&l->workingHandlers);
   (l->workingHandlersN)++;
   (l->freeHandlersN)--;

   unlink_cell(job);

   if(((struct job_struct *)(job->msg))->stickJob!=NULL){/*Sticky*/
      (l->jobsSPn)--;
   }else/*normal*/
      (l->jobsPn)--;

   h->theJob=job;

   ((struct job_struct *)(job->msg))->placement = RUNNING_JOBS_Q;
   ((struct job_struct *)(job->msg))->handlerid = h->id;

   /*Store starting time:*/
   gettimeofday(&(((struct job_struct *)(job->msg))->st),NULL);

   h->status=1;

   if( g_jobinfovar_d>-1){
      char buf[20];/*l_fillinTheFields uses it to convert IP*/
      l_fillinTheFields(1|4,buf,((struct job_struct *)(job->msg)),l);
   }/*if( g_jobinfovar_d>-1)*/

   gettimeofday(&(h->st), NULL);/*Store starting time*/

   {/*block*/
      char *buf=new_str("0000008100000000");/*startup and timeout for mkchat:*/
      /*00000081 == 1|128 (listen to peer and setsid)
        00000000 == infinite timeout*/
      buf=s_inc(buf,((struct job_struct *)(h->theJob->msg))->args);
      buf=s_inc(buf,p2cRUNANDNEXT);
      add_q1_queue(s_len(buf),buf,&h->thequeue);
      free(buf);
   }/*block*/

}/*startNewJob*/

static void l_add_handlers(
                              struct queue_base_struct *thecell,
                              struct queue_base_struct *thequeue,
                              struct localClient_struct *l
                          )
{
struct rhandler_struct *h=(struct rhandler_struct *)(thecell->msg);
int i,n=h->n;
   link_cell(thecell,thequeue);
   for(i=1; i<n;i++){
      struct rhandler_struct *hh=get_mem(1,sizeof(struct rhandler_struct));
      struct queue_base_struct *c;
         hh->port=h->port;
         s_let(h->passwd,hh->passwd);
         hh->n=h->n;
         hh->thenice=h->thenice;
         s_let(h->ip,hh->ip);
         hh->wsocket=hh->rsocket=-1;
         hh->id=++(l->topHndId);
         init_q1_queue(&(hh->thequeue));
         hh->theJob=NULL;
         c=get_mem(1,sizeof(struct queue_base_struct));
         c->msg=hh;
         c->len=sizeof(struct rhandler_struct);
         link_cell(c,thequeue);
   }/*for(i=1; i<n;i++)*/
}/*l_add_handlers*/

void initHandlersTable(struct localClient_struct *l)
{
struct queue_base_struct *c;

   l->HandlersTable=get_mem(l->topHndId+1,sizeof(struct queue_base_struct*));
   /*ATT! l->HandlersTable starts from 1, not 0!*/
   for(c=l->rawHandlers.next;c != &(l->rawHandlers);c=c->next){
      l->HandlersTable[((struct rhandler_struct *)(c->msg))->id]=c;
   }
}/*initHandlersTable*/

void l_killAllServers(struct localClient_struct *l)
{
struct queue_base_struct *c;
   for(c=l->rawHandlers.next;c != &(l->rawHandlers);c=c->next)
      kill_server(
         ((struct rhandler_struct *)(c->msg))->ip,
         ((struct rhandler_struct *)(c->msg))->port,
         ((struct rhandler_struct *)(c->msg))->passwd
      );
}/*l_killAllServers*/

int runlocalservers(void)
{
int i;
   g_rlocalsock=get_mem(g_numberofprocessors,sizeof(int));
   g_wlocalsock=get_mem(g_numberofprocessors,sizeof(int));
   for(i=0;i<g_numberofprocessors;i++){
      if(
         run_cmd(
            g_wlocalsock+i,
            g_rlocalsock+i,
            4,
            (char*)&go_to_chat,
            NULL)<1
        ){/*Fail!*/
           i--;
           g_numberofprocessors--;
      }
   }/*for(i=0;i<g_numberofprocessors;i++)*/
   if(g_numberofprocessors <1){/*Fail initialyzing servers*/
      free_mem(&g_rlocalsock);
      free_mem(&g_wlocalsock);
   }/*if(g_numberofprocessors <1)*/
   return g_numberofprocessors;
}/*runlocalservers*/

int initHandlers(struct localClient_struct *l)
{
   DIR *dirp;
   int i,n=0;
   struct dirent *d;
   struct rhandler_struct *h=NULL;
   struct queue_base_struct *c=NULL;

   /*Init local servers:*/
      if(g_numberofprocessors>0){
         if(g_rlocalsock==NULL)
            runlocalservers();
      }/*if(g_numberofprocessors>0)*/

      /*g_numberofprocessors may change!:*/
      if(g_numberofprocessors>0){
         h=get_mem(1,sizeof(struct rhandler_struct));
         *(h->ip)='\0';
         h->port=0;
         *(h->passwd)='\0';
         h->n=g_numberofprocessors;
         h->thenice=g_niceoflocalserver;
         h->wsocket=h->rsocket=-1;
         init_q1_queue(&h->thequeue);
         h->theJob=NULL;
         h->id=++(l->topHndId);
         c=get_mem(1,sizeof(struct queue_base_struct));
         c->msg=h;
         c->len=sizeof(struct rhandler_struct);
         l_add_handlers(c,&l->rawHandlers,l);
         (l->rawHandlersN)+=h->n;
         n+=h->n;

         for(i=0; i<g_numberofprocessors; i++){
            h->wsocket=g_wlocalsock[i];
            h->rsocket=g_rlocalsock[i];
         }/*for(i=0; i<g_numberofprocessors; i++)*/
      }/*if(g_numberofprocessors>0)*/

   /*Init remote servers:*/
      dirp = opendir(g_remote_srv_dir);
      if (dirp == NULL)return -1;

      for (d = readdir(dirp); d != NULL; d = readdir(dirp))
                      if(  ( (i=s_bcmp(SRVFILENAME,d->d_name))>0  )
                         &&(  isIp(d->d_name+i)  )
                        )
      {
         FILE *f=fopen(d->d_name,"r");
         int tmp;

            if (f==NULL) continue;
            h=get_mem(1,sizeof(struct rhandler_struct));
            /*Read the port number:*/
            if(fgets(h->ip,16,f)==NULL)goto initHandlersFails;
            if(  sscanf(h->ip,"%hu",&(h->port) )!=1  )goto initHandlersFails;

            /*Read the password:*/
            if(fgets(h->ip,16,f)==NULL)goto initHandlersFails;
            if(  sscanf(h->ip,"%d",&tmp )!=1  )goto initHandlersFails;
            if(tmp<0)goto initHandlersFails;
            int2hex(h->passwd,tmp,8);

            /*Read the number of allowed handlers:*/
            if(fgets(h->ip,16,f)==NULL)goto initHandlersFails;
            if(  sscanf(h->ip,"%d",&(h->n) )!=1  )goto initHandlersFails;
            if(h->n < 1) h->n=1;

            /*Read the nice:*/
            if(fgets(h->ip,16,f)==NULL)goto initHandlersFails;
            if(  sscanf(h->ip,"%d",&(h->thenice) )!=1  )goto initHandlersFails;

            /*Copy ip into h->ip:*/
            s_letn(d->d_name+i,h->ip,16);

            h->wsocket=h->rsocket=-1;

            init_q1_queue(&h->thequeue);
            h->theJob=NULL;
            h->id=++(l->topHndId);
            c=get_mem(1,sizeof(struct queue_base_struct));
            c->msg=h;
            c->len=sizeof(struct rhandler_struct);
            l_add_handlers(c,&l->rawHandlers,l);

            (l->rawHandlersN)+=h->n;
            n+=h->n;

            c=NULL;
            h=NULL;

            initHandlersFails:
                free_mem(&h);
                free_mem(&c);
                if(f!=NULL)
                   fclose(f);
      }/*for (d = readdir(dirp); d != NULL; d = readdir(dirp)) if()*/
      closedir(dirp);
      return n;
}/*initHandlers*/

/*Moves the job to the proper place: finiches it, or restarts it.
*/
struct queue_base_struct *jobFails(
                              struct queue_base_struct *c,
                              struct localClient_struct *l,
                              int *maxhitscounter,
                              int maxHits,
                              int handlerFails
                                       )
{
int isperformed=1,
    isremoved,/*if status[2]='1'*/
    jobinfomask=1|2;/*Placement and status - common filed*/
struct rhandler_struct *h;
struct queue_base_struct *theJobCell;
struct job_struct *theJob;

   if(c==NULL)return NULL;

   if(handlerFails<0){/*c is NOT a handler, it is a job cell!*/
      h=NULL;
      theJobCell=c;
      isperformed=0;
   }else{
      h=(struct rhandler_struct *)(c->msg);
      theJobCell=h->theJob;
   }
   theJob=(struct job_struct*)theJobCell->msg;

   isremoved=( (theJob->retcode)[2] == '1');

   if( (maxhitscounter==NULL)&&(!isremoved) ){/*Normal finishing*/
      put2q(theJobCell,&l->finishedJobsQ,&l_theval);
      (l->finishedJobsQn)++;
      theJob->placement=FINISHED_JOBS_Q;
   }else{

      if(maxhitscounter == &(theJob->hitTimeout))
         jobinfomask|=32;
      else if(maxhitscounter == &(theJob->hitFail))
         jobinfomask|=64;
      /*Else - nothing with hits!*/

      if (  ( ++(*maxhitscounter) > maxHits ) || isremoved  ){
         /*To many fails, or the job was removed*/
         put2q(theJobCell,&l->finishedJobsQ,&l_theval);
         /*Note, at this point l_theval(l->finishedJobsQ.prev) corresponds to
            the highest index of ordered jobs*/
         theJob->placement=FAILED_JOBS_Q;
         (l->failedJobsQn)++;/*It was failed!*/
         (l->finishedJobsQn)++;/*But, the queue counter must be incremented, too*/
      }else{
         /*Ok, move it to the main jobs queue back:*/
         if(theJob->stickJob==NULL){/*Non-sticky pipeline*/
            reverse_link_cell(theJobCell,&l->jobsP);
            theJob->placement=PIPELINE_Q;
            (l->jobsPn)++;
         }else{/*sticky pipeline*/
            reverse_link_cell(theJobCell,&l->jobsSP);
            theJob->placement=STICKY_PIPELINE_Q;
            (l->jobsSPn)++;
         }/*if(theJob->stickJob==NULL)*/
         isperformed=0;
      }/*if (  ++(*maxhitscounter) > maxHits ) ... else*/
   }/*if(maxhitscounter==NULL) ... else*/

   if( g_jobinfovar_d>-1)
      l_fillinTheFields(jobinfomask,NULL,theJob,l);
   if(h!=NULL){
      clear_q1_queue(&h->thequeue);
      h->theJob=NULL;
      (l->workingHandlersN)--;
      /*Re-use theJobCell - we need not it anymore:*/
      theJobCell=c->next;
   }else
      /*Re-use theJobCell - we need not it anymore:*/
      theJobCell=NULL;

   if(handlerFails>0){/*Move the cell from "working" to "raw" handlers:*/
         close(h->wsocket);
         if(h->rsocket!=h->wsocket)
            close(h->rsocket);
         h->rsocket=h->wsocket=-1;
         h->status=-2;/*Mark it as "raw"*/
         move_base_queue(c,&l->rawHandlers);
         /*Some statistics:*/
         (l->rawHandlersN)++;
   }else if(handlerFails == 0){
         l_add_to_freeHandlers(c,&l->freeHandlers);
         /*Some statistics:*/
         (l->freeHandlersN)++;
         h->status=-1;/*Mark it as "free"*/
   }/* else : handlerFails < 0 => do nothing, this is NOT a handler!*/

   if(isperformed){
      gettimeofday(&(theJob->ft), NULL);/*Store finishing time*/

      /*For future developng, not used yet:*/
#ifdef SKIP
      if(l->reqs & REPORT_JOB_PERFORMED){
         int ln=s_len(theJob->name);
         char buf[3];
         /*See rproto.h*/
         add_q1_queue(2,c2dJOBOK,&(l->toProcessor));
         add_q1_queue(2,int2hex(buf,ln,2),&l->toProcessor);
         add_q1_queue(ln,theJob->name,&l->toProcessor);
      }/*if(l.reqs & REPORT_JOB_PERFORMED)*/
#endif

      if(  (l->reqs & REPORT_ALL_JOB_PERFORMED)
         &&(l->workingHandlersN == 0)/*No running*/
         &&(
            (l->workingHandlersN/*No running*/
             +l->jobsQn/*No async. waititng*/
             +l->jobsSQn/*No sync. waititng*/
             +l->jobsPn/*No non-sticky pipelines*/
             +l->jobsSPn/*No sticky pipelines*/
            )==0
           )
        ){
           writexactly(l->w,c2dALLJOB,2);
           l->reqs =l->reqs & ~REPORT_ALL_JOB_PERFORMED ;
      }/*if(  (l.reqs & REPORT_ALL_JOB_PERFORMED)...)*/
   }/*if(isperformed)*/

   return theJobCell;

}/*jobFails*/

static void l_killJobPrg(struct job_struct *job,struct localClient_struct *l)
{
   struct rhandler_struct *h=(struct rhandler_struct*)
                                 (l->HandlersTable[job->handlerid]->msg);
      if(
         (job->pid<1)||
         (job->placement!=RUNNING_JOBS_Q)/*The jod is not runninig*/
        )
         return;

      if( *(h->ip) == '\0' )/*Local server*/
            kill(-(job->pid),SIGKILL);/*Send to the whole group!*/
      else{/*Remote server*/
         char thesig[5]="KILL";
         int2hex(l_pid,job->pid,hexwide(job->pid));
         s_let(reduce_signal(thesig),l_thesig);
         sendSig(h->ip);
      }
}/*l_killJobPrg*/

void  giveUpAll(struct localClient_struct *l)
{
struct queue_base_struct *c=l->workingHandlers.next;
int savedReqs=l->reqs;

   /*Clean up jobs queues:*/
   cleanJobsQueue( &(l->jobsQ),&(l->jobsQn) );
   cleanJobsQueue( &(l->jobsSQ),&(l->jobsSQn) );

   /*Clean up pipeline:*/
   cleanJobsQueue( &(l->jobsP),&(l->jobsPn) );
   cleanJobsQueue( &(l->jobsSP),&(l->jobsSPn) );
   l->reqs=0;

   while( c != &l->workingHandlers ){
      int tmp=1;
         l_killJobPrg(
            (struct job_struct*)(((struct rhandler_struct *)(c->msg))->theJob)->msg,
            l
         );
         c=jobFails(c,l,&tmp,0,0);/*1>0 so all jobs will be sent to failedJobs,
                                    last 0 means that we move handlers to free*/
   }/*while( c != &l->workingHandlers )*/
   l->reqs=savedReqs;
}/*giveUpAll*/

char *l_readArgs(int r, int n,char *buf)
{
   char *args=get_mem(255,sizeof(char));
   int N,i,l, top=255;
      int2hex(args,n,2);
      for(l=2,i=0; i<n;i++){
         if((l+2)>=top)
            args=realloc(args,top+=255);
         if(  (N=readHex(r,args+l,2, NULL,0)) < 0 )break;
         l+=2;
         if(N>255){N=-1;break;}
         if((l+N)>=top)
            args=realloc(args,top+=255);
         if( (readexactly(r,args+l , N)<0) ){N=-1;break;}
         l+=N;
      }/*for(l=2,i=0; i<n;i++)*/
      if(N<0){
         free(args); return NULL;
      }
      args=realloc(args,l+1);
      args[l]='\0';
      return args;
}/*l_readArgs*/

static void l_putTheRoot(struct queue_base_struct *theJobCell)
{
 struct job_struct tmp;
    tmp.name=NULL;tmp.args=NULL;
    tmp.stickJob=NULL;tmp.retcode[0]='\0';
    tmp.id=0;
   add_base_queue(sizeof(struct job_struct), &tmp, theJobCell);
}/*l_putTheRoot*/

static int l_job2pipe(struct queue_base_struct *jobCell,struct localClient_struct *l)
{
struct job_struct *job=(struct job_struct*)(jobCell->msg);

   unlink_cell(jobCell);
   if( job->type & STICKY_JOB_T ){/*Sticky - put it into jobsSP:*/
      link_cell(jobCell,&l->jobsSP);
      l->jobsSPn++;
      job->placement=STICKY_PIPELINE_Q;
   }else{/*Non-sticky - put it into jobsPn:*/
      link_cell(jobCell,&l->jobsP);
      l->jobsPn++;
      job->placement=PIPELINE_Q;
   }/*if(job->type & STICKY_JOB_T )...else*/
   if( g_jobinfovar_d>-1)
      l_fillinTheFields(1,NULL,job,l);
   return 0;

}/*l_job2pipe*/

static void l_report(int cmd,struct job_struct *job,struct localClient_struct *l)
{
char ip[20];
char thesig[4];
   switch(cmd){
      case d2cSENDSIG_i:/*Answers: "10" - 'job is not run' by a the client; "20" - can't read
                          a signal from Diana; <10 - see comments to l_makeAkill above*/
         /*One should read 3 bytes - the name of a signal: */
         if( readexactly(l->r,thesig,3)  ){
            writexactly(l->w,"20",2);/*Can't read from diana*/
            break;
         }/*if( readexactly(l->r,thesig,3)  )*/
         if( (job==NULL)||(job->handlerid < 1)||(job->pid < 1) )
            writexactly(l->w,"10",2);
         else{
            struct rhandler_struct *h=(struct rhandler_struct*)
               (l->HandlersTable[job->handlerid]->msg);
                 if(job->placement!=RUNNING_JOBS_Q){
                    /*The job is not running, can't send a signal!*/
                    writexactly(l->w,"10",2);
                    break;
                 }/*if(job->placement!=RUNNING_JOBS_Q)*/
                 if( *(h->ip) == '\0' ){/*Local server*/
                    /*Just make a kill() and send the answer to Diana:*/
                    writexactly(l->w,int2hex(thesig,l_makeAkill(thesig,job->pid),2),2);
                 }else{/*Remote server*/
                    /*The fllowing static buffers
                       static char l_thesig[4];
                       static char l_pid[17];
                      are used to pass parameters to l_q_sendAsignal (note, due to
                      fork() it will inherit this data despite of different process!): */
                      s_let(thesig,l_thesig);
                      int2hex(l_pid,job->pid,hexwide(job->pid));
                      /*Now sendSig -> queryTheServer ->sendAsignal -> l_q_sendAsignal
                        retuns the result of a kill():*/
                      writexactly(l->w,int2hex(thesig,sendSig(h->ip),2),2);
                 }/*if(ip2hex(h->ip, ip) == NULL) .. else*/
         }/*if( (job==NULL)||(job->handlerid < 1)||(job->pid < 1) )...else*/
         break;
      case d2cGETNAME2STICK_i:
         if( (job==NULL)||(job->stickJob==NULL) )
            writexactly(l->w,"00",2);/*Non-sticky job!*/
         else{
            int len=s_len(job->stickJob->name);
            writexactly(l->w,int2hex(ip,len,2),2);
            writexactly(l->w,job->stickJob->name,len);
         }/*if( (job==NULL)||(job->stickJob==NULL) ) ... else*/
         break;
      case d2cGETPID_i:
        if( (job==NULL)||(job->pid < 1) )
           writexactly(l->w,"010",3);
        else
           writeLong(l->w,job->pid);
        break;
      case d2cGETFAILQN_i:/*Send l->finishedJobsQn as 8 HEX digits*/
         writexactly(l->w,int2hex(ip,l->failedJobsQn,8),8);
         break;
      case d2cRMJOB_i:/*Read a name and return nothing:*/

         if(job!=NULL){
            int tmp=1;/*1>0 so all jobs will be sent to failedJobs,
                                    last 0 means that we move handlers to free*/
            unsigned long int *cnt=NULL;

               /*The job was removed:*/
               job->retcode[2]='1';

               switch(job->placement){
                  case FINISHED_JOBS_Q:
                  case FAILED_JOBS_Q:
                     /*Nothing to do*/
                     break;
                  case RUNNING_JOBS_Q:
                     /*Job is bound with a handler*/
                     /*Due to job->retcode[2]=='1', the job will be removed after
                       the server finishes it.
                     */
                     if( (job->pid)>0 )/*Try to kill it:*/
                        l_killJobPrg(job,l);
                     /*else - the job will be kille further, after the server starts it,
                       since job->retcode[2]=='1'*/
                     break;
                  case PIPELINE_Q:
                     cnt=&(l->jobsPn);
                     break;
                  case STICKY_PIPELINE_Q:
                     cnt=&(l->jobsSPn);
                     break;
                  case JOBS_Q:
                     cnt=&(l->jobsQn);
                     break;
                  case SYNCH_JOBS_Q:
                     cnt=&(l->jobsSQn);
                     break;
               }/*switch(job->placement)*/

               if(cnt==NULL)/*Nothing to do*/
                  break;
               /*Unlink from the current queue:*/
               unlink_cell(job->myQueueCell);
               (*cnt)--;
               /* and put it to failedQ:*/
               jobFails(job->myQueueCell,l,&tmp,0,-1);/*last -1 means that the
                  first argument is not a handler but the job cell */

         }/*if(job!=NULL)*/
         break;
      case d2cGETIP_i:/*Read a name and return IP*/
         if( (job==NULL)||(job->handlerid < 1) )writexactly(l->w,"00000000",8);
         else{
            struct rhandler_struct *h=(struct rhandler_struct*)
                  (l->HandlersTable[job->handlerid]->msg);
            if(ip2hex(h->ip, ip) == NULL)/*Local server*/
               writexactly(l->w,"00000000",8);
            else
               writexactly(l->w,ip,8);
         }/*if( (job==NULL)||(job->handlerid < 1) ) ... else */
         break;
      case d2cGETHITS_i:
         if(job==NULL)
            writexactly(l->w,"ffffff",6);
         else{
            writexactly(l->w,int2hex(ip,job->hitTimeout,2),2);
            writexactly(l->w,int2hex(ip,job->hitFail,2),2);
            writexactly(l->w,int2hex(ip,job->placement,2),2);
         }
         break;
      case d2cGETST_i:
         if(job==NULL)
            writexactly(l->w,"00030000",8);
         else
            writexactly(l->w,job->retcode,8);
         break;
      case d2cGETsTIME_i:
      case d2cGETfTIME_i:
      case d2cGETTIME_i:
         if(
              (job==NULL)
            ||( ((job->st).tv_sec<0)||((job->st).tv_usec<0) )
           )writexactly(l->w,"00000000",8);
         else{
            struct timeval *ft,tb;
            long t;
                  if ( ((job->ft).tv_sec<0)||((job->ft).tv_usec<0) ){
                     if(cmd==d2cGETfTIME_i){
                        writexactly(l->w,"00000000",8);
                        break;
                     }
                     gettimeofday(&tb,NULL);
                     ft=&tb;
                  }else
                     ft=&(job->ft);
                     switch(cmd){
                        case d2cGETsTIME_i:
                           t=
                           ( (job->st).tv_sec-(l->baseTime).tv_sec )*1000
                           +
                           ( (job->st).tv_usec-(l->baseTime).tv_usec )/1000;
                           break;
                        case d2cGETfTIME_i:
                           t=
                           ( ft->tv_sec-(l->baseTime).tv_sec )*1000
                           +
                           (ft->tv_usec-(l->baseTime).tv_usec)/1000;
                           break;
                        default:/*d2cGETTIME_i*/
                           t=(ft->tv_sec-(job->st).tv_sec)*1000
                              +(ft->tv_usec-(job->st).tv_usec)/1000;
                           break;
                     }/*switch(cmd)*/
                  writexactly(l->w,int2hex(ip,t,8),8);
         }/*if()... else*/
         break;
      default:/*unknown command, do nothing*/
         break;
   }/*swicth(cmd)*/
}/*l_report*/

static struct queue_base_struct *l_stickyJobFails(
                                                   struct queue_base_struct *c,
                                                   struct localClient_struct *l
                                                  )
{
   struct queue_base_struct *tmp=c->next;
   put2q(unlink_cell(c),&l->finishedJobsQ,&l_theval);

   (l->failedJobsQn)++;/*It was failed!*/
   (l->finishedJobsQn)++;/*But, the queue counter must be incremented, too*/
   /*Decrement the counter:*/
   if( (( (struct job_struct *)(c->msg) )->placement)==STICKY_PIPELINE_Q )
      (l->jobsSPn)--;/*The job was unlinked from the sicky pipeline*/
   else
      (l->jobsPn)--;/*The job was unlinked from the non-sicky pipeline*/

   ( (struct job_struct *)(c->msg) )->placement=FAILED_JOBS_Q;

   if( g_jobinfovar_d>-1)/*Place and retcode*/
      l_fillinTheFields(1|2,NULL,( (struct job_struct *)(c->msg) ),l);

   return tmp;
}/*l_stickyJobFails*/

/*The function tries to connect the handler with the server and on success moves the handler
  from "raw" to "free":*/
int checkHandlers(struct localClient_struct *l)
{
struct queue_base_struct *c;
int n=0;
   for(c=l->rawHandlers.next ;c != &l->rawHandlers; c=c->next){
      int fd[2];
      char buf[2];
      pid_t childpid;
      struct rhandler_struct *h=(struct rhandler_struct *)(c->msg);

      /*First, check is it a local server handler?*/

      if( *(h->ip) == '\0' ){/*Local!*/
         mysighandler_t oldPIPE;
         /* if compiler fails here, try to change the definition of
            mysighandler_t on the beginning of the file types.h*/
        oldPIPE=signal(SIGPIPE,SIG_IGN);

        /*Ping the server (h->ip is used as a buffer):*/
        if(
             (writexactly(h->wsocket,int2hex(h->ip,16384,8),8)!=0)
           ||(readHex(h->rsocket,buf,2,NULL,0)!= s2cOK_i)
          ){/*Fail! Try to restart:*/
             close(h->wsocket);
             close(h->rsocket);
             if(
                      run_cmd(
                         &(h->wsocket),
                         &(h->rsocket),
                         4,
                         (char*)&go_to_chat,
                         NULL
                      )<1
               )goto theserverFails;
             /*Ping again:*/
             if(
                  (writexactly(h->wsocket,int2hex(h->ip,16384,8),8)!=0)
                ||(readHex(h->rsocket,buf,2,NULL,0)!= s2cOK_i)
               )goto theserverFails;
        }/*if()*/
        {/*Block*/
            struct queue_base_struct *tmp=c->prev;
            /*Move the cell from "raw" handlers to "free":*/
            l_add_to_freeHandlers(c,&l->freeHandlers);
            c=tmp;
         }/*Block*/
         /*In principle, this is extra -- queue was initialized by
           initHandlers. But this is not dangerous:*/
         init_q1_queue(&(h->thequeue));

         h->status=-1;/*Mark it as a free*/
         l->rawHandlersN--;
         l->freeHandlersN++;
         n++;
         theserverFails:
            signal(SIGPIPE,oldPIPE);
            *(h->ip) = '\0';/*Was used as a buffer*/
      }else{
         /*First, create a socket, if needed: */
         if( (h->wsocket)<1 )/*Socket was NOT created!*/
            if (( h->wsocket= socket(AF_INET,SOCK_STREAM,0)) <1)
               continue; /*Fail!*/
         h->rsocket=h->wsocket;
         /* Open a pipe:*/
         if (pipe(fd)!= 0) return -1;
         /* Fork to create the child process:*/
         if((childpid = fork()) == -1)
            return -1;
         if(childpid){/*The father - waits CONNECT_TIMEOUT while the child will
                        establish a connection:*/
            struct timeval sv,tv;
            fd_set ds;
            int continuepolling=1;
               close(fd[1]);/*Close up writable pipe*/
               /*Now may read from fd[0]*/
               gettimeofday(&sv, NULL);/*Store starting time*/
               do{
                  FD_ZERO(&ds);
                  FD_SET(fd[0],&ds);
                  switch(select(fd[0]+1, &ds, NULL, NULL,
                                            adjust_timeout(&sv,&tv,CONNECT_TIMEOUT) ) ){
                     case 1:/*Child screams*/
                        {char buf[2];

                           if(
                                 (read(fd[0],buf,2)!=2)
                               ||(*buf!='1')
                             ){/*Child fails*/
                                continuepolling=0;
                                break;
                             }
                           /* Child reported success*/

                           {/*block*/
                              struct queue_base_struct *tmp=c->prev;
                              /*Move the cell from "raw" handlers to "free":*/
                              l_add_to_freeHandlers(c,&l->freeHandlers);
                              c=tmp;
                           }/*block*/

                           /*In principle, this is extra -- queue was initialized by
                             initHandlers. But this is not dangerous:*/
                           init_q1_queue(&(h->thequeue));

                           h->status=-1;/*Mark it as a free*/
                           l->rawHandlersN--;
                           l->freeHandlersN++;
                           n++;
                           continuepolling=0;
                           break;
                        }
                     default:
                        if (errno == EINTR)/*Interrupted by a signal, just continue:*/
                           continue;
                        /*else - timeout, or something is wrong. Give it up!*/
                        continuepolling=0;
                        break;
                  }/*switch*/
               }while(continuepolling);
               close(fd[0]);
               /*Kill the child!*/
               kill(childpid,SIGKILL);
               waitpid(childpid,&continuepolling,0);
         }else{/*The child*/
            struct sockaddr_in address;
               close(fd[0]);/*Close up readable pipe*/
               /*Now may write to fd[1]*/
               /**/
               if(resetRawHandler(h,l))/*Can't find port/passwd info!*/
                  l_exitFromCheckHandlers(fd[1],NULL);
               /*Now h->port and h->ip must be ok.*/

               /*
                  Problems with inet_pton in some platforms, this is
                  non-standart function!
                  inet_pton(AF_INET,h->ip,&address.sin_addr);
                */

               /*Instead:*/

               memset(&address,'\0',sizeof(struct sockaddr_in));
               address.sin_family = AF_INET;
               address.sin_port = htons(h->port);

               address.sin_addr.s_addr=inet_addr(h->ip);

               if (connect(h->wsocket,(struct sockaddr *)&address,sizeof(address)) != 0)
                  l_exitFromCheckHandlers(fd[1],h);

               /*Authorization:*/
               if(  writexactly(h->wsocket,h->passwd,8)  )
                  l_exitFromCheckHandlers(fd[1],h);
               if(  readexactly(h->rsocket,buf,2)  )
                  l_exitFromCheckHandlers(fd[1],h);
               if( hex2int(buf,2)!= s2cOK_i )

                  l_exitFromCheckHandlers(fd[1],h);
               /*Ok.*/
               /*Start new job:*/
               if(  writexactly(h->wsocket,c2sJOB,2)  )
                  l_exitFromCheckHandlers(fd[1],h);
               if(  readexactly(h->rsocket,buf,2)  )
                  l_exitFromCheckHandlers(fd[1],h);
               if( hex2int(buf,2)!= s2cOK_i )
                  l_exitFromCheckHandlers(fd[1],h);
               /*Ok, server is waiting for startup information.*/
               write(fd[1],"1\n",2);/*Success - inform the father*/
               close(fd[1]);
               pause();/*Sleep waiting sigkill from the father*/

               exit(0);/*We wouldn't like to go further, if pause() breaks by a signal, eh?*/
         }/*if(childpid) ... else*/

      }/*if( *(h->ip) == '\0' )...else...*/
   }/*for(c=l->rawHandlers.next ;c != &l->rawHandlers; c=c->next)*/

   return n;
}/*checkHandlers*/

int mkRemoteJobs(int s, int w, int r, struct localClient_struct *l)
{

   l->retcode=-1;
   l->repeatmainloop=1;
   l->nametable=NULL;
   l->jobsQn=l->jobsSQn=l->finishedJobsQn=l->failedJobsQn=0;
   l->jobsPn=l->jobsSPn=0;
   l->rawHandlersN=l->freeHandlersN=l->workingHandlersN=l->killServers=0;
   l->HandlersTable=NULL;
   l->allJobs=NULL;
   l->topJobId=l->maxTopJobId=0;
   l->topHndId=0;

   l->s=s;
   l->w=w;
   l->r=r;
      /*For future developng, not used yet:*/
#ifdef SKIP
   init_q1_queue(&l->toProcessor);
#endif
   init_base_queue(&l->rawHandlers);
   init_base_queue(&l->freeHandlers);
   init_base_queue(&l->workingHandlers);

   init_base_queue(&l->jobsQ);
   init_base_queue(&l->jobsSQ);

   init_ordered_queue(&l->finishedJobsQ, &l_putTheRoot);

   init_base_queue(&l->jobsP);
   init_base_queue(&l->jobsSP);

   /*Try to start local handler:*/
   g_daemonize=1;

   if(l->keepCleintAlive==0){
     char buf[3];
     /*At this point hang up waiting activation:*/
     if( readexactly(r,buf,2) )
        return -1;/*Diana fails?*/
   }else /*if(l->keepCleintAlive)*/
      l->keepCleintAlive=0;

   if( initHandlers(l)<1 )/*No handlers available!*/
         l->repeatmainloop=0;
   else{
      /*l->topHndId now is a number of active handlers*/
      initHandlersTable(l);
      /*Now l->HandlersTable[#] contains a pointer to the container with handler #*/

      if( checkHandlers(l)<1 )/*Can't establish connections!*/
         l->repeatmainloop=0;
      else{/*Ok, finish initiziation:*/
         l->reqs=0;
         l->retcode=0;
      }/*if( checkHandlers(l)<1 )...else*/
   }/*if( initHandlers(l)<1 )...else*/

   if(l->repeatmainloop)
       writexactly(s,"1",1);
   else
       writexactly(s,"0",1);
   close(s);

   gettimeofday(&l->baseTime,NULL);

   while(l->repeatmainloop){
      struct queue_base_struct *c=l->workingHandlers.next;
      /*Clear descriptors:*/
      FD_ZERO(&l->rfds);
      FD_ZERO(&l->wfds);

      /*First, read messages from Diana:*/
      FD_SET(r,&l->rfds);
      l->maxd=r;

      /*For future developng, not used yet:*/
#ifdef SKIP
      if( is_queue(&l->toProcessor) )/*Need to send something to Diana:*/
         FD_SET(w, &l->wfds);
      if(l->maxd<w)l->maxd=w;
#endif
      /*So, at preesent, we do NOT try to check 'w' by select()! BUT - it does NOT
        meas that the descriptor w will not be used at all! The client answers the
        processor synchronously, without checking the status of the descriptor. */

      /* Loop on all working handlers:*/
      while( c != &l->workingHandlers ){
         struct rhandler_struct *h=(struct rhandler_struct *)(c->msg);
         if(  h->status & 3 ) /*1 or 2 */

            if( remaining_time(&(h->st),g_startjob_timeout) == 0 ){/*Timeout!*/

               c=jobFails(c,l,&(((struct job_struct*)(h->theJob->msg))->hitTimeout),
                              MAX_HIT_START_TIMEOUT,1);

               /*Go to the next cell:*/
               continue;

            }/*if( remaining_time(&(h->st),g_startjob_timeout) == 0 )*/
         if( is_queue(&(h->thequeue)) ){/*Need to send something to a job:*/
            FD_SET(h->wsocket, &l->wfds);
            if(l->maxd<h->wsocket)l->maxd=h->wsocket;
         }
         /*And, of course, we have to read messages from the socket:*/
         FD_SET(h->rsocket, &l->rfds);
         if(l->maxd<h->rsocket)l->maxd=h->rsocket;
         /*Continue handlers loop*/
         c=c->next;
      }/*while( c != &l->workingHandlers )*/

      (l->maxd)++;

      /*Wait for nonblocking:*/
      l->selret=select(l->maxd, &l->rfds, &l->wfds, NULL,NULL);

      if(l->selret>0){/*Something interesting*/
         c=l->workingHandlers.next;
         while( c != &l->workingHandlers ){
            struct rhandler_struct *h=(struct rhandler_struct *)(c->msg);
               if(  FD_ISSET(h->rsocket,&l->rfds)  ){
                  /*Message from the job*/
                  char buf[20];
                  if(h->status == 2){/*Need confirmation*/
                     switch(readHex(h->rsocket,buf,2,NULL,0)){
                        case c2pNOK_i:/*Something is wrong.*/
                           if(   (readexactly(h->rsocket,buf,2))
                               ||(hex2int(buf,2) != c2pNEXT_i)
                             )
                              /*Something is wrong with handler:*/
                              c=jobFails(c,l,
                                 &(((struct job_struct*)(h->theJob->msg))->hitFail),
                                 ((struct job_struct*)(h->theJob->msg))->maxnhits,1);
                           else/*Job was not started, but handler is ready to work:*/
                              c=jobFails(c,l,
                                 &(((struct job_struct*)(h->theJob->msg))->hitFail),
                                 ((struct job_struct*)(h->theJob->msg))->maxnhits,0);

                           continue;
                        case c2pWRK_i:/*ok, the job is running*/

                           /*And, now we have to read PID:*/
                           {/*Block*/
                              int i;
                              if(
                                    ( (i=readHex(h->rsocket,buf,2,NULL,0))>0 )
                                  &&(  (((struct job_struct*)(h->theJob->msg))->pid
                                           =readHex(h->rsocket,buf,i,NULL,0))>0
                                    )
                                ){
                                   struct job_struct* theJob=
                                         (struct job_struct*)(h->theJob->msg);
                                   h->status =0;
                                   if( g_jobinfovar_d>-1)/*PID:*/
                                      l_fillinTheFields(8,buf,
                                         theJob,
                                         l);
                                   if(  (theJob->retcode[2])=='1')/*the job was removed*/
                                      l_killJobPrg(theJob,l);/*Try to kill it*/
                                   break;
                              }/*if(...)*/
                           }/*Block*/
                           /*No break, job fails! */
                        default:
                           /*Something is wrong with handler:*/
                           c=jobFails(c,l,
                              &(((struct job_struct*)(h->theJob->msg))->hitFail),
                              ((struct job_struct*)(h->theJob->msg))->maxnhits,1);
                           continue;
                     }/*switch(hex2int(buf,2))*/
                  }else{/*if(h->status == 2)*/
                     int *hitFail=&(((struct job_struct*)(h->theJob->msg))->hitFail);
                     if(
                         ( ((struct job_struct*)(h->theJob->msg))->type & ERRORLEVEL_JOB_T )
                         &&(((struct job_struct*)(h->theJob->msg))->errlevel == -2)
                       )/*errlevel == -2 means that the job is successfully finished
                          if it was started by the server. Here h->status must be 0, so
                          the job is assumed to be successful:*/
                           hitFail=NULL;/*jobFails(x,x,NULL,x,x) ends job sucessfully*/

                     /*h->status must be 0!*/
                    /*!!! future developing: some other info from the server!!!*/
                    /*Att! At the end of this the if() branch, the job is assumed to
                    be finished! See the comment 'Now job is finished!'*/
                     /*At pesent, we expect from the server only 1a  XXXXXXXX 1d:*/
                     switch(readHex(h->rsocket,buf,2,NULL,0)){
                        case c2pFIN_i:/*Normal finishing, read the status:*/
                            /*See the comment to c2pFIN, file rproto.h*/
                            {/*block*/
                               char isremoved=
                               (((struct job_struct*)(h->theJob->msg))->retcode)[2];
                               /*may be set up by l_report, if the job about to removed*/

                               int r=readexactly(h->rsocket,
                                           ((struct job_struct*)(h->theJob->msg))->retcode,
                                                      8);
                                (((struct job_struct*)(h->theJob->msg))->retcode)[2]=
                                      isremoved;
                                /*Now retcode[2] is restored, so jobFails will understand
                                  was the job removed, or it is just finished*/

                                /*Signals are transported throu the network as internal
                                  diana's numbers.*/
                                /*Convert diana's internal signals to system ones:*/
                                int2hex(
                                   ((struct job_struct*)(h->theJob->msg))->retcode+4,
                                   int2sig(
                                      hex2int(
                                         ((struct job_struct*)(h->theJob->msg))->retcode+4,
                                         4
                                      )
                                   ),
                                   -4
                                );

                                if(r){
                                    /*Something is wrong with handler:*/
                                    c=jobFails(c,l,
                                       hitFail,
                                       ((struct job_struct*)(h->theJob->msg))->maxnhits,1);
                                    continue;
                                }/*if(r)*/
                            }/*block*/
                            break;
                         case c2pDIED_i:/*Status is lost!*/
                            s_let("00040000",((struct job_struct*)(h->theJob->msg))->retcode);
                            break;
                         default:
                            /*Something is wrong with handler:*/
                            c=jobFails(c,l,
                               hitFail,
                               ((struct job_struct*)(h->theJob->msg))->maxnhits,1);
                            continue;
                     }/*switch*/
                     /*Here only c2pNEXT may be!*/
                     if(readHex(h->rsocket,buf,2,NULL,0)!= c2pNEXT_i){
                          /*Something is wrong with handler:*/
                          c=jobFails(c,l,
                             hitFail,
                             ((struct job_struct*)(h->theJob->msg))->maxnhits,1);
                          continue;
                     }
                     /*Now job is finished!*/
                     /*Successful, or not? By default, it is sucessful. But may be the
                       errorlevel of the job must be analysed:*/
                     if( ((struct job_struct*)(h->theJob->msg))->type & ERRORLEVEL_JOB_T ){
                        /*non-trivial success condition*/
                        if( ((struct job_struct*)(h->theJob->msg))->errlevel < 0)
                           hitFail=NULL;/*-1 -- default, -2 -- sucess*/
                        else{ /*returned exit code must exist and be <= errlevel*/
                           int retcode=((struct job_struct*)(h->theJob->msg))->errlevel;
                           char *sretcode=((struct job_struct*)(h->theJob->msg))->retcode;
                           if(
                                ( (sretcode[3]=='0')&&(sretcode[2]=='0') )/*Normal exit*/
                              &&( hex2int(sretcode,2) <= retcode )/*exit code mutches*/
                             )hitFail=NULL;/*Sucess*/
                        }/*if( ((struct job_struct*)(h->theJob->msg))->errlevel < 0)...else*/
                     }else
                        hitFail=NULL;/*Default, success*/

                     c=jobFails(c,l,
                        hitFail,
                        ((struct job_struct*)(h->theJob->msg))->maxnhits,0
                     );/*Move it to finishedJobsQ*/

                     continue;
                  }/*if(h->status == 2) ... else*/
               }/*if(  FD_ISSET(h->rsocket,&l->rfds)  )*/
               if(
                     (  FD_ISSET(h->wsocket,&l->wfds)  )
                   &&(  is_queue(&h->thequeue)  )
                 ){
                   /*Need to send a message to the job*/
                  char *buf;
                  ssize_t len;
                  len=get_q1_queue(&buf, &h->thequeue);
                  if(len>g_socket_buf_len)
                     len=g_socket_buf_len;
                  len=writeFromb(h->wsocket, buf, len);
                  if(len<1){/*Is it died (?)*/
                     /*No clear_q1_queue(&h->thequeue);! jobFails does it!:*/
                     c=jobFails(c,l,
                        &(((struct job_struct*)(h->theJob->msg))->hitFail),
                        ((struct job_struct*)(h->theJob->msg))->maxnhits,1);
                     continue;
                  }else{/*if(len<1)*/
                     accept_q1_queue(len,&h->thequeue);
                     h->status=2;
                  }/*if(len<1)..else*/
               }/*if()*/
               c=c->next;
         }/*while( c != &l->workingHandlers )*/
         /*Jobs are performed*/

         if(  FD_ISSET(r,&l->rfds)  ){
            /*Diana screams*/
            char buf[9];
            char *stickyname=NULL;

            struct job_struct *newjob=NULL;
            int ln=-1,thetype=0,cmd;
               switch(cmd=readHex(r,buf,2,NULL,0)){
                  case d2cSETSTART_TO_i:
                     g_startjob_timeout = readHex(r,buf,8,NULL,0);
                     break;
                  case d2cSETCONNECT_TO_i:
                     g_connect_timeout  = readHex(r,buf,8,NULL,0);
                     break;
                  case d2cSTOPTRACE_i:
                     clear_jobinfo();
                     break;
                  case d2cSTARTTRACE_i:
                     if( (ln=readHex(r,buf,2,NULL,0))<1 )break;/*What can I do?*/
                     {/*block*/
                       /*Here we assume ".var" for file 1 and ".nam" for file 2*/
                       char *fileroot=get_mem(ln+5,sizeof(char));/*Will be g_jobinfonam
                                                                   further*/
                        if( !readexactly(r,fileroot,ln) ){
                           /*Note here , if tracing already in progress, then NOTHING
                             will happen despite filenames are changed!
                           */
                           if(g_jobinfovar!=NULL)
                              free(g_jobinfovar);
                           g_jobinfovar=get_mem(ln+5,sizeof(char));
                           s_cat(g_jobinfovar,fileroot,".var");
                           if(g_jobinfonam!=NULL)
                              free(g_jobinfonam);

                           g_jobinfonam=fileroot;
                           /*And paste to the end ".nam":*/
                           while( (*fileroot)!='\0' )fileroot++;
                           s_let(".nam",fileroot);
                        }/*if( !readexactly(r,fileroot,ln) )*/
                     }/*block*/
                     break;
                  case d2cGETJINFO_i:
                     if( (ln=readHex(r,buf,2,NULL,0))<1 )break;/*What can I do?*/
                     {/*Block*/
                        char *fmt=get_mem(ln+1,sizeof(char));
                        char *thenaswer;
                        int n;
                        if( readexactly(r,fmt,ln) )break;/*What can I do?*/
                        if( (n=readHex(r,buf,8,NULL,0))<1 )break;/*What can I do?*/
                        ln=s_len(thenaswer=l_getJobInfoLine(fmt,n,l));
                        writexactly(w,int2hex(buf,ln,2),2);
                        writexactly(w,thenaswer,ln);
                        free(thenaswer);
                        free(fmt);
                     }/*Block*/
                     break;
                  case d2cGETFAILQN_i:
                     l_report(cmd,NULL,l);
                     break;
                  case d2cGETNAME2STICK_i:
                  case d2cSENDSIG_i:
                  case d2cGETPID_i:
                  case d2cGETHITS_i:
                  case d2cGETfTIME_i:
                  case d2cGETsTIME_i:
                  case d2cGETTIME_i:/*Report time spent by a job*/
                  case d2cGETST_i:/*Report status of a job*/
                  case d2cGETIP_i:/*Read a name and return IP*/
                  case d2cRMJOB_i:/*Read a name and return nothing*/
                     if( (ln=readHex(r,buf,2,NULL,0))<1 )break;/*What can I do?*/
                     /*Now ln is the job name length*/
                     newjob=get_mem(1,sizeof(struct job_struct));
                     newjob->name=get_mem(ln+1,sizeof(char));
                     if( readexactly(r,newjob->name,ln) )break;/*What can I do?*/
                     {/*block*/
                       struct job_struct *job=NULL;
                       if(l->nametable!=NULL)
                          job=(struct job_struct *)lookup(newjob->name,l->nametable);
                       l_report(cmd,job,l);
                     }/*block*/
                     free(newjob->name);
                     free(newjob);
                     newjob=NULL;
                     break;
                  case d2cGETNREM_i:/*Report number of running jobs*/
                     {/*block*/
                       unsigned long int n=l->workingHandlersN
                                          +l->jobsQn
                                          +l->jobsSQn
                                          +l->jobsPn
                                          +l->jobsSPn;
                       /*Now in in n we have a number of jobs in progress*/
                          writexactly(w,c2dREMJOBS,2);/*send a command*/
                          writexactly(w,int2hex(buf,ln=hexwide(n),1),1);/*send a width*/
                          {/*block*/
                             char *b=get_mem(ln+1,sizeof(char));
                                writexactly(w,int2hex(b,n,ln),ln);/*send a number*/
                                free(b);

                          }/*block*/
                     }/*block*/
                     break;
                  case d2cNEWJOB_i:/*Start a new job*/
                     if( (ln=readHex(r,buf,2,NULL,0))<1 )break;/*What can I do?*/
                     newjob=get_mem(1,sizeof(struct job_struct));
                     newjob->name=get_mem(ln+1,sizeof(char));

                     if( readexactly(r,newjob->name,ln) )break;/*What can I do?*/
                     if( (thetype=readHex(r,buf,8,NULL,0))<0 )break;/*What can I do?*/
                     /*Now thetype is a bit flag of types*/
                     if( thetype & STICKY_JOB_T ){/*Sticky - must read the sticky name:*/
                        int l;
                        if( (l=readHex(r,buf,2,NULL,0))<1 )break;/*What can I do?*/
                        stickyname=get_mem(l+1,sizeof(char));
                        if( readexactly(r,stickyname,l) )break;/*What can I do?*/
                     }/*if( thetype & STICKY_JOB_T )*/

                     if( thetype & ERRORLEVEL_JOB_T ){
                        if( readexactly(r,buf,2)<0   ) break;
                        /*Here we trust the sender - this must be the signed hex number*/
                        newjob->errlevel=hex2int(buf,2);
                     }else
                        newjob->errlevel=ERRORLEVEL_JOB_Di;

                     if( thetype & RESTART_JOB_T ){
                        if( (newjob->maxnhits=readHex(r,buf,2,NULL,0))<1 )break;
                     }else
                        newjob->maxnhits=RESTART_JOB_Di;

                     /*And, args:*/
                     if( (ln=readHex(r,buf,2,NULL,0))<1 )break;
                     /*Now ln is the number of arguments. Read them:*/
                     newjob->args=l_readArgs(r,ln,buf);
                     /*Note, here "buf" is correct - it must be of length 2*/

                     if(newjob->args==NULL)break;/*Fails*/

                     if(l->nametable==NULL)l->nametable=create_hash_table(
                                 g_jobnamestablesize,
                                 str_hash,str_cmp,
                                 NULL);

                     if( thetype & STICKY_JOB_T ){
                        newjob->stickJob=(struct job_struct *)
                            lookup(stickyname,l->nametable);
                        if(  newjob->stickJob==NULL ){
                           /*No such a name!*/
                           writexactly(w,c2dWRONGSTICKYNAME,2);
                           break;
                        }/*if(  newjob->stickJob==NULL )*/
                        if(     (thetype & FAIL_STICKY_JOB_T)
                              &&(newjob->placement==FAILED_JOBS_Q)
                          ){
                           /*Job to stick is failed, and this job must be failed, too:*/
                           writexactly(w,c2dFAILDTOSTICK,2);
                           break;
                        }/**/
                     }/*if( thetype & STICKY_JOB_T )*/
                     newjob->type=thetype;
                     if( lookup(newjob->name,l->nametable)==NULL ){
                        if( ++(l->topJobId) > TOP_JOBS_ID )
                           halt(TOOMANYJOBS,NULL);

                        newjob->id=l->topJobId;
                        install(newjob->name,newjob,l->nametable);
                        if( (l->topJobId)>=(l->maxTopJobId) ){
                           l->maxTopJobId+=DELTA_MAXTOPJOBS;
                           l->allJobs=realloc(l->allJobs,
                                  sizeof(struct job_struct *)*(l->maxTopJobId));
                           if( (l->allJobs)==NULL)
                              halt(NOTMEMORY,NULL);
                        }/*if( (l->topJobId)>=(l->maxTopJobId) )*/
                        (l->allJobs)[l->topJobId]=newjob;

                        newjob->hitTimeout=newjob->hitFail=0;
                        newjob->pid=0;
                        /*xx02xxxx means that the job was NOT running at all:*/
                        s_let("00020000",newjob->retcode);

                        /*No time at all:*/
                        (newjob->st).tv_sec=-1;
                        (newjob->st).tv_usec=-1;
                        (newjob->ft).tv_sec=-1;
                        (newjob->ft).tv_usec=-1;

                        c=get_mem(1,sizeof(struct queue_base_struct));

                        c->msg=newjob;
                        c->len=sizeof(struct job_struct);

                        newjob->myQueueCell = c;

                        if( thetype & SYNC_JOB_T) {/*sync queue*/
                           newjob->placement=SYNCH_JOBS_Q;
                           link_cell(c,&l->jobsSQ);
                           (l->jobsSQn)++;
                        }else{/*async queue*/
                           newjob->placement=JOBS_Q;
                           link_cell(c,&l->jobsQ);
                           (l->jobsQn)++;
                        }/*if( thetype & SYNC_JOB_T)*/
                        if(
                              ( g_jobinfovar_d> -1)/*Service in progress*/
                            ||(
                                  (newjob->id==1)
                                &&(g_jobinfovar!=NULL)/*Need to open the service*/
                              )
                          ){
                             switch(l_new_jobinfo(newjob,l)){
                                case -2: message(JOBINFOFILEISCORRUPTED,g_jobinfovar);
                                case -1: clear_jobinfo();
                                default:
                                   break;
                             }/*switch(l_new_jobinfo(newjob,l))*/
                        }/*if(...)*/
                        c=NULL;
                        newjob=NULL;
                        writexactly(w,c2dJOBRUNNING,2);
                     }else{/* already in use*/
                        writexactly(w,c2dWRONGJOBNAME,2);
                     }
                     break;
                  case d2cKILLALL_i:
                     l->killServers=1;
                     break;
                  case d2cFINISH_i:
                     l->keepCleintAlive=1;
                  case -1:/*Processor is died!*/
                     l->repeatmainloop=0;
                     break;
                  case d2cREPALLJOB_i:/*"REP stays for "REPORT", not "REPEAT"!*/
                     if(
                        (l->workingHandlersN/*No running*/
                         +l->jobsQn/*No async. waititng*/
                         +l->jobsSQn/*No sync. waititng*/
                         +l->jobsPn/*No non-sticky pipelines*/
                         +l->jobsSPn/*No sticky pipelines*/
                        )==0
                       )
                         writexactly(w,c2dALLJOB,2);
                     else
                        l->reqs |= REPORT_ALL_JOB_PERFORMED;
                     break;
                  case d2cNOREPALLJOB_i:
                     l->reqs = l->reqs & ~REPORT_ALL_JOB_PERFORMED;
                     break;
                  default:/*Unrecognized command*/
                     break;
               }/*switch(readHex(r,buf,2,NULL,0))*/

               free_mem(&stickyname);

               if(newjob!=NULL){
                  free_mem(&(newjob->name));
                  free(newjob);
               }/*if(newjob!=NULL)*/

         }/*if(  FD_ISSET(r,&l->rfds)  )*/

      /*For future developng, not used yet:*/
#ifdef SKIP
         if(    (  FD_ISSET(w,&l->wfds)  )
              &&( is_queue(&l->toProcessor) )
           ){
            /*Need to send something to Diana*/
            char *buf;
            ssize_t len;
               len=get_q1_queue(&buf, &l->toProcessor);
               if(len>g_socket_buf_len)
                     len=g_socket_buf_len;
               len=writeFromb(w, buf, len);
               if(len<1){/*Is it died (?)*/
                  clear_q1_queue(&l->toProcessor);
                  /*Should I die, too?*/
               }else{/*if(len<1)*/
                  accept_q1_queue(len,&l->toProcessor);
               }/*if(len<1)..else*/
         }/*if()*/
#endif

         /*What about queues?*/
         {/*block*/
            struct queue_base_struct *c=l->jobsSQ.prev;
               /*First, try sync:*/
               while( c != &l->jobsSQ ){
                  struct queue_base_struct *job=c;
                  if( l_theval(c)-1 != l_theval(l->finishedJobsQ.prev) )
                     break;/*Indeed, the queue jobsSQ is ORDERED! And,
                     it is impossible to be l_theval(c)<=l_theval(l->finishedJobsQ.prev)*/
                  c=c->prev;
                  if (l_job2pipe(job,l))/*Fail*/
                     continue;
                  l->jobsSQn--;
               }/*while( c != &l->jobsSQ )*/
               /*And, acynk - just add all stuff:*/
               c=l->jobsQ.prev;
               while( c != &l->jobsQ ){
                  struct queue_base_struct *job=c;
                  c=c->prev;
                  if (l_job2pipe(job,l))/*Fail*/
                     continue;
                  l->jobsQn--;
               }/*while( c != &l->jobsQ )*/
         }/*block*/
         /*Now try to handle pipelines:*/
         /*Sticky pipeline:*/
         {/*block*/
            /*Sticky pipeline:*/
            struct queue_base_struct *c;
            int Nnew=l->freeHandlersN;
            for(c=l->jobsSP.prev; c != &l->jobsSP; c=c->prev ){
               struct job_struct *job=(struct job_struct *)(c->msg);
               if( Nnew<=0 ){
                  /*No more free handlers!*/
                  /*First, check, are there handlers at all?*/
                  if( (l->freeHandlersN)+(l->workingHandlersN)<1 ){
                     /*No handlers at all! Clear the pipeline:*/
                     c=l_stickyJobFails(c,l);
                     /*Note, checkHandlers here is a nonsense since the jobs are sticky.*/
                     continue;
                  }/*if( (l->freeHandlersN)+(l->workingHandlersN)<1 )*/
                  /*else - give up the pipe:*/
                  break;
               }/*if( Nnew<=0 )*/

               switch(job->stickJob->placement){
                  case FAILED_JOBS_Q:
                    if(job->type & FAIL_STICKY_JOB_T){
                      c=l_stickyJobFails(c,l);
                      continue;
                    }/*if(job->type & FAIL_STICKY_JOB_T)*/
                    /*No break:*/
                  case FINISHED_JOBS_Q:
                    break;
                  default:
                     continue;
               }/*switch(stickJob->placement)*/

               if(  (job->stick2hndid =job->stickJob->handlerid)==0  )
                  continue;/*Job to stick to is still in the queue*/

               switch(
                    (
                       (struct rhandler_struct*)
                       (l->HandlersTable[job->stick2hndid]->msg)
                    )->status){
                  case -2:/*Raw handler - fail!*/
                     c=l_stickyJobFails(c,l);
                     break;
                  case -1:/*Ok, stick to it*/
                     {/*block*/
                        struct queue_base_struct *tmp=c->next;
                        startNewJob(
                                       c,
                                       l->HandlersTable[job->stick2hndid],
                                       l
                                    );
                        Nnew--;
                        c=tmp;
                     }/*block*/
                     break;
                  default:/*busy, just continue:*/
                     continue;
               }/*switch(l->HandlersTable[job->stick2hndid]->status)*/
            }/*for(c=l->jobsSP.prev; c != &l->jobsSP; c=c->prev )*/
         }/*block*/
         /*Non-sticky:*/
         {/*block*/
            struct queue_base_struct *c;
            for(c=l->jobsP.prev; c != &l->jobsP; c=c->prev ){
               struct queue_base_struct *tmp=c->next;
               if(  l->freeHandlers.prev == &(l->freeHandlers)  )
                  if(checkHandlers(l)<1){
                     /*No more free handlers!*/
                     /*First, check, are there handlers at all?*/
                     if( (l->freeHandlersN)+(l->workingHandlersN)<1 ){
                        /*No handlers at all!*/
                        c=l_stickyJobFails(c,l);
                        /*Clear the pipeline:*/
                        continue;
                     }/*if( (l->freeHandlersN)+(l->workingHandlersN)<1 )*/
                     /*else - give up the pipeline processing:*/
                     break;
                  }/*if(checkHandlers(l)<1)*/
               startNewJob(
                              c,
                              l->freeHandlers.prev,
                              l
               );
               c=tmp;

            }/*for(c=l->jobsP.prev; c != &l->jobsP; c=c->prev )*/
         }/*block*/
      }/*if(l.selret>0)*/
      /*Else - interrupted by a signal, or error. Just continue?*/
   }/*while(l->repeatmainloop)*/

   giveUpAll(l);

   shutdownAllHandlers(l);

   cleanJobsQueue(&(l->finishedJobsQ),&(l->finishedJobsQn));
   l->failedJobsQn=0;/*This is not a real queue counter!*/

      /*For future developng, not used yet:*/
#ifdef SKIP
   clear_q1_queue(&l->toProcessor);
#endif

   clear_base_queue(&l->freeHandlers);
   clear_base_queue(&l->workingHandlers);

   if(l->killServers)
      l_killAllServers(l);

   clear_base_queue(&l->rawHandlers);

   free_mem(&l->HandlersTable);

   /*
      clear_base_queue(&l->jobsQ); - performed by giveUpAll();
      clear_base_queue(&l->finishedJobsQ); - performed by cleanJobsQueue(&(l->finishedJobsQ),&(l->finishedJobsQn));
    */
   if (l->nametable!=NULL){
          hash_table_done(l->nametable);
          l->nametable=NULL;
   }/*if (l->nametable!=NULL)*/

   free_mem( &(l->allJobs));

   clear_jobinfo();

   /*Do NOT close pipe to diana!*/
   return l->retcode;
}/*mkRemoteJobs*/

int activateClient(int sig)
{
char buf[2];
int ret=0;
   if(             writexactly(g_toserver,"1\n",2)
                 ||readexactly(sig,buf,1)
                 ||(*buf!='1')
     ){
        close(g_fromserver);
        close(g_toserver);
        g_fromserver=g_toserver=-1;
        ret=-1;
      }
   close(sig);
   g_pipetoclient=-1;
   return ret;
}/*activateClient*/

pid_t startclient(int *p)
{
pid_t chld;
int toDiana[2],toClient[2],fdsig[2];

   if (pipe(toDiana)!= 0) return -1;
   if (pipe(toClient)!= 0) return -1;
   if(pipe(fdsig)!=0) return -1;/*This pipe will be used by a child to
                                   tell the father if fail.*/
   switch(chld=fork()){
      case -1:
         close(toDiana[0]);
         close(toDiana[1]);
         close(toClient[0]);
         close(toClient[1]);
         close(fdsig[0]);
         close(fdsig[1]);
         return -1;
      case 0:/*child*/
         {/*block*/
           struct localClient_struct *l=get_mem(1,sizeof(struct localClient_struct));
#ifdef SKIP
           int i,maxfd=(fdsig[0]>fdsig[1])?fdsig[0]:fdsig[1];
            for(i=0;i<=maxfd;i++){
               if(
                    (i!=toDiana[1])
                  &&(i!=toClient[0])
                  &&(i!=fdsig[1])
                 )close(i);
            }/*for(i=0;i<=maxfd;i++)*/
//            l_closeAllDescriptors(maxfd+1);
#endif
            /**/
            close(toDiana[0]);
            close(toClient[1]);
            close(fdsig[0]);
            /**/
            l->keepCleintAlive=0;
            do{
               mkRemoteJobs(fdsig[1],toDiana[1],toClient[0],l);
            }while(l->keepCleintAlive);
            free(l);
            exit(0);
         }/*block*/
      default:/*The father:*/
         close(toDiana[1]);
         close(toClient[0]);
         close(fdsig[1]);
         if(g_rlocalsock!=NULL){
            /*Need not sockets to local servers anymore!*/
            int i;
            for(i=0;i<g_numberofprocessors;i++){
               close(g_wlocalsock[i]);
               close(g_rlocalsock[i]);
            }/*for(i=0;i<g_numberofprocessors;i++)*/
            free_mem(&g_rlocalsock);
            free_mem(&g_wlocalsock);
         }/*if(g_rlocalsock!=NULL)*/
         g_fromserver=toDiana[0];
         g_toserver=toClient[1];

         if (*p<0){
            *p=fdsig[0];
         }else{
            if(activateClient(fdsig[0])){
               *p=-1;
            }/*if(activateClient(fdsig[0]))*/
         }/*if (*p>-1)...else*/
         g_jobscounter=0;
         break;
   }/*switch(fork())*/

   return chld;
}/*startclient*/

pid_t startserver(int n,int thenice)
{
pid_t chld;
   if(n<1)return 0;

   switch(chld=fork()){
      case -1:return -1;
      case 0:/*A child*/
         if(g_daemonize){/*Become a daemon - all descriptors to /dev/null;
                           start a new session. */
            int i;
            close(0);/*close stdin*/
            open("/dev/null",O_RDONLY); /* reopen stdin */
            close(1);/*close stdout*/
            close(2);/*close stderr*/
            if(  (i=open("/dev/null",O_WRONLY))>0 )/*reopen stdout*/
               dup(i); /* reopen stderr */
            setsid(); /* Detach from the current process group and
                        obtain a new process group */
         }/*if(g_daemonize)*/
         {int ret= runserver(n,thenice);
            exit(ret);
         }
      default: break;
   }/*switch(chldfork())*/
   return chld;
}/*startserver*/

