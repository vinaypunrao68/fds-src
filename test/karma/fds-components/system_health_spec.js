describe( 'Testing the modular system health status panel', function(){
    
    var $scope, $panel;
    
    beforeEach( function(){
        
        var fakeFilter;
        
        module( function( $provide ){
            
            $provide.value( 'translateFilter', fakeFilter );
        });
        
        fakeFilter = function( value ){
            return value;
        };
        
        module( 'status' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<health-tile data="health"></health-tile>';
            
            $scope = $rootScope.$new();
            $panel = angular.element( html );
            $compile( $panel )( $scope );
            
            $scope.health = {
                status: [
                    {
                        state: 'GOOD',
                        type: 'SERVICES',
                        message: 'All good'
                    },
                    {
                        state: 'GOOD',
                        type: 'CAPACITY',
                        message: 'All good'
                    },
                    {
                        state: 'GOOD',
                        type: 'FIREBREAK',
                        message: 'All good'
                    }
                ],
                overall: 'EXCELLENT'
            };
            
            $( 'body' ).append( $panel );
            $scope.$digest();
        });
    });
   
    it( 'should start out with 3 green icons', function(){
        
        var indicators = $panel.find( '.health-indicator' );
        expect( indicators.length ).toBe( 3 );
        
        for ( var i = 0; i < indicators.length; i++ ){
            var item = indicators[i];
            
            expect( $(item).hasClass( 'icon-excellent' ) ).toBe( true );
            expect( $(item).css( 'color' ) ).toBe( 'rgb(104, 192, 0)' );
        }
    });
    
    it( 'should have the middle be okay', function(){
        
        $scope.health.status[1].state = 'OKAY';
        $scope.$apply();
        
        var statuses = $panel.find( '.health-indicator' );
        var capacity = statuses[1];
        
        expect( $(capacity).hasClass( 'icon-issues' ) ).toBe( true );
        expect( $(capacity).css( 'color' ) ).toBe( 'rgb(192, 223, 0)' );
    });
    
    it( 'should have the top be unknown and bottom bad', function(){
        
        $scope.health.status[0].state = 'UNKNOWN';
        $scope.health.status[2].state = 'BAD';
        $scope.$apply();
        
        var statuses = $panel.find( '.health-indicator' );
        var services = statuses[0];
        var firebreak = statuses[2];
        
        expect( $(services).hasClass( 'icon-nothing' ) ).toBe( true );
        expect( $(services).css( 'color' ) ).toBe( 'rgb(160, 160, 160)' );
        
        expect( $(firebreak).hasClass( 'icon-warning' ) ).toBe( true );
        expect( $(firebreak).css( 'color' ) ).toBe( 'rgb(253, 141, 0)' );        
    });
    
});