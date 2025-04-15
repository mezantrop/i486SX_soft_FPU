CC = gcc
CFLAGS = -Wall -O2
TARGETS = double2temp temp2real real2double

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o
