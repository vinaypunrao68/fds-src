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
    
    $scope.ranges = [
        { min: 1000 },
        { min: 200 },
        { min: 4328 },
        { min: 12 },
        { min: 2945 },
        { min: 9232 },
        { min: 311 },
        { min: 105 },
        { min: 1265 },
        { min: 22 },
        { min: 811 },
        { min: 3220 },
        { min: 640 },
        { min: 2001 },
        { min: 1190 },
        { min: 8324 },
        { min: 5000 },
        { min: 611 },
        { min: 7129 },
        { min: 823 },
        { min: 71 },
        { min: 10001 },
        { min: 10002 },
        { min: 30, max: 200 }
    ];
    
    $scope.time = 1000;
    
    $scope.labelFunc = function( value ){
        return parseFloat( value ).toFixed( 1 );
    };
    
}]);