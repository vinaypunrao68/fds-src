angular.module( 'volumes' ).controller( 'qualityOfServiceController', ['$scope', '$modal_data_service', function( $scope, $modal_data_service ){

    var init = function(){
        $scope.priority = 10;
        $scope.capacity = 0;
        $scope.iopLimit = 300;
        $scope.editing = false;
    };

    $scope.priorityOptions = [10,9,8,7,6,5,4,3,2,1];

    $scope.updateData = function(){
        $modal_data_service.update({
            priority: $scope.priority,
            sla: $scope.capacity,
            limit: $scope.iopLimit
        });
    };

    $scope.$on( 'fds::create_volume_initialize', function(){
        $scope.updateData();
    });

    $scope.$on( 'fds::volume_done_editing', init );

    $scope.$watch( 'priority', function(){ $scope.updateData() ; });
    $scope.$watch( 'capacity', function(){ $scope.updateData() ; });
    $scope.$watch( 'iopLimit', function(){ $scope.updateData() ; });
    $scope.$watch( 'editing', function(){ $scope.$broadcast( 'fds::fui-slider-refresh' ); });

    init();

}]);
