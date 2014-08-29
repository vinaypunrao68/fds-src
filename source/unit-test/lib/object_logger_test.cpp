/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <vector>

#include <ObjectLogger.h>
#include <serialize.h>

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
template fds_uint32_t ObjectLogger::getObjects(fds_int32_t fileIndex,
            std::vector<boost::shared_ptr<ObjType> > & objects, fds_uint32_t max);
}

int main(int argc, char * argv[]) {
    ObjectLogger logger("obj_file.log", 1024 * 1024, 0, true);

    while (true) {
        for (int i = 0; i < 10000; ++i) {
            ObjType obj;
            logger.log(&obj);
        }

        /*
        std::vector<boost::shared_ptr<ObjType> > objs;
        fds_uint32_t count = logger.getObjects(-1, objs);
        fds_assert(100 == count);
        for (auto it : objs) {
            std::cout << it->num << " " << it->flag << std::endl;
        }
        */

        sleep(1);
    }

    return 0;
}

