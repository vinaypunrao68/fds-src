package com.formationds.om.webkit.rest.v08.presets;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.eclipse.jetty.server.Request;

import com.formationds.client.ical.Numbers;
import com.formationds.client.ical.RecurrenceRule;
import com.formationds.client.ical.WeekDays;
import com.formationds.client.ical.iCalFrequency;
import com.formationds.client.v08.model.DataProtectionPolicyPreset;
import com.formationds.client.v08.model.SnapshotPolicy;
import com.formationds.client.v08.model.SnapshotPolicy.SnapshotPolicyType;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;

public class GetDataProtectionPolicyPresets implements RequestHandler{

	private List<DataProtectionPolicyPreset> dataProtectionPresets;
	private DataProtectionPolicyPreset sparsePreset;
	private DataProtectionPolicyPreset normalPreset;
	private DataProtectionPolicyPreset densePreset;
	
	@Override
	public Resource handle(Request request, Map<String, String> routeParameters)
			throws Exception {
		
		String jsonString = ObjectModelHelper.toJSON( getDataProtectionPolicyPresets() );
		
		return new TextResource( jsonString );
	}
	
	public List<DataProtectionPolicyPreset> getDataProtectionPolicyPresets(){
		
		if ( dataProtectionPresets == null ){
			dataProtectionPresets = new ArrayList<>();
			
			dataProtectionPresets.add( getSparsePreset() );
			dataProtectionPresets.add( getNormalPreset() );
			dataProtectionPresets.add( getDensePreset() );
		}
		
		return dataProtectionPresets;
	}
	
	private DataProtectionPolicyPreset getSparsePreset(){
		
		if ( sparsePreset == null ){
			
			Numbers<Integer> zero = new Numbers<Integer>();
			zero.add( 0 );
			
			Numbers<Integer> one = new Numbers<Integer>();
			one.add( 1 );
			
			WeekDays<String> monday = new WeekDays<String>();
			monday.add( "MO" );
			
			Duration days = Duration.ofDays( 2 );
			RecurrenceRule dayRule = new RecurrenceRule();
			dayRule.setFrequency( iCalFrequency.DAILY.name() );
			dayRule.setHours( zero );
			dayRule.setMinutes( zero );
			
			SnapshotPolicy dayPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, dayRule, days );
			
			Duration weeks = Duration.ofDays( 7 );
			RecurrenceRule weekRule = new RecurrenceRule();
			weekRule.setFrequency( iCalFrequency.WEEKLY.name() );
			weekRule.setHours( zero );
			weekRule.setMinutes( zero );
			weekRule.setDays( monday );

			SnapshotPolicy weekPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, weekRule, weeks );
			
			Duration months = Duration.ofDays( 90 );
			RecurrenceRule monthRule = new RecurrenceRule();
			monthRule.setHours( zero );
			monthRule.setMinutes( zero );
			monthRule.setMonthDays( one );
			
