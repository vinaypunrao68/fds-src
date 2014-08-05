#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
import os
import sys
import argparse
import subprocess
import pdb
import time
import random
import hashlib
import optparse
import fdslib.BringUpCfg as fdscfg
from multiprocessing import Process, Array

class RandBytes():
    def __init__(self, srand):
        """Per object random bytes generator."""
        self.rb_seed = srand
        self.rb_gen  = random.Random(srand)

    def __get_random_bytes_iter(self, bytes_cnt):
        for b in xrange(bytes_cnt):
            yield self.rb_gen.getrandbits(8)

    def get_rand_bytes(self, bytes_cnt):
        """Generate a byte array of random bytes."""
        return bytearray(self.__get_random_bytes_iter(bytes_cnt))


class BlockGen():
    def __init__(self,
                 name,
                 type_is_dup,
                 chunk_size=4096,
                 min_chunks=1,
                 max_chunks=1,
                 rand_seed=None):
        """Initialize new BlockGen object to generate data base on arguments."""
        assert(isinstance(name, str))
        assert(isinstance(chunk_size, int))
        assert(isinstance(min_chunks, int))
        assert(isinstance(max_chunks, int))
        assert(chunk_size > 0)
        assert(min_chunks <= max_chunks)

        assert isinstance(type_is_dup, bool)
        self.bg_block_name  = name
        self.bg_type_is_dup = type_is_dup
        self.bg_chunk_size  = chunk_size
        self.bg_min_chunks  = min_chunks
        self.bg_max_chunks  = max_chunks
        assert isinstance(rand_seed, long) or rand_seed is None
        self.bg_rand_seed   = rand_seed
        self.bg_rand_bytes  = RandBytes(rand_seed)

        # last data chunk/block
        self.bg_last_block  = None
        self.bg_last_chunk  = None
        self.bg_last_chcnt  = 0

        self.bg_blocks_generated = 0

    def get_block(self, debug=False):
        """Generate a new data block, return (data, size)."""
        if self.bg_type_is_dup is True:
            block = self.__get_dup_block()
        else:
            block = self.__get_rand_block()

        if debug is True:
            # print hash and size of block
            m = hashlib.sha256()
            m.update(block)
            print "Digest size : ", m.digest_size
            print "Hex digest  : ", m.hexdigest()
            print "Data size   : ", len(block)

        assert(isinstance(block, bytearray))
        self.bg_blocks_generated += 1
        return block

    def __get_dup_block(self):
        if (self.bg_last_chunk == None):
            self.bg_last_chunk = self.bg_rand_bytes.get_rand_bytes(self.bg_chunk_size)
            assert (len(self.bg_last_chunk) == self.bg_chunk_size)

        chunks_cnt = random.randint(self.bg_min_chunks, self.bg_max_chunks)

        # TODO: allocate byte array with size, then copy the data in, instead of extend.
        block = bytearray()
        for i in xrange(0, chunks_cnt):
            block.extend(self.bg_last_chunk)
        self.bg_last_chcnt = chunks_cnt
        self.bg_last_block = block
        return block

    def __get_rand_block(self):
        block = bytearray()
        chunks_cnt = random.randint(self.bg_min_chunks, self.bg_max_chunks)
        for i in xrange(0, chunks_cnt):
            self.bg_last_chunk = self.bg_rand_bytes.get_rand_bytes(self.bg_chunk_size)
            assert (len(self.bg_last_chunk) == self.bg_chunk_size)
            block.extend(self.bg_last_chunk)
        self.bg_last_chcnt = chunks_cnt
        self.bg_last_block = block
        return block

class DataGen():
    def __init__(self,
                 file_size,
                 dup_ratio,
                 sel_policy,
                 random_blocks,
                 dup_blocks,
                 debug=False):
        """Initialize new DataGen object to feed data to a feeder."""
        assert(isinstance(file_size, int) and file_size > 0)
        assert(isinstance(dup_ratio, int) and dup_ratio >= 0)
        assert(isinstance(sel_policy, str))
        sel_policy = sel_policy.lower()
        assert(sel_policy == "fifo" or sel_policy == "random")
        assert(isinstance(random_blocks, list))
        assert(isinstance(dup_blocks, list))
        assert(len(random_blocks) > 0 or len(dup_blocks) > 0)
        self.dg_file_size       = file_size
        self.dg_dup_ratio       = dup_ratio
        self.dg_sel_policy      = sel_policy
        self.dg_random_blocks   = random_blocks
        self.dg_dup_blocks      = dup_blocks
        self.dg_rand_list       = True
        self.dg_debug           = debug
        self.dg_dup_bytes       = 0
        self.dg_rand_bytes      = 0

    def generate_data(self):
        """Generate a new data set and feed it to the feeder."""
        cur_wr_off = 0
        total_size = self.dg_file_size
        res_block  = bytearray(total_size)
        while cur_wr_off < total_size:
            # generate the data.
            blockgen = self.__get_next_blockgen()
            block    = blockgen.get_block()
            if len(block) > total_size - cur_wr_off:
                block_len = total_size - cur_wr_off
            else:
                block_len = len(block)
            res_block[cur_wr_off:(cur_wr_off + block_len)] = block[:block_len]
            cur_wr_off += block_len

            if blockgen.bg_type_is_dup is True:
                self.dg_dup_bytes += block_len
            else:
                self.dg_rand_bytes += block_len
            assert(cur_wr_off <= total_size)
            # TODO: keep track of % of dup/random completed, then finish the data set with the rest of non-completed.

        assert(cur_wr_off == total_size)

        if self.dg_debug is True:
            # print hash and size of block
            m = hashlib.sha256()
            m.update(res_block)
            print "Digest size : ", m.digest_size
            print "Hex digest  : ", m.hexdigest()
            print "Data size   : ", len(res_block)
            print "Dup bytes   : ", self.dg_dup_bytes
            print "Rand bytes  : ", self.dg_rand_bytes
            print "Dedup ratio : ", float(self.dg_dup_bytes) / float(total_size)

        assert(isinstance(res_block, bytearray))
        return res_block

    def __get_next_blockgen(self):
        if self.dg_sel_policy == "fifo":
            return self.__get_next_blockgen_fifo()
        else:
            assert(self.dg_sel_policy == "random")
            return self.__get_next_blockgen_random()


    def __get_next_blockgen_fifo(self):
        rand = self.dg_rand_list
        self.dg_rand_list = not rand
        if rand is True:
            if len(self.dg_random_blocks) == 0:
                return self.__get_next_blockgen()
            block = self.dg_random_blocks.pop(0)
            self.dg_random_blocks.append(block)
        else:
            if len(self.dg_dup_blocks) == 0:
                return self.__get_next_blockgen()
            block = self.dg_dup_blocks.pop(0)
            self.dg_dup_blocks.append(block)

        assert(isinstance(block, BlockGen))
        return block

    def __get_next_blockgen_random(self):
        # TODO: Add random select policy
        assert(False and "Feature not implemented")

