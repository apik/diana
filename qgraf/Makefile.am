bin_PROGRAMS = qgraf

## Name of downloaded qgraf source file by P.Nogueira
qgraf_SOURCES = qgraf.f	

qgraf_CPPFLAGS = $(QGRAF_CFLAGS)
qgraf_LDADD = $(QGRAF_LIBS)


qgraf.f:
	@DOWNLOAD_PROGRAM@ ./qgraf.tgz http://qgraf.tecnico.ulisboa.pt/links/qgraf-3.4.tgz
	tar -zxvf qgraf.tgz
	mv qgraf*.f qgraf.f

