angular.module( 'qos' ).factory( '$snapshot_service', ['$http', function( $http ){

    var service = {};
    
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

    service.createSnapshot = function( volumeId, newName, callback, failure ){
        
        return $http.post( '/api/volume/snapshot', {volumeId: volumeId, name: newName, retention: 0} )
            .success( callback ).error( failure );
    };

    service.createSnapshotPolicy = function( policy, callback, failure ){

        return $http.post( '/api/config/snapshot/policies', policy )
            .success( callback )
            .error( failure );
    };

    service.deleteSnapshotPolicy = function( policyId, callback, failure ){

        return $http.delete( '/api/config/snapshot/policies/' + policyId )
            .success( callback ).error( failure );
    };
    
    service.editSnapshotPolicy = function( policy, callback, failure ){
        
        return $http.put( '/api/config/snapshot/policies', policy )
            .success( callback ).error( failure );
    };

    service.attachPolicyToVolume = function( policy, volumeId, callback, failure ){

        return $http.put( '/api/config/snapshot/policies/' + policy.id + '/attach/' + volumeId )
            .success( callback )
            .error( failure );
    };

    service.detachPolicy = function( policy, volumeId, callback, failure ){

        return $http.put( '/api/config/snapshot/policies/' + policy.id + '/detach/' + volumeId )
            .success( callback ).error( failure );
    };

    service.cloneSnapshotToNewVolume = function( snapshot, volumeName, callback, failure ){

        return $http.post( '/api/snapshot/clone/' + snapshot.id + '/' + escape( volumeName ) )
            .success( callback ).error( failure );
    };

    return service;
}]);
