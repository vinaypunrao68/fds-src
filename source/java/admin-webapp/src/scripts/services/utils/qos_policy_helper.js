angular.module( 'utility-directives' ).factory( '$qos_policy_helper', ['$filter', function( $filter ){
    
    var service = {};
    
    service.presets = [
        {
            label: $filter( 'translate' )( 'volumes.qos.l_least_important' ),
            value: { priority: 10, limit: 0, sla: 0 }
        },
        {
            label: $filter( 'translate' )( 'volumes.qos.l_standard' ),
            value: { priority: 7, limit: 0, sla: 0 }                    
        },
        {
            label: $filter( 'translate' )( 'volumes.qos.l_most_important' ),
            value: { priority: 1, limit: 0, sla: 0 }                    
        },
        {
            label: $filter( 'translate' )( 'common.l_custom' ),
            value: undefined                   
        }
    ];
    
    service.convertRawToPreset = function( qos ){
        
        for ( var i = 0; i < service.presets.length - 1; i++ ){
            
            var preset = service.presets[i];
            
            if ( qos.priority === preset.value.priority &&
                qos.sla === preset.value.sla &&
                qos.limit === preset.value.limit ){
                
                return preset;
            }
        }
        
        // default is return custom
        var custom = {
            label: service.presets[3].label,
            value: qos
        };
        
        return custom;
    };
    
    return service;
    
}]);