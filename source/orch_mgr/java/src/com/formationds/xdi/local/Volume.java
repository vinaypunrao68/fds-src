package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.hibernate.Hibernate;

import javax.persistence.*;

@Entity
@Table(name="volume")
public class Volume implements Persistent {
    private Domain domain;
    private long id;
    private String name;
    private int objectSize;


    public Volume() {
    }

    public Volume(Domain domain, String name, int objectSize) {
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

    @Override
    public void initialize() {
        domain.initialize();
        Hibernate.initialize(this);
    }
}
