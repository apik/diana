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
#include <math.h>
/*#include <time.h>*/
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"


/*Removed?:*/
#ifdef PIPE_CHLD
void l_onsigchld(int i)
/* Sometimes, this paranoya may occurs:*/
/*int l_onsigchld(int)*/
{
pid_t pid;
ALLPIPES *thepipe;
   signal(SIGCHLD,&l_onsigchld);
   while( (pid=waitpid(0, &i, WNOHANG))>0 ){
      if(  (thepipe=lookup(&pid,g_allpipes))==NULL  ) continue;
      uninstall(&(thepipe->pid),g_allpipes);
   }/*while( (pid=waitpid(0, &i, WNOHANG))>0 )*/
}/*void l_onsigchld(int)*/
#endif

/*#define STACK_DUMP_ON_CRASH 1*/
/*The following code is GNU-specific!:*/
#ifdef STACK_DUMP_ON_CRASH
static void l_handle_sigsegv(int sig)
{
unsigned int i;
void *a;
char buf[128];
long pid;
  pid=getpid();
  sprintf(buf,"date >> /tmp/dianalog.%ld",pid);
  system(buf);

     a=__builtin_return_address(0);
     sprintf(buf," echo %p>> /tmp/dianalog.%ld",a,pid);
     system(buf);
fprintf(stderr,"%s\n",buf);
     if(a == 0) return;

     a=__builtin_return_address(1);
     sprintf(buf," echo %p>> /tmp/dianalog.%ld",a,pid);
     system(buf);
fprintf(stderr,"%s\n",buf);
     if(a == 0) return;

     a=__builtin_return_address(2);
     sprintf(buf," echo %p>> /tmp/dianalog.%ld",a,pid);
     system(buf);
fprintf(stderr,"%s\n",buf);
     if(a == 0) return;

     a=__builtin_return_address(3);
     sprintf(buf," echo %p>> /tmp/dianalog.%ld",a,pid);
     system(buf);
fprintf(stderr,"%s\n",buf);
     if(a == 0) return;
}
#endif

/*Destructor -- frees all about tables, file tt_load.c:*/
void tt_load_done(void);

/* Todo:
  Expects "path" to be semicolon-separated list of dirs.
  Re-creates path in such a manner:
  1) removes multi '/',':' by a single chars;
  2) adds trailing slashes to the end of each path entry.*/
char *correct_trailing_slash(char *path)
{
int i;
char *tmp;
  if(path == NULL) return NULL;
  if( (i=s_len(path)) == 0 ) return path;
  if(path[i-1]=='/') return path;
  s_let(path,tmp=get_mem(i+2,sizeof(char)))[i]='/';
  free(path);
  return tmp;
}/*correct_trailing_slash */

void first_init(void)
{
int i;
    { /* Determine mow many bits are contained by an integer:*/
       unsigned int j;
       for(g_bits_in_int=0,j=1;j;j<<=1,g_bits_in_int++);
       g_bits_in_int_1=g_bits_in_int-1;
    }
/*The following code is GNU-specific!:*/
#ifdef STACK_DUMP_ON_CRASH
signal(SIGSEGV,l_handle_sigsegv);
#endif

    for(i=0; i<32; i++){
       switch(1l << i){/*All non-NULL .par's are assumed to be significant:*/
          case STICKY_JOB_T:
             g_execattr.par[i]=new_str("");
             break;
          case ERRORLEVEL_JOB_T:
             g_execattr.par[i]=new_str(ERRORLEVEL_JOB_D);
             break;
          case RESTART_JOB_T:
             g_execattr.par[i]=new_str(RESTART_JOB_D);
             break;
          default:/*NULL means no parameters needed:*/
             g_execattr.par[i]=NULL;
       }/*switch(1l << i)*/
    }/*for(i=0; i<33; i++)*/
    g_execattr.attr=0;

    g_zeroVec=new_str(ZERO_VECTOR);
    s_let(DIAGRAM_COMP_DEFAULT,comp_d);
    s_let(PROTOTYPE_COMP_DEFAULT,comp_p);

    template_t=get_mem(1,sizeof(struct template_struct));
    template_t->next=NULL;
    template_t->pattern=new_str(DEFAULT_TEMPLATE_T);

    template_p=get_mem(1,sizeof(struct template_struct));
    template_p->next=NULL;
    template_p->pattern=new_str(DEFAULT_TEMPLATE_P);

    config_name=new_str(CONFIG);
    if(  (system_path=getenv(SYSTEM_PATH_NAME))!=NULL){
       system_path=new_str(system_path);
    }else
       system_path=new_str("");

    system_path=correct_trailing_slash(system_path);

    for(i=0;i<MAX_ARGV;i++)run_argv[i]=NULL;
    s_let(MOMENTUM_ID,momentum_id);
    s_let(VL_COUNTER_ID,vl_counter_id);
    s_let(LM_COUNTER_ID,lm_counter_id);
    s_let(F_COUNTER_ID,f_counter_id);
    s_let(FROM_COUNTER_ID,from_counter_id);
    s_let(TO_COUNTER_ID,to_counter_id);
    s_let(FFLOW_COUNTER_ID,fflow_counter_id);
    set_sub(digits,digits,digits);/* clear digits */
    set_str2set("0123456789",digits);
    set_sub(g_regchars,g_regchars,g_regchars);/* clear regchars*/
    set_str2set("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",g_regchars);

    *p11=p11[1]=p11[2]=11;p11[3]=0;
    *p12=p12[1]=p12[2]=12;p12[3]=0;
    *p13=p13[1]=p13[2]=13;p13[3]=0;
    *p14=p14[1]=p14[2]=14;p14[3]=0;

    p15=get_mem(max_marks_length,sizeof(char*));
    for(i=max_marks_length-2;!(i<0);i--)p15[i]=15;

    p15[max_marks_length-1]='\0';

    *p16=p16[1]=p16[2]=16;p16[3]=0;

    *programname=0;
    {/*Block begin*/
      /*Distribute and fill up array 'tenToThe':*/
      double p=g_bits_in_int/sizeof(int)*sizeof(long);
      /*Now 2^p - 1 is the largest long value*/
         g_maxlonglength=ceil(p/3.321928);/*10^g_maxlonglength > long*/
         /*3.32 = Log_2 (10), so now 10^g_maxlonglength is larger than maximal long*/
         *(tenToThe=get_mem(g_maxlonglength,sizeof(long)))=1;

         for(i=1; i<g_maxlonglength;i++)
            tenToThe[i]=(10l)*tenToThe[i-1];
    }/*Block end*/
    /*Default setting -- do not uset the sign of Dirac propagator directed against
      the fermion number flow:*/
    direction=&direction_noauto;
    /*cnf directive : flip majorana = enable|disable*/
/*    direction=&direction_auto;*/
#ifdef PIPE_CHLD
   /* if compiler fails here, try to change the definition of l_onsigchld, see above:*/
   signal(SIGCHLD,&l_onsigchld);
#endif
}/*first_init*/

