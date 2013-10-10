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
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>

#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"

word hash_main_id(void *tag, word tsize)/* hash function. First element of
tag containes number of the identifiers, follow allocated identifiers as
 ASCII-Z strings. */
{
 char *s=(char *)tag;
 char i=*s;
 word tmp=0;
    while(i--){
       (char*)s++;
       while(*s)tmp+=(*(char*)s++);
    }
    return(tmp % tsize);
}/*hash_main_id*/

int main_id_cmp(void *tag1, void *tag2)
{
return(m_cmp(tag1,tag2));
}/*main_id_cmp*/

void empty_destructor(void **tag, void **cell)
{
}/*empty_destructor*/

word alloc_id(void)
{
   if(main_id_table==NULL)
       main_id_table=create_hash_table(mainid_hash_size,hash_main_id,
                                    main_id_cmp,int_destructor);
   if (!(top_id<max_top_id))
      if ((id=realloc(id,(max_top_id+=DELTA_ID)*
                              sizeof(struct internal_id_cell)))==NULL)
         halt(NOTMEMORY,NULL);
   id[top_id].number=0;
   id[top_id].kind=0;
   id[top_id].id=NULL;
   id[top_id].type=0;
   id[top_id].form_id=NULL;
   id[top_id].mass=NULL;
   id[top_id].skip=0;
   id[top_id].Nnum='\0';
   id[top_id].Nmark='\0';
   id[top_id].Nind='\0';
   id[top_id].Nfnum='\0';
   id[top_id].Nfflow='\0';
   id[top_id].Nfromv='\0';
   id[top_id].Ntov='\0';
   id[top_id].Nvec='\0';

   return(top_id);
}/*alloc_id*/

void set_id(char *ident)
{
  int *ptr;
   *(ptr=get_mem(1,sizeof(int)))=top_id;
   if(install(m_let(ident,(id[top_id].id=get_mem(1,m_len(ident)))),
           ptr,main_id_table))
                    halt(DOUBLEDEF,m2s(ident));
   id[top_id].number=top_id;
}/*set_id*/

void int_destructor(void **tag, void **cell)
{
   free_mem(cell);
}/*int_destructor*/

int topology_cmp(void *tag1, void *tag2)
{
  char i;

   if((((pTOPOL)tag1)->i_n != ((pTOPOL)tag2)->i_n)||
      (((pTOPOL)tag1)->e_n != ((pTOPOL)tag2)->e_n))return(0);
   for(i=1;!(i>((pTOPOL)tag1)->i_n);i++){
     if ((((pTOPOL)tag1)->i_line)[i].from!=(((pTOPOL)tag2)->i_line)[i].from)
         return(0);
     if ((((pTOPOL)tag1)->i_line)[i].to != (((pTOPOL)tag2)->i_line)[i].to)
         return(0);
   }
   for(i=1;!(i>((pTOPOL)tag1)->e_n);i++){
     if ((((pTOPOL)tag1)->e_line)[i].from != (((pTOPOL)tag2)->e_line)[i].from)
        return(0);
     if ((((pTOPOL)tag1)->e_line)[i].to != (((pTOPOL)tag2)->e_line)[i].to)
        return(0);
   }
   return(1);
}/*topology_cmp*/

word hash_topology(void *tag, word tsize)
{
  int i;
  word h=0;
     for(i=((pTOPOL)tag)->i_n;i>0;i--){
        h=(h<<1)|(h>>g_bits_in_int_1);
        h^=(((pTOPOL)tag)->i_line)[i].to;
     }
     return(h % tsize);
}/*hash_topology*/

void flinks_destructor(void **tag, void **cell)
{
struct flinks_struct *tmp;
#ifdef DEBUG
  if ((*tag==NULL)||(*cell==NULL))
      halt("flinks_destructor:NULL argument.",NULL);
#endif
    free_mem(tag);
    tmp= *cell;
    if( (tmp->stream)!=NULL )
       fclose(tmp->stream);
    free_mem(cell);
}/*flinks_destructor*/

void def_destructor(void **tag, void **cell)
{
struct def_struct *tmp;
int i;
#ifdef DEBUG
  if ((*tag==NULL)||(*cell==NULL))
      halt("def_destructor:NULL argument.",NULL);
#endif
    free_mem(tag);
    tmp=*cell;
    free_mem(&(tmp->filename));
    for(i=0; i < tmp->top; i++)
       free( (tmp->str)[i] );
    free_mem(&(tmp->str));
    free_mem(cell);
}/*def_destructor*/

static word l_pipehash(void *tag, word tsize)
{
return *((pid_t*)tag) % tsize;
}/*l_pipehash*/

static int l_pipecmp(void *tag1,void *tag2)
{
   return (*((pid_t*)tag1))==(*((pid_t*)tag2));
}/*l_pipecmp*/

static void l_pipedestructor(void **tag, void **cell)
{
   if( (*((ALLPIPES**)cell))->r == g_pipeforwait )
      g_pipeforwait=-1;

   if((*((ALLPIPES**)cell))-> strr!= NULL)
      fclose((*((ALLPIPES**)cell))->strr);
   close((*((ALLPIPES**)cell))->w);
   kill((*((ALLPIPES**)cell))->pid,SIGKILL);

   free_mem(cell);
}/*l_pipedestructor*/

