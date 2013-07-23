#define _LHASH _FDS_HASH
#define FDSH_MAX_BUCKETS 8192
#define lh_new(hfn, cfn) fdsh_new(FDSH_MAX_BUCKETS, hfn, cfn)
#define lh_insert fdsh_insert
#define lh_retrieve fdsh_retrieve
#define lh_delete fdsh_delete
#define lh_free fdsh_free

typedef void *_FDS_HASH;

typedef unsigned long (*hash_fn_t)(const void *entry);
typedef int (*cmp_fn_t)(const void *entry1, const void *entry2);

typedef struct __fds_ht_elem {
  void *entry;
  struct __fds_ht_elem *next;
} fdsh_elem;

typedef struct __fds_ht_bucket {
  fdsh_elem *head;
} fdsh_bucket;
 
typedef struct __fds_ht_table {
  unsigned int n_bkts;
  hash_fn_t hfn;
  cmp_fn_t cfn;
  fdsh_bucket *h_bkts; // array
} fdsh_table;


static _FDS_HASH *fdsh_new(unsigned int max_bkts, hash_fn_t hfn, cmp_fn_t cfn) {

  fdsh_table *tbl = (fdsh_table *)kmalloc(sizeof(fdsh_table), GFP_KERNEL);
  if (tbl == NULL) {
    printk("Unable to allocate memory for hash table structure\n");
    return ((_FDS_HASH)0);
  }

  tbl->n_bkts = max_bkts;
  tbl->hfn = hfn;
  tbl->cfn = cfn;
  tbl->h_bkts = (fdsh_bucket *)vmalloc(sizeof(fdsh_bucket) * max_bkts);
  if (tbl->h_bkts == NULL) {
    printk("Unable to allocate memory for hash table buckets - requested (%x) bytes\n", (sizeof (fdsh_bucket) * max_bkts));
    return ((_FDS_HASH)0);
  }
  memset(tbl->h_bkts, 0, sizeof(fdsh_bucket) * max_bkts);
  return ((_FDS_HASH)tbl);
}

// If an elem already exists with the same key, replace the existing elem with the new one
// and return the old entry
static void *fdsh_insert(_FDS_HASH htable, void *entry) {

  unsigned int bkt;
  fdsh_elem *next, *new_elem;
  fdsh_table *tbl = (fdsh_table *)htable;
  void *r_entry = NULL;

  bkt = tbl->hfn(entry) % tbl->n_bkts;
  next = tbl->h_bkts[bkt].head;
  while (next != NULL) {
    if (tbl->cfn(next->entry, entry) == 0) {
      r_entry = next->entry;
      next->entry = entry;
      return (r_entry);
    }
    next = next->next;
  }
  new_elem = (fdsh_elem *)kmalloc(sizeof(fdsh_elem), GFP_KERNEL);
  new_elem->entry = entry;
  new_elem->next = tbl->h_bkts[bkt].head;
  tbl->h_bkts[bkt].head = new_elem;

  return (NULL);
}

static void *fdsh_retrieve(_LHASH htable, void *entry) {

  unsigned int bkt;
  fdsh_elem *next;
  fdsh_table *tbl = (fdsh_table *)htable;
  void *r_entry = NULL;

  bkt = tbl->hfn(entry) % tbl->n_bkts;
  next = tbl->h_bkts[bkt].head;
  while (next != NULL) {
    if (tbl->cfn(next->entry, entry) == 0) {
      r_entry = next->entry;
      return (r_entry);
    }
    next = next->next;
  }
  return (NULL);
}

static void *fdsh_delete(_LHASH htable, void *entry) {

  unsigned int bkt;
  fdsh_elem *next, **p_next;
  fdsh_table *tbl = (fdsh_table *)htable;
  void *r_entry = NULL;

  bkt = tbl->hfn(entry) % tbl->n_bkts;
  next = tbl->h_bkts[bkt].head;
  p_next = &(tbl->h_bkts[bkt].head);
  while (next != NULL) {
    if (tbl->cfn(next->entry, entry) == 0) {
      *p_next = next->next;
      r_entry = next->entry;
      kfree(next);
      return (r_entry);
    }
    next = next->next;
  }

  return (NULL);
}

static void *fdsh_free(_LHASH htable) {
  /*
   * Do nothing for now because this in kernel
   * implementation will be depracated when the
   * hypervisor code moves to user space.
   */
  return NULL;
}
