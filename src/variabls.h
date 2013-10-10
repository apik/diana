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
/* VARIABLS.H*/
#ifdef HALT_MODULE
int errorlevel =10;
#else
extern int errorlevel;
#endif

#ifdef MAIN_MODULE
                     
int g_debug_offset=0;/* line/filename info is stored into the beginning of the 
                        polish line. To store debug info, g_debug_offset = 7
                        (4 for encoded line number +1, 2 for the colomns number (segment+1
                        and offset+1 ), 1 for the file number +1)
		      */

/*optionally, switch off sorting by type(according to g_sortByType), cnf_read.c:*/
int g_sortByType=1;

char *g_jobinfovar=NULL;
int g_jobinfovar_d=-1;
char *g_jobinfonam=NULL;
int g_jobinfonam_d=-1;

int g_jobinfofieldslen=-1;

int g_pipeforwait=-1;
int *g_rlocalsock=NULL;
int *g_wlocalsock=NULL;
int g_numberofprocessors=1;
int g_niceoflocalserver=0;
char *g_lastjobname=NULL;
int g_pipetoclient=-1;
unsigned long int g_jobscounter=0;
int g_toserver=-1,g_fromserver=-1;
char *g_zeroVec=NULL;/*Will be assigned in first_init()*/
int g_uniqueRemarksName=1;/*0 if new "name" may rewrite existing one*/
int g_argc=0;
char **g_argv=NULL;
char *full_command_line=NULL;
long *tenToThe=NULL;
int g_maxlonglength=0;/*10^g_maxlonglength > long*/
int g_bits_in_int=0;
int g_bits_in_int_1=0;
int atleast_one_topology_has_coordinates = 0;
int wasrun=0;
int max_include=MAX_INCLUDE;
char cnf_comment=CNF_COMMENT;
char comma_char=COMMA_CHAR;
char mem_esc_char;
int islast=0;
int g_nocurrenttopology=0;
word count=0;
int g_tt_loaded=0;/*Number of loaded topology tables*/
int g_tt_try_loaded=0;/*Number of attempts to load topology tables*/
long g_wrk_counter=0;/*Number of topologies in wrk table used for momenta */
int message_enable=MSG_ENABLE_INIT;
set_of_char digits;
set_of_char g_regchars;
char cn_momentaset=0;
char cn_topologyid=0;
char max_topologyid=0;
char max_momentaset=0;

/* Identifiers for FORM id sustitutions.
 *
 * Initialisation:
 * #define ID "text" in comdef.h;
 * s_let(ID,id) in init.c;
 * token -> id; id->id_replace_table in cnf_read.c;
 * pxx[4]=\xx \xx \xx \0 in init.c
 * id table, array of internal_id_cell, see types.h, the structure internal_id_cell
 * contains a counter for every id named as Nid where id is id head.
 * in read_form_id(),cnf_read.c looking up id, If found, it is replaced by pxx
 * The proper cell of the array id is incremented,id[top_id].Nid++.
 *
 * Processing:
 * maccreateinput(),macro.c look up pxx for vertex AND for lines.
 * It does this only if a proper counter is not 0.
 * If found, it is replaced by actual value.
 * The proper cell of the array id is incremented,id[top_id].Nid++.
 */
char p11[4];/*num*/
char p12[4];/*fcount*/
char p13[4];/*fromvertex*/
char p14[4];/*tovertex*/
char *p15=NULL;/*marks*/
char p16[4];/*fflow*/
/*Returns 1, if direction of fermion coinside with direction of line l.
If it opposite, the function returns -1, If tadpole, return 1:*/
int (*direction)(char l)=NULL;

/******************* begin file names **********************/
char *config_name=NULL;
char *input_name=NULL;
char *log_name=NULL;
char *browser_out_name=NULL;
/******************* end file names **********************/
/******************* begin files **********************/
FILE *log_file=NULL;
FILE *browser_out_file=NULL;
FILE *outfile=NULL;
/******************* end files **********************/

/******************* begin allocatable arrays **********************/
struct identifiers_structure *identifiers=NULL;
word top_identifiers=0;
word max_top_identifiers=0;
/* In general, we have several indices groups.
 * Every group has its own identifier and is numbered starting from 0.
 * All indices are stored in the following array:*/
