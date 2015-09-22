package com.formationds.iodriver.model;

import static com.formationds.client.v08.converters.ExternalModelConverter.convertToInternalMediaPolicy;

import com.formationds.apis.MediaPolicy;
import com.formationds.client.v08.model.DataProtectionPolicy;
import com.formationds.client.v08.model.QosPolicy;
import com.formationds.client.v08.model.Volume;
import com.formationds.commons.NullArgumentException;

/**
 * QoS settings for a volume.
 */
public final class VolumeQosSettings
{
    /**
     * Constructor.
     * 
     * @param id The volume's unique ID.
     * @param iops_assured The IOPS to guarantee can always be met.
     * @param iops_throttle The maximum IOPS to allow.
     * @param priority Relative priority to other volumes.
     * @param commit_log_retention Seconds to keep full commit logs (snapshots of volumes can be
     *            created in this time).
     * @param mediaPolicy What media to use for this volume.
     */
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

    public static VolumeQosSettings fromVolume(Volume volume)
    {
        if (volume == null) throw new NullArgumentException("volume");
        
        QosPolicy qosPolicy = volume.getQosPolicy();
        DataProtectionPolicy dataProtectionPolicy = volume.getDataProtectionPolicy();
        
        return new VolumeQosSettings(volume.getId(),
                                     qosPolicy.getIopsMin(),
                                     qosPolicy.getIopsMax(),
                                     qosPolicy.getPriority(),
                                     dataProtectionPolicy.getCommitLogRetention().getSeconds(),
                                     convertToInternalMediaPolicy(volume.getMediaPolicy()));
    }
    
    /**
     * Do a deep copy of this object.
     * 
     * @return A duplicate object.
     */
    public VolumeQosSettings copy()
    {
        return new VolumeQosSettings(_id,
                                     _iops_assured,
                                     _iops_throttle,
                                     _priority,
                                     _commit_log_retention,
                                     _mediaPolicy);
    }

    /**
     * Get the seconds to keep full commit logs for this volume.
     * 
     * @return The current property value.
     */
    public long getCommitLogRetention()
    {
        return _commit_log_retention;
    }

    /**
     * Get the volume's unique ID.
     * 
     * @return The current property value.
     */
    public long getId()
    {
        return _id;
    }

    /**
     * Get the number of IOPS this volume is guaranteed to be able to dispatch.
     * 
     * @return The current property value.
     */
    public int getIopsAssured()
    {
        return _iops_assured;
    }

    /**
     * Get the maximum IOPS this volume will be allowed to dispatch.
     * 
     * @return The current property value.
     */
    public int getIopsThrottle()
    {
        return _iops_throttle;
    }

    /**
     * Get the type of media to use for this volume.
     * 
     * @return The current property value.
     */
    public MediaPolicy getMediaPolicy()
    {
        return _mediaPolicy;
    }

    /**
     * Get the relative priority of this volume.
     * 
     * @return The current property value.
     */
    public int getPriority()
    {
        return _priority;
    }

    @Override
    public String toString()
    {
        return "VolumeQosSettings("
               + "id:" + _id
               + ", iops_assured:" + _iops_assured
               + ", iops_throttle:" + _iops_throttle
               + ", priority:" + _priority
               + ", commit_log_retention:" + _commit_log_retention
               + ", mediaPolicy:" + _mediaPolicy
               + ")";
    }

    /**
     * Seconds to keep full commit logs.
     */
    private final long _commit_log_retention;

    /**
     * Unique ID for this volume.
     */
    private final long _id;

    /**
     * Guaranteed IOPS.
     */
    private final int _iops_assured;

    /**
     * Maximum IOPS.
     */
    private final int _iops_throttle;

    /**
     * Media to use.
     */
    private final MediaPolicy _mediaPolicy;

    /**
     * Relative priority.
     */
    private final int _priority;
}
