#!/usr/bin/python
#
# Copyright 2014 by Formation Data Systems, Inc.
#
import os
import sys
import argparse
import subprocess
from multiprocessing import Process, Array
import pdb
import time
import random
import hashlib

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
                 type_is_dup,
                 chunk_size=4096,
                 min_chunks=1,
                 max_chunks=1,
                 rand_seed=None):
        """Initialize new BlockGen object to generate data base on arguments."""
        assert(chunk_size > 0)
        assert(min_chunks <= max_chunks)

        assert isinstance(type_is_dup, bool)
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
            print "Block size  : ", m.block_size
            print "Hex digest  : ", m.hexdigest()

        return block

    def __get_dup_block(self):
        if (self.bg_last_chunk == None):
            self.bg_last_chunk = self.bg_rand_bytes.get_rand_bytes(self.bg_chunk_size)
            assert (len(self.bg_last_chunk) == self.bg_chunk_size)

        chunks_cnt = random.randint(self.bg_min_chunks, self.bg_max_chunks)

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
                 block_strategy,
                 random_blocks,
                 dup_blocks,
                 feeder):
        """Initialize new DataGen object to feed data to a feeder."""

    def generate_data(self):
        """Generate a new data set and feed it to the feeder."""

def test_bg():
    bg = BlockGen(True, min_chunks=1, max_chunks=2)
    barray = bg.get_block(debug=True)
    print len(barray)


    bg = BlockGen(False, min_chunks=1, max_chunks=2)
    barray = bg.get_block(debug=True)
    print len(barray)

def run_test(file_path):
    fp = open(file_path, 'w')
    dg = DataGen(4096,
                 50,
                 1,
                 None,
                 None,
                 fp)

    dg.generate_data()

if __name__ == "__main__":
    test_bg()
    run_test("/tmp/foobar")
