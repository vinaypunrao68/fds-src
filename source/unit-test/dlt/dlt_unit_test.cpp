/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#define CATCH_CONFIG_MAIN
#include "../catch/catch.hpp"
#include "dlt_helper_funcs.h"
#include <serialize.h>
using namespace fds;
using namespace fds::serialize;

TEST_CASE ("TokenGroup") {
    DltTokenGroup tokenGroup(4);
    tokenGroup[0]=10;
    tokenGroup[1]=20;
    tokenGroup[2]=30;
    tokenGroup[3]=40;
    CAPTURE ( tokenGroup[2].uuid_get_val() );

    for (uint i=0;i<4;i++) {
        REQUIRE (tokenGroup[i].uuid_get_val() == (i+1)*10);
    }
}

TEST_CASE ("Assigning values", "dlt" ) {
    DLT dlt(3,4,1,true);
    CAPTURE(dlt.getNumBitsForToken());
    CAPTURE(dlt.getDepth());
    CAPTURE( dlt.getNumTokens() );

    fillDltNodes(&dlt,10);
    verifyDltNodes(&dlt,10);
}

TEST_CASE ("Node Positions", "dlt") {
    DLT dlt(3,4,1,true);
    
    CAPTURE(dlt.getNumBitsForToken());
    CAPTURE(dlt.getDepth());
    CAPTURE( dlt.getNumTokens() );
    fillDltNodes(&dlt,10);

    dlt.setNode(4,0,31);
    // printDlt(&dlt);

    dlt.generateNodeTokenMap();
    const TokenList& allTokens= dlt.getTokens(31);
    REQUIRE(allTokens.size() == 2);

    TokenList tokenList;
    dlt.getTokens(&tokenList,31,0);
    REQUIRE(tokenList.size() == 1);
    REQUIRE(tokenList[0] == 4);
}

TEST_CASE ("Dlt Manager") {    
    DLT dlt1(3,4,1,true);
    DLT dlt2(3,4,2,true);

    fillDltNodes(&dlt1,10);        
    fillDltNodes(&dlt2,100);

    DLTManager dltMgr;
    dltMgr.add(dlt1);
    dltMgr.add(dlt2);

    const DLT *ptr=dltMgr.getDLT(1);

    REQUIRE (ptr->getNumTokens() == 8);
    REQUIRE (ptr->getVersion() == 1);

    ptr=dltMgr.getDLT(2);
    REQUIRE (ptr->getVersion() == 2);
    verifyDltNodes(ptr,100);

    DLT* clone=const_cast<DLT*>(ptr)->clone();
    REQUIRE (clone->getVersion() == 2);
    verifyDltNodes(clone,100);
}

TEST_CASE("Tokens") {
    fds_uint32_t numBitsForToken=16;
    fds_uint32_t dltDepth=4;
    fds_uint32_t hashsize=sizeof(fds_uint64_t)*8;

    fds_uint64_t high((fds_uint64_t)7<<(hashsize-numBitsForToken)),low((fds_uint64_t)9<<(hashsize-numBitsForToken));
    ObjectID objId(high,low);
    
    REQUIRE( objId.GetHigh() == ((fds_uint64_t)7<<(hashsize-numBitsForToken)) ) ;
    REQUIRE( objId.GetLow() == ((fds_uint64_t)9<<(hashsize-numBitsForToken)) );

    DLT dlt(numBitsForToken,dltDepth,1);
    fds_token_id token=dlt.getToken(objId);
    
    REQUIRE(token == 7);
}

TEST_CASE ("Mem Serialize") {
    Serializer * ser = getMemSerializer();
    int num=20;
    ser->writeI32(num);
    std::string name="testtest";
    ser->writeString(name);
    num=32;
    ser->writeI32(num);

    std::string buffer=ser->getBufferAsString();

    Deserializer* des= getMemDeserializer(buffer);

    num=10;

    des->readI32(num);
    REQUIRE(20 == num);

    std::string newName;
    des->readString(newName);
    REQUIRE(newName == "testtest");

    des->readI32(num);
    REQUIRE(32 == num);
}

TEST_CASE ("DLT Serialize") {
    Serializer *s = getMemSerializer();

    DLT dlt(3,4,1,true);
    CAPTURE(dlt.getNumBitsForToken());
    CAPTURE(dlt.getDepth());
    CAPTURE( dlt.getNumTokens() );

    fillDltNodes(&dlt,10);
        
    uint32_t bytesWritten = dlt.write(s);
    CAPTURE(bytesWritten);

    //cout<<"byteswritten:"<<bytesWritten<<endl;

    std::string buffer = s->getBufferAsString();

    DLT dlt1(0,0,0,false);
    
    Deserializer *d = getMemDeserializer(buffer);
    
    uint32_t bytesRead = dlt1.read(d);
    CAPTURE(bytesRead);
    //cout<<"bytesRead:"<<bytesRead<<endl;

    REQUIRE( bytesWritten == bytesRead);
    //printDlt(&dlt1);
    verifyDltNodes(&dlt1,10);
    REQUIRE (dlt1.getVersion() == 1);
    REQUIRE (dlt1.getNumTokens() == 8);
    REQUIRE (dlt1.getDepth() == 4);
    
}
