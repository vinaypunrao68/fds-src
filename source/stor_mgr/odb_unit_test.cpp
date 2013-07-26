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
#include <vector>

#include "stor_mgr/odb.h"
#include "util/random.h"
#include "util/testutil.h"
#include "util/hash/MurmurHash3.h"

#define MURMUR_SIZE 4

namespace osm {

class UnitTest {
 private:
  std::list<std::string>  unit_tests;
  ObjectDB               *odb;
  std::string             file;

  std::string ToHex(const fds_uint32_t *key, fds_uint32_t len) {
    std::ostringstream hash_oss;

    hash_oss << std::hex << std::setfill('0') << std::nouppercase;
    for (std::string::size_type i = 0; i < len; ++i) {
      hash_oss << std::setw(8) << key[i];
    }

    return hash_oss.str();
  }

  std::string ToHex(const std::string& key) {
    std::ostringstream hash_oss;

    hash_oss << std::hex << std::setfill('0') << std::nouppercase;
    for (std::string::size_type i = 0; i < key.size(); ++i) {
      hash_oss << std::setw(2) << static_cast<int>(key[i]);
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
    fds_uint32_t murmur_buf[MURMUR_SIZE];
    std::string murmur_hex;
    std::vector<std::string> written_objs;

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

      /*
       * Calculate and store murmur3 hash.
       */
      memset(murmur_buf, 0x00, sizeof(fds_uint32_t) * MURMUR_SIZE);
      MurmurHash3_x86_128(obj.data.c_str(),
                          obj.data.size(),
                          0,
                          murmur_buf);
      murmur_hex = ToHex(murmur_buf, MURMUR_SIZE);
      written_objs.push_back(murmur_hex);

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

      /*
       * Re-calculate the murmur3 hash.
       */
      memset(murmur_buf, 0x00, sizeof(fds_uint32_t) * MURMUR_SIZE);
      MurmurHash3_x86_128(get_obj.data.c_str(),
                          get_obj.data.size(),
                          0,
                          murmur_buf);
      murmur_hex = ToHex(murmur_buf, MURMUR_SIZE);

      if (written_objs[i] != murmur_hex) {
        std::cout << "Failed to get correct data for key "
                  << dl.offset << std::endl;
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
