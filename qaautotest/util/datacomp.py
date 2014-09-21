#!/usr/bin/env python

import os
import sys
import logging
if sys.version_info[0] < 3:
    try:
        import cStringIO as StringIO
    except ImportError:
        import StringIO
else:
    import io as StringIO


def main():
    ##################################################
    # Positive tests
    ##################################################
    # Create test files
    starting_fd = fd_test()
    fp_data = "abcd" * 1024
    file1 = open("fp_file1", "w")
    file1.write(fp_data)
    file1.close()
    file2 = open("fp_file2", "w")
    file2.write(fp_data)
    file2.close()
    
    # String
    DataComp("aaaaa", "aaaaa")

    # File path (string)
    DataComp("fp_file1", "fp_file2")

    # File object
    file1 = open("fp_file1", "r")
    file2 = open("fp_file2", "r")
    DataComp(file1, file2)
    file1.close()
    file2.close()

    # File descriptor
    fd1 = os.open("fp_file1", os.O_RDONLY)
    fd2 = os.open("fp_file2", os.O_RDONLY)
    DataComp(fd1, fd2)
    os.close(fd1)
    os.close(fd2)
    
    ##################################################
    # Negative tests
    ##################################################
    # Modify target test file
    fp_data_new = "x" + fp_data
    file2 = open("fp_file2", "w")
    file2.write(fp_data_new)
    file2.close()
    
    # Strings
    DataComp("abcd" * 765, "abcd" * 27389, False)
    DataComp("aaaaa", "aabaa", False)
    DataComp("d" * 32768, "d" * 32768 + "d", False)
    DataComp("d" * 32768, ("d" * 32768)[:-1], False)
    DataComp("d" * 65536, "d" * 65536 + "d", False)
    DataComp("d" * 65536, ("d" * 65536)[:-1], False)
    DataComp("d" * 98304, "d" * 98304 + "d", False)
    DataComp("d" * 98304, ("d" * 98304)[:-1], False)
    DataComp(((32768 * 128) - 8192) * "x", 32768 * 128 * "x", False)
    DataComp("x" * 1024 * 1024 * 256, "x" * 1024 * 1024 * 256 + "yyy", False)
    DataComp("x" * 1024 * 1024 * 256 + "yyy", "x" * 1024 * 1024 * 256, False)
    DataComp(fp_data, fp_data_new, False)
    DataComp(fp_data, fp_data + "x", False)
    DataComp("x" * 1024 * 99, "aaaaab", False)
    DataComp("aaaaab", "x" * 1024 * 99, False)
    DataComp("q" * 32768, "q" * 32768 * 4, False)
    DataComp("q" * 32768, "", False)
    
    # File path (string)
    DataComp("fp_file1", "fp_file2", False)
    
    # File object
    file1 = open("fp_file1", "r")
    file2 = open("fp_file2", "r")
    DataComp(file1, file2, False)
    file1.close()
    file2.close()
    
    # File descriptor
    fd1 = os.open("fp_file1", os.O_RDONLY)
    fd2 = os.open("fp_file2", os.O_RDONLY)
    DataComp(fd1, fd2, False)
    os.close(fd1)
    os.close(fd2)
    
    # Invalid Types
    try:
        DataComp(["a"], ["a"])
    except InvalidType:
        print("Received the correct InvalidType exception.")
        
    file1 = open("fp_file1", "a")
    file2 = open("fp_file2", "a")
    try:
        DataComp(file1, file2)
    except NoReadAccess:
        print("Received the correct NoReadAccess exception.")
    file1.close()
    file2.close()
    
    ending_fd = fd_test()
    if starting_fd != ending_fd:
        print("Leaked %s file descriptors during the unit test."
              %(ending_fd - starting_fd))
        
    os.remove("fp_file1")
    os.remove("fp_file2")
    os.remove("fp_file0")
    
def fd_test():
    """ A simple way to check for file descriptor leaks.  Find the number of
    the first open file descriptor at the start of the test.  If it's the same
    at the end of the test you haven't leaked any descriptors."""
    fd = os.open("fp_file0", os.O_CREAT|os.O_WRONLY)
    os.close(fd)
    return fd
    

