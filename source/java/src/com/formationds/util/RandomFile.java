/**
 * Created by monchier on 9/21/14.
 */

package com.formationds.util;

import java.io.*;
import java.util.Random;

public class RandomFile {
    int _obj_size;
    int _n_objects;
    int _size;
    Random _gen;
    File _randomFile;
    FileInputStream _is;

    RandomFile(int osize, int n_objects) {
        _obj_size = osize;
        _n_objects = n_objects;
        _size = osize * n_objects;
        _gen = new Random(123);
        System.out.println("Random File - size: " + _size + " n_objects: " + _n_objects + " obj_size: " + _obj_size);
    }

    public void create_file() throws FileNotFoundException, IOException {
        _randomFile = File.createTempFile("fdstrgen", "", new File("/tmp"));
        FileOutputStream out = new FileOutputStream(_randomFile);
        int size_kb = _size / 1024;
        int rem = _size - size_kb * 1024;
        byte[] buf = new byte[1024];
        // System.out.println("Size kb: " + size_kb);
        // System.out.println("Size rem: " + rem);
        for (int i = 0; i < size_kb; i++) {
            _gen.nextBytes(buf);
            out.write(buf);
            // System.out.println("Size: " + buf.length);
        }
        buf = new byte[rem];
        _gen.nextBytes(buf);
        // System.out.println("Size: " + buf.length);
        out.write(buf);
        out.close();
    }

    public void open() throws FileNotFoundException {
        _is = new FileInputStream(_randomFile);
    }

    public void close() throws IOException {
        // close file
        // need to close on destructor
        _is.close();
    }

    public byte[] get_obj() throws IOException {
        // if file is not open, open file
        // read bytes
        // return bytes
        byte[] buf = new byte[_obj_size];
        synchronized (_is) {
            _is.read(buf);
            return buf;
        }
    }

    public void read(byte[] buf) throws IOException {
        // if file is not open, open file
        // read bytes
        // return bytes
        synchronized (_is) {
            _is.read(buf);
        }
    }
}
