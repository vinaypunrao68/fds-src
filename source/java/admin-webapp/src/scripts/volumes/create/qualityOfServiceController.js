angular.module( 'volumes' ).controller( 'qualityOfServiceController', ['$scope', '$modal_data_service', function( $scope, $modal_data_service ){

    var init = function(){
        $scope.priority = 10;
        $scope.capacity = 0;
        $scope.iopLimit = 300;
        $scope.editing = false;
        
        $scope.updateData();
    };

    $scope.priorityOptions = [10,9,8,7,6,5,4,3,2,1];

    $scope.updateData = function(){
        $modal_data_service.update({
            priority: $scope.priority,
            sla: $scope.capacity,
            limit: $scope.iopLimit
        });
    };
    
    $scope.doneEditing = function(){
        $scope.editing = false;
        $scope.updateData();
    };

    $scope.$watch( 'volumeVars.clone', function( newVal ){
        
        if ( !angular.isDefined( newVal ) || newVal == null ){
            return;
        }
        
        // snapshots won't have this data.
        if ( !angular.isDefined( newVal.priority ) ){
            return;
        }
        
        $scope.priority = newVal.priority;
        $scope.iopLimit = newVal.limit;
        $scope.capacity = newVal.sla;
        
        $scope.updateData();
    });
    
    $scope.$watch( 'volumeVars.creating', function( newVal ){
        if ( newVal === true ){
            init();
        }
    });
    
    $scope.$watch( 'volumeVars.editing', function( newVal ){
        
        if ( newVal === true ){
            $scope.priority = $scope.volumeVars.selectedVolume.priority;
            $scope.capacity = $scope.volumeVars.selectedVolume.sla;
            $scope.iopLimit = $scope.volumeVars.selectedVolume.limit;;
            $scope.editing = false;

            $scope.updateData();
        }
    });

}]);
