#ifndef NO_DEBUGGER
#include <stdlib.h>
#include <stdio.h>
#include "tools.h"
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"
#include "trunsrun.h"

#define BREAKPOINT_HASH_SIZE 17
#define MAX_BREAKPOINTS 65535

#define DELTA_BREAKPIONTARRAY_SIZE 17

typedef struct{
   long lineNo;
   int fileNo;
   int N;
   char *properties;
} breakpointInfo;

word l_breakpointHash(void *tag, word tsize)
{
return ( (breakpointInfo *)tag )->lineNo % tsize ;
}/*l_breakpointHash*/

int l_breakpointCmp(void *tag1,void *tag2)
{
   if(  ( (breakpointInfo *)tag1 )->lineNo != ( (breakpointInfo *)tag2 )->lineNo  )
      return 0;
   if(  ( (breakpointInfo *)tag1 )->fileNo != ( (breakpointInfo *)tag2 )->fileNo  )
      return 0;
   return 1;
}/*l_breakpointCmp*/

void l_breakpointDone(void **tag, void **cell)
{
   free(( (breakpointInfo *)(*tag) )->properties);
   free_mem(tag);
}/*l_breakpointDone*/

static int l_quote_data(char *str)/*returns length of placed string*/
{
char buf[6];
char *tmp;
   switch(*str){
      case tpVAL:
         printf("<tpVAL>'%s'",quote_char(buf,*(str+1)));
         return 2;
      case tpBOOLEAN:
         switch(*(str+1)){
            case blFALSE:
              printf("<tpBOOL>'<False>'");
              break;
            case blTRUE:
              printf("<tpBOOL>'<True>'");
              break;
            default:
              printf("<tpBOOL>'%s'",quote_char(buf,*(str+1)));
              break;
         }/*switch(*(str+1))*/
         return 2;
      case tpSTRING:
         tmp=str;
         printf("<tpSTR>'");
         str++;
         while(*str!=TERM){
            printf("%s",quote_char(buf,*str++));
         }
         printf("'<TERM>");
         return(str-tmp+1);
#ifdef DEBUG
      default:
         halt("run:unknown type!",NULL);
#endif
   }/*switch(*str)*/
   return(0);
}/*l_quote_data*/

static void l_quote(char *str)
{
   char buf[6];
     if(*str!=PLSIGNAL){/* Regular string*/
         printf("'");
         for(;(*str)!='\0';str++)
            printf("%s",quote_char(buf,*str));
         printf("'");
     }else{/* Polish line*/
           printf("<p>");
           /* Load stack until command and do the command */
           str++;
           do{
              while(*str!=tpCOMMAND)
                    str+=l_quote_data(str);/*Put all data */
              str++;/* skip 'tpCOMMAND'*/
              printf("<tpCOMMAND>");
              switch(*str){
                 case opNOT:
                    printf("<opNOT>");
                    break;
                 case opLT:
                    printf("<opLT>");
                    break;
                 case opLE:
                    printf("<opLE>");
                    break;
                 case opGT:
                    printf("<opGT>");
                    break;
                 case opGE:
                    printf("<opGE>");
                    break;
                 case opEQ:
                    printf("<opEQ>");
                    break;
                 case opNE:
                    printf("<opNE>");
                    break;
                 case opAND:
                    printf("<opAND>");
                    break;
                 case opXOR:
                    printf("<opXOR>");
                    break;
                 case opOR:
                    printf("<opOR>");
                    break;
                 case opISDEF:
                    printf("<opISDEF>");
                    break;
                 case opCAT:
                    printf("<opCAT>");
                    break;
                 case cmIF:
                    printf("<cmIF>");
                    break;
                 case cmELSE:
                    printf("<cmELSE>");
                    break;
                 case cmLOOP:
                    printf("<cmLOOP>");
                    break;
                 case cmWLOOP:
                    printf("<cmWLOOP>");
                    break;
                 case cmOUTLINE:
                    printf("<cmOUTLINE>");
                    break;
                 case cmMACRO:
                    printf("<cmMACRO>'%s",quote_char(buf, *++str));
                    printf("%s'",quote_char(buf, *++str));
                    break;
                 case cmPROC:
                    printf("<cmPROC>'%s",quote_char(buf, *++str));
                    printf("%s'",quote_char(buf, *++str));
                    break;
                 case cmOFFLS:
                    printf("<cmOFFLS>");
                    break;
                 case cmONLS:
                    printf("<cmONLS>");
                    break;
                 case cmOFFTS:
                    printf("<cmOFFTS>");
                    break;
                 case cmONTS:
                    printf("<cmONTS>");
                    break;
                 case cmOFFBL:
                    printf("<cmOFFBL>");
                    break;
                 case cmONBL:
                    printf("<cmONBL>");
                    break;
                 case cmOFFOUT:
                    printf("<cmOFFOUT>");
                    break;
                 case cmONOUT:
                    printf("<cmONOUT>");
                    break;
              }/*switch*/
            }while(*++str);/* Repeat until string exhausted*/
     }/*else()Polish line*/
}/*l_quote*/

