from model.fds_id import FdsId
import json

class FdsIdConverter(object):
    '''
    Created on May 29, 2015
    
    @author: nate
    '''

    @staticmethod
    def to_json(an_id):
        
#         if an_id is not None and not isinstance(an_id, FdsId):
#             raise TypeError()
        
        id_dict = dict()
        
        id_dict["uuid"] = an_id.uuid
        id_dict["name"] = an_id.name
        
        id_dict = json.dumps( id_dict )
        
        return id_dict
        
    @staticmethod
    def build_id_from_json( j_str ):
        
        if not isinstance( j_str, dict ):
            j_str = json.loads( j_str )
        
        fds_id = FdsId()
        
        fds_id.uuid = j_str.pop( "uuid", -1 )
        fds_id.name = j_str.pop( "name", "UNKNOWN" )
        
        return fds_id