struct indices_struct **indices=NULL;/*root*/
struct indices_struct **iNdex=NULL;/*current*/
/* So indices[2][3] is just a third index of the second group.
 * The following variable is the total number of the indices group:*/
int numberOfIndicesGroups=-1;

struct ext_part_struct *ext_particles=NULL;/* Numeration from 0!*/
word max_top_ext=0;
int *qgraf_ext=NULL;/* Elements of this array with index given QGRAF to
external particles corresponds to indices of ext_particles*/
char **prototypes=NULL;
word top_prototype=0;
word max_top_prototype=0;
VERTEX *vertices=NULL;
/*is_ingoing field is for topology. For tadpoles it is set properly!
(i.e.+1 for one and -1 for another)*/

int max_vertices=0;
struct output_struct *output=NULL;
word top_out=0;
word max_top_out=0;
/******************* end allocatable arrays **********************/
/******************* begin hash table sizes **********************/
word g_Nallpipes = ALLPIPES_HASH_SIZE;
word def_hash_size=DEF_HASH_SIZE;
word set_hash_size=SET_HASH_SIZE;
word all_functions_hash_size=ALL_FUNCTIONS_HASH_SIZE;
word all_commuting_hash_size=ALL_COMMUTING_HASH_SIZE;
word vectors_hash_size=VECTORS_HASH_SIZE;
word topology_hash_size=TOPOLOGY_HASH_SIZE;
word gtopology_hash_size=GTOPOLOGY_HASH_SIZE;
word prototype_hash_size=PROTOTYPE_HASH_SIZE;
word mainid_hash_size=MAINID_HASH_SIZE;
word vec_group_hash_size=VEC_GROUP_HASH_SIZE;
/******************* end hash table sizes **********************/
/******************* begin hash tables **********************/
HASH_TABLE g_sighash_table=NULL;
HASH_TABLE g_allpipes=NULL;
HASH_TABLE g_ttTokens_table=NULL;
HASH_TABLE set_table=NULL;
HASH_TABLE def_table=NULL;
HASH_TABLE flinks_table=NULL;
HASH_TABLE all_functions_table=NULL;
HASH_TABLE all_commuting_table=NULL;
HASH_TABLE vectors_table=NULL;
HASH_TABLE g_wrk_vectors_table=NULL;
HASH_TABLE nousertopologies_table=NULL;
HASH_TABLE usertopologies_table=NULL;
HASH_TABLE gtopologies_table=NULL;
HASH_TABLE topology_id_table=NULL;
HASH_TABLE gtopology_id_table=NULL;
HASH_TABLE prototype_table=NULL;
HASH_TABLE main_id_table=NULL;
HASH_TABLE vec_group_table=NULL;
HASH_TABLE command_table=NULL;
/******************* end hash tables **********************/
/******************* begin modue CNF_READ.C **********************/
int g_last_ingoing=0;
char **g_vectorsInTheModel=NULL;
int g_topVectorsInTheModel=0;
int g_onlysimpleloopmomenta=ONLY_ELEMENTARY_LOOP_MOMENTA;
int g_max_loop_group=0;
int *g_loopmomenta_r=NULL;
int *g_zeromomentum=NULL;
int **g_loopmomenta=NULL;
int g_nloop=0;
int top_vec_id=0;
int max_top_vec_id=0;
char **vec_id=NULL;
/*
 * l_outPos, v_outPos :will contain the positions in the array
 * "output". Initially initialised by -1 to use them for marking.
 * Zero element nothing but the length:
 */
int *l_outPos=NULL;
int *v_outPos=NULL;
int max_marks_length=MAX_MARKS_LENGTH;
int max_order=INIT_MAX_COUPLING;
word includeparticletop=0;
int waspinclude=0;
int wasvinclude=0;
word *includeparticle=NULL;
int skiptadpoles=0;
char programname[MAX_STR_LEN];
int isdebug=0;
int max_ind_length=0;
int max_vec_length=0;
char **index_id=NULL;
char momentum_id[MAX_MOMENTUM_LEN];
MOMENT *vec_group=NULL;
int top_vec_group=1;
int max_top_vec_group=0;
int farrow=-1;/* ap: from anti- to particle.*/
int l_type=0;/* If defined at least one line type, it will be 1*/
int v_type=0;/* If defined at least one vertex type, it will be 1*/
char vl_counter_id[MAX_INDEXID_LEN];
char lm_counter_id[MAX_INDEXID_LEN];
char f_counter_id[MAX_INDEXID_LEN];
char from_counter_id[MAX_INDEXID_LEN];
char to_counter_id[MAX_INDEXID_LEN];
char fflow_counter_id[MAX_INDEXID_LEN];
/******************* end modue CNF_READ.C **********************/
/******************* begin modue HASHTABS.C **********************/
struct internal_id_cell *id=NULL;
word top_id=0;
word max_top_id=0;
aTOPOL *topologies=NULL;
word top_topol=0;
word max_top_topol=0;

