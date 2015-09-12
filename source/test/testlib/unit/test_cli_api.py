import unittest
import logging
import os
import sys
from fds.services.node_service import NodeService
from fds.services.fds_auth import FdsAuth
from fds.services.users_service import UsersService
from fds.model.node_state import NodeState
from fds.model.domain import Domain
import pdb


#pdb.set_trace()
fdsauth = FdsAuth()
fdsauth.login()

print fdsauth.get_hostname()
print fdsauth.get_username()
print fdsauth.get_token()
print fdsauth.is_authenticated()

nservice = NodeService(fdsauth)
nodes_list = nservice.list_nodes()
nodeS = NodeState()
nodeS2 = NodeState()

for node in nodes_list:
	print 'node = {}'.format(node)
	print 'node name = {}'.format(node.name)
	print 'node ip_v4_address= {}'.format(node.ip_v4_address)
	print 'node id = {}'.format(node.id)
	print 'node state = {}'.format(node.state)
	print 'node services = {}'.format(node.services)
	#print node.services["OM"][0].auto_name + " " + node.services["OM"][0].status
	print node.services["AM"][0].auto_name + " " + node.services["AM"][0].status + " node.id={}".format(node.id) + " service.id={}".format(node.services["AM"][0].id)
	print node.services["SM"][0].auto_name + " " + node.services["SM"][0].status + " node.id={}".format(node.id) + " service.id={}".format(node.services["SM"][0].id) 
	print node.services["DM"][0].auto_name + " " + node.services["DM"][0].status + " node.id={}".format(node.id) + " service.id={}".format(node.services["DM"][0].id)
	print node.services["PM"][0].auto_name + " " + node.services["PM"][0].status + " node.id={}".format(node.id) + " service.id={}".format(node.services["PM"][0].id)

	print("============================")

	#`print "node.id={}".format(node.services["AM"][0].id)
	#print "node.id={}".format(node.services["SM"][0].id)
	#print "node.id={}".format(node.services["DM"][0].id)
	#print "node.id={}".format(node.services["PM"][0].id)
	#service.py id
	#nservice.deactivate_node(node.id, nodeS)	
	#nodeS2.am=False
	#pdb.set_trace()	
	#nservice.activate_node(node.id, nodeS2)	
	print 'node state am = {}'.format(nodeS2.am)
	print 'node state sm = {}'.format(nodeS2.sm)
	print 'node state dm = {}'.format(nodeS2.dm)

