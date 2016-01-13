angular.module( 'form-directives' ).directive( 'fdsTextList', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/textlist/textlist.html',
        scope: { items: '=ngModel', placeholder: '@', addLabel: '@'},
        controller: function( $scope ){
            
            $scope.addRow = function(){
                $scope.items.push( '' );
            };
            
            $scope.deleteRow = function( index, item ){
                $scope.items.splice( index, 1 );
            };
        }
    };
});