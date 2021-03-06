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
/*TEXTS.H*/
/*begin init.c*/
#define ELAPSEDTIME "Elapsed time: %f (seconds).\n"
/*end init.c*/
/* begin halt.c*/
#define ERRORMSG "***ERROR***\n"
/*end halt.c*/
/*begin tools.c*/
#define CNATRETURNFROMINCLUDE "Can't return form include."
#define ENDFILE "     <-(l=%ld, c=%d) '%s'"
#define LEAVEFILE "(l=%ld, c=%d) '%s'->"
#define NOINCLUDESAVAIL "No includes at all!"
#define CANNOTOPEN "Can't open '%s'."
#define NOTMEMORY "Not enough memory."
#define SCANERACTIVE "Scanner is active: file:'%s' line:%ld col:%d\n"
#define UNEXPECTEDEOF "Unexpected EOF."
#define BUFEROVERFLOW "Scan buffer overflow."
#define NOINCLUDE "Too many includes."
#define NOREALLOCINCLUDE "Can't change maximal number of includes."
#define CBRACKET "')' expected."
#define BACKTOFILE "'%s'<-"
#define GOTOFILE "     ->'%s'"
/* end tools.c*/
/*begin topology.c*/
#define THELP1 "To specify a topology the string should be set "
#define THELP2 "like as following: "
#define THELP3 "Negative numbers mean end of external lines. "
#define THELP4 "All negative numbers should be ordered. "
#define OBRACKEDEXPECTED "'(' expected instead of '%s'."
#define  MINUSEXPECTED "'-' expected instead of '%s'."
#define TOOLONGTOPOLOGY "Too long topology."
#define UNEXPECTED "Unexpected '%s'."
#define INVVERTEX "Invalid vertex %s."
#define COMMAEXPECTED "',' expected instead of '%s'."
#define CBRACKEDEXPECTED "')' expected instead of '%s'."
#define INVTOPOLOGY "Invalid topology structure."
#define INVEXTLINE "Invalid structure of external lines."
/*end topology.c*/
/*begin truns.c*/
#define WRONGNEWCOMMENT "Comment must be a single character!"
#define NEWCOMMENTSET "***WARNING!*** New comment character '%s' is set!"
#define WRONGNEWCOMMA "Comma character must be a single character!"
#define NEWCOMMASET "***WARNING!*** New comma character '%s' is set!"
#define NEWESCAPE "***WARNING!*** New escape character '%s' is set!"
#define ERRORDIRECTIVE "Preprocessor: error directive: %s"
#define INCORRECTVERSION "Current version %s is incompatible with required"
#define FORNATALOWEDHERE "This control is not allowed here."
#define CANNOTCONVERTTONUMBER "Can't convert %s to number."
#define ARGSEXPECTED "Argument list expected."
#define POSITIONINFOUNAVAIL "Information about position is unavailable.\n"
#define INVCHARINLABEL "Invalid label '%s'."
#define DOUBLELABEL "Double defined label '%s'."
#define TOOMANYLABELS "Too many labels."
#define PROCCLEANED "***WARNING!***  %s unused functions rejected."
#define USINGKEYWORD "Can't use the keyword '%s' as a function or macro name."
#define RESETATOB "***WARNING!*** Argument reset '%s' -> '%s'."
#define WARNINGREDECLARATION "***WARNING!*** Redeclaration of non-empty function '%s'."
#define WARNINGREDEFINITION "***WARNING!*** Redefinition of '%s'."
#define WRONGARSINREDECLAR "Redeclaration of '%s': wrong number of arguments."
#define USINGMACRONAMEASPRC "Can't use name '%s': there is such operator."
#define TOOLONGMACROARG "Too long argument:\n%s\n"
#define TOOLONGSTRING "Too long string."
#define UNDEFINEDCONTROLSEQUENCE "Undefined control sequence: '%s'."
#define ELSEWITHOUTIF "Using 'else' without 'if'."
#define ELSEWITHOUT_IF "Using 'ELSE' without 'IF'."
#define ENDIFWITHOUTIF "Using 'endif' without 'if'."
#define ENDIFWITHOUT_IF "Using 'ENDIF' without 'IF'."
#define UNTERMINATEDIF "Uncomplete 'if'."
#define UNTERMINATED_IF "Uncomplete 'IF'."
#define SUPERFLUOUSrBRACKET "Superfluous right bracket."
#define UNEXPrBRACK "Unexpected ')'."
#define NONBOOL "Operation 'not' must have boolean argument."
#define NONSTRING "Both left and right arguments of 'cat' must be strings."
#define SUPERFLUOUSlBRACKET "Superfluous left brackets."
#define NORIGHTBRACKET "Can't find corresponding right bracket"
#define SYNTAXERROR "Syntax error"
#define TOOMANYRECS "Too deeply nested construction"
#define TOOMANNESTEDMACRO "Too deeply nested operator"
#define UNTERMINATEDSTR "Unterminated string."
#define DISKPROBLEM "Disk problem."
#define TOKENERROR " error! file:'%s' line:%ld col:%d\n"
#define INTERNALERROR "Internal error."
#define TOOMANYMACRO "Too many numbers of operators."
#define TRUNSEND "...}"
#define NAMENOTFOUND "Operator '%s' is undefined."
#define WRONGARGNUM "Operator '%s': wrong number of arguments."
#define WRONGEMPTYARGS "Operator '%s': ')' expected instead of '%s'."
#define WHEREARGS "Argument(s) expected in '%s'."
#define UNDEFLOOP "Using 'while' without 'do'."
#define UNCOMPLETEIF "Uncomplete 'if'."
#define UNTERMINATEDWHILE "Uncomplete 'while'."
#define LOOPWITHOUTWHILE "Using 'loop' without 'while'."
#define UNTERMINATEDLOOP "Uncomplete 'do'."
#define UNTERMINATED_FOR "Uncomplete 'FOR'."
#define PROCORPROGEXPECTED "'function' or 'program' expected."
#define FUNCTION "{function '%s'..."
#define PROGRAM "{program..."
#define GOTOMACRO "   [%s]"
#define TOOMANYPROCS "Too many functions."
#define TOOMANYLABELSGROUP "Too many labels group."
#define NOLABELSGROUP "No labels group."
/*end truns.c*/
/*begin run.c*/
#define STACKUNDERFLOW "Stack underflow."
#define RUNERROR "Runtime"
#define CANNOTWRITETOFILE "Can't write to '%s'."
#define PROCSTACKOVERFLOW "Function stack overflow."
/*end run.c*/
/*begin macro.c*/
#define TOOMANYSPECIALIMAGES "Too many special images."
#define INVSIG "Invalid signal specification."
#define CLIENTISDIED "Client is dead."
#define PIPEXPECTED "PID of swallowed process expected instead of '%s'."
#define LASTJOBNAMEISUNDEFINED "No jobs yet"
#define SERVERFAILS "Server fails!"
#define INVALIDCONTENT_PS "Invalid content of system bariable %s (%s)."
#define MODESAVESTACKOVERFLOW "Mode save stack overflow."
#define NOSAVEDMODES "No saved modes."
#define NOSUSPENDEDFILES "No suspended files."
#define CANNOTRESETPRG "Run-time compiler can't reset the program!"
#define SCANNERNOTINIT "Scanner is not initialized."
#define SCANNERBUSY "Scanner is busy."
#define PROCNOTFOUND "Function '%s' is undefined."
#define TOOBIGSTRINGNUMBER "Number of string (%s) too big."
#define NOOPENEDFILE "No opened file."
#define CANTOPENFILE "Can't open file %s."
#define INVCOUNT "Invalid parameter 'count' in 'copy'."
#define INVINDEX "Invalid parameter 'index' in 'copy'."
#define UNDEFINEDVAR "Variable '%s' is undefined."
#define UNDEFINEDLABEL "Label '%s' is undefined."
#define NUMERROR "Function '%s': cannot convert '%s'."
#define DIVBYZERO "Function '%s': division by 0."
#define INVALIDIDNUMBER "Invalid number of id '%s'."
#define INVALIDOUTNUMBER "Invalid number of output term '%s'."
#define OUTPUTEXPRESSIONNOTCREATED "Output expression not created yet."
#define INVALIDREORDERING "Invalid re-ordering set: '%s'."
#define INVALIDREORDERINGSTRING "Invalid re-ordering set."
#define TOOLONGNEWPROTOTYPE "Too long new prototype '%s'."
#define CLOCKSETACTIVE "Invalid clock number %s (valid from 1 to 10)."
#define SPECIALREMARKS "Wrong structure of special remark %s in topology table id=%d entry %d:\n  %s "
#define DIGITEXPECTED "digit expected"
#define TOOLARGENUM "invalid vertex index"
#define UNEXPECTEDSYM "unexpected symbol"
#define EMPTYREMARK "empty remark"
/*end macro.c*/
/*begin main module*/
#define FAILCREATINGWRKTABLE "Fail creating working table: %s"
#define FAILSAVETOP "Fail saving topologies: %s"
#define READINGCONFIG "Reading configuration from '%s'..."
#define RUNSIGNAL "Halt signal from interpreter."
#define NOERROR "Success."
#define INITIALIZATION \
"DIANA $Revision: 2.37 $ Copyright (C) 1999-2003 M.Tentyukov.\n<tentukov@physik.uni-bielefeld.de>"
#define DONEINIT "...done initialization."
#define ONLYVERIFY "Checking mode detected."
#define READDIAGRAMS "Reading diagrams..."
#define INDICATE_MESSAGE "%u of %u."
#define DONEDIAGRAMS "...done."
#define BROWSERMODEDETECT "Browser mode detected."
#define SORTINGDIAGRAMS "Sorting diagrams..."
#define DONE "...done."
#define LOADINGTABLES "Loading topology tables..."
#define PRINTINGDIAGRAMS "Printing diagrams..."
#define LOOKINGFORFIRRSTDIAGRAM "Looking for the first diagram..."
#define EXTRACALL "Extra call to interpreter..."
#define CALLTOINTERPRETER "Call to interpreter..."
#define EXITCODEFROMINTERPRETER "Exit status is %d."
#define BEGINTRUNS "******* Begin translation protocol.*******"
#define ENDTRUNS "******* End translation protocol.*******"
/*end main module*/
/*begin cmd_line.c*/
#define TOOSMALLNUM "Negative number (%s) not allowed."
#define TOOBIGNUM "Too big number (%s)."
#define CURRENTVERSION "$Revision: 2.37 $"
#define CURRENTOPTIONS "Current options:\n"
#define PROMPT "option="
#define ILLEGALOPTION "Illegal option."
#define ILLEGALVAL "Illegal option value '%s'."
#define MSG_DISABLE_TXT "disable"
#define MSG_ENABLE_TXT "enable"
#define UNDEFOPT "Undefined option '-%s'."
#define UNEXPECTEDENDOFOPTIONS "Unexpected end of options."
#define TOOLONGID "Too long identifier '%s'."
#define USERBREAK "Program terminated by user."
#define INVALIDTEMPLATE "Invalid pattern '%s'."
#define ILLEGALB "Illegal value of option 'b': '%s'."
#define ILLEGALE "Illegal value of option 'e': '%s'."
#define ILLEGALL "Illegal list: '%s'..."
#define TOOMANYROPT "Too many runtime options."
#define HELP1 "\nUsage: %s [options].  Available options:\n"
#define HELP2 "-l file      :  Only show  topologies and prototypes, file - output filename.\n"
#define HELP3 "-sd pattern  : Set sorting mode for diagrams, used together with -l;\n"
#define HELP4 "-sp pattern  : Set sorting mode for prototypes, used together with -l;\n"
#define HELP6 "-t identifier: Process only diagrams with given topology.\n"
#define HELP7 "-p identifier: Process only diagrams with given prototype.\n"
#define HELP8 "-b number    : Start diagram number.\n"
#define HELP9 "-e number    : Last diagram number.\n"
#define HELP10 "-c file      : Configuration file name; default - config.cnf.\n"
#define HELP11 "-x list      : Exclude diagrams.\n"
#define HELP12 "-i list      : Include diagrams.\n"
#define HELP13 "-f file      : Read options from file.\n"
#define HELP14 "-v           : Only check.\n"
#define HLP14A "-vt a|f|t    : Check momenta balance on topologies.\n"
#define HELP15 "-r option    : Option passed to interpreter.\n"
#define HELP16 "-h           : Show current help text.\n"
#define HELP17 "-q           : Immediate quit.\n"
#define HELP18 "-P           : Program file name, used if 'only interpret' is set.\n"
#define HELP19 "-o           : Show current options value.\n"
#define HELP20 "-m e/d       : messages enable/disable.\n"
#define HELP21 "--char       : Set default value for option.\n"
/*end cmd_line.c*/
/*begin hashtabs.c*/
#define DOUBLEDEF "Double defined '%s'."
/*end hashtabs.c*/
/*begin cnf_read.c:*/
#define ONLYELEMVEC "Only elementary loop momenta allowed"
#define FULLCMDLINE "Full command line:"
#define IDALREADYUSED " Id %s is already used."
#define THEREAREUNDEFINEDINDICES "There are undefined indices."
#define NOINEDINDICESGROUP "The particle does NOT contain the specified indices group."
#define UNDEFINEDINDICESGROUP "'%s': Undefined indices group."
#define UNKNOWNLINETYPE "'%s' unknown (wavy,arrowWavy,spiral,arrowSpiral,line,arrowLine)."
#define ONLYONEINCLUDESECTION "Only one 'include' secitons allowed fo particle or vertex."
#define INTERPRETEXPECTED "'interpret' expected instead of '%s'."
#define WRONGNUMBEROFINDICESINPROPAGATOR " Wrong number of indices in propagator."
#define REQPARTWITHOUTIND "Request of index allocation for patricle without index."
#define NUMBEREXPECTED "Number expected instead of '%s'."
#define OPENLOG "Protocol file '%s' is opened."
#define ONLYFORPROPAGATORS "Parameter '%s' allowed only in propagators."
#define INVALDESCCHAR "Invalid escape character: '%s'."
#define INVALIDNUMBER "Invalid number '%s'."
#define CANNOTRESETTABLE "Can't reset existing table."
#define INVALIDBEGINOFID "Invalid first character of id '%s'."
#define UNDEFINEDID "Undefined identifier '%s'."
#define UNDEFINED "Undefined '%s'."
#define UNDEFINEDORUSED "Undefined or already used index '%s'."
#define NOMODEL "Model is not described yet."
#define SUPERFLUOUSMOMENTUM "Superfluous momentum definition."
#define TOOMANYVECS "Too many vectors."
#define UNDEFMOM "Not all momenta were defined!"
#define DOUBLETOPOLOGY "Double defined topology: \n   %s"
#define WHEREINGOING "Ingoing particles are undefined."
#define COLEXPECTED "':' expected instead of '%s'."
#define QUOTEXPECTED "'\"' expected instead of '%s'."
#define FBREXPECTED "'}' expected instead of '%s'."
#define SBREXPECTED "']' expected instead of '%s'."
#define DISAGREEMENT "Type of identifier '%s' does not agree with previous."
#define ONLIPAAP "Unknown direction '%s', only 'ap' or 'pa' allowed."
#define TOOMANYPARTINVERT "Too many particles in vertex."
#define SEMICOLEXP "';' expected instead of '%s'."
#define EQUALEXP "'=' expected instead of '%s'."
#define TRUNSEXPECTED "'execute' expected instead of '%s'."
#define CALLEXPECTED "'call' expected instead of '%s'."
#define DOUBLEEXT "Can't reset number of external lines."
#define EXTDISAGREE "Ivalid number of external particles."
#define WHEREEXTPART "Undefined number of external lines."
#define WHEREINPUT "Input file name?"
#define ONLY1INDEX "Only one index per group per particle."
#define TOOMANYMOMENTASETS "Too many alternative momenta sets."
#define TOOMANYTOPOLOGYIDSETS "Too many alternative identifiers sets."
#define ONLYONECHARACTERALLOWED "Can't use '%s' as type, only one character allowed."
#define NOTOPOLOGIESYET "Topology must be described before coordinates are set."
#define COORDTYPEEXPECTED " Type of coordinates expected instead of %s."
#define UNSUFFICIENTLINEMARKS "Unsufficient line marks."
#define UNSUFFICIENTVERTEXMARKS "Unsufficient vertex marks."
#define MAXIMALALLOWEDNUMBER "Maximal allowed number is %s."
#define DOUBLEDEFREMNAME "Double defined remark name"
#define EMPTYREMNAME "Empty remark name is not allowed"
#define WRONGMOMENTABALANCE "Topology %u (id %s) momenta set %d: momenta in vertices %s don't balance."
#define WASWRONGMOMENTABALANCE "*** WARNING! *** %u unbalanced momenta sets!"
#define HALTWASWRONGMOMENTABALANCE " %u unbalanced momenta sets!"
/*end cnf_read.c*/
#define INDICESALL "Indices exhausted in group '%s'."
#define FERMIONICORDERNOT2 "Order of fermion interaction is not 2 (%d)!"
/*begin last_num.c*/
#define CURRUPTEDINPUT "File '%s' corrupted."
#define WRRONGSTARTNUMBER "First diagram number %u greate then last %u."
#define MAXDIAGRAM "Too many diagrams."
/*end last_num.c*/
/*begin read_inp.c*/
#define UNSUFFICIENTLOOPMOMS "Unsufficient loop momenta."
#define TOOMANYVETICES "Too many vertices."
#define TOOMANYLINES "Too many internal lines."
#define WRONGEXTERNALLINE "Invalid number '%s' of external line."
#define WRONGEXTERNALPART "External particles do not agree with QGRAF output."
#define FOURLINE "Too many lines in vertex."
#define COMMONPROTOKOLOUT "%4u. d%-4u: prototype: %5s, topology %5s.\n"
#define OUTPUTDIAGRAMINFO "%4u. d%-4u: prototype: %5s%s.\n"
/*end read_inp.c*/
#define CANNOTRESETEXISTOUTPUT "Can't reset existing output."
#define CANNOTCREATEOUTPUT "Can't execute this operator."
#define UNSUFFICIENTFERMID "Not enough different identifiers to process fermion line (%s)."
/*begin utils.c*/
#define CANNOTOPENTT "Can't open file"
#define TBLLOADED "   %s from file %s is loaded (id %d)"
#define SHAPE "Shape table"
#define TABLE "Table"
#define MOMENTA "Momenta table"
#define FAIL_LOADING_TABLE "Fail loading table %s: %s"
#define TABLE_NOT_LOADED "***WARNING!*** Table %s not loaded: %s"
#define NOTHINGTOLOAD "nothing to load"
#define FAIL_LOADING_MIXED_TABLE "mixed tables are not allowed"
#define WARNINGDIAGRAMCOUNT "***WARNING!***  Diagram %u:"
#define TOPOLOGYISUNDEFINED "Undefined topology '%s'."
#define UNEXPECTED_EOL "Unexpected end of line"
#define INVALIDBRACE "'x', 'y', 'c', 'f' or 's' expected after '{'"
#define STACK_OVERFLOW "Stack overflow"
#define UNCLOSEDBLOCK "Unsufficient '}'"
#define STACK_UNDERFLOW "Stack underflow"
#define PARAMETEREXPECTED "Parameter expected"
#define NUMEXPECTED "Number expected"
#define FONTNAMEEXPECTED "Font name expected"
/*end utils.c*/
/*begin browser.c*/
#define BROWSERHEAD1 "Configuration file: %s; input file: %s\nstart diagram: %u, end: %u; %u diagram found. \n"
#define BROWSERHEAD2a "Topology template:"
#define BROWSERHEAD2b "prototype template:"
#define BROWSERHEAD3 "Sorting: diagram='%s', prototypes='%s'.\n"
#define OUTPUTDIAGRAMINFO "%4u. d%-4u: prototype: %5s%s.\n"
#define PROTOTYPESFOUND "\n\n%u prototypes found.\n"
#define OUTPUTPROTOTYPESINFO "%4u.    %5s%s.\n"
#define FOUNDTOPOLOGIES "\n\nAll found topologies:\n"
#define OUTPUTFOUNDTOPOLOGY "%4u.  %5s%s: %s.\n"
#define NOTFOUNDTOPOLOGIES "\n\nDefined but not found topologies:\n"
#define UNDEFINEDTOPOLOGIES "\n\nUndefined topologies:\n"
#define ABSENT "Absent.\n"
/*end browser.c*/