class DataComp(object):
    """ Data comparison class.  Takes two files or strings and compares them.
    If a data mismatch is found, the offset into the file/string and a hex dump
    around the corruption is displayed.
    """
    def __new__(cls, source, target, assert_on_fail=True, log_level=None):
        obj = object.__new__(cls)
        obj.__init__(source, target, assert_on_fail, log_level)
        return obj.verify()
    
    def __init__(self, source, target, assert_on_fail=True, log_level=None):
        """
        @type source: string, int, or file
        @param source: Any of the following data types:
        * string buffer: A raw string to be compared
        * file path (string): The path to a file that should be opened for
        comparison.
        * file descriptor (int): An open file descriptor for the file that
        should be compared
        * file object: An open file object for that file that should be
        compared.
        
        @type target: string, int, or file
        @param target: Any of the following data types:
        * string buffer: A raw string to be compared
        * file path (string): The path to a file that should be opened for
        comparison.
        * file descriptor (int): An open file descriptor for the file that
        should be compared
        * file object: An open file object for that file that should be
        compared.
        
        @type assert_on_fail: bool
        @param assert_on_fail: Indicates whether an AssertionError should be
        raised if the data comparison fails.
        
        @type log_level: log.setLevel
        @param log_level: Set the logging level to something explicit.  If 
        log_level is set to None then the logger is set to the same level as
        the root logger (if defined), or logging.DEBUG.
        
        Note: When passing in a file descriptor or file object, the file must
        be opened in read mode or a NoReadAccess exception will be raised.
        Where a file path is provided, the EUID/EGID must have permission to
        read the file.
        """
        self.assert_on_fail = assert_on_fail
        self.read_buffer = 32768 # bytes; works well for NFS
        self.display_bytes = 20
        self.log = logging.getLogger("DataComp")
        if log_level == None:
            root_logger = logging.getLogger("")
            if root_logger.getEffectiveLevel() > logging.DEBUG:
                self.log.setLevel(root_logger.getEffectiveLevel())
            else:
                self.log.setLevel(logging.DEBUG)
        else:
            self.log.setLevel(log_level)
        self.close_src = True
        self.close_tgt = True
        self.source_fd = 0 # made non-0 if source is a file descriptor
        self.target_fd = 0 # made non-0 if target is a file descriptor
        
        if type(source) == file:
            self.src = source
            self.close_src = False
        elif type(source) == int:
            self.source_fd = os.dup(source)
            self.src = os.fdopen(self.source_fd, "rb")
        elif type(source) == str:
            try:
                if os.path.exists(source):
                    self.src = open(source, "rb")
                else:
                    self.src = StringIO.StringIO(source)
            except:
                self.src = StringIO.StringIO(source)
        else:
            raise InvalidType("source")
        
        if type(target) == file:
            self.tgt = target
            self.close_tgt = False
        elif type(target) == int:
            target_fd = os.dup(target)
            self.tgt = os.fdopen(target_fd, "rb")
        elif type(target) == str:
            try:
                if os.path.exists(target):
                    self.tgt = open(target, "rb")
                else:
                    self.tgt = StringIO.StringIO(target)
            except:
                self.tgt = StringIO.StringIO(target)
        else:
            raise InvalidType("target")
        
        try:
            self.src.read(1)
        except IOError:
            raise NoReadAccess("source")
        else:
            self.src.seek(0, 0)
        try:
            self.tgt.read(1)
        except IOError:
            raise NoReadAccess("target")
        else:
            self.tgt.seek(0, 0)
        
    def verify(self):
        identical = True
        src_buf = ""
        tgt_buf = ""
        self.src.seek(0, 2)
        source_bytes = self.src.tell()
        self.tgt.seek(0, 2)
        target_bytes = self.tgt.tell()
        if source_bytes != target_bytes:
            identical = False
            self.log.critical("Sizes are different.\nSource size: %s bytes.\n"
                              "Target size: %s bytes."
                              %(source_bytes, target_bytes))
        
        self.src.seek(0, 0)
        self.tgt.seek(0, 0)
        data_index = count = 0
        while True:
            # src_buf and tgt_buf should have 1X read_buffer of good data at 
            # the front (provided the corruption isn't in the first read) + 1X
            # read_buffer where the corruption can be found, + 1/2 of
            # display_bytes at the end to ensure we can show context around
            # the corruption.  The actual display should be trimmed at display
            # time.
            src_read = self.src.read(self.read_buffer)
            tgt_read = self.tgt.read(self.read_buffer)
            if (src_read == "") and (tgt_read == ""):
                break # we've read to the end of the file
            elif src_read == tgt_read:
                if count == 0:
                    src_buf = src_read
                    tgt_buf = tgt_read
                else:
                    if count > 1:
                        data_index += len(src_read)
                    src_buf = src_buf[(-1 * len(src_read)):] + src_read
                    tgt_buf = tgt_buf[(-1 * len(src_read)):] + tgt_read
                count += 1
                continue # the data is identical; continue verifying contents
            else:
                src_buf += (src_read + self.src.read(self.display_bytes / 2))
                tgt_buf += (tgt_read + self.tgt.read(self.display_bytes / 2))
                self.display_mismatch(src_buf, tgt_buf, data_index)
                identical = False
                if self.assert_on_fail:
                    raise AssertionError("Data mismatch detected!")
                break
            
        if identical:
            if self.close_src:
                self.src.close()
            if self.close_tgt:
                self.tgt.close()
            self.log.debug("Contents verified successfully.")
            return True
        else:
            return False
        
    def display_mismatch(self, src_data, tgt_data, offset):
        """ Takes two strings (which should be different) and displays the
        mismatch between them.
        
        offset:  How many bytes into the file/string src_data and tgt_data
        begin, i.e. how many bytes into the file/string does src_data[0] start.
        """
        # Find out the starting byte at which the corruption begins
        if src_data == tgt_data:
            self.log.warning("No data mismatch to display.")
            return
        for i in range(self.read_buffer * 3):
            try:
                if src_data[i] == tgt_data[i]:
                    continue
            except IndexError:
                pass # will hit this if the read buffers are of unequal size
            corruption_offset = offset + i + 1 #+1 because index starts at 0
            if i < self.display_bytes:
                drop_prefix = 0
                drop_suffix = self.display_bytes
            else:
                drop_prefix = i - (self.display_bytes / 2)
                drop_suffix = i + (self.display_bytes / 2)
            if len(src_data) > self.display_bytes:
                src_data = src_data[drop_prefix:drop_suffix]
            if len(tgt_data) > self.display_bytes:
                tgt_data = tgt_data[drop_prefix:drop_suffix]
            
            # Convert the data into hex as we may be comparing binary data
            src_hex_data = ""
            for i in src_data:
                src_hex_data += "%02x " %ord(i)
            tgt_hex_data = ""
            for i in tgt_data:
                tgt_hex_data += "%02x " %ord(i)
            
            for i in range(self.display_bytes):
                self.log.critical("Data mismatch error starting at byte "
                                  "offset %s" %corruption_offset)
                try:
                    if self.src.name != "<fdopen>":
                        self.log.critical("Source file: %s" %self.src.name)
                except AttributeError: # StringIO don't have names
                    pass
                try:
                    if self.tgt.name != "<fdopen>":
                        self.log.critical("Target file: %s" %self.tgt.name)
                except AttributeError: # StringIO don't have names
                    pass
                self.log.critical("\nExpected: %s"
                                  "\n  Actual: %s\n"
                                  %(src_hex_data, tgt_hex_data))
                return
    
    
class DataCompException(Exception):
    """ All exceptions thrown by the DataComp class are of this type
    or subclassed from it.
    """
    
class InvalidType(DataCompException):
    """ The error you get if one if the items passed in for comparison cannot 
    be read.
    """
    def __init__(self, var):
        """ The name of the variable that """
        self.var = var
    def __str__(self):
        return "InvalidType exception:\n" +\
               "%s is not a string, file descriptor, or file object." %self.var
    
class NoReadAccess(DataCompException):
    """ The error you get if one if the items passed in for comparison cannot 
    be read.
    """
    def __init__(self, var):
        """ The name of the variable that """
        self.var = var
    def __str__(self):
        return "InvalidType exception:\n" +\
               "Cannot open %s for read access." %self.var
    

if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG,
                        format='%(asctime)s %(name)s %(levelname)s %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')
    main()
