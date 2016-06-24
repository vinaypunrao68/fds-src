angular.module( 'base', [] );
angular.module( 'user-management', ['base'] );
angular.module( 'tenant-management', ['base'] );
angular.module( 'volume-management', ['base'] );
angular.module( 'node-management', ['base'] );
angular.module( 'activity-management', ['base'] );
angular.module( 'qos', ['base'] );
angular.module( 'statistics', ['base'] );

angular.module( 'modal-utils', [] );

angular.module( 'utility-directives', ['base'] );
angular.module( 'display-widgets', [] );
angular.module( 'angular-fui', [] );
angular.module( 'form-directives', ['utility-directives', 'angular-fui']);
angular.module( 'charts', ['utility-directives'] );

angular.module( 'main', ['user-management','templates-main'] );
angular.module( 'volumes', ['volume-management','form-directives','modal-utils', 'qos'] );
angular.module( 'system', ['node-management', 'user-management', 'utility-directives', 'display-widgets' ] );
angular.module( 'tenant', ['tenant-management', 'user-management', 'utility-directives', 'form-directives'] );
angular.module( 'admin-settings', ['user-management'] );
angular.module( 'status', ['activity-management', 'statistics', 'display-widgets', 'charts'] );
angular.module( 'debug', ['base', 'charts', 'form-directives'] );
angular.module( 'inbox', [] );
angular.module( 'user-page', ['user-management','tenant-management'] );
