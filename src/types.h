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
/*TYPES.H*/
#include <unistd.h>
#include "hash.h"
#include "remarks.h"

typedef void (*mysighandler_t)(int);
/* Sometimes, this paranoya may occurs:*/
/*typedef int (*mysighandler_t)(int);*/

typedef struct execattr_struct{
   unsigned long int attr;
   char *par[32];/*NULL means no par, "" menas default*/
} EXECATTR;

typedef void (*MKPSOUT)(char);

struct identifiers_structure{
   word i;/*index in "text" array*/
   char fc;/*"f" if function, "r" if commuting*/
   char lv;/*"l" if line; "v" if vertex*/
};

struct output_struct{
  word i;/*index of "text" array*/
  char t;/*'v' or 'l' -- vertex or line*/
  char pos;/*position in wrk_table*/
  int fcount;/*value of current fermionlines_count*/
  int fflow;/*1 fflow coincides with red; -1 - opposit.*/
  int majorana;/*'0',-1,+1 depending on co-incide fermion number flow with
                the fermion flow (+1), opposit (-1), or undefined (0), For vertices it
                will be -1, if at least one line has (-1).
                */
};
/******************* begin modue TOPOLOGY.C **********************/
struct line_struct{
   char from,to;
};
struct topology_struct{
   struct line_struct i_line[MAX_I_LINE+1];
   char i_n;/*number of internal lines*/
   struct line_struct e_line[MAX_E_LINE+1];
   char e_n;/*number of external lines*/
   /* Structure of external lines:(-1,1) is first, (-e_n, ...?) is last*/
};
typedef struct line_struct *pLINE;
typedef struct line_struct tLINE;
typedef struct topology_struct *pTOPOL;
typedef struct topology_struct tTOPOL;
/******************* end modue TOPOLOGY.C **********************/
/******************* begin modue HASH.C **********************/
/* Moved to hash.h */
/******************* end modue HASH.C **********************/

/******************* begin modue TRUNS.C **********************/

struct pexpand_struct{
    int arg_numb;
    char **out_str;
    long top_out;
    char **var;
    char wascall;
    HASH_TABLE labels_tables;
};
typedef struct pexpand_struct PEXPAND;

typedef char *MEXPAND(char *str);/*expand macros*/

struct def_struct{
  char *filename;
  int top;
  int maxtop;
  char **str;
  long offset;
  long line;
  int col;
};
/******************* end modue TRUNS.C **********************/

/******************* begin modue RUN.C **********************/
struct save_run_struct{
    word stacktop;
    HASH_TABLE variables_table;
    HASH_TABLE labels_tables;
};
/******************* end modue RUN.C **********************/

/******************* begin modue CMD_LINE.C **********************/
struct list_struct{
  struct list_struct *next;
  long from,to;
  char xi;
};

struct template_struct{
  struct template_struct *next;
  char *pattern;
};

/******************* end modue CMD_LINE.C **********************/
/******************* begin modue HASHTABS.C **********************/
struct internal_id_cell{
   word link[MAX_LINES_IN_VERTEX];/*for particles:
             id second particle,[0]
             id propagator;[1]
             for propagator:
             id first particle,[0]
             id second particle;[1]
             for vertex:
             id first particle,[0]
              . . .
             id last particle[MAX-1]*/
   word number;/* Number in the array*/
   char kind;/* for single particle:
              0 - particle is equal to antiparticle;
              1 - begin in propagator, 2 - end of propagator;

              Otherwice:
              -1 function (fermion), +1- commuting (boson);
              */
   char Nnum;/*Number of keywords "ind" in FORM id*/
   char Nmark;/*Number of keywords "mnum" in FORM id*/
   char Nind;/*Number of keywords "ind" in FORM id*/
   char Nfnum;/*Number of keywords "fnum" in FORM id*/
   char Nfflow;/*Number of keywords "fflow" in FORM id*/
   char Nfromv;/*Number of keywords "fromv" in FORM id*/
   char Ntov;/*Number of keywords "tov" in FORM id*/
   char Nvec;/*Number of keywords "vec" in FORM id*/
   char *id;/*Textual identifier;
              first element is the number of identifiers */
   char type;/* Identifier for prototypes*/
   char *form_id;/*FORM identifier*/
   char *mass;/*Mass of particle; will allocated only for propagators*/
   int skip;/*If it is set to 1, corresponding id never appear.*/
};/*struct internal_id_cell*/

struct flinks_struct{
   FILE *stream;
   word links;
};/*struct flinks_struct*/
/******************* end modue HASHTABS.C **********************/

