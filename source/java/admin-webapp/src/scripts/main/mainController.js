angular.module( 'main' ).controller( 'mainController', ['$scope', '$authentication', '$authorization', '$state', '$domain_service', '$filter', function( $scope, $authentication, $authorization, $state, $domain_service, $filter ){
    
    $scope.priority = 10;
    $scope.menuWidth = 38;
    $scope.rememberChecked = false;
    $scope.selected = {};
    $scope.domains = $domain_service.getDomains();
//    $scope.loggedInUser = $authorization.getUsername();

    $scope.menuItems = [{name: 'Account Details'},{name: 'Change Password'},{name: '-'},{ name: 'Logout'}];

    $scope.views = [
        { 
            id: 'status', link: 'homepage.status', 
            text: $filter( 'translate' )('status.title' ), 
            iconClass: 'icon-system', 
            selected: false },
//        { 
//            id: 'inbox', 
//            link: 'homepage.inbox', 
//            text: $filter( 'translate' )('inbox.title' ), 
//            iconClass: 'icon-inbox', 
//            selected: false },
        { 
            id: 'activity', 
            link: 'homepage.activity', 
            text: $filter( 'translate' )('activities.title' ), 
            iconClass: 'icon-activity', 
            selected: false },
        { 
            id: 'system', 
            link: 'homepage.system', 
            text: $filter( 'translate' )('system.title' ), 
            iconClass: 'icon-nodes', 
            selected: false, 
            permission: SYS_MGMT },
        { 
            id: 'volumes', 
            link: 'homepage.volumes', 
            data: { state: 'list' },
            text: $filter( 'translate' )('volumes.title' ), 
            iconClass: 'icon-volumes', 
            selected: false },
        { 
            id: 'users', 
            link: 'homepage.users', 
            text: $filter( 'translate' )('users.title' ), 
            iconClass: 'icon-users', 
            selected: false,
            permission: SYS_MGMT
        },
         { 
             id: 'tenants', 
             link: 'homepage.tenants', 
             text: $filter( 'translate' )('tenants.title' ), 
             iconClass: 'icon-tenant', 
             selected: false, 
             permission: SYS_MGMT }
//        { 
//            id: 'admin', 
//            link: 'homepage.admin', 
//            text: $filter( 'translate' )('admin.title' ), 
//            iconClass: 'icon-admin', 
//            selected: false }
    ];

    $scope.showMenu = function(){
        $scope.menuWidth = 115;
    };

    $scope.hideMenu = function(){
        $scope.menuWidth = 38;
    };

    $scope.navigate = function( item ) {

        $scope.views.forEach ( function( view ){
            view.selected = false;
        });

        item.selected = true;

        $state.transitionTo( item.link, item.data, {reload: true} );
    };

    $scope.login = function( event ){
        
        // this will stop it from processing anything where username and password don't exist.
        // in Safari hitting 'enter' on the password field triggers two login attempts
        // so this helps block the second one.
        if ( !angular.isDefined( $scope.username ) || !angular.isDefined( $scope.password ) 
           || ( $scope.username == '' && $scope.password == '' ) ){
            return;
        }

        $authentication.login( $scope.username, $scope.password );
    };

    $scope.logout = $authentication.logout;

    $scope.isAllowed = function( permission ){

        if ( !angular.isDefined( permission ) ){
            return true;
        }

        return $authorization.isAllowed( permission );
    };

    $scope.keyEntered = function( $event ){
        if ( $event.keyCode == 13 ){
            $scope.login();
        }
        else {
            $scope.error = '';
        }
    };

    $scope.itemSelected = function( item ){

        if ( item === $scope.menuItems[3] ){
            $scope.logout();
        }
        else if ( item === $scope.menuItems[0] || item === $scope.menuItems[1] ){
            var o = {
                link: 'homepage.account'
            };

            $scope.navigate( o );
        }
    };

    $scope.$watch( 'loggedInUser', function( newValue ){

        $scope.itemSelected( newValue );
    });

    $scope.$on( 'fds::authentication_logout', function(){
        $state.transitionTo( 'homepage' );
    });

    var determineWhereToGo = function(){
        
        if ( $scope.validAuth === false ){
            return;
        }
        
        var noState = true;
        
        for ( var i = 0; i < $scope.views.length; i++ ){

            var view = $scope.views[i];
            view.selected = false;

            if ( $state.current.name === view.link ){
                view.selected = true;
                noState = false;
                return;
            }
        }

        if ( noState && $state.current.name !== 'homepage.account' ){
            // if we get here, nothing was selected
            $scope.navigate( $scope.views[0] );
        }
    };
    
    $scope.$on( 'fds::authentication_success', determineWhereToGo );

    $scope.$watch( function(){ return $authentication.isAuthenticated; }, function( val ) {
        $scope.loggedInUser = $authorization.getUsername();
        
        if ( !angular.isDefined( $scope.loggedInUser ) ){
            $scope.validAuth = false;
            return;
        }
        
        $scope.validAuth = val;
        $scope.username = '';
        $scope.password = '';
        
        determineWhereToGo();
    });

    $scope.$watch( function(){ return $authentication.error; }, function( val ) {
        
        $scope.error = val;
    });
    
    determineWhereToGo();

    //trigger to kick us out if we are not authorized anymore
    $authorization.validateUserToken();

}]);
