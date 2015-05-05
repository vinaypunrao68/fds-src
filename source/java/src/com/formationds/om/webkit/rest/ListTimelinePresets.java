package com.formationds.om.webkit.rest;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.eclipse.jetty.server.Request;

import com.formationds.commons.model.RecurrenceRule;
import com.formationds.commons.model.SnapshotPolicy;
import com.formationds.commons.model.TimelinePreset;
import com.formationds.commons.model.builder.RecurrenceRuleBuilder;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.commons.model.type.iCalFrequency;
import com.formationds.commons.util.Numbers;
import com.formationds.commons.util.WeekDays;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class ListTimelinePresets implements RequestHandler{

	private Long oneday = 24L * 60L * 60L;	
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
		
		List<TimelinePreset> presets = new ArrayList<TimelinePreset>();
		
		presets.add( buildLeast() );
		presets.add( buildStandard() );
		presets.add( buildMosts() );
		
		return new TextResource( ObjectModelHelper.toJSON( presets ) );
	}
	
	private TimelinePreset buildLeast() throws ParseException {
		
		Numbers<Integer> zero = new Numbers<Integer>();
		zero.add( 0 );
		
		WeekDays<String> monday = new WeekDays<String>();
		monday.add( "MO" );
		
		TimelinePreset least = new TimelinePreset();
		least.setName( "Sparse Coverage" );
		least.setCommitLogRetention( oneday );
		
		SnapshotPolicy days = new SnapshotPolicy();
		days.setRetention( 2L * oneday );
		RecurrenceRule dayRule = (new RecurrenceRuleBuilder( iCalFrequency.DAILY )).byHour( zero ).byMinute( zero ).build();
		days.setRecurrenceRule( dayRule.toString() );
		
		SnapshotPolicy weeks = new SnapshotPolicy();
		weeks.setRetention( 7L * oneday );
		RecurrenceRule weekRule = (new RecurrenceRuleBuilder( iCalFrequency.WEEKLY ))
			.byHour( zero )
			.byMinute( zero )
			.byDay( monday )
			.build();
		weeks.setRecurrenceRule( weekRule.toString() );
		
		SnapshotPolicy months = new SnapshotPolicy();
		months.setRetention( 90L * oneday );
		RecurrenceRule monthRule = (new RecurrenceRuleBuilder( iCalFrequency.MONTHLY ))
			.byHour( zero )
			.byMinute( zero )
			.byMonthDay( new Numbers<Integer>(1) )
			.build();
		months.setRecurrenceRule( monthRule.toString() );
		
		SnapshotPolicy years = new SnapshotPolicy();
		years.setRetention( 2L * 366L * oneday );
		RecurrenceRule yearRule = (new RecurrenceRuleBuilder(iCalFrequency.YEARLY ))
			.byHour( zero )
			.byMinute( zero )
			.byMonth( new Numbers<Integer>( 1 ) )
			.byMonthDay( new Numbers<Integer>( 1 ) )
			.build();
		years.setRecurrenceRule( yearRule.toString() );
		
		least.getPolicies().add( days );
		least.getPolicies().add( weeks );
		least.getPolicies().add( months );
		least.getPolicies().add( years );
		
		return least;
	}
	
	private TimelinePreset buildStandard() throws ParseException {
		
		Numbers<Integer> zero = new Numbers<Integer>();
		zero.add( 0 );		
		
		WeekDays<String> monday = new WeekDays<String>();
		monday.add( "MO" );
		
		TimelinePreset standard = new TimelinePreset();
		standard.setName( "Standard" );
		standard.setCommitLogRetention( oneday );
		
		SnapshotPolicy days = new SnapshotPolicy();
		days.setRetention( 7L * oneday );
		RecurrenceRule dayRule = (new RecurrenceRuleBuilder( iCalFrequency.DAILY )).byHour( zero ).byMinute( zero ).build();
		days.setRecurrenceRule( dayRule.toString() );
		
		SnapshotPolicy weeks = new SnapshotPolicy();
		weeks.setRetention( 90L * oneday );
		RecurrenceRule weekRule = (new RecurrenceRuleBuilder( iCalFrequency.WEEKLY ))
			.byHour( zero )
			.byMinute( zero )
			.byDay( monday )
			.build();
		weeks.setRecurrenceRule( weekRule.toString() );
		
		SnapshotPolicy months = new SnapshotPolicy();
		months.setRetention( 180L * oneday );
		RecurrenceRule monthRule = (new RecurrenceRuleBuilder( iCalFrequency.MONTHLY ))
			.byHour( zero )
			.byMinute( zero )
			.byMonthDay( new Numbers<Integer>(1) )
			.build();
		months.setRecurrenceRule( monthRule.toString() );
		
		SnapshotPolicy years = new SnapshotPolicy();
		years.setRetention( 5L * 366L * oneday );
		RecurrenceRule yearRule = (new RecurrenceRuleBuilder(iCalFrequency.YEARLY ))
			.byHour( zero )
			.byMinute( zero )
			.byMonth( new Numbers<Integer>( 1 ) )
			.byMonthDay( new Numbers<Integer>( 1 ) )
			.build();
		years.setRecurrenceRule( yearRule.toString() );
		
		standard.getPolicies().add( days );
		standard.getPolicies().add( weeks );
		standard.getPolicies().add( months );
		standard.getPolicies().add( years );
		
		return standard;
	}

	private TimelinePreset buildMosts() throws ParseException {
		
		Numbers<Integer> zero = new Numbers<Integer>();
		zero.add( 0 );		
		
		WeekDays<String> monday = new WeekDays<String>();
		monday.add( "MO" );
		
		TimelinePreset most = new TimelinePreset();
		most.setName( "Dense Coverage" );
		most.setCommitLogRetention( oneday * 2L );
		
		SnapshotPolicy days = new SnapshotPolicy();
		days.setRetention( 30L * oneday );
		RecurrenceRule dayRule = (new RecurrenceRuleBuilder( iCalFrequency.DAILY )).byHour( zero ).byMinute( zero ).build();
		days.setRecurrenceRule( dayRule.toString() );
		
		SnapshotPolicy weeks = new SnapshotPolicy();
		weeks.setRetention( 210L * oneday );
		RecurrenceRule weekRule = (new RecurrenceRuleBuilder( iCalFrequency.WEEKLY ))
			.byHour( zero )
			.byMinute( zero )
			.byDay( monday )
			.build();
		weeks.setRecurrenceRule( weekRule.toString() );
		
		SnapshotPolicy months = new SnapshotPolicy();
		months.setRetention( 2L * 366L * oneday );
		RecurrenceRule monthRule = (new RecurrenceRuleBuilder( iCalFrequency.MONTHLY ))
			.byHour( zero )
			.byMinute( zero )
			.byMonthDay( new Numbers<Integer>(1) )
			.build();
		months.setRecurrenceRule( monthRule.toString() );
		
		SnapshotPolicy years = new SnapshotPolicy();
		years.setRetention( 15L * 366L * oneday );
		RecurrenceRule yearRule = (new RecurrenceRuleBuilder(iCalFrequency.YEARLY ))
			.byHour( zero )
			.byMinute( zero )
			.byMonth( new Numbers<Integer>( 1 ) )
			.byMonthDay( new Numbers<Integer>( 1 ) )
			.build();
		years.setRecurrenceRule( yearRule.toString() );
		
		most.getPolicies().add( days );
		most.getPolicies().add( weeks );
		most.getPolicies().add( months );
		most.getPolicies().add( years );
		
		return most;
	}
}
/*
{
    label: $filter( 'translate' )( 'volumes.snapshot.l_sparse' ),
    value: [{ range: 1, value: 1 },{range: 1, value: 2},{range: 2, value: 1},{range: 3, value: 30},{range: 4, value: 2}]
},
{
    label: $filter( 'translate' )( 'volumes.snapshot.l_standard' ),
    value: [{ range: 1, value: 1 },{range: 2, value: 1},{range: 3, value: 30},{range: 3, value: 180},{range: 4, value: 5}]                    
},
{
    label: $filter( 'translate' )( 'volumes.snapshot.l_dense' ),
    value: [{ range: 1, value: 2 },{range: 3, value: 30},{range: 3, value: 210},{range: 4, value: 2},{range: 4, value: 15}]                    
},
{
    label: $filter( 'translate' )( 'common.l_custom' ),
    value: undefined
}
];
*/