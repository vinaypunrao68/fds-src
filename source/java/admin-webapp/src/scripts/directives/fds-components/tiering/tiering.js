angular.module( 'volumes' ).directive( 'tieringPanel', function(){
    
    return {
        restrict: 'E', 
        transclude: false,
        replace: true,
        templateUrl: 'scripts/directives/fds-components/tiering/tiering.html',
        scope: { policy: '=ngModel', disabled: '=?' },
        controller: function( $scope, $media_policy_helper ){
            
            $scope.tieringChoices = $media_policy_helper.availablePolicies;
            
            if ( !angular.isDefined( $scope.disabled ) ){
                $scope.disabled = false;
            }
            
            var fixPolicySetting = function(){
                
                if ( angular.isNumber( $scope.policy )){
                    $scope.policy = $media_policy_helper.availablePolicies[ $scope.policy ];
                }
                else if ( angular.isString( $scope.policy ) ){
                    $scope.policy = $media_policy_helper.convertRawToObjects( $scope.policy );
                }
                
                if ( !angular.isDefined( $scope.policy ) || Object.keys( $scope.policy ).length === 0){
                    $scope.policy = $scope.tieringChoices[1];
                }
            };
            
            $scope.$watch( 'policy', function( newVal, oldVal ){
                
                if ( newVal === oldVal ){
                    return;
                }
                
                // get the actual policy name
                var oldName = oldVal;
                var newName = newVal;
                
                if ( angular.isDefined( newVal.value ) ){
                    newName = newVal.value;
                }
                
                if ( angular.isDefined( oldVal.value ) ){
                    oldName = oldVal.value;
                }
                
                if ( oldName == newName ){
                    return;
                }
                
                fixPolicySetting();
                 $scope.$emit( 'fds::media_policy_changed' );
            });
            
            fixPolicySetting();
        }
    };
});