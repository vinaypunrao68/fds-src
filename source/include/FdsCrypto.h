/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDSCRYPTO_H_
#define SOURCE_INCLUDE_FDSCRYPTO_H_

/**
 * This is a collection of crypto, checksum, hashing algorithms
 * that follow a common FDS interface and style.
 */
#include <string>
#include <cryptlib.h>
#include <sha.h>
#include <hash/MurmurHash3.h>

#include <fds_types.h>

namespace fds {

/**
 * Abstract base class that defines the interface to
 * fds hashing functions
 */
class FdsHashFunc {
  public:
    virtual ~FdsHashFunc() {}

    /**
     * Returns the name of the algorithm
     */
    virtual std::string getAlgorithmName() const = 0;
    /**
     * Calculates and returns a digest for the input
     * data. It assumes the digest is preallocated to
     * the correct length.
     * This function is stateless but is not static because
     * it's virtual. Can't have both.
     */
    virtual void calculateDigest(byte *digest,
                                 const byte *input,
                                 size_t length) = 0;
    /**
     * Updates a running digest with additional input.
     * The digest and state is maintained by the derived
     * class instance.
     */
    virtual void update(const byte *input, size_t length) = 0;
    /**
     * Returns the current digest state maintained by the
     * derived class and then resets digest to initial state.
     */
    virtual void final(byte *digest) = 0;
    /**
     * Restarts the digest state maintained by the derived class
     * to its initial state. This clear any previous state produced
     * with prior update() calls.
     */
    virtual void restart() = 0;
};

namespace hash {

class Sha1 : FdsHashFunc {
  private:
    CryptoPP::SHA1 myHash;
  public:
    Sha1();
    ~Sha1();

    static const fds_uint32_t numDigestBytes = CryptoPP::SHA1::DIGESTSIZE;
    static const fds_uint32_t numDigestBits  = numDigestBytes * 8;

    std::string getAlgorithmName() const;
    void calculateDigest(byte *digest,
                         const byte *input,
                         size_t length);
    static void calcDigestStatic(byte *digest,
                                 const byte *input,
                                 size_t length);

    void update(const byte *input, size_t length);
    void final(byte *digest);
    void restart();
};

class Murmur3 : FdsHashFunc {
  public:
    Murmur3();
    ~Murmur3();

    static const fds_uint32_t numDigestBytes = 16;
    static const fds_uint32_t numDigestBits  = numDigestBytes * 8;

    std::string getAlgorithmName() const;
    void calculateDigest(byte *digest,
                         const byte *input,
                         size_t length);
    static void calcDigestStatic(byte *digest,
                                 const byte *input,
                                 size_t length);

    // Note: murmur3 does NOT support incremental
    // hashing in its 128-bit version. Only 32-bit
    // incremental exists. Therefore, these functions
    // noop.
    void update(const byte *input, size_t length) {}
    void final(byte *digest) {}
    void restart() {}
};

}  // namespace hash
}  // namespace fds

#endif  // SOURCE_INCLUDE_FDSCRYPTO_H_
