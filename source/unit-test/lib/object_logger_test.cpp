/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>

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

  private:
    int32_t num;
    bool flag;
    std::string data;
};

int main(int argc, char * argv[]) {
    ObjectLogger logger("obj_file.log");

    while (true) {
        for (int i = 0; i < 10000; ++i) {
            ObjType obj;
            logger.log(&obj);
        }
        sleep(1);
    }

    return 0;
}

