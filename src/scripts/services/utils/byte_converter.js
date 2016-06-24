angular.module( 'base' ).factory( '$byte_converter', [ '$filter', function( $filter ){
    
    var service = {};
    
    service.KB = 1024;
    service.MB = service.KB*service.KB;
    service.GB = service.MB*service.KB;
    service.TB = service.GB*service.KB;
    service.PB = service.TB*service.KB;
    service.EB = service.PB*service.KB;
    
    service.byteStr = $filter( 'translate' )( 'common.bytes' );
    service.kbStr = $filter( 'translate' )( 'common.kb' );
    service.mbStr = $filter( 'translate' )( 'common.mb' );
    service.gbStr = $filter( 'translate' )( 'common.gb' );
    service.tbStr = $filter( 'translate' )( 'common.tb' );
    service.pbStr = $filter( 'translate' )( 'common.pb' );
    service.ebStr = $filter( 'translate' )( 'common.eb' );
    
    service.convertBytesToString = function( bytes, decimals ){
        
        if ( !angular.isDefined( decimals ) ){
            decimals = 2;
        }
        
        if ( bytes < service.KB ){
            return bytes + ' ' + service.byteStr;
        }
        else if ( bytes < service.MB ){
            return (bytes/service.KB).toFixed( decimals ) + ' ' + service.kbStr;
        }
        else if( bytes < service.GB ){
            return ( bytes / service.MB ).toFixed( decimals ) + ' ' + service.mbStr;
        }
        else if ( bytes < service.TB ){
            return ( bytes / service.GB ).toFixed( decimals ) + ' ' + service.gbStr;
        }
        else if ( bytes < service.PB ){
            return ( bytes / service.TB ).toFixed( decimals ) + ' ' + service.tbStr;
        }
        else if ( bytes < service.EB ){
            return ( bytes / service.PB ).toFixed( decimals ) + ' ' + service.pbStr;
        }
        else {
            return ( bytes / service.EB ).toFixed( decimals ) + ' ' + service.ebStr;
        }
    };
    
    service.convertFromUnitsToString = function( value, units, decimals ){
        
        var byteVal = value;
        
        if ( units === service.kbStr ){
            byteVal = value * service.KB;
        }
        else if ( units === service.mbStr ){
            byteVal = value * service.MB;
        }
        else if ( units === service.gbStr ){
            byteVal = value * service.GB;
        }
        else if ( units === service.tbStr ){
            byteVal = value * service.TB;
        }
        else if ( units === service.pbStr ){
            byteVal = value * service.PB;
        }
        else if ( units === service.ebStr ){
            byteVal = value * service.EB;
        }
        
        return service.convertBytesToString( byteVal, decimals );
        
    };
    
    return service;
}]);