AS = as
CC = gcc
CFLAGS = -m32
ASFLAGS =

TESTS = fadd_float fadd_tfloat

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
