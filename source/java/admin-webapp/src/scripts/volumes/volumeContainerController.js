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
    
}]);