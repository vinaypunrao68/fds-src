#include "vvc_private.h"

vvce_t *vvce_create(const char *blk_name, const int num_segments, const doid_t **doid_list) {

  int i;

  vvce_t *vvce = (vvce_t *)vvc_malloc(sizeof(vvce_t));
  
  if (vvce == NULL) {
    vvc_print("Mem allocation failed for vvce\n");
    return (NULL);
  }
  memset(vvce, 0, sizeof(vvce_t));

  vvce->blk_name = (char *)vvc_malloc(strlen(blk_name) + 1);
  if (vvce->blk_name == NULL) {
    vvc_print("Mem allocation failed for blk_name\n");
    return (NULL);
  }

  strcpy(vvce->blk_name, blk_name);
  vvce->num_segments = num_segments;
  vvce->doid_list_size = 2*num_segments;
  vvce->doid_list =  (doid_t *)vvc_malloc(vvce->doid_list_size * sizeof(doid_t));
  if (vvce->doid_list == NULL) {
    vvc_print("Mem allocation failed for doid_list\n");
    return (NULL);
  }

  memset(vvce->doid_list, 0, vvce->doid_list_size * sizeof(doid_t));
  for (i = 0; i < num_segments; i++) {
    memcpy(&vvce->doid_list[i], doid_list[i], sizeof(doid_t));
  }

  return(vvce);

}

void vvce_destroy(vvce_t *vvce) {

  if (vvce) {
    if (vvce->blk_name) {
      vvc_mfree(vvce->blk_name);
    }
    if (vvce->doid_list) {
      vvc_mfree(vvce->doid_list);
    }
    vvc_mfree(vvce);
  }
  return;
}
