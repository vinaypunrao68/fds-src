angular.module( 'volumes' ).controller( 'volumeContainerController', [ '$scope', '$filter', function( $scope, $filter ){
    
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
            value: { range: 0, value: 24 },
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
        {
            name: $filter( 'translate' )( 'common.l_hours' ).toLowerCase(),
            start: 0,
            end: 24,
            segments: 1,
            width: 5
        },
        {
            name: $filter( 'translate' )( 'common.l_days' ).toLowerCase(),
            start: 1,
            end: 7,
            width: 20
        },
        {
            name: $filter( 'translate' )( 'common.l_weeks' ).toLowerCase(),
            start: 1,
            end: 4,
            width: 15
        },
        {
            name: $filter( 'translate' )( 'common.l_months' ).toLowerCase(),
            start: 30,
            end: 360,
            segments: 11,
            width: 30
        },
        {
            name: $filter( 'translate' )( 'common.l_years' ).toLowerCase(),
            start: 1,
            end: 15,
            width: 25
        },
        {
            name: 'j',
            start: 15,
            end: 16,
            width: 5
        }
    ];
    
}]);