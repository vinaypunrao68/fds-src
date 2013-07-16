#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fds_commons.h"
#include "tvclib.h"

doid_t test_doid[5];
tvce_t tvc_entries[16];


int main (int argc, char *argv[]) {
  
  tvc_vhdl_t vhdl;
  int i;
  int n_entries = 16;
  int eol;

  if (argc < 3) {
    printf("Usage: %s vol_id, log_file_name load/crt\n", argv[0]);
    exit(0);
  }

  for (i = 0; i < 5; i++) {
    memset(&test_doid[i][0], 'D'+i, sizeof(doid_t));
    test_doid[i][15] = 0;
  }

  if (strcmp(argv[3], "load") == 0) {
    vhdl = tvc_vol_load(atoi(argv[1]), "FDS_SP_DB", argv[2]);
  } else {
    unsigned int ref_hint;

    vhdl = tvc_vol_create(atoi(argv[1]), "FDS_SP_DB", argv[2], 256);
    tvc_entry_append(vhdl, 11, 0x4488, "aabbcc.dd",0, test_doid[0], &ref_hint);
    tvc_entry_append(vhdl, 12, 0x200004488, "bbccdd.ee",0, test_doid[1], &ref_hint);
    tvc_entry_append(vhdl, 13, 0x200004489, "ccddee.ff",0, test_doid[2], &ref_hint);
    tvc_entry_append(vhdl, 14, 0x200005289, "ddeeff.gg",1, test_doid[3], &ref_hint);
    tvc_entry_append(vhdl, 15, 0x400004890, "eeffgg.hh",2, test_doid[4], &ref_hint);

  }

  // exit(0);

  tvc_entries_get(vhdl, 0x200004489, &n_entries, tvc_entries, &eol); 
  
  printf("Wrote 5 entries. Reading back last 3 entries: \n"); 
  for (i = 0; i < n_entries; i++) {
    tvce_t *tvce = &tvc_entries[i]; 
    printf("Entry %d: time - 0x%llx, blk_name - %s, seg_id - %d, doid - %s\n",
	   i, tvce->timestamp, tvce->blk_name, tvce->segment_id, tvce->doid);
  }

}
