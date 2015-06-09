package com.formationds.xdi;

import org.junit.Test;

import java.util.Iterator;
import java.util.Random;

import static org.junit.Assert.*;

public class FdsObjectFrameTest {
    public void testFramingScenario(long offset, long readLength, int objectSize) throws Exception {
        FdsObjectFrame firstFrame = FdsObjectFrame.firstFrame(offset, readLength, objectSize);
        assertEquals(Math.min(objectSize - firstFrame.internalOffset, readLength), firstFrame.internalLength);
        assertEquals(offset % objectSize, firstFrame.internalOffset);
        assertEquals(offset / objectSize, firstFrame.objectOffset);

        long totalRead = firstFrame.internalLength;
        long previousOffset = firstFrame.objectOffset;

        FdsObjectFrame frame = firstFrame.nextFrame();
        while(frame != null && frame.nextFrame() != null) {
            totalRead += frame.internalLength;
            assertEquals(objectSize, frame.internalLength);
            assertEquals(0, frame.internalOffset);
            assertEquals(previousOffset + 1, frame.objectOffset);

            previousOffset = frame.objectOffset;
            frame = frame.nextFrame();
        }

        FdsObjectFrame lastFrame = frame;

        if(lastFrame != null) {
            assertEquals(0, frame.internalOffset);
            assertEquals(readLength - totalRead, frame.internalLength);
            assertEquals(previousOffset + 1, frame.objectOffset);
            totalRead += frame.internalLength;
        }

        assertEquals(totalRead, readLength);
    }

    public void testIterator(long offset, long readLength, int objectSize) throws Exception {
        long totalRead = 0;
        for(FdsObjectFrame frame : FdsObjectFrame.frames(offset, readLength, objectSize)) {
            totalRead += frame.internalLength;
        }
        assertEquals(readLength, totalRead);
    }

    public final int MEG = 1024 * 1024;
    public final int GIG = MEG * 1024;

    @Test
    public void testFraming() throws Exception{
        testFramingScenario(0, 4096, 4096);
        testFramingScenario(0, 16384, 4096);
        testFramingScenario(0, 1024, 4096);
        testFramingScenario(1024, 4096, 4096);
        testFramingScenario(1024, 16384, 4096);

        testFramingScenario(0, 2 * MEG, 2 * MEG);
        testFramingScenario(1630, 4L * GIG, 2 * MEG);
    }

    @Test
    public void testIteration() throws Exception {
        testIterator(0, 4096, 4096);
        testIterator(0, 16384, 4096);
        testIterator(0, 1024, 4096);
        testIterator(1024, 4096, 4096);
        testIterator(1024, 16384, 4096);

        testIterator(0, 2 * MEG, 2 * MEG);
        testIterator(1630, 4L * GIG, 2 * MEG);
    }
}