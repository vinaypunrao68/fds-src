#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/major.h>

#include "tapdisk-vbd.h"
#include "blktap2.h"
#include "tapdisk-ring.h"

#include "ubd.h"
#include "hvisor_lib.h"

#define HVISOR_SECTOR_SIZE 512
#define HVISOR_MAX_RESPONSES_TO_HOLD 8
#define HVISOR_MAX_REQ_RSP_HOLD_TIME 1000 //usecs
#define HVISOR_MAX_PENDING_REQ_SEGS 5000
#define HVISOR_MAX_VBDS 1024

int reqs_in_hold = 0;


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

void ctrlcHandler(int signal);
static int hvisor_create_io_ring(td_vbd_t *vbd, const char *devname);
int  hvisor_create_blkdev(uint64_t vol_uuid, uint64_t capacity);
int hvisor_delete_blkdev(int minor);

#ifndef BLKTAP_UNIT_TEST

char  cppstr[2048];
#define cppout(...) sprintf(cppstr,__VA_ARGS__); \
					cppOut("s",cppstr);

#else

#define cppout printf

#endif



td_vbd_t*hvisor_vbd_create(uint64_t uuid, uint64_t capacity);

static void
__hvisor_run(td_vbd_t *vbd);

char data_image[16536];

void *hvisor_hdl;
td_vbd_t *hvisor_vbds[HVISOR_MAX_VBDS];

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
	  cppout("Failed to open control device %s: %d\n", BLKTAP2_CONTROL_DEVICE, errno);
          return errno;
	}

	err = ioctl(fd, BLKTAP2_IOCTL_ALLOC_TAP, &handle);
	close(fd);
	if (err == -1) {
		cppout("Failed to allocate new device: %d\n", errno);
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
	  cppout("Created ring device at %s\n", name);
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
	  cppout("Created IO device at %s\n", name);
	}

	cppout("New interface: ring: %u, device: %u, minor: %u\n",
	    handle.ring, handle.device, handle.minor);

	*minor = handle.minor;
	return 0;

fail:
	tap_ctl_free(handle.minor);
	return err;
}

void ctrlcHandler(int signal)
{
    td_ring_t *ring;
    td_vbd_t *vbd = hvisor_vbds[0];

#ifndef HVISOR_USPACE_TEST
    vbd->runningFlag = 0;
    ring  = &vbd->ring;
    if (ring->fd != -1)
        close(ring->fd);
#endif

#ifndef BLKTAP_UNIT_TEST
   ctrlCCallbackHandler(signal);
#endif

   exit(1);
}

td_vbd_t*
hvisor_vbd_create(uint64_t uuid, uint64_t capacity) {
    td_vbd_t *vbd;
    int i;

    vbd = calloc(1, sizeof(td_vbd_t));
    if (!vbd) {
        cppout("Failed to allocate tapdisk state\n");
        return NULL;
    }

    vbd->uuid     = uuid;
    vbd->minor    = -1;
    vbd->ring.fd  = -1;
    vbd->capacity = capacity;

    pthread_mutex_init(&vbd->vbd_mutex, 0);
    vbd->num_responses_in_ring = 0;
    vbd->num_pending_req_segs = 0;

    INIT_LIST_HEAD(&vbd->driver_stack);
    INIT_LIST_HEAD(&vbd->images);
    INIT_LIST_HEAD(&vbd->new_requests);
    INIT_LIST_HEAD(&vbd->pending_requests);
    INIT_LIST_HEAD(&vbd->failed_requests);
    INIT_LIST_HEAD(&vbd->completed_requests);
    INIT_LIST_HEAD(&vbd->next);
    gettimeofday(&vbd->ts, NULL);

    for (i = 0; i < MAX_REQUESTS; i++) {
        hvisor_vbd_initialize_vreq(vbd->request_list + i);
    }

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
		cppout("Failed to open %s: %d\n", devname, err);
		goto fail;
	}

	ring->mem = mmap(0, psize * BLKTAP_MMAP_REGION_SIZE,
			 PROT_READ | PROT_WRITE, MAP_SHARED|MAP_LOCKED, ring->fd, 0);
	if (ring->mem == MAP_FAILED) {
		err = -errno;
		cppout("Failed to mmap %s: %d\n", devname, err);
		goto fail;
	} else {
		cppout("MMapped ring region successfully\n");
	}

	ring->sring = (blkif_sring_t *)((unsigned long)ring->mem);
	BACK_RING_INIT(&ring->fe_ring, ring->sring, (psize * BLKTAP_RING_PAGES) );
	cppout("Initialized ring size:%d",RING_SIZE(&ring->fe_ring));

	ring->vstart =
		(unsigned long)ring->mem + (BLKTAP_RING_PAGES * psize);

	ioctl(ring->fd, BLKTAP_IOCTL_SETMODE, BLKTAP_MODE_INTERPOSE);
	cppout("Invoked set mode ioctl successfully on ring fd");

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

