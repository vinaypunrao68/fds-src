angular.module( 'user-management' ).factory( '$authorization', [ '$rootScope', '$document', function( $rootScope, $document ){

    var service = {};

    var resetUserFromCookie = function(){
        var cookies = $document[0].cookie.split( ';' );

       for ( var i = 0; i < cookies.length; i++ ){

            var key = cookies[i].slice( 0, cookies[i].indexOf( '=' ) ).trim();
            var value = cookies[i].slice( cookies[i].indexOf( '=' ) + 1 ).trim();

            if ( key && key === 'user' ){
                service.user = JSON.parse( value );
                return;
            }
        }
    };

    service.setUser = function( user ){
        service.user = user;
    };

    // encapsulating the user so that you have to ask
    // for specific information
    service.getUsername = function(){

        if ( !angular.isDefined( service.user ) ){
            resetUserFromCookie();
        }

        if ( angular.isDefined( service.user ) ){
            return service.user.username;
        }

        return undefined;
    };

    // access control service that can be used to see
    // if the current user should have access to something
    service.isAllowed = function( feature ){

        if ( !angular.isDefined( service.user ) ){
            resetUserFromCookie();
        }


        if ( !angular.isDefined( service.user ) ||
            !angular.isDefined( service.user.features ) ){
            return false;
        }

        for ( var i = 0; i < service.user.features.length; i++ ){
            if ( service.user.features[i] === feature ){
                return true;
            }
        }

        return false;
    };

    $rootScope.$on( 'fds::authentication_logout', function(){
        service.user = {};
    });

    return service;

}]);
