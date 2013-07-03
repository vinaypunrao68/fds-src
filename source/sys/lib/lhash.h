typedef void * _LHASH;

typedef unsigned long (*hash_fn_t)(const void *entry);
typedef int (*cmp_fn_t)(const void *entry1, const void *entry2);

_LHASH *lh_new(hash_fn_t hfn, cmp_fn_t cfn) {
  return (NULL);
}

void *lh_insert(_LHASH htable, void *entry) {
  return (NULL);
}

void *lh_retrieve(_LHASH htable, void *entry) {
  return (NULL);
}

void *lh_delete(_LHASH htable, void *entry) {
  return (NULL);
}
