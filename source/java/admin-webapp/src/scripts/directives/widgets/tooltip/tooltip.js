angular.module( 'display-widgets' ).directive( 'toolTip', function(){
    
    return {
        restrict: 'A',
        replace: false,
        transclude: false,
        scope: { text: '@', bindText: '=?', visible: '=?' },
        controller: function( $scope, $element, $timeout, $templateCache, $compile, $document ){
            
            $scope.show = false;
            $scope.target = {};
            $scope.top = '0px';
            $scope.left = '0px';
            $scope.textWidth = 0;
            $scope.ignore = false;
            $scope.mousePosition = { x: 0, y: 0 };
            
            var parent = $element[0];
            
            $scope.paintTooltip = function( target ){
                
                if ( target.clientLeft === $scope.target.clientLeft &&
                       target.clientTop === $scope.target.clientTop ){
                        
                    $scope.show = true;

                    $scope.top = /*$scope.mousePosition.y +*/ '-1000px';
                    $scope.left = /*$scope.mousePosition.x +*/ '-1000px';
                    $scope.below = true;
                    $scope.leftSide = true;
                    
                    // the 28 is the margin and padding included
                    $scope.textWidth = measureText( $scope.text, 13 ).width + 28;

                    var el = $($(parent).find( '.tooltip-body' )[0]);

                    var h = el.height();
                    var w = el.width();

                    // not ready to shift yet
                    if ( h <= 0 ){
                        $timeout( function(){ $scope.paintTooltip( target );}, 50 );
                        return;
                    }
                
                    var yPad = -1*h - 34;//-1*(h+12);
                    var xPad = -16;
                    $scope.below = false;

                    var o = $(parent).offset(); 
                    
//                    console.log( 'o.left: ' + o.left + ' o.top: ' + o.top + ' y: ' + $scope.mousePosition.y + ' x: ' + $scope.mousePosition.x );
//                    console.log( ' w: ' + w + ' h: ' + h + ' dw: ' + $document.width() + ' dh: ' + $document.height() );
                    
                    // shift to the left
                    if ( o.left + $scope.mousePosition.x + xPad + w > $document.width() ){
                        xPad = -1*(w+24);
                        $scope.leftSide = false;
                    }
                    
                    // flip below
                    if ( o.top + $scope.mousePosition.y + yPad < 0 ){
                        yPad = 12;
                        $scope.below = true;
                    }

                    $scope.top = ($scope.mousePosition.y + yPad ) + 'px';
                    $scope.left = ($scope.mousePosition.x + xPad ) + 'px';
                }
            };
            
            $scope.showTooltip = function( target ){
                
                $timeout( function(){
                    $scope.paintTooltip( target );
                }, 800 );
                         
            };
            
            var mousemove = function( $event ){
                $scope.mousePosition.x = $event.offsetX + $event.target.offsetLeft;
                $scope.mousePosition.y = $event.offsetY + $event.target.offsetTop;                
            };
            
            var mouseenter = function( $event ){
                
                if ( !angular.isDefined( $event ) ){
                    return;
                }
                
                if ( $scope.ignore === true ){
                    return;
                }
                
                $scope.show = false;
                $scope.target = $event.target;

                $scope.showTooltip( $.extend( {}, $scope.target ) );                           
            };
            
            var mouseleave = function( $event ){
              
                if ( !angular.isDefined( $event ) ){
                    return;
                }
                
                if ( $scope.ignore === true ){
                    return;
                }

                $scope.show = false;
                $scope.target = {};
            };
            
            // only go into true tool tip mode if visible is not defined.
            // if it is defined that means the parent controller will control 
            // whether or not it's displayed.
            if ( angular.isDefined( $scope.visible ) ){
                parent.addEventListener( 'mousemove', mousemove );
                parent.addEventListener( 'mouseenter', mouseenter );
                parent.addEventListener( 'mouseleave', mouseleave ); 
            }
            
            var tooltip = $templateCache.get('scripts/directives/widgets/tooltip/tooltip.html');
            var compiled = $compile( tooltip );            
            tooltip = compiled( $scope );
            $(parent).css( 'position', 'relative' );
            $(parent).append( tooltip );
            
            $scope.$on( '$destroy', function(){
                
                parent.removeEventListener( 'mouseleave', mouseleave );
                parent.removeEventListener( 'mousemove', mousemove );
                parent.removeEventListener( 'mouseenter', mouseenter );
            });
            
            $scope.$watch( 'visible', function( newVal ){
                $scope.show = newVal;
            });
        }
    };
    
});