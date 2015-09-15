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
#include <unistd.h>
#include <stdlib.h>
/*HEADERS.H*/
/******************* begin modue HALT.C **********************/
void halt(char *fmt, ...);
void message(char *fmt, ...);
/******************* end modue HALT.C **********************/
/******************* begin modue UTILS.C **********************/
char *quote_char(char *buf, char ch);

char *int2hex(char *buf, unsigned long int n, int w);

long int  hex2int(char *buf,int w);
/*
   Reads from the file descriptor fd len bytes and tries to convert them into
   unsigned int (assuming they are in HEX format). In success, returns read value,
   on error, returns -1 and sends the message "cmd" of size cmdlen to the fd (if
   cmd!=NULL).
*/
long int readHex(int fd, char *buf, size_t len, char *cmd, size_t cmdlen);

/*The following function transforms string representing SIGNED decimal number into
  the string representing SIGNED haexadecimal number. If w is >0, then it will be used
  as a width of produced HEX number, no error checkup, if w is not enough, only
  last w digits will be placed if w is too large, the leading 0 will be produced;
  if w<1, then the produced width will be defined automatically.
  buf will be used only to the first non-digit character; leading '+' will be ignored.
  If buf is not a number, the empty string will be produced:*/
char *dec2hex(char *decbuf,char *hexbuf,int w);

/*Evaluates number of HEX digits needed to fit the argument:*/
int hexwide(unsigned long int j);

/*Wrapper to the write()) syscall, to handle possible interrupts by unnblocked signals:*/
ssize_t writeFromb(int fd, char *buf, size_t count);

/*Wrapper to the read() syscall, to handle possible interrupts by unnblocked signals:*/
ssize_t read2b(int fd, char *buf, size_t count);

/*Reads exactly count bytes from the descriptor fd into buffer buf, independently on
  nonblocked signals and the MPU/buffer hits. Returns 0 or -1:
  */
int readexactly(int fd, char *buf, size_t count);

/*Wtites exactly count bytes from the  buffer buf intop the descriptor fd, independently on
  nonblocked signals and the MPU/buffer hits. Returns 0 or -1:
*/
int writexactly(int fd, char *buf, size_t count);

/*Converts str to long int, if str is not an int, halts the system:*/
long int str2long(char *str);

/* Compares integer vectors. Returns 0 if NOT coincide, or 1:*/
int int_eq(int *i1, int *i2);

void outPSinit(char *fname);
void outPSend(void);
void outPS(char *ch,MKPSOUT mkPSout);
/*
   buf is input, ret is output.
   ATTENTION! ret is allocated! On error, ret is creared, and then it is set up
   to a static string with a diagnostic.

   ATTENTION! '(' and ')' are ALWAYS escaped!

   All modifications are local agains block. Block is started by:
   {x(#) ... } - paints the content with shift along x. After the block, the current point
                is set to (old x, new y)
   {y(#) ... } - paints the content with shift along y. After the block, the current point
                 is set to (new x, old y)
   {xy(#)(#) ... } -  paints the content with shift along  both x and y. After the block, the current point
                   is set to (old x, old y)
   {f(fontname)(#) ... } set font "fontname" scaled by # (in 1/multiplier fractions
                         of fsize)
   {s(#) ... } - scale current font by # (in 1/multiplier fractions of fsize)
   {c(#)(#)(#) ... }  set RGB color

   show is the PS command to paint a string,  fsize is the base font size.
   All sizes are in 1/multiplier fractions of fsize.
   Returns >=0 ("complexity") - the number of non-trivial blocks.
   If the retirned value <0, then the  parsing error occured and -value is the number of
   a problem character (starting from 0).
 */
int parse_particle_image(char *fsize,char *multiplier,char *show, char *buf,char **ret,char ***fonts);
/*Checks balance of momenta in each vertex of momentaset cn_set in topology cn_top.
  Returns numbes of non-balanced vertices in returnedbuf.
  NO CHECKING!!:*/
char *checkTopMomentaBalance(char *returnedbuf,aTOPOL *topols,word cn_top,int cn_set);
/*
   Parses a string. For each identifier (someting starting with regchars and consists of
   regchars or digits) it allocates a new string identical to this id and invokes
   int process_token(int n, char *thetoken).If the latter returns !=0, parse_tokens_id
   immediately returns the value returned by process_token. Arguments of process_token:
   n -- order number of token, thetoken -- the token. After all tokens are processed,
   parse_tokens_id returns the number of processed tokens.
*/
int parse_tokens_id(
                    set_of_char regchars,
                    set_of_char digits,
                    char *str,
                    PROCESS_TOKEN *process_token
                   );