int
hvisor_create_blkdev(uint64_t vol_uuid, uint64_t capacity) {
    int minor = 0;
    char *io_devname = 0;
    char *ring_devname = 0;
    td_vbd_t *vbd;
    int err=0;

    // The capacity provided is in MB
    // Convert to bytes to pass in
    capacity *= (1024 * 1024);

    vbd = hvisor_vbd_create(vol_uuid, capacity);
  
    cppout("create blkdev for volume : %lx  capacity : %ld \n", vol_uuid, capacity);
    err = tap_ctl_allocate_device(&minor, &io_devname);
    if (err) {
        cppout("Failed to create ring and io devices for volume : %lx", vol_uuid);
        return (-1);
    }
  
    err = asprintf(&ring_devname, BLKTAP2_RING_DEVICE"%d", minor);
    if (err == -1) {
        err = -ENOMEM;
        cppout("Failed constructing ring device name for volume %lx ", vol_uuid);
        return (-1);
    }
  
    err = hvisor_create_io_ring(vbd, ring_devname);
    if (err) {
        cppout("Failed to create ring for volume %lx", vol_uuid);
        return (-1);
    }

    hvisor_vbds[minor] = vbd;
    vbd->minor = minor;
    vbd->runningFlag = 1;
    hvisor_set_capacity(minor, capacity);
    pthread_create(&vbd->tx_thread, NULL, __hvisor_run, vbd);
    return minor;
}


int
hvisor_delete_blkdev(int minor) {
    td_vbd_t *vbd;
    if (minor < HVISOR_MAX_VBDS && hvisor_vbds[minor] != NULL) { 
        vbd = hvisor_vbds[minor]; 
        vbd->runningFlag = 0;
        sleep(4);
        close(vbd->ring.fd);
        tap_ctl_free(minor);
        hvisor_vbds[minor] = NULL;
    }
    return 0;
}

int
hvisor_set_capacity(int minor, uint64_t capacity) {
    char devname[64];
    int fbd = -1, ret=0;
    memset(devname, 0x00, 64);
    sprintf(devname, "/dev/fbd%d",minor);
    fbd = open(devname, O_RDWR);

    if (fbd < 0)
        cppout( "Cannot open  the device: device was not  created \n"); 
     
    if((ret = ioctl(fbd, FBD_SET_TGT_SIZE, capacity)) < 0) {
        cppout("Error %d: setting the  target disk  size \n",ret);
        return ret;
    }
    return ret;
}

static int
hvisor_wait_for_events(td_vbd_t *vbd) {
    int ret;
    struct timeval tv;
    fd_set read_fds;

    tv.tv_sec  = 0;
    tv.tv_usec = HVISOR_MAX_REQ_RSP_HOLD_TIME;

    FD_ZERO(&read_fds);
    FD_SET(vbd->ring.fd, &read_fds);

    ret = select(vbd->ring.fd + 1, &read_fds,
                 0, 0, &tv);
  
    if (ret < 0)
        return ret;

    return 0;
}

// Caller should have acquired the vbd mutex
static inline void
hvisor_vbd_write_response_to_ring(td_vbd_t *vbd, blkif_response_t *rsp)
{
	td_ring_t *ring;
	blkif_response_t *rspp;

	ring = &vbd->ring;
	rspp = RING_GET_RESPONSE(&ring->fe_ring, ring->fe_ring.rsp_prod_pvt);
	memcpy(rspp, rsp, sizeof(blkif_response_t));
	cppout("Vol %d - Writing response for request %d at position %d\n", (int) vbd->uuid, (int) rsp->id, ring->fe_ring.rsp_prod_pvt);
	ring->fe_ring.rsp_prod_pvt++;
}

// Caller should have acquired the vbd mutex
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

	cppout("Vold %d - writing req %d, sec 0x%08"PRIx64", res %d to ring\n",
	       (int) vbd->uuid, (int)tmp.id, tmp.sector_number, vreq->status);

	if (rsp->status != BLKIF_RSP_OKAY){
	  cppout("returning BLKIF_RSP %d", rsp->status);
        }

	vbd->returned++;
	hvisor_vbd_write_response_to_ring(vbd, rsp);
}


