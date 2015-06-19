package com.formationds.commons.util;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class IoStreamUtil {
    public static byte[] buffer(InputStream stream) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        byte[] buf = new byte[4096];
        while(true) {
            int read = stream.read(buf);
            if(read == -1)
                break;
            baos.write(buf, 0, read);
        }

        return baos.toByteArray();
    }
}
