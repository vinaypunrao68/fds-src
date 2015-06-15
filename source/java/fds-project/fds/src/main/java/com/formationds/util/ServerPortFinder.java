package com.formationds.util;

import org.apache.log4j.Logger;

import java.io.IOException;
import java.net.ServerSocket;

public class ServerPortFinder {
    private static final Logger LOG = Logger.getLogger(ServerPortFinder.class);

    public int findPort(String description, int startPort) throws IOException {
        for (int i = startPort; i < Short.MAX_VALUE; i++) {
            LOG.debug("Attemping to bind to port " + i + " for ["+ description + "]");
            try {
                ServerSocket serverSocket = new ServerSocket(i);
                serverSocket.close();
                return i;
            } catch (IOException exception) {
            }
        }

        String message = "Unable to find available server port for [" + description + "]";
        LOG.error(message);
        throw new IOException(message);
    }
}
