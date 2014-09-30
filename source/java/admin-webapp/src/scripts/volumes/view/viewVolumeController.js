angular.module( 'volumes' ).controller( 'viewVolumeController', ['$scope', '$volume_api', '$modal_data_service', function( $scope, $volume_api, $modal_data_service ){

    $scope.snapshots = [];

    $scope.timeChoices = [{name: 'Hours'},{name: 'Days'},{name: 'Weeks'},{name: 'Months'},{name: 'Years'}];

    $scope.policies = [
        {name: 'Hourly', units: $scope.timeChoices[1], value: 0, use: false},
        {name: 'Daily', units: $scope.timeChoices[2], value: 0, use: false},
        {name: 'Weekly', units: $scope.timeChoices[3], value: 0, use: false},
        {name: 'Monthly', units: $scope.timeChoices[4], value: 0, use: false},
        {name: 'Yearly', units: $scope.timeChoices[4], value: 0, use: false}];

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

    $scope.clone = function( snapshot ){

        var newName = prompt( 'Name for new volume:' );
    };

    $scope.doneEditing = function(){

        $scope.generateSummaryText();
        $scope.editing = false;
    };

    $scope.formatDate = function( ms ){
        var d = new Date( parseInt( ms ) );
        return d.toString();
    };

    $scope.back = function(){
        $scope.$emit( 'fds::volume_done_editing' );
        $scope.$broadcast( 'fds::volume_done_editing' );
    };

    $scope.$on( 'fds::page_shown', function(){

        $volume_api.getSnapshots( $scope.$parent.selectedVolume.id, function( data ){ $scope.snapshots = data; } );
        $volume_api.getSnapshotPoliciesForVolume( $scope.$parent.selectedVolume.id,
            function( policies ){
//                $scope.policies = policies;
            });
    });

    $scope.$on( 'fds::page_hidden', function(){
    });

    $scope.generateSummaryText();

}]);
