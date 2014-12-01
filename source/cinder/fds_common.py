# Copyright (c) 2014 Formation Data Systems
# All Rights Reserved.

from oslo.config import cfg
import re
import requests

from cinder import context
from cinder.db.sqlalchemy import api
from cinder import exception
from cinder.image import image_utils
from cinder.openstack.common import log as logging
from cinder.openstack.common.processutils import ProcessExecutionError
from cinder.volume import driver
from cinder.brick import exception as brick_exception
from cinder.brick.iscsi import iscsi
from cinder.volume.driver import ISCSIDriver
from cinder.volume.drivers.fds.apis.AmService import Client as AmClient
from cinder.volume.drivers.fds.apis.ConfigurationService import Client as CsClient
from cinder.openstack.common import processutils
from cinder import utils
from cinder.volume import driver
from cinder.volume import utils as volutils

from apis.ttypes import *
from thrift.transport.TSocket import TFramedTransport
from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
from apis.AmService import Client as AmClient
from apis.ConfigurationService import Client as CsClient
import os
import uuid

from contextlib import contextmanager

LOG = logging.getLogger(__name__)

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
        sock = TFramedTransport(TSocket.TSocket(host, port))
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

    def create_volume(self, domain, volume):
        try:
            # FIXME: Hack for now
            self.cs.createVolume(domain,
                                 volume['name'],
                                 VolumeSettings(4096,
                                                VolumeType.BLOCK,
                                                volume['size'] * (1024 ** 3),
                                                0), # cont Commit Log Retention, wtf?
                                 0) # Tenant ID, default to admin
        except ApiException as ex:
            if ex.errorCode != 3:
                raise

    def delete_volume(self, domain, volume):
        try:
            self.cs.deleteVolume(domain, volume['name'])
        except ApiException as ex:
            if ex.errorCode != 1:
                raise


class NbdManager(object):
    def __init__(self, executor):
        self._execute = executor

    def attach_nbd_local(self, nbd_server, volume_name):
        (stdout, stderr) = self._execute('nbdadm.py', 'attach', nbd_server, volume_name, run_as_root=True)
        return stdout.strip()


    def detach_nbd_local(self, nbd_server, volume_name):
        self._execute('nbdadm.py', 'detach', nbd_server, volume_name, run_as_root=True)

    @staticmethod
    def execute_remote_nbd_operation(url, operation, nbd_server, volume_name):
        params = {'op': operation, 'host': nbd_server, 'volume': volume_name}
        result = requests.get(url, params=params)
        if result.status_code != 200:
            raise Exception('NBD REST service responded with HTTP status code %d' % result.status_code)
        json_result = result.json()
        if json_result['result'] != 'ok':
            raise Exception('NBD REST operation failed with error code %d: %s' % (json_result['error_code'], json_result['error']))

        return json_result['output'].strip().encode('ascii', 'ignore')

    def detach_nbd_remote(self, url, nbd_server, volume_name):
        self.execute_remote_nbd_operation(url, 'detach', nbd_server, volume_name)

    def attach_nbd_remote(self, url, nbd_server, volume_name):
        return self.execute_remote_nbd_operation(url, 'attach', nbd_server, volume_name)

    @contextmanager
    def use_nbd_local(self, nbd_server, name):
        yield self.attach_nbd_local(nbd_server, name)
        self.detach_nbd_local(nbd_server, name)

    def image_via_nbd(self, nbd_server, context, volume, image_service, image_id):
        with self.use_nbd_local(nbd_server, volume["name"]) as dev:
            LOG.warning('Copy image to volume: %s %s' % (dev, volume["size"]))
            temp_filename="/tmp/fds_vol_" + str(uuid.uuid4())
            image_utils.fetch_to_raw(
                context,
                image_service,
                image_id,
                temp_filename,
                size=volume["size"])
            self._execute('dd', 'if=' + temp_filename, 'of=' + dev, 'bs=4096', 'oflag=sync', run_as_root=True)

    def set_execute(self, execute):
        self._execute = execute