void scaner_init(char *name,char comment)
{
   set_of_char spaces,delimiters;
   char j;
      set_sub(spaces,spaces,spaces);/* clear spasces */
      /*assume all ASCII codes <=' ' as spaces:*/
      for (j=0;!(j>' ');j++)set_set(j,spaces);

      /* set separators:*/
        /* 1. set all NOT separators: */
      set_str2set(
      "._0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
         delimiters);

      set_set(0,delimiters); /* 2. add 0 */
      /* 3. inverse d -- now d containes separators and spaces */
      set_not(delimiters,delimiters);
      /* 4. remove spaces from d. That's all. */
      set_sub(delimiters,delimiters,spaces);

      /* 1 -- hash_enable. We start with hashing.*/
      if(sc_init(name,spaces,delimiters,comment,1,ESC_CHAR))
         halt(SCANNERBUSY,NULL);
      EOF_denied=1;/* EOF now denied*/
}/*scaner_init*/

void run_init(int is_end)
{
  char tmp[MAX_STR_LEN];
  int i;
     set_export_var("argc",long2str(tmp,run_argc));

     if(is_end)return;
     for(i=0; i<top_identifiers;i++){
        let2b(text[identifiers[i].i],tmp);
        if(identifiers[i].fc=='f')
           set_export_var(tmp,"f");
        else
           set_export_var(tmp,"c");
     }
}/*run_init*/

extern HASH_TABLE export_table;

void run_clear(void)
{
  char tmp[MAX_STR_LEN];
  int i;
     for(i=0; i<top_identifiers;i++){
        let2b(text[identifiers[i].i],tmp);
        uninstall(tmp,export_table);
     }
     clear_modestack();/*module run.c*/
}/*run_clear*/

