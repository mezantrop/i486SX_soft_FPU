/*
 * expediant "port" of linux 8087 emulator to 386BSD, with apologies -wfj
 */

/*
 * linux/kernel/math/math_emulate.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * Limited emulation 27.12.91 - mostly loads/stores, which gcc wants
 * even for soft-float, UNless you use bruce evans' patches. The patches
 * are great, but they have to be re-applied for every version, and the
 * library is different for soft-float and 80387. So emulation is more
 * practical, even though it's slower.
 *
 * 28.12.91 - loads/stores work, even BCD. I'll have to start thinking
 * about add/sub/mul/div. Urgel. I should find some good source, but I'll
 * just fake up something.
 *
 * 30.12.91 - add/sub/mul/div/com seem to work mostly. I should really
 * test every possible combination.
 */

/*
 * This file is full of ugly macros etc: one problem was that gcc simply
 * didn't want to make the structures as they should be: it has to try to
 * align them. Sickening code, but at least I've hidden the ugly things
 * in this one file: the other files don't need to know about these things.
 *
 * The other files also don't care about ST(x) etc - they just get addresses
 * to 80-bit temporary reals, and do with them as they please. I wanted to
 * hide most of the 387-specific things here.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: math_emulate.c,v 1.32 2007/01/24 13:08:13 hubertf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
/* #include <sys/user.h> */
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <machine/cpu.h>
#include <machine/reg.h>

#ifdef USERMODE
	#define ORIGINAL_USERMODE USERMODE
	#undef USERMODE
#endif
#define USERMODE(c, f) (ISPL(c) == SEL_UPL || ((f) & PSL_VM) != 0)

#define __ALIGNED_TEMP_REAL 1
#include <i386/i386/math_emu.h>

#define ST(x) (*__st((x)))
#define PST(x) ((const temp_real *) __st((x)))
#define	math_abort(tfp, ksi, signo, code) 	\
    do {					\
	    KSI_INIT_TRAP(ksi);			\
	    ksi->ksi_signo = signo;		\
	    ksi->ksi_code = code;		\
	    ksi->ksi_addr = (void *)info->tf_eip;\
	    tfp->tf_eip = oldeip;		\
	    return -1;				\
    } while (/*CONSTCOND*/0)

/*
 * We don't want these inlined - it gets too messy in the machine-code.
 */
static void fpop(void);
static void fpush(void);
static void fxchg(temp_real_unaligned * a, temp_real_unaligned * b);
static temp_real_unaligned * __st(int i);


int32_t fubyte(const void *addr);
int32_t fusword(const void *addr);
int32_t fuword(const void *addr);
int32_t subyte(void *addr, int value);
int32_t susword(void *addr, int value);
int32_t suword(void *addr, int value);

void dump_fpustack(void);


#define	fninit()	do { \
	I387.cwd = __INITIAL_NPXCW__;	\
	I387.swd = 0x0000;		\
	I387.twd = 0x0000;		\
} while (0)

