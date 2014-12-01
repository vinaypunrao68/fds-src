/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <FdsCrypto.h>

int main(int argc, char* argv[]) {
    /*
     * Sha1 full hash test
     */
    fds::hash::Sha1 sha1Hash;
    std::string testStr = "hello world";
    
    byte sha1FullDigest[fds::hash::Sha1::numDigestBytes];

    sha1Hash.calculateDigest(sha1FullDigest, (const byte *)testStr.c_str(), testStr.size());

    std::cout << "0x";
    for (fds_uint32_t i = 0; i < fds::hash::Sha1::numDigestBytes; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2)
                  << static_cast<int>(sha1FullDigest[i]);
    }
    std::cout << std::endl;

    /*
     * Sha1 incremental hash test
     */
    byte sha1IncDigest[fds::hash::Sha1::numDigestBytes];
    for (fds_uint32_t i = 0; i < testStr.size(); i++) {
        sha1Hash.update((const byte *)&(testStr[i]), sizeof(testStr[i]));
    }
    sha1Hash.final(sha1IncDigest);

    std::cout << "0x";
    for (fds_uint32_t i = 0; i < fds::hash::Sha1::numDigestBytes; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2)
                  << static_cast<int>(sha1IncDigest[i]);
    }
    std::cout << std::endl;

    /*
     * Murmur3 test
     */
    fds::hash::Murmur3 murmurHash;
    byte mmDigest[fds::hash::Murmur3::numDigestBytes];
    murmurHash.calculateDigest(mmDigest, (const byte *)testStr.c_str(), testStr.size());

    std::cout << "0x";
    for (fds_uint32_t i = 0; i < fds::hash::Murmur3::numDigestBytes; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2)
                  << static_cast<int>(mmDigest[i]);
    }
    std::cout << std::endl;

    /*
     * Sha256 full hash test
     */
    fds::hash::Sha256 sha256Hash;
    byte sha256FullDigest[fds::hash::Sha256::numDigestBytes];

    sha256Hash.calculateDigest(sha256FullDigest, (const byte *)testStr.c_str(), testStr.size());

    std::cout << "0x";
    for (fds_uint32_t i = 0; i < fds::hash::Sha256::numDigestBytes; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2)
                  << static_cast<int>(sha256FullDigest[i]);
    }
    std::cout << std::endl;

    /*
     * Sha256 incremental hash test
     */
    byte sha256IncDigest[fds::hash::Sha256::numDigestBytes];
    for (fds_uint32_t i = 0; i < testStr.size(); i++) {
        sha256Hash.update((const byte *)&(testStr[i]), sizeof(testStr[i]));
    }
    sha256Hash.final(sha256IncDigest);

    std::cout << "0x";
    for (fds_uint32_t i = 0; i < fds::hash::Sha256::numDigestBytes; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2)
                  << static_cast<int>(sha256IncDigest[i]);
    }
    std::cout << std::endl;

    return 0;
}
