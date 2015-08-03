angular.module( 'volumes' ).directive( 'tieringPanel', function(){
    
    return {
        restrict: 'E', 
        transclude: false,
        replace: true,
        templateUrl: 'scripts/directives/fds-components/tiering/tiering.html',
        scope: { policy: '=ngModel', disabled: '=?' },
        controller: function( $scope, $media_policy_helper ){
            
//            $scope.tieringChoices = $media_policy_helper.availablePolicies;
            
            if ( !angular.isDefined( $scope.disabled ) ){
                $scope.disabled = false;
            }
            
            var fixPolicySetting = function(){
                
                if ( !angular.isDefined( $scope.tieringChoices ) || $scope.tieringChoices.length === 0 ){
                    return;
                }
                
                if ( angular.isNumber( $scope.policy )){
                    $scope.policy = $media_policy_helper.availablePolicies[ $scope.policy ];
                }
                else if ( angular.isString( $scope.policy ) ){
                    $scope.policy = $media_policy_helper.convertRawToObjects( $scope.policy );
                }
                else if ( angular.isDefined( $scope.policy.value ) && angular.isString( $scope.policy.value ) ){
                    $scope.policy = $media_policy_helper.convertRawToObjects( $scope.policy.value );
                }
                
                if ( !$scope.policy || $scope.policy == null || !angular.isDefined( $scope.policy ) || Object.keys( $scope.policy ).length === 0){
                    $scope.policy = $scope.tieringChoices[1];
                }
            };
            
            var setCapabilities = function( policies ){
                $scope.tieringChoices = policies;  
                fixPolicySetting();
            };
            
            var refresh = function(){
                $media_policy_helper.getSystemCapabilities( setCapabilities );
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
                
                refresh();

                $scope.$emit( 'fds::media_policy_changed' );
            });
            
            $scope.$on( 'fds::tiering-choice-refresh', refresh );
            
            refresh();
        }
    };
});