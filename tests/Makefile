AS = as
CC = gcc
CFLAGS =
ASFLAGS =

TESTS_TFLOAT = fadd_tfloat_quad_pos fadd_tfloat_quad_neg \
	fadd_tfloat_quad_zero fadd_tfloat_denorm fadd_tfloat_overflow \
	fadd_tfloat_quad_nan fadd_tfloat_inf

TEST_S = fadd_float fildl_fstpt $(TESTS_TFLOAT)
TEST_C = fadd_print

.PHONY: all clean

all: $(TEST_S) $(TEST_C)

.for tst in $(TEST_S)
$(tst): $(tst).o
	$(CC) $(CFLAGS) -o $@ ${.ALLSRC}

$(tst).o: $(tst).s
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
