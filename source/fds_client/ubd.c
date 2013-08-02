#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/major.h>


//#include "tapdisk-image.h"
//#include "tapdisk-driver.h"
//#include "tapdisk-server.h"
//#include "tapdisk-interface.h"
//#include "tapdisk-disktype.h"
#include "tapdisk-vbd.h"
#include "blktap2.h"
#include "tapdisk-ring.h"
//#include "tap-ctl.h"

#define HVISOR_SECTOR_SIZE 512

#define DBG(_level, _f, _a...) tlog_write(_level, _f, ##_a)
#define ERR(_err, _f, _a...) tlog_error(_err, _f, ##_a)

#if 1                                                                        
#define ASSERT(p)							\
	do {								\
		if (!(p)) {						\
			DPRINTF("Assertion '%s' failed, line %d, "	\
				"file %s", #p, __LINE__, __FILE__);	\
			*(int*)0 = 0;					\
		}							\
	} while (0)
#else
#define ASSERT(p) ((void)0)
#endif 

static int hvisor_create_io_ring(td_vbd_t *vbd, const char *devname);

td_vbd_t*hvisor_vbd_create(uint16_t uuid);

static void
__hvisor_run(td_vbd_t *vbd);

char data_image[16536];

static inline void
hvisor_vbd_initialize_vreq(td_vbd_request_t *vreq)
{
	memset(vreq, 0, sizeof(td_vbd_request_t));
	INIT_LIST_HEAD(&vreq->next);
}

static int
tap_ctl_prepare_directory(const char *dir)
{
	int err;
	char *ptr, *name, *start;

	err = access(dir, W_OK | R_OK);
	if (!err)
		return 0;

	name = strdup(dir);
	if (!name)
		return ENOMEM;

	start = name;

	for (;;) {
		ptr = strchr(start + 1, '/');
		if (ptr)
			*ptr = '\0';

		err = mkdir(name, 0755);
		if (err && errno != EEXIST) {
			PERROR("mkdir %s", name);
			err = errno;
			break;
		}

		if (!ptr)
			break;
		else {
			*ptr = '/';
			start = ptr + 1;
		}
	}

	free(name);
	return err;
}

static int
tap_ctl_make_device(const char *devname, const int major,
		    const int minor, const int perm)
{
	int err;
	char *copy, *dir;

	copy = strdup(devname);
	if (!copy)
		return ENOMEM;

	dir = dirname(copy);

	err = tap_ctl_prepare_directory(dir);
	free(copy);

	if (err)
		return err;

	if (!access(devname, F_OK))
		if (unlink(devname)) {
			PERROR("unlink %s", devname);
			return errno;
		}

	err = mknod(devname, perm, makedev(major, minor));
	if (err) {
		PERROR("mknod %s", devname);
		return errno;
	}

	return 0;
}

int
tap_ctl_free(const int minor)
{
        int fd, err;

        fd = open(BLKTAP2_CONTROL_DEVICE, O_RDONLY);
        if (fd == -1) {
                EPRINTF("failed to open control device: %d\n", errno);
                return errno;
        }

        err = ioctl(fd, BLKTAP2_IOCTL_FREE_TAP, minor);
        close(fd);

        return err;
}


static int
tap_ctl_allocate_device(int *minor, char **devname)
{
	char *name;
	int fd, err;
	struct blktap2_handle handle;

	*minor = -1;
	if (!devname)
		return EINVAL;

	//fd = open(BLKTAP2_CONTROL_DEVICE, O_RDONLY);
	fd = open("/dev/blktap-control", O_RDONLY);
	if (fd == -1) {
	  printf("Failed to open control device %s: %d\n", BLKTAP2_CONTROL_DEVICE, errno);
		return errno;
	}

	err = ioctl(fd, BLKTAP2_IOCTL_ALLOC_TAP, &handle);
	close(fd);
	if (err == -1) {
		printf("Failed to allocate new device: %d\n", errno);
		return errno;
	}

	err = asprintf(&name, "%s%d", BLKTAP2_RING_DEVICE, handle.minor);
	if (err == -1) {
		err = ENOMEM;
		goto fail;
	}

	err = tap_ctl_make_device(name, handle.ring,
				  handle.minor, S_IFCHR | 0600);
	if (err) {
		printf("Creating ring device for %d failed: %d\n",
			handle.minor, err);
		goto fail;
	} else {
	  printf("Created ring device at %s\n", name);
	}
	free(name);

	if (*devname)
		name = *devname;
	else {
		err = asprintf(&name, "%s%d",
			       BLKTAP2_IO_DEVICE, handle.minor);
		if (err == -1) {
			err = ENOMEM;
			goto fail;
		}
		*devname = name;
	}

	err = tap_ctl_make_device(name, handle.device,
				  handle.minor, S_IFBLK | 0600);
	if (err) {
		EPRINTF("Creating IO device for %d failed: %d\n",
			handle.minor, err);
		goto fail;
	} else {
	  printf("Created IO device at %s\n", name);
	}

	printf("New interface: ring: %u, device: %u, minor: %u\n",
	    handle.ring, handle.device, handle.minor);

	*minor = handle.minor;
	return 0;

fail:
	tap_ctl_free(handle.minor);
	return err;
}


int main(int argc, char *argv[]) {

  td_vbd_t *vbd;
  int err;
  char *ring_devname = 0;
  char *io_devname = 0;
  int minor = 0;

  memset(data_image, 'x', sizeof(data_image));

  vbd = hvisor_vbd_create(0);

  err = tap_ctl_allocate_device(&minor, &io_devname);
  if (err) {
    printf("Failed to create ring and io devices\n");
    return (0);
 }

  err = asprintf(&ring_devname, BLKTAP2_RING_DEVICE"%d", minor);
  if (err == -1) {
    err = -ENOMEM;
    printf("Failed constructing ring device name\n");
    return (0);
 }

  err = hvisor_create_io_ring(vbd, ring_devname);
  if (err) {
    printf("Failed to create ring\n");
    return (0);
  }
  printf("All done. About to enter wait loop\n");
  __hvisor_run(vbd);

  return (0);

}

td_vbd_t*
hvisor_vbd_create(uint16_t uuid)
{
	td_vbd_t *vbd;
	int i;

	vbd = calloc(1, sizeof(td_vbd_t));
	if (!vbd) {
		printf("Failed to allocate tapdisk state\n");
		return NULL;
	}

	vbd->uuid     = uuid;
	vbd->minor    = -1;
	vbd->ring.fd  = -1;

	/* default blktap ring completion */
	//vbd->callback = tapdisk_vbd_callback;
	//vbd->argument = vbd;
    
	INIT_LIST_HEAD(&vbd->driver_stack);
	INIT_LIST_HEAD(&vbd->images);
	INIT_LIST_HEAD(&vbd->new_requests);
	INIT_LIST_HEAD(&vbd->pending_requests);
	INIT_LIST_HEAD(&vbd->failed_requests);
	INIT_LIST_HEAD(&vbd->completed_requests);
	INIT_LIST_HEAD(&vbd->next);
	gettimeofday(&vbd->ts, NULL);

	for (i = 0; i < MAX_REQUESTS; i++)
		hvisor_vbd_initialize_vreq(vbd->request_list + i);

	return vbd;
}

static int
hvisor_create_io_ring(td_vbd_t *vbd, const char *devname)
{
	
	int err, psize;
	td_ring_t *ring;

	ring  = &vbd->ring;
	psize = getpagesize();

	ring->fd = open(devname, O_RDWR);
	if (ring->fd == -1) {
		err = -errno;
		printf("Failed to open %s: %d\n", devname, err);
		goto fail;
	}

	ring->mem = mmap(0, psize * BLKTAP_MMAP_REGION_SIZE,
			 PROT_READ | PROT_WRITE, MAP_SHARED, ring->fd, 0);
	if (ring->mem == MAP_FAILED) {
		err = -errno;
		printf("Failed to mmap %s: %d\n", devname, err);
		goto fail;
	} else {
		printf("MMapped ring region successfully\n");
	}

	ring->sring = (blkif_sring_t *)((unsigned long)ring->mem);
	BACK_RING_INIT(&ring->fe_ring, ring->sring, psize);
	printf("Initialized ring\n");

	ring->vstart =
		(unsigned long)ring->mem + (BLKTAP_RING_PAGES * psize);

	ioctl(ring->fd, BLKTAP_IOCTL_SETMODE, BLKTAP_MODE_INTERPOSE);
	printf("Invoked set mode ioctl successfully on ring fd\n");

	return 0;

fail:
	if (ring->mem && ring->mem != MAP_FAILED)
		munmap(ring->mem, psize * BLKTAP_MMAP_REGION_SIZE);
	if (ring->fd != -1)
		close(ring->fd);
	ring->fd  = -1;
	ring->mem = NULL;
	return err;
}

static int hvisor_wait_for_events(td_vbd_t *vbd) {

  int ret;
  struct timeval tv;
  fd_set read_fds;

  tv.tv_sec  = 2;
  tv.tv_usec = 0;

  FD_ZERO(&read_fds);
  FD_SET(vbd->ring.fd, &read_fds);

  ret = select(vbd->ring.fd + 1, &read_fds,
	       0, 0, &tv);
  
  if (ret < 0)
    return ret;

  return (0);

}

static inline void
hvisor_vbd_write_response_to_ring(td_vbd_t *vbd, blkif_response_t *rsp)
{
	td_ring_t *ring;
	blkif_response_t *rspp;

	ring = &vbd->ring;
	rspp = RING_GET_RESPONSE(&ring->fe_ring, ring->fe_ring.rsp_prod_pvt);
	memcpy(rspp, rsp, sizeof(blkif_response_t));
	printf("Writing response for request %d at position %d\n", (int) rsp->id, ring->fe_ring.rsp_prod_pvt);
	ring->fe_ring.rsp_prod_pvt++;
}

static void
hvisor_vbd_make_response(td_vbd_t *vbd, td_vbd_request_t *vreq)
{
	blkif_request_t tmp;
	blkif_response_t *rsp;

	tmp = vreq->req;
	rsp = (blkif_response_t *)&vreq->req;

	rsp->id = tmp.id;
	rsp->operation = tmp.operation;
	rsp->status = vreq->status;

	printf("writing req %d, sec 0x%08"PRIx64", res %d to ring\n",
	    (int)tmp.id, tmp.sector_number, vreq->status);

	if (rsp->status != BLKIF_RSP_OKAY)
	  printf("returning BLKIF_RSP %d", rsp->status);

	vbd->returned++;
	hvisor_vbd_write_response_to_ring(vbd, rsp);
}


void
hvisor_complete_vbd_request(td_vbd_t *vbd, td_vbd_request_t *vreq) {
  hvisor_vbd_make_response(vbd, vreq);
  list_del(&vreq->next);
  hvisor_vbd_initialize_vreq(vreq);
}

void hvisor_complete_request(td_vbd_t *vbd, td_vbd_request_t *vreq) {
	if (!vreq->submitting && !vreq->secs_pending) {
		hvisor_complete_vbd_request(vbd, vreq);
	}	
}


void hvisor_queue_read(td_vbd_t *vbd, td_vbd_request_t *vreq, td_request_t treq)
{
	int      size    = treq.secs * HVISOR_SECTOR_SIZE;
	uint64_t offset  = treq.sec * (uint64_t)HVISOR_SECTOR_SIZE;
	char test_c;
	int i;

	printf("Received read request at offset %llx for %d bytes, buf - %p :\n", offset, size, treq.buf);
	
	if (offset + size < sizeof(data_image)) {
	  memcpy(treq.buf, data_image + offset, size);
	}
	for (i = 0; i < size; i++) {
	  printf("%2x", treq.buf[i]);
	}
	printf("\n");

	vbd->secs_pending  -= treq.secs;
	vreq->secs_pending -= treq.secs;
	vreq->blocked = treq.blocked;

	hvisor_complete_request(vbd, vreq);
}

void hvisor_queue_write(td_vbd_t *vbd, td_vbd_request_t *vreq, td_request_t treq)
{
	int      size    = treq.secs * HVISOR_SECTOR_SIZE;
	uint64_t offset  = treq.sec * (uint64_t)HVISOR_SECTOR_SIZE;
	int i;
	
	printf("Received write request at offset %llx for %d bytes, buf - %p : \n", offset, size, treq.buf);
	for (i = 0; i < size; i++) {
	  printf("%2x", treq.buf[i]);
	}
	printf("\n");
	
	if (offset + size < sizeof(data_image)) {
	  memcpy(data_image + offset, treq.buf, size);
	}

	vbd->secs_pending  -= treq.secs;
	vreq->secs_pending -= treq.secs;
	vreq->blocked = treq.blocked;

	hvisor_complete_request(vbd, vreq);
}


static int
hvisor_handle_request(td_vbd_t *vbd, td_vbd_request_t *vreq)
{
	char *page;
	td_ring_t *ring;
	td_image_t *image;
	td_request_t treq;
	uint64_t sector_nr;
	blkif_request_t *req;
	int i, err, id, nsects;

	req       = &vreq->req;
	id        = req->id;
	ring      = &vbd->ring;
	sector_nr = req->sector_number;

	vreq->submitting = 1;
	gettimeofday(&vbd->ts, NULL);
	gettimeofday(&vreq->last_try, NULL);


	for (i = 0; i < req->nr_segments; i++) {
		nsects = req->seg[i].last_sect - req->seg[i].first_sect + 1;
		page   = (char *)MMAP_VADDR(ring->vstart, 
					   (unsigned long)req->id, i);
		page  += (req->seg[i].first_sect << SECTOR_SHIFT);

		treq.id             = id;
		treq.sidx           = i;
		treq.blocked        = 0;
		treq.buf            = page;
		treq.sec            = sector_nr;
		treq.secs           = nsects;
		treq.image          = 0;
		treq.cb             = 0;
		treq.cb_data        = NULL;
		treq.private        = vreq;

		printf("%s: req %d seg %d sec 0x%08"PRIx64" secs 0x%04x "
		    "buf %p op %d\n", "Hvisor", id, i, treq.sec, treq.secs,
		    treq.buf, (int)req->operation);

		vreq->secs_pending += nsects;
		vbd->secs_pending  += nsects;

		switch (req->operation)	{
		case BLKIF_OP_WRITE:
			treq.op = TD_OP_WRITE;
			hvisor_queue_write(vbd, vreq, treq);
			break;

		case BLKIF_OP_READ:
			treq.op = TD_OP_READ;
			hvisor_queue_read(vbd, vreq, treq);
			break;
		}

		sector_nr += nsects;
	}

	err = 0;

out:
	vreq->submitting--;
	if (!vreq->secs_pending) {
		err = (err ? : vreq->error);
		hvisor_complete_vbd_request(vbd, vreq);
	}

	return err;

fail:
	vreq->status = BLKIF_RSP_ERROR;
	goto out;
}

static void
hvisor_pull_ring_requests(td_vbd_t *vbd)
{
	int idx;
	RING_IDX rp, rc;
	td_ring_t *ring;
	blkif_request_t *req;
	td_vbd_request_t *vreq;

	ring = &vbd->ring;
	if (!ring->sring)
		return;

	rp   = ring->fe_ring.sring->req_prod;
	xen_rmb();

	for (rc = ring->fe_ring.req_cons; rc != rp; rc++) {
		req = RING_GET_REQUEST(&ring->fe_ring, rc);
		++ring->fe_ring.req_cons;
		printf("Pulled request %d at position %d\n", (int) req->id, rc);

		idx  = req->id;
		vreq = &vbd->request_list[idx];

		ASSERT(list_empty(&vreq->next));
		ASSERT(vreq->secs_pending == 0);

		memcpy(&vreq->req, req, sizeof(blkif_request_t));
		vbd->received++;
		vreq->vbd = vbd;

		hvisor_handle_request(vbd, vreq);

		printf("%s: request %d \n", "Hvisor" , idx); 
	}
}

int
hvisor_vbd_kick(td_vbd_t *vbd)
{
	int n;
	td_ring_t *ring;

	ring = &vbd->ring;
	if (!ring->sring)
		return 0;

	n    = (ring->fe_ring.rsp_prod_pvt - ring->fe_ring.sring->rsp_prod);
	if (!n)
		return 0;

	vbd->kicked += n;
	printf("Pushing %d responses, rsp prod idx moving  from position %d to position %d\n", n, ring->fe_ring.sring->rsp_prod, ring->fe_ring.rsp_prod_pvt);
	RING_PUSH_RESPONSES(&ring->fe_ring);
	ioctl(ring->fd, BLKTAP_IOCTL_KICK_FE, 0);

	printf("kicking %d: rec: 0x%08"PRIx64", ret: 0x%08"PRIx64", kicked: "
	    "0x%08"PRIx64"\n", n, vbd->received, vbd->returned, vbd->kicked);

	return n;
}

static void
__hvisor_run(td_vbd_t *vbd)
{
  while (1) {
    int ret;

    ret = hvisor_wait_for_events(vbd);

    if (ret < 0)
      printf("server wait returned %d\n", ret);

    hvisor_pull_ring_requests(vbd);

    hvisor_vbd_kick(vbd);

  }
}
