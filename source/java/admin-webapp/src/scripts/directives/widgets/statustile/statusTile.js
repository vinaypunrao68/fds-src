angular.module( 'display-widgets' ).directive( 'statusTile', function(){

    return {
        restrict: 'E',
        transclude: true,
        templateUrl: 'scripts/directives/widgets/statustile/statusTile.html',
        scope: { calculatedData: '=?', notifications: '=', tileTitle: '@' },
        controller: function( $scope ){
            
//            $scope.wholeNumber = 0;
//            $scope.decimals = 0;
//            
//            var calculateNumbers = function(){
//                
//                if ( angular.isNumber( $scope.boldNumber ) ){
//                
//                    $scope.wholeNumber = Math.floor( $scope.boldNumber );
//                    $scope.decimals = ($scope.boldNumber - $scope.wholeNumber)*100;
//                }
//            };
//            
//            $scope.$watch( 'boldNumber', calculateNumbers );
        }
    };

});