/*This function is used to add a swallowed processes with pipes to
 the hash table g_allpipes:
*/
void store_pid(pid_t pid,int fdsend,int fdreceive)
{
ALLPIPES *pipetohash;
   pipetohash=get_mem(1,sizeof(ALLPIPES));

   pipetohash->r=fdreceive;
   pipetohash->w=fdsend;
   pipetohash->strr=fdopen(fdreceive,"r");
   pipetohash->pid=pid;

   if(g_allpipes == NULL)
              g_allpipes = create_hash_table(g_Nallpipes,
                                             &l_pipehash,
                                             &l_pipecmp,
                                             &l_pipedestructor);

   install(&(pipetohash->pid),pipetohash,g_allpipes);
}/*store_pid*/

static void l_storesig(char *thename, int thenumber)
{
int *num=get_mem(1,sizeof(int));
   (*num)=thenumber;
   install(thename,num,g_sighash_table);
}/*l_storesig*/

static void l_init_sighash(void)
{
      g_sighash_table=create_hash_table(nextPrime(mSIGUNUSED+3),str_hash,str_cmp,int_destructor);
      l_storesig("HUP", mSIGHUP);
      l_storesig("INT", mSIGINT);
      l_storesig("QUIT", mSIGQUIT);
      l_storesig("ILL", mSIGILL);
      l_storesig("ABRT", mSIGABRT);
      l_storesig("FPE", mSIGFPE);
      l_storesig("KILL", mSIGKILL);
      l_storesig("SEGV", mSIGSEGV);
      l_storesig("PIPE", mSIGPIPE);
      l_storesig("ALRM", mSIGALRM);
      l_storesig("TERM", mSIGTERM);
      l_storesig("USR1", mSIGUSR1);
      l_storesig("USR2", mSIGUSR2);
      l_storesig("CHLD", mSIGCHLD);
      l_storesig("CONT", mSIGCONT);
      l_storesig("STOP", mSIGSTOP);
      l_storesig("TSTP", mSIGTSTP);
      l_storesig("TTIN", mSIGTTIN);
      l_storesig("TTOU", mSIGTTOU);
      l_storesig("BUS", mSIGBUS);
      l_storesig("POLL", mSIGPOLL);
      l_storesig("PROF", mSIGPROF);
      l_storesig("SYS", mSIGSYS);
      l_storesig("TRAP", mSIGTRAP);
      l_storesig("URG", mSIGURG);
      l_storesig("VTALRM", mSIGVTALRM);
      l_storesig("XCPU", mSIGXCPU);
      l_storesig("XFSZ", mSIGXFSZ);
      l_storesig("IOT", mSIGIOT);
      l_storesig("EMT", mSIGEMT);
      l_storesig("STKFLT", mSIGSTKFLT);
      l_storesig("IO", mSIGIO);
      l_storesig("CLD", mSIGCLD);
      l_storesig("PWR", mSIGPWR);
      l_storesig("INFO", mSIGINFO);
      l_storesig("LOST", mSIGLOST);
      l_storesig("WINCH", mSIGWINCH);
      l_storesig("UNUSED", mSIGUNUSED);
}/*l_init_sighash*/

static char l_sig[4]="SIG";

/*Expects thesig is either a symbolic SIG... (SIG prefix may be omitted), or a number.
  All possible symbolic names see in comdef.h.
  If it is a symbolic name, the function returns a Diana-specific number; if it is a
  number, the function returns this number. The return buffer is thesig. On error,
  returns NULL. The first  character in the returned string is either '1' (if it was a
  symbolic name) or '0' (if it was a number). Returned numbers are in HEX format of 2
  digits wide:
 */
char *reduce_signal(char *thesig)
{
int i,thestat=0, digitlen=0;
char *ptr=thesig;

   for(i=0;*ptr!='\0';i++,ptr++)switch(*ptr){
         case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
            digitlen++;
            break;
         default:
            *ptr=toupper(*ptr);
            if(   (i<3) && ( (*ptr)==l_sig[i] )   ) thestat |= (1 << i);
            break;
   }/*for(i=0;*ptr!='\0';i++,ptr++)switch(*ptr)*/
   if(digitlen==i){/*A number*/
      int p=1;
      if(i>3) return NULL;/*Too long!*/

      /*Construct in thestat the integer:*/
      for(p=1,thestat=0,digitlen--; digitlen>-1;digitlen--,p*=10)switch(thesig[digitlen]){
         case '0':break;
         case '1':thestat+=p;break;
         case '2':thestat+=p*2;break;
         case '3':thestat+=p*3;break;
         case '4':thestat+=p*4;break;
         case '5':thestat+=p*5;break;
         case '6':thestat+=p*6;break;
         case '7':thestat+=p*7;break;
         case '8':thestat+=p*8;break;
         case '9':thestat+=p*9;break;
         default:break;
      }/*for(p=1,thestat=0,digitlen--; digitlen>-1;digitlen--,p*=10)switch(ptr[digitlen])*/
      if(thestat>255) return NULL;/*Too large!*/
      *thesig='0';
   }else if(digitlen == 0){/*A symbolic name*/
      int *n;
      if(thestat == 7)/*bitmask 111, the beginning corresponds to "SIG"*/
         ptr=thesig+3;
      else
         ptr=thesig;
      if(g_sighash_table==NULL)
         l_init_sighash();
      if(   ( n=lookup(ptr,g_sighash_table) )==NULL   )
         return NULL;
      thestat=(*n);
      *thesig='1';
   }else/*An error, digits mixed with alpha-chars.*/
      return NULL;
   /*Now thestat is the number from the table (if digitlen==0, *thesig=='0';) or converted
      from the argument (if digitlen == -1,*thesig='1') */

   int2hex(thesig+1, thestat, 2);
   return thesig;
}/*reduce_signal*/
