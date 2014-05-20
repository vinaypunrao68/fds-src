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
            "http://www.50-best.com/images/cat_memes/buy_a_boat.jpg",
            "http://assets.dogtime.com/asset/image/50eeec2bb7a2a379310000dd/column_Cat-meme_021.jpg",
            "http://weknowmemes.com/wp-content/uploads/2012/05/restraining-cat-meme.jpg",
            "http://assets.dogtime.com/asset/image/51a777f16dab622278003e0d/column_This-doesnt-concern-you-walk-away.jpg",
            "http://t2.gstatic.com/images?q=tbn:ANd9GcQz0ohNQrTZO11Cz3k4KtAJTWAEVdCgCq8MLjFpxugBamVTPwmx"
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
        double count = Math.random() * 20d;
        Counts counts = new Counts();
        counts.increment(volumeName, (int) count);
        return counts;
    }

    @Override
    public Counts consumeWriteCounts() {
        return consumeReadCounts();
    }
}
