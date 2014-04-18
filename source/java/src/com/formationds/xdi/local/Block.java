package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */


import org.hibernate.Hibernate;

import javax.persistence.*;

@Entity
@Table(name="block")
public class Block implements Persistent {
    private Blob blob;
    private long id = -1;
    private byte[] bytes;

    public Block(Blob blob, byte[] bytes) {
        this.blob = blob;
        this.bytes = bytes;
    }

    public Block() {
    }


    @Id
    @GeneratedValue
    @Column(name="id")
    @Override
    public long getId() {
        return id;
    }

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "blob_id", nullable = false)
    public Blob getBlob() {
        return blob;
    }

    @Column(name="bytes", nullable = false, columnDefinition = "BLOB")
    public byte[] getBytes() {
        return bytes;
    }

    public void setBlob(Blob blob) {
        this.blob = blob;
    }

    public void setId(long id) {
        this.id = id;
    }

    public void setBytes(byte[] bytes) {
        this.bytes = bytes;
    }

    @Override
    public void initialize() {
        Hibernate.initialize(this);
    }
}
