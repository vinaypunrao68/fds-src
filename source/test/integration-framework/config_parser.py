import config
import ConfigParser
import logging
import os
import sys
 
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
    if inventory_file == None:
        inventory_file = config.DEFAULT_INVENTORY_FILE
    inventory_path = "inventory/" + inventory_file        
    params = parse(fname=inventory_path,
                   file_dir=config.ANSIBLE_ROOT)
    if 'fds_om_host' not in params:
        raise KeyError, "fds_om_host not present in %s" % inventory_file
        sys.exit(1)
    else:
        return params['fds_om_host']