package com.formationds.om.repository.influxdb;

import java.util.*;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.SystemActivityEvent;
import com.formationds.commons.model.entity.UserActivityEvent;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.repository.EventRepository;
import com.formationds.om.repository.query.QueryCriteria;
import com.google.common.collect.Lists;
import org.influxdb.dto.Serie;

public class InfluxEventRepository extends InfluxRepository<Event, Long> implements EventRepository {

    public static final String EVENT_SERIES_NAME    = "events";
    public static final String EVENT_ID_COLUMN_NAME = "event_id";
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
        return Lists.newArrayList( EVENT_ID_COLUMN_NAME,
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
        // command is silently ignored if the database already exists.
        if ( !super.createDatabase( DEFAULT_EVENT_DB ) ) {
            logger.debug( "Database " + DEFAULT_EVENT_DB.getName() + " already exists." );
        }
        super.open( properties );
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

    public Optional<String> getEventIdColumnName() {
        return Optional.of( EVENT_ID_COLUMN_NAME );
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

    /**
     * Method to create a string from the query object that matches influx format
     *
     * @param queryCriteria the query criteria
     * @return a query string for influx event series based on the criteria
     */
    protected String formulateQueryString( QueryCriteria queryCriteria ) {

        StringBuilder sb = new StringBuilder();

        String prefix = SELECT + " * " + FROM + " " + getEntityName();
        sb.append( prefix );

        if ( queryCriteria.getRange() != null &&
             queryCriteria.getContexts() != null && queryCriteria.getContexts().size() > 0 ) {

            sb.append( " " + WHERE );
        }

        // do time range
        if ( queryCriteria.getRange() != null ) {

            String time = " ( " + getTimestampColumnName() + " >= " + queryCriteria.getRange().getStart() + " " + AND +
                          " " + getTimestampColumnName() + " <= " + queryCriteria.getRange().getEnd() + " ) ";

            sb.append( time );
        }

        return sb.toString();
    }

	@Override
	public List<? extends Event> query(QueryCriteria queryCriteria) {
        // get the query string
        String queryString = formulateQueryString( queryCriteria );

        // execute the query
        List<Serie> series = getConnection().getDBReader().query( queryString, TimeUnit.SECONDS );

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
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public FirebreakEvent findLatestFirebreak(Volume v, FirebreakType type) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	protected <R extends Event> R doPersist(R entity) {

		Serie s = new Serie.Builder(EVENT_SERIES_NAME)
                           .columns( EVENT_COLUMN_NAMES_A )
                           .values( toSerieRow( entity ) )
                           .build();
        getConnection().getDBWriter().write( TimeUnit.MILLISECONDS, s );
        return entity;
	}

    @Override
    protected <R extends Event> List<R> doPersist(Collection<R> entities) {
        Serie.Builder sb = new Serie.Builder(EVENT_SERIES_NAME)
                                    .columns( EVENT_COLUMN_NAMES_A );

        for (R e : entities) {
            sb.values( toSerieRow( e ) );
        }

        Serie s = sb.build();
        getConnection().getDBWriter().write( TimeUnit.MILLISECONDS, s );
        return (entities instanceof List ? ((List)entities) : new ArrayList<>( entities ) );
    }

    @Override
    protected void doDelete(Event entity) {
        // TODO Auto-generated method stub
    }

    /**
     * Convert an influxDB return type into VolumeDatapoints that we can use
     *
     * @param series
     *
     * @return
     */
    protected List<? extends Event> convertSeriesToEvents( List<Serie> series ) {

        final List<Event> events = new ArrayList<Event>();

        if ( series == null || series.size() == 0 ) {
            return events;
        }

        if (series.size() > 1) {
            logger.warn( "Expecting only one event series.  Skipping " + (series.size() - 1) + " unexpected series." );
        }

        events.addAll( series.stream().map( serie -> (Event) convertSeriesToEvents( serie ) )
                             .collect( Collectors.toList() ) );

        return events;
    }

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

    protected Event convertSeriesRowToEvent( Serie serie, int i ) {
        Map<String, Object> row = serie.getRows().get( i );

        Long id = Long.parseLong( row.get( EVENT_ID_COLUMN_NAME ).toString() );
        Long ts = Long.parseLong( row.get( getTimestampColumnName() ).toString() );
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
            EVENT_ID_COLUMN_NAME,
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
            e.getId(),
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

    private String fbEventVolId(Event e) {
        if (e instanceof FirebreakEvent)
            return ((FirebreakEvent)e).getVolumeId();
        return "";
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
