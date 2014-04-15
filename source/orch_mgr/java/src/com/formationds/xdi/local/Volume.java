package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.hibernate.Hibernate;
import org.joda.time.DateTime;

import javax.persistence.*;
import java.util.UUID;

@Entity
@Table(name="volume")
public class Volume implements Persistent {
    private Domain domain;
    private long id;
    private String name;
    private int objectSize;
    private long timestamp = DateTime.now().getMillis();
    private long uuidLow;
    private long uuidHigh;

    public Volume() {
        UUID uuid = UUID.randomUUID();
        uuidLow = uuid.getLeastSignificantBits();
        uuidHigh = uuid.getLeastSignificantBits();
    }

    public Volume(Domain domain, String name, int objectSize) {
        this();
        this.domain = domain;
        this.name = name;
        this.objectSize = objectSize;
    }

    @Override
    @Id
    @GeneratedValue
    @Column(name="id")
    public long getId() {
        return id;
    }

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "domain_id", nullable = false)
    public Domain getDomain() {
        return domain;
    }

    @Column(name="name", nullable = false)
    public String getName() {
        return name;
    }

    @Column(name="object_size", nullable = false)
    public int getObjectSize() {
        return objectSize;
    }

    @Column(name="tstamp", nullable = false)
    public long getTimestamp() {
        return timestamp;
    }

    @Column(name="uuid_low", nullable = false)
    public long getUuidLow() {
        return uuidLow;
    }

    @Column(name="uuid_high", nullable = false)
    public long getUuidHigh() {
        return uuidHigh;
    }

    public void setTimestamp(long timestamp) {
        this.timestamp = timestamp;
    }


    public void setDomain(Domain domain) {
        this.domain = domain;
    }

    public void setId(long id) {
        this.id = id;
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setObjectSize(int objectSize) {
        this.objectSize = objectSize;
    }

    public void setUuidLow(long uuidLow) {
        this.uuidLow = uuidLow;
    }

    public void setUuidHigh(long uuidHigh) {
        this.uuidHigh = uuidHigh;
    }

    @Override
    public void initialize() {
        domain.initialize();
        Hibernate.initialize(this);
    }
}
