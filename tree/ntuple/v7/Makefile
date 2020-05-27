CXXFLAGS_CUSTOM = -DROOT_RNTUPLE_MINI -std=c++14 -Wall -pthread -g -O2 -m64 \
  -Iinc -I../../../../install-git/include -I../../../builtins/xxhash
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = -lrt -llz4 -lz -llzma

NTPL_SOURCES = $(wildcard src/*.cxx) \
  ../../../core/base/src/TError.cxx \
	../../../core/base/v7/src/RLogger.cxx \
	../../../core/zip/src/RZip.cxx \
	../../../core/lzma/src/ZipLZMA.c \
	../../../core/lz4/src/ZipLZ4.cxx \
	../../../core/zstd/src/ZipZSTD.cxx \
	$(wildcard ../../../builtins/zstd/*.c) \
	../../../core/zip/src/ZInflate.c \
	../../../core/zip/src/ZDeflate.c \
	../../../core/zip/src/Bits.c \
	../../../io/io/src/RRawFile.cxx \
	../../../io/io/src/RRawFileUnix.cxx

all: main

main: main.cc
	g++ $(CXXFLAGS) -o $@ $^ $(NTPL_SOURCES) $(LDFLAGS)
