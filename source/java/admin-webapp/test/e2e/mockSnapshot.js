mockSnapshot = function(){
    angular.module( 'qos' ).factory( '$snapshot_service', function(){

        var service = {};
        
        var policies = [];

        service.getPolicies = function(){
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

            policy.id = (new Date()).getTime();
            policies.push( policy );
            
            if ( angular.isFunction( callback ) ){
                callback( policy );
            }
        };

        service.deleteSnapshotPolicy = function( policy, callback, failure ){
            alert( 'here' );
            for( i = 0; i < policies.length; i++ ){
                if ( policy.id === policies[i].id ){
                    break;
                }
            }
            
            policies.splice( i, 1 );
        };

        service.editSnapshotPolicy = function( policy, callback, failure ){
            console.log( 'saving policy: ' + policy.id );
            for( i = 0; i < policies.length; i++ ){
                if ( policy.id === policies[i].id ){
                    policies[i] = policy;
                    break;
                }
            }
            
            callback();
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
            callback();
        };

        service.attachPolicyToVolume = function( policy, volumeId, callback, failure ){
        };

        service.detachPolicy = function( policy, volumeId, callback, failure ){
            return policy;
        };

        service.cloneSnapshotToNewVolume = function( snapshot, volumeName, callback, failure ){
        };

        return service;
    });
};