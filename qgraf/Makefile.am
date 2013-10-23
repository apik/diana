bin_PROGRAMS = qgraf

## Name of downloaded qgraf source file by P.Nogueira
qgraf_SOURCES = qgraf-3.1.4.f	

qgraf_CPPFLAGS = $(QGRAF_CFLAGS)
qgraf_LDADD = $(QGRAF_LIBS)


qgraf-3.1.4.f:
	@DOWNLOAD_PROGRAM@ ./qgraf-3.1.4.tgz http://qgraf.ist.utl.pt/v3.1/qgraf-3.1.4.tgz 
	tar -zxvf qgraf-3.1.4.tgz 

