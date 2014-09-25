
#ifndef SOURCE_UNIT_TEST_DLT_DLT_H_
#define SOURCE_UNIT_TEST_DLT_DLT_H_

#include <iostream>
#include <bitset>
#include <dlt.h>
#include <iomanip>


using namespace std;
using namespace fds;
void fillDltNodes(DLT* dlt,int multiplier) {
    for (uint i=0;i<dlt->getNumTokens();i++) {
        for (uint j=0;j<dlt->getDepth();j++) {
            dlt->setNode(i,j,(i%10)*multiplier+j);
        }
    }
}


void verifyDltNodes(const DLT* dlt,int multiplier) {
    for (uint i=0;i<dlt->getNumTokens();i++) {
        DltTokenGroupPtr ptr=dlt->getNodes(i);
        for (uint j=0;j<dlt->getDepth();j++) {
            REQUIRE(ptr->get(j).uuid_get_val()== (i%10)*multiplier+j);
        }
    }
}

void printBits(std::string name,fds_uint64_t num) {
    bitset<64> bits = bitset<64>(num);
    cout<<setw(10)<<name<<":"<<bits<<endl;
}

#if 0  //  SAN
//ignore this ...
void testTokens() {
    //16/32/48/64
    fds_uint64_t high((fds_uint64_t)7<<48),low((fds_uint64_t)15<<48);

    cout<<setw(10)<<"pos"<<":"<<"0123456789012345678901234567890123456789012345678901234567890123"<<endl;
    cout<<setw(10)<<"pos"<<":"<<"3210987654321098765432109876543210987654321098765432109876543210"<<endl;


    printBits("high",high);
    printBits("low",low);

    ObjectID objId(high,low);
    printBits("o.hi",objId.GetHigh());
    printBits("o.lo",objId.GetLow());
    //cout<< objId.ToHex() <<endl;
    DLT dlt(16,4,1);
    fds_token_id token=dlt.getToken(objId);
    
    printBits("token",token);
    cout<<token<<endl;
    printBits("bitmsk",((1 << 16) - 1));

}
#endif
#endif
