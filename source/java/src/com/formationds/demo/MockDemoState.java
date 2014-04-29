package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Lists;

import java.util.Collections;
import java.util.List;
import java.util.Optional;

public class MockDemoState implements DemoState {
    private String[] images = new String[] {
            "http://i.imgur.com/MtlS0.jpg",
            "http://s2.quickmeme.com/img/49/49eb7809d1b084f7005b380d125e5230011fec134501671e245e66897bad242a.jpg",
            "http://uproxx.files.wordpress.com/2012/10/sudden-clarity-cat-25.jpg",
            "http://www.50-best.com/images/cat_memes/buy_a_boat.jpg"
    };
    private String searchExpression;
    private int readThrottle;
    private int writeThrottle;


    @Override
    public void setSearchExpression(String searchExpression) {
        this.searchExpression = searchExpression;
    }

    @Override
    public Optional<String> getCurrentSearchExpression() {
        return searchExpression == null ? Optional.<String>empty() : Optional.of(searchExpression);
    }

    @Override
    public Optional<ImageResource> peekReadQueue() {
        List<String> list = Lists.newArrayList(images);
        Collections.shuffle(list);
        return Optional.of(new ImageResource("42", list.get(0)));
    }

    @Override
    public Optional<ImageResource> peekWriteQueue() {
        return peekReadQueue();
    }

    @Override
    public int getReadThrottle() {
        return readThrottle;
    }

    @Override
    public void setReadThrottle(int value) {
        this.readThrottle = value;
    }

    @Override
    public int getWriteThrottle() {
        return writeThrottle;
    }

    @Override
    public void setWriteThrottle(int value) {
        this.writeThrottle = value;
    }

    @Override
    public Counts consumeReadCounts() {
        return makeFakeCounts("Volume1", "Volume2", "Volume3", "Volume4", "Volume5");
    }

    private Counts makeFakeCounts(String... volumeNames) {
        Counts counts = new Counts();
        for (String volumeName : volumeNames) {
            counts = counts.merge(oneVolume(volumeName));
        }
        return counts;
    }

    private Counts oneVolume(String volumeName) {
        double count = Math.random() * 100d;
        Counts counts = new Counts();
        counts.increment(volumeName, (int) count);
        return counts;
    }

    @Override
    public Counts consumeWriteCounts() {
        return consumeReadCounts();
    }
}