struct indices_struct{
  struct indices_struct *next;
  char *id;
};

struct momentum_struct{
   char *vec;/* What is the vector?*/
   char *text;/*Full text without sign*/
};
typedef struct momentum_struct MOMENT;

struct ext_part_struct{
  char *id;/*m-string, id of particle*/
  char *ind;/*m-string, indices*/
  int *momentum;/*momentum -- integer n-vector.*/
  char is_ingoing;/*0 if outgoing, 1 if ingoing*/
  int fcount;/*fermion number flow counter*/
  char fflow;/*1 -  fflow coincides with red; -1 - opposit.*/
  char majorana;/*'0',-1,+1  depending on co-incide fermion number flow with the
      fermion flow (+1), opposit (-1), or undefined (0)*/
};

struct topology_array_struct{
   pTOPOL topology;/*Reduced topology*/
   pTOPOL orig;/*Original topology, may be NULL*/
   char **id;/* Userdefined set of dentifiers. If not defined by User, set
               to 't'. For undefined topologies 'u'.*/
   char n_id;/*Number of alternative sets of id.*/
   char n_momenta;/*Number of alternative sets of momenta*/
   int ***momenta;/*Momenta array -- pointers to arrays of groups.
                   Each momentum is integer n-vector., i.e., 0 element is
                   its length.
                   NULL for undefined topologies.*/
   int ***ext_momenta;/*Momenta array -- for external legs
                   NULL for undefined topologies.*/
   char max_vertex;/*Number of vertices.*/
   int label;/*Bit 0: 0, if topology not occures, or 1;
               bit 1: 1, if topology was produced form generic, or 0.
               bit 2: 1, automatic momenta is implemented, or 0*/
   REMARK *remarks;/*Array of remarks*/
   word top_remarks;/*Number of allocated cells.*/
   /* Free cell, if the "name" field is NULL. */

   word num;/*Number -- equal to the current array index.*/
   /*ASCII-Z strings started from 1:*/
   char *l_subst;/*lines substitution*/
   char *v_subst;/*vertices substitution*/
   char *l_dir;/* lines directions*/
   char **linemarks;/*Identifiers which can be assigned to each of lines*/
   char **vertexmarks;/*Identifiers which can be assigned to each of vertex*/
   int coordinates_ok;/* 0, if coordinates are absent; 1, if they are ok*/

   /* Coordinates. They relate to USER topology,
      i.e. user numbering of vertex/lines.
      ATTENTION! The EXTERNAL vertices/lines are numbered in unconvinient
      manner:( :
      (-4,4)(-3,3)(-2,2)(-1,1).....
        0     1     2     3
   */
   double *ev;  /* external vertices */
   double *evl; /* external vertices labels*/
   double *iv;  /* internal vertices*/
   double *ivl; /* internal vertices labels*/
   double *el;  /* external lines */
   double *ell; /* external lines labels */
   double *il;  /* internal lines */
   double *ill; /* internal lines labels */
   /* To use postscript routines for each arc we need :
    * ox,ox -- the center coordinates;
    * rad -- the radius;
    * start_angle -- start angle;
    * end_angle -- target angle:*/
   double *rad;
   double *ox;
   double *oy;
   double *start_angle;
   double *end_angle;

};

typedef struct topology_array_struct aTOPOL;

struct diagram_struct{
   char n_id;
   word prototype;
   word topology;
   word number;
   word sort;
};

/*Variable verttices will be an array of this type, vertices[v][l]. Here v is the vertex
index, l is the index of INCOMING into v lines. Will be set up in read_inp.c in
  create_indices_table(); used in utils.c in create_vertex_text() and by the operator 
   vertexInfo (function macvertexInfo() fle macro.c):*/
struct vertices_struct{
  /* Note, number of coinciding tails is not stored here since it can be found as
     *(id[dtable[v][0]-1].id);(id is m-string!) */
   char is_ingoing;/*1 if ingoing; 0 if outgoing*/
   char n;/*the line number*/
   char *id;/*id of a corresponding particle - just cahr*, not m-string!*/
};
typedef struct vertices_struct VERTEX [MAX_LINES_IN_VERTEX];
typedef int PROCESS_TOKEN(int n,char *id);

/*This structure is used to chain all swallowed processes with pipes into
 the hash table g_allpipes:*/
typedef struct {
   int r,w;/*r-descriptor to read from a pipe, w - descriptor to write to a pipe*/
   FILE *strr;/*stream opened with fdopen(r,"r")*/
   pid_t pid;/*pid*/
} ALLPIPES;