def __test_bg():
    print "Unit test for Block Generator"
    bg = BlockGen(True, min_chunks=1, max_chunks=2)
    barray = bg.get_block(debug=True)

    bg = BlockGen(False, min_chunks=1, max_chunks=2)
    barray = bg.get_block(debug=True)

def __test_dg():
    print "Unit test for DataGen"
    rand_list  = [BlockGen(True, min_chunks=1, max_chunks=2)]
    dup_list   = [BlockGen(True, min_chunks=1, max_chunks=2)]
    file_sizes = [4096, 8192, 1048576, 1232343, 1231232, 3355, 1243456, 66432]
    for fs in file_sizes:
        dg = DataGen(fs,
                     50,
                     "fifo",
                     rand_list,
                     dup_list,
                     debug=True)

        data = dg.generate_data()
        assert(len(data) == fs)

def __run_ut():
    __test_bg()
    __test_dg()
    sys.exit(0)

def create_blockgens_from_cfg(block_names, block_cfg):
    blockgen_list = []
    for blk_name in block_names:
        cfg_blk  = filter(lambda blk: blk.nd_conf_dict['io-block-name'] == blk_name, block_cfg)
        assert(len(cfg_blk) == 1)
        cfg_blk  = cfg_blk[0]
        if cfg_blk.nd_conf_dict['is_dup'].lower() == 'true':
            is_dup = True
        else:
            is_dup = False
        blockgen = BlockGen(cfg_blk.nd_conf_dict['io-block-name'],
                            is_dup,
                            int(cfg_blk.nd_conf_dict['chunk_size']),
                            int(cfg_blk.nd_conf_dict['min_chunks']),
                            int(cfg_blk.nd_conf_dict['max_chunks']),
                            long(cfg_blk.nd_conf_dict['rand_seed']))
        blockgen_list.append(blockgen)
    return blockgen_list

if __name__ == "__main__":
    parser = optparse.OptionParser("usage: %prog [options]")
    parser.add_option('-f', '--file', dest='config_file',
                      help='configuration file (e.g. cfg/datagen_io.cfg)', metavar='FILE')
    parser.add_option('-v', '--verbose', action='store_true', dest='verbose',
                      help='enable verbosity')
    parser.add_option('-r', '--dryrun', action='store_true', dest='dryrun',
                      help='dry run, print commands only')
    parser.add_option('-U', '--unittest', action='store_true', dest='unittest',
                      help='Run the unit test and exit.')
    # TODO: fds-root is not needed for datagen, only for the FdsConfigRun environment
    #       Fix this dependency.
    parser.add_option('-R', '--fds-root', dest='fds_root', default='/fds',
                      help='specify fds-root directory')

    (options, args) = parser.parse_args()

    if options.config_file is None:
        print "You need to pass config file"
        sys.exit(1)

    if options.unittest is True:
        __run_ut()

    cfg = fdscfg.FdsConfigRun(None, options)

    # Get all the configuration
    cfg_blockgens = cfg.rt_get_obj('cfg_io_blocks')
    cfg_datagen   = cfg.rt_get_obj('cfg_datagen')

    # TODO: currently support only 1 datagen instance
    #       We can do better by having multiple datagen instances, running async.
    assert(len(cfg_datagen) == 1)

    for dg in cfg_datagen:
        # create list of rand blocks
        rand_blocks = create_blockgens_from_cfg(dg.nd_conf_dict['rand_blocks'], cfg_blockgens)

        # create list of dup blocks
        dup_blocks = create_blockgens_from_cfg(dg.nd_conf_dict['dup_blocks'], cfg_blockgens)

        datagen = DataGen(int(dg.nd_conf_dict['file_size']),
                          int(dg.nd_conf_dict['dup_ratio']),
                          dg.nd_conf_dict['sel_policy'],
                          rand_blocks,
                          dup_blocks,
                          options.verbose)
        datagen.generate_data()
