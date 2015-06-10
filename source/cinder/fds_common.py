# Copyright (c) 2014 Formation Data Systems
# All Rights Reserved.


import requests


from thrift.transport.TSocket import TFramedTransport
from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol
from fds_api.am_api.AMSvc import Client as AmClient
from fds_api.config_api.ConfigurationService import Client as CsClient
from fds_api.config_types.ttypes import *
from fds_api.common.ttypes import ApiException

from contextlib import contextmanager

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
                                 VolumeSettings(131072,
                                                VolumeType.BLOCK,
                                                volume['size'] * (1024 ** 3),
                                                0), # cont Commit Log Retention, wtf?
                                                    # ^ this is the duration to keep full
                                                    #   transaction replay logs. You can
                                                    #   create a new snapshot at any time
                                                    #   from now until the end of commit
                                                    #   log retention.
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
        params = {'op': operation, 'volume': volume_name}
        if nbd_server is not None:
            params["host"] = nbd_server
        result = requests.get(url, params=params)
        if result.status_code != 200:
            raise Exception('NBD REST service responded with HTTP status code %d' % result.status_code)
        json_result = result.json()
        if json_result['result'] != 'ok':
            raise Exception('NBD REST operation failed with error code %d: %s' % (json_result['error_code'], json_result['error']))

        return json_result['output'].strip().encode('ascii', 'ignore')

    def detach_nbd_remote(self, url, nbd_server, volume_name):
        self.execute_remote_nbd_operation(url, 'detach', nbd_server, volume_name)

    def detach_nbd_remote_all(self, url, volume_name):
        self.execute_remote_nbd_operation(url, 'detach', None, volume_name)

    def attach_nbd_remote(self, url, nbd_server, volume_name):
        return self.execute_remote_nbd_operation(url, 'attach', nbd_server, volume_name)

    @contextmanager
    def use_nbd_local(self, nbd_server, name):
        yield self.attach_nbd_local(nbd_server, name)
        self.detach_nbd_local(nbd_server, name)

    def set_execute(self, execute):
        self._execute = execute
