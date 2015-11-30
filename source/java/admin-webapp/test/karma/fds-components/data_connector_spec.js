describe( 'Test the data connector controller', function(){
    
    var $dc, $scope, $mock_dc_api, $iso_scope;
    
    beforeEach( function(){
        
        module( 'volumes' );
        module( 'templates-main' );
        
        module( function( $provide ){
            
            $provide.value( 'translateFilter', function( value ){ return value; } );
            $provide.factory( '$data_connector_api', function(){
                
                var service = {};
                
                service.types = [
                    {
                        type: 'BLOCK',
                        capacity: {
                            value: 10,
                            unit: 'GB'
                        }
                    },
                    {
                        type: 'OBJECT'
                    },
                    {
                        type: 'ISCSI',
                        capacity: {
                            value: 10,
                            unit: 'GB'
                        },
                        target: {
                            incomingUsers: [
                            ]
                        }
                    }
                ];
                
                service.getVolumeTypes = function( callback ){
                
                    callback( service.types );
                };
                
                return service;
            });
        });
        
        inject( function( $compile, $rootScope, $data_connector_api ){
            
            $mock_dc_api = $data_connector_api;
            var html = '<connector-panel ng-model="volumeType" enable="true"></connector-panel>';
            $dc = angular.element( html );
            $scope = $rootScope.$new();
            compEl = $compile( $dc )( $scope );
            
            $('body').append( $dc );
            $scope.$digest();    
            
            $iso_scope = compEl.isolateScope();
        });
    });
    
    it( 'Test that block shows the capacity block but not the iSCSI block', function(){
        
        $scope.volumeType = $mock_dc_api.types[0];
        $scope.$apply();

        var bs = $dc.find( '.block-settings' );
        var is = $dc.find( '.iscsi-settings' );
        expect( $(bs[0]).css( 'display' ) ).not.toContain( 'none' );
        expect( $(is[0]).css( 'display' ) ).toContain( 'none' );
    });
    
    it( 'Test that object shows neither block', function(){
        $scope.volumeType = $mock_dc_api.types[1];
        $scope.$apply();
        
        var bs = $dc.find( '.block-settings' );
        var is = $dc.find( '.iscsi-settings' );
        expect( $(bs[0]).css( 'display' ) ).toContain( 'none' );
        expect( $(is[0]).css( 'display' ) ).toContain( 'none' );
    });
    
    it( 'Test that iSCSI shows both blocks', function(){
        $scope.volumeType = $mock_dc_api.types[2];
        $scope.$apply();
        
        var bs = $dc.find( '.block-settings' );
        var is = $dc.find( '.iscsi-settings' );
        expect( $(bs[0]).css( 'display' ) ).not.toContain( 'none' );
        expect( $(is[0]).css( 'display' ) ).not.toContain( 'none' );
    });    

    it( 'Test that the username stuff works for iSCSI', function(){
        
        $iso_scope.volumeType = $mock_dc_api.types[2];
        $iso_scope._username = 'Fred';
        $iso_scope._password = 'Willard';
        $iso_scope.$apply();
        
        $iso_scope.refreshSelection();
        
        expect( $iso_scope.volumeType.target.incomingUsers[0].username ).toBe( 'Fred' );
        expect( $iso_scope.volumeType.target.incomingUsers[0].password ).toBe( 'Willard' );
    });
});