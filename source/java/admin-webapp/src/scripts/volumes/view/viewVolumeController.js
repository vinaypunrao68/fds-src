angular.module( 'volumes' ).controller( 'viewVolumeController', ['$scope', '$volume_api', '$modal_data_service', '$snapshot_service', function( $scope, $volume_api, $modal_data_service, $snapshot_service ){

    $scope.snapshots = [];

        $scope.timeChoices = [{name: 'Hours'},{name: 'Days'},{name: 'Weeks'},{name: 'Months'},{name: 'Years'}];

        $scope.policies = [
//            {name: 'Hourly', displayName: 'Hourly', units: $scope.timeChoices[1], value: 0, use: false},
            {name: 'Daily', displayName: 'Daily', units: $scope.timeChoices[2], value: 0, use: false},
            {name: 'Weekly', displayName: 'Weekly', units: $scope.timeChoices[3], value: 0, use: false},
            {name: 'Monthly', displayName: 'Monthly', units: $scope.timeChoices[4], value: 0, use: false},
            {name: 'Yearly', displayName: 'Yearly', units: $scope.timeChoices[4], value: 0, use: false}];
    
        $scope.savedPolicies = [];
    
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

                typeString += types[j].displayName;

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
        
        $snapshot_service.cloneSnapshotToNewVolume( snapshot, newName.trim(), function(){ alert( 'Clone successfully created.');} );
    };
    
    $scope.deleteSnapshot = function( snapshot ){
        
        $volume_api.deleteSnapshot( $scope.$parent.selectedVolume.id, snapshot.id, function(){ alert( 'Snapshot deleted successfully.' );} );
    };

    // basically this is "save my policy changes.
    //
    // Because we don't have a real design yet, this code
    // will likely be completely removed and most of this done on the 
    // server.
    $scope.doneEditing = function(){

        // helper to create a real policy from the UI parts
        var createPolicy = function( makePolicy ){
             var policy = {
                id: makePolicy.id,
                name: makePolicy.name.toUpperCase(),
                recurrenceRule: {
                    FREQ: makePolicy.displayName.toUpperCase()
                },
                retention: $snapshot_service.convertTimePeriodToSeconds( makePolicy.value, makePolicy.units.name.toUpperCase() )
            };
            
            return policy;
        };
        
        $scope.generateSummaryText();

        // for each policy on the screen...
        for ( var i = 0; i < $scope.policies.length; i++ ){

            var sPolicy = $scope.policies[i];
            
            // if it has an ID then it's already exists
            if ( angular.isDefined( sPolicy.id ) ){
                
                // if it's not in use, we need to delete and remove it
                if ( !angular.isDefined( sPolicy.use ) || sPolicy.use === false ){
                    
                    var id = sPolicy.id;
                    
                    $snapshot_service.detachPolicy( sPolicy, $scope.$parent.selectedVolume.id, function( result ){
                        $snapshot_service.deleteSnapshotPolicy( id, function(){} );
                    });
                }
                // else we need to just edit it
                else {
                    $snapshot_service.editSnapshotPolicy( createPolicy( sPolicy ), function(){} );
                }
            }
            else {
                
                // if it's in use, create it.
                if ( $scope.policies[i].use === true ){
                    $snapshot_service.createSnapshotPolicy( createPolicy( sPolicy ), function( policy ){
                        $snapshot_service.attachPolicyToVolume( policy, $scope.$parent.selectedVolume.id, function(){} );
                    } );
                }
            }
        }
        
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

    // when we get shown, get all the snapshots and policies.  THen do the chugging
    // to display the summary and set the hidden forms.
    $scope.$on( 'fds::page_shown', function(){

        $volume_api.getSnapshots( $scope.$parent.selectedVolume.id, function( data ){ $scope.snapshots = data; } );
        $volume_api.getSnapshotPoliciesForVolume( $scope.$parent.selectedVolume.id,
            function( realPolicies ){
                
                $scope.savedPolicies = realPolicies;
                
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
                
                // redo the string
                $scope.generateSummaryText();
            });
    });

    $scope.$on( 'fds::page_hidden', function(){
    });

    $scope.generateSummaryText();

}]);
