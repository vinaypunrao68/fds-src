mockAuth();
mockVolume();
mockSnapshot();
mockTenant();
mockNode();
mockStats();

var createNewStat = function( vol ){
    
    var limit = parseInt( vol.limit );
    
    if ( limit === 0 ){
        limit = 10000;
    }
    
    var iops = limit - ( Math.random()*(limit * 0.15) );
    var puts = Math.round( iops * 0.4 );
    var ssd = Math.round( iops * 0.55 );
    var gets = Math.round( iops * 0.05 );
    
    // rationalize usage to correct values
    vol.current_usage.size += (vol.rate / 60);

    var minute = {
        time: (new Date()).getTime(),
        PUTS: puts,
        GETS: gets,
        SSD_GETS: ssd,
        LBYTES: vol.current_usage.size,
        PBYTES: Math.round( vol.current_usage.size * 0.4),
        ROLLPOINT: true
    };
    
    return minute;
};

var sumStats = function( statArray ){
    
    var result = {
            time: (new Date()).getTime(),
            PUTS: 0,
            GETS: 0,
            SSD_GETS: 0,
            LBYTES: statArray[ statArray.length-1].LBYTES,
            PBYTES: statArray[ statArray.length-1].LBYTES,
            ROLLPOINT: false
    };
    
    for ( var i = 0; i < statArray.length; i++ ){
        
        var stat = statArray[i];
        
        result.PUTS += stat.PUTS;
        result.GETS += stat.GETS;
        result.SSD_GETS += stat.SSD_GETS;
    }
    
    return result;
};

var addStats = function( volume, stat ){
    
    var newStat = createNewStat( volume );
    
    // add to minute section
    stat.minute.push( newStat );
    
    // roll up?
    if ( stat.minute.length > 60 ){
        
        if ( stat.minute[0].ROLLPOINT === true ){
            stat.hour.push( sumStats( stat.minute ) );
            newStat.ROLLPOINT = true;
        }
        
        stat.minute = stat.minute.splice( 0, 1 );
    }
    
    // roll up?
    if ( stat.hour.length > 24 ){
        stat.hour = stat.hour.splice( 0, 1 );
    }
};

var computeStats = function(){
    
    var vols = window.localStorage.getItem( 'volumes' );
    
    for ( var i = 0; i < vols.length; i++ ){
        
        var volume = vols[i];
        var stat = window.localStorage.getItem( volume.id + '_stats' );
        
        if ( !angular.isDefined( stat ) || stat === null ){
            
            var minute = createNewStat( volume );
            stat = {
                minute: [ minute ],
                hour: []
            };
        }
        else {
            stat = addStats( volId, stat );
        }
        
        window.localStorage.setItem( vol.id + '_stats', stat );
    }
};

setInterval( computeStats, 60000 );