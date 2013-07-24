/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Unit test driver for ObjectDB.
 */
#include <sstream>
#include <iostream>  // NOLINT(*)
#include <iomanip>
#include <ios>
#include <string>
#include <list>

#include "stor_mgr/odb.h"
#include "util/random.h"
#include "util/testutil.h"

namespace osm {

class UnitTest {
 private:
  std::list<std::string>  unit_tests;
  ObjectDB               *odb;
  std::string             file;

  std::string ToHex(const std::string& key) {
    std::ostringstream hash_oss;

    for (std::string::size_type i = 0; i < key.size(); ++i) {
      hash_oss << std::hex << std::setfill('0')
               << std::setw(2) << std::nouppercase
               << static_cast<int>(key[i]);
    }

    return hash_oss.str();
  }

  /** Basic functionality test.
   * @return 0 on success, < 0 on failure.
   */
  int basic_test() {
    fds::Error err(fds::ERR_OK);
    fds_uint32_t iterations = 1000;
    fds_uint32_t object_size = 4096;

    odb = new ObjectDB(file);

    DiskLoc   dl;
    dl.vol_id  = 0;
    dl.file_id = 0;
    ObjectBuf obj;
    std::string obj_hex;

    /*
     * Create random data generator
     * with some arbitrary seed.
     */
    leveldb::Random rnd(1234);

    for (fds_uint32_t i = 0; i < iterations; i++) {
      /*
       * Set disk location offset based on i.
       */
      dl.offset = i;

      /*
       * Get random 4K data.
       */
      obj.data.clear();
      leveldb::test::RandomString(&rnd, object_size, &obj.data);
      obj.size = object_size;

      err = odb->Put(dl, obj);
      if (err != fds::ERR_OK) {
        std::cout << "Failed to put at key " << dl.offset << std::endl;
        delete odb;
        return -1;
      }
    }
    std::cout << "Successfully put " << iterations
              << " objects of size " << object_size << std::endl;

    for (fds_uint32_t i = 0; i < iterations; i++) {
      dl.offset = i;

      ObjectBuf get_obj;
      err = odb->Get(dl, get_obj);
      if (err != fds::ERR_OK) {
        std::cout << "Failed to get key " << dl.offset << std::endl;
        delete odb;
        return -1;
      }
    }
    std::cout << "Successfully got " << iterations
              << " objects of size " << object_size << std::endl;

    odb->PrintHistoPut();
    odb->PrintHistoGet();

    delete odb;
    return 0;
  }  // basic_test

  int batch_test() {
    return 0;
  }  // batch_test

 public:
  explicit UnitTest(const std::string& filename) :
      odb(NULL),
      file(filename) {
    unit_tests.push_back("basic");
    unit_tests.push_back("batch");
  }

  void Run(const std::string& testname) {
    int result;
    std::cout << "Running unit test \"" << testname << "\"" << std::endl;

    if (testname == "basic") {
      result = basic_test();
    } else if (testname == "batch") {
      result = batch_test();
    } else {
      std::cout << "Unknown unit test " << testname << std::endl;
    }

    if (result == 0) {
      std::cout << "Unit test \"" << testname << "\" PASSED"  << std::endl;
    } else {
      std::cout << "Unit test \"" << testname << "\" FAILED" << std::endl;
    }
    std::cout << std::endl;
  }

  void Run() {
    for (std::list<std::string>::iterator it = unit_tests.begin();
         it != unit_tests.end();
         ++it) {
      Run(*it);
    }
  }
};

}  // namespace osm

int main(int argc, char *argv[]) {
    std::string db_filename;

    for (int i = 0; i < argc; i++) {
      if (strncmp(argv[i], "--filename=", 11) == 0) {
        db_filename = argv[i] + 11;
        std::cout << "Set filename to " << db_filename << std::endl;
      }
    }

    osm::UnitTest unittest(db_filename);
    unittest.Run();

    return 0;
}
