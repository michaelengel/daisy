#**********************************************************************
#*                                                                    *
#*                 Licensed Materials - Property of IBM               *
#*          © Copyright IBM Corp. 2000   All Rights Reserved          *
#*                                                                    *
#**********************************************************************

CFLAGS =	-w -g -DSMALL_MACHINE_MODEL -qarch=ppc
CC =		/bin/cc
LD =		/bin/ld
DEST =		/tmp/daisy

DAISYSRCS =			\
	main.c			\
	vliw.c			\
	dis.c			\
	dis_tbl.c		\
	opcode_fmt_fcn.c	\
	opcode_fmt_tbl.c	\
	bitvec.c		\
	optypes.c		\
	ppc_map.c		\
	ppc_map_tbl.c		\
	mult.c			\
	ld_st_up.c		\
	combine.c		\
	cache.c			\
	stringsx-v2p.c		\
	stringsx-p2v.c		\
	stats.c			\
	hash.c			\
	util.c			\
	kernel.c		\
	debug.c			\
	flags.c			\
	brlink.c		\
	simul.c			\
	simul_shm.c		\
	cnts_wr.c		\
	cnts_stubs.c		\
	vliw_iter.c		\
	vliw_dump.c		\
	contin.c		\
	heap.c			\
	instrum.c		\
	rd_config.c		\
	bcrl.c			\
	shm.c			\
	trace.c			\
	ilp.c			\
	ccr-v2p.c		\
	ld_motion.c		\
	bct.c			\
	errno_txt.c		\
	copyprop.c		\
	regmap.c		\
	must-alias.c		\
	cache_simul.c		\
	cache_config.c		\
	cache-trace.c		\
	fd_gets.c		\
	ports.c			\
	to_native.c		\
	record.c		\
	re-exec.c		\
	group_exit.c		\
	pdf.c			\
	loophdr.c		\
	loadver.c		\
	fmr_commit.c		\
	cluster.c		\
	addic0.c		\
	rz0.c			\
	rlwimi.c		\
	del_dead.c		\
	resrc.c			\
	tree_pos.c		\
	rename.c		\
	unify.c			\
	offset.c		\
	bitvec_s.s		\
	r13.s			\
	ldver.s

DAISYOBJS = 	$(DAISYSRCS:.c=.o)

all:	daisy gen_resrc

daisy:	$(DAISYOBJS)
	$(CC) -Wa,"-mPPC" -o daisy $(DAISYOBJS)

gen_resrc:	gen_resrc.c max_resrcs.h rename.h vliw.h gen_resrc.h /usr/include/stdio.h /usr/include/assert.h
	$(CC) -g -o gen_resrc gen_resrc.c

resrc_list.h:	gen_resrc resrc_list.header resrc_list.trailer
	./gen_resrc

resrc_offset.h:	gen_resrc resrc_offset.header resrc_offset.trailer
	./gen_resrc

r13_hdr.s:	gen_resrc
	./gen_resrc

r13.s:		r13_hdr.s r13_base.s
	/bin/cat r13_hdr.s r13_base.s > r13.s

r13.o:		/bin/as -m PPC -o r13.o r13.s

ldver.s:	r13_hdr.s ldver_base.s
	/bin/cat r13_hdr.s ldver_base.s > ldver.s

ldver.o:	/bin/as -m PPC -o ldver.o ldver.s

install:	daisy
	cp daisy $(DEST)/bin

clean:
		\rm -f $(DAISYOBJS) daisy
