angular.module( 'volumes' ).directive( 'protectionPolicyPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        // protectionPolicies = { continuous: <num>, snapshotPolicies: [] }
        scope: { protectionPolicies: '=ngModel', saveOnly: '@' },
        templateUrl: 'scripts/directives/fds-components/snapshot-policies/protection-policy.html',
        controller: function( $scope, $filter ){
            
            if ( !angular.isDefined( $scope.saveOnly ) ){
                $scope.saveOnly = false;
            }
            
            /**
            * This is all the range settings for our waterfall widget.
            * very little functionality is in here outside of defining the rules
            * for our waterfall slider
            **/
            
            var pluralLabelFixer = function( plural, singular ){

                return function( value ){

                    var str = value + ' ';

                    if ( value === 1 ){
                        str += singular;
                    }
                    else {
                        str += plural;
                    }

                    return str;
                };
            };
            
            var translate = function( key ){
                return $filter( 'translate' )( key );
            };

            $scope.sliders = [
                {
                    value: { range: 1, value: 1 },
                    name: translate( 'volumes.l_continuous' )
                },
                {
                    value: { range: 1, value: 2 },
                    name: translate( 'common.l_days' )
                },
                {
                    value: { range: 2, value: 2 },
                    name: translate( 'common.l_weeks' )
                },
                {
                    value: { range: 3, value: 60 },
                    name: translate( 'common.l_months' )
                },
                {
                    value: { range: 4, value: 10 },
                    name: translate( 'common.l_years' )
                }
            ];

            $scope.range = [
                //0
                {
                    name: translate( 'common.l_hours' ).toLowerCase(),
                    start: 0,
                    end: 24,
                    segments: 1,
                    width: 5,
                    min: 24,
                    selectable: false
                },
                //1
                {
                    name: translate( 'common.l_days' ),
                    selectName: translate( 'common.l_days' ),
                    start: 1,
                    end: 7,
                    width: 20,
                    labelFunction: pluralLabelFixer( translate( 'common.l_days' ).toLowerCase(),
                        translate( 'common.l_day' ).toLowerCase() )            
                },
                //2
                {
                    name: translate( 'common.l_weeks' ),
                    selectName: translate( 'common.l_weeks' ),
                    start: 1,
                    end: 4,
                    width: 15,
                    labelFunction: pluralLabelFixer( translate( 'common.l_weeks' ).toLowerCase(),
                        translate( 'common.l_week' ).toLowerCase() )            
                },
                //3
                {
                    name: translate( 'common.l_days' ),
                    selectName: translate( 'common.l_months_by_days' ),
                    start: 30,
                    end: 360,
                    segments: 11,
                    width: 30,
                    labelFunction: pluralLabelFixer( translate( 'common.l_days' ).toLowerCase(),
                        translate( 'common.l_day' ).toLowerCase() )
                },
                //4
                {
                    name: translate( 'common.l_years' ),
                    selectName: translate( 'common.l_years' ),
                    start: 1,
                    end: 16,
                    width: 25,
                    labelFunction: pluralLabelFixer( translate( 'common.l_years' ).toLowerCase(),
                        translate( 'common.l_year' ).toLowerCase() )
                },
                //5
                {
                    name: translate( 'common.l_forever' ).toLowerCase(),
                    selectName: translate( 'common.l_forever' ),
                    allowNumber: false,
                    start: 16,
                    end: 16,
                    width: 5,
                    labelFunction: function( value ){
                        if ( value === 16 ){
                            return '\u221E';
                        }

                        return;
                    }
                }
            ];
            
            // here we set up the drop down values
            $scope.times = [];
            // TODO: Internationalize the time scale so it can use 24 hour time    
            for ( var i = 0; i < 24; i++ ){
                var time = i%12;

                if ( time === 0 ){
                    time = 12;
                }

                time += ':00';

                if ( i > 11 ){
                    time += ' p.m.';
                }
                else {
                    time += 'a.m.';
                }

                $scope.times.push( { displayName: time, hour: i } );
            }
            
            $scope.days = [
                { value: 'MO', displayName: translate( 'volumes.snapshot.l_on_monday' ) },
                { value: 'TU', displayName: translate( 'volumes.snapshot.l_on_tuesday' ) },
                { value: 'WE', displayName: translate( 'volumes.snapshot.l_on_wednesday' ) },
                { value: 'TH', displayName: translate( 'volumes.snapshot.l_on_thursday' ) },
                { value: 'FR', displayName: translate( 'volumes.snapshot.l_on_friday' ) },
                { value: 'SA', displayName: translate( 'volumes.snapshot.l_on_saturday' ) },
                { value: 'SU', displayName: translate( 'volumes.snapshot.l_on_sunday' ) }
            ];
            
            $scope.months = [
                { value: 1, displayName: translate( 'volumes.snapshot.l_first_day_of_the_month' ) },
                { value: -1, displayName: translate( 'volumes.snapshot.l_last_day_of_the_month' ) },
                { value: 1, displayName: translate( 'volumes.snapshot.l_first_weekly' ) },
                { value: -1, displayName: translate( 'volumes.snapshot.l_last_weekly' ) }
            ];
            
            $scope.years = [
                { month: 0, displayName: translate( 'volumes.snapshot.l_january' ) },
                { month: 1, displayName: translate( 'volumes.snapshot.l_february' ) },
                { month: 2, displayName: translate( 'volumes.snapshot.l_march' ) },
                { month: 3, displayName: translate( 'volumes.snapshot.l_april' ) },
                { month: 4, displayName: translate( 'volumes.snapshot.l_may' ) },
                { month: 5, displayName: translate( 'volumes.snapshot.l_june' ) },
                { month: 6, displayName: translate( 'volumes.snapshot.l_july' ) },
                { month: 7, displayName: translate( 'volumes.snapshot.l_august' ) },
                { month: 8, displayName: translate( 'volumes.snapshot.l_september' ) },
                { month: 9, displayName: translate( 'volumes.snapshot.l_october' ) },
                { month: 10, displayName: translate( 'volumes.snapshot.l_november' ) },
                { month: 11, displayName: translate( 'volumes.snapshot.l_december' ) }
            ];
///////////////////////////////////////////////////////////////////////////////////            
            
            /**
            * This is the real controller logic
            **/
            
            var watcher;
            $scope.editing = false;
            $scope.hourChoice = $scope.times[0];
            $scope.dayChoice = $scope.days[0];
            $scope.monthChoice = $scope.months[0];
            $scope.yearChoice = $scope.years[0];
            
            var setSliderValue = function( slider, days ){
                
                // years
                if ( days >= 366 ){
                    slider.value = { value: Math.round( days / 366 ), range: 4 };
                }
                // months
                else if ( days >= 30 ){
                    slider.value = { value: days, range: 3 };
                }
                // weeks
                else if ( days >= 7 ){
                    slider.value = { value: Math.round( days / 7 ), range: 2 };
                }
                else if ( days === 0 ){
                    slider.value = { value: 16, range: 5 };
                }
                else {
                    slider.value = { value: days, range: 1 };
                }
            };
            
            var buildPolicies = function(){
                
                var policies = [];
                
                for ( var i = 1; i < $scope.sliders.length; i++ ){
                    var policy = { recurrenceRule: {}};
                    var slider = $scope.sliders[i];
                    
                    policy.name = slider.policyName;
                    policy.id = slider.policyId;
                    
                    // find retention
                    switch( slider.value.range ){
                        case 1:
                            policy.retention = slider.value.value * 24*60*60;
                            break;
                        case 2:
                            policy.retention = slider.value.value * 7 * 24*60*60;
                            break;
                        case 3:
                            policy.retention = slider.value.value * 24*60*60;
                            break;
                        case 4:
                            policy.retention = slider.value.value * 366*24*60*60;
                            break;
                        default:
                            policy.retention = 0;
                    }
                    
                    // set the frequency
                    switch( i ){
                        case 1:
                            policy.recurrenceRule.FREQ = 'DAILY';
                            break;
                        case 2: 
                            policy.recurrenceRule.FREQ = 'WEEKLY';
                            break;
                        case 3:
                            policy.recurrenceRule.FREQ = 'MONTHLY';
                            break;
                        case 4:
                            policy.recurrenceRule.FREQ = 'YEARLY';
                            break;
                        default:
                            policy.recurrenceRule.FREQ = 'DAILY';
                            break;
                    }
                
                    // set the timing variables
                    policy.recurrenceRule.BYHOUR = [$scope.hourChoice.hour];
                    
                    // weeklies or higher
                    if ( i > 1 ){
                        policy.recurrenceRule.BYDAY = [$scope.dayChoice.value];
                    }
                    
                    // monthlies or higher
                    if ( i > 2 ){
                        // first or last day of the month
                        if ( $scope.monthChoice === $scope.months[0] || $scope.monthChoice === $scope.months[1] ){
                            policy.recurrenceRule.BYMONTHDAY = [$scope.monthChoice.value];
                        }
                        else {
                            policy.recurrenceRule.BYWEEKNO = [$scope.monthChoice.value];
                        }
                    }
                    
                    // yearlies
                    if ( i > 3 ){
                        policy.recurrenceRule.BYMONTH = [$scope.yearChoice.month];
                    }
                    
                    policies.push( policy );
                }
                
                return policies;
            };
            
            var translatePoliciesToScreen = function(){
            
                // do continuous first... we expect it to always be at least 1 day and no partial days
                var cDays = $scope.protectionPolicies.continuous / (60*60*24);
                setSliderValue( $scope.sliders[0], cDays );
                
                for ( var i = 0; angular.isDefined( $scope.protectionPolicies.policies ) && i < $scope.protectionPolicies.policies.length; i++ ){
                    
                    var policy = $scope.protectionPolicies.policies[i];
                    var slider = {};
                    
                    switch( policy.recurrenceRule.FREQ ){
                        case 'DAILY':
                            slider = $scope.sliders[1];
                            
                            if ( angular.isDefined( policy.recurrenceRule.BYHOUR ) ){
                                $scope.hourChoice = $scope.times[ policy.recurrenceRule.BYHOUR[0] ];
                            }
                            
                            break;
                        case 'WEEKLY':
                            slider = $scope.sliders[2];
                            
                            if ( !angular.isDefined( policy.recurrenceRule.BYDAY ) ){
                                break;
                            }
                            
                            for ( var dayIt = 0; dayIt < $scope.days.length; dayIt++ ){
                                if ( policy.recurrenceRule.BYDAY[0].indexOf( $scope.days[dayIt].value ) != -1 ){
                                    $scope.dayChoice = $scope.days[dayIt];
                                    break;
                                }
                            }
                            
                            break;
                        case 'MONTHLY':
                            slider = $scope.sliders[3];
                            
                            if ( !angular.isDefined( policy.recurrenceRule.BYMONTHDAY ) && !angular.isDefined( policy.recurrenceRule.BYWEEKNO ) ){
                                break;
                            }
                            
                            if ( angular.isDefined( policy.recurrenceRule.BYMONTHDAY ) ){
                                if ( policy.recurrenceRule.BYMONTHDAY[0] == 1 ){
                                    $scope.monthChoice = $scope.months[0];
                                }
                                else {
                                    $scope.monthChoice = $scope.months[1];
                                }
                            }
                            else {
                                if ( policy.recurrenceRule.BYWEEKNO[0] == 1 ){
                                    $scope.monthChoice = $scope.months[2];
                                }
                                else {
                                    $scope.monthChoice = $scope.months[3];
                                }
                            }
                            
                            break;
                        case 'YEARLY':
                            slider = $scope.sliders[4];
                            
                            if ( !angular.isDefined( policy.recurrenceRule.BYMONTH ) ){
                                break;
                            }
                            
                            $scope.yearChoice = $scope.years[ parseInt( policy.recurrenceRule.BYMONTH[0] ) ];
                            
                            break;
                        default:
                            continue;
                    }
                    
                    // retention time is in seconds
                    setSliderValue( slider, Math.round( policy.retention / (60*60*24)) );
                    
                    // add some items we'll need late
                    slider.policyId = policy.id;
                    slider.policyName = policy.name;
                }
            };
            
            var initWatcher = function(){
                watcher = $scope.$watch( 'protectionPolicies', function(){
                    translatePoliciesToScreen();
                });
            };
            
            var translateScreenToPolicies = function(){
                
                if ( angular.isFunction( watcher ) ){
                    watcher();
                }
                
                $scope.protectionPolicies.continuous = $scope.sliders[0].value.value * 60*60*24;
                
                $scope.protectionPolicies.policies = buildPolicies();
                initWatcher();
            };
            
            $scope.save = function(){
                translateScreenToPolicies();
                $scope.editing = false;
                
                $scope.$emit( 'fds::protection_policy_changed' );
            };
            
            $scope.cancel = function(){
                
                $scope.editing = false;
                translatePoliciesToScreen();
            };
            
            $scope.$on( 'fds::cancel_editing', $scope.cancel );
            
            initWatcher();
            
        }
    };
    
});