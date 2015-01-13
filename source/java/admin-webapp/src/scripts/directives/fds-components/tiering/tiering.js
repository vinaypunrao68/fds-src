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
//                { label: $filter( 'translate' )( 'volumes.tiering.l_ssd_preferred' ), value: 'SSD_PREFERRED' },
                { label: $filter( 'translate' )( 'volumes.tiering.l_disk_only' ), value: 'HDD_ONLY' },
//                { label: $filter( 'translate' )( 'volumes.tiering.l_disk_preferred' ), value: 'HDD_PREFERRED' }
            ];
            
            var fixPolicySetting = function(){
                
                if ( angular.isNumber( $scope.policy )){
                    $scope.policy = $scope.tieringChoices[ $scope.policy ];
                }
                else if ( angular.isString( $scope.policy ) ){
                    for ( var i = 0; i < $scope.tieringChoices.length; i++ ){
                        if ( $scope.tieringChoices[i].value === $scope.policy ){
                            $scope.policy = $scope.tieringChoices[i];
                            break;
                        }
                    }
                }
                
                if ( !angular.isDefined( $scope.policy ) || Object.keys( $scope.policy ).length === 0){
                    $scope.policy = $scope.tieringChoices[0];
                }
            };
            
            $scope.$watch( 'policy', function(){
                fixPolicySetting();
                 $scope.$emit( 'fds::media_policy_changed' );
            });
        }
    };
});