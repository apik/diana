\Begin(boson)
[Wp,Wm ;w; ww(num,ff=fflow,fn=fnum,ind:1,ind:2,vec);0;arrowWavy, 5, 2; W{xy(2)(5){s(15)+}}]
[A, A  ;a; AA(num,ff=fflow,fn=fnum,ind:1,ind:2,vec);0;wavy, 4, 2; {f(Symbol)(15)g}]
\End(boson)
\Begin(fermion)
[f,F ;f; ff(num,ff=fflow,fn=fnum,vec ); 0;arrowLine,0,2;{f(Symbol)(15)f}]
[l,l ;l; ll(num,ff=fflow,fn=fnum,vec ); 0;line,2,2;{f(Symbol)(15)l}]
\End(fermion)
\Begin(ghost)
\End(ghost)
\Begin(vertex)
[F,f,A;; Ffa(num,ff=fflow,fn=fnum,ind:3,vec:1,vec:2,vec:3)]
[l,f,Wm;; lfWm(num,ff=fflow,fn=fnum,ind:3,vec:1,vec:2,vec:3)]
[F,l,Wp;; FlWp(num,ff=fflow,fn=fnum,ind:3,vec:1,vec:2,vec:3)]
\End(vertex)
