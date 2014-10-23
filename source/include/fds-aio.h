/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_AIO_H_
#define SOURCE_INCLUDE_FDS_AIO_H_

#include <sys/uio.h>
#include <boost/thread/condition.hpp>

#include <cpplist.h>
#include <fds_assert.h>
#include <concurrency/Mutex.h>

namespace fds {

class FdsAIO
{
  public:
    static const int iov_max = 4;

    virtual ~FdsAIO();
    FdsAIO(int iovcnt, int fd);

    inline int aio_cur_iov() {
        return io_cur_idx;
    }
    inline void aio_set_fd(int fd) {
        io_fd = fd;
    }
    inline void aio_reset() {
        io_cnt     = 0;
        io_cur_idx = 0;
    }
    /**
     * Setup a vector buffer at the index location.
     */
    inline void aio_set_iov(int idx, char *base, size_t len)
    {
        fds_assert((idx < FdsAIO::iov_max) && ((io_cnt + 1) < FdsAIO::iov_max));
        io_vec[idx].iov_len  = len;
        io_vec[idx].iov_base = base;
        io_cnt++;
    }
    inline void aio_reset_iov(char *base, size_t len)
    {
        io_vec[0].iov_len  = len;
        io_vec[0].iov_base = base;
        io_cnt     = 1;
        io_cur_idx = 0;
    }
    /**
     * Assign the current tracking ptr and len from iov of the given index.
     */
    inline void aio_assign_iov(int idx)
    {
        fds_assert(idx < io_cnt);
        io_cur_idx = idx;
        io_cur_len = io_vec[idx].iov_len;
        io_cur_ptr = reinterpret_cast<char *>(io_vec[idx].iov_base);
    }
    virtual void aio_read();
    virtual void aio_write();
    virtual void aio_on_error();

    virtual void aio_rearm_read() = 0;
    virtual void aio_read_complete() = 0;

    virtual void aio_rearm_write() = 0;
    virtual void aio_write_complete() = 0;

    /**
     * Used in sync path if we have to wait.
     */
    void aio_wait(fds_mutex *mtx, boost::condition *waitq);
    void aio_wakeup(fds_mutex *mtx, boost::condition *waitq);

  protected:
    ChainLink               io_link;
    int                     io_fd;
    int                     io_cnt;
    struct iovec            io_vec[FdsAIO::iov_max];

    /**
     * Common function to do read/write to iovec.  Pass true if this is for read.
     */
    virtual void aio_do_iov(bool rd);

  private:
    bool                    io_wait;
    int                     io_retry;
    int                     io_cur_idx;
    ssize_t                 io_cur_len;
    char                   *io_cur_ptr;
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_AIO_H_
