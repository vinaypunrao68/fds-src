#include <iostream>
#include <string>
#include <thread>

#include <util/Log.h>
#include <fds_obj_cache.h>

using namespace fds;

typedef unordered_map<ObjectID, ObjectBuf *, ObjectHash> persistent_store_t; 

FdsObjectCache *ocache;
fds_mutex *pstore_lock;
persistent_store_t pstore;
fds_log *oct_log;

void simulate_read_io(fds_volid_t vol_id, ObjectID obj_id) {
  
  ObjectBuf *obj_pers_copy = NULL;
  ObjBufPtrType objBufPtr = NULL;

  pstore_lock->lock();
  if (pstore.count(obj_id) > 0) {
    obj_pers_copy = pstore[obj_id];
  }
  pstore_lock->unlock();
  if (obj_pers_copy)
    objBufPtr = ocache->object_retrieve(vol_id, obj_id);
  else {
    FDS_PLOG(oct_log) << "Read request for non-existent object " << obj_id;
    return;
  }

  FDS_PLOG(oct_log) << "Read request for object " << obj_id;

  if (objBufPtr) {
    assert(obj_pers_copy != NULL);
    if ((obj_pers_copy->size == objBufPtr->size) && 
	(obj_pers_copy->data == objBufPtr->data)) {
      FDS_PLOG(oct_log) << "Successfully read object " << obj_id
			<< " off cache.";
    } 
    else {
      FDS_PLOG(oct_log) << "Object " << obj_id << " read from cache "
			<< " does not match with object in store.";
      assert(0);
    }
    ocache->object_release(vol_id, obj_id, objBufPtr);
    return;
  }
  // 2. Cache miss. Get a new object buf allocated from cache for the sz.
  FDS_PLOG(oct_log) << "Cache miss for object " << obj_id;

  
  fds_uint32_t obj_size = obj_pers_copy->size;
  objBufPtr = ocache->object_alloc(vol_id, obj_id, obj_size);
  
  FDS_PLOG(oct_log) << "Successfully allocated object buf for object " << obj_id
		    << " off cache.";

  usleep(1000); // Simulate time to read from disk.

  // ... retrieve object from persistent store and store it in the buf
  memcpy((void *)objBufPtr->data.c_str(), (void *)obj_pers_copy->data.c_str(), obj_size);

  // Make it availabe in cache for future reads
  ocache->object_add(vol_id, obj_id, objBufPtr, false); // read data is always clean

  FDS_PLOG(oct_log) << "Successfully added object " << obj_id
		    << " to cache.";

  //release reference
  ocache->object_release(vol_id, obj_id, objBufPtr);  

}

void simulate_sync_write_io(fds_volid_t vol_id, ObjectID obj_id, std::string obj_data) {

  ObjectBuf *obj_pers_copy = NULL;
  ObjBufPtrType objBufPtr = NULL;

  pstore_lock->lock();

  if (pstore.count(obj_id) > 0) {
    obj_pers_copy = pstore[obj_id];
  }

  pstore_lock->unlock();

  if (obj_pers_copy) {
    if (obj_pers_copy->data != obj_data) {
      // Hash collision. Flag and return.
      FDS_PLOG(oct_log) << "Hash collision detected for objects with id "
			<< obj_id;
      return;
    }
  } 
  if (obj_pers_copy)
    objBufPtr = ocache->object_retrieve(vol_id, obj_id);
  if (objBufPtr) {
    //assert(obj_pers_copy != NULL);
    if ((obj_pers_copy->size == objBufPtr->size) && 
	(obj_pers_copy->data == objBufPtr->data)) {
      FDS_PLOG(oct_log) << "Successfully read object " << obj_id
			<< " off cache.";
    } 
    else {
      FDS_PLOG(oct_log) << "Object " << obj_id << " read from cache "
			<< " does not match with object in store.";
      assert(0);
    }
    ocache->object_release(vol_id, obj_id, objBufPtr);
    return;
  }

  objBufPtr = ocache->object_alloc(vol_id, obj_id, obj_data.size());
  FDS_PLOG(oct_log) << "Successfully allocated object buf for object " << obj_id
		    << " off cache."; 
  memcpy((void *)objBufPtr->data.c_str(), (void *)obj_data.c_str(), obj_data.size()); 

  if (!obj_pers_copy) {
    ObjectBuf *new_obj = new ObjectBuf();
    new_obj->data = obj_data;
    new_obj->size = obj_data.size();
    pstore_lock->lock();
    pstore[obj_id] = new_obj;
    pstore_lock->unlock();
    usleep(500); // Simulate time to write to disk
  }
  ocache->object_add(vol_id, obj_id, objBufPtr, false);
  FDS_PLOG(oct_log) << "Successfully added object " << obj_id
		    << " to cache.";

  ocache->object_release(vol_id, obj_id, objBufPtr);  

}

