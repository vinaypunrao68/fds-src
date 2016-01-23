/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.om.repository.influxdb;

import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.DateRange;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.SystemActivityEvent;
import com.formationds.commons.model.entity.UserActivityEvent;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.repository.EventRepository;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.query.QueryCriteria.QueryType;
import com.google.common.collect.Lists;
import org.influxdb.dto.Serie;

import java.util.*;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

public class InfluxEventRepository extends InfluxRepository<Event, Long> implements EventRepository {

    public static final String EVENT_SERIES_NAME    = "events";
    public static final String EVENT_TYPE_COLUMN_NAME = "event_type";
    public static final String EVENT_SEVERITY_COLUMN_NAME = "event_severity";
    public static final String EVENT_CATEGORY_COLUMN_NAME = "event_category";
    public static final String EVENT_KEY_COLUMN_NAME = "event_key";
    public static final String EVENT_ARGS_COLUMN_NAME = "event_args";
    public static final String EVENT_DFLT_MESSAGE_COLUMN_NAME = "event_dflt_msg";

    /** User Activity Event data - only present if the EventType is USER_ACTIVITY */
    public static final String USER_EVENT_USERID_COLUMN_NAME = "user_event_uid";

    /** Firebreak Event data - only present if EventType is FIREBREAK_EVENT */
    public static final String FBEVENT_VOL_ID_COLUMN_NAME = "fb_event_volid";

    /** Firebreak Event data - only present if EventType is FIREBREAK_EVENT */
    public static final String FBEVENT_VOL_NAME_COLUMN_NAME = "fb_event_volname";

    /** Firebreak Event data - only present if EventType is FIREBREAK_EVENT */
    public static final String FBEVENT_TYPE_COLUMN_NAME = "fb_event_type";

    /** Firebreak Event data - only present if EventType is FIREBREAK_EVENT */
    public static final String FBEVENT_USAGE_COLUMN_NAME = "fb_event_usage_bytes";

    /** Firebreak Event data - only present if EventType is FIREBREAK_EVENT */
    public static final String FBEVENT_SIGMA_COLUMN_NAME = "fb_event_sigma";

    /**
     * the static list of metric names store in the influxdb database.
     */
    public static final List<String> EVENT_COLUMN_NAMES = Collections.unmodifiableList( getColumnNames() );

    // internal array of metric names, necessary for influxdb api
    private static final String[] EVENT_COLUMN_NAMES_A =
        EVENT_COLUMN_NAMES.toArray( new String[EVENT_COLUMN_NAMES.size()] );

    public static final InfluxDatabase DEFAULT_EVENT_DB =
        new InfluxDatabase.Builder( "om-eventdb" )
            .addShardSpace( "default", "30d", "1d", "/.*/", 1, 1 )
            .build();

    /**
     * @return the list of metric names in the order they are stored.
     */
    public static List<String> getColumnNames() {
        return Lists.newArrayList( TIMESTAMP_COLUMN_NAME,
                                   EVENT_TYPE_COLUMN_NAME,
                                   EVENT_CATEGORY_COLUMN_NAME,
                                   EVENT_SEVERITY_COLUMN_NAME,
                                   EVENT_KEY_COLUMN_NAME,
                                   EVENT_DFLT_MESSAGE_COLUMN_NAME,
                                   EVENT_ARGS_COLUMN_NAME,
                                   USER_EVENT_USERID_COLUMN_NAME,
                                   FBEVENT_TYPE_COLUMN_NAME,
                                   FBEVENT_VOL_ID_COLUMN_NAME,
                                   FBEVENT_VOL_NAME_COLUMN_NAME,
                                   FBEVENT_USAGE_COLUMN_NAME,
                                   FBEVENT_SIGMA_COLUMN_NAME );
    }

    public InfluxEventRepository( String url, String adminUser, char[] adminCredentials ) {
        super( url, adminUser, adminCredentials );
    }

    /**
     * @param properties the connection properties
     */
    @Override
    synchronized public void open( Properties properties ) {
        super.open( properties );

        // command is silently ignored if the database already exists.
        super.createDatabaseAsync( DEFAULT_EVENT_DB );
    }

    @Override
    public String getInfluxDatabaseName() { return DEFAULT_EVENT_DB.getName(); }

    @Override
    public String getEntityName() {
        return EVENT_SERIES_NAME;
    }

    @Override
    public String getTimestampColumnName() {
        return super.getTimestampColumnName();
    }

    public Optional<String> getEventTypeColumnName() {
        return Optional.of( EVENT_TYPE_COLUMN_NAME );
    }

