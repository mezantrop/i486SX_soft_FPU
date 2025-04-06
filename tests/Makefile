AS = as
CC = gcc
CFLAGS =
ASFLAGS =

TESTS_TFLOAT = fadd_tfloat_quad_pos fadd_tfloat_quad_neg \
	fadd_tfloat_quad_zero fadd_tfloat_denorm fadd_tfloat_overflow \
	fadd_tfloat_quad_nan fadd_tfloat_inf

TEST_S = fadd_float fildl_fstpt $(TESTS_TFLOAT)
TEST_C = fadd_print fcalc fvars short_div_float double_minus_float \
	float_plus_ld ld_mul_int ld_plus_neg_float neg_double_mul_long \
	neg_ld_div_char

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
	$(CC) $(CFLAGS) -o $@ ${.ALLSRC}

$(tst_c).o: $(tst_c).c
	$(CC) $(CFLAGS) -c -o $@ ${.IMPSRC}
.endfor

clean:
	rm -rf $(TEST_S) $(TEST_C) *.o
