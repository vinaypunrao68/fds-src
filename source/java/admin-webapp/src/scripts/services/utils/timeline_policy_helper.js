angular.module( 'utility-directives' ).factory( '$timeline_policy_helper', ['$filter', function( $filter ){
    
    var service = {};
    
   
    // abbreviation map
    var abbreviations = [
        { abbr: 'MO', string: $filter( 'translate' )( 'volumes.snapshot.l_monday' ) },
        { abbr: 'TU', string: $filter( 'translate' )( 'volumes.snapshot.l_tuesday' ) },
        { abbr: 'WE', string: $filter( 'translate' )( 'volumes.snapshot.l_wednesday' ) },
        { abbr: 'TH', string: $filter( 'translate' )( 'volumes.snapshot.l_thursday' ) },
        { abbr: 'FR', string: $filter( 'translate' )( 'volumes.snapshot.l_friday' ) }
    ];
    
    var months = [
        '',
        $filter( 'translate' )( 'volumes.snapshot.l_january' ),
        $filter( 'translate' )( 'volumes.snapshot.l_february' ),
        $filter( 'translate' )( 'volumes.snapshot.l_march' ),
        $filter( 'translate' )( 'volumes.snapshot.l_april' ),
        $filter( 'translate' )( 'volumes.snapshot.l_may' ),
        $filter( 'translate' )( 'volumes.snapshot.l_june' ),
        $filter( 'translate' )( 'volumes.snapshot.l_july' ),
        $filter( 'translate' )( 'volumes.snapshot.l_august' ),
        $filter( 'translate' )( 'volumes.snapshot.l_september' ),
        $filter( 'translate' )( 'volumes.snapshot.l_october' ),
        $filter( 'translate' )( 'volumes.snapshot.l_november' ),
        $filter( 'translate' )( 'volumes.snapshot.l_december' )
    ];
    
    /***************** Code begin ************************/
    
    service.arePoliciesEqual = function( policy1, policy2 ){
        
        for ( var i = 0; i < policy1.length; i++ ){
            
            var rule = policy1[i];
            
            var match = true;
            
            // find the matching one frequenct-wise and then check values
            for ( var j = 0; j < policy2.length; j++ ){
                
                var rule2 = policy2[j];
                
                // got the match
                if ( rule.recurrenceRule.FREQ === rule2.recurrenceRule.FREQ ){
                    
                    // all types have byhour and by minute so check these two first as a sanity check.
                    if ( rule.recurrenceRule.BYMINUTE[0] != rule2.recurrenceRule.BYMINUTE[0] ||
                        rule.recurrenceRule.BYHOUR[0] != rule2.recurrenceRule.BYHOUR[0] ){
                        match = false;
                        break;
                    }
                    
                    // if there is a day field, check it
                    if ( angular.isDefined( rule.recurrenceRule.BYDAY ) ){
                        
                        if ( !angular.isDefined( rule2.recurrenceRule.BYDAY ) || rule.recurrenceRule.BYDAY[0] != rule2.recurrenceRule.BYDAY[0] ){
                            match = false;
                            break;
                        }
                    }
                     
                    // if there is a by month day field, check it
                    if ( angular.isDefined( rule.recurrenceRule.BYMONTHDAY ) ){
                        
                        if ( !angular.isDefined( rule2.recurrenceRule.BYMONTHDAY ) || rule.recurrenceRule.BYMONTHDAY[0] != rule2.recurrenceRule.BYMONTHDAY[0] ){
                            match = false;
                            break;
                        }
                    }
                    
                    // if there is a month field, check it
                    if ( angular.isDefined( rule.recurrenceRule.BYMONTH ) ){
                        
                        if ( !angular.isDefined( rule2.recurrenceRule.BYMONTH ) || rule.recurrenceRule.BYMONTH[0] != rule2.recurrenceRule.BYMONTH[0] ){
                            match = false;
                            break;
                        }
                    }
                    
                    if ( parseInt( rule.retentionTime.seconds ) !== parseInt( rule2.retentionTime.seconds ) ){
                        match = false;
                    }
                }// match found
            }// for j
            
            if ( match === false ){
                return false;
            }
        }// for i
        
        return true;
    };

    service.convertAbbreviationToString = function( a ){
        
        for ( var i = 0; i < abbreviations.length; i++ ){
            
            if ( abbreviations[i].abbr.indexOf( a ) !== -1 ){
                return abbreviations[i].string;
            }
        }
        
        return a;
    };
    
    service.convertMonthNumberToString = function( month ){
        return months[month];
    };
    
    return service;
    
}]);