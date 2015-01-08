angular.module( 'utility-directives' ).directive( 'sortHelper', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: true,
        templateUrl: 'scripts/directives/utils/sorthelper/sorthelper.html',
        scope: { data: '=ngModel', value: '@' },
        controller: function( $scope ){
            
            $scope.reverse = false;
            
            $scope.headerClicked = function(){
                
                var sortValue = $scope.value;
                
                if ( $scope.reverse === true ){
                    sortValue = '-' + sortValue;
                }
                
                $scope.data = sortValue;
                $scope.reverse = !$scope.reverse;
            };
            
        }
    }; 
});