// Caller should have acquired the vbd mutex
void
hvisor_complete_vbd_request(td_vbd_t *vbd, td_vbd_request_t *vreq) {
  if (!vreq->submitting && !vreq->secs_pending) {
    hvisor_vbd_make_response(vbd, vreq);
    vbd->num_responses_in_ring++;

    if ((reqs_in_hold) && (vbd->num_pending_req_segs < HVISOR_MAX_PENDING_REQ_SEGS)) {
      cppout("HVISOR BLKTAP: Issuing new requests will resume (%d)\n", vbd->num_pending_req_segs);
      reqs_in_hold = 0;
    }
    list_del(&vreq->next);
    hvisor_vbd_initialize_vreq(vreq);
  }
}

#define hvisor_complete_request  hvisor_complete_vbd_request

static void
hvisor_complete_td_request(void *arg1, void *arg2,
			   fbd_request_t *freq, int res)
{
        int err;
	// td_image_t *image = freq->image;
	td_vbd_t *vbd = (td_vbd_t *)arg1;
	td_vbd_request_t *vreq = (td_vbd_request_t *)arg2;

	ASSERT(freq->req_magic == FDS_UBD_IO_MAGIC_IN_USE);
	freq->req_magic = FDS_UBD_IO_MAGIC_NOT_IN_USE;

	pthread_mutex_lock(&vbd->vbd_mutex);

	vbd->num_pending_req_segs --;

	cppout("UBD: Recv completion callback with vbd - %p, vol- %lx, vreq - %p, fbd_req - %p, res - %d, pending_req_segs - %d\n",
	       vbd, vbd->uuid, vreq, freq, res, vbd->num_pending_req_segs);


        err = (res <= 0 ? res : -res);
        vbd->secs_pending  -= freq->secs;
        vreq->secs_pending -= freq->secs;
        // vreq->blocked = freq->blocked;

	/*
	// Ignore errors for now to continue with testing
        if (err) {
                vreq->status = BLKIF_RSP_ERROR;
                vreq->error  = (vreq->error ? : err);
                if (err != -EBUSY) {
                        vbd->errors++;
                }
        }
	*/



	hvisor_complete_vbd_request(vbd, vreq);

	if (vbd->num_responses_in_ring >= HVISOR_MAX_RESPONSES_TO_HOLD) {
	  hvisor_vbd_kick(vbd);
	}

	pthread_mutex_unlock(&vbd->vbd_mutex);
	
	free(freq);
}



void hvisor_queue_read(td_vbd_t *vbd,
                       td_vbd_request_t *vreq,
                       td_request_t treq) {
    int      size    = treq.secs * HVISOR_SECTOR_SIZE;
    uint64_t offset  = treq.sec * (uint64_t)HVISOR_SECTOR_SIZE;
    char test_c;
    int i;
    int rc = 0;
    fbd_request_t *p_new_req;

    p_new_req = (fbd_request_t *)malloc(sizeof(fbd_request_t));
    memset(p_new_req, 0 , sizeof(fbd_request_t ));
    p_new_req->sec = treq.sec;
    p_new_req->secs = treq.secs;
    p_new_req->buf = treq.buf;
    p_new_req->op = treq.op;
    p_new_req->io_type = 0;
    p_new_req->req_data = (void *)vreq;
    p_new_req->len = size;
    p_new_req->vbd = vbd;
    p_new_req->vReq = vreq;
    p_new_req->volUUID = vbd->uuid;
    p_new_req->hvisorHdl = hvisor_hdl;
    p_new_req->cb_request = hvisor_complete_td_request;
    p_new_req->req_magic = FDS_UBD_IO_MAGIC_IN_USE;

    cppout("Received read request at offset %lx for %d bytes, buf - %p \n", offset, size, p_new_req->buf);

    cppout("UBD: Sending read req to hypervisor with vbd - %p, vol - %lx, vreq - %p, fbd_req - %p\n",
           vbd, vbd->uuid, vreq, p_new_req);

    /* queue the  request  to the per volume queue */
    // Push the request into AM
    rc = pushFbdReq(p_new_req);
    if (rc) {
        hvisor_complete_td_request((void *)vbd, (void *)vreq, p_new_req, rc);
    }
}