depend:
		makedepend -- $(CFLAGS) -- $(SRCS) $(DAISYSRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.

main.o:		  main.c dis.h resrc_list.h resrc_map.h resrc_offset.h
main.o:		  stats.h hash.h branch.h dis_tbl.h rename.h vliw.h contin.h
main.o:		  opcode_fmt.h
main.o:		  /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
main.o:		  /usr/include/fcntl.h /usr/include/unistd.h
main.o:		  /usr/include/float.h
vliw.o:		  vliw.c dis.h opcode_fmt.h resrc_list.h rename.h vliw.h max_resrcs.h
vliw.o:		  dis_tbl.h rd_config.h vliw-macros.h bitvec-macros.h
vliw.o:		  unify.h
vliw.o:		  /usr/include/stdio.h /usr/include/assert.h
dis.o:		  dis.c dis.h dis_tbl.h opcode_fmt.h 
dis.o:		  latency.h
dis.o:		  Makefile
dis.o:		  /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
dis.o:		  /usr/include/a.out.h  /usr/include/time.h
dis_tbl.o:	  dis_tbl.c dis.h dis_tbl.h opcode_fmt.h resrc_list.h
dis_tbl.o:	  latency.h
dis_tbl.o:	  Makefile
dis_tbl.o:	  /usr/include/values.h
opcode_fmt_fcn.o: opcode_fmt_fcn.c dis.h resrc_list.h opcode_fmt.h dis_tbl.h
opcode_fmt_fcn.o: /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
opcode_fmt_tbl.o: opcode_fmt_tbl.c opcode_fmt.h resrc_list.h dis_tbl.h
opcode_fmt_tbl.o: /usr/include/values.h
bitvec.o:	  bitvec.c dis.h opcode_fmt.h
bitvec.o:	  /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
ppc_map.o:	  ppc_map.c dis.h resrc_list.h resrc_map.h resrc_offset.h
ppc_map.o:	  rename.h vliw.h dis_tbl.h opcode_fmt.h cacheconst.h
ppc_map.o:	  gen_resrc.h
ppc_map.o:	  /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
ppc_map_tbl.o:	  ppc_map_tbl.c resrc_list.h resrc_map.h resrc_offset.h
mult.o:		  mult.c dis.h resrc_list.h resrc_map.h rename.h vliw.h opcode_fmt.h
mult.o:		  /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
ld_st_up.o:	  ld_st_up.c dis.h rename.h vliw.h opcode_fmt.h rd_config.h
ld_st_up.o:	  resrc_list.h dis_tbl.h unify.h
ld_st_up.o:	  /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
combine.o:	  combine.c dis.h rename.h vliw.h opcode_fmt.h rd_config.h
combine.o:	  resrc_list.h dis_tbl.h unify.h
combine.o:	  /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
optypes.o:	  optypes.c dis.h dis_tbl.h resrc_list.h opcode_fmt.h vliw.h
optypes.o:	  /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
cache.o:	  cache.c
cache.o:	  /usr/include/stdio.h /usr/include/assert.h
stats.o:	  stats.c dis.h opcode_fmt.h stats.h resrc_offset.h
stats.o:	  /usr/include/stdio.h /usr/include/assert.h
stats.o:	  /usr/include/fcntl.h
hash.o:		  hash.c dis.h opcode_fmt.h stats.h hash.h resrc_list.h
hash.o:		  /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
stringsx-v2p.o:	  stringsx-v2p.c dis.h resrc_list.h resrc_map.h resrc_offset.h
stringsx-v2p.o:	  opcode_fmt.h
stringsx-v2p.o:	  /usr/include/values.h /usr/include/stdio.h /usr/include/assert.h
stringsx-p2v.o:	  stringsx-p2v.c dis.h opcode_fmt.h resrc_list.h rename.h
stringsx-p2v.o:	  vliw.h opcode_fmt.h
stringsx-p2v.o:	  /usr/include/values.h
util.o:		  util.c /usr/include/stdio.h /usr/include/assert.h
kernel.o:	  kernel.c dis.h opcode_fmt.h resrc_list.h
kernel.o:	  branch.h rename.h vliw.h max_resrcs.h resrc_map.h
debug.o:	  debug.c /usr/include/stdio.h /usr/include/fcntl.h
flags.o:	  flags.c /usr/include/stdio.h debug.h
r13.o:		  r13.s   r13_hdr.s
ldver.o:	  ldver.s r13_hdr.s
bitvec_s.o:	  bitvec_s.s
brlink.o:	  brlink.c dis.h opcode_fmt.h resrc_list.h dis_tbl.h rename.h vliw.h
brlink.o:	  rd_config.h unify.h
brlink.o:	  /usr/include/stdio.h /usr/include/assert.h /usr/include/values.h 
cnts_wr.o:	  cnts_wr.c cnts.h /usr/include/strings.h /usr/include/stdlib.h
cnts_wr.o:	  /usr/include/stdio.h /usr/include/assert.h 
cnts_wr.o:	  /usr/include/fcntl.h
cnts_stubs.o:	  cnts_stubs.c cnts.h
simul.o:	  simul.c rename.h vliw.h dis.h opcode_fmt.h max_resrcs.h resrc_list.h
simul.o:	  resrc_map.h resrc_offset.h dis_tbl.h stats.h hash.h 
simul.o:	  rd_config.h gen_resrc.h cache.h debug.h simul.h
simul.o:	  /usr/include/stdio.h /usr/include/assert.h 
vliw_iter.o:	  vliw_iter.c rename.h vliw.h dis.h opcode_fmt.h max_resrcs.h
vliw_iter.o:	  resrc_list.h
vliw_iter.o:	  /usr/include/stdio.h /usr/include/assert.h 
vliw_dump.o:	  vliw_dump.c rename.h vliw.h dis.h opcode_fmt.h max_resrcs.h
vliw_dump.o:	  resrc_list.h
vliw_dump.o:	  /usr/include/stdio.h /usr/include/assert.h 
rd_config.o:	  rd_config.c rd_config.h max_resrcs.h opcode_fmt.h
rd_config.o:	  resrc_list.h gen_resrc.h vliw.h dis.h rename.h
rd_config.o:	  /usr/include/stdio.h /usr/include/assert.h
rd_config.o:	  /usr/include/fcntl.h /usr/include/sys/stat.h
rd_config.o:	  /usr/include/values.h
instrum.o:	  instrum.c rename.h vliw.h max_resrcs.h resrc_list.h resrc_offset.h
instrum.o:	  dis.h dis_tbl.h opcode_fmt.h 
instrum.o:	  /usr/include/stdio.h /usr/include/assert.h
instrum.o:	  /usr/include/errno.h /usr/include/sys/errno.h
instrum.o:	  /usr/include/sys/mode.h
instrum.o:	  /usr/include/unistd.h /usr/include/fcntl.h
contin.o:	  contin.c contin.h rename.h vliw.h max_resrcs.h resrc_list.h
contin.o:	  dis.h opcode_fmt.h
contin.o:	  /usr/include/stdio.h /usr/include/assert.h
contin.o:	  /usr/include/float.h
heap.o:		  heap.c
heap.o:		  /usr/include/values.h
bcrl.o:		  bcrl.c rename.h vliw.h dis.h opcode_fmt.h resrc_list.h dis_tbl.h
bcrl.o:		  /usr/include/stdio.h /usr/include/assert.h
shm.o:		  shm.c resrc_list.h resrc_map.h resrc_offset.h
shm.o:		  /usr/include/stdio.h /usr/include/stdlib.h
shm.o:		  /usr/include/sys/types.h /usr/include/sys/mman.h
shm.o:		  /usr/include/errno.h /usr/include/signal.h
shm.o:		  /usr/include/sys/shm.h
trace.o:	  trace.c
trace.o:	  dis.h opcode_fmt.h dis_tbl.h branch.h stats.h
trace.o:	  rename.h vliw.h max_resrcs.h resrc_list.h
trace.o:	  /usr/include/stdio.h /usr/include/assert.h
trace.o:	  /usr/include/signal.h /usr/include/fcntl.h
trace.o:	  /usr/include/errno.h /usr/include/sys/errno.h
trace.o:	  /usr/include/sys/mman.h /usr/include/sys/vmuser.h
ilp.o:		  ilp.c rename.h vliw.h max_resrcs.h resrc_list.h opcode_fmt.h dis.h
ilp.o:		  /usr/include/stdio.h /usr/include/assert.h
ilp.o:		  /usr/include/values.h
ccr-v2p.o:	  ccr-v2p.c dis.h dis_tbl.h resrc_map.h resrc_list.h
ccr-v2p.o:	  resrc_offset.h gen_resrc.h opcode_fmt.h
ccr-v2p.o:	  /usr/include/stdio.h /usr/include/assert.h
ccr-v2p.o:	  /usr/include/values.h
ld_motion.o:	  ld_motion.c dis.h rename.h vliw.h resrc_list.h opcode_fmt.h
ld_motion.o:	  dis_tbl.h unify.h
ld-motion.o:	  /usr/include/stdio.h /usr/include/assert.h
ld-motion.o:	  /usr/include/values.h
bct.o:		  bct.c rename.h vliw.h dis.h dis_tbl.h opcode_fmt.h resrc_list.h
bct.o:		  max_resrcs.h rd_config.h vliw-macros.h unify.h
bct.o:		  /usr/include/stdio.h /usr/include/assert.h
bct.o:		  /usr/include/values.h
errno_txt.o:	  /usr/include/sys/errno.h
copyprop.o:	  copyprop.c /usr/include/values.h
copyprop.o:	  /usr/include/stdio.h /usr/include/assert.h
copyprop.o:	  rename.h vliw.h dis.h dis_tbl.h rd_config.h max_resrcs.h resrc_list.h
copyprop.o:	  opcode_fmt.h 
regmap.o:	  /usr/include/stdio.h /usr/include/assert.h /usr/include/values.h
regmap.o:	  regmap.c rename.h vliw.h dis.h opcode_fmt.h 
regmap.o:	  max_resrcs.h resrc_list.h 
must-alias.o:	  must-alias.c rename.h vliw.h dis.h opcode_fmt.h
must-alias.o:	  max_resrcs.h resrc_list.h 
must-alias.o:	  /usr/include/stdio.h /usr/include/assert.h /usr/include/values.h
cache_simul.o:	  cache_simul.c cache.h cacheconst.h
cache_simul.o:	  /usr/include/stdio.h  /usr/include/assert.h
cache_simul.o:	  /usr/include/stdlib.h /usr/include/string.h
cache_config.o:	  cache_config.c cache.h cacheconst.h
cache_config.o:	  /usr/include/stdio.h /usr/include/assert.h
cache_config.o:	  /usr/include/fcntl.h /usr/include/sys/stat.h
cache-trace.o:	  cache-trace.c
cache-trace.o:	  /usr/include/stdio.h /usr/include/assert.h
cache-trace.o:	  /usr/include/fcntl.h
cache-trace.o:	  cacheconst.h
fd_gets.o:	  fd_gets.c
fd_gets.o:	  /usr/include/stdio.h /usr/include/assert.h
ports.o:	  ports.c dis.h opcode_fmt.h resrc_list.h
ports.o:	  /usr/include/values.h
to_native.o:	  to_native.c resrc_offset.h
simul_shm.o:	  simul_shm.c simul.h
simul_shm.o:	  /usr/include/stdio.h /usr/include/sys/shm.h
simul_shm.o:	  /usr/include/sys/ipc.h
record.o:	  record.c
record.o:	  /usr/include/stdio.h /usr/include/assert.h
record.o:	  dis.h vliw.h opcode_fmt.h resrc_list.h max_resrcs.h rename.h
re-exec.o:	  re-exec.c
re-exec.o:	  /usr/include/stdio.h /usr/include/assert.h
re-exec.o:	  /usr/include/fcntl.h /usr/include/unistd.h
group_exit.o:	  group_exit.c
group_exit.o:	  /usr/include/stdio.h /usr/include/assert.h
group_exit.o:	  /usr/include/values.h
group_exit.o:	  vliw.h max_resrcs.h resrc_list.h dis.h rename.h opcode_fmt.h
pdf.o:	          pdf.c
pdf.o:	          brcorr.h vliw.h max_resrcs.h resrc_list.h dis.h rename.h
pdf.o:	          opcode_fmt.h
pdf.o:		  /usr/include/stdio.h /usr/include/assert.h
pdf.o:		  /usr/include/fcntl.h /usr/include/values.h
loophdr.o:	  loophdr.c
loophdr.o:	  /usr/include/stdio.h /usr/include/assert.h
loophdr.o:	  /usr/include/values.h
loophdr.o:	  opcode_fmt.h resrc_list.h
loadver.o:	  loadver.c
loadver.o:	  /usr/include/stdio.h /usr/include/assert.h
loadver.o:	  resrc_offset.h
fmr_commit.o:	  loadver.c
fmr_commit.o:	  vliw.h dis.h dis_tbl.h resrc_map.h gen_resrc.h resrc_offset.h
fmr_commit.o:	  max_resrcs.h resrc_list.h rename.h opcode_fmt.h
fmr_commit.o:	  /usr/include/stdio.h /usr/include/assert.h
fmr_commit.o:	  /usr/include/values.h
cluster.o:	  /usr/include/stdio.h /usr/include/assert.h
cluster.o:	  /usr/include/values.h
cluster.o:	  cluster.c
cluster.o:	  vliw.h dis.h resrc_list.h opcode_fmt.h max_resrcs.h
cluster.o:	  rename.h
addic0.o:	  /usr/include/stdio.h /usr/include/assert.h
addic0.o:	  /usr/include/values.h
addic0.o:	  addic0.c
addic0.o:	  vliw.h dis.h resrc_list.h opcode_fmt.h max_resrcs.h
addic0.o:	  rename.h
rz0.o:		  /usr/include/stdio.h /usr/include/assert.h
rz0.o:		  /usr/include/values.h
rz0.o:		  rz0.c
rz0.o:		  vliw.h dis.h dis_tbl.h resrc_list.h opcode_fmt.h max_resrcs.h
rz0.o:		  rename.h
rlwimi.o:	  /usr/include/stdio.h /usr/include/assert.h
rlwimi.o:	  /usr/include/values.h
rlwimi.o:	  rlwimi.c
rlwimi.o:	  vliw.h dis.h dis_tbl.h resrc_list.h opcode_fmt.h max_resrcs.h
rlwimi.o:	  rename.h
del_dead.o:	  /usr/include/stdio.h /usr/include/assert.h
del_dead.o:	  /usr/include/values.h
del_dead.o:	  del_dead.c
del_dead.o:	  vliw.h dis.h resrc_list.h opcode_fmt.h max_resrcs.h
del_dead.o:	  rename.h
resrc.o:	  /usr/include/stdio.h /usr/include/assert.h
resrc.o:	  /usr/include/values.h
resrc.o:	  resrc.c
resrc.o:	  vliw.h dis.h resrc_list.h opcode_fmt.h max_resrcs.h
resrc.o:	  rename.h
tree_pos.o:	  /usr/include/stdio.h /usr/include/assert.h
tree_pos.o:	  /usr/include/values.h
tree_pos.o:	  tree_pos.c
tree_pos.o:	  vliw.h dis.h resrc_list.h opcode_fmt.h max_resrcs.h
tree_pos.o:	  rename.h unify.h
rename.o:	  /usr/include/stdio.h /usr/include/assert.h
rename.o:	  /usr/include/values.h
rename.o:	  rename.c
rename.o:	  vliw.h dis.h resrc_list.h opcode_fmt.h max_resrcs.h
rename.o:	  rename.h unify.h vliw-macros.h
unify.o:	  /usr/include/stdio.h /usr/include/assert.h
unify.o:	  /usr/include/values.h
unify.o:	  unify.c
unify.o:	  vliw.h dis.h resrc_list.h opcode_fmt.h max_resrcs.h
unify.o:	  rename.h unify.h
offset.o:	  /usr/include/stdio.h /usr/include/assert.h
offset.o:	  /usr/include/values.h
offset.o:	  offset.c
offset.o:	  dis.h resrc_list.h opcode_fmt.h offset.h
