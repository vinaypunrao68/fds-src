angular.module( 'modal-utils' ).factory( '$modal_data_service', function(){

    var service = {};
    var data = {};

    service.start = function(){
        data = {};
    };

    service.update = function( obj ){
        data = $.extend( {}, data, obj );
    };

    service.getData = function(){
        return data;
    };

    return service;
});
