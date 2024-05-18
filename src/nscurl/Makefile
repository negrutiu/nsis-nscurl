
# -------------------------------
# Mandatory definitions
# -------------------------------
#   ARCH=X86|x64
#   CHAR=ANSI|Unicode
#   OUTDIR=<output_directory>
# -------------------------------
# Optional definitions
# -------------------------------
#   CUSTOM_CFLAGS
#   CUSTOM_LDFLAGS
#   CUSTOM_RCFLAGS


PROJECT  = NScurl
BIN      = $(PROJECT).dll
OBJ      = pluginapi.o main.o utils.o crypto.o curl.o queue.o gui.o resource.res
INC      =
LIB      = -lcurl -lssl -lcrypto -lbrotlidec -lbrotlicommon -lnghttp2 -lzlib -lzstd -ladvapi32 -lkernel32 -luser32 -lversion -lws2_32 -lcrypt32 -lmsvcrt -lgcc

_OBJ     = $(patsubst %,$(OUTDIR)/%,$(OBJ))
_BIN     = $(patsubst %,$(OUTDIR)/%,$(BIN))

DEF      = $(OUTDIR)/lib$(PROJECT).def
STATIC   = $(OUTDIR)/lib$(PROJECT).a

#
# https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
# http://linux.die.net/man/1/gcc
# http://linux.die.net/man/1/ld
#

# vcpkg
ifeq ($(ARCH), X64)
    VCPKG_TRIPLET = x64-mingw-static
else
    VCPKG_TRIPLET = x86-mingw-static
endif

VCPKG_INSTALLED = ../../vcpkg/$(VCPKG_TRIPLET)/installed/$(VCPKG_TRIPLET)

INC += -I$(VCPKG_INSTALLED)/include -I$(VCPKG_INSTALLED)/include/openssl
LIB += -L$(VCPKG_INSTALLED)/lib

# ARCH
ifeq ($(ARCH), X64)
	CFLAGS  += -Wall -DNDEBUG
	LDFLAGS += -Wl,-e'DllMain' -Wl,--high-entropy-va
	RCFLAGS += -F pe-x86-64
else
	CFLAGS  += -Wall -DNDEBUG -march=pentium2
	LDFLAGS += -Wl,-e'_DllMain'
	RCFLAGS += -F pe-i386
endif

# CHAR
ifeq ($(CHAR), ANSI)
	CFLAGS  += -DMBCS -D_MBCS
	LDFLAGS +=
else
	CFLAGS  += -municode -DUNICODE -D_UNICODE
	LDFLAGS +=
endif

# https://sourceforge.net/p/mingw-w64/wiki2/gnu%20printf/
CFLAGS += -D__USE_MINGW_ANSI_STDIO=0

CFLAGS += \
	-mdll \
	-s \
	-O3 \
	-fPIE \
	-ffunction-sections \
	-fdata-sections \
	-fno-unwind-tables \
	-fno-asynchronous-unwind-tables \
    -Wno-unused-function \
	$(INC) \
	$(CUSTOM_CFLAGS)

LDFLAGS += \
	$(CFLAGS) \
	-static \
	-Wl,--gc-sections \
	-Wl,--no-seh \
	-Wl,--nxcompat \
	-Wl,--dynamicbase \
	-Wl,--enable-auto-image-base \
	-Wl,--enable-stdcall-fixup \
	-Wl,--output-def,$(DEF) \
	-Wl,--out-implib,$(STATIC) \
	$(LIB) \
	$(CUSTOM_LDFLAGS)

RCFLAGS += \
	$(CUSTOM_RCFLAGS)


.PHONY: clean all-before all all-after

clean:
	@echo.
	if exist $(OUTDIR) rd /S /Q $(OUTDIR)

all: all-before $(_BIN) all-after

all-before:
	if not exist $(OUTDIR) mkdir $(OUTDIR)

# Link
$(_BIN): $(_OBJ)
	@echo.
	gcc $(_OBJ) -o $(_BIN) $(LDFLAGS)

# Compile .c
$(OUTDIR)/%.o: %.c
	gcc $(CFLAGS) -o $@ -c $<

$(OUTDIR)/%.o: nsis/pluginapi.c
	gcc $(CFLAGS) -o $@ -c $<

# Compile .rc
$(OUTDIR)/%.res: %.rc
	windres -o $@ -i $< -O coff --input-format=rc $(RCFLAGS)
