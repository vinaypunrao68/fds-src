angular.module( 'volumes' ).controller( 'volumeContainerController', [ '$scope', function( $scope ){
    
    /**
    volumeVars : {
        creating : bool // pages watch this to know if we are now creating a new volume
        editing: bool // same, but for editing.  This expects selectedVolume to be set
        selectedVolume: volume // when a volume is chosen for editing, viewing, or other this will get set.
        viewing: bool // pages watch for this to know if we are now viewing a volume
        toClone: bool // to clone or not to clone
        clone: volume // the volume that we should be cloning.  Clean this up when you leave.
    }
    
    
    
    **/
    
    $scope.volumeVars = {};
    
    $scope.sliders = [
        {
            value: 0,
            name: 'Continuous'
        },
        {
            value: 0,
            name: 'Days'
        },
        {
            value: 0,
            name: 'Weeks'
        },
        {
            value: 0,
            name: 'Months'
        },
        {
            value: 0,
            name: 'Years'
        }
    ];
    
    $scope.range = [
        {
            start: 0,
            end: 23
        },
        {
            start: 1,
            end: 7
        }
    ];
    
}]);