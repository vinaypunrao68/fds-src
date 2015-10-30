/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <vector>
#include <string>
#include <thread>
#include <gmock/gmock.h>
#include <util/sqliteDB.h>
#include <util/stringutils.h>
#include <fds_types.h>
#include <fds_process.h>

namespace fds {
std::string logname = "sqlitedb_test";

struct SqliteUnitTest : ::testing::Test {
    virtual void SetUp() override {
    }

    virtual void TearDown() override {
    }
};

TEST_F(SqliteUnitTest, SqliteDB) {

    std::unique_ptr<SqliteDB> sqliteDB_ptr(new SqliteDB("sqlite.db"));
    std::string query = "create table if not exists testTbl (id integer, filename text not null)";
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query));

    query = "create index if not exists testIdIdx on testTbl (id)";
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query));

    query = util::strformat("insert into testTbl (id, filename) values (%ld, '%s')", 1, "file1");
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query));

    query = util::strformat("insert into testTbl (id, filename) values (%ld, '%s')", 1, "file11");
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query));

    query = util::strformat("insert into testTbl (id, filename) values (%ld, '%s')", 2, "file2");
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query));

    query = util::strformat("insert into testTbl (id, filename) values (%ld, '%s')", 3, "file3");
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query));

    query = util::strformat("insert into testTbl (id, filename) values (%ld, '%s')", 4, "file4");
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query));

    fds_uint64_t value;
    query = util::strformat("select id from testTbl where filename='%s'", "file1"); 
    sqliteDB_ptr->getIntValue(query, value);
    EXPECT_EQ(1, value);
    
    query = util::strformat("select id from testTbl where filename='%s'", "file2");
    sqliteDB_ptr->getIntValue(query, value);
    EXPECT_EQ(2, value);
    
    query = util::strformat("select id from testTbl where filename='%s'", "file3");
    sqliteDB_ptr->getIntValue(query, value);
    EXPECT_EQ(3, value);
    
    query = util::strformat("select id from testTbl where filename='%s'", "file4"); 
    sqliteDB_ptr->getIntValue(query, value);
    EXPECT_EQ(4, value);

    std::set<std::string> filenames;
    query = util::strformat("select filename from testTbl where id=%ld", 1);
    sqliteDB_ptr->getTextValues(query, filenames);
    EXPECT_THAT(filenames, ::testing::ElementsAre("file1", "file11"));

    query = util::strformat("delete from testTbl where id=%ld and filename='%s'", 1, "file11");
    EXPECT_EQ(ERR_OK, sqliteDB_ptr->execute(query));

    sqliteDB_ptr->dropDB();
}

} // end namespace fds

int main(int argc, char** argv) {
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

