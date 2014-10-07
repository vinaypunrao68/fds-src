angular.module( 'angular-fui' ).directive( 'fuiDropdown', function(){

    return {
        restrict: 'E',
        transclude: false,
        replace: true,
        // items format is [{name: 'text', ...}, ...]
        // type is 'action' of 'select'
        scope: {selected: '=', defaultLabel: '=', items: '=', background: '@', type: '@', callback: '=', backgroundColor: '@', skinny: '@', color: '@', border: '@'},
        templateUrl: 'scripts/directives/angular-fui/fui-dropdown/fui-dropdown.html',
        controller: function( $scope, $element, $timeout ){

            $scope.leftShift = '0px';
            $scope.open = false;
            $scope.currentLabel = $scope.defaultLabel;

            var positionList = function(){

                // test whether the drop down will be on the screen.
                var ulElem = $element.find( 'ul.dropdown-menu' );
                var offset = ulElem.offset();
                var c = offset.left + ulElem[0].offsetWidth;

                if ( c > (innerWidth-2) ){
                    $scope.leftShift = ((innerWidth-2) - c) + 'px';
                }
            };

            $scope.openList = function(){
                $scope.open = !$scope.open;

                positionList();

                $timeout( positionList, 50 );
            };

            $scope.close = function(){
                $scope.open = false;
            };

            $scope.selectItem = function( item ){

                if ( !angular.isDefined( item ) ){
                    return;
                }

                if ( $scope.type !== 'action' ){

                    if ( !angular.isDefined( item ) ){
                        item = $scope.items[0];
                    }

                    $scope.currentLabel = item.name;
                    $scope.selected = item;
                }
                else{
                    $scope.callback( item );
                }
            };

            $scope.$watch( 'defaultLabel', function(){
                $scope.currentLabel = $scope.defaultLabel;
            });

            $scope.$watch( 'selected', function( newValue, oldValue ){

                if ( !angular.isDefined( $scope.currentLabel ) || (angular.isDefined( newValue ) && newValue !== oldValue) ){
                    $scope.selectItem( newValue );
                }

                $scope.$emit( 'change' );
            });
        },
        link: function( $scope, $element, $attrs ){

            if ( !angular.isDefined( $attrs.type ) ){
                $attrs.type = 'select';
            }
        }
    };
});