/*Allocates and returns identical (123etc) or unit (1111etc) substitutions of a length |l|.
  Zero char is -1, trailing '\0' is always added.
  If l<0, performs 1111etc, otherwise, performs 123etc:*/
char *alloc123Or111etc(int l);
/*If ret<0, halts the system with diagnostics msg equipped error text:*/
void checkTT(int ret, char *msg);
/*Returns text corresponding to tt errors:*/
char *tt_error_txt(char *arg,int i);
/* Appends c to the end of s1 with memory rellocation for s1:*/
char *s_addchar(char *s1, char c);
/*Concatenates s1 and s2 with memory rellocation for s1:*/
char *s_inc(char *s1, char *s2);
void read_topology_tables(void);
int getindexes(char *buf, int *moment, int maxLen,int outlen);
/*converts long to a string representation and returns it:*/
char *long2str(char *buf, long n);
/*This function performs execvp(cmd,argv) and swallows  stdin (*send) and stdout
  (*receive) of the executed command. If noblock!=0, then the reading will not be
  blocked:*/
pid_t swallow_cmd(FILE **send, FILE **receive, char *cmd,  char *argv[], int noblock);
void allocate_indices_structures(void);
void allocate_topology_coordinates( aTOPOL *theTopology );
char *invertsubstitution(char *str, char *substitution);

FILE *open_file_follow_system_path(char *name, char *mode);
FILE *open_system_file(char *name);/* attempts open file for reading
  first in the current directory, then in system directory and
   returns pointer to opened file. If fail, halt process*/
int skip_prototype(void);/* Test is current prototype is actual*/
/*Returns index of fermion line continuing line l from vertex v.
If it is impossible, returns 0:*/
char fcont(char l, char v);
/*Returns vertex index of the second end of line l in vertex v:*/
char endline(char l, char v);
/*Returns index of fermion line outgoing from vertex v.
If it is impossible, returns 0:*/
char outfline(char v);
/*Returns vertex index which is a source of internal line l:*/
char fromvertex(char l);
/*Returns vertex index which is end of internal line l:*/
char tovertex(char l);

void create_table(word ***buf,word ***table,int vertex,
                   int internal_lines, int external_lines);
void clear_table(word ***buf,word ***table,int vertex);
/*Verifies matching string to template pattern. Returns 1, if match, or 0:*/
int cmp_template(char *string,char *pattern);
/*Detect type of particle on line l in table table:*/
char get_type(char l,word **table);
/*Detect order number of id of particle on line l in table table:*/
char get_number(char l,word **table);

/* Exchnges a and b elements into arr:*/
void swapch(char* arr, char a, char b);
void swapw(word* arr, int a, int b);

/*Returns 1, if direction of fermion coinside with direction of line l AND
the fermion number flow is +1, i.e., positive dirac direction coincides with
topological one. Otherwise, returns -1, If tadpole, return 1.
For Majorana particles it returns 1:*/
int direction_auto(char l);

/*Returns 1, if direction of fermion coinside with direction of line l.
For Majorana particles it returns 1.
If it opposite, the function returns -1, If tadpole, return 1:*/
int direction_noauto(char l);

/*Returns 1, if direction of propagator coinside with direction of line l.
If it opposite, the function returns -1, If tadpole, return 1:*/
int pdirection(char l);

/* Allocates indices in memory as follows:
 *<n, number of groups><group1><index as m-string> ... <group n><index as m-string>
 * Input array "info" is:
 * <n, number of groups><group1><number of indices> ....
 :*/
char *alloc_indices(int *info);

/*Creates momentum text from 'momentum' according to 'group':*/
char *build_momentum(char *group, int *momentum, char *buf, int sign);
/*Set momenta of form input text in wrktable:*/
void create_vertex_text(char v);
/*Set momenta of form input text in wrktable:*/
void create_line_text(char l);
/*Forms form_id into text[text_ind] according to current fermionlines_count:*/
void set_form_id(word curId,word text_ind);
void addtoidentifiers (word textindex,char fc, char lv);
void out_undefined_topology(void);
/*Performs str[i]=pattern[str[i]];returns 0 if ok, or -1:*/
int substitutel(char *str, char *pattern);
/*Performs str[i]=str[pattern[i]];returns 0 if ok, or -1:*/
int substituter(char *str, char *pattern);
/*Resets prototype according to pattern, returns pointer to prototype:*/
char *p_reoder(char *prototype, char *pattern);
/*Lets from to to till "[","{" ro "(":*/
char *let2b( char *from,char *to);
/*Comparesa and b till "[","{" ro "(":*/
int cmp2b(char *a, char *b);
void unlink_all_files(void);
word unlink_file(char *name);
FILE *link_file(char *name, long pos);
FILE *link_stream(char *name, long pos);
void keep_file(char *name);/*Sets number of links to 0xFFFF.*/
int must_diagram_be_skipped(void);
char *letnv(char *from,char *to, word n);
int initPS(char *header, char *target);
void clear_lvmarks(void);
/*The following function returns the smallest prime number <= n:*/
long nextPrime(long n);