void hvisor_queue_write(td_vbd_t *vbd, td_vbd_request_t *vreq, td_request_t treq)
{
	int      size    = treq.secs * HVISOR_SECTOR_SIZE;
	uint64_t offset  = treq.sec * (uint64_t)HVISOR_SECTOR_SIZE;
	int i;
	int rc = 0;
	fbd_request_t *p_new_req;
#if BLKTAP_UNIT_TEST
	static int num_write_reqs = 0;
#endif

	p_new_req = (fbd_request_t *)malloc(sizeof(fbd_request_t));
	memset(p_new_req, 0 , sizeof(fbd_request_t ));
	p_new_req->sec = treq.sec;
	p_new_req->secs = treq.secs;
	p_new_req->buf = treq.buf;
	p_new_req->op = treq.op;
	p_new_req->io_type = 1;
	p_new_req->req_data = (void *)vreq;
	p_new_req->len = size;
  	p_new_req->vbd = vbd;
  	p_new_req->vReq = vreq;
  	p_new_req->volUUID = vbd->uuid;
  	p_new_req->hvisorHdl = hvisor_hdl;
  	p_new_req->cb_request = hvisor_complete_td_request;
	p_new_req->req_magic = FDS_UBD_IO_MAGIC_IN_USE;

#if BLKTAP_UNIT_TEST 
	cppout("Received write request at offset %lx for %d bytes, buf - %p : \n", offset, size, p_new_req->buf);
//	for (i = 0; i < size; i++) {
//	  printf("%2x", treq.buf[i]);
//	}
//	printf("\n");

	if (offset + size < sizeof(data_image)) {
	  memcpy(data_image + offset, p_new_req->buf, size);
	}
  #if BLKTAP_UNIT_TEST_CRASH
	if (num_write_reqs == 2) {
		ASSERT(0);
	}
  #endif
	num_write_reqs++;

	hvisor_complete_td_request((void *)vbd, (void *)vreq, p_new_req, rc);
#else

	cppout("UBD: Sending write req to hypervisor with vbd - %p, vol - %lx, vreq - %p, fbd_req - %p\n",
	       vbd, vbd->uuid, vreq, p_new_req);

	/* queue the  request  to the per volume queue */
        rc = amUbdPutBlob(p_new_req);
	if (rc) {
	  hvisor_complete_td_request((void *)vbd, (void *)vreq, p_new_req, rc);
	}
#endif

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

		pthread_mutex_lock(&vbd->vbd_mutex);
		vbd->num_pending_req_segs++;
		cppout( "%s: Issuing req %d seg %d sec 0x%08"PRIx64" secs 0x%04x "
		    "buf %p op %d num_pending_segs - %d\n", "Hvisor", id, i, treq.sec, treq.secs,
		       treq.buf, (int)req->operation, vbd->num_pending_req_segs);


		vreq->secs_pending += nsects;
		vbd->secs_pending  += nsects;

		pthread_mutex_unlock(&vbd->vbd_mutex);

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

	pthread_mutex_lock(&vbd->vbd_mutex);

	vreq->submitting--;
	
	if (!vreq->secs_pending) {
		err = (err ? : vreq->error);
		hvisor_complete_vbd_request(vbd, vreq);
	}

	pthread_mutex_unlock(&vbd->vbd_mutex);

	return err;

fail:
	vreq->status = BLKIF_RSP_ERROR;
	goto out;
}

static int
hvisor_issue_new_requests(td_vbd_t *vbd)
{
        int err;
        td_vbd_request_t *vreq, *tmp;

        tapdisk_vbd_for_each_request(vreq, tmp, &vbd->new_requests) {
	  // If we have sent too many outstanding reqs to hvisor,
	  // We need to throttle back.
	  pthread_mutex_lock(&vbd->vbd_mutex);
	  if (vbd->num_pending_req_segs >= HVISOR_MAX_PENDING_REQ_SEGS) {
	    if (!(reqs_in_hold)) {
		cppout("HVISOR BLKTAP: Too many outstanding requests (%d). Holding from issuing new requests to HVISOR\n", vbd->num_pending_req_segs);
		reqs_in_hold = 1;
	      }
	    pthread_mutex_unlock(&vbd->vbd_mutex);
	    break;
	  }
	  pthread_mutex_unlock(&vbd->vbd_mutex);
	  list_del(&vreq->next);
	  INIT_LIST_HEAD(&vreq->next);
	  err = hvisor_handle_request(vbd, vreq);

	  if (err)
	    return err;
        }

        return 0;
}

