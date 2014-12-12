describe( 'Test login permutations', function(){

//    var scope, mainController, authenticateservice,
//        $rootScope, $httpBackend, authorizationservice, $state, $domainservice;
//
//    beforeEach( module( 'user-management' ) );
//    beforeEach( module( 'main' ) );
//    beforeEach( module( 'ui.router' ) );
//
//    beforeEach( inject( function( $injector ){
//
//        $httpBackend = $injector.get( '$httpBackend' );
//
//        $httpBackend.when( 'POST', '/api/auth/token?login=admin&password=admin' )
//            .respond( { token: 'something' } );
//
//        $rootScope = $injector.get( '$rootScope' );
//
//        scope = $rootScope.$new();
//
//        var $controller = $injector.get( '$controller' );
//        authenticateservice = $injector.get( '$authentication' );
//        authorizationservice = $injector.get( '$authorization' );
//        $state = $injector.get( '$state' );
//        $domainservice = $injector.get( '$domain_service' );
//
//        $state.transitionTo = function( state ){
//        };
//
//        mainController = function(){
//            return $controller( 'mainController', {
//                '$scope': scope,
//                '$authentication': authenticateservice,
//                '$authorization': authorizationservice,
//                '$state': $state,
//                '$domain_service': $domainservice
//            });
//        };
//    }));
//
//    afterEach( function(){
//        $httpBackend.verifyNoOutstandingExpectation();
//        $httpBackend.verifyNoOutstandingRequest();
//    });
//
//    it ( 'should log in and out successfully', function(){
//        var controller = mainController();
//
//        scope.username = 'admin';
//        scope.password = 'admin';
//        scope.login();
//
//        $httpBackend.expectPOST( '/api/auth/token?login=admin&password=admin' )
//            .respond( { token: 'something' } );
//        $httpBackend.flush();
//
//        scope.$apply();
//        expect( scope.validAuth ).toBe( true );
//
//        scope.logout();
//        scope.$apply();
//        expect( scope.validAuth ).toBe( false );
//    });
});
