/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_OBJECTID_H_
#define SOURCE_INCLUDE_OBJECTID_H_

#include <FdsCrypto.h>
#include <fds_types.h>

namespace fds {

/**
 * Wrapper class to generate object IDs.
 * This is a stateless class and only wraps
 * up related definitions and static functions
 * together.
 */
class ObjIdGen {
  public:
    /**
     * Defines the hash to use
     */
    typedef fds::hash::Sha1 GeneratorHash;
    /**
     * Computes the object ID for a buffer and length.
     * The object id should be preallocated to the
     * correct size and only set.
     */
    static void genObjectId(const fds_byte_t *input,
                            size_t length,
                            ObjectID *objId) {
        GeneratorHash::calcDigestStatic(objId->digest,
                                        input,
                                        length);
    }
    /**
     * Computes the object ID for a buffer and length.
     * A copy of the object id is returned.
     */
    static ObjectID genObjectId(const fds_byte_t *input, size_t length) {
        ObjectID objId;
        genObjectId(input, length, &objId);
        return objId;
    }

    /**
     * Computes the object ID for a buffer and length.
     * A copy of the object id is returned.
     */
    static ObjectID genObjectId(const char *input, size_t length) {
        ObjectID objId;
        genObjectId((const fds_byte_t *)input, length, &objId);
        return objId;
    }
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_OBJECTID_H_
