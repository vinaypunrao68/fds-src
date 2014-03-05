/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <FdsCrypto.h>

int main(int argc, char* argv[]) {
    fds::hash::Sha1 myHash;
    std::string testStr = "hello world";
    
    byte digest[fds::hash::Sha1::numDigestBytes];

    myHash.calculateDigest(digest, (const byte *)testStr.c_str(), testStr.size());

    std::cout << "0x";
    for (fds_uint32_t i = 0; i < fds::hash::Sha1::numDigestBytes; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(digest[i]);
    }
    std::cout << std::endl;

    byte digest2[fds::hash::Sha1::numDigestBytes];
    for (fds_uint32_t i = 0; i < testStr.size(); i++) {
        myHash.update((const byte *)&(testStr[i]), sizeof(testStr[i]));
    }
    myHash.final(digest2);

    std::cout << "0x";
    for (fds_uint32_t i = 0; i < fds::hash::Sha1::numDigestBytes; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(digest2[i]);
    }
    std::cout << std::endl;

    fds::hash::Murmur3 murmurHash;
    byte digest3[fds::hash::Murmur3::numDigestBytes];
    murmurHash.calculateDigest(digest3, (const byte *)testStr.c_str(), testStr.size());

    std::cout << "0x";
    for (fds_uint32_t i = 0; i < fds::hash::Murmur3::numDigestBytes; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(digest3[i]);
    }
    std::cout << std::endl;

    return 0;
}
