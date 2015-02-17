angular.module( 'status' ).directive( 'healthTile', function(){
    
    return {
        restrict: 'E', 
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/fds-components/healthtile/healthtile.html',
        // health object: { overall: enum, status: [{ type: enum, state: enum, message: text },...]}
        scope: { data: '=' },
        controller: function( $scope, $filter ){
            
            var GOOD = 'GOOD';
            var OKAY = 'OKAY';
            var BAD = 'BAD';
            var UNKNOWN = 'UNKNOWN';
            
            $scope.colors = [];
            $scope.colors[GOOD] = '#68C000';
            $scope.colors[OKAY] = '#C0DF00';
            $scope.colors[BAD] = '#FD8D00';
            $scope.colors[UNKNOWN] = '#A0A0A0';
            
            $scope.iconClasses = [];
            $scope.iconClasses[GOOD] = 'icon-excellent';
            $scope.iconClasses[OKAY] = 'icon-issues';
            $scope.iconClasses[BAD] = 'icon-warning';
            $scope.iconClasses[UNKNOWN] = 'icon-nothing';
        }
    };
});