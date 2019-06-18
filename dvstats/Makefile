#**********************************************************************
#*                                                                    *
#*                 Licensed Materials - Property of IBM               *
#*          © Copyright IBM Corp. 2000   All Rights Reserved          *
#*                                                                    *
#**********************************************************************

#CFLAGS =	-w -O -qchars=signed -qarch=com
CFLAGS =	-w -g -qdbxextra -qchars=signed -qinitauto=af -qcheck -qarch=com
CC =		/usr/bin/xlc
#CC =		/u/risc/bin/nxlc
DEST =		/tmp/dvstats

VSTATSRCS =			\
	read_vliwcnts.c		\
	process_vliwcnts.c	\
	graph.c			\
	dis_tbl.c		\
	op_freq.c		\
	hash.c			\
	offset.c		\
	info_dir.c		\
	libutil.c		\
	daisy_op_mapping.c

VSTATOBJS = 	$(VSTATSRCS:.c=.o)

all:	dvstats

dvstats::	$(VSTATOBJS)
	$(CC) -o dvstats $(VSTATOBJS)

install:	dvstats
	cp dvstats $(DEST)/bin

clean:
		\rm -f $(VSTATOBJS) dvstats
depend:
		makedepend -- $(CFLAGS) -- $(SRCS) $(VSTATSRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.

read_vliwcnts.o: read_vliwcnts.c /usr/include/stdio.h /usr/include/assert.h
read_vliwcnts.o: /usr/include/standards.h vliwcnts.h brcorr.h
read_vliwcnts.o: parse.h opcodes.h PSOpcodes.h instrum.h
read_vliwcnts.o: op_mapping.h cnts_wr.h translate.h
process_vliwcnts.o: process_vliwcnts.c /usr/include/stdio.h
process_vliwcnts.o: /usr/include/standards.h /usr/include/assert.h
process_vliwcnts.o: vliwcnts.h
process_vliwcnts.o: dis.h translate.h parse.h opcodes.h
process_vliwcnts.o: PSOpcodes.h instrum.h op_mapping.h
graph.o:      graph.c /usr/include/stdio.h /usr/include/standards.h instrum.h
dis_tbl.o:    dis_tbl.c dis.h
op_freq.o:    op_freq.c /usr/include/stdio.h /usr/include/standards.h
op_freq.o:    /usr/include/assert.h dis.h parse.h opcodes.h
op_freq.o:    PSOpcodes.h op_mapping.h
hash.o:       hash.c /usr/include/stdio.h /usr/include/assert.h
hash.o:       brcorr.h
offset.o:     /usr/include/stdio.h /usr/include/assert.h
offset.o:     offset.c offset.h
info_dir.o:		info_dir.c /usr/include/stdio.h
libutil.o:		libutil.c /usr/include/stdio.h
daisy_op_mapping.o:	daisy_op_mapping.c
daisy_op_mapping.o:	dis_tbl.h parse.h op_mapping.h
