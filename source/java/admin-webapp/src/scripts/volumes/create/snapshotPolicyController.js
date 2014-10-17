angular.module( 'volumes' ).controller( 'snapshotPolicyController', ['$scope', '$modal_data_service', '$snapshot_service', '$volume_api', function( $scope, $modal_data_service, $snapshot_service, $volume_api ){

    $scope.policies = [];
    
    var init = function(){

        $scope.timeChoices = [{name: 'Hours'},{name: 'Days'},{name: 'Weeks'},{name: 'Months'},{name: 'Years'}];

        $scope.policies = [
//            {name: 'Hourly', displayName: 'Hourly', units: $scope.timeChoices[1], value: 0, use: false},
            {name: 'Daily', displayName: 'Daily', units: $scope.timeChoices[2], value: 0, use: false},
            {name: 'Weekly', displayName: 'Weekly', units: $scope.timeChoices[3], value: 0, use: false},
            {name: 'Monthly', displayName: 'Monthly', units: $scope.timeChoices[4], value: 0, use: false},
            {name: 'Yearly', displayName: 'Yearly', units: $scope.timeChoices[4], value: 0, use: false}];

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
                        FREQ: tempPolicy.name.toUpperCase()
                    },
                    retention: $snapshot_service.convertTimePeriodToSeconds( tempPolicy.value, tempPolicy.units.name.toUpperCase() )
                };
                
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
                }
            }
        }

        $scope.updatePolicyDefs();
    };

    $scope.$on( 'change', $scope.updatePolicyDefs );

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
