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
                    value: { range: 2, value: 1 },
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
            
            // expect the value slider to be at...
            expect( $scope.sliders[0].position.toFixed( 1 ) ).toBe( '46.3' );
            expect( $scope.sliders[1].position.toFixed( 1 ) ).toBe( '231.5' );
            expect( $scope.sliders[2].position.toFixed( 1 ) ).toBe( '277.5' );
            expect( $scope.sliders[3].position.toFixed( 1 ) ).toBe( '395.4' );
            expect( $scope.sliders[4].position.toFixed( 1 ) ).toBe( '783.2' );
            
            // expect the box that shows the starting position of the slider to be...
            expect( $(startHandles[0]).css( 'display' ) ).toBe( 'none' );
            
            var stripAndRound = function( handle ){
                var px = $(handle).css( 'left' );
                px = px.substr( 0, px.length-2 );

                return parseFloat( px ).toFixed( 1 );
            };
            
            expect( stripAndRound( startHandles[1] ) ).toBe( '46.3' );
            expect( stripAndRound( startHandles[2] ) ).toBe( '231.5' );
            expect( stripAndRound( startHandles[3] ) ).toBe( '277.5' );
            expect( stripAndRound( startHandles[4] ) ).toBe( '395.4' );
            
            // expect the tick marks all exist
            var tickMarks = $wf.find( '.tick-mark' );
            expect( tickMarks.length ).toBe( 36 );
            
            // expect the start of each range to be thick
            expect( $(tickMarks[0]).hasClass( 'thick-tick-mark' ) ).toBe( true );
            expect( $(tickMarks[6]).hasClass( 'thick-tick-mark' ) ).toBe( true );
            expect( $(tickMarks[9]).hasClass( 'thick-tick-mark' ) ).toBe( true );
            expect( $(tickMarks[20]).hasClass( 'thick-tick-mark' ) ).toBe( true );
            expect( $(tickMarks[35]).hasClass( 'thick-tick-mark' ) ).toBe( true );
            
            // makes sure the first bar is filled and the others are dots
            var fillbars = $wf.find( '.fill-bar' );
            expect( fillbars.length ).toBe( 5 );
            expect( $(fillbars[0]).hasClass( 'solid-fill' ) ).toBe( true );
            expect( $(fillbars[1]).css( 'background-image' ).indexOf( 'Blue-dot.svg' ) ).not.toBe( -1);
            expect( $(fillbars[2]).hasClass( 'solid-fill' ) ).toBe( false );
            expect( $(fillbars[3]).hasClass( 'solid-fill' ) ).toBe( false );
            expect( $(fillbars[4]).hasClass( 'solid-fill' ) ).toBe( false );
            
            
            return true;
        }, 500 );
        
    });
    
    it( 'should draw correctly when slider values are changed', function(){
        
        waitsFor( function(){

            if ( $scope.sliders[0].position === undefined ){
                return false;
            }
            
            $scope.sliders[4].value = { range: 5, value: 16 };
            $scope.sliders[3].value = { range: 4, value: 3 };
            $scope.sliders[2].value = { range: 3, value: 210 };
            $scope.sliders[1].value = { range: 2, value: 3 };
            $scope.sliders[0].value = { range: 1, value: 4 };
            
            $scope.$apply();
            
            // expect the value slider to be at...
            expect( $scope.sliders[0].position.toFixed( 1 ) ).toBe( '136.3' );
            expect( $scope.sliders[1].position.toFixed( 1 ) ).toBe( '323.5' );
            expect( $scope.sliders[2].position.toFixed( 1 ) ).toBe( '520.4' );
            expect( $scope.sliders[3].position.toFixed( 1 ) ).toBe( '678.2' );
            expect( $scope.sliders[4].position.toFixed( 1 ) ).toBe( '921.0' );
            
            return true;
        }, 500 );
    });
    
    it( 'should draw correctly when sliders are changed via clicking', function(){
        
        waitsFor( function(){

            if ( $scope.sliders[0].position === undefined ){
                return false;
            }
            
            var bars = $wf.find( '.bar' );
            $(bars[4]).trigger( {type: 'click', clientX: 991} );
            $(bars[3]).trigger( {type: 'click', clientX: 755} );
            $(bars[2]).trigger( {type: 'click', clientX: 591} );
            $(bars[1]).trigger( {type: 'click', clientX: 400} );
            $(bars[0]).trigger( {type: 'click', clientX: 212} );
            
            $scope.$apply();
            
            expect( $scope.sliders[4].position.toFixed( 1 ) ).toBe( '921.0' );
            expect( $scope.sliders[4].value.range ).toBe( 5 );
            expect( $scope.sliders[4].value.value ).toBe( 16 );
            expect( $scope.sliders[3].position.toFixed( 1 ) ).toBe( '678.2' );
            expect( $scope.sliders[3].value.range ).toBe( 4 );
            expect( $scope.sliders[3].value.value ).toBe( 3 );
            expect( $scope.sliders[2].position.toFixed( 1 ) ).toBe( '520.4' );
            expect( $scope.sliders[2].value.range ).toBe( 3 );
            expect( $scope.sliders[2].value.value ).toBe( 210 );        
            expect( $scope.sliders[1].position.toFixed( 1 ) ).toBe( '323.5' );
            expect( $scope.sliders[1].value.range ).toBe( 2 );
            expect( $scope.sliders[1].value.value ).toBe( 3 ); 
            expect( $scope.sliders[0].position.toFixed( 1 ) ).toBe( '136.3' );
            expect( $scope.sliders[0].value.range ).toBe( 1 );
            expect( $scope.sliders[0].value.value ).toBe( 4 );             
            
            return true;
        }, 500 );
    });
    
    it( 'should draw correctly when sliders are changed via direct entry', function(){
        
        waitsFor( function(){

            if ( $scope.sliders[0].position === undefined ){
                return false;
            }
            
            var tooltips = $wf.find( '.display-only' );
            var editButton = $wf.find( '.icon-admin' );
            var editDialogs = $wf.find( '.edit-box' );
            var spinnerInput = $wf.find( 'input' );
            
            expect( tooltips.length ).toBe( 5 );
            expect( editButton.length ).toBe( 5 );
            expect( editDialogs.length ).toBe( 5 );
            expect( spinnerInput.length ).toBe( 5 );
            
            var setSliderVal = function( sliderIndex, dropdownIndex ){
                
                $(tooltips[sliderIndex]).trigger( {type: 'mouseenter'} );
                expect( $(editDialogs[sliderIndex]).css( 'display' ) ).toBe( 'none' );
                $(editButton[sliderIndex]).click();
                expect( $(editDialogs[sliderIndex]).css( 'display' ) ).toBe( 'block' );

                $(editDialogs[sliderIndex]).find( '.dropdown-toggle' ).click();
                var items = $(editDialogs[sliderIndex]).find( 'li' );

                $(editDialogs[sliderIndex]).find( 'a' )[dropdownIndex].click();
                
                $(editDialogs[sliderIndex]).find( '.set-value-button' ).click();
            };
            
            // set the yearlies to Forever
            setSliderVal( 4, 4 );
            // set the monthlies to 3 years
            setSliderVal( 3, 3 );
            // TODO: Figure out how to simluate typing into the spinner field correctly.
            $scope.sliders[3].value.value = 3;
            // set the weeklies to 210 days
            setSliderVal( 2, 2 );
            $scope.sliders[2].value.value = 210;
            // dailies to 3 weeks
            setSliderVal( 1, 1 );
            $scope.sliders[1].value.value = 3;
            // continuous to 4 days
            setSliderVal( 0, 0 );
            $scope.sliders[0].value.value = 4;
            
            $scope.$apply();
            
            expect( $scope.sliders[4].position.toFixed( 1 ) ).toBe( '921.0' );
            expect( $scope.sliders[4].value.range ).toBe( 5 );
            expect( $scope.sliders[4].value.value ).toBe( 16 );
            expect( $scope.sliders[3].position.toFixed( 1 ) ).toBe( '678.2' );
            expect( $scope.sliders[3].value.range ).toBe( 4 );
            expect( $scope.sliders[3].value.value ).toBe( 3 );
            expect( $scope.sliders[2].position.toFixed( 1 ) ).toBe( '520.4' );
            expect( $scope.sliders[2].value.range ).toBe( 3 );
            expect( $scope.sliders[2].value.value ).toBe( 210 );        
            expect( $scope.sliders[1].position.toFixed( 1 ) ).toBe( '323.5' );
            expect( $scope.sliders[1].value.range ).toBe( 2 );
            expect( $scope.sliders[1].value.value ).toBe( 3 ); 
            expect( $scope.sliders[0].position.toFixed( 1 ) ).toBe( '136.3' );
            expect( $scope.sliders[0].value.range ).toBe( 1 );
            expect( $scope.sliders[0].value.value ).toBe( 4 );             
            
            return true;
        }, 500 );
    });
    
    it( 'should move sliders to the right if a lower ranked one is moved right', function(){
        
        waitsFor( function(){

            if ( $scope.sliders[0].position === undefined ){
                return false;
            }
            
            $scope.sliders[0].value = { range: 4, value: 12 };
            
            $scope.$apply();
            
            expect( $scope.sliders[1].value.value ).toBe( 12 );
            expect( $scope.sliders[1].value.range ).toBe( 4 );
            expect( $scope.sliders[2].value.value ).toBe( 12 );
            expect( $scope.sliders[2].value.range ).toBe( 4 );
            expect( $scope.sliders[3].value.value ).toBe( 12 );
            expect( $scope.sliders[3].value.range ).toBe( 4 );
            expect( $scope.sliders[4].value.value ).toBe( 12 );
            expect( $scope.sliders[4].value.range ).toBe( 4 );
            
            expect( $scope.sliders[0].position.toFixed( 1 ) ).toBe( '813.2' );
            expect( $scope.sliders[1].position.toFixed( 1 ) ).toBe( '813.2' );
            expect( $scope.sliders[2].position.toFixed( 1 ) ).toBe( '813.2' );
            expect( $scope.sliders[3].position.toFixed( 1 ) ).toBe( '813.2' );
            expect( $scope.sliders[4].position.toFixed( 1 ) ).toBe( '813.2' );
            
            return true;
        }, 500 );
    });
    
    it( 'should not be able to push higher ranked sliders to the left when a low ranked on is moved', function(){
        
        waitsFor( function(){

            if ( $scope.sliders[0].position === undefined ){
                return false;
            }
            
            $scope.sliders[4].value = { range: 1, value: 1 };
            
            $scope.$apply();
            
            expect( $scope.sliders[4].value.range ).toBe( 3 );
            expect( $scope.sliders[4].value.value ).toBe( 60 );
            
            // expect the value slider to be at...
            expect( $scope.sliders[0].position.toFixed( 1 ) ).toBe( '46.3' );
            expect( $scope.sliders[1].position.toFixed( 1 ) ).toBe( '231.5' );
            expect( $scope.sliders[2].position.toFixed( 1 ) ).toBe( '277.5' );
            expect( $scope.sliders[3].position.toFixed( 1 ) ).toBe( '395.4' );
            expect( $scope.sliders[4].position.toFixed( 1 ) ).toBe( '395.4' );
            
            return true;
        }, 500 );
    });
});