			SnapshotPolicy monthPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, monthRule, months );
			
			Duration years = Duration.ofDays( 2L * 366L );
			RecurrenceRule yearRule = new RecurrenceRule();
			yearRule.setHours( zero );
			yearRule.setMinutes( zero );
			yearRule.setMonths( one );
			yearRule.setMonthDays( one );
				
			SnapshotPolicy yearPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, yearRule, years );
			
			List<SnapshotPolicy> snapshotPolicies = new ArrayList<>();
			snapshotPolicies.add( dayPolicy );
			snapshotPolicies.add( weekPolicy );
			snapshotPolicies.add( monthPolicy );
			snapshotPolicies.add( yearPolicy );
			
			
			sparsePreset = new DataProtectionPolicyPreset( 0L, "Sparse Coverage", Duration.ofDays( 1 ), snapshotPolicies );
		}
		
		return sparsePreset;
	}
	
	private DataProtectionPolicyPreset getNormalPreset(){
		
		if ( normalPreset == null ){

			Numbers<Integer> zero = new Numbers<Integer>();
			zero.add( 0 );
			
			Numbers<Integer> one = new Numbers<Integer>();
			one.add( 1 );
			
			WeekDays<String> monday = new WeekDays<String>();
			monday.add( "MO" );
			
			Duration days = Duration.ofDays( 7 );
			RecurrenceRule dayRule = new RecurrenceRule();
			dayRule.setFrequency( iCalFrequency.DAILY.name() );
			dayRule.setHours( zero );
			dayRule.setMinutes( zero );
			
			SnapshotPolicy dayPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, dayRule, days );
			
			Duration weeks = Duration.ofDays( 90 );
			RecurrenceRule weekRule = new RecurrenceRule();
			weekRule.setFrequency( iCalFrequency.WEEKLY.name() );
			weekRule.setHours( zero );
			weekRule.setMinutes( zero );
			weekRule.setDays( monday );

			SnapshotPolicy weekPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, weekRule, weeks );
			
			Duration months = Duration.ofDays( 180 );
			RecurrenceRule monthRule = new RecurrenceRule();
			monthRule.setHours( zero );
			monthRule.setMinutes( zero );
			monthRule.setMonthDays( one );
			
			SnapshotPolicy monthPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, monthRule, months );
			
			Duration years = Duration.ofDays( 5L * 366L );
			RecurrenceRule yearRule = new RecurrenceRule();
			yearRule.setHours( zero );
			yearRule.setMinutes( zero );
			yearRule.setMonths( one );
			yearRule.setMonthDays( one );
				
			SnapshotPolicy yearPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, yearRule, years );
			
			List<SnapshotPolicy> snapshotPolicies = new ArrayList<>();
			snapshotPolicies.add( dayPolicy );
			snapshotPolicies.add( weekPolicy );
			snapshotPolicies.add( monthPolicy );
			snapshotPolicies.add( yearPolicy );
			
			
			normalPreset = new DataProtectionPolicyPreset( 1L, "Standard Coverage", Duration.ofDays( 1 ), snapshotPolicies );			
		}
		
		return normalPreset;
		
	}
	
	private DataProtectionPolicyPreset getDensePreset(){
	
		if ( densePreset == null ){
			
			Numbers<Integer> zero = new Numbers<Integer>();
			zero.add( 0 );
			
			Numbers<Integer> one = new Numbers<Integer>();
			one.add( 1 );
			
			WeekDays<String> monday = new WeekDays<String>();
			monday.add( "MO" );
			
			Duration days = Duration.ofDays( 30 );
			RecurrenceRule dayRule = new RecurrenceRule();
			dayRule.setFrequency( iCalFrequency.DAILY.name() );
			dayRule.setHours( zero );
			dayRule.setMinutes( zero );
			
			SnapshotPolicy dayPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, dayRule, days );
			
			Duration weeks = Duration.ofDays( 210 );
			RecurrenceRule weekRule = new RecurrenceRule();
			weekRule.setFrequency( iCalFrequency.WEEKLY.name() );
			weekRule.setHours( zero );
			weekRule.setMinutes( zero );
			weekRule.setDays( monday );

			SnapshotPolicy weekPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, weekRule, weeks );
			
			Duration months = Duration.ofDays( 2 * 366 );
			RecurrenceRule monthRule = new RecurrenceRule();
			monthRule.setHours( zero );
			monthRule.setMinutes( zero );
			monthRule.setMonthDays( one );
			
			SnapshotPolicy monthPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, monthRule, months );
			
			Duration years = Duration.ofDays( 15L * 366L );
			RecurrenceRule yearRule = new RecurrenceRule();
			yearRule.setHours( zero );
			yearRule.setMinutes( zero );
			yearRule.setMonths( one );
			yearRule.setMonthDays( one );
				
			SnapshotPolicy yearPolicy = new SnapshotPolicy( SnapshotPolicyType.SYSTEM_TIMELINE, yearRule, years );
			
			List<SnapshotPolicy> snapshotPolicies = new ArrayList<>();
			snapshotPolicies.add( dayPolicy );
			snapshotPolicies.add( weekPolicy );
			snapshotPolicies.add( monthPolicy );
			snapshotPolicies.add( yearPolicy );
			
			
			densePreset = new DataProtectionPolicyPreset( 2L, "Dense Coverage", Duration.ofDays( 2 ), snapshotPolicies );				
		}
		
		return densePreset;
	}
}