#define TT_NONFATAL_TXT "Unspecified non-fatal error."
#define TT_FATAL_TXT "Unspecified fatal error."
#define TT_INVALID_TXT "Invalid topology."
#define TT_NOTFOUND_TXT "Not found by lookup."
/*Removed:
#define TT_DBL_MAXTOP_TXT "Attempt to change maximal number of topologies during loading a table."
*/
#define TT_TOOMANYTOPS_TXT "Too many topologies."
#define TT_DOUBLEDEFNAM_TXT "Double defined topology name."
#define TT_FORMAT_TXT "Token conversion error."
#define TT_EMPTYTABLE_TXT "Empty tables not allowed."
#define TT_CANTINITSCAN_TXT "Can't initialize a scanner."
#define TT_SCANNER_NOTINIT_TXT "Call getToken without initializing of a scanner."
#define TT_DOUBLETOPS_TXT "Two topologies in the table are coincide after reduction."
#define TT_TOOMANYMOMS_TXT "Too many momenta sets."
#define TT_DOUBLEDEFTOKEN_TXT "Double defined translation token."
#define TT_TOPUNDEF_TXT "Undefined topology name."
#define TT_REM_DOUBLEDEF_TXT "Double defined remark name."
#define TT_REM_EMPTYNAME_TXT "Empty name is not allowed."
#define TT_REM_UNEXPECTED_EQ_TXT "Unexpected '='."
#define TT_CANNOT_WRITE_TXT "Can't write to disk."

#define STARTSERVER "Server is running."
#define  SERVERALREADYISRUNNING "Can't start server %s: already running!"
#define SOCKETERROR "Socket error %d."
#define JOBINFOFILEISCORRUPTED " *** WARNING! *** Job info file '%s' is corrupted."
#define TOOMANYJOBS "Too many jobs"
