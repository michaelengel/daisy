.set r3,3;

	.globl	.get_1st_one

	.csect	H_get_1st_one{PR}      

.get_1st_one:
	cntlz	r3,r3
	bcr	20,0
