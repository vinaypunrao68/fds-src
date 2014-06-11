/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#define CATCH_CONFIG_MAIN
#include "../catch/catch.hpp"
#include <serialize.h>
#include <unistd.h>
#include <DataMgr.h>
#include <VolumeMeta.h>

using namespace fds;
using namespace fds::serialize;

namespace fds {
DataMgr *dataMgr;
}  // namespace fds


TEST_CASE("ObjectId") {
    ObjectID id1, id2;

    id1.SetId("company");
    std::string buffer;
    id1.getSerialized(buffer);
    id2.loadSerialized(buffer);
    // std::cout << id1.ToHex() << ":" << id2.ToHex() << std::endl;
    REQUIRE(id1.ToString() == id2.ToHex());
}



TEST_CASE("Metadata") {
    MetadataPair meta,meta1;
    meta.key = "name";
    meta.value = "fds";

    std::string buffer;
    meta.getSerialized(buffer);
    meta1.loadSerialized(buffer);

    REQUIRE(meta.key == meta1.key);
    REQUIRE(meta.value == meta1.value);
}

TEST_CASE("BlobObjectInfo") {
    BlobObjectInfo b1,b2;

    b1.offset = 101;
    b1.size   = 200;
    b1.sparse = false;
    b1.blob_end = true;

    std::string buffer;
    b1.getSerialized(buffer);
    b2.loadSerialized(buffer);

    REQUIRE(b2.offset == 101);
    REQUIRE(b2.size   == 200);
    REQUIRE(b2.sparse == false);
    REQUIRE(b2.blob_end == true);
}

TEST_CASE("BlobObjectList") {
    BlobObjectInfo b1,b2;
    BlobObjectList l1,l2;

    b1.offset = 101;
    b1.size   = 200;
    b1.sparse = false;
    b1.blob_end = true;
    l1[b1.offset] = b1;

    b1.offset = 102;
    b1.size   = 200;
    b1.sparse = false;
    b1.blob_end = true;
    l1[b1.offset] = b1;

    std::string buffer;
    l1.getSerialized(buffer);
    l2.loadSerialized(buffer);

    REQUIRE(l2.size()  == 2);
    REQUIRE(l2[101].size   == 200);
    REQUIRE(l2[101].offset == 101);
    REQUIRE(l2[102].offset == 102);
}

TEST_CASE("BlobNode") {
    BlobNode n1, n2;
    BlobObjectInfo b1, b2;

    b1.offset = 101;
    b1.size   = 200;
    b1.sparse = false;
    b1.blob_end = true;
    n1.obj_list[b1.offset] = b1;


    b1.offset = 102;
    b1.size   = 200;
    b1.sparse = false;
    b1.blob_end = true;
    n1.obj_list[b1.offset] = b1;

    n1.blob_name = "fds";
    n1.version   = 10;
    n1.vol_id    = 1;
    n1.meta_list.push_back(MetadataPair("company" , "fds"));
    n1.meta_list.push_back(MetadataPair("ceo" , "Mark Lewis"));

    std::string buffer;
    n1.getSerialized(buffer);
    n2.loadSerialized(buffer);

    REQUIRE(n2.blob_name == "fds");
    REQUIRE(n2.version == 10);
    REQUIRE(n2.vol_id  == 1);

    REQUIRE(n2.meta_list.size()  == 2);
    REQUIRE(n2.meta_list[1].key  == "ceo");
    REQUIRE(n2.obj_list[101].size   == 200);
    REQUIRE(n2.obj_list[101].offset == 101);
    REQUIRE(n2.obj_list[102].offset == 102);
}
