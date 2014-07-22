# Copyright (c) 2014 Formation Data Systems
# All Rights Reserved.

from oslo.config import cfg
import re

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
from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
from apis.AmService import Client as AmClient
from apis.ConfigurationService import Client as CsClient
import os
import uuid

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
        #self.target_helper = self.get_target_helper(self.db)
        self.tgtadm = self.get_target_admin()
        super(FDSISCSIDriver, self).__init__(*args, **kwargs)
        self.configuration.append_config_values(volume_opts)
        self.backend_name =\
            self.configuration.safe_get('volume_backend_name') or 'FDS_iSCSI'
        self.protocol = 'iSCSI'

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
            try:
                fds.cs.createVolume(self.fds_domain, volume['name'], VolumeSettings(4096, VolumeType.BLOCK, volume['size'] * (1024 ** 3)))
            except ApiException as ex:
                if ex.errorCode != 3:
                    raise

    def delete_volume(self, volume):
        with self._get_services() as fds:
            self._detach_fds_block_dev(volume['name'])
            try:
                fds.cs.deleteVolume(self.fds_domain, volume['name'])
            except ApiException as ex:
                if ex.errorCode != 1:
                    raise


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
            LOG.warning('Copy image to volume: %s %s' % (dev, volume["size"]))
            tempfilename="/tmp/fds_vol_" + str(uuid.uuid4())
            image_utils.fetch_to_raw(
                context,
                image_service,
                image_id,
                tempfilename,
                size=volume["size"])
            self._execute('dd', 'if=' + tempfilename, 'of=' + dev, 'bs=4096', run_as_root=True)

    def copy_volume_to_image(self, context, volume, image_service, image_meta):
        with self._use_block_device( volume["name"]) as dev:
            image_utils.upload_volume(
                context,
                image_service,
                image_meta,
                dev)

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

    # the code below is more or less copied entirely from LVMISCSI

    def set_execute(self, execute):
        super(FDSISCSIDriver, self).set_execute(execute)
        self.tgtadm.set_execute(execute)

    def _create_tgtadm_target(self, iscsi_name, iscsi_target,
                              volume_path, chap_auth, lun=0,
                              check_exit_code=False, old_name=None):
        # NOTE(jdg): tgt driver has an issue where with alot of activity
        # (or sometimes just randomly) it will get *confused* and attempt
        # to reuse a target ID, resulting in a target already exists error
        # Typically a simple retry will address this

        # For now we have this while loop, might be useful in the
        # future to throw a retry decorator in common or utils
        attempts = 2
        while attempts > 0:
            attempts -= 1
            try:
                # NOTE(jdg): For TgtAdm case iscsi_name is all we need
                # should clean this all up at some point in the future
                tid = self.tgtadm.create_iscsi_target(
                    iscsi_name,
                    iscsi_target,
                    0,
                    volume_path,
                    chap_auth,
                    check_exit_code=check_exit_code,
                    old_name=old_name)
                break

            except brick_exception.ISCSITargetCreateFailed:
                if attempts == 0:
                    raise
                else:
                    LOG.warning(_('Error creating iSCSI target, retrying '
                                  'creation for target: %s') % iscsi_name)
        return tid

    def ensure_export(self, context, volume):
        """Synchronously recreates an export for a logical volume."""
        # NOTE(jdg): tgtadm doesn't use the iscsi_targets table
        # TODO(jdg): In the future move all of the dependent stuff into the
        # cooresponding target admin class

        if isinstance(self.tgtadm, iscsi.LioAdm):
            try:
                volume_info = self.db.volume_get(context, volume['id'])
                (auth_method,
                 auth_user,
                 auth_pass) = volume_info['provider_auth'].split(' ', 3)
                chap_auth = self._iscsi_authentication(auth_method,
                                                       auth_user,
                                                       auth_pass)
            except exception.NotFound:
                LOG.debug(_("volume_info:%s"), volume_info)
                LOG.info(_("Skipping ensure_export. No iscsi_target "
                           "provision for volume: %s"), volume['id'])
                return

            iscsi_name = "%s%s" % (self.configuration.iscsi_target_prefix,
                                   volume['name'])
            volume_path = self._attach_fds_block_dev(volume['name'])
            iscsi_target = 1

            self._create_tgtadm_target(iscsi_name, iscsi_target,
                                       volume_path, chap_auth)

            return

        if not isinstance(self.tgtadm, iscsi.TgtAdm):
            try:
                iscsi_target = self.db.volume_get_iscsi_target_num(
                    context,
                    volume['id'])
            except exception.NotFound:
                LOG.info(_("Skipping ensure_export. No iscsi_target "
                           "provisioned for volume: %s"), volume['id'])
                return
        else:
            iscsi_target = 1  # dummy value when using TgtAdm

        chap_auth = None

        # Check for https://bugs.launchpad.net/cinder/+bug/1065702
        old_name = None
        volume_name = volume['name']
        iscsi_name = "%s%s" % (self.configuration.iscsi_target_prefix,
                               volume_name)
        volume_path = self._attach_fds_block_dev(volume['name'])

        # NOTE(jdg): For TgtAdm case iscsi_name is the ONLY param we need
        # should clean this all up at some point in the future
        self._create_tgtadm_target(iscsi_name, iscsi_target,
                                   volume_path, chap_auth,
                                   lun=0,
                                   check_exit_code=False,
                                   old_name=old_name)

        return

    def _ensure_iscsi_targets(self, context, host):
        """Ensure that target ids have been created in datastore."""
        # NOTE(jdg): tgtadm doesn't use the iscsi_targets table
        # TODO(jdg): In the future move all of the dependent stuff into the
        # cooresponding target admin class
        if not isinstance(self.tgtadm, iscsi.TgtAdm):
            host_iscsi_targets = self.db.iscsi_target_count_by_host(context,
                                                                    host)
            if host_iscsi_targets >= self.configuration.iscsi_num_targets:
                return

            # NOTE(vish): Target ids start at 1, not 0.
            target_end = self.configuration.iscsi_num_targets + 1
            for target_num in xrange(1, target_end):
                target = {'host': host, 'target_num': target_num}
                self.db.iscsi_target_create_safe(context, target)

    def create_export(self, context, volume):
        return self._create_export(context, volume)

    def _create_export(self, context, volume, vg=None):
        iscsi_name = "%s%s" % (self.configuration.iscsi_target_prefix,
                               volume['name'])
        #volume_path = "/dev/%s/%s" % (vg, volume['name'])
        volume_path = self._attach_fds_block_dev(volume['name'])
        model_update = {}

        # TODO(jdg): In the future move all of the dependent stuff into the
        # cooresponding target admin class
        if not isinstance(self.tgtadm, iscsi.TgtAdm):
            lun = 0
            self._ensure_iscsi_targets(context, volume['host'])
            iscsi_target = self.db.volume_allocate_iscsi_target(context,
                                                                volume['id'],
                                                                volume['host'])
        else:
            lun = 1  # For tgtadm the controller is lun 0, dev starts at lun 1
            iscsi_target = 0  # NOTE(jdg): Not used by tgtadm

        # Use the same method to generate the username and the password.
        chap_username = utils.generate_username()
        chap_password = utils.generate_password()
        chap_auth = self._iscsi_authentication('IncomingUser', chap_username,
                                               chap_password)

        tid = self._create_tgtadm_target(iscsi_name, iscsi_target,
                                         volume_path, chap_auth)

        model_update['provider_location'] = self._iscsi_location(
            self.configuration.iscsi_ip_address, tid, iscsi_name, lun)
        model_update['provider_auth'] = self._iscsi_authentication(
            'CHAP', chap_username, chap_password)
        return model_update

    def remove_export(self, context, volume):
        """Removes an export for a logical volume."""
        # NOTE(jdg): tgtadm doesn't use the iscsi_targets table
        # TODO(jdg): In the future move all of the dependent stuff into the
        # cooresponding target admin class

        if isinstance(self.tgtadm, iscsi.LioAdm):
            try:
                iscsi_target = self.db.volume_get_iscsi_target_num(
                    context,
                    volume['id'])
            except exception.NotFound:
                LOG.info(_("Skipping remove_export. No iscsi_target "
                           "provisioned for volume: %s"), volume['id'])
                return

            self.tgtadm.remove_iscsi_target(iscsi_target, 0, volume['id'],
                                            volume['name'])

            return

        elif not isinstance(self.tgtadm, iscsi.TgtAdm):
            try:
                iscsi_target = self.db.volume_get_iscsi_target_num(
                    context,
                    volume['id'])
            except exception.NotFound:
                LOG.info(_("Skipping remove_export. No iscsi_target "
                           "provisioned for volume: %s"), volume['id'])
                return
        else:
            iscsi_target = 0

        try:

            # NOTE: provider_location may be unset if the volume hasn't
            # been exported
            location = volume['provider_location'].split(' ')
            iqn = location[1]

            # ietadm show will exit with an error
            # this export has already been removed
            self.tgtadm.show_target(iscsi_target, iqn=iqn)

        except Exception:
            LOG.info(_("Skipping remove_export. No iscsi_target "
                       "is presently exported for volume: %s"), volume['id'])
            return

        self.tgtadm.remove_iscsi_target(iscsi_target, 0, volume['name_id'],
                                        volume['name'])
        self._detach_fds_block_dev(volume['name'])

    # def migrate_volume(self, ctxt, volume, host, thin=False, mirror_count=0):
    #     """Optimize the migration if the destination is on the same server.
    #
    #     If the specified host is another back-end on the same server, and
    #     the volume is not attached, we can do the migration locally without
    #     going through iSCSI.
    #     """
    #
    #     false_ret = (False, None)
    #     if volume['status'] != 'available':
    #         return false_ret
    #     if 'location_info' not in host['capabilities']:
    #         return false_ret
    #     info = host['capabilities']['location_info']
    #     try:
    #         (dest_type, dest_hostname, dest_vg, lvm_type, lvm_mirrors) =\
    #             info.split(':')
    #         lvm_mirrors = int(lvm_mirrors)
    #     except ValueError:
    #         return false_ret
    #     if (dest_type != 'LVMVolumeDriver' or dest_hostname != self.hostname):
    #         return false_ret
    #
    #     if dest_vg != self.vg.vg_name:
    #         vg_list = volutils.get_all_volume_groups()
    #         vg_dict = \
    #             (vg for vg in vg_list if vg['name'] == dest_vg).next()
    #         if vg_dict is None:
    #             message = ("Destination Volume Group %s does not exist" %
    #                        dest_vg)
    #             LOG.error(_('%s'), message)
    #             return false_ret
    #
    #         helper = 'sudo cinder-rootwrap %s' % CONF.rootwrap_config
    #         dest_vg_ref = lvm.LVM(dest_vg, helper,
    #                               lvm_type=lvm_type,
    #                               executor=self._execute)
    #         self.remove_export(ctxt, volume)
    #         self._create_volume(volume['name'],
    #                             self._sizestr(volume['size']),
    #                             lvm_type,
    #                             lvm_mirrors,
    #                             dest_vg_ref)
    #
    #     volutils.copy_volume(self.local_path(volume),
    #                          self.local_path(volume, vg=dest_vg),
    #                          volume['size'],
    #                          execute=self._execute)
    #     self._delete_volume(volume)
    #     model_update = self._create_export(ctxt, volume, vg=dest_vg)
    #
    #     return (True, model_update)

    def _iscsi_location(self, ip, target, iqn, lun=None):
        return "%s:%s,%s %s %s" % (ip, self.configuration.iscsi_port,
                                   target, iqn, lun)

    def _iscsi_authentication(self, chap, name, password):
        return "%s %s %s" % (chap, name, password)

    # def ensure_export(self, context, volume):
    #     volume_name = volume['name']
    #     iscsi_name = "%s%s" % ("fds-cinder-", volume_name)
    #     volume_path = self._attach_fds_block_dev(volume_name)
    #     model_update = self.target_helper.ensure_export(context, volume, iscsi_name, volume_path)
    #     if model_update:
    #         self.db.volume_update(context, volume['id'], model_update)
    #
    # def create_export(self, context, volume):
    #     volume_path = self._attach_fds_block_dev(volume['name'])
    #     data = self.target_helper.create_export(context, volume, volume_path)
    #     return {
    #         'provider_location': data['location'],
    #         'provider_auth': data['auth'],
    #     }
    #
    # def remove_export(self, context, volume):
    #     self.target_helper.remove_export(context, volume)
    #     self._detach_fds_block_dev(volume['name'])

