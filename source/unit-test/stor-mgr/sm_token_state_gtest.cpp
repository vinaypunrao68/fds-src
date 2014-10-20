/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <set>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_process.h>
#include <object-store/SmTokenState.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

static std::string logname = "smtoken_state";

TEST(SmTokenState, initialize) {
    TokenDescTable tbl;
    GLOGNORMAL << "Newly allocated table must be invalid";
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
        fds_uint16_t fileId = tbl.getWriteFileId(tok, diskio::diskTier);
        EXPECT_EQ(fileId, SM_INVALID_FILE_ID);
        fileId = tbl.getWriteFileId(tok, diskio::flashTier);
        EXPECT_EQ(fileId, SM_INVALID_FILE_ID);
    }
    // there must be no valid tokens
    SmTokenSet tokSet = tbl.getSmTokens();
    EXPECT_EQ(tokSet.size(), 0);

    for (fds_uint32_t run = 0; run < 5; ++run) {
        SmTokenSet toks;
        if (run > 0) {
            // create token set with all some tokens
            for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; tok += run) {
                toks.insert(tok);
            }
        }

        // initialize table
        TokenDescTable tokTbl;
        tokTbl.initialize(toks);
        GLOGNORMAL << "Run " << run << ": Initial token desc table - " << tokTbl;
        for (SmTokenSet::const_iterator cit = toks.cbegin();
             cit != toks.cend();
             ++cit) {
            fds_uint16_t fileId = tokTbl.getWriteFileId(*cit, diskio::diskTier);
            EXPECT_EQ(fileId, SM_INIT_FILE_ID);
            fileId = tokTbl.getWriteFileId(*cit, diskio::flashTier);
            EXPECT_EQ(fileId, SM_INIT_FILE_ID);
        }
        SmTokenSet retTokSet = tbl.getSmTokens();
        EXPECT_TRUE(retTokSet == tokSet);
    }
}

TEST(SmTokenState, update) {
    SmTokenSet tokSet;
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; tok += 2) {
        tokSet.insert(tok);
    }
    TokenDescTable tokTbl;
    tokTbl.initialize(tokSet);

    for (SmTokenSet::const_iterator cit = tokSet.cbegin();
         cit != tokSet.cend();
         ++cit) {
        fds_uint16_t fileId = 0x0012;
        tokTbl.setWriteFileId(*cit, diskio::diskTier, fileId);
        tokTbl.setCompactionState(*cit, diskio::diskTier, true);

        fds_uint16_t fid = tokTbl.getWriteFileId(*cit, diskio::diskTier);
        EXPECT_EQ(fileId, fid);
        EXPECT_TRUE(tokTbl.isCompactionInProgress(*cit, diskio::diskTier));
    }
    GLOGNORMAL << "Every other token has 0x0012 fileId and compaction flag -- "
               << tokTbl;
}

TEST(SmTokenState, comparison) {
    GLOGDEBUG << "Testing SmTokenState comparison";
    SmTokenSet tokSet;
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; tok += 5) {
        tokSet.insert(tok);
    }
    TokenDescTable tokTbl;
    tokTbl.initialize(tokSet);
    GLOGNORMAL << "Initial computation - " << tokTbl;

    TokenDescTable tokTbl2;
    tokTbl2.initialize(tokSet);
    GLOGNORMAL << "Initial computation 2 - " << tokTbl;
    EXPECT_TRUE(tokTbl == tokTbl2);

    // change first token
    SmTokenSet::const_iterator cit = tokSet.cbegin();
    EXPECT_TRUE(cit != tokSet.cend());
    tokTbl2.setCompactionState(*cit, diskio::diskTier, true);
    EXPECT_FALSE(tokTbl == tokTbl2);

    // change it back
    tokTbl2.setCompactionState(*cit, diskio::diskTier, false);
    EXPECT_TRUE(tokTbl == tokTbl2);
}
}  // namespace fds

int main(int argc, char * argv[]) {
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

