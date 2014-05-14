# Copyright (c) 2014 Formation Data Systems
# All Rights Reserved.

from oslo.config import cfg

from cinder import context
from cinder.db.sqlalchemy import api
from cinder import exception
from cinder.image import image_utils
from cinder.openstack.common import log as logging
from cinder.volume import driver
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
    cfg.StrOpt('fds_domain',
                default='',
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

class FDSBlockVolumeDriver(driver.VolumeDriver):
    VERSION = "0.0.1"

    def __init__(self, *args, **kwargs):
        super(FDSBlockVolumeDriver, self).__init__(*args, **kwargs)
        self.configuration.append_config_values(volume_opts)
        self.attached_devices = dict()

    def _get_services(self):
        am_connection_info = (self.configuration.fds_am_host, self.configuration.fds_am_port)
        cm_connection_info = (self.configuration.fds_cm_host, self.configuration.fds_cm_port)
        fds = FDSServices(am_connection_info, cm_connection_info)
        return fds

    @property
    def fds_domain(self):
        return self.configuration.fds_domain

    @property
    def fds_volume(self):
        return self.configuration.fds_volume

    def _attach_fds_block_dev(self, fds, name):
        if self.attached_devices.has_key(name):
            return self.attached_devices[name]

        fds.am.attachVolume(self.fds_domain, name)
        # FIXME: attachVolume needs to expose a device name instead of us looking through /dev
        cur_attached_devices = set(self.attached_devices.itervalues())
        for dev in os.listdir('/dev'):
            if dev.startswith('fbd') and dev[:3].isdigit() and dev not in cur_attached_devices:
                dev_path = "/dev/" + dev
                self.attached_devices[name] = dev_path
                return "/dev/" + dev

    def _detach_fds_block_dev(self, fds, name):
        # FIXME: do something here
        # del self.attached_devices[name]
        pass

    def check_for_setup_error(self):
        # FIXME: do checks
        # ensure domain exists
        pass

    def create_volume(self, volume):
        with self._get_services() as fds:
            fds.cs.createVolume(self.fds_domain, volume['name'], VolumePolicy(4096))


    def delete_volume(self, volume):
        with self._get_services() as fds:
            if volume['name'] in self.attached_devices:
                self._detach_fds_block_dev(self, volume['name'])
            fds.cs.deleteVolume(self.fds_domain, volume['name'])

    def initialize_connection(self, volume, connector):
        with self._get_services() as fds:
            device = self._attach_fds_block_dev(volume['name'])
            return {
                'driver_volume_type': 'local',
                'data': {'device_path': device }
            }

    def terminate_connection(self, volume, connector, **kwargs):
        self._detach_fds_block_dev(self, volume['name'])

    def get_volume_stats(self, refresh=False):
        # FIXME: real stats
        return {
            'driver_version': self.VERSION,
            'free_capacity_gb': 'unknown',
            'total_capacity_gb': 'unknown',
            'reserved_percentage': self.configuration.reserved_percentage,
            'storage_protocol': 'local',
            'vendor_name': 'Formation Data Systems, Inc.',
            'volume_backend_name': 'FDS',
        }

    def local_path(self, volume):
        return self.attached_devices.get(volume['name'], None)
