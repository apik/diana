\MINVERSION(2.29)
\Begin(program,routines.rtn)
\section(common,browser,regular)
\Begin(initialization)
\End(initialization)

\section(regular)
\Begin(output,@GET(_processname).in)
   #define TOPOLOGY "\topologyid()"
   \outputDummyMomenta(NM)
   *----------------------------------------- 
   g Rq =\integrand();
   \offblanklines\offleadingspaces

   \breakfolder(\GET(foldername)M)
   \Begin(DUMMYMOMENTA,M,.sort,)
     id \M() = '\M()';
     id SS(n?,\M(),0) = SS(n,'\M()',0);
   \End(DUMMYMOMENTA)
\End(output)
\End(program)

