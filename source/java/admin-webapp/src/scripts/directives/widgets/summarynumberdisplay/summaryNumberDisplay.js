angular.module( 'display-widgets' ).directive( 'summaryNumberDisplay', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/statustile/statusTile.html',
        // data is an array of numbers and can hold up to 3
        // [{number: number, description: desc, suffix: suffix}, ... ]
        scope: { data: '=ngModel' }, 
        controller: function( $scope ){
            
            $scope.visibleIndex = 0;
            
            var calculateNumbers = function( item ){
                
                if ( angular.isNumber( item.number ) ){
                
                    item.wholeNumber = Math.floor( item.number );
                    item.decimals = (item.number - item.wholeNumber)*100;
                }
            };
            
            $scope.$watch( 'data', function( newVal ){
                
                if ( $scope.data.length === 0 ){
                    return;
                }
                
                for ( var i = 0; i < $scope.data.length; i++ ){
                    calculateNumbers();
                }
            });
        }
    };
    
});