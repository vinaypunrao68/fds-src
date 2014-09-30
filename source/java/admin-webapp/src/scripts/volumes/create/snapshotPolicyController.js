angular.module( 'volumes' ).controller( 'snapshotPolicyController', ['$scope', '$modal_data_service', '$snapshot_service', function( $scope, $modal_data_service, $snapshot_service ){

    $scope.policies = [];

    var init = function(){

        $scope.summaryText = '';
        $scope.timeChoices = [{name: 'Hours'},{name: 'Days'},{name: 'Weeks'},{name: 'Months'},{name: 'Years'}];

        $scope.policies = [
//            {name: 'Hourly', displayName: 'Hourly', units: $scope.timeChoices[1], value: 0, use: false},
            {name: 'Daily', displayName: 'Daily', units: $scope.timeChoices[2], value: 0, use: false},
            {name: 'Weekly', displayName: 'Weekly', units: $scope.timeChoices[3], value: 0, use: false},
            {name: 'Monthly', displayName: 'Monthly', units: $scope.timeChoices[4], value: 0, use: false},
            {name: 'Yearly', displayName: 'Yearly', units: $scope.timeChoices[4], value: 0, use: false}];

        $scope.editing = false;

    };

    $scope.generateSummaryText = function(){

        var types = [];

        for ( var i = 0; i < $scope.policies.length; i++ ){

            if ( $scope.policies[i].use === true || $scope.policies[i].use === 'checked' ){
                types.push( $scope.policies[i] );
            }
        }

        var typeString = '';

        for( var j = 0; j < types.length; j++ ){

            if ( types[j].use === true || types[j].use === 'checked' ){

                if ( (j+1) === types.length && j !== 0 ){
                    typeString += ' and ';
                }

                typeString += types[j].name;

                if ( (j+2) < types.length ){
                    typeString += ', ';
                }
            }
        }

        if ( typeString === '' ){
            typeString = 'None';
        }

        $scope.summaryText = typeString;
    };

    $scope.updatePolicyDefs = function(){

        $scope.generateSummaryText();

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

    $scope.$on( 'change', $scope.updatePolicyDefs );

    $scope.$on( 'fds::create_volume_initialize', init );

    $scope.generateSummaryText();

}]);
