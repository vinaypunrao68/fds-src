angular.module( 'display-widgets' ).directive( 'fdsModal', function(){
    
    return {
        restrict: 'E',
        replace: true,
        transclude: false,
        templateUrl: 'scripts/directives/widgets/fds-modal/fds-modal.html',
        scope: { showEvent: '@', confirmEvent: '@' },
        // event should be:  { text: '', type: 'INFO|WARN|ERROR|QUESTION', ok: Func, cancel: Func }
        controller: function( $scope, $rootScope, $filter ){
            
            $scope.$event = {};
            $scope.width = 0;
            $scope.show = false;
            $scope.showCancel = false;
            
            $scope.cancel = function(){
                $scope.$event = {};
                $scope.show = false;
            };
            
            $scope.okPressed = function(){
                
                if ( angular.isFunction( $scope.$event.confirm ) ){
                    $scope.$event.confirm( true );
                }
                
                $scope.cancel();
            };
            
            var showDialog = function( eventName, $event ){
                
                $scope.$event = $event;
                
                var size = measureText( $event.text, 12 );
                
                if ( size.width > 500 ){
                    $scope.width = '500px';
                }
                else if ( size.width < 350 ){
                    $scope.width = '350px';
                }
                else {
                    $scope.width = size.width + 'px';
                }
                
                if ( !angular.isDefined ( $event.title ) ){
                    
                    switch( $event.type ){
                        case 'WARN':
                            $event.title = $filter( 'translate' )( 'common.l_warning' );
                            break;
                        case 'ERROR':
                            $event.title = $filter( 'translate' )( 'common.l_error' );
                            break;
                        case 'CONFIRM':
                            $event.title = $filter( 'translate' )( 'common.l_confirm' );
                            break;
                        default:
                            $event.title = $filter( 'translate' )( 'common.l_info' );
                            break;
                            
                    }
                }
                
                $scope.show = true;
            };
            
            $rootScope.$on( $scope.showEvent,  function( eventName, $event ){
                
                $scope.okText = $filter( 'translate' )('common.l_ok');
                $scope.cancelText = $filter( 'translate' )( 'common.l_cancel' );
                $scope.showCancel = false;
                
                showDialog( eventName, $event );
            });
            
            $rootScope.$on( $scope.confirmEvent, function( eventName, $event ){
                
                $scope.okText = $filter( 'translate' )( 'common.l_yes' );
                $scope.cancelText = $filter( 'translate' )( 'common.l_no' );
                $scope.showCancel = true;
                
                showDialog( eventName, $event );
            });
        }
    };
    
});