package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.Iterator;

public class FdsObjectFrame {
    public long objectOffset;
    public int internalOffset;
    public int internalLength;

    private long readLength;
    private long readOffset;
    private int objectSize;

    public static FdsObjectFrame firstFrame(long readOffset, long readLength, int objectSize) {
        FdsObjectFrame seed = new FdsObjectFrame();
        seed.readLength = readLength;
        seed.readOffset = readOffset;
        seed.objectSize = objectSize;
        return nextFrame(seed);
    }

    public FdsObjectFrame entireObject() {
        return firstFrame(objectSize * objectOffset, objectSize, objectSize);
    }

    public FdsObjectFrame nextFrame() {
        return nextFrame(this);
    }

    public static FdsObjectFrame nextFrame(FdsObjectFrame frame) {
        if(frame == null || frame.readLength == 0)
            return null;

        FdsObjectFrame next = new FdsObjectFrame();
        next.objectSize = frame.objectSize;
        next.objectOffset = frame.readOffset / frame.objectSize;
        next.internalOffset = (int) (frame.readOffset % frame.objectSize);
        next.internalLength = (int) Math.min(frame.readLength, frame.objectSize - next.internalOffset);
        next.readLength = frame.readLength - next.internalLength;
        next.readOffset = frame.readOffset + next.internalLength;
        return next;
    }

    public static Iterable<FdsObjectFrame> frames(long readOffset, long readLength, int objectSize) {
        return new Iterable<FdsObjectFrame>() {
            @Override
            public Iterator<FdsObjectFrame> iterator() {
                return new FrameIterator(readOffset, readLength, objectSize);
            }
        };
    }

    public static class FrameIterator implements Iterator<FdsObjectFrame> {
        private FdsObjectFrame next;

        public FrameIterator(long readOffset, long readLength, int objectSize) {
            next = firstFrame(readOffset, readLength, objectSize);
        }

        @Override
        public boolean hasNext() {
            return next != null;
        }

        @Override
        public FdsObjectFrame next() {
            FdsObjectFrame current = next;
            next = nextFrame(current);
            return current;
        }
    }
}


