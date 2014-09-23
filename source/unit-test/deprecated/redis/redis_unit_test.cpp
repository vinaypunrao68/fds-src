#define CATCH_CONFIG_MAIN
#include "../catch/catch.hpp"

#include <kvstore/redis.h>
#include <iostream>
#include <util/Log.h>

using namespace redis;
using namespace std;
using namespace fds;

TEST_CASE("invalid-host") {
    try {
        Redis* r= new Redis("xxxxx");
        r->sendCommand("ping");
    } catch(RedisException &e ) {
        LOGWARN << e.what() ;
    }
}

TEST_CASE ("hash") {
    Redis* r= new Redis();
    Reply reply = r->sendCommand("del testhash");
    //reply.dump();
    reply = r->sendCommand("ping");
    //reply.dump();
    reply = r->sendCommand("hmset test:hash one 1 two 2 three 3");
    //reply.dump();
        
    std::vector<string> vec;
    
    reply = r->hgetall("test:hash");
    //reply.dump();
    
    reply.toVector(vec);
    
    
    REQUIRE (vec.size() == 6);
    REQUIRE (vec[0] == "one");
    REQUIRE (vec[5] == "3");
    
    delete r;
}

TEST_CASE ("get-set") {
    for ( uint i=0; i<10 ; i++ ) {
    Redis* r= new Redis();
    Reply reply = r->set("test:name","fds");
    //reply.dump();
    REQUIRE (reply.getStatus() == "OK" );
    reply = r->get("test:name");

    REQUIRE ( reply.getString() == "fds" );
    delete r;
    }
}

TEST_CASE ("binary") {    
    std::string s,h,s1;
    s.append(1,10);
    s.append(1,0);
    s.append(1,3);
    
    Redis::encodeHex(s,h);
    Redis* r= new Redis();
    r->sendCommand("set test:binval %s",h.c_str());
    Reply reply= r->get("test:binval");
    Redis::decodeHex(reply.getString(),s1);
    REQUIRE ( s1.length() == 3 );
    REQUIRE ( s1[0] == 10 );
    REQUIRE ( s1[2] == 3 );

    reply = r->set("test:binval2",s);
    reply = r->get("test:binval2");
    reply.dump();
    s1= reply.getString();
    REQUIRE ( s1.length() == 3 );
    REQUIRE ( s1[0] == 10 );
    REQUIRE ( s1[2] == 3 );


    delete r;
}