static HASH_TABLE l_breakpointInfoTable=NULL;
static int l_breakpointN=0;
static int l_breakpointTopN=0;
static int l_breakpointTotalN=0;
static breakpointInfo **l_breakpoints=NULL;

char *l_findline(char *buf, int len,char *fname, long line)
{
FILE *f;
long i;

   if ( (*fname==0)||(line<1) )
      return NULL ;

    if((f=open_file_follow_system_path(fname,"rt"))==NULL)
        return NULL;

    for(i=0;i!=line;i++)
      if(fgets(buf,len,f)==NULL){
         fclose(f);
         return NULL;
      }

    fclose(f);
    return buf;   
}/*l_findline*/

/*Returns <0 on error, or index in l_breakpoints*/
static int l_setBreakpoint(char *fn,char *breakpointTxt, char *breakpointProps)
{
char *b;
long int l;
int i;
int rcode=0;
char *fname=NULL;
breakpointInfo *brInf=NULL;

   /*Parse breakpointTxt, it must be kind of 'filename:linenumber' :*/
   for(i=0; breakpointTxt[i]!=':'; i++)
      if(breakpointTxt[i]=='\0')
        break;
   if(breakpointTxt[i]!='\0')
      i++;
   else{/*No ':' - current file should be used*/
     if(fn[0]=='\0')/*No current file!*/
        return -1;
     i=0;
   }
   /*Now breakpointTxt+i points to a line number */

   if(breakpointTxt[i]=='\0')
      return -2;

   l=strtol(breakpointTxt+i,&b,10);
   if( b == (breakpointTxt+i) )
      return -3;
   /*Now l is the line number*/

   
   /*Allocate fname and copy:*/
   if(i==0)
     fname=new_str(fn);
   else{
      (fname=get_mem(i,sizeof(char)))[i-1]='\0';
      for(i-=2;!(i<0);i--)
         fname[i]=breakpointTxt[i];
   }

   b=get_mem(MAX_STR_LEN,sizeof(char));

   rcode = -4;/*File not found*/

   /*Try to find the file among used:*/
   for(i=0;i<top_fname;i++)
      if(!s_scmp(macros_f_name[i],fname))
         break;    
   if(i>=top_fname)goto theend;/*not found*/

   for(i=0;s_scmp(macros_f_name[i],fname);i++)
      if(i==top_fname)goto theend;/*not found*/

   rcode = -5;/*Line not found*/

   if( l_findline(b, MAX_STR_LEN,fname, l)==NULL )
      goto theend;/*not found*/

   rcode = -6;/*Too manu breakpoints*/
   if(l_breakpointN+1 >= MAX_BREAKPOINTS)
      goto theend;

   /*No non-fatal errors:*/
   rcode=l_breakpointN;

   brInf=get_mem(1,sizeof(breakpointInfo));

   if(l_breakpointN>=l_breakpointTopN){
      l_breakpoints=realloc(l_breakpoints,
                        (l_breakpointTopN+=DELTA_BREAKPIONTARRAY_SIZE)
                           *sizeof(breakpointInfo*)
                    );
      if(l_breakpoints==NULL)
         halt(NOTMEMORY,NULL);
   }/*if(l_breakpointN>=l_breakpointTopN)*/
   l_breakpoints[l_breakpointN]=brInf;

   brInf->N=l_breakpointN;

   brInf->properties=new_str(breakpointProps);
   brInf->lineNo=l;
   brInf->fileNo=i;

   /*The cell is ready*/
   if(l_breakpointInfoTable==NULL)
      l_breakpointInfoTable=
         create_hash_table( BREAKPOINT_HASH_SIZE,
                           &l_breakpointHash,
                           &l_breakpointCmp,
                           &l_breakpointDone);
   else{/*Is this breakpoint already set?*/
     breakpointInfo *oldBP=lookup(brInf,l_breakpointInfoTable);
     if( oldBP!=NULL ){
        rcode=oldBP->N;/*Return index of previous breakpoint*/
        goto theend;
     }/*if( oldBP!=NULL )*/
   }/*if(l_breakpointInfoTable==NULL)...else*/
   
   install(brInf,brInf,l_breakpointInfoTable);/*Install the cell*/
   brInf=NULL;/*detach*/
   
   l_breakpointTotalN++;
   l_breakpointN++;

   theend:
      free_mem(&fname);
      free_mem(&b);
      if(brInf!=NULL){
         free(brInf->properties);
         free(brInf);
      }/*if(brInf!=NULL)*/
      return rcode;   
}/*l_setBreakpoint*/


