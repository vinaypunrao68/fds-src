angular.module( 'volumes' ).directive( 'tieringPanel', function(){
    
    return {
        restrict: 'E', 
        transclude: false,
        replace: true,
        templateUrl: 'scripts/directives/fds-components/tiering/tiering.html',
        scope: { policy: '=ngModel' },
        controller: function( $scope, $filter ){
            
            $scope.tieringChoices = [
                { label: $filter( 'translate' )( 'volumes.tiering.l_ssd_only' ), value: 'SSD_ONLY' },
                { label: $filter( 'translate' )( 'volumes.tiering.l_ssd_preferred' ), value: 'SSD_PREFERRED' },
                { label: $filter( 'translate' )( 'volumes.tiering.l_disk_only' ), value: 'DISK_ONLY' },
                { label: $filter( 'translate' )( 'volumes.tiering.l_disk_preferred' ), value: 'DISK_PREFERRED' }
            ];
            
            var fixPolicySetting = function(){
                if ( angular.isNumber( $scope.policy )){
                    $scope.policy = $scope.tieringChoices[ $scope.policy ];
                }
            };
            
            $scope.$watch( 'policy', fixPolicySetting );
        }
    };
});