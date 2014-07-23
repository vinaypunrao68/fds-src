package com.formationds.util.blob;

/**
 * Created by umesh on 7/18/14.
 */
public enum Mode {
    TRUNCATE(1);

    Mode(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }

    private final int value;
}
