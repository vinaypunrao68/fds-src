angular.module( 'volumes' ).controller( 'volumeContainerController', [ '$scope', '$filter', function( $scope, $filter ){
    
    /**
    volumeVars : {
        creating : bool // pages watch this to know if we are now creating a new volume
        editing: bool // same, but for editing.  This expects selectedVolume to be set
        selectedVolume: volume // when a volume is chosen for editing, viewing, or other this will get set.
        viewing: bool // pages watch for this to know if we are now viewing a volume
        toClone: bool // to clone or not to clone
        clone: volume // the volume that we should be cloning.  Clean this up when you leave.
    }
    
    
    
    **/
    
    $scope.volumeVars = {};
    
    var pluralLabelFixer = function( plural, singular ){
        
        return function( value ){
            
            var str = value + ' ';
            
            if ( value === 1 ){
                str += singular;
            }
            else {
                str += plural;
            }
            
            return str;
        };
    }
    
    $scope.sliders = [
        {
            value: { range: 1, value: 1 },
            name: $filter( 'translate' )( 'volumes.l_continuous' )
        },
        {
            value: { range: 1, value: 2 },
            name: $filter( 'translate' )( 'common.l_days' )
        },
        {
            value: { range: 2, value: 2 },
            name: $filter( 'translate' )( 'common.l_weeks' )
        },
        {
            value: { range: 3, value: 60 },
            name: $filter( 'translate' )( 'common.l_months' )
        },
        {
            value: { range: 4, value: 10 },
            name: $filter( 'translate' )( 'common.l_years' )
        }
    ];
    
    $scope.range = [
        {
            name: $filter( 'translate' )( 'common.l_hours' ).toLowerCase(),
            start: 0,
            end: 24,
            segments: 1,
            width: 5,
            min: 24,
            selectable: false
        },
        {
            name: $filter( 'translate' )( 'common.l_days' ).toLowerCase(),
            selectName: $filter( 'translate' )( 'common.l_days' ),
            start: 1,
            end: 7,
            width: 20,
            labelFunction: pluralLabelFixer( $filter( 'translate' )( 'common.l_days' ).toLowerCase(),
                $filter( 'translate' )( 'common.l_day' ).toLowerCase() )            
        },
        {
            name: $filter( 'translate' )( 'common.l_weeks' ).toLowerCase(),
            selectName: $filter( 'translate' )( 'common.l_weeks' ),
            start: 1,
            end: 4,
            width: 15,
            labelFunction: pluralLabelFixer( $filter( 'translate' )( 'common.l_weeks' ).toLowerCase(),
                $filter( 'translate' )( 'common.l_week' ).toLowerCase() )            
        },
        {
            name: $filter( 'translate' )( 'common.l_days' ),
            selectName: $filter( 'translate' )( 'common.l_months_by_days' ),
            start: 30,
            end: 360,
            segments: 11,
            width: 30,
            labelFunction: pluralLabelFixer( $filter( 'translate' )( 'common.l_days' ).toLowerCase(),
                $filter( 'translate' )( 'common.l_day' ).toLowerCase() )
        },
        {
            name: $filter( 'translate' )( 'common.l_years' ),
            selectName: $filter( 'translate' )( 'common.l_years' ).toLowerCase(),
            start: 1,
            end: 16,
            width: 25,
            labelFunction: pluralLabelFixer( $filter( 'translate' )( 'common.l_years' ).toLowerCase(),
                $filter( 'translate' )( 'common.l_year' ).toLowerCase() )
        },
        {
            name: $filter( 'translate' )( 'common.l_years' ).toLowerCase(),
            selectName: $filter( 'translate' )( 'common.l_forever' ).toLowerCase(),
            allowNumber: false,
            start: 15,
            end: 16,
            width: 5,
            labelFunction: function( value ){
                if ( value === 16 ){
                    return '\u221E';
                }
                
                return;
            }
        }
    ];
    
}]);