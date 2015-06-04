package com.formationds.client.v08.model;

import java.time.Duration;
import java.time.Instant;

public class Snapshot extends AbstractResource<Long>{

	private final Duration retention;
	private final Long volumeId;
	private final Instant creationTime;
	
	private Snapshot(){
		this( 0L, "None", 0L, null, null );
	}
	
	public Snapshot(Long uid, String name) {
		this(uid, name, 0L, null, null);
	}
	
	public Snapshot(Long uid, String name, Long volumeId, Duration retention, Instant creationTime ){
		super( uid, name );
		
		this.volumeId = volumeId;
		this.retention = retention;
		this.creationTime = creationTime;
	}
	
	public Duration getRetention(){
		return this.retention;
	}
	
	public Long getVolumeId(){
		return this.volumeId;
	}
	
	public Instant getCreationTime(){
		return this.creationTime;
	}

}
