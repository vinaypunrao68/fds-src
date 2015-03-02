mockSnapshot = function(){
    angular.module( 'qos' ).factory( '$snapshot_service', function(){

        var service = {};
        
        var policies = [];
        
        var num = (new Date()).getTime();
        
        var nextNum = function(){
            
            num++;
            return num;
        };
        
        var savePolicies = function(){
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return;
            }
            
            window.localStorage.setItem( 'policies', JSON.stringify( policies ) );
        };

        service.getPolicies = function(){
            
            if ( angular.isDefined( window ) && angular.isDefined( window.localStorage ) ){
                policies = JSON.parse( window.localStorage.getItem( 'policies' ) );
            }
            
            // handles the init case
            if ( policies === null ){
                policies = [];
                savePolicies();
            }
            
            return policies;
        };
        
        service.convertTimePeriodToSeconds = function( value, timeUnit ){

            var mult;

            switch( timeUnit ){
        //        case 'HOURLY':
        //            mult = 60*60;
        //            break;
                case 'DAYS':
                    mult = 24*60*60;
                    break;
                case 'WEEKS':
                    mult = 7*24*60*60;
                    break;
                case 'MONTHS':
                    mult = 31*24*60*60;
                    break;
                case 'YEARS':
                    mult = 366*24*60*60;
                    break;
                default:
                    mult = 60*60;
            }

            return (value*mult);
        };

        service.convertSecondsToTimePeriod = function( seconds ){

            var rtn = { value: 0, units: 0 };

            if ( seconds % 366*24*60*60 === 0 ){
                rtn.value = seconds / (366*24*60*60);
                rtn.units = 4;
            }
            else if ( seconds % 31*24*60*60 === 0 ){
                rtn.value = seconds / (31*24*60*60);
                rtn.units = 3;
            }
            else if ( seconds % 7*24*60*60 === 0 ){
                rtn.value = seconds / (7*24*60*60);
                rtn.units = 2;
            }
            else if ( seconds % (24*60*60) === 0 ){
                rtn.value = seconds / (24*60*60);
                rtn.units = 1;
            }
            else {
                rtn.value = seconds / 3600;
                rtn.units = 0;
            }

            return rtn;
        };          

        service.createSnapshotPolicy = function( policy, callback, failure ){

            policy.id = nextNum();
            
            policies.push( policy );
            
            if ( angular.isFunction( callback ) ){
                callback( policy );
            }
            
            savePolicies();
        };

        service.deleteSnapshotPolicy = function( policy, callback, failure ){
        
            for( i = 0; i < policies.length; i++ ){
                if ( policy.id === policies[i].id ){
                    break;
                }
            }
            
            policies.splice( i, 1 );
            
            savePolicies();
        };

        service.editSnapshotPolicy = function( policy, callback, failure ){
            
            for( i = 0; i < policies.length; i++ ){
                if ( policy.id === policies[i].id ){
                    policies[i] = policy;
                    break;
                }
            }
            
            savePolicies();
            
            if ( angular.isFunction( callback ) ){
                callback();
            }
        };
        
        service.saveSnapshotPolicies = function( volumeId, policies, callback, failure ){
            
            // creating / editing the new selections
            for ( var i = 0; i < policies.length; i++ ){

                var sPolicy = policies[i];

                // currently there is a field called timelineTime that we don't use but it must be present
                sPolicy.timelineTime = 0;

                // if it has an ID then it's already exists
                if ( angular.isDefined( sPolicy.id ) ){

                    service.editSnapshotPolicy( sPolicy, function(){} );
                }
                else {
                    
                    service.createSnapshotPolicy( sPolicy, function( policy ){
                        service.attachPolicyToVolume( policy, volumeId, function(){} );
                    });
                }
            }
            
            if ( angular.isFunction( callback ) ){
                callback();
            }
            
            savePolicies();
        };

        service.attachPolicyToVolume = function( policy, volumeId, callback, failure ){
            
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return;
            }
            
            var policySet = window.localStorage.getItem( 'volume_' + volumeId + '_snapshot_policies' );
            
            if ( !angular.isDefined( policySet ) || policySet === null ){
                policySet = [];
            }
            else {
                policySet = JSON.parse( policySet );
            }
            
            policySet.push( policy );
            
            window.localStorage.setItem( 'volume_' + volumeId + '_snapshot_policies', JSON.stringify( policySet ) );
        };

        service.detachPolicy = function( policy, volumeId, callback, failure ){
            
            if ( !angular.isDefined( window ) || !angular.isDefined( window.localStorage ) ){
                return;
            }
            
            var policySet = window.localStorage.getItem( 'volume_' + volumeId + '_snapshot_policies' );
            
            if ( angular.isDefined( policySet ) && policySet !== null ){

                policySet = JSON.parse( policySet );
                
                for ( var i = 0; i < policySet.length; i++ ){
                    
                    if ( policySet[i].id === policy.id ){
                        policySet.splice( i, 1 );
                        break;
                    }
                }
            }            
            
            window.localStorage.setItem( 'volume_' + volumeId + '_snapshot_policies', JSON.stringify( policySet ) );
            
            return policy;
        };

        service.cloneSnapshotToNewVolume = function( snapshot, volumeName, callback, failure ){
        };

        return service;
    });
};