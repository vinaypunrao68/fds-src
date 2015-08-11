# Copyright (c) 2014 Formation Data Systems
# All Rights Reserved.

from oslo.config import cfg
import traceback
import uuid
import paramiko

from cinder import context
from cinder.db.sqlalchemy import api
from cinder import exception
from cinder.image import image_utils
from cinder.openstack.common import log as logging
from cinder.openstack.common.processutils import ProcessExecutionError
from cinder.volume import driver

from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
import os

from contextlib import contextmanager
from cinder.volume.drivers.fds.fds_common import FDSServices, NbdManager

LOG = logging.getLogger(__name__)

volume_opts = [
    cfg.StrOpt('fds_nbd_server',
                default='localhost',
                help='FDS NBD server hostname'),
    cfg.StrOpt('fds_domain',
                default='fds',
                help='FDS storage domain'),
    cfg.StrOpt('fds_am_host',
               default='localhost',
               help='AM hostname'),
    cfg.IntOpt('fds_am_port',
               default=9988,
               help='AM port'),
    cfg.StrOpt('fds_cs_host',
               default='localhost',
               help='CS hostname'),
    cfg.IntOpt('fds_cs_port',
               default=9090,
               help='CS port')]

CONF = cfg.CONF
CONF.register_opts(volume_opts)

class RoundRobinPool:
    def __init__(self, pool):
        self.pool = pool
        self.idx = 0

    def next(self):
        item = self.pool[self.idx]
        self.idx = (self.idx + 1) % len(self.pool)
        return item

class FDSNBDDriver(driver.VolumeDriver):
    VERSION = "0.0.1"

    def __init__(self, *args, **kwargs):
        super(FDSNBDDriver, self).__init__(*args, **kwargs)
        self.configuration.append_config_values(volume_opts)
        self.nbd = NbdManager(self._execute)

        nbds = [x for x in str(self.configuration.fds_nbd_server).split(",")]
        self.nbd_attach_endpoints = RoundRobinPool(nbds)
        self.nbd_image_endpoints = RoundRobinPool(nbds)

        endpts = []
        for am_host in str(self.configuration.fds_am_host).split(","):
            endpts.append(((am_host, self.configuration.fds_am_port),
                                   (self.configuration.fds_cs_host, self.configuration.fds_cs_port)))
        self.endpoints = RoundRobinPool(endpts)
	LOG.warning("FDS DRIVER: initalized with config - domain: %s nbd: %s om: %s" % (self.configuration.fds_domain, nbds, (self.configuration.fds_cs_host, self.configuration.fds_cs_port)));


    def set_execute(self, execute):
        super(FDSNBDDriver, self).set_execute(execute)
        self.nbd = NbdManager(execute)

    def _get_services(self):
        (am_connection_info, cs_connection_info) = self.endpoints.next()
        fds = FDSServices(am_connection_info, cs_connection_info)
        return fds

    @property
    def fds_domain(self):
        return self.configuration.fds_domain

    def check_for_setup_error(self):
        # FIXME: do checks
        # ensure domain exists
	LOG.warning("FDS DRIVER: setup check");

    def create_volume(self, volume):
        LOG.warning('FDS_DRIVER: create volume %s begin' % volume['name'])
        try:
            with self._get_services() as fds:
                fds.create_volume(self.fds_domain, volume)
                LOG.warning('FDS_DRIVER: create volume %s success' % volume['name'])
        except Exception as e:
            LOG.warning('FDS_DRIVER: create volume %s failed with exception %s' % (volume['name'], traceback.format_exc()))
            raise

    def delete_volume(self, volume):
        LOG.warning('FDS_DRIVER: delete volume %s begin' % volume['name'])
        try:
            with self._get_services() as fds:
                fds.delete_volume(self.fds_domain, volume)
                LOG.warning('FDS_DRIVER: delete volume %s success' % volume['name'])
        except Exception as e:
            LOG.warning('FDS_DRIVER: delete volume %s failed with exception %s' % (volume['name'], traceback.format_exc()))
            raise

    @staticmethod
    def host_to_nbdd_url(host):
        return "http://%s:10511" % host

    def initialize_connection(self, volume, connector):
        LOG.warning('FDS_DRIVER: attach volume %s to %s begin' % (volume['name'], connector['ip']))
        try:
            url = self.host_to_nbdd_url(connector['ip'])
            device = self.nbd.attach_nbd_remote(url, self.nbd_attach_endpoints.next(), volume['name'])
            LOG.warning('FDS_DRIVER: attach volume %s to %s success' % (volume['name'], connector['ip']))
            return {
                'driver_volume_type': 'local',
                'data': {'device_path': device }
            }

        except Exception as e:
            LOG.warning('FDS_DRIVER: attach volume %s to %s failed with exception %s' % (volume['name'], connector['ip'], traceback.format_exc()))
            raise

    def terminate_connection(self, volume, connector, **kwargs):
        LOG.warning('FDS_DRIVER: detach volume %s to %s begin' % (volume['name'], connector['ip']))
        try:
            url = self.host_to_nbdd_url(connector['ip'])
            self.nbd.detach_nbd_remote_all(url, volume['name'])
            LOG.warning('FDS_DRIVER: detach volume %s to %s success' % (volume['name'], connector['ip']))
        except Exception as e:
            LOG.warning('FDS_DRIVER: detach volume %s to %s failed with exception %s' % (volume['name'], connector['ip'], traceback.format_exc()))
            raise

    def get_volume_stats(self, refresh=False):
        # FIXME: real stats
        return {
            'driver_version': self.VERSION,
            'free_capacity_gb': 4000,
            'total_capacity_gb': 4000,
            'reserved_percentage': self.configuration.reserved_percentage,
            'storage_protocol': 'local',
            'vendor_name': 'Formation Data Systems, Inc.',
            'volume_backend_name': 'FDS',
        }

    # we probably don't need to worry about exports (ceph/rbd doesn't)
    def remove_export(self, context, volume):
        pass

    def create_export(self, context, volume):
        pass

    def ensure_export(self, context, volume):
        pass

    def copy_image_to_volume(self, context, volume, image_service, image_id):
        """Fetch the image from image_service and write it to the volume."""
        nbd_server = self.nbd_image_endpoints.next()
        with self.nbd.use_nbd_local(nbd_server, volume["name"]) as dev:
            image_utils.fetch_to_raw(context,
                                 image_service,
                                 image_id,
                                 dev,
                                 128 * 1024,
                                 size=volume['size'])

    def copy_volume_to_image(self, context, volume, image_service, image_meta):
        """Copy the volume to the specified image."""
        nbd_server = self.nbd_image_endpoints.next()
        with self.nbd.use_nbd_local(nbd_server, volume["name"]) as dev:
            image_utils.upload_volume(context,
                                  image_service,
                                  image_meta,
                                  dev)

#            temp_filename="/tmp/fds_vol_" + str(uuid.uuid4())
#            image_utils.fetch_to_raw(
#                context,
#                image_service,
#                image_id,
#                temp_filename,
#                size=volume["size"])
#            self._execute('dd', 'if=' + temp_filename, 'of=' + dev, 'bs=131072', 'oflag=sync', run_as_root=True)
