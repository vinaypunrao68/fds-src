angular.module( 'display-widgets' ).directive( 'slideWindowStack', function(){
    
    return {
        restrict: 'E',
        transclude: true,
        replace: true,
        scope: { globalVars: '=?'},
        templateUrl: 'scripts/directives/widgets/slidewindowstack/slidewindowstack.html',
        controller: function( $scope, $element, $interval ){
            
            $scope.slides = [];
            $scope.currentStack = [];
            $scope.totalHeight = 40;
            var checkBack = undefined;
            
            var setTotalHeight = function(){
                var slide = $scope.currentStack[ $scope.globalVars.index ];
                $scope.totalHeight = slide.$element.find( '.ng-scope' ).height();
                
                if ( (!angular.isDefined( $scope.totalHeight ) || $scope.totalHeight === null || $scope.totalHeight === 0) &&
                   !angular.isDefined( checkBack ) ){
                    checkBack = $interval( setTotalHeight, 100 );
                }
                else {
                    $interval.cancel( checkBack );
                }
            };
            
            var setIndex = function(){
                if ( angular.isDefined( $scope.globalVars ) ){
                    $scope.globalVars.index = $scope.currentStack.length - 1;
                    
                    setTotalHeight();
                }
            };
            
            this.registerSlide = function( slide ){
                
                if ( $scope.currentStack.length === 0 ){
                    $scope.next( slide );
                }
                
                $scope.slides.push( slide );
            };
            
            $scope.globalVars.back = function(){
                $scope.back( $scope.currentStack.length - 2 );
            };
            
            $scope.back = function( $index ){

                for ( var i = 0; i < $scope.currentStack.length; i++ ){
                    $scope.currentStack[i].left = $scope.currentStack[i].left + 100;
                }
                
                var slide = $scope.currentStack.pop();
                
                if ( $index+1 !== $scope.currentStack.length ){
                    $scope.back( $index );
                }
                
                setIndex();
            };
            
            $scope.globalVars.next = function( slidename ){
                
                for ( var i = 0; i < $scope.slides.length; i++ ){
                    if ( $scope.slides[i].name === slidename ){
                        $scope.next( $scope.slides[i] );
                        break;
                    }
                }
            };
            
            $scope.next = function( slide ){
                
                $scope.currentStack.push( slide );
                
                for ( var i = 0; i < $scope.currentStack.length; i++ ){
                    $scope.currentStack[i].left = $scope.currentStack[i].left - 100;
                }
                
                setIndex();
            };
        }
    };
});

angular.module( 'display-widgets' ).directive( 'slideWindowSlide', function(){
    
    return {
        restrict: 'E',
        require: '^slideWindowStack',
        transclude: true,
        replace: true,
        templateUrl: 'scripts/directives/widgets/slidewindowstack/slidewindowslide.html',
        scope: { name: '@', breadcrumbName: '@', shownFunction: '=?' },
        controller: function( $scope, $element ){
            $scope.left = 100;
            $scope.visible = true;
            
            $scope.$watch( 'left', function( newVal ){
                
                if ( newVal === 0 && angular.isFunction( $scope.shownFunction ) ){
                    $scope.shownFunction();
                }
            });
        },
        link: function( $scope, $element, $attr, $container ){
            
            $scope.$element = $element;
            $container.registerSlide( $scope );
        }
    };
});