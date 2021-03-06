# Copyright (c) 2014 Formation Data Systems
# All Rights Reserved.

from oslo.config import cfg
import paramiko

from cinder import context
from cinder.db.sqlalchemy import api
from cinder import exception
from cinder.image import image_utils
from cinder.openstack.common import log as logging
from cinder.openstack.common.processutils import ProcessExecutionError
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
               help='CS port'),
    cfg.StrOpt('ssh_user',
               default='root',
               help='ssh user'),
    cfg.StrOpt('ssh_key',
               default=None,
               help='ssh key'),
    cfg.StrOpt('ssh_password',
               default=None,
               help='ssh password'),
    ];


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
        cs_connection_info = (self.configuration.fds_cs_host, self.configuration.fds_cs_port)
        fds = FDSServices(am_connection_info, cs_connection_info)
        return fds

    @property
    def fds_domain(self):
        return self.configuration.fds_domain

    @property
    def fds_volume(self):
        return self.configuration.fds_volume

    def ssh_execute(self, ip, cmd):
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.WarningPolicy())
        if self.configuration.ssh_password is not None:
            ssh.connect(ip,
                        username=self.configuration.ssh_user,
                        password=self.configuration.ssh_password)
        elif self.configuration.ssh_key is not None:
            ssh.connect(ip,
                        username=self.configuration.ssh_user,
                        key_filename=self.configuration.ssh_key)
        else:
            ssh.connect(ip, username=self.configuration.ssh_user)

        channel = ssh.get_transport().open_session()
        channel.exec_command(cmd)
        stdout_handle = channel.makefile()
        stderr_handle = channel.makefile_stderr()
        exit_code = channel.recv_exit_status()
        ssh.close()
        return exit_code, stdout_handle.read(), stderr_handle.read()


    def _attach_fds_block_dev(self, fds, name, ip=None):
        if ip is not None:
            host_spec = self.configuration.ssh_user + '@' + ip
            key = self.configuration.ssh_key
            cmd = 'nbdadm.py attach "%s" "%s"' % (self.configuration.fds_nbd_server, name)
            (exit_code, stdout, stderr) = self.ssh_execute(ip, 'nbdadm.py attach "%s" "%s"' % (self.configuration.fds_nbd_server, name))
            if exit_code != 0:
                raise ProcessExecutionError(stdout = stdout, stderr = stderr, exit_code = exit_code, cmd = cmd, description='failed to attach volume on host ' + ip)
            #(stdout, stderr) = self._execute('ssh', '-i', key, '-o', 'StrictHostKeyChecking=no', host_spec, 'nbdadm.py', 'attach', self.configuration.fds_nbd_server, name, run_as_root=False)
            return stdout.strip()
        else:
            (stdout, stderr) = self._execute('nbdadm.py', 'attach', self.configuration.fds_nbd_server, name, run_as_root=True)
            return stdout.strip()



    def _detach_fds_block_dev(self, fds, name, ip=None):
        if ip is not None:
            host_spec = self.configuration.ssh_user + '@' + ip
            key = self.configuration.ssh_key
            cmd = 'ndbadm.py detach "%s "%s"' % (self.configuration.fds_nbd_server, name)
            (exit_code, stdout, stderr) = self.ssh_execute(ip, 'nbdadm.py attach "%s" "%s"' % (self.configuration.fds_nbd_server, name))
            if exit_code != 0:
                raise ProcessExecutionError(stdout = stdout, stderr = stderr, exit_code = exit_code, cmd = cmd, description='failed to detach volume on host ' + ip)
            #(stdout, stderr) = self._execute('ssh', '-i', key, '-o', 'StrictHostKeyChecking=no', host_spec, 'nbdadm.py', 'detach', self.configuration.fds_nbd_server, name, run_as_root=False)
        else:
            (stdout, stderr) = self._execute('nbdadm.py', 'detach', self.configuration.fds_nbd_server, name, run_as_root=True)


    @contextmanager
    def _use_block_device(self, fds, name, ip = None):
        yield self._attach_fds_block_dev(fds, name, ip)
        self._detach_fds_block_dev(fds, name, ip)

    def check_for_setup_error(self):
        # FIXME: do checks
        # ensure domain exists
        pass

    def create_volume(self, volume):
        with self._get_services() as fds:
            volume_size = volume['size'] * 1024 * 1024 * 1024
            LOG.error(_('Volume name %s, volume size %s' % (volume['name'], volume_size)))
            fds.cs.createVolume(self.fds_domain, volume['name'], VolumeSettings(4096, VolumeType.BLOCK, volume_size))


    def delete_volume(self, volume):
        with self._get_services() as fds:
            self._detach_fds_block_dev(self, volume['name'])
            fds.cs.deleteVolume(self.fds_domain, volume['name'])

    def initialize_connection(self, volume, connector):
        with self._get_services() as fds:
            device = self._attach_fds_block_dev(fds, volume['name'], connector['ip'])
            return {
                'driver_volume_type': 'local',
                'data': {'device_path': device }
            }

    def terminate_connection(self, volume, connector, **kwargs):
        with self._get_services() as fds:
            self._detach_fds_block_dev(fds, volume['name'], connector['ip'])

    def get_volume_stats(self, refresh=False):
        # FIXME: real stats
        return {
            'driver_version': self.VERSION,
            'free_capacity_gb': 100,
            'total_capacity_gb': 100,
            'reserved_percentage': self.configuration.reserved_percentage,
            'storage_protocol': 'local',
            'vendor_name': 'Formation Data Systems, Inc.',
            'volume_backend_name': 'FDS',
        }

    #def local_path(self, volume):
    #    return self.attached_devices.get(volume['name'], None)

    # we probably don't need to worry about exports (ceph/rbd doesn't)
    def remove_export(self, context, volume):
        pass

    def create_export(self, context, volume):
        pass

    def ensure_export(self, context, volume):
        pass

    def copy_image_to_volume(self, context, volume, image_service, image_id):
        with self._get_services() as fds:
            with self._use_block_device(fds, volume["name"]) as dev:
                image_utils.fetch_to_raw(
                    context,
                    image_service,
                    image_id,
                    dev,
                    self.configuration.volume_dd_blocksize,
                    size=volume["size"])

    def copy_volume_to_image(self, context, volume, image_service, image_meta):
        with self._get_services() as fds:
            with self._use_block_device(fds, volume["name"]) as dev:
                image_utils.upload_volume(
                    context,
                    image_service,
                    image_meta,
                    dev)


