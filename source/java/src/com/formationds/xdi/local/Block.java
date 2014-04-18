package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */


import org.hibernate.Hibernate;

import javax.persistence.*;

@Entity
@Table(name="block", indexes = {
        @Index(name = "blob_position_idx", columnList = "blob_id, position")
})
public class Block implements Persistent {
    private long blobId;
    private long id = -1;
    private long position;

    private byte[] bytes;

    public Block(long blobId, long position, byte[] bytes) {
        this.blobId = blobId;
        this.position = position;
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

    @Column(name="blob_id")
    public long getBlobId() {
        return blobId;
    }

    @Column(name="position")
    public long getPosition() {
        return position;
    }

    @Column(name="bytes", nullable = false, columnDefinition = "BLOB")
    public byte[] getBytes() {
        return bytes;
    }

    public void setBlobId(long blobId) {
        this.blobId = blobId;
    }

    public void setPosition(long position) {
        this.position = position;
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
