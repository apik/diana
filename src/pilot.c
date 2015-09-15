/*
    This file is part of DIANA program (DIagram ANAlyser) $Revision: 2.37 $.
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
#include "tools.h"
#define MAIN_MODULE
#include "comdef.h"
#include "types.h"
#include "variabls.h"
#include "headers.h"
#include "texts.h"
#include "tt_type.h"

#ifdef MTRACE_DEBUG
#include <mcheck.h>
#endif

/*Output message after each STEP_INDICATOR diagrams:*/
#define STEP_INDICATOR 100

/*These two lines will keep the  array of the translated main TML program:*/
long number_of_trunslated_lines=0;
char **array_of_trunslated_lines=NULL;
/*They can't be static since they are used in truns.c*/

/*These two lines are used to temporarily keep the array of the translated
  TML function. They can NOT be a static: if fail, the truns module must clean up
  translated array. But these variables are used to keep pointers to array!
  Addresses of these variables are used in truns.c::ttop_out, truns.c::out_str!!
 */
static long current_number_of_trunslated_lines=0;
static char **current_array_of_trunslated_lines=NULL;

word i;

static char currentrevision[]=CURRENTVERSION;
char *currentRevisionNumber=currentrevision;

/*To remove restriction on the order of fermionic interaction look for
FERMIONICORDERNOT2 in the file cnf_read.c*/

/*
 ***** CREATING TABLES ********************
   1. The program reads qlist and creates dtable[vertex][line]
   vertex 1..vcount, line -ext ...-1 1 ext_lines
   (create_primary_diagram_table, read_inp.c).
   2. The program reduces external lines in dtable according to the fixed
   topology part (define_topology, read_inp.c).
   3. The program reads resulting topology (define_topology, read_inp.c).
   4. The program reduces resulting topology, and copies dtable into wrktable
      with proper permutations. Then it sorts multilines in wrktable
      and copies wrktable back to dtable (create_diagram_table, read_inp.c).
       *** So, in 'dtable' we have red topology, lines are directed
       *** down (from smaller to bigger numbers)

   5. Further in wrktable we will store a proper text output array.
*/
/******  BUILDING OUTPUT ***********/
/*
   Momenta are stored in the topology according to REDUCED topology!

   There are 2 steps:
     The first one is performed before invoking an interpreter (the function
        build_form_input() is invoked from main() and described in the module
        read_inp.c) while
     the second one is performed by the operator \createinput() by means of
        the function maccreateinput() from the file macro.c
   build_form_input() is invoked before run. It builds an array of the FORM terms
   in proper order without any concretisations like momenta, fnum and so on.

   build_form_input():
      we create the array 'output' containig FORM expression for vertices/lines in
      correct order. External lines are NOT contained by this array. fnum are set
      at this point; fflow are still unspecified ( will be specified in
      maccreateinput() ).
      l_outPos, v_outPos :will contain the positions in the array  'output'. Zero
      element nothing but the length.

      MAJORANA fermions:
      Elements of the array 'output' contains a field int majorana, which is set in
      build_form_input() as follows: '0',-1,+1 depending on co-incide fermion number
      flow with the fermion flow (+1), opposit (-1), or undefined (0), For vertices it
      will be -1, if at least one line has (-1). Actual value of this field may be
      obtained via 'output' array using l_outPos or v_outPos, see above. Keyword
      'fflow' is expanded into this value.
         'majorana' field is present into external particles structure, too. The same
      meaning. Access: for external leg i<0 we can use:
      ext_particles[-i].majorana
         in 2.30 - fields 'fflow' are added to elements of the array 'output' and
      to the external particles structure. The field is undefined for 'output' elements
      corresponding to vertices, and is +1 for all propagators/external legs,
      except majorana. For majorana particles this field is +1, if the direction of
      the propagator in a topology coincides with the accepted ferniom number flow,
      or -1, if it is opposite.
         This field is used to set correct momentum sign for majorana particles.

   maccreateinput():
      define_momenta() (read_inp.c) -- assignes pointer momenta
      to current momenta set form current topology, see The substitutions map from
      variabls.h;

      Further {} retaltes to g_flip_momenta_on_lsubst!=0. The latter is controlled
      by the config. file directive "flip momenta = enable|disable",
      enable stays for g_flip_momenta_on_lsubst=1:

      create_line_text(red)  (utils.c) -- builds textual momenta for line
         using the sign {l_dir[ ]*}direction( ), i.e. it restores momenta agains l_dir
         (if g_flip_momenta_on_lsubst !=0)
         and changes signes according to direction(). The latter is the pointer to one
         of two functions from utils.c ( depending on the configuration directive
         flip majorana = enable|disable):
            direction_noauto:
               Returns 1, if direction of fermion coinside with direction of line l.
               If it opposite, the function returns -1, If tadpole, return 1.
               For Majorana particles it returns 1, if the direction of the propagator
               in 'red' topology corresponds to the selected fflow, othervice, -1.
            direction_auto:
               Returns 1, if direction of fermion coinside with direction of line l AND
               the fermion number flow is +1, i.e., positive dirac direction coincides with
               topological one. Otherwise, returns -1, If tadpole, return 1.
               For Majorana particles it returns 1, if the direction of the propagator
               in 'red' topology corresponds to the selected fflow, othervice, -1.

      Then all remaining keywords are replaced by the actual values (num, marks, fromvertex,
      tovertex)

      create_vertex_text(red) -- builds textual momenta for vertex using the sign
                {l_dir[ ]*}(+-1)
                 ^^^^^^^^  ^^^
      for internal lines   depending on is_ingoing field of the array vertices[v][l].
      This array is filled up in the function create_indices_table() from read_inp.c.
      The vertices[v][l].is_ingoing == 0|1 according to the TOPOLOGY reduced); l is
      the index of lines coincident to the vertex v and counts from 0 to the total number
      of lines coincident this vertex.

   Further:
      Operator \momentum(line,vec) returns momentum on line l corresponding to
         vector vec. If vec is empty, returns full momentum. The direction relates
         to the red topology, i.e. {l_dir[l]*}direction(l) to get a correct sign
         for a fermionic propagator.

         The text returned by the \momentum() operator is the same as produced by
         create_line_text().

         For external legs the direction of momenta for fermions will be set considering
         fermion number flow.

      For drawing lines the function ndir(red) (macro.c) is used, for internal lines
         it returns 0, if direction of the line  in red topology coinsides with
         direction of the same line in nusr topology, otherwise, -1. For external:
         returns 0, if ingoing, otherwise, -1.
         Operator \vectorOnLine(line,vec) returns momentum on line l corresponding to
         vector vec. If vec is empty, returns full momentum. Returned value is directed
         along the nusr line.

   SYNOPSIS momenta directions, {} retaltes to g_flip_momenta_on_lsubst!=0:
      creat e_line_text:    {l_dir[ ]}*direction()
      creat e_vertex_text: {l_dir[ ]}*(vertices[v][l].is_ingoing)?1:(-1)
      macmomentum:         {l_dir[ ]}*direction()
      vectorOnLine         {l_dir[ ]}*topologies[ ].l_dir[ ]*l_dir[ ]
      ndir:                l_dir[ ]
*/

