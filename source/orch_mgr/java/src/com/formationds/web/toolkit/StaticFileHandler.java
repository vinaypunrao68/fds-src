package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;

import javax.servlet.http.HttpServletRequest;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.HashMap;
import java.util.Map;

public class StaticFileHandler implements RequestHandler {

    public static final Map<String, String> MIME_TYPES;
    private String webDir;

    static {
        String[][] EXTENSIONS = new String[][]{
                {".gif", "image/gif"},
                {".pdf", "application/pdf"},
                {".doc", "application/vnd.ms-word"},
                {".jpg", "image/jpg"},
                {".jpeg", "image/jpeg"},
                {".png", "image/png"},
                {".ico", "image/bmp"},
                {".css", "text/css"},
                {".js", "text/javascript"},
                {".coffee", "text/coffeescript"},
                {".svg", "text/svg"},
                {".txt", "text/plain"},
                {".html", "text/html"},
                {".gz", "application/gzip"}
        };

        MIME_TYPES = new HashMap<>();
        for (String[] mapping : EXTENSIONS) {
            MIME_TYPES.put(mapping[0], mapping[1]);
        }
    }


    public StaticFileHandler(String webDir) {
        this.webDir = webDir;
    }


    @Override
    public Resource handle(Request request) throws FileNotFoundException {
        String resource = request.getRequestURI();
        String mimeType = MIME_TYPES.getOrDefault(getExtension(resource), "application/octet-stream");

        File file = new File(webDir, resource);
        if (!file.exists()) {
            return new FourOhFour();
        }

        return new StreamResource(new FileInputStream(file), mimeType);
    }

    private String getExtension(String resource) {
        int offset = resource.lastIndexOf('.');
        return resource.substring(offset).toLowerCase();
    }
}