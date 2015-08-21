package com.formationds.nfs;


import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;

public class BinDiff {
    public static void main(String[] args) throws Exception {
        if (args.length != 3) {
            System.out.println("Usage: bindiff leftFile rightFile blockSize");
            return;
        }

        new BinDiff().execute(args[0], args[1], Integer.parseInt(args[2]));
    }

    public void execute(String left, String right, int blockSize) throws Exception {
        InputStream leftStream = new FileInputStream(left);
        InputStream rightStream = new FileInputStream(right);

        byte[] leftBuf = new byte[blockSize];
        byte[] rightBuf = new byte[blockSize];

        int blockId = 0;

        while (true) {
            int leftRead = leftStream.read(leftBuf);
            int rightRead = rightStream.read(rightBuf);

            if (leftRead == 0 && rightRead == 0) {
                System.out.println("Streams are identical");
                return;
            }

            if (leftRead != rightRead) {
                System.out.println("Streams lengths differ at block " + blockId);
                dump(blockId, leftRead, leftBuf, rightRead, rightBuf);
                return;
            }

            if (!equals(leftBuf, rightBuf, leftRead)) {
                System.out.println("Streams differ at block" + blockId);
                dump(blockId, leftRead, leftBuf, rightRead, rightBuf);
            }

            blockId++;
        }
    }

    private boolean equals(byte[] leftBuf, byte[] rightBuf, int length) {
        for (int i = 0; i < length; i++) {
            if (leftBuf[i] != rightBuf[i]) {
                return false;
            }
        }

        return true;
    }

    private void dump(int blockId, int leftLength, byte[] leftBuf, int rightLength, byte[] rightBuf) throws Exception {
        dumpOne(blockId, "left", leftBuf, leftLength);
        dumpOne(blockId, "right", rightBuf, rightLength);
    }

    private void dumpOne(int blockId, String prefix, byte[] buf, int length) throws Exception {
        FileOutputStream out = new FileOutputStream(prefix + "-" + Long.toString(blockId));
        out.write(buf, 0, length);
        out.flush();
        out.close();
    }
}