int
math_emulate(struct trapframe *info, ksiginfo_t *ksi) {
	struct lwp *l = curlwp;
	uint16_t cw, code;
	temp_real tmp;
	char * address;
	u_long oldeip;
	int32_t override_seg, override_addrsize, override_datasize;
	int32_t prefix;

	static int instruction_counter = 0;  /* Global instruction counter */

	printf("\n========== FPU INSTRUCTION #%d ==========\n",
		instruction_counter++);

	u_char *instr = (u_char *)info->tf_eip; 
	printf("math_emulate: DEBUG: opcode=0x%02x 0x%02x at EIP=0x%08x\n",
		instr[0], instr[1], info->tf_eip);


	override_seg = override_addrsize = override_datasize = 0;
	override_seg++;
	override_seg--;

	if (!USERMODE(info->tf_cs, info->tf_eflags))
		panic("math emulator called from supervisor mode");

	/* ever used fp? */
	if ((l->l_md.md_flags & MDL_USEDFPU) == 0) {
		struct pcb *pcb = lwp_getpcb(l);
		
		if (i386_use_fxsave)
			cw = pcb->pcb_savefpu.sv_xmm.fx_cw;
		else 
			cw = pcb->pcb_savefpu.sv_87.s87_cw;

		fninit();
		I387.cwd = cw;
		l->l_md.md_flags |= MDL_USEDFPU;
	}

	if (I387.cwd & I387.swd & 0x3f)
		I387.swd |= 0x8000;
	else
		I387.swd &= 0x7fff;

	I387.fip = oldeip = info->tf_eip;

	/*
	 * Scan for instruction prefixes. More to be politically correct
	 * than anything else. Prefixes aren't useful for the instructions
	 * we can emulate anyway.
	 */
        
	while (1) {
		prefix = fubyte((const void *)info->tf_eip);
		switch (prefix) {
		case INSPREF_LOCK:
			math_abort(info, ksi, SIGILL, ILL_ILLOPC);
		case INSPREF_REPN:
		case INSPREF_REPE:
			break;
		case INSPREF_CS:
		case INSPREF_SS:
		case INSPREF_DS:
		case INSPREF_ES:
		case INSPREF_FS:
		case INSPREF_GS:
			override_seg = prefix;
			break;
		case INSPREF_OSIZE:
			override_datasize = prefix;	
			break;
		case INSPREF_ASIZE:
			override_addrsize = prefix;
			break;
		case -1:
			math_abort(info, ksi, SIGSEGV, SEGV_ACCERR);
		default:
			goto done;
		}
		info->tf_eip++;
	}

done:
	code = htons(fusword((uint16_t *) info->tf_eip)) & 0x7ff;

	printf("DEBUG: CODE: 0x%04x\n", code);

	info->tf_eip += 2;
	*((uint16_t *) &I387.fcs) = (uint16_t) info->tf_cs;
	*((uint16_t *) &I387.fcs + 1) = code;

	switch (code) {
	case 0x1d0: /* fnop */
		return(0);
	case 0x1e0: /* fchs */
		ST(0).exponent ^= 0x8000;
		return(0);
	case 0x1e1: /* fabs */
		ST(0).exponent &= 0x7fff;
		return(0);
	case 0x1e4: /* fxtract XXX */
		ftst(PST(0));
		return(0);
	case 0x1e8: /* fld1 */
		fpush();
		ST(0) = CONST1;
		return(0);
	case 0x1e9: /* fld2t */
		fpush();
		ST(0) = CONSTL2T;
		return(0);
	case 0x1ea: /* fld2e */
		fpush();
		ST(0) = CONSTL2E;
		return(0);
	case 0x1eb: /* fldpi */
		fpush();
		ST(0) = CONSTPI;
		return(0);
	case 0x1ec: /* fldlg2 */
		fpush();
		ST(0) = CONSTLG2;
		return(0);
	case 0x1ed: /* fldln2 */
		fpush();
		ST(0) = CONSTLN2;
		return(0);
	case 0x1ee: /* fldz */
		fpush();
		ST(0) = CONSTZ;
		return(0);
	case 0x1fc: /* frndint */
		frndint(PST(0),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 0x1fd: /* fscale */
		/* incomplete and totally inadequate -wfj */
		Fscale(PST(0), PST(1), &tmp);
		real_to_real(&tmp,&ST(0));
		return(0);			/* 19 Sep 92*/
	case 0x2e9: /* XXX */
		fucom(PST(1),PST(0));
		fpop(); fpop();
		return(0);
	case 0x3d0: case 0x3d1: /* XXX */
		return(0);
	case 0x3e2: /* fclex */
		I387.swd &= 0x7f00;
		return(0);
	case 0x3e3: /* fninit */
		fninit();
		return(0);
	case 0x3e4: /* XXX */
		return(0);
	case 0x6d9: /* fcompp */
		fcom(PST(1),PST(0));
		fpop(); fpop();
		return(0);
	case 0x7e0: /* fstsw ax */
		*(u_short *) &info->tf_eax = I387.swd;
		return(0);
	case 0x1d1: case 0x1d2: case 0x1d3:
	case 0x1d4: case 0x1d5: case 0x1d6: case 0x1d7:
	case 0x1e2: case 0x1e3:
	case 0x1e6: case 0x1e7:
	case 0x1ef:
		math_abort(info, ksi, SIGILL, ILL_ILLOPC);
	case 0x1e5:
		uprintf("math_emulate: fxam not implemented\n\r");
		math_abort(info, ksi, SIGILL, ILL_ILLOPC);
	case 0x1f0: case 0x1f1: case 0x1f2: case 0x1f3:
	case 0x1f4: case 0x1f5: case 0x1f6: case 0x1f7:
	case 0x1f8: case 0x1f9: case 0x1fa: case 0x1fb:
	case 0x1fe: case 0x1ff:
		uprintf("math_emulate: 0x%04x not implemented\n",
		    code + 0xd800);
		math_abort(info, ksi, SIGILL, ILL_ILLOPC);
	}

	printf("DEBUG: CODE >> 3: 0x%04x\n", code >> 3);

	switch (code >> 3) {
	case 0x18: /* fadd */
		fadd(PST(0),PST(code & 7),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 0x19: /* fmul */
		fmul(PST(0),PST(code & 7),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 0x1a: /* fcom */
		fcom(PST(code & 7),PST(0));
		return(0);
	case 0x1b: /* fcomp */
		fcom(PST(code & 7),PST(0));
		fpop();
		return(0);
	case 0x1c: /* fsubr */
		real_to_real(&ST(code & 7),&tmp);
		tmp.exponent ^= 0x8000;
		fadd(PST(0),&tmp,&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 0x1d: /* fsub */
		ST(0).exponent ^= 0x8000;
		fadd(PST(0),PST(code & 7),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 0x1e: /* fdivr */
		fdiv(PST(0),PST(code & 7),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 0x1f: /* fdiv */
		fdiv(PST(code & 7),PST(0),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 0x38: /* fld */
        	printf("DEBUG: === START FLD ===\n");

        	fpush();
        	ST(0) = ST(code & 7);

        	printf("DEBUG: === END FLD ===\n");

		return(0);
	case 0x39: /* fxch */
		fxchg(&ST(0),&ST(code & 7));
		return(0);
	case 0x3b: /* XXX */
		ST(code & 7) = ST(0);
		fpop();
		return(0);
	case 0x98: /* fadd */
        	printf("DEBUG: === START FADD ===\n");
        	dump_fpustack();  // Print FPU state before fadd

        	fadd(PST(0), PST(code & 7), &tmp);
        	real_to_real(&tmp, &ST(code & 7));

        	dump_fpustack();  // Print FPU state after fadd
        	printf("DEBUG: === END FADD ===\n");

		return(0);
	case 0x99: /* fmul */
		fmul(PST(0),PST(code & 7),&tmp);
		real_to_real(&tmp,&ST(code & 7));
		return(0);
	case 0x9a: /* XXX */
		fcom(PST(code & 7),PST(0));
		return(0);
	case 0x9b: /* XXX */
		fcom(PST(code & 7),PST(0));
		fpop();
		return(0);			
	case 0x9c: /* fsubr */
		ST(code & 7).exponent ^= 0x8000;
		fadd(PST(0),PST(code & 7),&tmp);
		real_to_real(&tmp,&ST(code & 7));
		return(0);
	case 0x9d: /* fsub */
		real_to_real(&ST(0),&tmp);
		tmp.exponent ^= 0x8000;
		fadd(PST(code & 7),&tmp,&tmp);
		real_to_real(&tmp,&ST(code & 7));
		return(0);
	case 0x9e: /* fdivr */
		fdiv(PST(0),PST(code & 7),&tmp);
		real_to_real(&tmp,&ST(code & 7));
		return(0);
	case 0x9f: /* fdiv */
		fdiv(PST(code & 7),PST(0),&tmp);
		real_to_real(&tmp,&ST(code & 7));
		return(0);
	case 0xb8: /* ffree */
		printf("ffree not implemented\n\r");
		math_abort(info, ksi, SIGILL, ILL_ILLOPC);
	case 0xb9: /* fstp XXX */

        	printf("DEBUG: === START FSTP ===\n");
        	dump_fpustack();

        	fxchg(&ST(0), &ST(code & 7));

        	dump_fpustack();
        	printf("DEBUG: === END FSTP ===\n");

		return(0);
	case 0xba: /* fst */

        	printf("DEBUG: === START FST ===\n");
        	dump_fpustack();

        	ST(code & 7) = ST(0);

        	dump_fpustack();
        	printf("DEBUG: === END FST ===\n");

		return(0);
	case 0xbb: /* XXX */
		ST(code & 7) = ST(0);
		fpop();
		return(0);
	case 0xbc: /* fucom */
		fucom(PST(code & 7),PST(0));
		return(0);
	case 0xbd: /* fucomp */
		fucom(PST(code & 7),PST(0));
		fpop();
		return(0);
	case 0xd8: /* faddp */

        	printf("DEBUG: === START FADDP ===\n");
        	dump_fpustack();

        	printf("DEBUG: faddp target register: ST(%d)\n", code & 7);

        	fadd(PST(code & 7), PST(0), &tmp);
        	real_to_real(&tmp, &ST(code & 7));
        	fpop();

        	dump_fpustack();

        	temp_real_unaligned *tmp_unaligned = (temp_real_unaligned *)&tmp;
        	printf("DEBUG: Temporary result from fadd:\n");
        	printf("Exponent: %04x, Significand: %04x %04x %04x %04x\n",
               		tmp_unaligned->exponent, tmp_unaligned->m3,
               		tmp_unaligned->m2, tmp_unaligned->m1, tmp_unaligned->m0);

        	printf("DEBUG: === END FADDP ===\n");

		return(0);
	case 0xd9: /* fmulp */
		fmul(PST(code & 7),PST(0),&tmp);
		real_to_real(&tmp,&ST(code & 7));
		fpop();
		return(0);
	case 0xda: /* XXX */
		fcom(PST(code & 7),PST(0));
		fpop();
		return(0);
	case 0xdc: /* fsubrp */
		ST(code & 7).exponent ^= 0x8000;
		fadd(PST(0),PST(code & 7),&tmp);
		real_to_real(&tmp,&ST(code & 7));
		fpop();
		return(0);
	case 0xdd: /* fsubp */
		real_to_real(&ST(0),&tmp);
		tmp.exponent ^= 0x8000;
		fadd(PST(code & 7),&tmp,&tmp);
		real_to_real(&tmp,&ST(code & 7));
		fpop();
		return(0);
	case 0xde: /* fdivrp */
		fdiv(PST(0),PST(code & 7),&tmp);
		real_to_real(&tmp,&ST(code & 7));
		fpop();
		return(0);
	case 0xdf: /* fdivp */
		fdiv(PST(code & 7),PST(0),&tmp);
		real_to_real(&tmp,&ST(code & 7));
		fpop();
		return(0);
	case 0xf8: /* XXX */
		printf("ffree not implemented\n\r");
		math_abort(info, ksi, SIGILL, ILL_ILLOPC);
	case 0xf9: /* XXX */
		fxchg(&ST(0),&ST(code & 7));
		return(0);
	case 0xfa: /* XXX */
	case 0xfb: /* XXX */
		ST(code & 7) = ST(0);
		fpop();
		return(0);
	}

	printf("DEBUG: (CODE >> 3) & 0xe7: 0x%04x\n", (code >> 3) & 0xe7);

	switch ((code>>3) & 0xe7) {
	case 0x22:
		put_short_real(PST(0),info,code);

		dump_fpustack();		/* DEBUG */

		return(0);
	case 0x23:
		put_short_real(PST(0),info,code);
		fpop();

		dump_fpustack();		/* DEBUG */

		return(0);
	case 0x24:
		address = ea(info,code);
		copyin((u_long *) address, (u_long *) &I387, 28);
		return(0);
	case 0x25:
		address = ea(info,code);
		*(u_short *) &I387.cwd =
			fusword((u_short *) address);
		return(0);
	case 0x26:
		address = ea(info,code);
		copyout((u_long *) &I387, (u_long *) address, 28);
		return(0);
	case 0x27:
		address = ea(info,code);
		susword((u_short *) address, I387.cwd);
		return(0);
	case 0x62:
		put_long_int(PST(0),info,code);
		return(0);
	case 0x63:
		put_long_int(PST(0),info,code);
		fpop();
		return(0);
	case 0x65:
		/* 1. Get real from mem (var) -> tmp
		   2. Save tmp into FPU register 0 */
		printf("DEBUG: === START 0x65 ===\n");

		fpush();
		get_temp_real(&tmp,info,code);
		real_to_real(&tmp,&ST(0));

		printf("DEBUG: === END 0x65 ===\n");

		return(0);
	case 0x67:
		put_temp_real(PST(0),info,code);
		fpop();
		return(0);
	case 0xa2:
		put_long_real(PST(0),info,code);
		return(0);
	case 0xa3:
		put_long_real(PST(0),info,code);
		fpop();
		return(0);
	case 0xa4:
		address = ea(info,code);
		copyin((u_long *) address, (u_long *) &I387, 108);
		return(0);
	case 0xa6:
		address = ea(info,code);
		copyout((u_long *) &I387, (u_long *) address, 108);
		fninit();
		return(0);
	case 0xa7:
		address = ea(info,code);
		susword((u_short *) address, I387.swd);
		return(0);
	case 0xe2:
		put_short_int(PST(0),info,code);
		return(0);
	case 0xe3:
		put_short_int(PST(0),info,code);
		fpop();
		return(0);
	case 0xe4:
		fpush();
		get_BCD(&tmp,info,code);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 0xe5:
		fpush();
		get_longlong_int(&tmp,info,code);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 0xe6:
		put_BCD(PST(0),info,code);
		fpop();
		return(0);
	case 0xe7:
		put_longlong_int(PST(0),info,code);
		fpop();
		return(0);
	}

	printf("DEBUG: (CODE >> 9): 0x%04x\n", code >> 9);

	switch (code >> 9) {
	case 0:
		printf("DEBUG: Calling get_short_real()\n");

		get_short_real(&tmp,info,code);

		printf("DEBUG: Returned from get_short_real()\n");

		break;
	case 1:
		get_long_int(&tmp,info,code);
		break;
	case 2:
		printf("DEBUG: Calling get_long_real()\n");

		get_long_real(&tmp,info,code);

		printf("DEBUG: Returned from get_long_real()\n");

		break;
	case 4:
		get_short_int(&tmp,info,code);
	}

	printf("DEBUG: (CODE >> 3) & 0x27): 0x%04x\n", (code>>3) & 0x27);

	switch ((code>>3) & 0x27) {
	case 0:
		fadd(&tmp,PST(0),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 1:
		fmul(&tmp,PST(0),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 2:
		fcom(&tmp,PST(0));
		return(0);
	case 3:
		fcom(&tmp,PST(0));
		fpop();
		return(0);
	case 4:
		tmp.exponent ^= 0x8000;
		fadd(&tmp,PST(0),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 5:
		ST(0).exponent ^= 0x8000;
		fadd(&tmp,PST(0),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 6:
		fdiv(PST(0),&tmp,&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	case 7:
		fdiv(&tmp,PST(0),&tmp);
		real_to_real(&tmp,&ST(0));
		return(0);
	}

	printf("DEBUG: (CODE & 0x138): 0x%04x\n", code & 0x138);

	if ((code & 0x138) == 0x100) {
		fpush();
		real_to_real(&tmp,&ST(0));
		return(0);
	}
	printf("Unknown math-insns: %04x:%08x %04x\n\r",(u_short)info->tf_cs,
		info->tf_eip,code);
	math_abort(info, ksi, SIGFPE, FPE_FLTINV);
}

static void fpop(void)
{
    uint32_t top = (I387.swd >> 11) & 7;
    top = (top + 1) & 7;
    I387.swd = (I387.swd & ~0x00003800) | (top << 11);
}

static void fpush(void)
{
    uint32_t top = (I387.swd >> 11) & 7;
    top = (top - 1) & 7;
    I387.swd = (I387.swd & ~0x00003800) | (top << 11);
}


/*
static void fpop(void)
{
	uint32_t tmp;

	tmp = I387.swd & 0xffffc7ff;
	I387.swd += 0x00000800;
	I387.swd &= 0x00003800;
	I387.swd |= tmp;
}

static void fpush(void)
{
	uint32_t tmp;

	tmp = I387.swd & 0xffffc7ff;
	I387.swd -= 0x00000800;
	I387.swd &= 0x00003800;
	I387.swd |= tmp;
}
*/

static void fxchg(temp_real_unaligned * a, temp_real_unaligned * b)
{
	temp_real_unaligned c;

	c = *a;
	*a = *b;
	*b = c;
}

static temp_real_unaligned * __st(int i)
{
	i += I387.swd >> 11;
	i &= 7;
	return (temp_real_unaligned *) (i*10 + (char *)(I387.st_space));
}


/* Fetch/store wrappers */

int fubyte(const void *addr) {
    uint8_t value;

    if (ufetch_8(addr, &value) != 0) {
        printf("fubyte: read error at address %p\n", addr);
        return -1;
    }

    return (int)value;
}

int fusword(const void *addr) {
	uint16_t value;

	if (ufetch_16(addr, &value) != 0)
		return -1;

	return (int)value;
}

int fuword(const void *addr) {
	uint32_t value;

	if (ufetch_32(addr, &value) != 0)
		return -1;

	return (int)value;
}

int subyte(void *addr, int value) {
    if (ustore_8(addr, (uint8_t)value) != 0) {
        printf("subyte: write error for value 0x%02X at address %p\n", value, addr);
        return -1;
    }

    printf("subyte: wrote value 0x%02X at address %p\n", value, addr);
    return 0;
}

int susword(void *addr, int value) {
	return ustore_16(addr, (uint16_t)value);
}

int suword(void *addr, int value) {
	return ustore_32(addr, (uint32_t)value);
}

/*
 * linux/kernel/math/ea.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * Calculate the effective address.
 */


static int __regoffset[] = {
	tEAX, tECX, tEDX, tEBX, tESP, tEBP, tESI, tEDI
};

#define REG(x) (((int *)curlwp->l_md.md_regs)[__regoffset[(x)]])

static char * sib(struct trapframe * info, int mod)
{
	u_char ss,index,base;
	int32_t offset = 0;

	base = fubyte((char *) info->tf_eip);
	info->tf_eip++;
	ss = base >> 6;
	index = (base >> 3) & 7;
	base &= 7;
	if (index == 4)
		offset = 0;
	else
		offset = REG(index);
	offset <<= ss;
	if (mod || base != 5)
		offset += REG(base);
	if (mod == 1) {
		offset += (signed char) fubyte((char *) info->tf_eip);
		info->tf_eip++;
	} else if (mod == 2 || base == 5) {
		offset += (signed) fuword((u_long *) info->tf_eip);
		info->tf_eip += 4;
	}
	I387.foo = offset;
	I387.fos = 0x17;
	return (char *) offset;
}

char * ea(struct trapframe * info, u_short code)
{
	u_char mod,rm;
	int32_t* tmp;
	int offset = 0;

	mod = (code >> 6) & 3;
	rm = code & 7;
	if (rm == 4 && mod != 3)
		return sib(info,mod);
	if (rm == 5 && !mod) {
		offset = fuword((u_long *) info->tf_eip);
		info->tf_eip += 4;
		I387.foo = offset;
		I387.fos = 0x17;
		return (char *) offset;
	}
	tmp = (int32_t*)&REG(rm);
	switch (mod) {
		case 0: offset = 0; break;
		case 1:
			offset = (signed char) fubyte((char *) info->tf_eip);
			info->tf_eip++;
			break;
		case 2:
			offset = (signed) fuword((u_long *) info->tf_eip);
			info->tf_eip += 4;
			break;
#ifdef notyet
		case 3:
			math_abort(info, ksi, SIGILL, ILL_ILLOPN);
#endif
	}
	
	printf("DEBUG: ea(): mod: %d, rm: %d, offset: %x\n", mod, rm, offset);

	I387.foo = offset;
	I387.fos = 0x17;
	return offset + (char *) *tmp;
}
/*
 * linux/kernel/math/get_put.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * This file handles all accesses to user memory: getting and putting
 * ints/reals/BCD etc. This is the only part that concerns itself with
 * other than temporary real format. All other cals are strictly temp_real.
 */

void get_short_real(temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;
	short_real sr;

	addr = ea(info,code);

	printf("DEBUG: get_short_real(): ea() -> addr: %p\n", addr);

	sr = fuword((u_long *) addr);

	printf("DEBUG: get_short_real(): fuword() -> %x\n", sr);

	short_to_temp(&sr,tmp);

    	printf("DEBUG: tmp after short_to_temp(): exponent: %04x, significand: %08x %08x\n",
        	tmp->exponent, tmp->a, tmp->b);
}

void get_long_real(temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;
	long_real lr;

	addr = ea(info,code);

	printf("DEBUG: get_long_real(): ea() -> addr: %p\n", addr);

	lr.a = fuword((u_long *) addr);
	lr.b = fuword((u_long *) addr + 1);

	printf("DEBUG: get_long_real(): fuword() -> lr.a: %x\n", lr.a);
	printf("DEBUG: get_long_real(): fuword() -> lr.b: %x\n", lr.b);

	long_to_temp(&lr,tmp);

	printf("DEBUG: tmp after long_to_temp(): exponent: %04x, significand: %08x %08x\n",
		tmp->exponent, tmp->a, tmp->b);
}

void get_temp_real(temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;

	addr = ea(info,code);

	printf("DEBUG: get_temp_real(): ea() -> addr: %p\n", addr);

	tmp->b = fuword((u_long *) addr);
	tmp->a = fuword((u_long *) addr + 1);

	tmp->exponent = fusword((u_short *) addr + 4);

	printf("DEBUG: get_temp_real(): fuword() -> tmp->a: %x\n", tmp->a);
	printf("DEBUG: get_temp_real(): fuword() -> tmp->b: %x\n", tmp->b);
	printf("DEBUG: get_temp_real(): fuword() -> tmp->exponent: %x\n",
		tmp->exponent);
}

void get_short_int(temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);
	ti.a = (signed short) fusword((u_short *) addr);
	ti.b = 0;
	if ((ti.sign = (ti.a < 0)) != 0)
		ti.a = - ti.a;
	int_to_real(&ti,tmp);
}

void get_long_int(temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);
	ti.a = fuword((u_long *) addr);
	ti.b = 0;
	if ((ti.sign = (ti.a < 0)) != 0)
		ti.a = - ti.a;
	int_to_real(&ti,tmp);
}

void get_longlong_int(temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);
	ti.a = fuword((u_long *) addr);
	ti.b = fuword((u_long *) addr + 1);
	if ((ti.sign = (ti.b < 0)) != 0)
		__asm("notl %0 ; notl %1\n\t"
			"addl $1,%0 ; adcl $0,%1"
			:"=r" (ti.a),"=r" (ti.b)
			:"0" (ti.a),"1" (ti.b));
	int_to_real(&ti,tmp);
}

#define MUL10(low,high) \
__asm("addl %0,%0 ; adcl %1,%1\n\t" \
"movl %0,%%ecx ; movl %1,%%ebx\n\t" \
"addl %0,%0 ; adcl %1,%1\n\t" \
"addl %0,%0 ; adcl %1,%1\n\t" \
"addl %%ecx,%0 ; adcl %%ebx,%1" \
:"=a" (low),"=d" (high) \
:"0" (low),"1" (high):"cx","bx")

#define ADD64(val,low,high) \
__asm("addl %4,%0 ; adcl $0,%1":"=r" (low),"=r" (high) \
:"0" (low),"1" (high),"r" ((u_long) (val)))

void get_BCD(temp_real * tmp, struct trapframe * info, u_short code)
{
	int k;
	char * addr;
	temp_int i;
	u_char c;

	addr = ea(info,code);
	addr += 9;
	i.sign = 0x80 & fubyte(addr--);
	i.a = i.b = 0;
	for (k = 0; k < 9; k++) {
		c = fubyte(addr--);
		MUL10(i.a, i.b);
		ADD64((c>>4), i.a, i.b);
		MUL10(i.a, i.b);
		ADD64((c&0xf), i.a, i.b);
	}
	int_to_real(&i,tmp);
}

void put_short_real(const temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;
	short_real sr;

	short_real tsr;		/* DEBUG */

	addr = ea(info,code);
	
	tsr = fuword((u_long *) addr);
	printf("DEBUG: put_short_real(): before temp_to_short() -> %x\n", tsr);

	temp_to_short(tmp,&sr);

	printf("DEBUG: put_short_real(): temp_to_short() -> %x\n", sr);

	suword((u_long *) addr,sr);

	tsr = fuword((u_long *) addr);
	printf("DEBUG: put_short_real(): suword() -> %x\n", tsr);

}

void put_long_real(const temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;
	long_real lr;

	addr = ea(info,code);
	temp_to_long(tmp,&lr);
	suword((u_long *) addr, lr.a);
	suword((u_long *) addr + 1, lr.b);
}

void put_temp_real(const temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;

	addr = ea(info,code);
	suword((u_long *) addr, tmp->a);
	suword((u_long *) addr + 1, tmp->b);
	susword((u_short *) addr + 4, tmp->exponent);
}

void put_short_int(const temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);
	real_to_int(tmp,&ti);
	if (ti.sign)
		ti.a = -ti.a;
	susword((u_short *) addr,ti.a);
}

void put_long_int(const temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);
	real_to_int(tmp,&ti);
	if (ti.sign)
		ti.a = -ti.a;
	suword((u_long *) addr, ti.a);
}

void put_longlong_int(const temp_real * tmp,
	struct trapframe * info, u_short code)
{
	char * addr;
	temp_int ti;

	addr = ea(info,code);
	real_to_int(tmp,&ti);
	if (ti.sign)
		__asm("notl %0 ; notl %1\n\t"
			"addl $1,%0 ; adcl $0,%1"
			:"=r" (ti.a),"=r" (ti.b)
			:"0" (ti.a),"1" (ti.b));
	suword((u_long *) addr, ti.a);
	suword((u_long *) addr + 1, ti.b);
}

#define DIV10(low,high,rem) \
__asm("divl %6 ; xchgl %1,%2 ; divl %6" \
	:"=d" (rem),"=a" (low),"=r" (high) \
	:"0" (0),"1" (high),"2" (low),"c" (10))

void put_BCD(const temp_real * tmp,struct trapframe * info, u_short code)
{
	int32_t k,rem;
	char * addr;
	temp_int i;
	u_char c;

	addr = ea(info,code);
	real_to_int(tmp,&i);
	if (i.sign)
		subyte(addr+9,0x80);
	else
		subyte(addr+9,0x00);
	for (k = 0; k < 9; k++) {
		DIV10(i.a,i.b,rem);
		c = rem;
		DIV10(i.a,i.b,rem);
		c += rem<<4;
		subyte(addr++,c);
	}
}

/*
 * linux/kernel/math/mul.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * temporary real multiplication routine.
 */


static void shift(int * c)
{
	__asm("movl (%0),%%eax ; addl %%eax,(%0)\n\t"
		"movl 4(%0),%%eax ; adcl %%eax,4(%0)\n\t"
		"movl 8(%0),%%eax ; adcl %%eax,8(%0)\n\t"
		"movl 12(%0),%%eax ; adcl %%eax,12(%0)"
		::"r" ((long) c):"ax");
}

static void mul64(const temp_real * a, const temp_real * b, int * c)
{
	__asm("movl (%0),%%eax\n\t"
		"mull (%1)\n\t"
		"movl %%eax,(%2)\n\t"
		"movl %%edx,4(%2)\n\t"
		"movl 4(%0),%%eax\n\t"
		"mull 4(%1)\n\t"
		"movl %%eax,8(%2)\n\t"
		"movl %%edx,12(%2)\n\t"
		"movl (%0),%%eax\n\t"
		"mull 4(%1)\n\t"
		"addl %%eax,4(%2)\n\t"
		"adcl %%edx,8(%2)\n\t"
		"adcl $0,12(%2)\n\t"
		"movl 4(%0),%%eax\n\t"
		"mull (%1)\n\t"
		"addl %%eax,4(%2)\n\t"
		"adcl %%edx,8(%2)\n\t"
		"adcl $0,12(%2)"
		::"S" ((long) a),"c" ((long) b),"D" ((long) c)
		:"ax","dx");
}

void fmul(const temp_real * src1, const temp_real * src2, temp_real * result)
{
	int32_t i,sign;
	int32_t tmp[4] = {0,0,0,0};

	sign = (src1->exponent ^ src2->exponent) & 0x8000;
	i = (src1->exponent & 0x7fff) + (src2->exponent & 0x7fff) - 16383 + 1;
	if (i<0) {
		result->exponent = sign;
		result->a = result->b = 0;
		return;
	}
	if (i>0x7fff) {
		set_OE();
		return;
	}
	mul64(src1,src2,tmp);
	if (tmp[0] || tmp[1] || tmp[2] || tmp[3])
		while (i && tmp[3] >= 0) {
			i--;
			shift(tmp);
		}
	else
		i = 0;
	result->exponent = i | sign;
	result->a = tmp[2];
	result->b = tmp[3];
}

/*
 * linux/kernel/math/div.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * temporary real division routine.
 */

static void shift_left(int * c)
{
	__asm volatile("movl (%0),%%eax ; addl %%eax,(%0)\n\t"
		"movl 4(%0),%%eax ; adcl %%eax,4(%0)\n\t"
		"movl 8(%0),%%eax ; adcl %%eax,8(%0)\n\t"
		"movl 12(%0),%%eax ; adcl %%eax,12(%0)"
		::"r" ((long) c):"ax");
}

static void shift_right(int * c)
{
	__asm("shrl $1,12(%0) ; rcrl $1,8(%0) ; rcrl $1,4(%0) ; rcrl $1,(%0)"
		::"r" ((long) c));
}

static int try_sub(int * a, int * b)
{
	char ok;

	__asm volatile("movl (%1),%%eax ; subl %%eax,(%2)\n\t"
		"movl 4(%1),%%eax ; sbbl %%eax,4(%2)\n\t"
		"movl 8(%1),%%eax ; sbbl %%eax,8(%2)\n\t"
		"movl 12(%1),%%eax ; sbbl %%eax,12(%2)\n\t"
		"setae %%al":"=a" (ok):"c" ((long) a),"d" ((long) b));
	return ok;
}

static void div64(int * a, int * b, int * c)
{
	int32_t tmp[4];
	int32_t i;
	uint32_t mask = 0;

	c += 4;
	for (i = 0 ; i<64 ; i++) {
		if (!(mask >>= 1)) {
			c--;
			mask = 0x80000000;
		}
		tmp[0] = a[0]; tmp[1] = a[1];
		tmp[2] = a[2]; tmp[3] = a[3];
		if (try_sub(b,tmp)) {
			*c |= mask;
			a[0] = tmp[0]; a[1] = tmp[1];
			a[2] = tmp[2]; a[3] = tmp[3];
		}
		shift_right(b);
	}
}

void fdiv(const temp_real * src1, const temp_real * src2, temp_real * result)
{
	int32_t i,sign;
	int32_t a[4],b[4],tmp[4] = {0,0,0,0};

	sign = (src1->exponent ^ src2->exponent) & 0x8000;
	if (!(src2->a || src2->b)) {
		set_ZE();
		return;
	}
	i = (src1->exponent & 0x7fff) - (src2->exponent & 0x7fff) + 16383;
	if (i<0) {
		set_UE();
		result->exponent = sign;
		result->a = result->b = 0;
		return;
	}
	a[0] = a[1] = 0;
	a[2] = src1->a;
	a[3] = src1->b;
	b[0] = b[1] = 0;
	b[2] = src2->a;
	b[3] = src2->b;
	while (b[3] >= 0) {
		i++;
		shift_left(b);
	}
	div64(a,b,tmp);
	if (tmp[0] || tmp[1] || tmp[2] || tmp[3]) {
		while (i && tmp[3] >= 0) {
			i--;
			shift_left(tmp);
		}
		if (tmp[3] >= 0)
			set_DE();
	} else
		i = 0;
	if (i>0x7fff) {
		set_OE();
		return;
	}
	if (tmp[0] || tmp[1])
		set_PE();
	result->exponent = i | sign;
	result->a = tmp[2];
	result->b = tmp[3];
}

/*
 * linux/kernel/math/add.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * temporary real addition routine.
 *
 * NOTE! These aren't exact: they are only 62 bits wide, and don't do
 * correct rounding. Fast hack. The reason is that we shift right the
 * values by two, in order not to have overflow (1 bit), and to be able
 * to move the sign into the mantissa (1 bit). Much simpler algorithms,
 * and 62 bits (61 really - no rounding) accuracy is usually enough. The
 * only time you should notice anything weird is when adding 64-bit
 * integers together. When using doubles (52 bits accuracy), the
 * 61-bit accuracy never shows at all.
 */

#define NEGINT(a) \
__asm("notl %0 ; notl %1 ; addl $1,%0 ; adcl $0,%1" \
	:"=r" (a->a),"=r" (a->b) \
	:"0" (a->a),"1" (a->b))

static void signify(temp_real * a)
{

    printf("DEBUG: signify() before: exponent: %04x, significand: %08x %08x\n",
           a->exponent, a->a, a->b);

	a->exponent += 2;
	__asm("shrdl $2,%1,%0 ; shrl $2,%1"
		:"=r" (a->a),"=r" (a->b)
		:"0" (a->a),"1" (a->b));

    printf("DEBUG: signify() after shift: exponent: %04x, significand: %08x %08x\n",
           a->exponent, a->a, a->b);


	if (a->exponent < 0) {
		printf("DEBUG: signify() calling NEGINT\n");
		
		NEGINT(a);

        	printf("DEBUG: signify() after NEGINT: exponent: %04x, significand: %08x %08x\n",
               		a->exponent, a->a, a->b);
	}

	a->exponent &= 0x7fff;

    printf("DEBUG: signify() final: exponent: %04x, significand: %08x %08x\n",
           a->exponent, a->a, a->b);

}

static void unsignify(temp_real * a)
{

    printf("DEBUG: unsignify() before: exponent: %04x, significand: %08x %08x\n",
           a->exponent, a->a, a->b);

	if (!(a->a || a->b)) {

		printf("DEBUG: unsignify() zero value detected, setting exponent to 0\n");

		a->exponent = 0;
		return;
	}
	a->exponent &= 0x7fff;
	if (a->b < 0) {
		printf("DEBUG: unsignify() calling NEGINT\n");
		NEGINT(a);
		a->exponent |= 0x8000;

        printf("DEBUG: unsignify() after NEGINT: exponent: %04x, significand: %08x %08x\n",
               a->exponent, a->a, a->b);

	}
	while (a->b >= 0) {

        printf("DEBUG: unsignify() before shift: exponent: %04x, significand: %08x %08x\n",
               a->exponent, a->a, a->b);


		a->exponent--;
		__asm("addl %0,%0 ; adcl %1,%1"
			:"=r" (a->a),"=r" (a->b)
			:"0" (a->a),"1" (a->b));

        printf("DEBUG: unsignify() after shift: exponent: %04x, significand: %08x %08x\n",
               a->exponent, a->a, a->b);

		if (a->b <= 0) {
			break;
		}
	}
}

void fadd(const temp_real * src1, const temp_real * src2, temp_real * result)
{
	temp_real a,b;
	int32_t x1,x2,shft;

	x1 = src1->exponent & 0x7fff;
	x2 = src2->exponent & 0x7fff;


    printf("DEBUG: fadd() source operands:\n");
    printf("src1: exponent: %04x, significand: %08x %08x\n", 
           src1->exponent, src1->a, src1->b);
    printf("src2: exponent: %04x, significand: %08x %08x\n", 
           src2->exponent, src2->a, src2->b);

    dump_fpustack();

	if (x1 > x2) {
		a = *src1;
		b = *src2;
		shft = x1-x2;
	} else {
		a = *src2;
		b = *src1;
		shft = x2-x1;
	}

	printf("DEBUG: fadd() shift amount: %d\n", shft);


	if (shft >= 64) {
		printf("DEBUG: fadd() shift too large, returning early with a\n");
		*result = a;
		return;
	}
	if (shft >= 32) {
		printf("DEBUG: fadd() shifting b by 32\n");
		b.a = b.b;
		b.b = 0;
		shft -= 32;
	}

	__asm("shrdl %4,%1,%0 ; shrl %4,%1"
		:"=r" (b.a),"=r" (b.b)
		:"0" (b.a),"1" (b.b),"c" ((char) shft));

    printf("DEBUG: fadd() after shifting b:\n");
    printf("b (shifted): exponent: %04x, significand: %08x %08x\n", 
           b.exponent, b.a, b.b);


	signify(&a);
	signify(&b);

    printf("DEBUG: fadd() before addl:\n");
    printf("a: exponent: %04x, significand: %08x %08x\n", a.exponent, a.a, a.b);
    printf("b: exponent: %04x, significand: %08x %08x\n", b.exponent, b.a, b.b);


	__asm("addl %4,%0 ; adcl %5,%1"
		:"=r" (a.a),"=r" (a.b)
		:"0" (a.a),"1" (a.b),"g" (b.a),"g" (b.b));

    printf("DEBUG: fadd() after addl:\n");
    printf("a: exponent: %04x, significand: %08x %08x\n", a.exponent, a.a, a.b);

	unsignify(&a);

    printf("DEBUG: fadd() result:\n");
    printf("result: exponent: %04x, significand: %08x %08x\n", 
           a.exponent, a.a, a.b);

    dump_fpustack();

	*result = a;
}

/*
 * linux/kernel/math/compare.c
 *
 * (C) 1991 Linus Torvalds
 */

/*
 * temporary real comparison routines
 */


#define clear_Cx() (I387.swd &= ~0x4500)

static void normalize(temp_real * a)
{
	int32_t i = a->exponent & 0x7fff;
	int32_t sign = a->exponent & 0x8000;

	if (!(a->a || a->b)) {
		a->exponent = 0;
		return;
	}
	while (i && a->b >= 0) {
		i--;
		__asm("addl %0,%0 ; adcl %1,%1"
			:"=r" (a->a),"=r" (a->b)
			:"0" (a->a),"1" (a->b));
	}
	a->exponent = i | sign;
}

void ftst(const temp_real * a)
{
	temp_real b;

	clear_Cx();
	b = *a;
	normalize(&b);
	if (b.a || b.b || b.exponent) {
		if (b.exponent < 0)
			set_C0();
	} else
		set_C3();
}

void fcom(const temp_real * src1, const temp_real * src2)
{
	temp_real a;

	a = *src1;
	a.exponent ^= 0x8000;
	fadd(&a,src2,&a);
	ftst(&a);
}

void fucom(const temp_real * src1, const temp_real * src2)
{
	fcom(src1,src2);
}

/*
 * linux/kernel/math/convert.c
 *
 * (C) 1991 Linus Torvalds
 */


/*
 * NOTE!!! There is some "non-obvious" optimisations in the temp_to_long
 * and temp_to_short conversion routines: don't touch them if you don't
 * know what's going on. They are the adding of one in the rounding: the
 * overflow bit is also used for adding one into the exponent. Thus it
 * looks like the overflow would be incorrectly handled, but due to the
 * way the IEEE numbers work, things are correct.
 *
 * There is no checking for total overflow in the conversions, though (ie
 * if the temp-real number simply won't fit in a short- or long-real.)
 */
/*
void short_to_temp(const short_real * a, temp_real * b)
{
	if (!(*a & 0x7fffffff)) {
		b->a = b->b = 0;
		if (*a)
			b->exponent = 0x8000;
		else
			b->exponent = 0;
		return;
	}
	b->exponent = ((*a>>23) & 0xff)-127+16383;
	if (*a<0)
		b->exponent |= 0x8000;
	b->b = (*a<<8) | 0x80000000;
	b->a = 0;
}
*/

void short_to_temp(const short_real * a, temp_real * b)
{

	printf("DEBUG: short_to_temp(): src: %x\n", *a);

        if (!(*a & 0x7fffffff)) {  
                b->a = b->b = 0;
                b->exponent = (*a) ? 0x8000 : 0;
                return;
        }

        b->exponent = ((*a >> 23) & 0xff) - 127 + 16383;

        if (*a < 0)  
                b->exponent |= 0x8000;

        uint64_t mantissa = ((uint64_t)(*a & 0x007FFFFF) | 0x00800000) << 40;
	printf("Debug: short_to_temp(): Mantissa (before shifting): %llx\n",
		mantissa);

        b->a = (uint32_t)(mantissa >> 32);
        b->b = (uint32_t)(mantissa);
	printf("Debug: short_to_temp(): Mantissa split. b->a: %x, b->b: %x\n", b->a, b->b);
}


void long_to_temp(const long_real * a, temp_real * b)
{
	if (!a->a && !(a->b & 0x7fffffff)) {
		b->a = b->b = 0;
		if (a->b)
			b->exponent = 0x8000;
		else
			b->exponent = 0;
		return;
	}
	b->exponent = ((a->b >> 20) & 0x7ff)-1023+16383;
	if (a->b<0)
		b->exponent |= 0x8000;
	b->b = 0x80000000 | (a->b<<11) | (((u_long)a->a)>>21);
	b->a = a->a<<11;
}

/*
void temp_to_short(const temp_real * a, short_real * b)
{

	if (!(a->exponent & 0x7fff)) {
		*b = (a->exponent)?0x80000000:0;
		return;
	}
	*b = ((((long) a->exponent)-16383+127) << 23) & 0x7f800000;
	if (a->exponent < 0)
		*b |= 0x80000000;
	*b |= (a->b >> 8) & 0x007fffff;
	switch (ROUNDING) {
		case ROUND_NEAREST:
			if ((a->b & 0xff) > 0x80)
				++*b;
			break;
		case ROUND_DOWN:
			if ((a->exponent & 0x8000) && (a->b & 0xff))
				++*b;
			break;
		case ROUND_UP:
			if (!(a->exponent & 0x8000) && (a->b & 0xff))
				++*b;
			break;
	}
}
*/

void temp_to_short(const temp_real *a, short_real *b) {
    printf("DEBUG: temp_to_short() input: exponent=0x%04X, significand=0x%08X%08X\n",
           a->exponent, a->a, a->b);

    if (!(a->exponent & 0x7FFF)) {
        *b = (a->exponent) ? 0x80000000 : 0;
        return;
    }

    int exp = ((a->exponent & 0x7FFF) - 16383 + 127);

    if (exp <= 0) {
        *b = (a->exponent & 0x8000) ? 0x80000000 : 0;
        return;
    } else if (exp >= 255) {
        *b = (a->exponent & 0x8000) ? 0xFF800000 : 0x7F800000;
        return;
    }

    uint32_t mantissa = (a->a >> 8) & 0x007FFFFF;
    uint8_t guard_bit = (a->a >> 7) & 1;
    uint8_t round_bit = (a->a >> 6) & 1;
    uint8_t sticky_bit = (a->a & 0x3F) || a->b;

    switch (ROUNDING) {
        case ROUND_NEAREST:
            if (guard_bit && (round_bit || sticky_bit))
                mantissa++;
            break;
        case ROUND_DOWN:
            if ((a->exponent & 0x8000) && (guard_bit || round_bit || sticky_bit))
                mantissa++;
            break;
        case ROUND_UP:
            if (!(a->exponent & 0x8000) && (guard_bit || round_bit || sticky_bit))
                mantissa++;
            break;
    }

    *b = (exp << 23) | mantissa;
    if (a->exponent & 0x8000)
        *b |= 0x80000000;

    printf("DEBUG: temp_to_short() result: 0x%08X\n", *b);
}


void temp_to_long(const temp_real * a, long_real * b)
{
	if (!(a->exponent & 0x7fff)) {
		b->a = 0;
		b->b = (a->exponent)?0x80000000:0;
		return;
	}
	b->b = (((0x7fff & (long) a->exponent)-16383+1023) << 20) & 0x7ff00000;
	if (a->exponent < 0)
		b->b |= 0x80000000;
	b->b |= (a->b >> 11) & 0x000fffff;
	b->a = a->b << 21;
	b->a |= (a->a >> 11) & 0x001fffff;
	switch (ROUNDING) {
		case ROUND_NEAREST:
			if ((a->a & 0x7ff) > 0x400)
				__asm("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
		case ROUND_DOWN:
			if ((a->exponent & 0x8000) && (a->b & 0xff))
				__asm("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
		case ROUND_UP:
			if (!(a->exponent & 0x8000) && (a->b & 0xff))
				__asm("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
	}
}

void frndint(const temp_real * a, temp_real * b)
{
	int32_t shft =  16383 + 63 - (a->exponent & 0x7fff);
	uint32_t underflow;

	if ((shft < 0) || (shft == 16383+63)) {
		*b = *a;
		return;
	}
	b->a = b->b = underflow = 0;
	b->exponent = a->exponent;
	if (shft < 32) {
		b->b = a->b; b->a = a->a;
	} else if (shft < 64) {
		b->a = a->b; underflow = a->a;
		shft -= 32;
		b->exponent += 32;
	} else if (shft < 96) {
		underflow = a->b;
		shft -= 64;
		b->exponent += 64;
	} else {
		underflow = 1;
		shft = 0;
	}
	b->exponent += shft;
	__asm("shrdl %2,%1,%0"
		:"=r" (underflow),"=r" (b->a)
		:"c" ((char) shft),"0" (underflow),"1" (b->a));
	__asm("shrdl %2,%1,%0"
		:"=r" (b->a),"=r" (b->b)
		:"c" ((char) shft),"0" (b->a),"1" (b->b));
	__asm("shrl %1,%0"
		:"=r" (b->b)
		:"c" ((char) shft),"0" (b->b));
	switch (ROUNDING) {
		case ROUND_NEAREST:
			__asm("addl %4,%5 ; adcl $0,%0 ; adcl $0,%1"
				:"=r" (b->a),"=r" (b->b)
				:"0" (b->a),"1" (b->b)
				,"r" (0x7fffffff + (b->a & 1))
				,"m" (*&underflow));
			break;
		case ROUND_UP:
			if ((b->exponent >= 0) && underflow)
				__asm("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
		case ROUND_DOWN:
			if ((b->exponent < 0) && underflow)
				__asm("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
	}
	if (b->a || b->b)
		while (b->b >= 0) {
			b->exponent--;
			__asm("addl %0,%0 ; adcl %1,%1"
				:"=r" (b->a),"=r" (b->b)
				:"0" (b->a),"1" (b->b));
		}
	else
		b->exponent = 0;
}

void Fscale(const temp_real *a, const temp_real *b, temp_real *c) 
{
	temp_int ti;

	*c = *a;
	if(!c->a && !c->b) {				/* 19 Sep 92*/
		c->exponent = 0;
		return;
	}
	real_to_int(b, &ti);
	if(ti.sign)
		c->exponent -= ti.a;
	else
		c->exponent += ti.a;
}

void real_to_int(const temp_real * a, temp_int * b)
{
	int32_t shft =  16383 + 63 - (a->exponent & 0x7fff);
	uint32_t underflow;

	b->a = b->b = underflow = 0;
	b->sign = (a->exponent < 0);
	if (shft < 0) {
		set_OE();
		return;
	}
	if (shft < 32) {
		b->b = a->b; b->a = a->a;
	} else if (shft < 64) {
		b->a = a->b; underflow = a->a;
		shft -= 32;
	} else if (shft < 96) {
		underflow = a->b;
		shft -= 64;
	} else {
		underflow = 1;
		shft = 0;
	}
	__asm("shrdl %2,%1,%0"
		:"=r" (underflow),"=r" (b->a)
		:"c" ((char) shft),"0" (underflow),"1" (b->a));
	__asm("shrdl %2,%1,%0"
		:"=r" (b->a),"=r" (b->b)
		:"c" ((char) shft),"0" (b->a),"1" (b->b));
	__asm("shrl %1,%0"
		:"=r" (b->b)
		:"c" ((char) shft),"0" (b->b));
	switch (ROUNDING) {
		case ROUND_NEAREST:
			__asm("addl %4,%5 ; adcl $0,%0 ; adcl $0,%1"
				:"=r" (b->a),"=r" (b->b)
				:"0" (b->a),"1" (b->b)
				,"r" (0x7fffffff + (b->a & 1))
				,"m" (*&underflow));
			break;
		case ROUND_UP:
			if (!b->sign && underflow)
				__asm("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
		case ROUND_DOWN:
			if (b->sign && underflow)
				__asm("addl $1,%0 ; adcl $0,%1"
					:"=r" (b->a),"=r" (b->b)
					:"0" (b->a),"1" (b->b));
			break;
	}
}

void int_to_real(const temp_int * a, temp_real * b)
{
	b->a = a->a;
	b->b = a->b;
	if (b->a || b->b)
		b->exponent = 16383 + 63 + (a->sign? 0x8000:0);
	else {
		b->exponent = 0;
		return;
	}
	while (b->b >= 0) {
		b->exponent--;
		__asm("addl %0,%0 ; adcl %1,%1"
			:"=r" (b->a),"=r" (b->b)
			:"0" (b->a),"1" (b->b));
	}
}

void dump_fpustack(void) {
    printf("DEBUG: FPU Stack Dump:\n");
    for (int i = 0; i < 8; i++) {
        printf("ST(%d): exponent: %04x, significand: %04x %04x %04x %04x\n",
               i, ST(i).exponent, ST(i).m3, ST(i).m2, ST(i).m1, ST(i).m0);
    }

    printf("DEBUG: Raw FPU Stack Memory Dump:\n");

    uint8_t *raw_data = (uint8_t *)&I387;
    size_t raw_size = sizeof(I387);

    for (size_t i = 0; i < raw_size; i += 16) {
        printf("%04zx: ", i);
        for (size_t j = 0; j < 16 && (i + j) < raw_size; j++) {
            printf("%02x ", raw_data[i + j]);
        }
        printf("\n");
    }
}


#ifdef ORIGINAL_USERMODE
#undef USERMODE
#define USERMODE ORIGINAL_USERMODE
#undef ORIGINAL_USERMODE
#endif