aTOPOL *gtopologies=NULL;
word top_gtopol=0;
word max_top_gtopol=0;

/******************* end modue HASHTABS.C **********************/
/******************* begin modue TRUNS.C **********************/
int g_keep_spaces=0;
char *macro_arg=NULL;
word m_ptr=0;
word m_c_ptr=0;
word max_m_ptr=0;
int usedindex_all=1;
int index_all=1;
/******************* end modue TRUNS.C **********************/
/******************* begin modue RUN.C **********************/
int g_trace_on=0;
int g_trace_on_really=0;
word var_hash_size=VAR_HASH_SIZE;
word export_hash_size=EXPORT_HASH_SIZE;
/******************* end modue RUN.C **********************/
/* Main program variables:*/
int mode=0;
/******************* begin modue TOPOLOGY.C **********************/
char ext_lines=0;
char max_vertex=0;
int restrict_tails=0;
/******************* end modue TOPOLOGY.C **********************/
/******************* begin modue MACRO.C **********************/

EXECATTR g_execattr;
int g_autosubst=0;
char *g_bridge_subst=NULL;
int g_bridge_sign=1;
char *g_chord_subst=NULL;
int g_chord_sign=1;

word g_wasAbstractMomentum=0;/*Number of topologies which have "abstract" momenta*/
int g_ignoreMomenta=0;/*if !=0, all momenta come from a table will be ignored*/
/*Set up by macsaveTopologies if there was at least one "Mtbl":*/
int g_wasMomentaTable=0;
char *g_ext_coords_ev=NULL;
char *g_ext_coords_evl=NULL;
char *g_ext_coords_el=NULL;
char *g_ext_coords_ell=NULL;

int run_argc=0;
char *run_argv[MAX_ARGV];
/******************* end modue MACRO.C **********************/
/******************* begin modue CMD_LINE.C **********************/
int g_daemonize=0;
struct template_struct *template_p=NULL;
struct template_struct *template_t=NULL;
struct list_struct *xi_list_top=NULL;
char comp_d[MAX_COMP_D];
char comp_p[MAX_COMP_P];
word start_diagram=0;
word finish_diagram=0;
/******************* end modue CMD_LINE.C **********************/
/******************* begin modue LAST_NUM.C **********************/
set_of_char *dmask=NULL;
long dmask_size=0;
/******************* end modue LAST_NUM.C **********************/
/******************* begin modue READ_INP.C **********************/
/*left - left multipliers, rigtht - rigtht multipliers:*/
int *g_left_spinors=NULL;
int *g_right_spinors=NULL;