// Caller should have acquired the vbd mutext lock
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
		cppout("Pulled request %d at position %d\n", (int) req->id, rc);

		idx  = req->id;
		vreq = &vbd->request_list[idx];

		ASSERT(list_empty(&vreq->next));
		ASSERT(vreq->secs_pending == 0);

		memcpy(&vreq->req, req, sizeof(blkif_request_t));
		vbd->received++;
		vreq->vbd = vbd;

		tapdisk_vbd_move_request(vreq, &vbd->new_requests);

		// hvisor_handle_request(vbd, vreq);

		cppout("%s: queueing request with idx %d \n", "Hvisor" , idx); 
	}
}


// Caller should have acquired the vbd mutex lock
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
	cppout("Vol 0x%lx - Pushing %d responses, rsp prod idx moving  from position %d to position %d\n", 
	       vbd->uuid, n, ring->fe_ring.sring->rsp_prod, ring->fe_ring.rsp_prod_pvt);
	RING_PUSH_RESPONSES(&ring->fe_ring);
	ioctl(ring->fd, BLKTAP_IOCTL_KICK_FE, 0);

	cppout("vol 0x%lx minor %d - kicking %d: rec: 0x%08"PRIx64", ret: 0x%08"PRIx64", kicked: "
	       "0x%08"PRIx64"\n", vbd->uuid, vbd->minor, n, vbd->received, vbd->returned, vbd->kicked);

	vbd->num_responses_in_ring = 0;

	return n;
}

static void
__hvisor_run(td_vbd_t *vbd)
{
  int ret;
  while (vbd->runningFlag) {

    ret = hvisor_wait_for_events(vbd);

    if (ret < 0){
      cppout("server wait returned %d\n", ret);
    }

    pthread_mutex_lock(&vbd->vbd_mutex);
    hvisor_pull_ring_requests(vbd);
    pthread_mutex_unlock(&vbd->vbd_mutex);

    hvisor_issue_new_requests(vbd);

    pthread_mutex_lock(&vbd->vbd_mutex);
    hvisor_vbd_kick(vbd);
    pthread_mutex_unlock(&vbd->vbd_mutex);

   ret=0;
  }
}


#ifdef HVISOR_USPACE_TEST

static void
hvisor_complete_td_request_noop(void *arg1, void *arg2,
                           fbd_request_t *freq, int res) {

  cppout("HVISOR USPACE Test: Received response: %d\n", res);
  free(freq);

}


int8_t  buf[4096];
int send_test_io(int offset, int vol_id) {
  int len = 4096;
  int  i;
  fbd_request_t *p_new_treq;
  p_new_treq = (fbd_request_t *)malloc(sizeof(fbd_request_t));

  for ( i = 1; i < len; i++)
    buf[i] = i;

  //        memcpy((void *)(p_new_treq->buf), (void *)buf, len);

  p_new_treq->op = 1;
  p_new_treq->io_type = 1;
  p_new_treq->buf = buf;;
  p_new_treq->sec = offset/HVISOR_SECTOR_SIZE;
  p_new_treq->secs = 8;
  p_new_treq->len = len;
  p_new_treq->volUUID = vol_id;
  p_new_treq->vbd = NULL;
  p_new_treq->vReq = NULL;
  p_new_treq->hvisorHdl = hvisor_hdl;
  p_new_treq->cb_request = hvisor_complete_td_request_noop;
           
  /*
   * TODO: Let's not always return 0 here.
   */
  pushFbdReq(p_new_treq);
  return 0;
}

int8_t  read_buf[4096];
int read_test_io(int offset, int vol_id)
{
    fbd_request_t *p_new_treq;
    int len = 4096;
    p_new_treq = (fbd_request_t *)malloc(sizeof(fbd_request_t));

    memset((void *)read_buf, 0, len);
    p_new_treq->op = 0;
    p_new_treq->io_type = 0;
    p_new_treq->buf = buf;;
    p_new_treq->sec = offset/HVISOR_SECTOR_SIZE;
    p_new_treq->secs = 8;
    p_new_treq->len = len;
    p_new_treq->volUUID = vol_id;
    p_new_treq->vbd = NULL;
    p_new_treq->vReq = NULL;
    p_new_treq->hvisorHdl = hvisor_hdl;
    p_new_treq->cb_request = hvisor_complete_td_request_noop;

    pushFbdReq(p_new_treq);
    return 0;

}

#endif
