angular.module( 'form-directives' ).directive( 'navitem', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        scope: { label: '@', icon: '@', notifications: '=', selected: '=', iconClass: '@', color: '@', hoverColor: '@'},
            templateUrl: 'scripts/directives/widgets/navitem/navitem.html',
        controller: function( $scope, $element ){

            if ( angular.isDefined( $scope.iconClass ) && $scope.iconClass !== '' ){
                var navIcon = $element.find( '.nav-icon' );
                navIcon.addClass( $scope.iconClass );
            }
        }
    };
});
