/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "util/concurrency/ThreadPool.h"

namespace fds {

/**
 * Worker constructor
 *
 * @param owner (i) Reference to owning threadpool
 *
 * Creating a new worker creates a thread with
 * no initial job to do;
 */
Worker::Worker(fds_threadpool* owner)
    : _owner(owner),
      _is_done(true),
      _thread(boost::bind(&Worker::loop, this)),
      _list_mutex("worker set mutex") {
}

/**
 * Worker destructor
 *
 * Acts as a join waiting for the
 * current operation is complete.
 */
Worker::~Worker() {
  _list_mutex.lock();
  if (!_task.empty()) {
    _task.push_back(Task());
    _thread.join();
  }
  _list_mutex.unlock();
}

/**
 * Sets task for worker to do.
 *
 * @param func (i) Nullary task function pointer 
 *
 * Makes sure there is not a task already running
 * and adds this task.
 */
void Worker::set_task(const Task& func) {
  assert(!func.empty());
  assert(_is_done);
  _is_done = false;

  _list_mutex.lock();
  _task.push_back(func);
  _list_mutex.unlock();
}

/*
 * The worker's work loop. A task is pulled
 * from the _last list and performed.
 */
void Worker::loop() {
  while (true) {
    /*
     * No job.
     */
    _list_mutex.lock();
    if (_task.empty()) {
      _list_mutex.unlock();
      break;
    }

    Task task = _task.front();
    _task.pop_front();
    _list_mutex.unlock();

    /*
     * Run the task and try to handle and
     * exceptions thrown.
     */
    try {
      task();
    }
    catch(const std::exception& e) {
      std::cout << "Unhandled std::exception in worker "
                << e.what() << std::endl;
    }
    catch(...) {
      std::cout << "Unhandled non-exception in worker"
                << std::endl;
    }

    /*
     * When the job is done, notify the owning
     * thread pool so it can assign new work.
     */
    _is_done = true;
    _owner->task_done(this);
  }
}

fds_threadpool::fds_threadpool(fds_uint32_t num_threads)
    : _num_threads(num_threads),
      _tasks_remaining(0),
      _mutex("thread pool mutex") {

  /*
   * Start the empty worker threads.
   */
  _mutex.lock();
  while (num_threads-- > 0) {
    Worker* worker = new Worker(this);
    _free_workers.push_front(worker);
  }
  _mutex.unlock();
}

/*
 * Thread pool destructor. This waits
 * for any existing tasks to be completed.
 * Though new jobs can still be scheduled
 * this should be avoided as it will block
 * the destruction of the pool indefinitely.
 */
fds_threadpool::~fds_threadpool() {
  /*
   * Wait for the outstanding tasks to
   * complete.
   */
  join();

  /*
   * Make sure there are no more tasks remaining.
   */
  _mutex.lock();
  assert(_tasks.empty());
  assert(_free_workers.size() == _num_threads);

  /*
   * Destroy the worker threads.
   */
  while (!_free_workers.empty()) {
    Worker *worker = _free_workers.front();
    _free_workers.pop_front();
    delete worker;
  }
  _mutex.unlock();
}

/*
 * Waits for all tasks to complete. Can
 * be thought of as a joinall(). However,
 * new jobs can still be scheduled while
 * join() is wating and it will then wait
 * for these new jobs as well.
 */
void fds_threadpool::join() {
  fds_scoped_lock _slock(_mutex);
  while (_tasks_remaining > 0) {
    /*
     * Wait until a job finishes
     * then check again.
     */
    _condition.wait(_slock.boost());
  }
}

/*
 * Schedules a task to be run.
 *
 * @param task (i) Nullary pointer to task function
 * 
 * The base way to schedule jobs in the thread pool.
 * Jobs that exceed the current size of the pool are
 * queued for work.
 */
void fds_threadpool::schedule(Task task) {
  fds_scoped_lock _slock(_mutex);

  _tasks_remaining++;

  /*
   * If there's a free worker assign the
   * task. Otherwise, queue it.
   */
  if (!_free_workers.empty()) {
    (_free_workers.front())->set_task(task);
    _free_workers.pop_front();
  } else {
    _tasks.push_back(task);
  }
}

/*
 * Notify the threadpool a task has completed.
 *
 * @param worker (i) The worker whose task completed
 * 
 * Should only be called from a worker thread
 * upon task completion.
 */
void fds_threadpool::task_done(Worker* worker) {
  fds_scoped_lock _slock(_mutex);

  /*
   * Assign another task to this worker if
   * there's more. Otherwise, put the worker
   * on the free_workers queue.
   */
  if (!_tasks.empty()) {
    worker->set_task(_tasks.front());
    _tasks.pop_front();
  } else {
    _free_workers.push_front(worker);
  }

  /*
   * Wake up any thread waiting for join()
   */
  _tasks_remaining--;
  if (_tasks_remaining == 0) {
    _condition.notify_all();
  }
}

}  // namespace fds
