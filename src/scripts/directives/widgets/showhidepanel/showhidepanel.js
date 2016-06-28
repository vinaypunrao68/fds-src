angular.module( 'display-widgets' ).directive( 'showHidePanel', function(){

    return {
        restrict: 'E',
        replace: true,
        transclude: true,
        scope: { showText: '@', hideText: '@', contentHeight: '@' },
            templateUrl: 'scripts/directives/widgets/showhidepanel/showhidepanel.html',
        controller: function( $scope ){
            
            $scope.isOpen = false;
            
            $scope.hidePanel = function(){
                $scope.panelText = $scope.showText;
                $scope.isOpen = false;
            };
            
            $scope.showPanel = function(){
                $scope.panelText = $scope.hideText;
                $scope.isOpen = true;
            };
            
            $scope.togglePanel = function(){
                
                if ( $scope.isOpen === true ){
                    $scope.hidePanel();
                }
                else {
                    $scope.showPanel();
                }
            };
        },
        link: function( $scope, $element, $attrs ){
            $scope.panelText = $attrs.showText;
        }
    };
});