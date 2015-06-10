/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import java.util.*;
import java.util.function.Consumer;

/**
 * @author ptinius
 */
public class ConsecutiveSpliterator<T>
    implements Spliterator<List<T>> {

    private final Spliterator<T> wrappedSpliterator;
    private final int n;
    private final Deque<T> deque;
    private final Consumer<T> dequeConsumer;

    /**
     * @param wrappedSpliterator the {@link java.util.Spliterator}
     * @param n the {@code int} representing grouping size
     */
    public ConsecutiveSpliterator( Spliterator<T> wrappedSpliterator, int n ) {
        this.wrappedSpliterator = wrappedSpliterator;
        this.n = n;
        this.deque = new LinkedList<>();
        this.dequeConsumer = deque::addLast;
    }

    /**
     * @param action The action
     *
     * @return Returns {@code false} if no remaining elements existed upon
     *         entry to this method, else {@code true}.
     *
     * @throws NullPointerException if the specified action is null
     */
    @Override
    public boolean tryAdvance( Consumer<? super List<T>> action ) {
        deque.pollFirst();
        fillDeque();
        if( deque.size() == n ) {
            List<T> list = new ArrayList<>( deque );
            action.accept( list );
            return true;
        } else {
            return false;
        }
    }

    @SuppressWarnings( "StatementWithEmptyBody" )
    private void fillDeque() {
        while( ( deque.size() < n ) &&
               wrappedSpliterator.tryAdvance( dequeConsumer ) );
    }

    /**
     * @return Returns a {@code Spliterator} covering some portion of the elements, or
     *         {@code null} if this spliterator cannot be split
     */
    @Override
    public Spliterator<List<T>> trySplit() {
        return null;
    }

    /**
     * @return Returns the exact size, if known, else {@code -1}.
     */
    @Override
    public long estimateSize() {
        return wrappedSpliterator.estimateSize();
    }

    /**
     * @return Returns a representation of characteristics
     */
    @Override
    public int characteristics() {
        return wrappedSpliterator.characteristics();
    }
}
