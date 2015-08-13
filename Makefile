MPS_VER=1.114.0
MPS_KIT=mps-kit-$(MPS_VER)
MPS_URL=http://www.ravenbrook.com/project/mps/release/$(MPS_VER)/$(MPS_KIT).tar.gz

CFLAGS+=-g -O2 -std=c11
CFLAGS+=-I$(MPS_KIT)/code
CFLAGS+=-pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual \
        -Wstrict-prototypes -Wmissing-prototypes

LDFLAGS+=-lpthread

all: main

.PHONY: clean

$(MPS_KIT):
	curl "$(MPS_URL)" -o $(MPS_KIT).tar.gz
	tar xmf $(MPS_KIT).tar.gz

mps.o: $(MPS_KIT)
	$(CC) $(CFLAGS) -DCONFIG_VAR_COOL -c "$(MPS_KIT)/code/mps.c"

main: mps.o

clean:
	rm -rf main mps.o
	rm -r $(MPS_KIT).tar.gz
