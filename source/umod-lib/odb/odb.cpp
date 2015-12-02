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

int doCopyFile(void * arg, const char* fname, fds_uint64_t length) {
    fds_assert(fname && *fname != 0);

    CopyDetails * details = reinterpret_cast<CopyDetails *>(arg);
    GLOGNORMAL << "Copying file '" << fname << "' to directory '" << details->destPath
            << "' from '" << details->srcPath;

    std::string srcFile = details->srcPath + "/" + fname;
    std::string destFile = details->destPath + "/" + fname;

    std::ifstream infile(srcFile.c_str(), std::fstream::binary);
    std::ofstream outfile(destFile.c_str(), std::fstream::binary);
    if (static_cast<fds_uint64_t>(-1) == length) {
        outfile << infile.rdbuf();
    } else if (length) {
        char * buffer = new char[length];
        infile.read(buffer, length);
        outfile.write(buffer, length);
        delete[] buffer;
    }
    outfile.close();
    infile.close();

    return 0;
}

/** Constructs odb with filename.
 *
 * @param filename (i) Name of file for backing storage.
 *
 * @return ObjectDB object.
 */
ObjectDB::ObjectDB(const std::string& filename,
                   fds_bool_t sync_write)
        : file(filename) {
    /*
     * Setup DB options
     */
    options.create_if_missing = 1;
    options.filter_policy     =
            leveldb::NewBloomFilterPolicy(FILTER_BITS_PER_KEY);
    options.write_buffer_size = WRITE_BUFFER_SIZE;

    write_options.sync = sync_write;

    env.reset(new leveldb::CopyEnv(*leveldb::Env::Default()));
    options.env = env.get();

    leveldb::Status status;
    LOGDEBUG << "opening leveldb";
    {
        leveldb::DB* ptr;
        status = leveldb::DB::Open(options, file, &ptr);
        db.reset(ptr);
    }

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
    db.reset();
}

/**
 * All operations on this levelDB will fail after calling
 * this method
 */
void ObjectDB::closeAndDestroy() {
    delete options.filter_policy;
    options.filter_policy = nullptr;
    db = nullptr;
    leveldb::DestroyDB(file, leveldb::Options());
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
    if (!db) {
        return fds::ERR_NOT_READY;
    }

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
    if (!db) {
        return fds::ERR_NOT_READY;
    }

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
    if (!db) {
        return fds::ERR_NOT_READY;
    }

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
    if (!db) {
        return fds::ERR_NOT_READY;
    }

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
    if (!db) {
        return fds::ERR_NOT_READY;
    }

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
    if (!db) {
        return fds::ERR_NOT_READY;
    }

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
    if (!db) {
        return fds::ERR_NOT_READY;
    }

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

std::vector<ObjectID> ObjectDB::GetKeys() const {
    fds::Error err(ERR_OK);
    std::vector<ObjectID> allKeys;
    leveldb::Iterator* it = db->NewIterator(read_options);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        allKeys.push_back(ObjectID(it->key().ToString()));
    }
    return allKeys;
}

/** Takes a persistent snapshot of the leveldb in ObjectDB
 *
 * @param fileName (i) Directory where the snapshot of leveldb is stored.
 *
 * @return ERR_OK if successful, err otherwise.
 */
fds::Error ObjectDB::PersistentSnap(const std::string& fileName,
                                    leveldb::CopyEnv **cenv) {

    fds_assert(!fileName.empty());
    fds::Error err(ERR_OK);
    leveldb::CopyEnv *env;

    if (!db) {
        return fds::ERR_NOT_READY;
    }

    env = static_cast<leveldb::CopyEnv*>(options.env);
    fds_assert(env);
    if (!env) {
        GLOGCRITICAL << "Take persistent snapshot failed " << fileName;
        err = ERR_INVALID_ARG;
        return err;
    }
    env->DeleteDir(fileName);
    leveldb::Status status = env->CreateDir(fileName);
    if (!status.ok()) {
        GLOGNORMAL << " CreateDir failed for " << fileName << "status " << status.ToString() ;
        err = ERR_DISK_WRITE_FAILED;
        return err;
    }

    CopyDetails *details = new (std::nothrow)CopyDetails(file, fileName);
    if (details) {
        status = env->Copy(file, &doCopyFile, reinterpret_cast<void *>(details));
        if (!status.ok()) {
            GLOGNORMAL << " Copy failed for " << file << " destination file " << fileName;
            err = ERR_DISK_WRITE_FAILED;
            delete details;
            return err;
        }
        delete details;
    } else {
        err = ERR_OUT_OF_MEMORY;
    }

    *cenv = env;
    return err;
}

}  // namespace osm
}  // namespace fds
