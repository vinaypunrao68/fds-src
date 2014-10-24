angular.module( 'form-directives' ).directive( 'macBrowser', function(){
    
    /**
    
    NOTE: This is written as a static two level browser right now
    in the interest of time.
    
    **/
    
    return {
        
        restrict: 'E',
        replace: true,
        transclude: true,
        templateUrl: 'scripts/directives/widgets/macbrowser/macbrowser.html',
        scope: { data: '=', selected: '=ngModel', childrenFunction: '=?', childrenProperty: '@', columns: '=', height: '@', browserTitle: '@',
               cancelFunction: '=?', chooseFunction: '=?'},
        controller: function( $scope ){
            
            $scope.children = [];
            
            if ( !angular.isDefined( $scope.height ) || parseInt( $scope.height ) < 100 ){
                $scope.height = 250;
            }
            
            var augment = function( list, level, parent ){
                
                for ( var i = 0; i < list.length; i++ ){
                    list[i].level = level;
                    
                    if ( angular.isDefined( parent ) ){
                        list[i].parent = parent;
                    }
                    
                    var childs = list[i][$scope.childrenProperty];
                    
                    if ( angular.isDefined( childs ) && childs.length > 0 ){
                        augment( childs, level+1, list[i] );
                    }
                }
            };

            $scope.itemSelected = function( item ){
                
                if ( !angular.isDefined( item ) ){
                    return;
                }
                
                // de-select anyone else that is selected
                var siblings = [];
                
                if ( angular.isDefined( item.parent ) ){
                    siblings = item.parent[$scope.childrenProperty];
                }
                else {
                    siblings = $scope.data;
                }
                    
                for ( var j = 0; j < siblings.length; j++ ){
                    siblings[j].selected = false;
                }
                
                item.selected = true;
                
                if ( item.level === 0 ){
                    
                    if ( item.hasOwnProperty( $scope.childrenProperty ) ){
                        $scope.children = item[$scope.childrenProperty];
                    }
                    else if ( angular.isFunction( $scope.childrenFunction ) ){
                        
                        $scope.children = [{name: 'Loading...'}];
                        
                        $scope.childrenFunction( item, function( childs ){
                            item[$scope.childrenProperty] = childs;
                            augment( $scope.data, 0 );
                            $scope.itemSelected( item );
                            
                            if ( item[$scope.childrenProperty].length > 0 ){
                                $scope.itemSelected( item[$scope.childrenProperty][0] );
                            }
                        });
                    }
                    else {
                        item[$scope.childrenProperty] = [];
                    }
                }
                else {
                    
                    $scope.selected = item;
                }
            };
            
            $scope.donePressed = function(){
                
                if ( angular.isFunction( $scope.chooseFunction ) ){
                    $scope.chooseFunction();
                }
            };
            
            $scope.cancelPressed = function(){
                
                if ( angular.isFunction( $scope.cancelFunction ) ){
                    $scope.cancelFunction();
                }                
            };
            
            $scope.$watch( 'data', function( newVal ){
                augment( $scope.data, 0 );
                $scope.itemSelected( $scope.data[0] );
            });
        },
        link: function( $scope, $element, $attrs ){
            
                        
            if ( !angular.isDefined( $attrs.childrenProperty ) || $attrs.childrenProperty === '' ){
                $attrs.childrenProperty = 'children';
            }
        }
    };
});