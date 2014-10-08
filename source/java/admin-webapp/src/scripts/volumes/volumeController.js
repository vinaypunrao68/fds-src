angular.module( 'volumes' ).controller( 'volumeController', [ '$scope', '$volume_api', '$element', '$timeout', '$compile', '$snapshot_service', function( $scope, $volume_api, $element, $timeout, $compile, $snapshot_service ){
    
    $scope.volumes = [];
    $scope.capacity = 10;
    $scope.iopLimit = 300;
    $scope.priority = 10;
    $scope.editing = false;
    $scope.editingVolume = {};

    $scope.priorityOptions = [10,9,8,7,6,5,4,3,2,1];

    var putEditRowAway = function(){

        $scope.editing = false;

        var el = $( '#volume-temp-table' );
        var eRow = $( '#volume-edit-row' ).detach();
        el.append( eRow );
    };

    $scope.clicked = function( volume){
        $scope.volumeVars.selectedVolume = volume;
        $scope.volumeVars.next( 'viewvolume' );
    };
    
    $scope.takeSnapshot = function( $event, volume ){
        
        $event.stopPropagation();
        
        $volume_api.createSnapshot( volume.id, volume.name + '.' + (new Date()).getTime(), function(){ alert( 'Snapshot created.' );} );
    };

    $scope.edit = function( $event, volume ) {

        $event.stopPropagation();

        var el = $( '#volume-edit-row' );
        var compiled = $compile( el );

        angular.element( $event.currentTarget ).closest( 'tr' ).after( el );
        $scope.editing = true;
        compiled( $scope );

        $scope.editingVolume = volume;
        $scope.priority = volume.priority;
        $scope.iopLimit = volume.limit;
        $scope.capacity = volume.sla;

        $timeout( function(){
            $scope.$broadcast( 'fds::fui-slider-refresh' );
        }, 50 );
    };

    $scope.delete = function( volume ){
        // TODO:  Create an FDS confirm dialog
        var rtn = confirm( 'Are you sure you want to delete volume: ' + volume.name + '?' );

        if ( rtn === true ){
            $volume_api.delete( volume );
        }
    };

    $scope.createNewVolume = function(){
        putEditRowAway();
        $scope.volumeVars.next( 'createvolume' );
    };

    $scope.save = function(){
        putEditRowAway();
        var volume = $scope.editingVolume;
        volume.priority = $scope.priority;
        volume.sla = $scope.capacity;
        volume.limit = $scope.iopLimit;
        $volume_api.save( volume );
    };

    $scope.cancel = function(){
        putEditRowAway();
    };

    $scope.$on( 'fds::volume_done_editing', function(){
        $scope.editing = false;
    });

    $scope.$watch( function(){ return $volume_api.volumes; }, function(){

        if ( !$scope.editing ) {
            $scope.volumes = $volume_api.volumes;
        }
    });

    $scope.$on( 'fds::authentication_logout', function(){
        $scope.volumes = [];
    });

    $volume_api.refresh();

}]);
