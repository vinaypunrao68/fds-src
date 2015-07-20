from model.platform.address import Address
import json
class AddressConverter(object):
    '''
    Created on Jun 5, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(address):
        
#         if not isinstance(address, Address):
#             raise TypeError()
        
        j_address = dict()
        
        j_address["ipv4Address"] = address.ipv4address
        j_address["ipv6Address"] = address.ipv6address
        
        j_address = json.dumps(j_address)
        
        return j_address
    
    @staticmethod
    def build_address_from_json(j_address):
        
        if not isinstance(j_address, dict):
            j_address = json.loads(j_address)
            
        address = Address()
        
        address.ipv4address = j_address.pop("ipv4Address", address.ipv4address)
        address.ipv6address = j_address.pop("ipv6Address", address.ipv6address)
        
        return address
