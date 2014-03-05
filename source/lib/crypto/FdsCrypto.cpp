/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>

#include <FdsCrypto.h>
#include <sha.h>

namespace fds {
namespace hash {

// SHA1 implementations
Sha1::Sha1() {
}

Sha1::~Sha1() {
}

std::string
Sha1::getAlgorithmName() const {
    return "SHA1";
}

void
Sha1::calculateDigest(byte *digest,
                          const byte *input,
                          size_t length) {
    calcDigestStatic(digest, input, length);
}

void
Sha1::calcDigestStatic(byte *digest,
                           const byte *input,
                           size_t length) {
    CryptoPP::SHA1().CalculateDigest(digest,
                                     input,
                                     length);
}

void
Sha1::update(const byte *input, size_t length) {
    myHash.Update(input, length);
}

void
Sha1::final(byte *digest) {
    myHash.Final(digest);
}

void
Sha1::restart() {
    myHash.Restart();
}

// SHA256 implementations
Sha256::Sha256() {
}

Sha256::~Sha256() {
}

std::string
Sha256::getAlgorithmName() const {
    return "SHA256";
}

void
Sha256::calculateDigest(byte *digest,
                          const byte *input,
                          size_t length) {
    calcDigestStatic(digest, input, length);
}

void
Sha256::calcDigestStatic(byte *digest,
                           const byte *input,
                           size_t length) {
    CryptoPP::SHA256().CalculateDigest(digest,
                                     input,
                                     length);
}

void
Sha256::update(const byte *input, size_t length) {
    myHash.Update(input, length);
}

void
Sha256::final(byte *digest) {
    myHash.Final(digest);
}

void
Sha256::restart() {
    myHash.Restart();
}

// Murmur3 implementation
Murmur3::Murmur3() {
}

Murmur3::~Murmur3() {
}

std::string
Murmur3::getAlgorithmName() const {
    return "MURMUR3";
}

void
Murmur3::calculateDigest(byte *digest,
                         const byte *input,
                         size_t length) {
    calcDigestStatic(digest, input, length);
}

void
Murmur3::calcDigestStatic(byte *digest,
                         const byte *input,
                         size_t length) {
    MurmurHash3_x64_128(input,
                        length,
                        0,
                        digest);
}

}  // namespace hash
}  // namespace fds
