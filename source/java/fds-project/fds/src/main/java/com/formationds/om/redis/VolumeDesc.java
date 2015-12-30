/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.redis;

import FDS_ProtocolInterface.FDSP_AppWorkload;
import FDS_ProtocolInterface.FDSP_ConsisProtoType;
import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.protocol.svc.types.ResourceState;
import com.google.common.base.MoreObjects;

import java.math.BigInteger;
import java.util.Map;

/**
 * @author ptinius
 */
public class VolumeDesc
{
    private final Map<String,String> fields;
    private VolumeSettings settings;

    public VolumeDesc( final Map<String, String> fields )
    {
        this.fields = fields;
    }

    private double toDouble( final String string )
    {
        if( string != null && string.matches( "^[-+]?\\d+$" ) )
        {
            return Double.valueOf( string );
        }

        return -1.0;
    }

    private BigInteger toBigInteger( final String string )
    {
        if( string != null && string.matches( "^[-+]?\\d+$" ) )
        {
            return new BigInteger( string, 10 );
        }

        return BigInteger.valueOf( -1L );
    }

    private long toLong( final String string )
    {
        if( string != null && string.matches( "^[-+]?\\d+$" ) )
        {
            return Long.valueOf( string );
        }

        return -1;
    }

    private int toInteger( final String string )
    {
        if( string != null && string.matches( "^[-+]?\\d+$" ) )
        {
            return Integer.valueOf( string );
        }

        return -1;
    }

    public BigInteger id() { return toBigInteger( fields.get( "uuid" ) ); }

    public long tenantId() { return toLong( fields.get( "tennant.id" ) ); }
    public long localDomainId() { return toLong( fields.get( "local.domain.id" ) ); }
    public long globalDomainId( ) { return toLong( fields.get( "global.domain.id" ) ); }
    public long parentVolumeId() { return toLong( fields.get( "parentvolumeid" ) ); }
    public long createTime() { return toLong( fields.get( "create.time" ) ); }
    public long backupVolumeId() { return toLong( fields.get( "backup.vol.id" ) ); }
    public long retention() { return toLong( fields.get( "continuous.commit.log.retention" ) ); }


    public int replicaCount() { return toInteger( fields.get( "replica.count" ) ); }
    public int redundancyCount() { return toInteger( fields.get( "redundancy.count" ) ); }
    public int maxObjectSize() { return toInteger( fields.get( "objsize.max" ) ); }
    public int writeQuorum() { return toInteger( fields.get( "write.quorum" ) ); }
    public int readQuorum() { return toInteger( fields.get( "read.quorum" ) ); }
    public int policyId() { return toInteger( fields.get( "volume.policy.id" ) ); }
    public int archivePolicyId() { return toInteger( fields.get( "archive.policy.id" ) ); }
    public int placementPolicyId() { return toInteger( fields.get( "placement.policy.id" ) ); }
    public int relativePriority() { return toInteger( fields.get( "relative.priority" ) ); }

    public boolean isSnapshot()
    {
        return Integer.valueOf( fields.get( "fsnapshot" ) ) != 0;
    }

    public double capacity() { return toDouble( fields.get( "capacity" ) ); }
    public double quotaMax() { return toDouble( fields.get( "quota.max" ) ); }
    public double iopsMin() { return toDouble( fields.get( "iops.min" ) ); }
    public double iopsMax() { return toDouble( fields.get( "iops.max" ) ); }

    public String name() { return fields.get( "name" ); }

    public ResourceState state( )
    {
        return ResourceState.findByValue( Integer.valueOf( fields.get( "state" ) ) );
    }

    public MediaPolicy mediaPolicy( )
    {
        return MediaPolicy.findByValue( Integer.valueOf( fields.get( "media.policy" ) ) );
    }

    public FDSP_AppWorkload workload( )
    {
        return FDSP_AppWorkload.findByValue( Integer.valueOf( fields.get( "app.workload" ) ) );
    }

    public VolumeType type( )
    {
        return VolumeType.findByValue( Integer.valueOf( fields.get( "type" ) ) );
    }

    public FDSP_ConsisProtoType consistencyProtocol( )
    {
        return FDSP_ConsisProtoType.findByValue(
            Integer.valueOf( fields.get( "conistency.protocol" ) ) );
    }

    public VolumeSettings settings() { return settings; }
    public void settings( final VolumeSettings settings ) { this.settings = settings; }

    @Override
    public String toString( )
    {
        return MoreObjects.toStringHelper( this )
                          .add( "uuid", id( ) )
                          .add( "name", name( ) )
                          .add( "placementPolicyId", placementPolicyId( ) )
                          .add( "readQuorum", readQuorum( ) )
                          .add( "state", state( ) )
                          .add( "localDomainId", localDomainId( ) )
                          .add( "archivePolicyId", archivePolicyId( ) )
                          .add( "backupVolumeId", backupVolumeId( ) )
                          .add( "redundancyCount", redundancyCount( ) )
                          .add( "createTime", createTime( ) )
                          .add( "writeQuorum", writeQuorum( ) )
                          .add( "relativePriority", relativePriority( ) )
                          .add( "policyId", policyId( ) )
                          .add( "tenantId", tenantId( ) )
                          .add( "consistencyProtocol", consistencyProtocol( ) )
                          .add( "globalDomainId", globalDomainId( ) )
                          .add( "quotaMax", quotaMax( ) )
                          .add( "capacity", capacity( ) )
                          .add( "maxObjectSize", maxObjectSize( ) )
                          .add( "replicaCount", replicaCount( ) )
                          .add( "iopsMin", iopsMin( ) )
                          .add( "iopsMax", iopsMax( ) )
                          .add( "parentVolumeId", parentVolumeId( ) )
                          .add( "workload", workload( ) )
                          .add( "snapshot", isSnapshot( ) )
                          .add( "settings", settings( ) )
                          .toString( );
    }
}
