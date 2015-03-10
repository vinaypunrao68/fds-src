package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Random;

public class ByteBufferUtility {
    public static ByteBuffer randomBytes(int length) {
        byte[] bytes = new byte[length];
        Random random = new Random();
        random.nextBytes(bytes);
        return ByteBuffer.wrap(bytes);
    }

    public static int readIntoByteBuffer(ByteBuffer buffer, InputStream inputStream) throws IOException {
        if(buffer.remaining() == 0)
            return 0;

        if(buffer.hasArray()) {
            byte[] array = buffer.array();
            int offset = buffer.position();
            int length = buffer.remaining();

            int readCount = fillBuffer(inputStream, array, offset, length);

            buffer.position(offset + readCount);
            return readCount == 0 ? -1 : readCount;
        } else {
            byte[] array = new byte[buffer.remaining()];
            int readCount = fillBuffer(inputStream, array, 0, array.length);
            buffer.put(array);
            return readCount == 0 ? -1 : readCount;
        }
    }

    public static void writeFromByteBuffer(ByteBuffer buffer, OutputStream outputStream) throws IOException {
        if(buffer.remaining() == 0)
            return;

        if(buffer.hasArray()) {
            byte[] array = buffer.array();
            outputStream.write(array, buffer.position(), buffer.remaining());
        } else {
            byte[] array = new byte[buffer.remaining()];
            buffer.get(array);
            outputStream.write(array);
        }
    }

    public static void flipRelative(ByteBuffer buffer, int position) {
        buffer.limit(buffer.position());
        buffer.position(position);
    }

    private static int fillBuffer(InputStream inputStream, byte[] array, int offset, int length) throws IOException {
        int readCount = 0;
        while(readCount < length) {
            int read = inputStream.read(array, offset + readCount, length - readCount);
            if(read == -1)
                break;
            readCount += read;
        }
        return readCount;
    }
}
