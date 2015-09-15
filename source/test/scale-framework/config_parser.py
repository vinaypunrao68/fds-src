import config
import ConfigParser
import logging
import os
import re
import sys
 
import lib

logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)
    
def parse(fname, file_dir):
    params = {}
    parser = ConfigParser.ConfigParser()
    config_file = os.path.join(file_dir, fname)
    parser.read(config_file)
    sections = parser.sections()
    print sections
    for section in sections:
        options = parser.options(section)
        for option in options:
            try:
                params[option] = parser.get(section, option)
                if params[option] == -1:
                    log.info("skipping: %s" % option)
            except:
                log.info("Exception on %s!" % option)
                params[option] = None
    return params

def get_om_ipaddress_from_inventory(inventory_file):
    '''
    Given an inventory file for ansible, parses that inventory file and finds
    the ip address of the OM node.
    '''
    inventory_path = os.path.join(config.ANSIBLE_INVENTORY,
                                  inventory_file)
    fds_om_host = None
    with open(inventory_path, 'r') as f:
        records = f.readlines()
        for record in records:
            if record.startswith('fds_om_host'):
                fds_om_host = record.strip().split("=")[1]
                break
    if fds_om_host is None:
        raise KeyError, "fds_om_host not present in %s" % inventory_file
        sys.exit(1)
    else:
        if not lib.is_valid_ip(fds_om_host):
            # convert the hostname to ip address
            fds_om_host = lib.hostname_to_ip(fds_om_host)
            if fds_om_host:
                return fds_om_host
            else:
                raise ValueError, "Invalid OM hostname or ip: %s" %fds_om_host
        else:
            return fds_om_host

def get_ips_from_inventory(inventory_file):
    '''
    Parse the given ansible inventory file given as parameter to this
    function, and find all the ip addresses in that cluster.
    
    Arguments:
    ----------
    inventory_file : str
        the name of the ansible inventory file to be parsed.
    
    Returns:
    --------
    list : the list containing all the ip addresses.
    '''
    inventory_path = os.path.join(config.ANSIBLE_INVENTORY,
                                  inventory_file)
    ip_addresses = []
    seen = False
    with open(inventory_path, 'r') as f:
        records = f.readlines()
        for i in xrange(0, len(records)):
            if re.search('^\s*[0-9]', records[i]):
                ip_addresses.append(records[i].strip())
            if records[i].startswith('[') and not seen:
                i += 1
                while not records[i].startswith('['):
                    ip = lib.hostname_to_ip(records[i].strip())
                    if ip != "0.0.0.0":
                        ip_addresses.append(ip)
                    i += 1
                seen = True
                break
    return ip_addresses