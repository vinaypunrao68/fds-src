package com.formationds.sc;

import java.util.AbstractList;

public class LongArrayBackedList extends AbstractList<Long> {
    private long[] data;

    public LongArrayBackedList(long[] data) {
        this.data = data;
    }

    @Override
    public Long get(int index) {
        return data[index];
    }

    @Override
    public int size() {
        return data.length;
    }
}
