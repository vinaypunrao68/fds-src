angular.module( 'user-management', [] );
angular.module( 'volume-management', [] );
angular.module( 'node-management', [] );
angular.module( 'activity-management', [] );
angular.module( 'qos', [] );
angular.module( 'statistics', [] );

angular.module( 'modal-utils', [] );

angular.module( 'utility-directives', [] );
angular.module( 'display-widgets', [] );
angular.module( 'angular-fui', [] );
angular.module( 'form-directives', ['utility-directives', 'angular-fui']);
angular.module( 'charts', ['utility-directives'] );

angular.module( 'main', ['user-management','templates-main'] );
angular.module( 'volumes', ['volume-management','form-directives','modal-utils', 'qos'] );
angular.module( 'system', ['node-management', 'user-management'] );
angular.module( 'admin-settings', ['user-management'] );
angular.module( 'status', ['activity-management', 'statistics', 'display-widgets', 'charts'] );
angular.module( 'inbox', [] );
angular.module( 'user-page', [] );
