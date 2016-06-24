describe( 'Testing the status timeline widget', function(){
    
    var $scope, $timeline;
    
    beforeEach( function(){

        module( 'display-widgets' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<status-timeline ng-model="firebreakModel" domain="firebreakDomain" range="firebreakRange" date-range="lastTwentyFour" divisions="24" icon-class="icon-firebreak" right-label="Now" left-label="24 hours ago" middle-label="12"></status-timeline>';

            $scope = $rootScope.$new();
            $timeline = angular.element( html );
            $compile( $timeline )( $scope );
            
            $scope.firebreakModel = [];
            $scope.lastTwentyFour = { start: ((new Date()).getTime()/1000) - (24*60*60), end: ((new Date()).getTime()/1000 ) };
            $scope.firebreakDomain = [ 3600*24, 3600*12, 3600*6, 3600*3, 3600, 0 ];
            $scope.firebreakRange = ['#389604', '#68C000', '#C0DF00', '#FCE300', '#FD8D00', '#FF5D00'];
            
            $( 'body' ).append( $timeline );
            $scope.$digest();
        });
    });
    
    it( 'should have 24 hours and all should be green', function(){
        
        var buttons = $timeline.find( '.color-box' );
        expect( buttons.length ).toBe( 24 );
        
        for ( var i = 0; i < buttons.length; i++ ){
            expect( $(buttons[i]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        }
    });
    
    it ( 'should show a firebreak on the box associated with 20 hours ago', function(){
        
        $scope.firebreakModel = [
            { x: ((new Date()).getTime()/1000) - 20.1*60*60, y: 0 }
        ];
        
        $scope.$apply();
        
        var buttons = $timeline.find( '.color-box' );
        
        for ( var i = 0; i < 3; i++ ){
            expect( $(buttons[i]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        }
        
        expect( $(buttons[3]).css( 'background-color' ) ).toBe( 'rgb(255, 93, 0)' );
        expect( $(buttons[4]).css( 'background-color' ) ).toBe( 'rgb(253, 141, 0)' );
        expect( $(buttons[5]).css( 'background-color' ) ).toBe( 'rgb(252, 227, 0)' );
        expect( $(buttons[6]).css( 'background-color' ) ).toBe( 'rgb(252, 227, 0)' );
        expect( $(buttons[7]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[8]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[9]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[10]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[11]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[12]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[13]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[14]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[15]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[16]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[17]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[18]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[19]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[20]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[21]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[22]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[23]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        
        var event = $timeline.find( '.event' );
        expect( event.css( 'left' ) ).toBe( ((23-20)*22) + 'px' );
    });
    
    it( 'should handle more than one firebreak with appropriate coloring', function(){
        
        var buttons = $timeline.find( '.color-box' );
        expect( buttons.length ).toBe( 24 );
        
        for ( var i = 0; i < buttons.length; i++ ){
            expect( $(buttons[i]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        }
    });
    
    it ( 'should show a firebreak on the box associated with 20 hours ago', function(){
        
        $scope.firebreakModel = [
            { x: ((new Date()).getTime()/1000) - 20.1*60*60, y: 0 },
            { x: ((new Date()).getTime()/1000) - 3.1*60*60, y: 0 }
        ];
        
        $scope.$apply();
        
        var buttons = $timeline.find( '.color-box' );
        
        expect( $(buttons[0]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[1]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[2]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[3]).css( 'background-color' ) ).toBe( 'rgb(255, 93, 0)' );
        expect( $(buttons[4]).css( 'background-color' ) ).toBe( 'rgb(253, 141, 0)' );
        expect( $(buttons[5]).css( 'background-color' ) ).toBe( 'rgb(252, 227, 0)' );
        expect( $(buttons[6]).css( 'background-color' ) ).toBe( 'rgb(252, 227, 0)' );
        expect( $(buttons[7]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[8]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[9]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[10]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[11]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[12]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[13]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[14]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[15]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[16]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[17]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[18]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[19]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[20]).css( 'background-color' ) ).toBe( 'rgb(255, 93, 0)' );
        expect( $(buttons[21]).css( 'background-color' ) ).toBe( 'rgb(253, 141, 0)' );
        expect( $(buttons[22]).css( 'background-color' ) ).toBe( 'rgb(252, 227, 0)' );
        expect( $(buttons[23]).css( 'background-color' ) ).toBe( 'rgb(252, 227, 0)' );
        
        var event = $timeline.find( '.event' );
        expect( $(event[0]).css( 'left' ) ).toBe( ((23-20)*22) + 'px' );
        expect( $(event[1]).css( 'left' ) ).toBe( ((23-3)*22) + 'px' );
    });
    
    it( 'should handle values outside of the range that may affect the color of the first square', function(){
        
        $scope.firebreakModel = [
            { x: ((new Date()).getTime()/1000) - 26.1*60*60, y: 0 },
            { x: ((new Date()).getTime()/1000) - 20.1*60*60, y: 0 }
        ];
        
        $scope.$apply();
        
        var buttons = $timeline.find( '.color-box' );
        
        expect( $(buttons[0]).css( 'background-color' ) ).toBe( 'rgb(252, 227, 0)' );
        expect( $(buttons[1]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[2]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[3]).css( 'background-color' ) ).toBe( 'rgb(255, 93, 0)' );
        expect( $(buttons[4]).css( 'background-color' ) ).toBe( 'rgb(253, 141, 0)' );
        expect( $(buttons[5]).css( 'background-color' ) ).toBe( 'rgb(252, 227, 0)' );
        expect( $(buttons[6]).css( 'background-color' ) ).toBe( 'rgb(252, 227, 0)' );
        expect( $(buttons[7]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[8]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[9]).css( 'background-color' ) ).toBe( 'rgb(192, 223, 0)' );
        expect( $(buttons[10]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[11]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[12]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[13]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[14]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[15]).css( 'background-color' ) ).toBe( 'rgb(104, 192, 0)' );
        expect( $(buttons[16]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[17]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[18]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[19]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[20]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[21]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[22]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        expect( $(buttons[23]).css( 'background-color' ) ).toBe( 'rgb(56, 150, 4)' );
        
        var event = $timeline.find( '.event' );
        expect( $(event[0]).hasClass( 'ng-hide' ) ).toBe( true );
        expect( $(event[1]).css( 'left' ) ).toBe( ((23-20)*22) + 'px' );
    });

});