int *g_topologyLabel=NULL;
int thisistadpole=0;
word diagram_count=0;
char coefficient[NUM_STR_LEN];
word **dtable=NULL;
word **dbuf=NULL;
word **wrktable=NULL;
word **wrkbuf=NULL;
int vcount=0;
int int_lines=0;
pTOPOL new_topol=NULL,old_topol=NULL;
char *l_subst=NULL;/*lines substitution*/
char *il_subst=NULL;/*invert lines substitution*/
char *lt_subst=NULL;/*lines substitution -- invert cn_topology*/
char *vt_subst=NULL;/*vertex substitution -- invert cn_topology*/
char *v_subst=NULL;/*vertices substitution*/
char *iv_subst=NULL;/*invert vertices substitution*/
char *l_dir=NULL;/* lines directions*/
word cn_topol=0;
int g_flip_momenta_on_lsubst=FLIP_MOMENTA_ON_LSUBST;
/* The substitutions map:
 * There are 4 topologies:
 * 1)   red  -- reduced topology coincides with the diagram table
 * 2)   usr  -- user defined
 * 3)   nusr -- usr  modified for the current diagram by [lv]substitution
 * 4)   can  -- canonical topology (unique internal part), or just
 *              internal part of the topology set by \setinternaltopology
 * Topologies 1) and 2) are situated in the array "topologies".
 *   1): topologies[cn_topol].topology
 *   2): topologies[cn_topol].orig
 * There are the following substitutions:
 * topologies[cn_topol].l_subst[Vusr]=Vred (lines)
 * topologies[cn_topol].v_subst[Vusr]=Vred (vertices)
 * l_subst[Vred]=Vnusr (lines)
 * v_subst[Vred]=Vnusr (vertices)
 * il_subst[Vnusr]=Vred (lines)
 * iv_subst[Vnusr]=Vred (vertices)
 * lt_subst[Vred]=Vusr  (lines)
 * vt_subst[Vred]=Vusr  (lines)
 * lsubst[Vred]=Vcan (lines)
 * vsubst[Vred]=Vcan (vertices)
 * ltsubst[Vcan]=Vred  (lines)
 * vtsubst[Vcan]=Vred  (vertices)
 * And the following direction descriptors:
 * topologies[cn_topol].l_dir[Vusr]:  usr<->red
 * l_dir[Vred]: ADDITIONAL nusr <-> red set by lsubstitution
 * ldir[Vred]: red<->can
 *
 * The variables lsubst ltsubst vsubst vtsubs ldir are visible only
 * in the module "macro.c".
 *
 * Momenta are stored in the topology according to REDUCED topology!
 * operator \lsubstitution() reorders line numbering and directions. E.g:
 * operators \lsubstitution(2:-1:3:-4:5) acts as follows:
 *   directions of 2 and 4 lines upset;
 *   numbering of lines changes: 1 line becomes 2; 2 becomes 1, 3,4,5 remain to
 *   be untouched.
 *
 * momenta change together with the line directions, so logically momenta are not
 * affected by changing of the direction, e.g.
 *
 *  ------->------  =>  -------<------
 *        p1                  -p1
 *
 * Concerning steps of building of an output, see BUILDING OUTPUT in the beginning of the
 * file "pilot.c" ( just before the function main() ).
 *
 */
struct diagram_struct *diagram=NULL;
word cn_prototype=0;
int **momenta=NULL;
char momenta_top=0;
char **text;
int top_text=0;
int max_top_text=0;
int text_created;
int fermionlines_count;/*Start from 1*/
char ** linemarks=NULL;
char ** vertexmarks=NULL;
int g_tt_wrk=0;/*id of working table*/
/******************* end modue READ_INP.C **********************/
/******************* begin modue BROWSER.C **********************/
char comp_arrange[COMP_ARR_L];/*This string is used to comparing in sort
                                routine*/
/******************* end modue BROWSER.C **********************/
/******************* begin modue MACRO.C **********************/

/*See "The substituions map" in "variabls.h"*/
char lsubst[MAX_I_LINE],vsubst[MAX_I_LINE],ldir[MAX_I_LINE];
char ltsubst[MAX_I_LINE],vtsubst[MAX_I_LINE];

tTOPOL buf_canonical_topology;
tTOPOL buf_internal_canonical_topology;
pTOPOL canonical_topology=NULL;
pTOPOL internal_canonical_topology=NULL;
char **formout=NULL;
int top_formout=0,max_top_formout=0;
/******************* end modue MACRO.C **********************/
/******************* begin modue UTILS.C **********************/
char *system_path=NULL;
int g_tt_top_full=0;
int g_tt_max_top_full=0;
int *g_tt_full=0;
int g_tt_top_int=0;
int g_tt_max_top_int=0;
int *g_tt_int=0;
char *g_ttnames=NULL;
set_of_char *g_wrkmask=NULL;
long g_wrkmask_size=0;

