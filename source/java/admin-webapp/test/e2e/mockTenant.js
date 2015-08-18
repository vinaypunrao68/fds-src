mockTenant = function(){
    
    angular.module( 'tenant-management' ).factory( '$tenant_service', ['$user_service', function( $user_service ){

        var service = {};

        service.tenants = [];

        service.getTenants = function( callback, failure ){

            if ( angular.isDefined( callback ) ){
                callback( service.tenants );
            }
        };

        service.createTenant = function( tenant, callback, failure ){

            tenant.uid = (new Date()).getTime();
            service.tenants.push( tenant );

            if ( angular.isDefined( callback ) ){
                callback( tenant );
            }
        };

        service.attachUser = function( tenant, userId, callback, failure ){

            var attach = function( users ){

                for ( var i = 0; i < users.length; i++ ){
                    
                    if ( users[i].uid === userId ){

                        users[i].tenant = {
                            uid: tenant.uid,
                            name: tenant.name
                        };
                        
                        break;
                    }
                }
                
                if ( angular.isFunction( callback ) ){
                    callback();
                }
            };
            
            $user_service.getUsers( attach );
            
            return;
        };
        
        service.revokeUser = function( tenant, userId, callback, failure ){
            
            var revoke = function( users ){
                
                for ( var i = 0; i < users.length; i++ ){
                    if ( users[i].uid === userId ){
                        users[i].tenant = undefined;
                    }
                }
                
                if ( angular.isFunction( callback ) ){
                    callback();
                }  

            };
            
            $user_service.getUsers( revoke );
            
            return;
        };

        return service;

    }]);
    
};