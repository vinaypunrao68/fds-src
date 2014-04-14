package com.formationds.xdi.local;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.google.common.collect.Lists;
import org.hibernate.Hibernate;

import javax.persistence.*;
import java.util.List;

@Entity
@Table(name="blob")
public class Blob implements Persistent {
    private long id;
    private Volume volume;
    private String name;
    private String metadataJson;
    private List<Block> blocks;


    public Blob(Volume volume, String name) {
        this.volume = volume;
        this.name = name;
        blocks = Lists.newArrayList();
    }

    public Blob() {
        blocks = Lists.newArrayList();
    }

    @Id
    @GeneratedValue
    @Column(name="id")
    @Override
    public long getId() {
        return id;
    }

    @OneToMany
    @JoinColumn(name="blob_id")
    public List<Block> getBlocks() {
        return blocks;
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


    void setBlocks(List<Block> blocks) {
        this.blocks = blocks;
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
        for (Block block : blocks) {
            block.initialize();
        }
    }
}
