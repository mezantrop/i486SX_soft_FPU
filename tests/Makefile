AS = as
CC = gcc
CFLAGS = -m32
ASFLAGS =


TESTS_TFLOAT = fadd_tfloat_pos fadd_tfloat_neg fadd_tfloat_zero \
	fadd_tfloat_denorm fadd_tfloat_overflow fadd_tfloat_nan \
	fadd_tfloat_inf

TESTS = fadd_float $(TESTS_TFLOAT)

.PHONY: all clean

all: $(TESTS)

.for tst in $(TESTS)
$(tst): $(tst).o
	$(CC) $(CFLAGS) -o $@ ${.ALLSRC}

$(tst).o: $(tst).s
	$(AS) $(ASFLAGS) -o $@ ${.IMPSRC}
.endfor

clean:
	rm -rf $(TESTS) *.o
