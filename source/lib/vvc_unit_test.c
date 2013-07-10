#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fds_commons.h"
#include "vvclib.h"

vvc_vhdl_t vhdl = 0;

char cmd_wd[128];

void process_crtvol_cmd(char *cmd_line);
void process_loadvol_cmd(char *cmd_line);
void process_add_cmd(char *cmd_line);
void process_update_cmd(char *cmd_line);
void process_delete_cmd(char *cmd_line);
void process_show_cmd(char *cmd_line);

int main(int argc, char *argv) {

  char *line_ptr = 0;
  size_t n_bytes;

  while(1) {
    printf(">");
    getline(&line_ptr, &n_bytes, stdin);
    sscanf(line_ptr, "%s", cmd_wd);
    if (strcmp(cmd_wd, "quit") == 0) {
      break;
    } else if (strcmp(cmd_wd, "crtvol") == 0) {
      process_crtvol_cmd(line_ptr);
    } else if (strcmp(cmd_wd, "loadvol") == 0) {
      process_loadvol_cmd(line_ptr);
    } else if (strcmp(cmd_wd, "add") == 0) {
      process_add_cmd(line_ptr);
    } else if (strcmp(cmd_wd, "update") == 0) {
      process_update_cmd(line_ptr);
    } else if (strcmp(cmd_wd, "delete") == 0) {
      process_delete_cmd(line_ptr);
    } else if (strcmp(cmd_wd, "show") == 0) {
      process_show_cmd(line_ptr);
    } else {
      printf("Unknown command %s. Please try again\n", cmd_wd);
    }
  }

  if (line_ptr) {
    free(line_ptr);
  }

}

char blk_name[255];

void process_crtvol_cmd(char *line_ptr) {

  volid_t vol_id;

  sscanf(line_ptr, "crtvol %d", &vol_id);
  vhdl = vvc_vol_create(vol_id, "FDS_SP_DB", 1024);
  return;
}

void process_loadvol_cmd(char *line_ptr) {

  volid_t vol_id;

  sscanf(line_ptr, "loadvol %d", &vol_id);
  vhdl = vvc_vol_load(vol_id, "FDS_SP_DB");
  return;
}

void process_add_cmd(char *line_ptr) {

  int bytes_read = 0;
  int n_segments = 0;
  doid_t **doid_list = 0;
  int i,rc;
  char *next_pos = line_ptr;

  sscanf(next_pos, "add %s %d%n", blk_name, &n_segments, &bytes_read);
  next_pos += bytes_read;
  
  doid_list = (doid_t **)malloc(n_segments * sizeof(doid_t *) + n_segments * sizeof (doid_t));

  for (i = 0; i < n_segments; i++) {
    fds_object_id_t *p_obj_id;

    doid_list[i] = (doid_t *) ((char *)doid_list + n_segments * sizeof(doid_t *) + i * sizeof(doid_t));
    memset(&doid_list[i][0], 0, sizeof(doid_t));
    p_obj_id  = (fds_object_id_t *)&(doid_list[i][0]);
    sscanf(next_pos, "%llx-%llx%n", &(p_obj_id->hash_high), &(p_obj_id->hash_low), &bytes_read);
    next_pos += bytes_read;
  }

  rc = vvc_entry_create(vhdl, blk_name, n_segments, (const doid_t **)doid_list);

  if (rc) {
    printf("Error on creating vvc entry. Error code : %d\n", rc);
  }

  free(doid_list);

}

void process_update_cmd(char *line_ptr) {

  int bytes_read = 0;
  int n_segments = 0;
  doid_t **doid_list = 0;
  int i, rc;
  char *next_pos = line_ptr;

  sscanf(next_pos, "update %s %d%n", blk_name, &n_segments, &bytes_read);
  next_pos += bytes_read;

  doid_list = (doid_t **)malloc(n_segments * sizeof(doid_t *) + n_segments * sizeof (doid_t));

  for (i = 0; i < n_segments; i++) {
    fds_object_id_t *p_obj_id;

    doid_list[i] = (doid_t *) ((char *)doid_list + n_segments * sizeof(doid_t *) + i * sizeof(doid_t));
    memset(&doid_list[i][0], 0, sizeof(doid_t));
    p_obj_id  = (fds_object_id_t *)&(doid_list[i][0]);
    sscanf(next_pos, "%llx-%llx%n", &p_obj_id->hash_high, &p_obj_id->hash_low, &bytes_read);
    next_pos += bytes_read;
  }

  rc = vvc_entry_update(vhdl, blk_name, n_segments, (const doid_t **)doid_list);

  if (rc) {
    printf("Error on updating vvc entry. Error code : %d\n", rc);
  }

  free(doid_list);

}

void process_delete_cmd(char *line_ptr) {

  int rc;

  sscanf(line_ptr, "delete %s", blk_name);
  
  rc = vvc_entry_delete(vhdl, blk_name);

  if (rc) {
    printf("Error on deleting vvc entry. Error code : %d\n", rc);
  }

}

void process_show_cmd(char *line_ptr) {

  int n_segments = 0;
  doid_t *doid_list = 0;
  int i, rc;

  sscanf(line_ptr, "show %s", blk_name);
  
  rc = vvc_entry_get(vhdl, blk_name, &n_segments, &doid_list);

  if (rc) {
    printf("Error on retrieving vvc entry. Error code : %d\n", rc);
  } else {
    printf("%s %d", blk_name, n_segments);
    for (i = 0; i < n_segments; i++) {
      fds_object_id_t *p_obj_id;
      p_obj_id = (fds_object_id_t *)&doid_list[i][0];
      printf(" %llx-%llx", p_obj_id->hash_high, p_obj_id->hash_low);
    }
    printf("\n");
  }
  if (doid_list) {
    free(doid_list);
  }

}




