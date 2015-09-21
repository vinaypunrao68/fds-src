mockAuth();
mockVolume();
mockSnapshot();
mockTenant();
mockNode();
mockStats();
mockActivities();
mockMedia();

/*
* The volume size has changed - update that so the volume screen sees that too
*/
var updateVolume = function( vol ){
    
    var volumes = JSON.parse( window.localStorage.getItem( 'volumes' ) );
    
    for ( var i = 0; i < volumes.length; i++ ){
        
        if ( volumes[i].id === vol.id ){
            volumes[i] = vol;
            break;
        }
    }
    
    window.localStorage.setItem( 'volumes', JSON.stringify( volumes ) );
};

/**
* Create a new stat objcet
**/
var createNewStat = function( vol ){
    
    var limit = parseInt( vol.qosPolicy.iopsMax );
    
    if ( limit === 0 ){
        limit = 10000;
    }
    
    var iops = limit - ( Math.random()*(limit * 0.15) );
    var puts = Math.round( iops * 0.4 );
    var ssd;
    var gets;
    
    if ( vol.mediaPolicy === 'SSD_ONLY' ){
        ssd = Math.round( iops * 0.6 );
        gets = 0;
    }
    else if ( vol.mediaPolicy === 'HDD_ONLY' ){
        ssd = 0;
        gets = Math.round( iops * 0.6 );
    }
    else {
        ssd = Math.round( iops * 0.35 );
        gets = Math.round( iops * 0.25 );
    }
    
    // rationalize usage to correct values
    vol.status.currentUsage.size = parseInt( vol.status.currentUsage.size ) + parseInt((vol.rate / 60).toFixed( 0 ));

    var minute = {
        time: (new Date()).getTime(),
        PUTS: puts,
        GETS: gets,
        SSD_GETS: ssd,
        LBYTES: vol.status.currentUsage.size,
        PBYTES: Math.round( vol.status.currentUsage.size * 0.4),
        ROLLPOINT: true
    };
    
    updateVolume( vol );
    
    return minute;
};

/**
* Sum many stats together to form a roll up stat view
**/
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

/**
* A new stat is added - handle the deletion of data and rolling items up to a longer time interval
**/
var addStats = function( volume, stat ){
    
    var newStat = createNewStat( volume );
    newStat.ROLLPOINT = false;
    
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
    
    return stat;
};

/**
* Add a new stat value for all of our volumes
**/
var computeStats = function(){
    
    var vols = JSON.parse( window.localStorage.getItem( 'volumes' ) );
    
    for ( var i = 0; vols != null && i < vols.length; i++ ){
        
        var volume = vols[i];
        var stat = JSON.parse( window.localStorage.getItem( volume.uid + '_stats' ) );
        
        if ( !angular.isDefined( stat ) || stat === null ){
            
            var minute = createNewStat( volume );
            stat = {
                minute: [ minute ],
                hour: []
            };
        }
        else {
            stat = addStats( volume, stat );
        }
        
        window.localStorage.setItem( volume.uid + '_stats', JSON.stringify( stat ) );
    }
};

setInterval( computeStats, 60000 );