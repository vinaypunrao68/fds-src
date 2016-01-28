#!/opt/fds-deps/embedded/bin/python

import argparse
from fdscli.services.fds_auth import *
from fdscli.services.node_service import NodeService
from fdscli.model.platform.node import Node
from fdscli.model.platform.service import Service

def addNodes(args):

    authSession = FdsAuth(args.config)
    print authSession

    node_service = NodeService(authSession)
    nodes = node_service.list_nodes()

    for node in nodes:
        print node.state
        if node.state == 'DISCOVERED':
            node.services['AM'] = [Service(name="AM", a_type="AM")]
            node.services['DM'] = [Service(name="DM", a_type="DM")]
            node.services['SM'] = [Service(name="SM", a_type="SM")]
            node_service.add_node( node.id, node )
    print nodes

    return

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Command line arguments for add_node.py')
    parser.add_argument('--config',
            default='/root/.fdscli.conf',
            dest='config',
            help='location of the fdscli config file')
    args = parser.parse_args()
    addNodes(args)
