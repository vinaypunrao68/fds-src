/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import java.util.List;
import java.util.Spliterator;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

/**
 * @author ptinius
 */
public class StreamHelper {
    /**
     * @param stream the {@link java.util.stream.Stream} to be grouped
     * @param n the {@code int} representing the grouping size
     *
     * @return Returns the {@link java.util.stream.Stream} representing the
     *         {@link List} of grouped elements
     */
    public static <E> Stream<List<E>> consecutiveStream( Stream<E> stream, int n ) {
        Spliterator<E> spliterator = stream.spliterator();
        Spliterator<List<E>> wrapper = new ConsecutiveSpliterator<>( spliterator, n );
        return StreamSupport.stream( wrapper, false );
    }
}
