#!/usr/bin/python

import os, re, sys, time
import SocketServer

class FdsUDPHandler(SocketServer.BaseRequestHandler):
    """
    This class works similar to the TCP handler class, except that
    self.request consists of a pair of data and client socket, and since
    there is no connection the client address must be given explicitly
    when sending data back via sendto().
    """
    def handle(self):
        data = self.request[0].strip()
        socket = self.request[1]
        os.write(self.server.datafile, "{} wrote:".format(self.client_address[0]) + " clock:" + str(time.time()) + "\n")
        os.write(self.server.datafile, data + "\n------- End of sample ------\n")
        os.fsync(self.server.datafile)
        socket.sendto(data.upper(), self.client_address)

    def terminate(self):
        os.close(self.server.datafile)

if __name__ == "__main__":
    HOST, PORT = "10.1.10.139", 2003
    server = SocketServer.UDPServer((HOST, PORT), FdsUDPHandler)
    fo = open("counters.dat","w")
    fh = fo.fileno()
    server.datafile = fh
    server.serve_forever()
    os.close(fh)



