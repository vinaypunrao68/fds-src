angular.module( 'utility-directives' ).factory( '$resize_service', ['$window', '$rootScope', function( $window, $rootScope ){

    var service = {};

    service.listeners = [];

    service.register = function( id, callback ){
        service.listeners[id] = callback;
    };

    service.unregister = function( id ){
        delete service.listeners[id];
    };

    notify = function(){
        for ( var key in service.listeners ){
            var callback = service.listeners[key];
            callback( $window.width, $window.height );
        }
    };

    $rootScope.$on( '$destroy', function(){
        for( var key in service.listeners ){
            service.unregister( key );
        }

        $window.removeEventListener( 'resize', notify );
    });

    init = function(){
        $window.addEventListener( 'resize', notify );
    }();

    return service;
}]);
