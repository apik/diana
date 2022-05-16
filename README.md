DIANA
=====

DIagram ANAlizer - QGRAF based program for efficient Feynman diagram generation and analysis written by M.Tentyukov

##### Installation:

1. Download latest QGRAF source file qgraf-3.x.y.f from http://cfif.ist.utl.pt/~paulo/qgraf.html 
and put it into `qgraf/` subdirectory if internet connection is available and WGET or CURL are in your path it will be downloaded automatically
2. For this step GNU autotools http://www.gnu.org/software/autoconf/ needed:

`~$autoreconf -i`

3. Configure:

`~$./configure` or using prefix to install to specific location `~$./configure --prefix=...`

4. Build:

`~$make`

5. Install:

`~$make install`


##### Usage:

Assuming that `${prefix}/bin` is in your `$PATH` run:

`diana -c create.cnf`
