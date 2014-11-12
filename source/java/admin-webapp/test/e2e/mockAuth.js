mockAuth = function() {

    var adminPassword = 'admin';

    var admin = {
        identifier: 'admin',
        id: 0,
        username: 'admin',
        features: ['SYS_MGMT','Volume Management','TENANT_MGMT','User Management']
    };

    var goldman = {
        identifier: 'goldman',
        id: '1',
        username: 'goldman',
        features: ['Volume Management','User Management']
    };

    var user = {};

    var users = [admin];

    angular.module( 'user-management', [] ).factory( '$authentication', function(){

        var service = {};

        service.login = function( username, password ){

            if ( username === 'admin' && password === adminPassword ){
                service.isAuthenticated = true;
                service.error = undefined;
                user = admin;
            }
            else if ( username === 'goldman' && password === 'goldman' ){
                service.isAuthenticated = true;
                service.error = undefined;
                user = goldman;
            }
            else {
                service.isAuthenticated = false;
                service.error = 'Invalid credentials - please try again';
            }
        };

        service.logout = function(){
            service.isAuthenticated = false;
            service.error = undefined;
        };

        service.getUserInfo = function(){
            return user;
        };

        service.isAuthenticated = false;
        service.error = undefined;

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

        service.user = user;

        service.setUser = function( user ){
            service.user = user;
        };

        // encapsulating the user so that you have to ask
        // for specific information
        service.getUsername = function(){

            return user.identifier;
        };

        // access control service that can be used to see
        // if the current user should have access to something
        service.isAllowed = function( feature ){

            for ( var i = 0; angular.isDefined( user.features ) && i < user.features.length; i++ ){
                if ( user.features[i] === feature ){
                    return true;
                }
            }

            return false;
        };

        service.validateUserToken = function( success, failure ){

            if ( angular.isFunction( success ) ){
                success( user.id );
            }
        };

        return service;

    });

    angular.module( 'user-management' ).factory( '$user_service', function(){

        var service = {};

        service.changePassword = function( userId, newPassword, success, failure ){

            if ( userId === 'FAIL_PLEASE' ){
                failure( 500, 'Password change failed.' );
                return;
            }

            adminPassword = newPassword;

            success();
        };

        service.createUser = function( username, password, success, failure ){

            var user = { identifier: username, password: password, id: (new Date()).getTime() };
            users.push( user );

            success( user );
        };

        service.getUsers = function( callback ){
            callback( users );
        };

        return service;

    });
};
