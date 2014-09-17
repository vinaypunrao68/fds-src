angular.module( 'display-widgets' ).directive( 'statusTile', function(){

    return {
        restrict: 'E',
        transclude: true,
        templateUrl: 'scripts/directives/widgets/statustile/statusTile.html',
        scope: { boldNumber: '=', description: '=', notifications: '=', title: '@' },
        controller: function( $scope ){

        }
    };

});
