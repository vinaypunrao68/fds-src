var setFirebreak = function( volumeName, state ){
    
    var volumes = JSON.parse( window.localStorage.getItem( 'volumes' ) );
    var volume;
    
    for ( var i = 0; i < volumes.length; i++ ){
        if ( volumes[i].name = volumeName ){
            volume = volumes[i];
            break;
        }
    }
    
    if ( !angular.isDefined( volumes ) ){
        console.log( 'No volume found with that name' );
        return;
    }
    
    var time = (new Date()).getTime() / 1000;
    
    if ( !angular.isDefined( state ) ){
        state = 'red';
    }
    
    switch( state ){
        case 'orange':
            time -= 3600;
            break;
        case 'yellow':
            time -= 3*3600;
            break;
        case 'lime':
            time -= 6*3600;
            break;
        case 'lightgreen':
            time -= 12*3600;
            break;
        case 'green':
            time = 0;
            break;
        default:
            break;
    }
    
    var fb = { time: time };
    
    window.localStorage.setItem( volume.id + '_fb', JSON.stringify( fb ) );
    
};

var showVolume = function( name ){
    
    var volumes = JSON.parse( window.localStorage.getItem( 'volumes' ) );
    
    for ( var i = 0; i < volumes.length; i++ ){
        
        if ( name === volumes[i].name ){
            console.log( JSON.stringify( volumes[i] ) );
            break;
        }
    }
};

var listVolumes = function( full ){
    
    if ( !angular.isDefined( full ) ){
        full = false;
    }
    
    var volumes = JSON.parse( window.localStorage.getItem( 'volumes' ) );
    
    for ( var i = 0; i < volumes.length; i++ ){
        
        var str = volumes[i].id + ': ' + volumes[i].name;
        
        if ( full === true ){
            str = JSON.stringify( volumes[i] );
        }
        
        console.log( str );
    }
};