angular.module( 'volumes' ).controller( 'volumeController', [ '$scope', '$volume_api', '$element', '$timeout', '$compile', '$snapshot_service', function( $scope, $volume_api, $element, $timeout, $compile, $snapshot_service ){
    
    $scope.date = new Date();
    $scope.volumes = [];
    $scope.capacity = 10;
    $scope.iopLimit = 300;
    $scope.priority = 10;
    $scope.editing = false;
    $scope.editingVolume = {};

    $scope.clicked = function( volume){
        $scope.volumeVars.selectedVolume = volume;
        $scope.volumeVars.viewing = true;
        $scope.volumeVars.next( 'viewvolume' );
    };
    
    $scope.takeSnapshot = function( $event, volume ){
        
        $event.stopPropagation();
        
        $volume_api.createSnapshot( volume.id, volume.name + '.' + (new Date()).getTime(), function(){ alert( 'Snapshot created.' );} );
    };

    $scope.edit = function( $event, volume ) {

        $event.stopPropagation();
        
        $scope.volumeVars.selectedVolume = volume;
        $scope.volumeVars.editing = true;
        $scope.volumeVars.next( 'createvolume' );
    };

    $scope.delete = function( volume ){
        // TODO:  Create an FDS confirm dialog
        var rtn = confirm( 'Are you sure you want to delete volume: ' + volume.name + '?' );

        if ( rtn === true ){
            $volume_api.delete( volume );
        }
    };

    $scope.createNewVolume = function(){
        $scope.volumeVars.creating = true;
        $scope.volumeVars.next( 'createvolume' );
    };

    $scope.save = function(){

        var volume = $scope.editingVolume;
        volume.priority = $scope.priority;
        volume.sla = $scope.capacity;
        volume.limit = $scope.iopLimit;
        $volume_api.save( volume );
        
        $scope.volumeVars.creating = false;
    };

    $scope.$on( 'fds::volume_done_editing', function(){
        $scope.editing = false;
    });
    
    $scope.$on( 'fds::authentication_logout', function(){
        $scope.volumes = [];
    });

    $scope.$watch( function(){ return $volume_api.volumes; }, function(){

        if ( !$scope.editing ) {
            $scope.volumes = $volume_api.volumes;
        }
    });

    $volume_api.refresh();

}]);
