angular.module( 'volumes' ).controller( 'volumeCreateController', ['$scope', '$volume_api', '$modal_data_service', function( $scope, $volume_api, $modal_data_service ){

    $scope.save = function(){

        var volume = $modal_data_service.getData();

        if ( volume.sla === 'None' ){
            volume.sla = 0;
        }

        if ( volume.limit === 'None' ){
            volume.limit = 0;
        }

        $volume_api.save( volume );

        $scope.cancel();
    };

    $scope.cancel = function(){
        $scope.$emit( 'fds::volume_done_editing' );
        $scope.$broadcast( 'fds::volume_done_editing' );
    };

    $scope.$watch( '$parent.creating', function(){
        if ( $scope.$parent.creating === true ){
            $modal_data_service.start();
            $scope.$broadcast( 'fds::create_volume_initialize' );
        }
    });

}]);
