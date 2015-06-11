package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Sets;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.Set;
import java.util.function.BiConsumer;
import java.util.function.BinaryOperator;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.stream.Collector;
import java.util.stream.IntStream;
import java.util.stream.Stream;

public class JsonArrayCollector implements Collector<JSONObject, JSONArray, JSONArray> {
    @Override
    public Supplier<JSONArray> supplier() {
        return () -> new JSONArray();
    }

    @Override
    public BiConsumer<JSONArray, JSONObject> accumulator() {
        return (a, o) -> a.put(o);
    }

    @Override
    public BinaryOperator<JSONArray> combiner() {
        return (a, b) -> {
            JSONArray combined = new JSONArray();
            for (int i = 0; i < a.length(); i++) {
                combined.put(a.get(i));
            }
            for (int i = 0; i < b.length(); i++) {
                combined.put(b.get(i));
            }
            return combined;
        };
    }

    @Override
    public Function<JSONArray, JSONArray> finisher() {
        return a -> a;
    }

    @Override
    public Set<Characteristics> characteristics() {
        return Sets.newHashSet();
    }

    public static Stream<JSONObject> asStream(JSONArray array) {
        return IntStream.range(0, array.length())
                .mapToObj(i -> array.getJSONObject(i));
    }
}
