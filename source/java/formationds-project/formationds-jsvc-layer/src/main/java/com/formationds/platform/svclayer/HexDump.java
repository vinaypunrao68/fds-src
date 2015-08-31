/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;

/**
 * Util for printing hex dumps of binary data in format similar to *nix hexdump and od tools.
 */
// TODO: if this ends up being used heavily, it may be worth refactoring to a class and provide
// config options and a builder.  Also, may move to generic util package.
public class HexDump {

    // prevent instantiation
    private HexDump() {}

    public static final int DEFAULT_WIDTH = 16;

    /**
     * @param bytes the byte array to encode
     *
     * @return the byte array encoded as a hex string
     */
    public static String encodeHex( byte[] bytes ) {
        StringWriter sw = new StringWriter( bytes.length * 2 );
        try ( PrintWriter out = new PrintWriter( sw ) ) {
            writeHex( out, bytes, 0, bytes.length );
        }
        return sw.toString();
    }

    /**
     * @param bytes the byte array to encode
     *
     * @return the bytes encoded as hex and ascii with each line using {@link #DEFAULT_WIDTH}
     */
    public static String encodeHexAndAscii( byte[] bytes ) {
        StringWriter sw = new StringWriter( bytes.length * 2 );
        try ( PrintWriter out = new PrintWriter( sw ) ) {
            writeHexAndAscii( out, bytes, 0, DEFAULT_WIDTH );
        }
        return sw.toString();
    }

    /**
     * Print the bytes encoded as hex and ascii with each line using {@link #DEFAULT_WIDTH} to
     * System.out
     *
     * @param bytes the byte array to encode
     */
    public static void printHexAndAscii( byte[] bytes ) {
        printHexAndAscii( System.out, bytes, 0, DEFAULT_WIDTH );
    }

    /**
     * Print the bytes encoded as hex and ascii with each line using {@link #DEFAULT_WIDTH} to
     * the specified output
     *
     * @param out   the output
     * @param bytes the byte array to encode
     */
    public static void printHexAndAscii( OutputStream out, byte[] bytes ) {
        printHexAndAscii( new PrintWriter( out ), bytes, 0, DEFAULT_WIDTH );
    }

    /**
     * Print the bytes encoded as hex and ascii with each line using {@link #DEFAULT_WIDTH} to
     * the specified output
     *
     * @param out   the output
     * @param bytes the byte array to encode
     */
    public static void printHexAndAscii( PrintWriter out, byte[] bytes ) {
        printHexAndAscii( out, bytes, 0, DEFAULT_WIDTH );
    }

    /**
     * Print the bytes encoded as hex and ascii with each line using {@link #DEFAULT_WIDTH} to
     * System.out
     *
     * @param bytes  the byte array to encode
     * @param offset the starting offset
     * @param width  the number of bytes to represent in each line
     */
    public static void printHexAndAscii( byte[] bytes, int offset, int width ) {
        printHexAndAscii( System.out, bytes, offset, width );
    }

    /**
     * Print the bytes encoded as hex and ascii with each line using {@link #DEFAULT_WIDTH} to
     * the specified output.
     *
     * @param out    the output
     * @param bytes  the byte array to encode
     * @param offset the starting offset
     * @param width  the number of bytes to represent in each line
     */
    public static void printHexAndAscii( OutputStream out, byte[] bytes, int offset, int width ) {
        printHexAndAscii( new PrintWriter( out ), bytes, offset, width );
    }

    /**
     * Print the bytes encoded as hex and ascii with each line using {@link #DEFAULT_WIDTH} to
     * the specified output.
     *
     * @param out    the output
     * @param bytes  the byte array to encode
     * @param offset the starting offset
     * @param width  the number of bytes to represent in each line
     */
    public static void printHexAndAscii( PrintWriter out, byte[] bytes, int offset, int width ) {
        writeHexAndAscii( out, bytes, offset, width );
    }

    private static void writeHexAndAscii( PrintWriter out, byte[] bytes, int offset, int width ) {
        for ( int index = 0; (index + offset) < bytes.length; index += width ) {
            // for large byte[] may need to format the offset with a larger padding)
            out.printf( "0x%-4x: ", index + offset );
            writeHex( out, bytes, index + offset, width );
            out.append( " : " );
            writeAscii( out, bytes, index + offset, width );
            out.println();
        }
        out.flush();
    }

    private static void writeHex( PrintWriter out, byte[] bytes, int offset, int width ) {
        for ( int index = 0; index < width; index++ ) {
            if ( index + offset < bytes.length ) {
                byte v = bytes[index + offset];
                out.append( Character.forDigit( (v >> 4) & 0xF, 16 ) );
                out.append( Character.forDigit( v & 0xF, 16 ) );
            } else {
                // pad with spaces up to width
                out.append( "  " );
            }
        }
        out.flush();
    }

    private static void writeAscii( PrintWriter out, byte[] bytes, int offset, int width ) {
        width = Math.min( width, bytes.length - offset );
        for ( int index = 0; index < width; index++ ) {
            if ( index + offset < bytes.length ) {
                int bc = bytes[index + offset] & 0xFF;
                if ( bc < 0x20 || bc > 0x7E ) {
                    out.append( "." );
                } else {
                    out.append( (char) bc );
                }
            } else {
                // pad with spaces
                out.append( " " );
            }
        }
        out.flush();
    }
}
