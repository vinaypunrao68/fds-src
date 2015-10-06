"""
:author: Brian Madden
:email: brian@formationds.com
"""
import collections
import struct

class DLT(object):
    """
    Represents a DLT structure. Currently can deserialize a DLT structure based
    on the serialize format as of Apr. 28 2015.
    """

    @staticmethod
    def load_dlt(dlt_file):
        """
        Reads a DLT file from disk, deserializes, and returns a python version of the structure.
        :param dlt_file: Path to the DLT file to read/deserialize
        :return: A DLT structure, which is a list of lists, where each inner list index represents a DLT token, and each
        value is a NodeUUID for a node responsible for that token.
        """

        # Open the DLT file
        try:
            fh = open(dlt_file, 'rb')
        except IOError as e:
            return None

        header = struct.unpack('>QQ3i', fh.read(28))
        version = header[0]
        timestamp = header[1]
        numBitsForToken = header[2]
        depth = header[3]
        numTokens = header[4]

        # Unpack the UUIDs
        node_uuid_list = []
        # The the number of UUIDs to unpack
        node_list_size = struct.unpack('>I', fh.read(4))[0]
        for i in xrange(node_list_size):
            node_uuid_list.append(struct.unpack('>Q', fh.read(8))[0])

        dlt_list = []
        f_byte = (node_list_size <= 256)
        for i in xrange(numTokens):
            dlt_group = [-1] * depth
            for j in xrange(depth):
                if f_byte:
                    idx = struct.unpack('>b', fh.read(1))[0]
                    dlt_group[j] = node_uuid_list[idx]
                else:
                    idx = struct.unpack('>h', fh.read(2))[0]
                    dlt_group[j] = node_uuid_list[idx]

            dlt_list.append(dlt_group)

        return dlt_list


    @staticmethod
    def transpose_dlt(dlt):
        """
        Transpose a DLT from the list of lists to a dict in the form:
        {
           'nodeUUID': [tokenID1, tokenID2, ...],
        }
        :param dlt: The DLT to transpose
        :return: The transposed DLT
        """
        dlt_dict = collections.defaultdict(list)
        # Enumerate over all of the token groups
        for count, group in enumerate(dlt):
            for node in group:
                dlt_dict[node].append(count)

        return dlt_dict
