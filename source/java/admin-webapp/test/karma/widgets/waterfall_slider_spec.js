describe( 'Waterfall slider widget', function(){
    
    var $scope, $wf;
    
    beforeEach( function(){
        
        module( function( $provide ){
            $provide.value( '$interval', function( call, time ){
                setInterval( call, 0 );
                return {};
            });
        });
        
        module( function( $provide ){
            $provide.value( '$timeout', function( call, time ){
                
                if ( parseInt( time ) === 0 ){
                    time = 5;
                }
                console.log( 'timeout called: ' + time );
                setTimeout( call, time );
            });
        });
        
        module( 'form-directives' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            $scope = $rootScope.$new();
            var html = '<waterfall-slider style="width: 1000px;font-size: 15.5px;font-family: Lato;" sliders="sliders" range="ranges" enabled="enabled"></waterfall-slider>';
            $wf = angular.element( html );
            $compile( $wf )( $scope );

            $scope.sliders = [
                {
                    value: { range: 1, value: 1 },
                    name: 'Continuous'
                },
                {
                    value: { range: 1, value: 2 },
                    name: 'Days'
                },
                {
                    value: { range: 2, value: 2 },
                    name: 'Weeks'
                },
                {
                    value: { range: 3, value: 60 },
                    name: 'Months'
                },
                {
                    value: { range: 4, value: 10 },
                    name: 'Years'
                }
            ];

            $scope.ranges = [
                //0
                {
                    name: 'hours',
                    start: 0,
                    end: 24,
                    segments: 1,
                    width: 5,
                    min: 24,
                    selectable: false
                },
                //1
                {
                    name: 'days',
                    selectName: 'days',
                    start: 1,
                    end: 7,
                    width: 20          
                },
                //2
                {
                    name: 'weeks',
                    selectName: 'weeks',
                    start: 1,
                    end: 4,
                    width: 15  
                },
                //3
                {
                    name: 'days',
                    selectName: 'months by days',
                    start: 30,
                    end: 360,
                    segments: 11,
                    width: 30
                },
                //4
                {
                    name: 'years',
                    selectName: 'years',
                    start: 1,
                    end: 16,
                    width: 25
                },
                //5
                {
                    name: 'forever',
                    selectName: 'forever',
                    allowNumber: false,
                    start: 16,
                    end: 16,
                    width: 5
                }
            ]; 
            
            $('body').append( $wf );
            $scope.$digest();
        });
    });
    
    it( 'should start out looking correct', function(){
        
        var endHandles = $wf.find( '.ending-handle' );
        var startHandles = $wf.find( '.starting-handle' );
        
        expect( endHandles.length ).toBe( 5 );
        expect( startHandles.length ).toBe( 5 );
        
        waitsFor( function(){
            
            if ( $scope.sliders[0].position === undefined ){
                return false;
            }
            $scope.$apply();
            expect( $scope.sliders[0].position.toFixed( 1 ) ).toBe( '46.3' );
            expect( $scope.sliders[1].position.toFixed( 1 ) ).toBe( '231.5' );
            expect( $scope.sliders[2].position.toFixed( 1 ) ).toBe( '277.5' );
            expect( $scope.sliders[3].position.toFixed( 1 ) ).toBe( '395.4' );
            expect( $scope.sliders[4].position.toFixed( 1 ) ).toBe( '648.2' );
            
            return true;
        }, 500 );
        
    });
    
});