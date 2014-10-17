/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds-aio.h>

namespace fds {

FdsAIO::~FdsAIO() {}
FdsAIO::FdsAIO(int iovcnt, int fd) : fdsio::Request(false), io_cnt(iovcnt), io_fd(fd)
{
    memset(io_vec, 0, sizeof(io_vec));
    io_retry   = 0;
    io_cur_idx = 0;
    io_cur_len = 0;
    io_cur_ptr = NULL;
}

/**
 * aio_read
 * --------
 */
void
FdsAIO::aio_read()
{
    aio_do_iov(true);
}

/**
 * aio_write
 * ---------
 */
void
FdsAIO::aio_write()
{
    aio_do_iov(false);
}

/**
 * aio_on_error
 * ------------
 */
void
FdsAIO::aio_on_error()
{
}

/**
 * aio_do_iov
 * ----------
 */
void
FdsAIO::aio_do_iov(bool rd)
{
    ssize_t len;

    while (io_cur_idx < io_cnt) {
        if (io_cur_ptr == NULL) {
            aio_reset_iov(io_cur_idx);
            fds_assert(io_cur_len > 0);
            fds_assert(io_cur_ptr != NULL);
        }
        if (rd == true) {
            len = read(io_fd, io_cur_ptr, io_cur_len);
        } else {
            len = write(io_fd, io_cur_ptr, io_cur_len);
        }
        if (len > 0) {
            io_cur_ptr += len;
            io_cur_len -= len;
            if (io_cur_len == 0) {
                /* We finish with the current vector. */
                char   *base = reinterpret_cast<char *>(io_vec[io_cur_idx].iov_base);
                size_t  ldif = static_cast<size_t>(io_cur_ptr - base);
                fds_assert(ldif == io_vec[io_cur_idx].iov_len);

                io_cur_ptr = NULL;
                io_cur_len = 0;
                io_cur_idx++;
            }
            continue;
        }
        if (errno != EAGAIN) {
            aio_on_error();
        } else {
            if (rd == true) {
                aio_rearm_read();
            } else {
                aio_rearm_write();
            }
        }
        return;
    }
    fds_verify(io_cur_idx == io_cnt);
    fds_verify(io_cur_ptr == NULL);
    if (rd == true) {
        aio_read_complete();
    } else {
        aio_write_complete();
    }
}

}  // namespace fds
