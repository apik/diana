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

/*tt_gate.h*/

#ifndef TT_GATE_H
#define TT_GATE_H

#include "hash.h"
#include "tt_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tt_scannerStruct {
   void *extrainfo;
   char *fname;
   char *buf;
   int max;
   int *state;
   int *err;
                              } tt_scannerType;

char *tg_scannerGetToken(tt_scannerType *s);
void tg_scannerDone(tt_scannerType *s);/*Must free s->extrainfo!*/
int tg_scannerInit(tt_scannerType *s);/*Must allocate s->extrainfo in success and return 0
                       If fails, must set sc->err properly. */
int tg_reduceTopology(tt_singletopol_type *topology);
/*Looks up the topology top in the table. NO REDUCTION!
  The topology is assumed to be reduced!
  nv is the number of vertices. If it is <0, then it will be evaluated by the
  subroutine.
  The variable t is already allocate (tt_singletopol_type *tt_initSingleTopology())
  If top!=NULL then it assumed that the topology t must be copied from top,
  otherwice it is assumed to be ready :*/
long tt_lookupFromTop(tt_singletopol_type *t,int table, pTOPOL top, int nv);

#define tg_scannerOutputMessage output_scanner_msg

#ifdef __cplusplus
}
#endif
#endif
