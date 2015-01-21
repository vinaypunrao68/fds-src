#!/usr/bin/python
import socket
import unittest


def get_ip(hostname):
    return socket.gethostbyname(hostname)


def get_my_ip():
    return get_ip('localhost')


class TestSequenceFunctions(unittest.TestCase):
    def test_get_ip(self):
        self.assertEqual(get_ip('luke'),'10.1.10.222')

if __name__ == '__main__':
    unittest.main()
    