void last_done(void)
{
  word i;
    if ( tenToThe!=NULL)free_mem(&tenToThe);
#ifdef PIPE_CHLD
    signal(SIGCHLD,SIG_DFL);/*To avoid unnecessary recursion*/
#endif
    if( g_allpipes!=NULL){
       /*Zombies danger! From this point and up to the end of the task, all opened pipes
         bacome to be zombies:*/
       hash_table_done(g_allpipes);
       g_allpipes=NULL;
    }
    if(g_sighash_table!=NULL){
       hash_table_done(g_sighash_table);
       g_sighash_table=NULL;
    }

    if (all_functions_table!=NULL){
       hash_table_done(all_functions_table);
       all_functions_table=NULL;
    }
    if (all_commuting_table!=NULL){
       hash_table_done(all_commuting_table);
       all_commuting_table=NULL;
    }
    if (vectors_table!=NULL){
       hash_table_done(vectors_table);
       vectors_table=NULL;
    }

    free_mem(&g_wrkmask);
    g_wrkmask_size=0;

    if (g_wrk_vectors_table!=NULL){
       hash_table_done(g_wrk_vectors_table);
       g_wrk_vectors_table=NULL;
    }/*if (g_wrk_vectors_table!=NULL)*/

    if (g_ttTokens_table!=NULL){
       hash_table_done(g_ttTokens_table);
       g_ttTokens_table=NULL;
    }/*if (g_ttTokens_table!=NULL)*/

    if (usertopologies_table!=NULL){
       hash_table_done(usertopologies_table);
       usertopologies_table=NULL;
    }/*if (usertopologies_table!=NULL)*/

    if (nousertopologies_table!=NULL){
       hash_table_done(nousertopologies_table);
       nousertopologies_table=NULL;
    }/*if (nousertopologies_table!=NULL)*/

    if (topology_id_table!=NULL){
       hash_table_done(topology_id_table);
       topology_id_table=NULL;
    }/*if (topology_id_table!=NULL)*/

    if (main_id_table!=NULL){
       hash_table_done(main_id_table);
       main_id_table=NULL;
    }/*if (main_id_table!=NULL)*/

    if (vec_group_table!=NULL){
       hash_table_done(vec_group_table);
       vec_group_table=NULL;
    }/*if (vec_group_table!=NULL)*/

    if (prototype_table!=NULL){
       hash_table_done(prototype_table);
       prototype_table=NULL;
    }/*if (prototype_table!=NULL)*/

    free_mem(&p15);
    free_mem(&config_name);
    free_mem(&system_path);
    free_mem(&input_name);
    close_file(&log_file);
    free_mem(&log_name);

    free_mem(&dmask);
    if(dtable!=NULL)
       clear_table(&dbuf,&dtable,MAX_VERTEX);
    if(wrktable!=NULL)
       clear_table(&wrkbuf,&wrktable,MAX_VERTEX);

    free_mem(&l_outPos);
    free_mem(&v_outPos);

    if(new_topol!=NULL)
       free_mem(&new_topol);
    if(old_topol!=NULL)
       free_mem(&old_topol);
    free_mem(&l_subst);
    free_mem(&il_subst);
    free_mem(&lt_subst);
    free_mem(&vt_subst);
    free_mem(&v_subst);
    free_mem(&iv_subst);
    free_mem(&l_dir);
    free_mem(&diagram);
    if(prototypes!=NULL)
      for(i=0;i<top_prototype;i++)
         free_mem(&(prototypes[i]));
    free_mem(&prototypes);

    free_mem(&momenta);

    if(text!=NULL){
      for(i=0; i<top_text;i++)
         free_mem(&(text[i]));
      free_mem(&text);
    }

    if(formout!=NULL){
       for(i=0; i<top_formout;i++)
           free_mem(&(formout[i]));
       top_formout=max_top_formout=0;
       free_mem(&formout);
    }
    free_mem(&includeparticle);
    free_mem(&vertices);
    free_mem(&output);
    if(message_enable){/*output user time*/
       struct rusage rusage;
       if(getrusage(RUSAGE_SELF,&rusage)==0){
          /*Success, output the time*/
          double delta_time= (double)rusage.ru_utime.tv_usec;
          delta_time=delta_time/1000000.0+(double)(rusage.ru_utime.tv_sec);
          fprintf(stderr,ELAPSEDTIME,delta_time);
       }/*if(getrusage(RUSAGE_SELF,&rusage)==0)*/
       /*else -- just ignore*/
    }/*if(message_enable)*/
    clear_lvmarks();
    free_mem( &full_command_line);
    tt_load_done();
    free_mem(&g_tt_full);
    free_mem(&g_tt_int);

    if(max_top_vec_id!=0){
       free_mem(&vec_id);/*Note, the contents of vec_id belongs th the hash table
                           vectors_table and can't be deleted here!*/
       top_vec_id=max_top_vec_id=0;
    }/*if(max_top_vec_id!=0)*/
    free_mem(&g_ext_coords_ev);
    free_mem(&g_ext_coords_evl);
    free_mem(&g_ext_coords_el);
    free_mem(&g_ext_coords_ell);
    if(g_loopmomenta!=NULL){
       for(i=0; i<g_nloop; i++)
          free(g_loopmomenta[i]);
       free_mem(&g_loopmomenta);
       g_nloop=0;
    }/*if(g_loopmomenta!=NULL)*/

    free_mem(&g_loopmomenta_r);
    free_mem(&g_zeromomentum);

    free_mem(&g_zeroVec);
    free_mem(&g_bridge_subst);
    free_mem(&g_chord_subst);
    for(i=1; i<32; i++)
       free_mem(g_execattr.par+i);
    free_mem(&g_lastjobname);
    free_mem(&g_rlocalsock);
    free_mem(&g_wlocalsock);

    free_mem(&g_vectorsInTheModel);
    g_topVectorsInTheModel=0;

    clear_jobinfo();
}/*last_done*/
