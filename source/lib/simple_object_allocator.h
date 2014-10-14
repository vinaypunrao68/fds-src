#ifndef __SIMPLE_OBJ_ALLOC_H_

#define __SIMPLE_OBJ_ALLOC_H_

#include <fds_obj_cache.h>

namespace fds {

  class simple_object_allocator: public slab_object_allocator_base {

  public:
    simple_object_allocator() {

    }
    ~simple_object_allocator() {

    }

    ObjCacheBufPtrType allocate_object_buf(fds_uint32_t obj_size) {

      ObjCacheBufPtrType newObjBufPtr(new ObjectCacheBuf());
      newObjBufPtr->resize(obj_size);
      return newObjBufPtr;

    }

    void return_object_buf_to_pool(ObjCacheBufPtrType obj_buf) {
      // don't do anything. Let the buf ptr fall out of scope and
      // destructor be called so the memory will be freed.
    }

  };

}

#endif