void make_canonical_topology(void);
/*converts long to a string representation, allocaltes a buffer and returns it:*/
char *new_long2str(long n);
/*Saves translated wrk table (if present) into a named file.
  Returns number of saved topologies:*/
long save_wrk_table(char *fname);

/*Loads wrk table from a file. If wrk table exists, deletes it first. All
  loaded vectors are stored into g_wrk_vectors_table. Returns number of
  loaded topologies:*/
int load_wrk_table(char *fname);

/*Concatenates i1 and i2 with memory relocation for i1, if s==0 upset appended i2:*/
int *int_inc(int *i1, int *i2,int s);

/*The following function send the hexadecimal representation
  of an unsigned long integer n into a pipe fd atomically in such a form:
  XXxxxxxxx
  ^^the length
  i.e. dec. 15 will be sent as "01f".
  Problem with this function is that the operation must be atomic.
  For the size of the atomic IO operation, the  POSIX standard
  dictates 512 bytes. Linux PIPE_BUF is quite considerably, 4096.
  Anyway, in all systems we assume PIPE_BUF>20*/
int writeLong(int fd,unsigned long int n);
/******************* end modue UTILS.C ************************/

/***************** begin module TOPOLOGY.C ************************/
pTOPOL newTopol( pTOPOL topol );
char *check_momenta_balance(
   char *returnedbuf,
   pLINE elines,/*external lines*/
   pLINE ilines,/*internal lines*/
   char nel,/*number of external lines*/
   char nil,/*number of  internal lines*/
   char nv,/*number of vertices*/
   char *is_ingoing,/*0 if outgoing, 1 if ingoing*/
   int *zmom,/*zero momentum*/
   int **emom,/*external momenta*/
   int **imom/*internal momenta*/
   );

/* Sorts internal part of the topology: to>from; ordering of pairs:*/
pTOPOL sortinternalpartoftopology (pTOPOL topology);
/*Copies t1 into t2, returns pointer to t2.*/
pTOPOL top_let(pTOPOL t1,pTOPOL t2);
/*Converts topology to string reprezentation:*/
char *top2str(pTOPOL topology, char *str);
/*Reads topology using standart scaner:*/
pTOPOL read_topol(pTOPOL topology,
                  char *l_subst,char *v_subst,char *l_directions);
/* Reduce topology i_topology to canonical form by changing vertices only:*/
int reduce_topology(int first_changed_vertex,pTOPOL i_topology, pTOPOL r_topology,
                    char *l_subst, char *v_subst,char *l_directions);
/* Reduce topology i_topology to canonical form:*/
int reduce_topology_full(pTOPOL i_topology, pTOPOL r_topology,
                    char *l_subst, char *v_subst,char *l_directions);

/* Reduce topology i_topology to canonical second form:*/
int reduce_internal_topology(pTOPOL i_topology, pTOPOL r_topology,
                    char *l_subst, char *v_subst,char *l_directions);
/*Returns value of original user topology:*/
pTOPOL get_orig_topology(pTOPOL topology);
int top_cmp(const void *a,const void *b);/*Used by qsort*/
/*Reads string representation of the topology, returns number of internal
vertex or -1 on error:*/
int string2topology(char *str,pTOPOL topology);
/* Reoders i_topology according to l_s, l_d and v_s, saves it in r_topology
and returns r_topology: r_topology=>subst[i_topology],l_d[i_topology]:*/
pTOPOL reset_topology(pTOPOL i_topology, pTOPOL r_topology,
                    char *l_s, char *v_s,char *l_d);

