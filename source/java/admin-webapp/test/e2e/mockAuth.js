mockAuth = function() {

    var adminPassword = 'admin';

    var admin = {
        uid: 0,
        name: 'admin',
        features: ['SYS_MGMT','VOL_MGMT','TENANT_MGMT','USER_MGMT']   
    };

    var goldman = {
        uid: 1,
        name: 'goldman',
        features: ['VOL_MGMT','USER_MGMT']   
    };

    var user = {};

    var users = [admin];

    angular.module( 'user-management', [] ).factory( '$authentication', function(){

        var service = {};
        
        var saveUser = function(){
            
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return;
            }
            
            window.localStorage.setItem( 'user', JSON.stringify( user ) );
        };

        service.login = function( username, password ){
            if ( username == 'admin' && password == adminPassword ){
                service.isAuthenticated = true;
                service.error = undefined;
                user = admin;
                
                saveUser();
            }
            else if ( username === 'goldman' && password === 'goldman' ){
                service.isAuthenticated = true;
                service.error = undefined;
                user = goldman;
                
                saveUser();
            }
            else {
                service.isAuthenticated = false;
                service.error = 'Invalid credentials - please try again';
            }
        };

        service.logout = function(){
            service.isAuthenticated = false;
            service.error = undefined;
            
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return;
            }
            
            window.localStorage.removeItem( 'user' );
        };

        service.getUserInfo = function(){
            return user;
        };

        service.isAuthenticated = false;
        service.error = undefined;

        if ( angular.isDefined( window ) && angular.isDefined( window.localStorage ) ){
            
            var raw = window.localStorage.getItem( 'user' );
            
            if ( angular.isDefined( raw ) && raw !== null ){
                
                user = JSON.parse( raw );
                service.isAuthenticated = true;   
            }
        }
        
        return service;
    });

    angular.module( 'user-management' ).factory( '$domain_service', function(){

        var service = {};

        service.getDomains = function(){

            if ( !angular.isDefined( service.domains ) ){
                service.domains = [{fqdn: 'com.goldman', name: 'FDS Global'}];
            }

            return service.domains;
        }

        return service;
    });

    angular.module( 'user-management' ).factory( '$authorization', function(){

        var service = {};
        service.user = user;

        service.setUser = function( user ){
            service.user = user;
        };

        // encapsulating the user so that you have to ask
        // for specific information
        service.getUsername = function(){

            if ( user !== null ){
                return user.name;
            }
            else {
                return undefined;
            }
        };

        // access control service that can be used to see
        // if the current user should have access to something
        service.isAllowed = function( feature ){

            for ( var i = 0; user !== null && angular.isDefined( user ) && angular.isDefined( user.features ) && i < user.features.length; i++ ){
                if ( user.features[i] === feature ){
                    return true;
                }
            }

            return false;
        };

        service.validateUserToken = function( success, failure ){

            if ( angular.isFunction( success ) ){
                success( user.uid );
            }
        };

        return service;

    });

    angular.module( 'user-management' ).factory( '$user_service', [ '$q', function( $q ){

        var service = {};

        service.changePassword = function( user, success, failure ){

            if ( user.password === 'FAIL_PLEASE' ){
                failure( 500, 'Password change failed.' );
                return;
            }

            adminPassword = user.password;
            
            success();
        };

        service.createUser = function( user, success, failure ){

            user.uid = (new Date()).getTime();
            
            users.push( user );

            var deferred = $q.defer();
            
            success( user );
            deferred.resolve();

            return deferred.promise;
        };

        service.getUsers = function( callback ){
            callback( users );
        };

        return service;

    }]);
};
