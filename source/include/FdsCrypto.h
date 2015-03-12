/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDSCRYPTO_H_
#define SOURCE_INCLUDE_FDSCRYPTO_H_

extern "C" {
#include <openssl/sha.h>
}

#include <string>


#include "fds_types.h"

namespace fds {

/**
 * Abstract base class that defines the interface to
 * fds hashing functions
 */
class FdsHashFunc {
  public:
    virtual ~FdsHashFunc() {}

    /**
     * Updates a running digest with additional input.
     * The digest and state is maintained by the derived
     * class instance.
     */
    virtual void update(const uint8_t *input, size_t length) = 0;
    /**
     * Returns the current digest state maintained by the
     * derived class and then resets digest to initial state.
     */
    virtual void final(uint8_t *digest) = 0;
    /**
     * Restarts the digest state maintained by the derived class
     * to its initial state. This clear any previous state produced
     * with prior update() calls.
     */
    virtual void restart() = 0;
};

namespace hash {

class Sha1 : FdsHashFunc {
    SHA_CTX myHash;
  public:
    Sha1()
    { restart(); }

    static void calcDigestStatic(uint8_t *digest,
                                 const uint8_t *input,
                                 size_t length)
    {
        Sha1 sha;
        sha.update(input, length);
        sha.final(digest);
    }

    void update(const uint8_t *input, size_t length)
    { SHA1_Update(&myHash, input, length); }

    void final(uint8_t *digest)
    { SHA1_Final(digest, &myHash); }

    void restart()
    { SHA1_Init(&myHash); }
};

}  // namespace hash

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDSCRYPTO_H_
