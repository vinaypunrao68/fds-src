describe( 'Summary number display functionality', function(){
    
    var $scope, $switch;
    
    beforeEach( function(){
        
        module( 'display-widgets' );
        module( 'templates-main' );
        
        inject( function( $compile, $rootScope ){
            
            var html = '<summary-number-display ng-model="calculatedData" auto-change="false"></summary-number-display>';
            $scope = $rootScope.$new();
            $sDisplay = angular.element( html );
            $compile( $sDisplay )( $scope );
            
            $scope.calculatedData = [
                {
                    number: 3000.01,
                    description: 'Chipmunks singing right now',
                    suffix: 'CSN'
                },
                {
                    numer: 0.005,
                    description: 'Chipmunks without songbooks',
                    suffic: 'CKS'
                }
            ]
            
            $('body').append( $sDisplay );
            $scope.$digest();
            
        });
    });
   
    it( 'should display the first item and do the maths correctly', function(){
        
        var mainNumber = $sDisplay.find( '.main-number' );
        var decimalNumber = $sDisplay.find( '.decimal-number' );
        
        expect( $(mainNumber).text().trim() ).toBe( '3000' );
        expect( $(decimalNumber).text().trim() ).toBe( '.01' );
    });
});