int distribute_momenta_groups(
   pLINE elines,/*external lines*/
   pLINE ilines,/*internal lines*/
   char nel,/*number of external lines*/
   char nil,/*number of  internal lines*/
   int **mloop,/*loop momenta (starting from 0!)*/
   int nloop,/*number of loop momenta*/
   int *zmom,/*zero momentum*/
   int **emom,/*external momenta (starting from 1!)*/
   int **imom,/*internal momenta. This is RETURNED value. It must be initialised:
               allocated int *imom[nil+1], all NULL.*/
   char *ldirs,/*directions ldirs[usr] usr <-> red, starting from 1*/
   char *mloopMarks/*the array of length nil+1, NOT an ASCII-Z! E.g :
                     -1 0 0 2 0 1 -- means that the first loop momenta must be at the 5
                        1 2 3 4 5    and the second - at 3.
                     This pointer may be NULL, if the order of integration momenta is not
                     a matter.*/
   );
/*Reduces momentum by simplifying groups and (possible) extracting emom.
  If the resulting momentum appears to be 0, replaces it by zmom.
  This function destroys old value of momentum and allocates new one.*/
int *reduce_momentum(
                        int *momentum,
                        int *zmom,/*zero momentum*/
                        int *emom/*Sum of incoming external momenta*/
                    );
/***************** end module TOPOLOGY.C ************************/

/***************** begin module HASH.C ************************/
/*Moved to hash.h*/
/***************** end module HASH.C ************************/

/***************** begin module TRUNS.C ************************/
void typeFilePosition(char *fname, long line, int col, FILE *outstream, FILE *logfile);
void clear_program(void);/*Clears result of translation of main program*/
void clear_sets(void);/*Clears preprocessor settings*/
void IF_done(void);/*Cleans preprocessor IF stack*/
void for_done(void);/*Cleans FOR stack.*/
void command_done(void);/*Clears command hash table*/
void command_init(void);/*Initializes command table*/
void clear_label_stack(void);/*Clears current_labels_stack */
void clear_defs(void);/*Clears macro tabel, if it is not necessary more.*/
void reject_proc(void);/*Call cleanroc() and proc_done, if need.*/
int cleanproc(void);/*Cleans unused procedures.*/
/*Trunslator. Input entering from standart scaner.'top_output_array' is
reference to output string counter, 'output_array' is reference to
array of text strings:*/
int truns(long *top_output_array, char ***output_array);
void truns_done(void);/*destructor*/
/*Macro register function. 'name' - macro mane, 'arg_numb'- number of
arguments, 'flags' - 0 if no flags;  bit 0,if not need debug,
              current labels level - 1, etc(reserved); 'mexp' --
pointer to expansing function. See file "macro.c":*/
void register_macro(char *name,char arg_numb,int flags,MEXPAND *mexp);
/*Types 'line' string from file 'fmane' and marks position 'col':*/
void pointposition(char *fname, long line, int col);
/* Destructor of tables of procedure names:*/
void proc_done(void);
/*Reads list of tokens between brackets. After each argument calls setarg:*/
void read_list(void (*setarg)(void *),void *tmp,char *str,int fullexp);
/***************** end module TRUNS.C ************************/

/***************** begin module RUN.C ************************/
/*Run function: 'inp_array' - array of ASCII-Z strings of length out_len'.
When normal end of work, 'run' returns 0. If it finishes working
by macrofunction 'exit', it  returns integer value, defined by  'exit'.
Variable char *run_msg may containes arbitrary text set by "exit'.*/
int run(long out_len,char *inp_array[]);
void run_done(void); /*Destructor*/
/* Returns 1 if 'id' is present into variables_table:*/
int ifdef(char *id);
/*Sets value 'varval' into run variable 'varname'. Returns
1 if variable with shuch name was already existed:*/
int set_run_var(char *varname, char *varval);
int set_export_var(char *varname, char *varval);
void clear_modestack(void);
/***************** end module RUN.C ************************/

/***************** begin module MACRO.C ************************/
void macro_init(void);/*Constructor*/
void macro_done(void);/*Destructor. Call it after last 'truns()'!*/
/***************** end module MACRO.C ************************/

/***************** begin module CMD_LINE.C ************************/
void read_command_line(int argc, char *argv[]);/*Reads runtime options*/
void run_argv_done(void);/* Free list of runtime cmdline arguments*/
void cmd_line_done(void);/*Destructor*/
/***************** end module CMD_LINE.C ************************/

