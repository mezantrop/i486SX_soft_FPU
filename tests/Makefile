AS = as
CC = gcc
CFLAGS =
ASFLAGS =
LDFLAGS = -lm

TESTS_FLOAT = fadd_float_float fdiv_float_float fmul_float_float fild_fstpt


TESTS_TFLOAT = fadd_tfloat_quad_pos fadd_tfloat_quad_pos_pi \
	fadd_tfloat_quad_pos_1	fadd_tfloat_quad_neg fadd_tfloat_quad_zero \
	fadd_tfloat_denorm fadd_tfloat_overflow fadd_tfloat_quad_nan \
	fadd_tfloat_inf fmul_tfloat_tfloat fdiv_tfloat_tfloat \
	fsub_tfloat_quad fcom_tfloat_quad_neg fcom_tfloat_quad_pos \
	frndint_tfloat_neg fscale_tfloat_tfloat fsqrt_tfloat \
	fmul_bytes fmul_tfloat_tfloat_neg fyl2x_tfloat_tfloat

TEST_S = fildl_fstpt get_long_real $(TESTS_FLOAT) $(TESTS_TFLOAT)

TEST_C = fadd_print fcalc fvars short_div_float double_minus_float \
	float_plus_ld ld_mul_int ld_plus_neg_float neg_double_mul_long \
	neg_ld_div_char try_sub neg_ld_mul_ld pi ld_log2l_ld ld_log2l \
	ld_pow_ld

.PHONY: all clean

all: $(TEST_S) $(TEST_C)

.for tst_s in $(TEST_S)
$(tst_s): $(tst_s).o
	$(CC) $(CFLAGS) -o $@ ${.ALLSRC}

$(tst_s).o: $(tst_s).s
	$(AS) $(ASFLAGS) -o $@ ${.IMPSRC}
.endfor

.for tst_c in $(TEST_C)
$(tst_c): $(tst_c).o
	$(CC) $(CFLAGS) -o $@ ${.ALLSRC} ${LDFLAGS}

$(tst_c).o: $(tst_c).c
	$(CC) $(CFLAGS) -c -o $@ ${.IMPSRC}
.endfor

clean:
	rm -rf $(TEST_S) $(TEST_C) *.o