    @Override
    public long countAll() {
        // execute the query
        List<Serie> series = getConnection().getDBReader()
                                            .query( "select count from " + getEntityName(),
                                                    TimeUnit.MILLISECONDS );

        Object o = series.get( 0 ).getRows().get(0).get( "count" ) ;
        if (o instanceof Number)
            return ((Number)o).longValue();
        else
            throw new IllegalStateException( "Invalid type returned from select count query" );
    }

    @Override
    public long countAllBy( Event entity ) {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
	public List<? extends Event> query(QueryCriteria queryCriteria) {
        // get the query string
        String queryString = formulateQueryString( queryCriteria, FBEVENT_VOL_ID_COLUMN_NAME );

        // execute the query
        List<Serie> series = getConnection().getDBReader().query( queryString, TimeUnit.MILLISECONDS );

        // convert from influxdb format to FDS model format
        List<? extends Event> datapoints = convertSeriesToEvents( series );

        return datapoints;
	}

    @Override
	public void delete(Event entity) {
		// TODO Deleting an entity from Influx requires the row timestamp

	}

	@Override
	public void close() {
		// TODO Auto-generated method stub

	}

	@Override
	public List<UserActivityEvent> queryTenantUsers(QueryCriteria queryCriteria,
			List<Long> tenantUsers) {

        QueryCriteria criteria = new QueryCriteria( QueryType.USER_ACTIVITY_EVENT );
        String queryBase = formulateQueryString( criteria, FBEVENT_VOL_ID_COLUMN_NAME );
        StringBuilder queryString = new StringBuilder( queryBase );

        if ( ! queryBase.contains( WHERE )) {
            queryString.append( " " ).append( WHERE ).append( " " );
        }

        if (tenantUsers.size() > 1) {
            queryString.append( " ( " );
        }
        Iterator<Long> idIter = tenantUsers.iterator();
        while (idIter.hasNext()) {

            Long id = idIter.next();
            queryString.append( " " )
                       .append( USER_EVENT_USERID_COLUMN_NAME )
                       .append( " = " )
                       .append( id )
                       .append( "" );

            if (idIter.hasNext()) {
                queryString.append( " " )
                           .append( OR )
                           .append( " " );
            }
        }

        if (tenantUsers.size() > 1) {
            queryString.append( " ) " );
        }

        // execute the query
        List<Serie> series = getConnection().getDBReader()
                                            .query( queryString.toString(), TimeUnit.MILLISECONDS );

        if ( series.size() == 0 ) {
            return null;
        }

        // convert from influxdb format to FDS model format
        List<? extends Event> datapoints = convertSeriesToEvents( series );

        return (List<UserActivityEvent>)datapoints;
	}

    @Override
    public EnumMap<FirebreakType, FirebreakEvent> findLatestFirebreaks( Volume v ) {

        // create base query (select * from EVENT_SERIES_NAME where
        QueryCriteria criteria = new QueryCriteria( QueryType.FIREBREAK_EVENT, DateRange.last24Hours() );

        // criteria is still using old model classes.
        criteria.addContext( new com.formationds.client.v08.model.Volume( Long.valueOf( v.getId() ),
                                                                          v.getName() ) );

        String queryBase = formulateQueryString( criteria,
                                                 FBEVENT_VOL_ID_COLUMN_NAME );

        StringBuilder queryString = new StringBuilder( queryBase )
                                        .append( " " )
                                        .append( AND )
                                        .append( " " )
                                        .append( EVENT_TYPE_COLUMN_NAME )
                                        .append( " = '" )
                                        .append( EventType.FIREBREAK_EVENT.name() )
                                        .append( "' " );

        // execute the query
        List<Serie> series = getConnection().getDBReader().query( queryString.toString(), TimeUnit.MILLISECONDS );

        EnumMap<FirebreakType, FirebreakEvent> results = new EnumMap<>( FirebreakType.class );

        // we only care about the latest event of each type, but there may be older events.
        // scan through the series until we have found both a perf and capacity event, or exhausted the list
        for ( Serie s : series ) {

            List<? extends Event> fbevents = convertSeriesToEvents( s );
            for ( Event e : fbevents ) {
                // unchecked cast.  if they are not FirebreakEvents, then the above query is broken.
                FirebreakEvent fbe = (FirebreakEvent) e;
                results.putIfAbsent( fbe.getFirebreakType(), fbe );

                if ( results.size() == FirebreakType.values().length ) {
                    break;
                }
            }

            if ( results.size() == FirebreakType.values().length ) {
                break;
            }

        }

        return results;
    }

    @Override
    public FirebreakEvent findLatestFirebreak( Volume v, FirebreakType type ) {
        EnumMap<FirebreakType, FirebreakEvent> results = findLatestFirebreaks( v );
        return results.getOrDefault( type, null );
    }

    /**
     * @return the latest active firebreaks in the last 24 hours across all volumes
     */
    @Override
    public Map<Long, EnumMap<FirebreakType, FirebreakEvent>> findLatestFirebreaks() {

        // create base query (select * from EVENT_SERIES_NAME where
        QueryCriteria criteria = new QueryCriteria( QueryType.FIREBREAK_EVENT, DateRange.last24Hours() );
        String queryBase = formulateQueryString( criteria, FBEVENT_VOL_ID_COLUMN_NAME );
        String queryString = new StringBuilder( queryBase )
                                 .append( " " )
                                 .append( AND )
                                 .append( " " )
                                 .append( EVENT_TYPE_COLUMN_NAME )
                                 .append( " = '" )
                                 .append( EventType.FIREBREAK_EVENT.name() )
                                 .append( "' " ).toString();

        // execute the query
        List<Serie> series = getConnection().getDBReader().query( queryString, TimeUnit.MILLISECONDS );

        // convert from influxdb format to FDS model format
        List<? extends Event> datapoints = convertSeriesToEvents( series );

        // TODO: can we map as Map<Volume, EnumMap<FirebreakType, FirebreakEvent>> in one stream expression?
        List<FirebreakEvent> fbevents = datapoints.stream()
                                                  .map( event -> (FirebreakEvent) event )
                                                  .sorted( ( e1, e2 ) -> e1.getVolumeId()
                                                                           .compareTo( e2.getVolumeId() ) )
                                                  .collect( Collectors.toList() );

        Map<Long, EnumMap<FirebreakType, FirebreakEvent>> results = new HashMap<>();
        fbevents.stream()
                .forEach( f -> {
                    long volId = f.getVolumeId();
                    FirebreakType type = f.getFirebreakType();
                    EnumMap<FirebreakType, FirebreakEvent> pt = results.get( volId );
                    if ( pt == null ) {
                        pt = new EnumMap<>( FirebreakType.class );
                        results.put( volId, pt );
                    }
                    pt.put( type, f );
                } );

        return results;
    }

	@Override
    protected <R extends Event> R doPersist( R entity ) {

		Serie s = new Serie.Builder(EVENT_SERIES_NAME)
                           .columns( EVENT_COLUMN_NAMES_A )
                           .values( toSerieRow( entity ) )
                           .build();
        getConnection().getAsyncDBWriter().write( TimeUnit.MILLISECONDS, s );
        return entity;
	}

    @Override
    protected <R extends Event> List<R> doPersist(Collection<R> entities ) {
        Serie.Builder sb = new Serie.Builder(EVENT_SERIES_NAME)
                                    .columns( EVENT_COLUMN_NAMES_A );

        for (R e : entities) {
            sb.values( toSerieRow( e ) );
        }

        Serie s = sb.build();
        getConnection().getAsyncDBWriter().write( TimeUnit.MILLISECONDS, s );
        return (entities instanceof List ? ((List)entities) : new ArrayList<>( entities ) );
    }

    @Override
    protected void doDelete(Event entity) {
        // TODO Auto-generated method stub
    }

    /**
     * Convert an influxDB return type into VolumeDatapoints that we can use
     *
     * @param series the series to convert
     *
     * @return the list of events
     */
    protected List<? extends Event> convertSeriesToEvents( List<Serie> series ) {

        final List<Event> events = new ArrayList<Event>();

        if ( series == null || series.size() == 0 ) {
            return events;
        }

        if (series.size() > 1) {
            logger.warn( "Expecting only one event series.  Skipping " + (series.size() - 1) + " unexpected series." );
        }

        series.stream().forEach( serie -> events.addAll( convertSeriesToEvents( serie ) ) );

        return events;
    }

    /**
     *
     * @param serie the serie to convert
     * @return the list of event from the series
     */
    protected List<? extends Event> convertSeriesToEvents( Serie serie ) {

        final List<Event> events = new ArrayList<Event>();

        if ( serie == null || serie.getRows().size() == 0 ) {
            return events;
        }

        int numRows = serie.getRows().size();
        for ( int i = 0; i < numRows; i++ ) {
            events.add( convertSeriesRowToEvent( serie, i ) );
        }
        return events;
    }

    /**
     *
     * @param serie the serie to convert
     * @param i the row from the serie to convert to an event
     *
     * @return the event
     */
    protected Event convertSeriesRowToEvent( Serie serie, int i ) {
        Map<String, Object> row = serie.getRows().get( i );

        Long ts = ( (Double)row.get( getTimestampColumnName() ) ).longValue();
        EventType type = EventType.valueOf( row.get( EVENT_TYPE_COLUMN_NAME ).toString().toUpperCase() );
        EventCategory category = EventCategory.valueOf( row.get( EVENT_CATEGORY_COLUMN_NAME ).toString().toUpperCase( ) );
        EventSeverity severity = EventSeverity.valueOf( row.get( EVENT_SEVERITY_COLUMN_NAME ).toString().toUpperCase( ) );
        String key = row.get( EVENT_KEY_COLUMN_NAME ).toString();
        String dflt = row.getOrDefault( EVENT_DFLT_MESSAGE_COLUMN_NAME, "" ).toString();

        //
        String args = row.getOrDefault( EVENT_ARGS_COLUMN_NAME, "" ).toString();

        // TODO: this is probably not correct...
        Object[] arga = ((Object[])ObjectModelHelper.toObject( args, Object[].class ));

        switch (type) {
            case SYSTEM_EVENT:
                return new SystemActivityEvent(ts, category, severity, dflt, key, arga);

            case USER_ACTIVITY:
                long userId = ((Number)row.getOrDefault( USER_EVENT_USERID_COLUMN_NAME, -1L )).longValue();
                return new UserActivityEvent(ts, userId, category, severity, dflt, key, arga);

            case FIREBREAK_EVENT:

                String fbtypeStr = row.getOrDefault( FBEVENT_TYPE_COLUMN_NAME, "" ).toString();
                long volId = ((Number)row.getOrDefault( FBEVENT_VOL_ID_COLUMN_NAME, -1L )).longValue();
                String volName = row.getOrDefault( FBEVENT_VOL_NAME_COLUMN_NAME, "" ).toString();
                long usage = ((Number)row.getOrDefault( FBEVENT_USAGE_COLUMN_NAME, 0L )).longValue();
                double sigma = ((Number)row.getOrDefault( FBEVENT_SIGMA_COLUMN_NAME, 0D )).doubleValue();

                // TODO: get correct tenant for volume?  (if not storing in influx, requires separate configdb access)
                return new FirebreakEvent( new Volume( 0, Long.toString( volId ), "", volName ),
                                           FirebreakType.valueOf( fbtypeStr.trim().toUpperCase( Locale.US ) ) ,
                                           ts,
                                           usage,
                                           sigma );

            default:
                throw new IllegalStateException( "Unsupported event type " + type );
        }
    }

    /**
     * Convert the event to an InfluxDB Serie
     *
     * @param e the event to convert.
     *
     * @return the Serie
     */
    protected Object[] toSerieRow(Event e) {
        /*
            TIMESTAMP_COLUMN_NAME,
            EVENT_TYPE_COLUMN_NAME,
            EVENT_CATEGORY_COLUMN_NAME,
            EVENT_SEVERITY_COLUMN_NAME,
            EVENT_KEY_COLUMN_NAME,
            EVENT_DFLT_MESSAGE_COLUMN_NAME,
            EVENT_ARGS_COLUMN_NAME,
            USER_EVENT_USERID_COLUMN_NAME,
            FBEVENT_TYPE_COLUMN_NAME,
            FBEVENT_VOL_ID_COLUMN_NAME,
            FBEVENT_VOL_NAME_COLUMN_NAME,
            FBEVENT_USAGE_COLUMN_NAME,
            FBEVENT_SIGMA_COLUMN_NAME;
         */
        return new Object[] {
            e.getInitialTimestamp(),
            e.getType().name(),
            e.getCategory().name(),
            e.getSeverity().name(),
            e.getMessageKey(),
            e.getDefaultMessage(),
            messageArgJsonString( e ),
            userId( e ),
            fbEventType( e ),
            fbEventVolId( e ),
            fbEventVolName( e ),
            fbEventVolUsage( e ),
            fbEventSigma( e )
        };
    }

    private String messageArgJsonString( Event e ) {
        return ObjectModelHelper.toJSON( e.getMessageArgs() );
    }

    private long userId(Event e) {
        if (e instanceof UserActivityEvent)
            return ((UserActivityEvent)e).getUserId();
        else
            return 0;
    }

    private String fbEventType(Event e) {
        if (e instanceof FirebreakEvent)
            return ((FirebreakEvent)e).getFirebreakType().name();
        return "";
    }

    private Long fbEventVolId(Event e) {
        if (e instanceof FirebreakEvent)
            return ((FirebreakEvent)e).getVolumeId();
        return -1L;
    }

    private String fbEventVolName(Event e) {
        if (e instanceof FirebreakEvent)
            return ((FirebreakEvent)e).getVolumeName();
        return "";
    }

    private Long fbEventVolUsage(Event e) {
        if (e instanceof FirebreakEvent)
            return ((FirebreakEvent)e).getCurrentUsageBytes();
        return 0L;
    }

    private Double fbEventSigma(Event e) {
        if (e instanceof FirebreakEvent)
            return ((FirebreakEvent)e).getSigma();
        return 0D;
    }
}
