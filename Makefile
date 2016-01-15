MPS_VER=1.114.0
MPS_KIT=mps-kit-$(MPS_VER)
MPS_URL=http://www.ravenbrook.com/project/mps/release/$(MPS_VER)/$(MPS_KIT).tar.gz

SANFLAGS=-fsanitize=undefined -fsanitize=address

CFLAGS+=-g -O2 -std=c11
CFLAGS+=-I$(MPS_KIT)/code
CFLAGS+=-pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual \
        -Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=$(SANFLAGS)

LDFLAGS+=-lpthread
LDFLAGS+=$(SANFLAGS)

OBJS=  main.o
OBJS+= loader.o
OBJS+= stack.o
OBJS+= mps.o

all: main

.PHONY: clean

$(MPS_KIT):
	curl "$(MPS_URL)" -o $(MPS_KIT).tar.gz
	tar xmf $(MPS_KIT).tar.gz

main.o: $(MPS_KIT)

mps.o: $(MPS_KIT)
	$(CC) $(CFLAGS) -DCONFIG_VAR_COOL -c "$(MPS_KIT)/code/mps.c"

main: $(OBJS)

clean:
	rm -rf main $(OBJS)
	rm -f $(MPS_KIT).tar.gz
