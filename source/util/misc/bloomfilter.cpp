
#include <hash/MurmurHash2.h>
#include <util/bloomfilter.h>
namespace fds { namespace util {

uint32_t seed[] = {
    0xABABABAB, 0xABCBCBCB, 0xDBDBDBDB, 0xEBAFAFAB,
    0xCBAFADAB, 0x1245ABAB, 0x7890ABAB, 0xABAB7890,
    0xDEFB4378, 0x37089DAE, 0x9089774F, 0xDE890723,
    0x4097878F, 0x0899078A, 0x098636BE, 0x5630873F,
    0xBBABABAB, 0xBBCBCBCB, 0xBBDBDBDB, 0xBBAFAFAB,
    0xBBAFADAB, 0xB245ABAB, 0xBB90ABAB, 0xBBAB7890,
    0xBEFB4378, 0xB7089DAE, 0xB089774F, 0xBE890723,
    0xB097878F, 0xB899078A, 0xB98636BE, 0xB630873F
};


BloomFilter::BloomFilter(uint32_t totalBits, uint8_t bitsPerKey) :
        totalBits(totalBits), bitsPerKey(bitsPerKey) {
    bits.reset(new boost::dynamic_bitset<>(totalBits));
}

void BloomFilter::add(const ObjectID& objID) {
    std::vector<uint32_t> positions = generatePositions(objID.GetId(), objID.getDigestLength());
    add(positions);
}

void BloomFilter::add(const std::string& data) {
    std::vector<uint32_t> positions = generatePositions(data.data(), data.length());
    add(positions);
}

bool BloomFilter::lookup(const ObjectID& objID) const {
    std::vector<uint32_t> positions = generatePositions(objID.GetId(), objID.getDigestLength());
    return lookup(positions);
}

bool BloomFilter::lookup(const std::string& data) const {
    std::vector<uint32_t> positions = generatePositions(data.data(), data.length());
    return lookup(positions);
}

void BloomFilter::add(const std::vector<uint32_t>& positions) {
    for (uint i = 0 ; i < bitsPerKey ; i++) {
        bits->set(positions[i], true);
    }
}

bool BloomFilter::lookup(const std::vector<uint32_t>& positions) const {
    for (uint i = 0 ; i < bitsPerKey ; i++) {
        if (!bits->test(positions[i])) return false;
    }
    return true;
}

void BloomFilter::merge(const BloomFilter& filter) {
   *bits |= *(filter.bits); 
}

std::vector<uint32_t> BloomFilter::generatePositions(const void* data, uint32_t len) const {
    std::vector<uint32_t> positions;
    positions.reserve(bitsPerKey);
    for (uint i = 0 ; i < bitsPerKey ; i++) {
        positions.push_back(MurmurHash2(data, len, seed[i]) % totalBits);
    }
    return positions;
}

uint32_t BloomFilter::write(serialize::Serializer*  s) const {
    uint32_t bytes = 0;
    std::string data;
    bytes += s->writeI32(bitsPerKey);
    bytes += s->writeI32(totalBits);
    boost::to_string(*bits, data);
    bytes += s->writeString(data);
    return bytes;
}

uint32_t BloomFilter::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    std::string data;
    bytes += d->readI32(bitsPerKey);
    bytes += d->readI32(totalBits);
    bytes += d->readString(data);
    fds_assert(data.length() == totalBits);
    bits.reset(new boost::dynamic_bitset<>(data));
    return bytes;
    
}

uint32_t BloomFilter::getEstimatedSize() const {
    uint32_t bytes = 0;
    bytes += bits->size() + 10*4 ;
    return bytes;
}
}  // namespace util
}  // namespace fds
