package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.hibernate.Hibernate;
import org.json.JSONObject;

import javax.persistence.*;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

@Entity
@Table(name="blob")
public class Blob implements Persistent {
    private long id;
    private long byteCount;
    private Volume volume;
    private String name;
    private String metadataJson = "{}";

    public Blob() {
    }

    public Blob(Volume volume, String name) {
        this.volume = volume;
        this.name = name;
    }

    @Id
    @GeneratedValue
    @Column(name="id")
    @Override
    public long getId() {
        return id;
    }

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "volume_id", nullable = false)
    public Volume getVolume() {
        return volume;
    }

    @Column(name = "name")
    public String getName() {
        return name;
    }

    @Column(name="metadata_json", columnDefinition = "CLOB")
    public String getMetadataJson() {
        return metadataJson;
    }

    @Column(name="byte_count")
    public long getByteCount() {
        return byteCount;
    }

    @Transient
    public Map<String, String> getMetadata() {
        Map<String, String> metadata = new HashMap<>();
        JSONObject o = new JSONObject(getMetadataJson());
        Iterator<String> keys = o.keys();
        while (keys.hasNext()) {
            String k = keys.next();
            metadata.put(k, o.getString(k));
        }
        return metadata;
    }

    public void setByteCount(long byteCount) {
        this.byteCount = byteCount;
    }

    public void setVolume(Volume volume) {
        this.volume = volume;
    }

    public void setId(long id) {
        this.id = id;
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setMetadataJson(String metadataJson) {
        this.metadataJson = metadataJson;
    }

    @Override
    public void initialize() {
        Hibernate.initialize(this);
    }
}
