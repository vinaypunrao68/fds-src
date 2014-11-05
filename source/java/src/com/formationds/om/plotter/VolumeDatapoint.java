package com.formationds.om.plotter;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.joda.time.DateTime;
import org.json.JSONObject;

/**
 * @deprecated the POC functionality has been replaced
 *             {@link com.formationds.commons.model.entity.VolumeDatapoint}
 *
 *             will be removed in the future
 */
@Deprecated
public class VolumeDatapoint implements Comparable<VolumeDatapoint> {
    private DateTime dateTime;
    private String volumeName;
    private String key;
    private double value;

    public VolumeDatapoint(DateTime dateTime, String volumeName, String key, double value) {
        this.dateTime = dateTime;
        this.volumeName = volumeName;
        this.key = key;
        this.value = value;
    }

    public VolumeDatapoint(JSONObject object) {
        dateTime = new DateTime(Long.parseLong(object.getString("timestamp")));
        volumeName = object.getString("volume");
        key = object.getString("key");
        value = Double.parseDouble(object.getString("value"));
    }

    public DateTime getDateTime() {
        return dateTime;
    }

    public String getVolumeName() {
        return volumeName;
    }

    public String getKey() {
        return key;
    }

    public double getValue() {
        return value;
    }

    @Override
    public int compareTo(VolumeDatapoint o) {
        return o.getDateTime().compareTo(dateTime);
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        VolumeDatapoint that = (VolumeDatapoint) o;

        if (Double.compare(that.value, value) != 0) return false;
        if (!dateTime.equals(that.dateTime)) return false;
        if (!key.equals(that.key)) return false;
        if (!volumeName.equals(that.volumeName)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result;
        long temp;
        result = dateTime.hashCode();
        result = 31 * result + volumeName.hashCode();
        result = 31 * result + key.hashCode();
        temp = Double.doubleToLongBits(value);
        result = 31 * result + (int) (temp ^ (temp >>> 32));
        return result;
    }
}