void simulate_async_write_io(fds_volid_t vol_id, ObjectID obj_id, std::string obj_data) {

  ObjectBuf *obj_pers_copy = NULL;
  ObjBufPtrType objBufPtr = NULL;

  pstore_lock->lock();
  if (pstore.count(obj_id) > 0) {
    obj_pers_copy = pstore[obj_id];
  }
  pstore_lock->unlock();

  if (obj_pers_copy) {
    if (obj_pers_copy->data != obj_data) {
      // Hash collision. Flag and return.
      FDS_PLOG(oct_log) << "Hash collision detected for objects with id "
			<< obj_id;
      return;
    }
  } 
  
  if (obj_pers_copy)
    objBufPtr = ocache->object_retrieve(vol_id, obj_id);

  if (objBufPtr) {
    // assert(obj_pers_copy != NULL);
    if ((obj_pers_copy->size == objBufPtr->size) && 
	(obj_pers_copy->data == objBufPtr->data)) {
      FDS_PLOG(oct_log) << "Successfully read object " << obj_id
			<< " off cache.";
    } 
    else {
      FDS_PLOG(oct_log) << "Object " << obj_id << " read from cache "
			<< " does not match with object in store.";
      assert(0);
    }
    ocache->object_release(vol_id, obj_id, objBufPtr);
    return;
  }

  objBufPtr = ocache->object_alloc(vol_id, obj_id, obj_data.size());
  FDS_PLOG(oct_log) << "Successfully allocated object buf for object " << obj_id
		    << " off cache."; 
  memcpy((void *)objBufPtr->data.c_str(), (void *)obj_data.c_str(), obj_data.size()); 

  if (obj_pers_copy) {
    ocache->object_add(vol_id, obj_id, objBufPtr, false);
    FDS_PLOG(oct_log) << "Successfully added object " << obj_id
		      << " to cache";
    ocache->object_release(vol_id, obj_id, objBufPtr);  
    return;
  }

  ocache->object_add(vol_id, obj_id, objBufPtr, true);
  FDS_PLOG(oct_log) << "Successfully added object " << obj_id
		    << " to cache with dirty flag set";

  ObjectBuf *new_obj = new ObjectBuf();
  new_obj->data = obj_data;
  new_obj->size = obj_data.size();
  pstore_lock->lock();
  pstore[obj_id] = new_obj;
  pstore_lock->unlock();

  usleep(500); // Simulate time to write to disk

  ocache->mark_object_clean(vol_id, obj_id, objBufPtr);

  FDS_PLOG(oct_log) << "Marked clean object " << obj_id;

  ocache->object_release(vol_id, obj_id, objBufPtr);  

}


void write_objects(int thread_id) {

  fds_volid_t my_vol_id = thread_id % 5;

  for (int i = 0; i < 1000; i++) {

    ObjectID obj_id(thread_id, i);
    std::string obj_data = std::string("This is object ") + obj_id.ToString();
    if (i%2 == 0) {
      simulate_sync_write_io(my_vol_id, obj_id, obj_data);
    } else {
      simulate_async_write_io(my_vol_id, obj_id, obj_data);
    }

  }

}

void read_objects(int thread_id) {

  std::default_random_engine rgen2(thread_id);
  std::uniform_int_distribution<fds_uint64_t> obj_id_distribution(0, 40);
  fds_volid_t my_vol_id = thread_id % 5;

  for (int i = 0; i < 4; i++) {

    fds_uint64_t working_set_start = i * 250 + (thread_id/5) * 50;

    for (int j = 0; j < 320; j++) {

      //      fds_uint64_t obj_id_low = working_set_start + obj_id_distribution(rgen2);
      fds_uint64_t obj_id_low = working_set_start + j%40; 
      ObjectID obj_id(thread_id, obj_id_low);
      simulate_read_io(my_vol_id, obj_id);

    }

  }
  
}

void log_stats() {

  while(1) {

    usleep(500000);
    ocache->log_stats_to_file("ocache_stats.dat");
    
  }

}

int main(int argc, char *argv[]) {

  oct_log = new fds_log("ocache_unit_test", "logs");
  FDS_PLOG(oct_log) << "Created logger" << endl;

  pstore_lock = new fds_mutex("OC Unit Test Mutex");

  ocache = new FdsObjectCache(500 * 32, 
			       slab_allocator_type_default, 
			       eviction_policy_type_default,
			       oct_log);

  for (int i = 0; i < 5; i++) {
    ocache->vol_cache_create(i, (40 + (i * 20))*32, (200 + (i * 50))*32); 
  }

  std::thread *stats_thread = new std::thread(log_stats);
  
  std::vector<std::thread *> write_threads;

  for (int i = 0; i < 30; i++) {
    std::thread *next_thread = new std::thread(write_objects, i);
    write_threads.push_back(next_thread);
  }

  for (int i = 0; i < 30; i++) {
    write_threads[i]->join();
  }

  std::vector<std::thread *> read_threads;

  for (int i = 0; i < 10; i++) {
    std::thread *next_thread = new std::thread(read_objects, i);
    read_threads.push_back(next_thread);
  }

  for (int i = 0; i < 10; i++) {
    read_threads[i]->join();
  }

  FDS_PLOG(oct_log) << "Test done.";


}
