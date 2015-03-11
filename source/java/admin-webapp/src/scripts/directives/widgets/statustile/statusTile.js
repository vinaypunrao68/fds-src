angular.module( 'display-widgets' ).directive( 'statusTile', function(){

    return {
        restrict: 'E',
        transclude: true,
        templateUrl: 'scripts/directives/widgets/statustile/statusTile.html',
        scope: { calculatedData: '=?', notifications: '=', tileTitle: '@', rowDisplay: '@' },
        controller: function( $scope ){
        }
    };

});
