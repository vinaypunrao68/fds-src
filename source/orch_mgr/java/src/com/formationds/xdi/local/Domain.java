package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.hibernate.Hibernate;

import javax.persistence.*;

@Entity
@Table(name="domain")
public class Domain implements Persistent {
    private long id;
    private String name;

    public Domain() {
    }

    public Domain(long id, String name) {
        this.id = id;
        this.name = name;
    }

    @Id
    @GeneratedValue
    @Column(name="id")
    public long getId() {
        return id;
    }

    @Override
    public void initialize() {
        Hibernate.initialize(this);
    }

    @Column(name="name", nullable = false)
    public String getName() {
        return name;
    }

    public void setId(long id) {
        this.id = id;
    }

    public void setName(String name) {
        this.name = name;
    }
}
