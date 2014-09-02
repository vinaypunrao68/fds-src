/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <vector>
#include <limits>

#include <serialize.h>
#include <ObjectLogger.h>

using namespace fds;    // NOLINT

static int32_t epoch = 0;

struct ObjType : serialize::Serializable {
    virtual uint32_t write(serialize::Serializer * s) const override {
        uint32_t bytes = s->writeI32(num);
        bytes += s->writeBool(flag);
        bytes += s->writeString(data);
        return bytes;
    }

    virtual uint32_t read(serialize::Deserializer * d) override {
        uint32_t bytes = d->readI32(num);
        bytes += d->readBool(flag);
        bytes += d->readString(data);
        return bytes;
    }

    ObjType() : num(epoch++), flag(num % 2) {
        char * ptr = static_cast<char *>(malloc(500));
        data = std::string(ptr, 500);
        delete ptr;
    }

    int32_t num;
    bool flag;
    std::string data;
};

namespace fds {
template class ObjectLogger::Iterator<ObjType>;

template boost::shared_ptr<ObjectLogger::Iterator<ObjType> >
        ObjectLogger::newIterator(fds_int32_t index);
}

int main(int argc, char * argv[]) {
    ObjectLogger logger("obj_file.log", std::numeric_limits<fds_uint64_t>::max(), 0);

    while (true) {
        for (int i = 0; i < 100; ++i) {
            ObjType obj;
            logger.log(&obj);
        }

        boost::shared_ptr<ObjectLogger::Iterator<ObjType> > iter =
                logger.newIterator<ObjType>(-1);
        for (; iter->valid(); iter->next()) {
            boost::shared_ptr<ObjType> val = iter->value();
            std::cout << val->num << " " << val->flag << std::endl;
        }

        sleep(10);
    }

    return 0;
}

