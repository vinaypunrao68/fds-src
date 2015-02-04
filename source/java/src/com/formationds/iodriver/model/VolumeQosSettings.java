package com.formationds.iodriver.model;

import com.formationds.apis.MediaPolicy;

public final class VolumeQosSettings
{
    public VolumeQosSettings(long id,
                             int iops_assured,
                             int iops_throttle,
                             int priority,
                             long commit_log_retention,
                             MediaPolicy mediaPolicy)
    {
        _id = id;
        _iops_assured = iops_assured;
        _iops_throttle = iops_throttle;
        _priority = priority;
        _commit_log_retention = commit_log_retention;
        _mediaPolicy = mediaPolicy;
    }

    public VolumeQosSettings copy()
    {
        return new VolumeQosSettings(_id,
                                     _iops_assured,
                                     _iops_throttle,
                                     _priority,
                                     _commit_log_retention,
                                     _mediaPolicy);
    }

    public long getCommitLogRetention()
    {
        return _commit_log_retention;
    }

    public long getId()
    {
        return _id;
    }

    public int getIopsAssured()
    {
        return _iops_assured;
    }

    public int getIopsThrottle()
    {
        return _iops_throttle;
    }

    public MediaPolicy getMediaPolicy()
    {
        return _mediaPolicy;
    }

    public int getPriority()
    {
        return _priority;
    }

    private final long _commit_log_retention;

    private final long _id;

    private final int _iops_assured;

    private final int _iops_throttle;

    private final MediaPolicy _mediaPolicy;

    private final int _priority;
}
