/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <assert.h>
#include <string>

#include <odb.h>
#include "leveldb/db.h"
#include "leveldb/filter_policy.h"

namespace fds {
namespace osm {

#define WRITE_BUFFER_SIZE   50 * 1024 * 1024;
#define FILTER_BITS_PER_KEY 128  // Todo: Change this to the max size of DiskLoc

/** Constructs odb with filename.
 *
 * @param filename (i) Name of file for backing storage.
 *
 * @return ObjectDB object.
 */
ObjectDB::ObjectDB(const std::string& filename)
        : file(filename) {
    /*
     * Setup DB options
     */
    options.create_if_missing = 1;
    options.filter_policy     =
            leveldb::NewBloomFilterPolicy(FILTER_BITS_PER_KEY);
    options.write_buffer_size = WRITE_BUFFER_SIZE;

    write_options.sync = true;

    leveldb::Status status = leveldb::DB::Open(options, file, &db);

    /* Open has to succeed */
    if (!status.ok())
    {
        throw OsmException(std::string(__FILE__) + ":" + std::to_string(__LINE__) +
                           " :leveldb::DB::Open(): " + status.ToString());
    }

    histo_all.Clear();
    histo_put.Clear();
    histo_get.Clear();
}

/** Destructs odb.
 * @return None
 */
ObjectDB::~ObjectDB() {
    delete options.filter_policy;
    delete db;
}

/** Puts an object at a disk location.
 *
 * @param disk_location (i) Location to put obj.
 * @param obj_buf       (i) Object data.
 *
 * @return ERR_OK if successful, err otherwise.
 */
fds::Error ObjectDB::Put(const DiskLoc& disk_location,
                         const ObjectBuf& obj_buf) {
    fds::Error err(fds::ERR_OK);

    leveldb::Slice key((const char *)&disk_location, sizeof(disk_location));
    leveldb::Slice value(obj_buf.getData(), obj_buf.getSize());

    timer_start();
    leveldb::Status status = db->Put(write_options, key, value);
    timer_stop();
    timer_update_put_histo();
    if (!status.ok()) {
        err = fds::Error(fds::ERR_DISK_WRITE_FAILED);
    }

    return err;
}

/** Puts an object at a disk location.
 *
 * @param disk_location (i) Location to put obj.
 * @param obj_id       (i) Object Id.
 *
 * @return ERR_OK if successful, err otherwise.
 */
fds::Error ObjectDB::Put(const DiskLoc& disk_location,
                         const ObjectID& obj_id) {
    fds::Error err(fds::ERR_OK);

    leveldb::Slice key((const char *)&disk_location, sizeof(disk_location));
    leveldb::Slice value((char *)obj_id.GetId(), obj_id.getDigestLength());


    timer_start();
    leveldb::Status status = db->Put(write_options, key, value);
    timer_stop();
    timer_update_put_histo();
    if (!status.ok()) {
        err = fds::Error(fds::ERR_DISK_WRITE_FAILED);
    }

    return err;
}


/** Delete an object from a disk location.
 *
 * @param disk_location (i) Location to get obj.
 * @param obj_buf       (o) Object data.
 *
 * @return ERR_OK if successful, err otherwise.
 */
fds::Error ObjectDB::Delete(const ObjectID& object_id)
{
    fds::Error err(fds::ERR_OK);

    leveldb::Slice key((const char*)object_id.GetId(), object_id.getDigestLength());
    std::string value;

    timer_start();
    leveldb::Status status = db->Delete(write_options, key);
    timer_stop();
    timer_update_get_histo();
    if (status.IsNotFound() == true) {
        err = ERR_NOT_FOUND;
    }

    return err;
}


/** Puts an object at a disk location.
 *
 * @param obj_id (i)   object ID.
 * @param obj_buf       (i) Object data.
 *
 * @return ERR_OK if successful, err otherwise.
 */
fds::Error ObjectDB::Put(const ObjectID& object_id,
                         const ObjectBuf& obj_buf) {
    fds::Error err(fds::ERR_OK);

    leveldb::Slice key((const char *)object_id.GetId(), object_id.getDigestLength());
    leveldb::Slice value(obj_buf.getData(), obj_buf.getSize());

    timer_start();
    leveldb::Status status = db->Put(write_options, key, value);
    timer_stop();
    timer_update_put_histo();
    if (status.IsNotFound() == true) {
        err = ERR_NOT_FOUND;
    }

    return err;
}

/** Gets an object from a disk location.
 *
 * @param disk_location (i) Location to get obj.
 * @param obj_buf       (o) Object data.
 *
 * @return ERR_OK if successful, err otherwise.
 */
fds::Error ObjectDB::Get(const DiskLoc& disk_location,
                         ObjectBuf& obj_buf) {
    fds::Error err(fds::ERR_OK);

    leveldb::Slice key((const char *)&disk_location, sizeof(disk_location));
    std::string value = "";

    timer_start();
    leveldb::Status status = db->Get(read_options, key, &value);
    timer_stop();
    timer_update_get_histo();
    if (status.IsNotFound() == true) {
        return ERR_NOT_FOUND;
    } else if (!status.ok()) {
        // TODO(Andrew): Get a better error code than this. We're
        // not returning DISK_READ_FAILED because we don't want
        // legacy code to silently keep using the err code.
        return ERR_NO_BYTES_READ;
    }

    *(obj_buf.data) = value;

    return err;
}


/** Delete an object from a disk location.
 *
 * @param disk_location (i) Location to get obj.
 * @param obj_buf       (o) Object data.
 *
 * @return ERR_OK if successful, err otherwise.
 */
fds::Error ObjectDB::Delete(const DiskLoc& disk_location) {
    fds::Error err(fds::ERR_OK);

    leveldb::Slice key((const char *)&disk_location, sizeof(disk_location));
    std::string value;

    timer_start();
    leveldb::Status status = db->Delete(write_options, key);
    timer_stop();
    timer_update_get_histo();
    if (status.IsNotFound() == true) {
        err = ERR_NOT_FOUND;
    }

    return err;
}


/** Gets an object from the hash value of the object contents objectID
 *
 * @param disk_location (i) Location to get obj.
 * @param obj_buf       (o) Object data.
 *
 * @return ERR_OK if successful, err otherwise.
 */
fds::Error ObjectDB::Get(const ObjectID& obj_id,
                         ObjectBuf& obj_buf) {
    fds::Error err(fds::ERR_OK);

    leveldb::Slice key((const char *)obj_id.GetId(), obj_id.getDigestLength());
    std::string value;

    timer_start();
    leveldb::Status status = db->Get(read_options, key, &value);
    timer_stop();
    timer_update_get_histo();
    if (status.IsNotFound() == true) {
        return ERR_NOT_FOUND;
    } else if (!status.ok()) {
        // TODO(Andrew): Get a better error code than this. We're
        // not returning DISK_READ_FAILED because we don't want
        // legacy code to silently keep using the err code.
        return ERR_NO_BYTES_READ;
    }

    *(obj_buf.data) = value;

    return err;
}

}  // namespace osm
}  // namespace fds
