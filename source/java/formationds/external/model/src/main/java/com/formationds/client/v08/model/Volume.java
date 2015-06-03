/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import com.formationds.client.ical.RecurrenceRule;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Volume extends AbstractResource<Long> {

    public static class Builder {

        private final String volumeName;
        private final Tenant              tenant;
        private final Map<String, String> tags = new HashMap<>();
        private String               application;
        private VolumeStatus         status;
        private VolumeSettings       settings;
        private MediaPolicy          mediaPolicy;
        private DataProtectionPolicy dataProtectionPolicy;
        private VolumeAccessPolicy   accessPolicy;
        private QosPolicy            qosPolicy;

        public Builder( Tenant tenant, String volumeName ) {
            this.tenant = tenant;
            this.volumeName = volumeName;
        }

        public Builder addTag(String k, String v) { this.tags.put(k, v); return this; }
        public Builder addTags(Map<String,String> tags) { this.tags.putAll(tags); return this; }
        public Builder application(String app) { this.application = app; return this; }
        public Builder status(VolumeStatus status) { this.status = status; return this; }
        public Builder mediaPolicy(MediaPolicy m) { this.mediaPolicy = m; return this; }
        public Builder accessPolicy(VolumeAccessPolicy ap) { this.accessPolicy = ap; return this; }
        public Builder qosPolicy(QosPolicy qp) { this.qosPolicy = qp; return this; }

        public Builder dataProtectionPolicy(DataProtectionPolicy dpp) { this.dataProtectionPolicy = dpp; return this; }
        public Builder dataProtectionPolicy(DataProtectionPolicyPreset preset) {
            return dataProtectionPolicy( DataProtectionPolicy.fromPreset( preset ) );
        }
        public Builder dataProtectionPolicy(long commitLogRetention,
                                            long snapshotRetentionTime,
                                            RecurrenceRule rr,
                                            TimeUnit unit ) {
            List<SnapshotPolicy> sp = new ArrayList<>( );
            sp.add( new SnapshotPolicy( rr, Duration.of( snapshotRetentionTime, unit ) ) );
            return dataProtectionPolicy( new DataProtectionPolicy( Duration.of( commitLogRetention, unit), sp) );
        }

        public Volume create() {
            return null;
        }
    }

    private Tenant               tenant;
    private Map<String, String>  tags;
    private String               application;
    private VolumeStatus         status;
    private VolumeSettings       settings;
    private MediaPolicy          mediaPolicy;
    private DataProtectionPolicy dataProtectionPolicy;
    private VolumeAccessPolicy   accessPolicy;
    private QosPolicy            qosPolicy;
    private Instant				 created;

    public Volume( Long uid,
            String name,
            Tenant tenant,
            String application,
            VolumeStatus status,
            VolumeSettings settings,
            MediaPolicy mediaPolicy,
            DataProtectionPolicy dataProtectionPolicy,
            VolumeAccessPolicy accessPolicy,
            QosPolicy qosPolicy,
            Instant created,
            Map<String, String> tags ) {
        super( uid, name );
        this.tenant = tenant;
        this.tags = tags;
        this.application = application;
        this.status = status;
        this.settings = settings;
        this.mediaPolicy = mediaPolicy;
        this.dataProtectionPolicy = dataProtectionPolicy;
        this.accessPolicy = accessPolicy;
        this.qosPolicy = qosPolicy;
        this.created = created;
    }
    
    public Volume( Long uid,
    		String name ){
    	this( uid, name, null, null, null, null, null, null, null, null, null, null );
    }

    public Tenant getTenant() {
        return tenant;
    }

    public void setTenant( Tenant tenant ) {
        this.tenant = tenant;
    }

    public Map<String, String> getTags() {
        return tags;
    }

    public void addTag( String tag, String val ) {
        this.tags.put( tag, val );
    }

    public void removeTag( String tag ) {
        this.tags.remove( tag );
    }

    public String getApplication() {
        return application;
    }

    public void setApplication( String application ) {
        this.application = application;
    }

    public VolumeStatus getStatus() {
        return status;
    }

    public void setStatus( VolumeStatus status ) {
        this.status = status;
    }

    public VolumeSettings getSettings() {
        return settings;
    }

    public void setSettings( VolumeSettings settings ) {
        this.settings = settings;
    }
    
    public Instant getCreated(){ return created; }

    public MediaPolicy getMediaPolicy() { return mediaPolicy; }

    public DataProtectionPolicy getDataProtectionPolicy() { return dataProtectionPolicy; }

    public QosPolicy getQosPolicy() { return qosPolicy; }

    public VolumeAccessPolicy getAccessPolicy() { return accessPolicy; }

}