/******************* end modue UTILS.C **********************/
#else
/* Common variables:*/
extern int g_debug_offset;
extern int g_sortByType;
extern char *g_jobinfovar;
extern int g_jobinfovar_d;
extern char *g_jobinfonam;
extern int g_jobinfonam_d;
extern int g_jobinfofieldslen;
extern int g_pipeforwait;
extern int *g_rlocalsock;
extern int *g_wlocalsock;
extern int g_numberofprocessors;
extern int g_niceoflocalserver;
extern char *g_lastjobname;
extern int g_pipetoclient;
extern unsigned long int g_jobscounter;
extern int g_toserver,g_fromserver;
extern char *g_zeroVec;
extern int g_uniqueRemarksName;
extern int g_argc;
extern char **g_argv;
extern char *full_command_line;
extern long *tenToThe;
extern int g_maxlonglength;/*10^g_maxlonglength > long*/
extern int g_bits_in_int;
extern int g_bits_in_int_1;
extern int atleast_one_topology_has_coordinates;
extern int wasrun;
extern int max_include;
extern char cnf_comment;
extern char comma_char;
extern char mem_esc_char;
extern int islast;
extern int g_nocurrenttopology;
extern int g_tt_loaded;
extern int g_tt_try_loaded;
extern long g_wrk_counter;
extern word count;
extern int message_enable;
extern char p11[4];
extern char p12[4];
extern char p13[4];
extern char p14[4];
extern char *p15;
extern char p16[4];
extern int (*direction)(char l);
extern char cn_momentaset;
extern char cn_topologyid;
extern char max_topologyid;
extern char max_momentaset;
extern set_of_char g_regchars;
extern set_of_char digits;
extern char *input_name;
extern char *log_name;
extern char *config_name;
extern FILE *log_file;
extern FILE *browser_out_file;
extern FILE *outfile;
extern struct identifiers_structure  *identifiers;
extern word top_identifiers;
extern word max_top_identifiers;
extern struct indices_struct **indices;
extern struct indices_struct **iNdex;
extern int numberOfIndicesGroups;
extern struct ext_part_struct *ingoing;
extern struct ext_part_struct *ext_particles;
extern word max_top_ext;
extern int *qgraf_ext;
extern word vec_group_hash_size;
extern word g_Nallpipes;
extern word def_hash_size;
extern word set_hash_size;
extern word all_functions_hash_size;
extern word all_commuting_hash_size;
extern word vectors_hash_size;
extern word topology_hash_size;
extern word gtopology_hash_size;
extern word prototype_hash_size;
extern word mainid_hash_size;
extern int g_max_loop_group;
extern int *g_loopmomenta_r;
extern int g_last_ingoing;
extern char **g_vectorsInTheModel;
extern int g_topVectorsInTheModel;
extern int g_onlysimpleloopmomenta;
extern int *g_zeromomentum;
extern int **g_loopmomenta;
extern int g_nloop;
extern int top_vec_id;
extern int max_top_vec_id;
extern char **vec_id;
extern int *l_outPos;
extern int *v_outPos;
extern int max_marks_length;
extern int max_order;
extern word includeparticletop;
extern int waspinclude;
extern int wasvinclude;
extern word *includeparticle;
extern int skiptadpoles;
extern char programname[MAX_STR_LEN];
extern int isdebug;
extern int max_ind_length;
extern int max_vec_length;
extern char **index_id;
extern char momentum_id[MAX_MOMENTUM_LEN];
extern MOMENT *vec_group;
extern int top_vec_group;
extern int max_top_vec_group;
extern int farrow;
extern int l_type;
extern int v_type;
extern char vl_counter_id[MAX_INDEXID_LEN];
extern char lm_counter_id[MAX_INDEXID_LEN];
extern char f_counter_id[MAX_INDEXID_LEN];
extern char from_counter_id[MAX_INDEXID_LEN];
extern char to_counter_id[MAX_INDEXID_LEN];
extern char fflow_counter_id[MAX_INDEXID_LEN];
extern HASH_TABLE flinks_table;
extern HASH_TABLE g_sighash_table;
extern HASH_TABLE g_allpipes;
extern HASH_TABLE g_ttTokens_table;
extern HASH_TABLE set_table;
extern HASH_TABLE def_table;
extern HASH_TABLE vec_group_table;
extern HASH_TABLE all_functions_table;
extern HASH_TABLE all_commuting_table;
extern HASH_TABLE vectors_table;
extern HASH_TABLE g_wrk_vectors_table;
extern HASH_TABLE nousertopologies_table;
extern HASH_TABLE usertopologies_table;
extern HASH_TABLE gtopologies_table;
extern HASH_TABLE topology_id_table;
extern HASH_TABLE gtopology_id_table;
extern HASH_TABLE prototype_table;
extern HASH_TABLE main_id_table;
extern HASH_TABLE prototype_table;
extern HASH_TABLE command_table;
extern struct internal_id_cell *id;
extern word top_id;
extern word max_top_id;
extern aTOPOL *topologies;
extern word top_topol;
extern word max_top_topol;
extern aTOPOL *gtopologies;
extern word top_gtopol;
extern word max_top_gtopol;
extern int g_keep_spaces;
extern char *macro_arg;
extern word m_ptr;
extern word m_c_ptr;
extern word max_m_ptr;
extern int usedindex_all;
extern int index_all;
extern int g_trace_on;
extern int g_trace_on_really;
extern word var_hash_size;
extern word export_hash_size;
extern int mode;
extern char ext_lines;
extern char max_vertex;
extern int restrict_tails;
extern EXECATTR g_execattr;
extern int g_autosubst;
extern char *g_bridge_subst;
extern int g_bridge_sign;
extern char *g_chord_subst;
extern int g_chord_sign;
extern word g_wasAbstractMomentum;
extern int g_ignoreMomenta;
extern int g_wasMomentaTable;
extern char *g_ext_coords_ev;
extern char *g_ext_coords_evl;
extern char *g_ext_coords_el;
extern char *g_ext_coords_ell;
extern char *browser_out_name;
extern int g_daemonize;
extern struct template_struct *template_p;
extern struct template_struct *template_t;
extern struct list_struct *xi_list_top;
extern int run_argc;
extern char *run_argv[MAX_ARGV];
extern char comp_d[MAX_COMP_D];
extern char comp_p[MAX_COMP_P];
extern word start_diagram;
extern word finish_diagram;
extern set_of_char *dmask;
extern long dmask_size;
extern int *g_left_spinors;
extern int *g_right_spinors;
extern int *g_topologyLabel;
extern int thisistadpole;
extern word diagram_count;
extern char coefficient[NUM_STR_LEN];
extern word **dtable;
extern word **dbuf;
extern word **wrktable;
extern word **wrkbuf;
extern int vcount;
extern int int_lines;
extern pTOPOL new_topol,old_topol;
extern char *l_subst;
extern char *il_subst;
extern char *lt_subst;
extern char *vt_subst;
extern char *v_subst;
extern char *iv_subst;
extern char *l_dir;
extern word cn_topol;
extern int g_flip_momenta_on_lsubst;
extern struct diagram_struct *diagram;
extern char **prototypes;
extern word top_prototype;
extern word max_top_prototype;
extern word cn_prototype;
extern int **momenta;
extern char momenta_top;
extern char **text;
extern int top_text;
extern int max_top_text;
/*
extern struct tmp_mom_struct *tmp_mom;
extern word top_tmp_mom;
extern word max_tmp_top_mom;
extern struct vcall_struct *vcall;
extern word top_vcall;
extern word max_top_vcall;
*/
extern VERTEX *vertices;
extern int max_vertices;
extern int text_created;
extern struct output_struct *output;
extern word top_out;
extern word max_top_out;
extern int fermionlines_count;
extern char ** linemarks;
extern char ** vertexmarks;
extern int g_tt_wrk;/*id of working table*/
extern char comp_arrange[COMP_ARR_L];
extern char lsubst[MAX_I_LINE];
extern char vsubst[MAX_I_LINE];
extern char ldir[MAX_I_LINE];
extern char ltsubst[MAX_I_LINE];
extern char vtsubst[MAX_I_LINE];
extern tTOPOL buf_canonical_topology;
extern pTOPOL canonical_topology;
extern tTOPOL buf_internal_canonical_topology;
extern pTOPOL internal_canonical_topology;
extern char **formout;
extern int top_formout,max_top_formout;
extern char *system_path;
extern int g_tt_top_full;
extern int g_tt_max_top_full;
extern int *g_tt_full;
extern int g_tt_top_int;
extern int g_tt_max_top_int;
extern int *g_tt_int;
extern char *g_ttnames;
extern set_of_char *g_wrkmask;
extern long g_wrkmask_size;
#endif
