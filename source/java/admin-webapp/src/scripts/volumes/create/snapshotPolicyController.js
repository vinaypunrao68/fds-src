angular.module( 'volumes' ).controller( 'snapshotPolicyController', ['$scope', '$modal_data_service', '$snapshot_service', '$volume_api', '$filter', function( $scope, $modal_data_service, $snapshot_service, $volume_api, $filter ){

    $scope.policies = [];
    
    var translate = function( key ){
        return $filter( 'translate' )( key );
    };
    
    var dayRules = [
        { name: 'MO', displayName: translate( 'volumes.snapshot.l_on_monday' ) },
        { name: 'TU', displayName: translate( 'volumes.snapshot.l_on_tuesday' ) },
        { name: 'WE', displayName: translate( 'volumes.snapshot.l_on_wednesday' ) },
        { name: 'TH', displayName: translate( 'volumes.snapshot.l_on_thursday' ) },
        { name: 'FR', displayName: translate( 'volumes.snapshot.l_on_friday' ) },
        { name: 'SA', displayName: translate( 'volumes.snapshot.l_on_saturday' ) },
        { name: 'SU', displayName: translate( 'volumes.snapshot.l_on_sunday' ) }
    ];
    
    var monthRules = [
        { name: '1', displayName: translate( 'volumes.snapshot.l_on_1' ) },
        { name: '2', displayName: translate( 'volumes.snapshot.l_on_2' ) },
        { name: '3', displayName: translate( 'volumes.snapshot.l_on_3' ) },
        { name: '4', displayName: translate( 'volumes.snapshot.l_on_4' ) },
        { name: '5', displayName: translate( 'volumes.snapshot.l_on_5' ) },
        { name: '6', displayName: translate( 'volumes.snapshot.l_on_6' ) },
        { name: '7', displayName: translate( 'volumes.snapshot.l_on_7' ) },
        { name: '8', displayName: translate( 'volumes.snapshot.l_on_8' ) },
        { name: '9', displayName: translate( 'volumes.snapshot.l_on_9' ) },
        { name: '10', displayName: translate( 'volumes.snapshot.l_on_10' ) },
        { name: '11', displayName: translate( 'volumes.snapshot.l_on_11' ) },
        { name: '12', displayName: translate( 'volumes.snapshot.l_on_12' ) },
        { name: '13', displayName: translate( 'volumes.snapshot.l_on_13' ) },
        { name: '14', displayName: translate( 'volumes.snapshot.l_on_14' ) },
        { name: '15', displayName: translate( 'volumes.snapshot.l_on_15' ) },
        { name: '16', displayName: translate( 'volumes.snapshot.l_on_16' ) },
        { name: '17', displayName: translate( 'volumes.snapshot.l_on_17' ) },
        { name: '18', displayName: translate( 'volumes.snapshot.l_on_18' ) },
        { name: '19', displayName: translate( 'volumes.snapshot.l_on_19' ) },
        { name: '20', displayName: translate( 'volumes.snapshot.l_on_20' ) },
        { name: '21', displayName: translate( 'volumes.snapshot.l_on_21' ) },
        { name: '22', displayName: translate( 'volumes.snapshot.l_on_22' ) },
        { name: '23', displayName: translate( 'volumes.snapshot.l_on_23' ) },
        { name: '24', displayName: translate( 'volumes.snapshot.l_on_24' ) },
        { name: '25', displayName: translate( 'volumes.snapshot.l_on_25' ) },
        { name: '26', displayName: translate( 'volumes.snapshot.l_on_26' ) },
        { name: '27', displayName: translate( 'volumes.snapshot.l_on_27' ) },
        { name: '28', displayName: translate( 'volumes.snapshot.l_on_28' ) },
        { name: '-1', displayName: translate( 'volumes.snapshot.l_on_l' ) },
    ];
    
    // TODO: Internationalize the time scale so it can use 24 hour time    
    $scope.startTimeChoices = [];
        
    for ( var i = 0; i < 24; i++ ){
        var time = i;

        if ( i % 12 === 0 ){
            time = 12;
        }

        time += ':00';

        if ( i > 11 ){
            time += ' p.m.';
        }
        else {
            time += 'a.m.';
        }

        $scope.startTimeChoices.push( { name: time, hour: i } );
    }
        
    var init = function(){

        $scope.timeChoices = [{name: 'Hours'},{name: 'Days'},{name: 'Weeks'},{name: 'Months'},{name: 'Years'}];
            
        var everyday = [{displayName: 'everyday'}];
            


        $scope.policies = [
            {name: 'Daily', displayName: 'Daily', units: $scope.timeChoices[2], value: 0, use: false, rules: everyday, startTime: $scope.startTimeChoices[0], startRule: everyday[0], timeEnabled: true, ruleEnabled: false },
            {name: 'Weekly', displayName: 'Weekly', units: $scope.timeChoices[3], value: 0, use: false, rules: dayRules, startTime: $scope.startTimeChoices[0], startRule: dayRules[6], timeEnabled: false, ruleEnabled: true },
            {name: 'Monthly', displayName: 'Monthly', units: $scope.timeChoices[4], value: 0, use: false, rules: monthRules, startTime: $scope.startTimeChoices[0], startRule: monthRules[0], timeEnabled: false, ruleEnabled: true }];

        $scope.editing = false;
    };
    
    $scope.updatePolicyDefs = function(){

        var policyList = [];

        for ( var i = 0; i < $scope.policies.length; i++ ){

            if ( $scope.policies[i].use === true ){

                var tempPolicy = $scope.policies[i];

                var policy = {
                    name: tempPolicy.name.toUpperCase(),
                    recurrenceRule: {
                        FREQ: tempPolicy.name.toUpperCase(),
                        BYHOUR: [tempPolicy.startTime.hour]
                    },
                    retention: $snapshot_service.convertTimePeriodToSeconds( tempPolicy.value, tempPolicy.units.name.toUpperCase() )
                };
                
                // if it's a weekly
                if ( tempPolicy === $scope.policies[1] ){
                    policy.recurrenceRule.BYDAY = [tempPolicy.startRule.name];
                }
                
                // if it's a monthly
                if ( tempPolicy === $scope.policies[2] ){
                    policy.recurrenceRule.BYMONTHDAY = [tempPolicy.startRule.name];
                }
                
                if ( angular.isDefined( tempPolicy.id ) ){
                    policy.id = tempPolicy.id;
                }
                
                if ( angular.isDefined( tempPolicy.use ) ){
                    policy.use = tempPolicy.use;
                }
                
                if ( angular.isDefined( tempPolicy.displayName ) ){
                    policy.displayName = tempPolicy.displayName;
                }

                policyList.push( policy );
            }
        }

        $modal_data_service.update({
            snapshotPolicies: policyList
        });
    };

    $scope.done = function(){
        $scope.updatePolicyDefs();
        $scope.editing = false;
    };
    
    var translatePoliciesToScreen =  function( realPolicies ){

        // set the UI policies to the real data.
        for ( var i = 0; i < realPolicies.length; i++ ){
            for ( var j = 0; j < $scope.policies.length; j++ ){
                if ( realPolicies[i].name.indexOf( $scope.policies[j].name.toUpperCase() ) != -1 ){
                    $scope.policies[j].name = realPolicies[i].name;
                    $scope.policies[j].use = true;

                    var ret = $snapshot_service.convertSecondsToTimePeriod( realPolicies[i].retention );
                    $scope.policies[j].value = ret.value;
                    $scope.policies[j].units = $scope.timeChoices[ret.units];
                    $scope.policies[j].id = realPolicies[i].id;
                    
                    // set the hour
                    if ( angular.isDefined( realPolicies[i].recurrenceRule.BYHOUR ) && realPolicies[i].recurrenceRule.BYHOUR.length > 0 ){
                        $scope.policies[0].startTime = $scope.startTimeChoices[ realPolicies[i].recurrenceRule.BYHOUR ];
                    }
                    else {
                        $scope.policies[0].startTime = $scope.startTimeChoices[ 1 ];
                    }
                    
                    // set the day rule
                    if ( $scope.policies[j].rules === dayRules ){
                        var day = realPolicies[i].recurrenceRule.BYDAY[0];
                        
                        for ( var k = 0; k < dayRules.length; k++ ){
                            if ( dayRules[k].name === day ){
                                $scope.policies[j].startRule = dayRules[k];
                                break;
                            }
                        }
                    }
                    
                    // set the month rules
                    if ( $scope.policies[j].rules === monthRules ){
                        var monthDay = realPolicies[i].recurrenceRule.BYMONTHDAY[0];
                        
                        for ( var l = 0; l < monthRules.length; l++ ){
                            if ( monthRules[l].name === monthDay ){
                                $scope.policies[j].startRule = monthRules[l];
                                break;
                            }
                        }
                    }
                }
            }
        }

        $scope.updatePolicyDefs();
    };

    $scope.$on( 'change', function(){
        
        for ( var i = 1; i < $scope.policies.length; i++ ){
            $scope.policies[i].startTime = $scope.policies[0].startTime;
        }
        
        $scope.updatePolicyDefs 
    });

    $scope.$watch( 'volumeVars.creating', function( newVal ){
        if ( newVal === true ){
            init();
        }
    });
    
    $scope.$watch( 'volumeVars.clone', function( newVal ){
    
        if ( !angular.isDefined( newVal ) || newVal == null ){
            return;
        }
        
        $volume_api.getSnapshotPoliciesForVolume( $scope.volumeVars.clone.id, function( realPolicies ){
            
            // get rid of the IDs... it shouldn't have any
            for ( var i = 0; i < realPolicies.length; i++ ){
                realPolicies[i].name = realPolicies[i].name.substring( realPolicies[i].name.indexOf( '_' ) + 1 );
                realPolicies[i].recurrenceRule.FREQ = realPolicies[i].name;
            }
            
            translatePoliciesToScreen( realPolicies );
        });
    });
    
    $scope.$watch( 'volumeVars.editing', function( newVal ){
        
        if ( newVal === false || !angular.isDefined( newVal ) ){
            return;
        }
        
        init();
        
        $volume_api.getSnapshotPoliciesForVolume( $scope.volumeVars.selectedVolume.id, function( realPolicies ){
            $scope.volumeVars.selectedVolume.snapshotPolicies = realPolicies;
            translatePoliciesToScreen( realPolicies );
        });
           
    });

}]);
