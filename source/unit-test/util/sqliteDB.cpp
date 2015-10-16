/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <testlib/SvcMsgFactory.h>
#include <vector>
#include <string>
#include <thread>
#include <google/profiler.h>

#include <util/sqliteDB/sqliteDB.h>


struct SqliteUnitTest : ::testing::Test {
    virtual void SetUp() override {
    }

    virtual void TearDown() override {
    }
};

TEST_F(SqliteUnitTest, SqliteDB) {

    std::unique_ptr<SqliteDB> sqliteDB_ptr("/fds/user-repo/sqlite.db");
    std::string query = "create table if not exists testTbl (id integer filename test not null)";

    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query);

    std::string query = util::strformat("insert into testTbl (id filename) values (%ld, '%s')", 1, "file1");
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query);

    query = util::strformat("insert into testTbl (id filename) values (%ld, '%s')", 2, "file2");
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query);

    query = util::strformat("insert into testTbl (id filename) values (%ld, '%s')", 3, "file3");
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query);

    query = util::strformat("insert into testTbl (id filename) values (%ld, '%s')", 4, "file4");
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query);

    int value;
    query = util::strformat("select id from testTbl where filename=%s", "file1"); 
    sqliteDB->getIntValue(query, value);
    EXPECT_EQ(1, value);
    
    query = util::strformat("select id from testTbl where filename=%s", "file2");
    sqliteDB->getIntValue(query, value);
    EXPECT_EQ(2, value);
    
    
    query = util::strformat("select id from testTbl where filename=%s", "file3");
    sqliteDB->getIntValue(query, value);
    EXPECT_EQ(3, value);
    
    
    query = util::strformat("select id from testTbl where filename=%s", "file4"); 
    sqliteDB->getIntValue(query, value);
    EXPECT_EQ(4, value);
    
    
}

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
