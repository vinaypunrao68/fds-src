/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#define CATCH_CONFIG_MAIN
#include "../../include/catch.hpp"
#include "dlt_helper_funcs.h"
#include <serialize.h>
#include <unistd.h>

using namespace fds;
using namespace fds::serialize;

TEST_CASE ("TokenGroup" , "[token]") {
    DltTokenGroup tokenGroup(4);
    tokenGroup[0]=10;
    tokenGroup[1]=20;
    tokenGroup[2]=30;
    tokenGroup[3]=40;
    CAPTURE ( tokenGroup[2].uuid_get_val() );
    //cout << tokenGroup ; 
    for (uint i=0;i<4;i++) {
        REQUIRE (tokenGroup[i].uuid_get_val() == (i+1)*10);
    }
}

TEST_CASE ("Assigning values", "[dlt]" ) {
    DLT dlt(3,4,1,true);
    CAPTURE(dlt.getNumBitsForToken());
    CAPTURE(dlt.getDepth());
    CAPTURE( dlt.getNumTokens() );

    fillDltNodes(&dlt,10);
    verifyDltNodes(&dlt,10);

    fillDltNodes(&dlt,0);
    dlt.generateNodeTokenMap();
    LOGNORMAL << dlt ;
}

TEST_CASE ("Node Positions", "[dlt]") {
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

TEST_CASE ("Dlt Manager" , "[mgr]") {
    DLT dlt1(3,4,1,true);
    DLT dlt2(3,4,2,true);
    DLT dlt3(3,4,3,true);

    fillDltNodes(&dlt1,10);        
    fillDltNodes(&dlt2,100);
    fillDltNodes(&dlt3,20);

    fds_uint8_t maxDlts=2;
    DLTManager dltMgr(maxDlts);
    dltMgr.add(dlt1);
    dltMgr.add(dlt2);
    
    // check teh first DLT
    const DLT *ptr=dltMgr.getDLT(1);
    REQUIRE (ptr->getNumTokens() == 8);
    REQUIRE (ptr->getVersion() == (fds_uint64_t) 1);

    // check the second DLT
    ptr=dltMgr.getDLT(2);
    REQUIRE (ptr->getVersion() == (fds_uint64_t) 2);
    verifyDltNodes(ptr,100);

    // check cloning
    DLT* clone=const_cast<DLT*>(ptr)->clone();
    REQUIRE (clone->getVersion() == (fds_uint64_t) 2);
    verifyDltNodes(clone,100);

    // check adding Serialized
    std::string buffer;
    dlt3.getSerialized(buffer);

    dltMgr.addSerializedDLT(buffer);

    ptr=dltMgr.getDLT(3);
    REQUIRE (ptr->getVersion() == (fds_uint64_t) 3);
    verifyDltNodes(ptr,20);

    
}

TEST_CASE("Tokens" , "[token]") {
    fds_uint32_t numBitsForToken=16;
    fds_uint32_t dltDepth=4;
    fds_uint32_t hashsize=sizeof(fds_uint64_t)*8;

#if 0    // SAN 
    fds_uint64_t high((fds_uint64_t)7<<(hashsize-numBitsForToken)),low((fds_uint64_t)9<<(hashsize-numBitsForToken));
    ObjectID objId(high,low);
    
    REQUIRE( objId.GetHigh() == ((fds_uint64_t)7<<(hashsize-numBitsForToken)) ) ;
    REQUIRE( objId.GetLow() == ((fds_uint64_t)9<<(hashsize-numBitsForToken)) );

    DLT dlt(numBitsForToken,dltDepth,1);
    fds_token_id token=dlt.getToken(objId);
    
    REQUIRE(token == 7);
#endif 
}

TEST_CASE ("Mem Serialize" ,"[serialize]") {
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

TEST_CASE ("DLT Serialize" , "[dlt][serialize]") {
    Serializer *s = getMemSerializer();

    DLT dlt(8,4,1,true);
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
    REQUIRE (dlt1.getVersion() == (fds_uint64_t) 1);
    REQUIRE (dlt1.getNumTokens() == pow(2,dlt1.getNumBitsForToken()));
    REQUIRE (dlt1.getDepth() == 4);
    
}

TEST_CASE ("DLT Serialize2","[dlt][serialize]") {
    DLT dlt(8,4,1,true);
    CAPTURE(dlt.getNumBitsForToken());
    CAPTURE(dlt.getDepth());
    CAPTURE(dlt.getNumTokens());

    fillDltNodes(&dlt,10);
        
    std::string buffer;
    dlt.getSerialized(buffer);

    DLT dlt1(0,0,0,false);
    
    dlt1.loadSerialized(buffer);
    //printDlt(&dlt1);
    verifyDltNodes(&dlt1,10);
    REQUIRE (dlt1.getVersion() == (fds_uint64_t) 1);
    REQUIRE (dlt1.getNumTokens() == pow(2,dlt1.getNumBitsForToken()));
    REQUIRE (dlt1.getDepth() == 4);
}

TEST_CASE ("DLT Manager Serialize" ,"[dlt][serialize][mgr]") {
    DLT dlt1(3,4,1,true);
    DLT dlt2(3,4,2,true);
    DLT dlt3(3,4,3,true);

    fillDltNodes(&dlt1,10);
    fillDltNodes(&dlt2,100);
    fillDltNodes(&dlt3,20);

    fds_uint8_t maxDlts=2;
    DLTManager* dltMgr = new DLTManager(maxDlts);
    dltMgr->add(dlt1);
    dltMgr->add(dlt2);
    dltMgr->add(dlt3);

    SECTION ("serialize") {
        Serializer *s = serialize::getMemSerializer();
        uint32_t bytesWritten = dltMgr->write(s);
        std::string buffer= s->getBufferAsString();
        
        Deserializer *d = serialize::getMemDeserializer(buffer);
        uint32_t bytesRead = dltMgr->read(d);
        REQUIRE( bytesWritten == bytesRead );
    }

    SECTION ("load/store on file") {
        std::string filename="/tmp/dlt.mgr.data";
        unlink(filename.c_str());
        dltMgr->storeToFile(filename);
        dltMgr = new DLTManager();
        dltMgr->loadFromFile(filename);
    }

    const DLT* ptr;

    ptr=dltMgr->getDLT(1);
    REQUIRE (ptr->getVersion() == (fds_uint64_t) 1);
    verifyDltNodes(ptr,10);

    ptr=dltMgr->getDLT(2);
    REQUIRE (ptr->getVersion() == (fds_uint64_t) 2);
    verifyDltNodes(ptr,100);

    ptr=dltMgr->getDLT(3);
    REQUIRE (ptr->getVersion() == (fds_uint64_t) 3);
    verifyDltNodes(ptr,20);

    const_cast<DLT*>(ptr)->generateNodeTokenMap();
}
