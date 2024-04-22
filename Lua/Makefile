CC=emcc
AR=emar
ARFLAGS=rcs
RANLIB=echo
CFLAGS=-O2 -Wall --std=c99
LDFLAGS=

MYCFLAGS=
MYLDFLAGS=
MYLIBS=

OBJ_DIR=.

CORE_O= lapi.o lcode.o lctype.o ldebug.o \
        ldo.o ldump.o lfunc.o lgc.o \
        llex.o lmem.o lobject.o lopcodes.o \
        lparser.o lstate.o lstring.o ltable.o \
        ltm.o lundump.o lvm.o lzio.o

LIB_O= lauxlib.o lbaselib.o lcorolib.o ldblib.o \
       liolib.o lmathlib.o loadlib.o loslib.o \
       lstrlib.o ltablib.o lutf8lib.o linit.o

ALL_O= $(addprefix $(OBJ_DIR)/,$(CORE_O) $(LIB_O))
ALL_A= liblua.a

all: $(ALL_A)

$(ALL_A): $(ALL_O)
	$(AR) $(ARFLAGS) $@ $(ALL_O)


clean:
	rm -f $(ALL_O) $(ALL_A)

%.o: %.c
	$(CC) $(CFLAGS) $(MYCFLAGS) -c $< -o $@
