describe( 'Testing the modular tiering choice panels', function(){
    
    var $scope, $panel;
    
    beforeEach( function(){
        
        var fakeFilter;
        
        fakeFilter = function( value ){
            return value;
        };
        
        mock_media = {
            availablePolicies: [
                { label: 'SSD Only', value: 'SSD' },
                { label: 'HDD Only', value: 'HDD' },
                { label: 'Hybrid policy', value: 'HYBRID' }
            ],
            getSystemCapabilities: function( callback ){
                callback( this.availablePolicies );
            },
            convertRawToObjects: function( policy ){
        
                for ( var i = 0; i < this.availablePolicies.length; i++ ){
                    if ( this.availablePolicies[i].value === policy ){
                        return this.availablePolicies[i];
                    }
                }

                return null;
            }
        };
        
        module( 'utility-directives' );
        
        module( function( $provide ){
            $provide.value( 'translateFilter', fakeFilter );
            $provide.value( '$media_policy_helper', mock_media );
        });
        
        module( 'volumes' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<tiering-panel ng-model="mediaPolicy"></tiering-panel>';
            
            $scope = $rootScope.$new();
            $panel = angular.element( html );
            $compile( $panel )( $scope );
            
            $scope.mediaPolicy = {};
            
            $( 'body' ).append( $panel );
            $scope.$digest();
        });
    });
    
    it( 'should set the media policy to hdd preferred', function(){

        $scope.mediaPolicy = 1;
        $scope.$apply();

        var buttons = $panel.find('.button');
        
        expect( $(buttons[1]).hasClass( 'selected' ) ).toBe( true );    
        expect( $scope.mediaPolicy.value ).toBe( 'HDD' );
    });
    
    it( 'should set the media policy to SSD only', function(){
        
        $scope.mediaPolicy = {};
        $scope.$apply();

        var buttons = $panel.find('.button');
        
        expect( $(buttons[1]).hasClass( 'selected' ) ).toBe( true );
        expect( $scope.mediaPolicy.value ).toBe( 'HDD' );
    });
    
    it( 'should set the media policy to HDD only', function(){
        
        $scope.mediaPolicy = 'HYBRID';
        $scope.$apply();
        
        var buttons = $panel.find('.button');
        
        expect( $(buttons[2]).hasClass( 'selected' ) ).toBe( true );
        expect( $scope.mediaPolicy.value ).toBe( 'HYBRID' );
    });
});