# Copyright (c) 2014 Formation Data Systems
# All Rights Reserved.

from oslo.config import cfg

from cinder import context
from cinder.db.sqlalchemy import api
from cinder import exception
from cinder.image import image_utils
from cinder.openstack.common import log as logging
from cinder.openstack.common.processutils import ProcessExecutionError
from cinder.volume import driver
from cinder.volume.driver import ISCSIDriver
from cinder.volume.drivers.fds.apis.AmService import Client as AmClient
from cinder.volume.drivers.fds.apis.ConfigurationService import Client as CsClient

from apis.ttypes import *
from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
from apis.AmService import Client as AmClient
from apis.ConfigurationService import Client as CsClient
import os

from contextlib import contextmanager

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
               help='CS port')
]

CONF = cfg.CONF
CONF.register_opts(volume_opts)


class FDSServices(object):
    def __init__(self, am=("localhost", 9988), cs=("localhost", 9090)):
        self.am_connection_info = am
        self.cs_connection_info = cs

        self.config_service_transport = None
        self.config_service_client = None

        self.am_transport = None
        self.am_client = None

    @staticmethod
    def _make_connection(host, port):
        sock = TSocket.TSocket(host, port)
        sock.open()
        protocol = TBinaryProtocol.TBinaryProtocol(sock)
        return sock, protocol

    @property
    def am(self):
        (host, port) = self.am_connection_info
        if self.am_client is None:
            (transport, protocol) = self._make_connection(host, port)
            self.am_client = AmClient(protocol)
            self.config_service_transport = transport
        return self.am_client

    @property
    def cs(self):
        (host, port) = self.cs_connection_info
        if self.config_service_client is None:
            (transport, protocol) = self._make_connection(host, port)
            self.config_service_client = CsClient(protocol)
            self.am_transport = transport
        return self.config_service_client

    def __exit__(self, exc_type, exc_val, exc_tb):
        # make a best effort to close the connections
        try:
            if self.am_transport is not None:
                self.am_transport.close()

        except:
            pass  # make a best effort to close the connections, possibly log

        try:
            if self.config_service_transport is not None:
                self.config_service_transport.close()

        except:
            pass

    def __enter__(self):
        return self


class FDSISCSIDriver(driver.ISCSIDriver):
    def __init__(self, *args, **kwargs):
        self.db = kwargs.get('db')
        self.target_helper = self.get_target_helper(self.db)
        super(FDSISCSIDriver, self).__init__(*args, **kwargs)
        self.configuration.append_config_values(volume_opts)

    def set_execute(self, execute):
        super(FDSISCSIDriver, self).set_execute(execute)
        if self.target_helper is not None:
            self.target_helper.set_execute(execute)

    @staticmethod
    def get_nbdadm_path():
        #me = os.path.realpath(__file__)
        #return os.path.join(os.path.dirname(me), 'nbdadm')
        return 'nbdadm.py'

    def _get_services(self):
        am_connection_info = (self.configuration.fds_am_host, self.configuration.fds_am_port)
        cs_connection_info = (self.configuration.fds_cs_host, self.configuration.fds_cs_port)
        fds = FDSServices(am_connection_info, cs_connection_info)
        return fds

    @property
    def fds_domain(self):
        return self.configuration.fds_domain

    @property
    def fds_volume(self):
        return self.configuration.fds_volume

    def create_volume(self, volume):
        with self._get_services() as fds:
            fds.cs.createVolume(self.fds_domain, volume['name'], VolumeSettings(4096, VolumeType.BLOCK, volume['size'] * (1024 ** 3)))


    def delete_volume(self, volume):
        with self._get_services() as fds:
            self._detach_fds_block_dev(volume['name'])
            fds.cs.deleteVolume(self.fds_domain, volume['name'])


    def _attach_fds_block_dev(self, name):
        (stdout, stderr) = \
            self._execute(self.get_nbdadm_path(), 'attach', self.configuration.fds_nbd_server, name, run_as_root=True)

        return stdout.strip()

    def _detach_fds_block_dev(self, name):
        self._execute(self.get_nbdadm_path(), 'detach', self.configuration.fds_nbd_server, name, run_as_root=True)

    @contextmanager
    def _use_block_device(self, name):
        yield self._attach_fds_block_dev(name)
        self._detach_fds_block_dev(name)

    def copy_image_to_volume(self, context, volume, image_service, image_id):
        with self._use_block_device(volume["name"]) as dev:
            image_utils.fetch_to_raw(
                context,
                image_service,
                image_id,
                dev,
                self.configuration.volume_dd_blocksize,
                size=volume["size"])

    def copy_volume_to_image(self, context, volume, image_service, image_meta):
        with self._use_block_device( volume["name"]) as dev:
            image_utils.upload_volume(
                context,
                image_service,
                image_meta,
                dev)

    def ensure_export(self, context, volume):
        volume_name = volume['name']
        iscsi_name = "%s%s" % ("fds-cinder-", volume_name)
        volume_path = self._attach_fds_block_dev(volume_name)
        model_update = self.target_helper.ensure_export(context, volume, iscsi_name, volume_path)
        if model_update:
            self.db.volume_update(context, volume['id'], model_update)

    def create_export(self, context, volume):
        volume_path = self._attach_fds_block_dev(volume['name'])
        data = self.target_helper.create_export(context, volume, volume_path)
        return {
            'provider_location': data['location'],
            'provider_auth': data['auth'],
        }

    def remove_export(self, context, volume):
        self.target_helper.remove_export(context, volume)
        self._detach_fds_block_dev(volume['name'])

    def get_volume_stats(self, refresh=False):
        # FIXME: real stats
        return {
            'driver_version': self.VERSION,
            'free_capacity_gb': 1000,
            'total_capacity_gb': 1000,
            'reserved_percentage': self.configuration.reserved_percentage,
            'storage_protocol': 'iSCSI',
            'vendor_name': 'Formation Data Systems, Inc.',
            'volume_backend_name': 'FDS',
        }

    def check_for_setup_error(self):
        pass
