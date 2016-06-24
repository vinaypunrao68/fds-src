describe( 'Textlist functionality', function(){
    
    var $scope, $list, $iso_scope;
    
    beforeEach( function(){
        
        module( 'form-directives' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<fds-text-list ng-model="texts" placeholder="Enter Text" add-label="Sou"></fds-text-list>';
            $scope = $rootScope.$new();
            $list = angular.element( html );
            var compEl = $compile( $list )( $scope );
            
            $scope.texts = ["one"];
            
            $('body').append( $list );
            $scope.$digest();
            
            $iso_scope = compEl.isolateScope();
        });
    });
    
    it( 'Should start with one item in the list', function(){
        
        expect( $scope.texts.length ).toBe( 1 );
        var itemCount = $list.find( '.fds-text-list-item' ).length;
        
        expect( itemCount ).toBe( 1 );
    });
    
    it( 'should be able to add another line', function(){
        
        $iso_scope.addRow();
        $iso_scope.$apply();
        
        expect( $scope.texts.length ).toBe( 2 );
        expect( $scope.texts[1] ).toBe( '' );
    });
    
    it( 'should be able to set a value on the new line', function(){
        $scope.texts[1] = 'Fishes';
        $scope.$apply();
        
        expect( $scope.texts.length ).toBe( 2 );
        expect( $list.find( '.fds-text-list-item' ).length ).toBe( 2 );
        
    });
    
    it( 'should be able to delete a row', function(){
        $iso_scope.deleteRow( 0 );
        $iso_scope.$apply();
        
        // takes a second for the change to get applied throughout the scopes.
        setTimeout( function(){
            expect( $scope.texts.length ).toBe( 1 );
            expect( $scope.texts[0] ).toBe( 'Fishes' );
        }, 1000 );
    });
    
    it( 'should reflect a row being deleted in the ui', function(){
        
        expect( $list.find( '.fds-text-list-item' ).length ).toBe( 1 );
        
        $scope.texts.splice( 0, 1 );
        $scope.$apply();
        
        expect( $list.find( '.fds-text-list-item' ).length ).toBe( 0 );
    });
});