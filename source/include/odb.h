/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Object database class. The object database is a key-value store
 * that provides local objec storage.
 */
#ifndef SOURCE_STOR_MGR_ODB_H_
#define SOURCE_STOR_MGR_ODB_H_

#include <iostream>  // NOLINT(*)
#include <string>

#include <fds_types.h>
#include <fds_error.h>
#include <leveldb/db.h>
#include <leveldb/env.h>
#include <util/histogram.h>
#include <concurrency/Mutex.h>
#include <concurrency/RwLock.h>

namespace fds {
namespace osm {

struct OsmException : std::exception {
    std::string msg;

    OsmException(const std::string& msg) : msg(msg) {
    }

    const char* what() const noexcept{
        return msg.c_str();
    }
};

  class ObjectDB {
 public:
    /*
     * Constructors
     */
    explicit ObjectDB(const std::string& filename);

    /*
     * Destructors
     */
    ~ObjectDB();

    fds::Error Put(const DiskLoc& disk_location,
                   const ObjectBuf& object_buf);

    fds::Error Put(const DiskLoc& disk_location,
                   const ObjectID& object_id);

    fds::Error Get(const DiskLoc& disk_location,
                   ObjectBuf& obj_buf);

    fds::Error Delete(const DiskLoc& disk_location);

    fds::Error Put(const ObjectID& obj_id,
                   const ObjectBuf& object_buf);

    fds::Error Get(const ObjectID& obj_id,
                   ObjectBuf& obj_buf);
    fds::Error Delete(const ObjectID& obj_id);
    void PrintHistoAll() {
      std::cout << "Microseconds per op:" << std::endl
                << histo_all.ToString() << std::endl;
    }
    void PrintHistoPut() {
      std::cout << "Microseconds per put op:" << std::endl
                << histo_put.ToString() << std::endl;
    }
    void PrintHistoGet() {
      std::cout << "Microseconds per get op:" << std::endl
                << histo_get.ToString() << std::endl;
    }
    leveldb::DB *GetDB() { 
       return db;
    }
    leveldb::ReadOptions GetReadOptions() { 
        return read_options;
    }
    leveldb::Options GetOptions() { 
       return options;
    }

    void  rdLock()  {
       rwlock.read_lock();
    }

    void  rdUnlock()  {
       rwlock.read_lock();
    }
   
    void wrLock() { 
       rwlock.write_lock();
    }

    void wrUnlock() { 
       rwlock.write_unlock();
    }

 private:
    std::string file;

    /*
     * Database structure where we're currently
     * storing objects. This will likey move to
     * a different structure in the future.
     */
    leveldb::DB* db;

    /*
     * Database options. These are not expected
     * to change.
     */
    leveldb::Options      options;
    leveldb::WriteOptions write_options;
    leveldb::ReadOptions  read_options;

    fds_rwlock   rwlock;

    /*
     * Statistics recording
     */
    leveldb::Histogram histo_all;
    leveldb::Histogram histo_put;
    leveldb::Histogram histo_get;
    double start;
    double finish;
    double micros;

    /*
     * Needed: tier info, local filename
     */

    /** Starts internal timer.
     */
    void timer_start() {
      start  = leveldb::Env::Default()->NowMicros();
      finish = start;
      micros = 0;
    }

    /** Stops internal timer.
     */
    void timer_stop() {
      finish = leveldb::Env::Default()->NowMicros();
      micros = finish - start;
      histo_all.Add(micros);
    }

    /** Returns timer value in micro seconds.
     * @return microsecond timer value
     */
    double get_timer() {
      return micros;
    }

    /** Returns timer value in seconds.
     * @return seconds timer value
     */
    double get_timer_sec() {
      return (get_timer() * 1e-6);
    }

    /** Meant to be called immediately following
     * a timer_stop() for a get() operation.
     */
    void timer_update_get_histo() {
      histo_get.Add(get_timer());
    }

    /** Meant to be called immediately following
     * a timer_stop() for a put() operation.
     *
     */
    void timer_update_put_histo() {
      histo_put.Add(get_timer());
    }
  };
}  // namespace osm
}  // namespace fds

#endif  // SOURCE_STOR_MGR_ODB_H_
