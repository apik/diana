\Begin(boson)
[g,g   ;g; VV(num,ind:1 ,ind:2 ,vec, 3)*i_;0;spiral, 5, 2]
[A,A   ;a; VV(num,ind:1 ,ind:2 ,vec, 0)*i_ ;0; wavy, 3, 3]
\End(boson)
\Begin(fermion)
[u,U   ;u ; FF(num,fnum,vec, 0 )*i_; mmU;arrowLine,0,2]
\End(fermion)
\Begin(ghost)
[gg,GG; 0 ; SS(num,vec,0)*i_ ;0;arrowLine,10,2]
\End(ghost)
\Begin(vertex)
[U,A,u;; F(num,fnum,ind:2, 1,0, 1)*(-i_)*em*Qu]
[g,g,g,g; 4; V(num,ind:1,ind:2,ind:3,ind:4, 4)*(-i_)*gs^2]
[g,g,g;; V(num,ind:1,ind:2,ind:3,vec:1,vec:2,vec:3, 3)*gs ]
[GG,g,gg; a; V(num,ind:2,vec:2, 1)*(-gs)]
[U,g,u;a; F(num,fnum,ind:2,1,0, 1)*(-i_)*gs]
\End(vertex)
