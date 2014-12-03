angular.module( 'volumes' ).directive( 'protectionPolicyPanel', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        // protectionPolicies = { continuous: <num>, snapshotPolicies: [] }
        scope: { protectionPolicies: '=ngModel' },
        templateUrl: 'scripts/directives/fds-components/snapshot-policies/protection-policy.html',
        controller: function( $scope, $filter ){
            
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

            $scope.sliders = [
                {
                    value: { range: 1, value: 1 },
                    name: $filter( 'translate' )( 'volumes.l_continuous' )
                },
                {
                    value: { range: 1, value: 2 },
                    name: $filter( 'translate' )( 'common.l_days' )
                },
                {
                    value: { range: 2, value: 2 },
                    name: $filter( 'translate' )( 'common.l_weeks' )
                },
                {
                    value: { range: 3, value: 60 },
                    name: $filter( 'translate' )( 'common.l_months' )
                },
                {
                    value: { range: 4, value: 10 },
                    name: $filter( 'translate' )( 'common.l_years' )
                }
            ];

            $scope.range = [
                //0
                {
                    name: $filter( 'translate' )( 'common.l_hours' ).toLowerCase(),
                    start: 0,
                    end: 24,
                    segments: 1,
                    width: 5,
                    min: 24,
                    selectable: false
                },
                //1
                {
                    name: $filter( 'translate' )( 'common.l_days' ).toLowerCase(),
                    selectName: $filter( 'translate' )( 'common.l_days' ),
                    start: 1,
                    end: 7,
                    width: 20,
                    labelFunction: pluralLabelFixer( $filter( 'translate' )( 'common.l_days' ).toLowerCase(),
                        $filter( 'translate' )( 'common.l_day' ).toLowerCase() )            
                },
                //2
                {
                    name: $filter( 'translate' )( 'common.l_weeks' ).toLowerCase(),
                    selectName: $filter( 'translate' )( 'common.l_weeks' ),
                    start: 1,
                    end: 4,
                    width: 15,
                    labelFunction: pluralLabelFixer( $filter( 'translate' )( 'common.l_weeks' ).toLowerCase(),
                        $filter( 'translate' )( 'common.l_week' ).toLowerCase() )            
                },
                //3
                {
                    name: $filter( 'translate' )( 'common.l_days' ),
                    selectName: $filter( 'translate' )( 'common.l_months_by_days' ),
                    start: 30,
                    end: 360,
                    segments: 11,
                    width: 30,
                    labelFunction: pluralLabelFixer( $filter( 'translate' )( 'common.l_days' ).toLowerCase(),
                        $filter( 'translate' )( 'common.l_day' ).toLowerCase() )
                },
                //4
                {
                    name: $filter( 'translate' )( 'common.l_years' ),
                    selectName: $filter( 'translate' )( 'common.l_years' ).toLowerCase(),
                    start: 1,
                    end: 16,
                    width: 25,
                    labelFunction: pluralLabelFixer( $filter( 'translate' )( 'common.l_years' ).toLowerCase(),
                        $filter( 'translate' )( 'common.l_year' ).toLowerCase() )
                },
                //5
                {
                    name: $filter( 'translate' )( 'common.l_years' ).toLowerCase(),
                    selectName: $filter( 'translate' )( 'common.l_forever' ).toLowerCase(),
                    allowNumber: false,
                    start: 15,
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
///////////////////////////////////////////////////////////////////////////////////            
            
            /**
            * This is the real controller logic
            **/
            
            var watcher;
            $scope.editing = false;
            
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
            
            var buildPolicy = function( slider, frequency ){
                
                // find retention
                var retention = 0;
                switch( slider.value.range ){
                    case 1:
                        retention = slider.value.value * 24*60*60;
                        break;
                    case 2:
                        retention = slider.value.value * 7 * 24*60*60;
                        break;
                    case 3:
                        retention = slider.value.value * 24*60*60;
                        break;
                    case 4:
                        retention = slider.value.value * 366*24*60*60;
                        break;
                    default:
                        retention = 0;
                }
                
                var policy = {
                    retention: retention,
                    recurrenceRule: {FREQ: frequency},
                    BYHOUR: [0],
                    BYMONTHDAY: [0],
                    BYYEAR: [0],
                    BYDAY: ['SU'],
                    
                    // these two are set when the policies come in and are set on the widget
                    name: slider.policyName,
                    id: slider.policyId
                };
                
                return policy;
            };
            
            var translatePoliciesToScreen = function(){
            
                // do continuous first... we expect it to always be at least 1 day and no partial days
                var cDays = $scope.protectionPolicies.continuous / (1000*60*60*24);
                setSliderValue( cDays, $scope.sliders[0] );
                
                for ( var i = 0; angular.isDefined( $scope.protectionPolicies.policies ) && i < $scope.protectionPolicies.policies.length; i++ ){
                    
                    var policy = $scope.protectionPolicies.policies[i];
                    var slider = {};
                    
                    switch( policy.recurrenceRule.FREQ ){
                        case 'DAILY':
                            slider = $scope.sliders[1];
                            break;
                        case 'WEEKLY':
                            slider = $scope.sliders[2];
                            break;
                        case 'MONTHLY':
                            slider = $scope.sliders[3];
                            break;
                        case 'YEARLY':
                            slider = $scope.sliders[4];
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
                
                $scope.protectionPolicies.continuous = $scope.sliders[0].value.value * 1000 *60 *60 *24;
                
                $scope.protectionPolicies.policies = [];
                $scope.protectionPolicies.policies.push( buildPolicy( $scope.sliders[1], 'DAILY' ) );
                $scope.protectionPolicies.policies.push( buildPolicy( $scope.sliders[2], 'WEEKLY' ) );
                $scope.protectionPolicies.policies.push( buildPolicy( $scope.sliders[3], 'MONTHLY' ) );
                $scope.protectionPolicies.policies.push( buildPolicy( $scope.sliders[4], 'YEARLY' ) );
                
                initWatcher();
            };
            
            $scope.save = function(){
                translateScreenToPolicies();
                $scope.editing = false;
                
                $scope.$emit( 'fds::protection_policy_changed' );
            };
            
            initWatcher();
            
        }
    };
    
});