angular.module( 'form-directives' ).directive( 'treeGrid', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/treegrid/treegrid.html',
        // columns are defined like this: name: <name>, class: <classname>, property: <property> where the property is what we use to display the value
        // data is a list of items that have properties from the column def and optionally a children: [] if the childrenProperty is not supplied.  This structure
        // will be augmented with "loaded" boolean, "expanded" boolean and a "level" integer.  These are added for drawing purposes.
        scope: { childrenFunction: '=', childrenProperty: '@', iconProperty: '@', columns: '=', treeTitle: '@', selection: '=ngModel', data: '=' },
        controller: function( $scope ){

            $scope.flatData = [];
            
            var flattenData = function( list, level ){
                
                for ( var i = 0; i < list.length; i++ ){
                    list[i].level = level;
                    
                    if ( level === 0 ){
                        list[i].show = true;
                    }
                    
                    $scope.flatData.push( list[i] );
                    
                    // we're not dealing with lazy loading here so... just add 'em
                    if ( list[i].hasOwnProperty( $scope.childrenProperty ) ){
                        var childs = list[i][$scope.childrenProperty];
                        list[i].loaded = true;
                        
                        if ( childs.length > 0 ){
                            flattenData( childs, level+1 );
                        }
                    }
                    // we potentially have a lazy-loading scenario so if the function
                    // exists we need to populate each item with a lazy loading temp node.
                    else if ( angular.isFunction( $scope.childrenFunction ) ){
                        var lNode = {
                            tempNode: true,
                        };
                        
                        lNode[$scope.columns[0].property] = 'Loading...';
                        
                        list[i][$scope.childrenProperty] = lNode;
                    }
                }
            };
            
            var childrenReturned = function( node, childs ){
                
            };
            
            var showChildren = function( node, show ){
                
                var childs = node[$scope.childrenProperty];
                
                for ( var i = 0; i < childs.length; i++ ){
                    childs[i].show = show;
                }
            };
            
            $scope.selectRow = function( datum ){
                
                if ( $scope.selection === datum ){
                    $scope.selection = {};
                }
                else {
                    $scope.selection = datum;
                }
            };
            
            $scope.toggleNode = function( node, $event ){
                $event.stopPropagation();
                node.expanded = !node.expanded;
                
                if ( node.loaded === true && angular.isDefined( node[$scope.childrenProperty] ) ){
                    showChildren( node, node.expanded );
                }
                else if ( node.loaded === false && angular.isFunction( $scope.childrenFunction ) ){
                    
                    $scope.childrenFunction( node, function( childs ){
                        node[$scope.childrenProperty] = childs;
                        node.loaded = true;
                        showChildren( node, node.expanded );
                    });
                }
                
            };
            
            $scope.$watch( 'data', function( newVal ){
                
                if ( newVal.length > 0 ){
                    flattenData( newVal, 0 );
                }
            });
        },
        link: function( $scope, $element, $attrs ){
            
            if ( !angular.isDefined( $attrs.childrenProperty ) ){
                $attrs.childrenProperty = 'children';
            }
        }
    };
});