/***************** begin module INIT.C ************************/
void first_init(void);/*Some setting*/
/* Init scaner:*/
void scaner_init(char *name,char comment);
void run_init(int is_end);/*Run initializations*/
void run_clear(void);/*Clear global definitions*/
void last_done(void);/*destructor for global initialisations*/
/*chech is there the trailing / ; if not, add it:*/
char *correct_trailing_slash(char *path);
/*This function is used to add a swallowed processes with pipes to
 the hash table g_allpipes:
*/
void store_pid(pid_t pid,int fdsend,int fdreceive);
/***************** end module INIT.C ************************/

/***************** begin module HASHTABS.C ************************/
word hash_main_id(void *tag, word tsize);
word alloc_id(void);
void set_id(char *id);/*id -- m-string*/
void empty_destructor(void **tag, void **cell);
int main_id_cmp(void *tag1, void *tag2);
void int_destructor(void **tag, void **cell);
int topology_cmp(void *tag1, void *tag2);
word hash_topology(void *tag, word tsize);
void flinks_destructor(void **tag, void **cell);
void def_destructor(void **tag, void **cell);

/*Expects thesig is either a symbolic SIG... (SIG prefix may be omitted), or a number.
  All possible symbolic names see in comdef.h.
  If it is a symbolic name, the function returns a Diana-specific number; if it is a
  number, the function returns this number. The return buffer is thesig. On error,
  returns NULL. The first  character in the returned string is either '1' (if it was a
  symbolic name) or '0' (if it was a number). Returned numbers ae in HEX format of 2
  digits wide:
 */
char *reduce_signal(char *thesig);
/***************** end module HASHTABS.C ************************/

/***************** begin module CNF_READ.C ************************/
void check_momenta_balance_on_each_topology(void);
void read_config_file_before_truns(void);
void cnf_read_done(void);
int is_digits(char *str);/*returns 1 if all chars are digits.*/
/***************** end module CNF_READ.C ************************/
/***************** begin module LAST_NUM.C ************************/
void detect_last_diagram_number(void);
/***************** end module LAST_NUM.C ************************/
/***************** begin module READ_INP.C ************************/
/*Allocates momenta for current topology:*/
int automatic_momenta_group(
        char *mloopMarks,
        /*the array of length nil+1, NOT an ASCII-Z! E.g :
          -1 0 0 2 0 1 -- means that the first loop momenta must be at the 5
          1 2 3 4 5    and the second - at 3.
          This pointer may be NULL, if the order of integration momenta is not
          a matter.*/
        word cn_top
    );
void process_topol_tables(int mOrC/*momenta 0 coords 1*/);
void skip_until_first_actual_diagram(void);
void skip_to_new_diagram(void);
void detect_number_and_coefficient(void);
void new_diagram(void);
void create_primary_diagram_table(void);
void define_topology(void);
int compare_topology(void);
void create_diagram_table(void);
word set_prototype(char *prot);
void define_prototype(void);
void fill_up_diagram_array(word count);
void output_common_protocol(word count);
void define_momenta(void);
void create_indices_table(void);
void build_form_input(void);
char *get_undefinedtopologyid(char *, char *, HASH_TABLE *, word, char *, word );
word *newword( word);
/***************** end module READ_INP.C ************************/
/***************** begin module BROWSER.C ************************/
void out_browse_head(word count);
void browser_done(void);
void sort_diargams(word count);
void print_diagrams(word count);
void sort_prototypes(void);
void print_prototypes(void);
void print_all_found_topologies(void);
void print_not_found_topologies(void);
void print_undefined_topologies(void);
/***************** end module BROWSER.C ************************/
/***************** begin module INT2SIG.C **********************/
int sig2int(int thesignal);
/*Converts system diana's internal signals to system ones:*/
int int2sig(int thesignal);
/***************** end module INT2SIG.C **********************/
/***************** begin module REXEC.C **********************/
pid_t startclient(int *p);
pid_t startserver(int n, int thenice);

/*1 if ailve, otherwise, 0:*/
int isServerOk(char *ip,unsigned short int theport, char *passwd);
/*Wrapper to isServerOk, 1 if alive, otherwise, 0:*/
int pingSrv(char *ip);
/*1 on success, if it can't connect with the server, 0:*/
int kill_server(char *ip,unsigned short int theport, char *passwd);
/*Wrapper to kill_server, 1 on success, if it can't connect with the server, 0:*/
int killSrv(char *ip);
char *defip(char *buf, size_t len);
int activateClient(int sig);

int runlocalservers(void);

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
   int ttymode,/*0 - nothing, &1 - reopen stdin &2 - reopen stdout &4 - daemonizeing*/
   char *cmd,
   char *argv[]
   );

void clear_jobinfo(void);
