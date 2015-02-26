angular.module( 'angular-fui' ).directive( 'fuiDropdown', function(){

    return {
        restrict: 'E',
        transclude: false,
        replace: true,
        // items format is [{name: 'text', ...}, ...]
        // type is 'action' of 'select'
        scope: {selected: '=', defaultLabel: '=', items: '=', background: '@', type: '@', callback: '=', 
                backgroundColor: '@', skinny: '@', color: '@', border: '@', nameProperty: '@', width: '@',
                enabled: '=?'},
        templateUrl: 'scripts/directives/angular-fui/fui-dropdown/fui-dropdown.html',
        controller: function( $scope, $element, $timeout, $document ){

            $scope.leftShift = '0px';
            $scope.topShift = '100%';
            $scope.td = false;
            $scope.open = false;
            $scope.currentLabel = $scope.defaultLabel;
            
            if ( !angular.isDefined( $scope.enabled ) ){
                $scope.enabled = true;
            }

            if ( !angular.isDefined( $scope.nameProperty ) ){
                $scope.nameProperty = 'name';
            }
            
            // figure out if we're in a table and make css adjustment
            var tdParents = $( $element[0] ).parents( 'td' );
            
            if ( angular.isDefined( tdParents ) && tdParents.length > 0 ){
                $scope.td = true;
            }
            
            var positionList = function(){

                var off = $element.offset();
                var ulElem = $element.find( 'ul.dropdown-menu' );
                var ulHeight = ulElem.height();
                var dHeight = $document.height();
                var buttonHeight = $element.height();
                
                // should the box go up?
                if ( parseInt( off.top ) + buttonHeight + ulHeight > dHeight ){
                    $scope.topShift = (-1 * ( buttonHeight + ulHeight - 12 )) + 'px';
                }
                else {
                    $scope.topShift = '100%';
                }
                
                // should the box slide left?
                var buttonWidth = $element.width();
                var dWidth = $document.width();
                var ulWidth = ulElem.width();
                
                if ( parseInt( off.left ) + ulWidth > dWidth ){
                    $scope.leftShift = (-1 * ( ulWidth - buttonWidth )) + 'px';
                }
                else {
                    $scope.leftShift = '0px';
                }
            };

            $scope.openList = function(){
                
                if ( $scope.enabled === false ){
                    return;
                }
                
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

                    $scope.currentLabel = item[ $scope.nameProperty ];
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

                if ( !angular.isDefined( newValue ) ){
                    $scope.currentLabel = $scope.defaultLabel;
                    return;
                }
                
                if ( !angular.isDefined( $scope.currentLabel ) || (angular.isDefined( newValue ) && newValue !== oldValue) ){
                    $scope.selectItem( newValue );
                }

                $scope.$emit( 'change' );
                $scope.$emit( 'fui::dropdown_change', newValue );
            });
        },
        link: function( $scope, $element, $attrs ){

            if ( !angular.isDefined( $attrs.type ) ){
                $attrs.type = 'select';
            }
            
            if ( !angular.isDefined( $attrs.nameProperty ) ){
                $attrs.nameProperty = 'name';
            }
        }
    };
});
