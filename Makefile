CC = gcc
LIBS = -L/usr/lib64 -L/lib64 -lfuse -lz
INCLUDE = -I/usr/include -I/usr/include/fuse
CFLAGS = -Wall -pipe -O0 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -g -std=gnu99
GAMEMOD = mm_vid.o lfosh_lib.o arc_dat.o fall2_dat.o fragall__.o mm_lod.o sc2000_dat.o jagg_dat.o \
          gob_stk.o dune2_pak.o as688_mlb.o cf_dat.o bs_rarc.o tlj_xarc.o toee_dat.o g17_dat.o \
          comm_dir.o ufoamh_vfs.o ja2_slf.o dk_dat.o fall_dat.o gta3_img.o ult7_dat.o aod_dat.o \
          sforce_pak.o nfs4_viv.o ftl_dat.o ta_hapi.o artifex_cub.o fez_pak.o mm_snd.o canon_fw.o \
          risen_pak.o
BASEMOD = gamefs.o generic.o

all: gamefs

gamefs.o: modules.h

$(BASEMOD): %.o: %.c  generic.h gamefs.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(GAMEMOD): %.o: %.c  generic.h gamefs.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

gamefs: $(BASEMOD) $(GAMEMOD)
	$(CC) $(CFLAGS) $(LIBS) $(BASEMOD) $(GAMEMOD) -o gamefs

clean:
	rm -f *.o gamefs

rebuild: clean all

.PHONY: all clean rebuild