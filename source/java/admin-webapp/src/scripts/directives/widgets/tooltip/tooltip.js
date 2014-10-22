angular.module( 'display-widgets' ).directive( 'tooltip', function(){
    
    return {
        restrict: 'E',
        transclude: true,
        replace: true,
        templateUrl: 'scripts/directives/widgets/tooltip/tooltip.html',
        scope: { event: '=' },
        controller: function( $scope, $element, $timeout, $document ){
    
            $scope.show = false;
            $scope.target = {};
            $scope.top = '0px';
            $scope.left = '0px';
            $scope.ignore = false;
            $scope.mousePosition = { x: 0, y: 0 };
            
            $scope.overTooltip = function( $event ){
                
                var screenPosition = $( $element[0] ).offset();
                
                var mouseX = $event.clientX;
                var mouseY = $event.clientY;
                var myX = screenPosition.left;
                var myY = screenPosition.top;
                var myX2 = $element[0].offsetWidth + myX;
                var myY2 = $element[0].offsetWidth + myY;
                
                if ( mouseX >= myX && mouseX <= myX2
                    && mouseY >= myY && mouseY <= myY2 ){
                    return true;
                }
                
                return false;
            };
            
            $scope.paintTooltip = function( target ){
                
                if ( target.clientLeft === $scope.target.clientLeft &&
                       target.clientTop === $scope.target.clientTop ){
                        
                    $scope.show = true;

                    $scope.top = /*$scope.mousePosition.y +*/ '-1000px';
                    $scope.left = /*$scope.mousePosition.x +*/ '-1000px';
                    $scope.below = true;
                    $scope.leftSide = true;

                    var el = $( $element[0] ).children( '.tooltip-body' );

                    var h = el.height();
                    var w = el.width();

                    // not ready to shift yet
                    if ( h === 0 ){
                        $timeout( function(){ $scope.paintTooltip( target );}, 50 );
                        return;
                    }
                    
                    var parentH = target.parentElement.clientHeight;
                    var parentW = target.parentElement.clientWidth;
                    $scope.below = false;

                    var yPad = -1*(h+12);
                    var xPad = 0;

                    // shift it right
                    if ( $scope.mousePosition.x + xPad + w > parentW ){
                        xPad = xPad - w;
                        $scope.leftSide = false;
                    }

                    // shift it down
                    if ( $scope.mousePosition.y + yPad < 0 ){
                        yPad = 10;
                        $scope.below = true;
                    }

                    $scope.top = ($scope.mousePosition.y + yPad) + 'px';
                    $scope.left = ($scope.mousePosition.x + xPad) + 'px';
//                    console.log( 'mx: ' + $scope.mousePosition.x + ' xpad: ' + xPad );
                }
            };
            
            $scope.showTooltip = function( target ){
                
                $timeout( function(){
                    $scope.paintTooltip( target );
                }, 800 );
                         
            };
            
            $element[0].parentElement.addEventListener( 'mousemove', function( $event ){
                $scope.mousePosition.x = $event.offsetX + $event.target.offsetLeft;
                $scope.mousePosition.y = $event.offsetY + $event.target.offsetTop;
            });
            
            $scope.$watch( 'event', function( newVal, oldVal ){
                
                if ( !angular.isDefined( newVal ) ){
                    return;
                }
                
                if ( $scope.ignore === true ){
                    return;
                }

                if ( newVal.type === 'mouseleave' ){

                    // see if we're over the tooltip
                    if ( $scope.overTooltip( newVal ) === true ){
                        return;
                    }
                    
                    $scope.show = false;
                    $scope.target = {};
                }
                else if ( newVal.type === 'mouseenter' || newVal.type === 'mouseover' ){
                    $scope.show = false;
                    $scope.target = newVal.target;
                    
                    $scope.showTooltip( $.extend( {}, $scope.target ) );
                }
                
            });
        }
    };
});