static int l_varIterator(void *info, HASH_CELL *m, word it,word ic)
{
printf("'%s' = '%s'\n",(char*)(m->tag),(char*)(m->cell));
return 0;
}/*l_varIterator*/

static int l_skipPause=0;
static char l_pauseBuf[MAX_STR_LEN];
static int l_topPauseBuf=0;
static int l_next=0;

/*tracer - visible only from run.c:*/
int doTrace(long n, char *inp_array[])
{
int c=1;
static long l_lnmem=0;
static char *l_fnam="";
static int l_run_depth=-1;
static int l_col=0;

   if(g_trace_on_really==0){/*check possible conditions*/

      /*First: what about brakepoints?*/
      if(g_debug_offset&&
         (l_breakpointTotalN>0)
        ){
           char *fnam=macros_f_name[inp_array[n][6]-1];
           if( fnam[0]!='\0' ){
                 breakpointInfo bp,*resBp;                 
                 bp.lineNo=decode(inp_array[n], BASE);
                 bp.fileNo=inp_array[n][6]-1;
                 if( (resBp=lookup(&bp,l_breakpointInfoTable))!=NULL )
                    if(
                         (bp.lineNo!=l_lnmem)||
                         (l_fnam!=fnam)
                      ){ 
                          printf("Hit breakpoint %d\n",resBp->N);
                          g_trace_on_really=1;
                       }/*if( (...)||(...)||(...) )*/
           }/*if( macros_f_name[inp_array[n][6]-1][0]='\0' )*/
         }/*if(g_debug_offset&&(l_breakpointTotalN>0))*/
      /*Next?*/
      if(l_next!=0){
         
         if(  
              (  (l_run_depth==-1)||
                 (l_run_depth>=top_save_run)
              )&&
              ( 
                ((macros_f_name[inp_array[n][6]-1])[0]!='\0') &&
                (
                 (decode(inp_array[n], BASE)!=l_lnmem)||
                 ( (macros_f_name[inp_array[n][6]-1])!=l_fnam )
                )
              )
           ){
               g_trace_on_really=1;
               l_next=0;
               l_run_depth=-1;
            }
      }/*if(l_next!=0)*/
   }/*if(g_trace_on_really==0)*/

   if(g_trace_on_really==0)
      return 0;

   if(l_skipPause>0)
      l_skipPause--;
   else {
      if(l_topPauseBuf==0){
        if(n>-1){
           printf("Pline:% ld:",n);
           l_quote(inp_array[n]+g_debug_offset);
           putchar('\n');
           putchar('\n');
           if(g_debug_offset){
              char *fnam= macros_f_name[inp_array[n][6]-1];
              long ln=decode(inp_array[n], BASE);
              int col=(inp_array[n][4]-1)*127+inp_array[n][5]-1;
              if( (*fnam)!='\0' ){
                 l_lnmem=ln;l_fnam=fnam;l_col=col;
              }
              printf("Line:%ld col:%d file:'%s'\n",l_lnmem,l_col,fnam);
              typeFilePosition(l_fnam,l_lnmem,l_col, stdout, NULL);
           }           
        }

        do switch(c=getchar()){
           case 'v':case 'V':
           case 'e':case 'E':
           case 'n':case 'N':
           case 'c':case 'C':
           case 's':case 'S':
           case 'j':case 'J':
           case 'i':case 'I':
           case 'b':case 'B':
              l_pauseBuf[l_topPauseBuf++]=c;
              if(l_topPauseBuf>=MAX_STR_LEN)
                  halt(TOOLONGSTRING,NULL);
              /*no break!*/
           default:
              c=1; /*continue*/
              break;
           case '\n':case EOF:
              c=0; /*break the loop*/
              break;
        }while(c);
      }/*if(l_topPauseBuf==0)*/
      /*Now something is in the buffer, 
        or if it is empty, the user just has hit <enter>:*/
      if(l_topPauseBuf==0)/*The user just has hit <enter>*/
         l_pauseBuf[l_topPauseBuf++]='n';

      switch(l_pauseBuf[--l_topPauseBuf]){
           case 'v':case 'V':
               printf("Local variables:\n");
               hash_foreach(variables_table,NULL,&l_varIterator);
               return 1;
               break;
           case 'e':case 'E':
               printf("Exported (global) variables:\n");
               hash_foreach(export_table,NULL,&l_varIterator);
               return 1;
               break;
           case 'n':case 'N':
               printf("next\n");
               l_next=1;
               g_trace_on_really=0;
               l_run_depth=top_save_run;
               break;
           case 'c':case 'C':
               printf("continue\n");
               g_trace_on_really=0;
               break;
           case 's':case 'S':
               l_next=1;
               g_trace_on_really=0;
               printf("step\n");
               break;
           case 'i':case 'I':
               printf("step by one instruction\n");
               break;
           case 'j':case 'J':
               do{
                  char bf[NUM_STR_LEN];
                  l_skipPause=0;
                  fputs("Enter number: ", stdout);
                  if(fgets(bf,NUM_STR_LEN,stdin)!=NULL)
                     sscanf(bf,"%d",&l_skipPause);
                  if(l_skipPause<0)
                     l_skipPause=0;
               }while(l_skipPause==0);
               printf("jump %d\n",l_skipPause);
               break;
           case 'b':case 'B':
               c=-1;
               if(g_debug_offset)
               do{
                  char bf[MAX_STR_LEN];
                  c=-10;
                  fputs("Enter info in terms filename:linenumber: ", stdout);
                  if(fgets(bf,MAX_STR_LEN,stdin)!=NULL)
                     switch(c=l_setBreakpoint(l_fnam,bf, "r")){
                      case -10:
                      case -1:
                      case -2:
                      case -3:
                         printf("Breakpoint must be kind of 'filename:linenumber'\n");
                         break;
                      case -4:
                         printf("File not found\n");
                         break;
                      case -5:
                         printf(" Line not found\n");
                         break;
                      case -6:
                         printf("Too many breakpoints\n");
                         break;
                     }/*switch(c=l_setBreakpoint(...))*/
               }while(c<0);
               printf("Breakpoint %d\n",c);
               return 1;
      }/*switch(l_pauseBuf[--l_topPauseBuf])*/
   }/*if(l_skipPause>0) ... else*/

   return 0;/*by default only once*/
}/*doTrace*/

/*destructor - visible only from run.c:*/
void debugger_done(void)
{
  if(l_breakpointInfoTable!=NULL){
   hash_table_done(l_breakpointInfoTable);
   l_breakpointInfoTable=NULL;
  }
  if(l_breakpoints!=NULL){
    free_mem(&l_breakpoints);
    l_breakpointN=l_breakpointTopN=l_breakpointTotalN=0;
  }
}/*debugger_done*/

#endif /*#ifndef NO_DEBUGGER*/