extern tt_table_type **tt_table;

int main(int argc, char *argv[])
{
int callrun;

#ifdef MTRACE_DEBUG
mtrace();
#endif
   /* Start initialisation:*/
   /* Pick out the number from the string "$Revision: 2.37 $":*/
   {/*Block begin*/
        char *tmp=currentrevision;
        for( tmp=currentrevision;(*tmp!=' ')&&(*tmp!='\0');tmp++);
        if(*tmp == ' ') tmp++;
        for( currentRevisionNumber=tmp;(*tmp!=' ')&&(*tmp!='\0');tmp++);
        *tmp='\0';
   }/*Block end*/

   /* Check here is there the request for the version information.
    * If so, output the revision number and quit, if there are no any
    * other option:
    */
   for(i=1; i<argc; i++)
      if( (s_cmp(argv[i],"-V")==0)||(s_cmp(argv[i],"-version")==0)){
        printf("%s\n",currentRevisionNumber);
        if(argc == 2) exit(0);
      }

   first_init();/*module init.c*/
   read_command_line(argc, argv);/*module cmd_line.c*/
   message(INITIALIZATION,NULL);
   /*End initialisation*/
   message(READINGCONFIG,config_name);
   scaner_init(config_name,cnf_comment);/*module init.c*/
   read_config_file_before_truns();/*module cnf_read.c*/
   if( is_bit_set(&mode,bitCHECKMOMENTABALANCE)&&
       (!is_bit_set(&mode,bitONLYINTERPRET))
    ){

       check_momenta_balance_on_each_topology();
   }
   message(DONE,NULL);

   /*Translation of the TM program:*/
   {char tmp[MAX_STR_LEN];

       if (
             (is_bit_set(&mode,bitONLYINTERPRET))||
             (!is_bit_set(&mode,bitBROWS))||
             (is_bit_set(&mode,bitVERIFY))||
             (is_bit_set(&mode,bitFORCEDTRUNSLATION))
          ){
             command_init();/*module truns.c*/
             if(!is_bit_set(&mode,bitONLYINTERPRET)){
                if (set_table==NULL)set_table=create_hash_table(
                      set_hash_size,str_hash,str_cmp,c_destructor);
                if(is_bit_set(&mode,bitBROWS))
                  install(new_str("_specmode"),new_str("browser"),set_table);
                else
                  install(new_str("_specmode"),new_str("common"),set_table);
             }
             message(BEGINTRUNS,NULL);
             do{
               if(truns(&current_number_of_trunslated_lines,
                           &current_array_of_trunslated_lines)){
                  number_of_trunslated_lines=
                                        current_number_of_trunslated_lines;
                  current_number_of_trunslated_lines=0;
                  array_of_trunslated_lines=
                                         current_array_of_trunslated_lines;
                  current_array_of_trunslated_lines=NULL;
               }
             }while(number_of_trunslated_lines==0);
             message(ENDTRUNS,NULL);
             mem_esc_char=esc_char;
             macro_done();/*module macro.c*/
             reject_proc();/*module truns.c*/
             command_done();/*module truns.c*/
             clear_defs();/*module truns.c*/
             clear_label_stack();/*module truns.c*/
             for_done();/*module truns.c*/
             IF_done();/*module truns.c*/
             if(s_scmp(sc_get_token(tmp),"translate"))
                halt(UNEXPECTED ,tmp);
       }
   }
   sc_done();/*module tools.c*/
   /*TM program is translated. The resulting code is in array_of_trunslated_lines*/

/*Keep the table up to the end of the program!:*/
#ifdef SKIP
   if (vectors_table!=NULL){
      hash_table_done(vectors_table);/*module hash.c*/
      vectors_table=NULL;
   }
#endif

   message(DONEINIT,NULL);

   if (is_bit_set(&mode,bitVERIFY)){
     message(ONLYVERIFY,NULL);
     errorlevel=0;halt(NOERROR,NULL);
   }

   if (g_ttnames != NULL){/*Topology tables were defined in the config file*/
      HASH_TABLE wrktrans;
      message(LOADINGTABLES,NULL);
      read_topology_tables();/*module utils.c*/
      free_mem(&g_ttnames);
      /*Create the working table:*/
      g_tt_wrk=tt_createNewTable(g_defaultTokenDictSize,g_defaultHashTableSize);

      /*Check success, if not, halt the system:*/
      checkTT(g_tt_wrk,FAILCREATINGWRKTABLE);

      /*Collect translations from all tables:*/
      wrktrans=tt_table[g_tt_wrk-1]->htrans;
      /*Full toplogies:*/
      for(i=0;i<g_tt_top_full;i++){
         HASH_TABLE  ttrans=tt_table[g_tt_full[i]-1]->htrans;
         if( ttrans!=NULL )
              binary_operations_under_hash_table(
                 ttrans,/*"from"*/
                 wrktrans,/*"to"*/
                 -1);/*see "hash.c"; op>0 -> add enties to "to" from "from", only if they
                 are absent in "to", op<0 -> rewrite entries in "to" by the corresponding
                 "from", op==0 -> remove all entries in "to" matching "from"*/
      }/*for(i=0;i<g_tt_top_full;i++)*/
      /*Internal topologies:*/
      for(i=0;i<g_tt_top_int;i++){
         HASH_TABLE  ttrans=tt_table[g_tt_int[i]-1]->htrans;
         if( ttrans!=NULL )
              binary_operations_under_hash_table(
                 ttrans,/*"from"*/
                 wrktrans,/*"to"*/
                 -1);/*see "hash.c"; op>0 -> add enties to "to" from "from", only if they
                 are absent in "to", op<0 -> rewrite entries in "to" by the corresponding
                 "from", op==0 -> remove all entries in "to" matching "from"*/
      }/*for(i=0;i<g_tt_top_int;i++)*/

      /*Override translations, if present, in wrk table; in loaded tables
        translations remain to be untranslated!:*/
      if( g_ttTokens_table!=NULL )
              binary_operations_under_hash_table(
                 g_ttTokens_table,/*"from"*/
                 wrktrans,/*"to"*/
                 -1);/*see "hash.c"; op>0 -> add enties to "to" from "from", only if they
                 are absent in "to", op<0 -> rewrite entries in "to" by the corresponding
                 "from", op==0 -> remove all entries in "to" matching "from"*/

      /*Now wrk table is ready*/
      /*Override translations, if present, in full toplogies:*/
      if( g_ttTokens_table!=NULL )for(i=0;i<g_tt_top_full;i++){
         HASH_TABLE  ttrans=tt_table[g_tt_full[i]-1]->htrans;
         if( ttrans!=NULL )/*BTW, it CANNOT be a NULL! See, how tt_createNewTableis invoked
                             in the beginning of tt_loadTable, file tt_load.c*/
              binary_operations_under_hash_table(
                 g_ttTokens_table,/*"from"*/
                 ttrans,/*"to"*/
                 -1);/*see "hash.c"; op>0 -> add enties to "to" from "from", only if they
                 are absent in "to", op<0 -> rewrite entries in "to" by the corresponding
                 "from", op==0 -> remove all entries in "to" matching "from"*/
      }/*for(i=0;i<g_tt_top_full;i++)*/
      message(DONE,NULL);
   }/*if (g_ttnames != NULL)*/

   /*If only interpret, we need not analyse diagrams:*/
   if(is_bit_set(&mode,bitONLYINTERPRET)){
    int exitcode;
      run_init(1);/*module init.c*/
      esc_char=mem_esc_char;
      islast=1;
      g_nocurrenttopology=1;/*Similar to islast, but specially for topologies.*/
      message(CALLTOINTERPRETER,NULL);
      exitcode=run(number_of_trunslated_lines,array_of_trunslated_lines);
      message(DONE,NULL);
      {char endmessage[MAX_STR_LEN];
         sprintf(endmessage,EXITCODEFROMINTERPRETER,exitcode);
         errorlevel=exitcode;
         halt(endmessage,NULL);
      }
      /*Stop running...*/
   }/*if(is_bit_set(&mode,bitONLYINTERPRET)*/

   /*ok, if we are here then we have to read diagrams...*/
   detect_last_diagram_number();/*module last_num.c*/
   message(LOOKINGFORFIRRSTDIAGRAM,NULL);
   scaner_init(input_name,0);/*module init.c*/
   skip_until_first_actual_diagram();/*module read_inp.c*/
   create_table(&dbuf,&dtable,MAX_VERTEX,MAX_I_LINE,ext_lines);/*module utils.c*/
   create_table(&wrkbuf,&wrktable,MAX_VERTEX,MAX_I_LINE,ext_lines);
   message(DONE,NULL);
   message(READDIAGRAMS,NULL);

   for(i=start_diagram;!(i>finish_diagram);i++){/*begin main loop*/
      skip_to_new_diagram();/* module read_inp.c Reads until '['*/

      if(i % STEP_INDICATOR == 0){
        char mes[MAX_STR_LEN];
          sprintf(mes,INDICATE_MESSAGE,i,finish_diagram);
          message(mes,NULL);
      }/*if(i % STEP_INDICATOR == 0)*/

      /* Check should we process this diagram:*/
      if (set_in(i % 250,dmask[i / 250])) continue;

      new_diagram();/*module read_inp.c. Some initializations
                                                       before each diagram.*/

      detect_number_and_coefficient();/*module read_inp.c
                                        This function must receive from
                                        scaner exactly 'd###'*/
      if(i!=diagram_count)
         halt(CURRUPTEDINPUT,input_name);
      create_primary_diagram_table();/*module read_inp.c*/

      if ( skiptadpoles && thisistadpole )
         continue;
      define_topology();/*module read_inp.c*/
      /*compare_topology() returns 1 if topology is not actual.
      If topology is undefined, this procedure will call
      'out_undefined_topology();' (if bitBROWS is not set) and set bitTERROR.
      */
      if(compare_topology())/*module read_inp.c*/
         continue;

      create_diagram_table();/*module read_inp.c*/

      define_prototype();/*module read_inp.c*/
      if(!is_bit_set(&mode,bitFORCEDTRUNSLATION))
          if(skip_prototype())/*module utils.c*/
            continue;
      create_indices_table();/*module read_inp.c*/
      if(must_diagram_be_skipped())/*module utils.c*/
         continue;
      if (is_bit_set(&mode,bitBROWS))
         fill_up_diagram_array(count);/*module read_inp.c*/
      count++;
      if(is_bit_set(&mode,bitFORCEDTRUNSLATION)){
         if(g_tt_try_loaded>0)/*Automatic momenta distribution mechanism*/
            callrun=1;
         else if(   ( is_bit_set(&mode,bitTERROR) )&&(!is_bit_set(&mode,bitBROWS)))
           callrun=0;
         else
           callrun=1;
      }else{
         callrun=(
              (!is_bit_set(&mode,bitTERROR) )&&
              (!is_bit_set(&mode,bitBROWS) )
         );
      }
      /*Run the interpreter:*/
      if(callrun){
         build_form_input();/*module read_inp.c*/
         run_init(0);/*module init.c*/
         esc_char=mem_esc_char;
         if(run(number_of_trunslated_lines,array_of_trunslated_lines)==HALT)
             set_bit(&mode,bitRUNSIGNAL);
         run_clear();/*module init.c*/
         if(is_bit_set(&mode, bitSKIP)){
            unset_bit(&mode,bitSKIP);
            count--;
         }else if((is_bit_set(&mode,bitFORCEDTRUNSLATION))&&
                            (skip_prototype()/*module utils.c*/)){
            count--;
         }else{
            if(g_topologyLabel!=NULL){/*First occurence, otherwise will be NULL*/
               /*Mark topology as occurred*/
               set_bit(g_topologyLabel,0);
               if(g_tt_try_loaded>0){/*Table were loaded*/

                  if( topologies[cn_topol].orig==NULL )/*Undefined topology*/
                     process_topol_tables(0);/*try to find momenta, read_inp.c*/

                  if(topologies[cn_topol].coordinates_ok==0)/*No coords*/
                     process_topol_tables(1);/*try to find coordinates, read_inp.c*/
                  /*Now current topology is marked, and wrk toable is built.*/
               }else{
                  if( topologies[cn_topol].orig==NULL ){/*Undefined topology*/
                     if( automatic_momenta_group(NULL,cn_topol)==1)
                        /*Now momenta are distributed automatically:*/
                        set_bit(g_topologyLabel,2);
                        /*Bit 0: 0, if topology not occures, or 1;
                          bit 1: 1, if topology was produced form generic, or 0.
                          bit 2: 1, automatic momenta is implemented, or 0
                        */
                  }/*if( topologies[cn_topol].orig==NULL )*/
               }/*if(g_tt_try_loaded>0)...else...*/
            }/*if(g_topologyLabel!=NULL)*/
            output_common_protocol(count);/*module read_inp.c*/
         }/*if(is_bit_set(&mode, bitSKIP)) ... else ... else*/
         if(is_bit_set(&mode,bitRUNSIGNAL)){
            sc_done();
            errorlevel=0;
            halt(RUNSIGNAL,NULL);
         }/*if(is_bit_set(&mode,bitRUNSIGNAL))*/
      }/*if(callrun)*/
   }/*end main loop*/

   sc_done();
   message(DONEDIAGRAMS,NULL);

   /*Check should we run the interpreter once more:*/
   if(is_bit_set(&mode,bitEND)){
      /*To use some of specmode operators during "extra call":*/
      new_diagram();

      run_init(1);
      esc_char=mem_esc_char;
      islast=1;

      g_nocurrenttopology=1;/*Similar to islast, but specially for topologies.*/
      cn_topol=top_topol;/*Indicate that there is no special topology */
      message(EXTRACALL,NULL);
      if(run(number_of_trunslated_lines,array_of_trunslated_lines)==HALT){
         errorlevel=0;
         halt(RUNSIGNAL,NULL);
      }
      message(DONE,NULL);
   }/*if(is_bit_set(&mode,bitEND))*/

   if (is_bit_set(&mode,bitBROWS)){
      message(BROWSERMODEDETECT,NULL);
      out_browse_head(count);/*module browser.c*/
      message(SORTINGDIAGRAMS,NULL);
      sort_diargams(count);/*module browser.c*/
      message(DONE,NULL);
      message(PRINTINGDIAGRAMS,NULL);
      print_diagrams(count);/*module browser.c*/
      message(DONE,NULL);
      sort_prototypes();/*module browser.c*/
      print_prototypes();/*module browser.c*/
      print_all_found_topologies();/*module browser.c*/
      print_not_found_topologies();/*module browser.c*/
      print_undefined_topologies();/*module browser.c*/
   }/*if (is_bit_set(&mode,bitBROWS))*/
   errorlevel=0;
   halt(NOERROR,NULL);
   return(0);/*Actually, the control can't reach this point*/
}/*main*/
