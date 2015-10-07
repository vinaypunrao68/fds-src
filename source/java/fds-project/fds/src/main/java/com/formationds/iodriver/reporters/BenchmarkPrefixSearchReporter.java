//package com.formationds.iodriver.reporters;
//
//import java.io.Closeable;
//import java.io.PrintStream;
//
//import com.formationds.commons.NullArgumentException;
//
//public class BenchmarkPrefixSearchReporter implements Closeable
//{
//    public BenchmarkPrefixSearchReporter(PrintStream output, WorkloadEventListener listener)
//    {
//        if (output == null) throw new NullArgumentException("output");
//        if (listener == null) throw new NullArgumentException("listener");
//        
//        _closed = new AtomicBoolean(false);
//        _output = output;
//        
//    }
//}
