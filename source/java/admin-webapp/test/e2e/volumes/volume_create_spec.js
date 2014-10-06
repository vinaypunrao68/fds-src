require( '../utils' );

describe( 'Testing volume creation permutations', function(){

    browser.get( '#/' );
    var createLink;
    var mainEl;
    var createEl;
    var newText;

    var mockVol = function(){
        angular.module( 'volume-management', [] ).factory( '$volume_api', function(){

            var volService = {};
            volService.volumes = [];

            volService.delete = function( volume ){

                var temp = [];

                volService.volumes.forEach( function( v ){
                    if ( v.name != volume.name ){
                        temp.push( v );
                    }
                });

                volService.volumes = temp;
            };

            volService.save = function( volume, callback ){

                volService.volumes.push( volume );

                if ( angular.isDefined( callback ) ){
                    callback( volume );
                }
            };

            volService.getSnapshots = function( volumeId, callback, failure ){
                //callback( [] );
            };

            volService.getSnapshotPoliciesForVolume = function( volumeId, callback, failure ){
//                callback( [] );
            };

            volService.refresh = function(){};

            return volService;
        });

        angular.module( 'volume-management' ).factory( '$data_connector_api', function(){

            var api = {};
            api.connectors = [];

            var getConnectorTypes = function(){
        //        return $http.get( '/api/config/data_connectors' )
        //            .success( function( data ){
        //                api.connectors = data;
        //            });

                api.connectors = [{
                    type: 'Block',
                    api: 'Basic, Cinder',
                    options: {
                        max_size: '100',
                        unit: ['GB', 'TB', 'PB']
                    },
                    attributes: {
                        size: '10',
                        unit: 'GB'
                    }
                },
                {
                    type: 'Object',
                    api: 'S3, Swift'
                }];
            }();

            api.editConnector = function( connector ){

                api.connectors.forEach( function( realConnector ){
                    if ( connector.type === realConnector.type ){
                        realConnector = connector;
                    }
                });
            };

            return api;
        });
    };

    var mockSnap = function(){
        angular.module( 'qos' ).factory( '$snapshot_api', ['$http', function( $http ){

            var service = {};

            service.createSnapshotPolicy = function( policy, callback, failure ){
            };

            service.deleteSnapshotPolicy = function( policy, callback, failure ){
            };

            service.attachPolicyToVolue = function( policy, volumeId, callback, failure ){
            };

            service.detachPolicy = function( policy, volumeId, callback, failure ){
            };

            service.cloneSnapshotToNewVolume = function( snapshot, volumeName, callback, failure ){
            };

            return service;
        }]);
    };

    browser.addMockModule( 'volume-management', mockVol );
    browser.addMockModule( 'qos', mockSnap );

    it( 'should not find any volumes in the table', function(){

        login();
        goto( 'volumes' );

        browser.sleep( 200 );

        createLink = element( by.css( 'a.new_volume') );
        createEl = $('.create-panel.volumes');
        createButton = element( by.buttonText( 'Create Volume' ) );
        mainEl = element.all( by.css( '.slide-window-stack-slide' ) ).get(0);
        newText = element( by.model( 'name') );

        var volumeTable = $('tr');

        expect( volumeTable.length ).toBe( undefined );
    });

    it( 'pressing create should show the create dialog and push the screen to the left', function(){

        createLink.click();
        browser.sleep( 200 );
        expect( createEl.getAttribute( 'style' ) ).not.toContain( 'hidden' );
        expect( mainEl.getAttribute( 'style' ) ).toContain( '-100%' );
    });

    it( 'should save with just a name', function(){
        newText.sendKeys( 'Test Volume' );
        browser.sleep( 100 );
        createButton.click();

        expect( mainEl.getAttribute( 'style' ) ).not.toContain( '-100%' );

        element.all( by.repeater( 'volume in volumes' )).then( function( results ){
            expect( results.length ).toBe( 1 );
        });


        element.all( by.css( '.volume-row .name' ) ).then( function( nameCols ){

            nameCols.forEach( function( td ){
                td.getText().then( function( txt ){
                    expect( txt ).toBe( 'Test Volume' );
                });
            });
        });

        // check that the default priority was correct
        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
            priorityCols.forEach( function( td ){
                td.getText().then( function( txt ){
                    expect( txt ).toBe( '10' );
                });
            });
        });
    });

    it ( 'should be able to edit the priority of a volume', function(){

        var edit = element( by.css( '.button-group span.fui-new' ) );
        edit.click();

        browser.sleep( 210 );

        var editRow = element.all( by.css( '#volume-edit-row' ) ).get( 0 );
        expect( editRow.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );

        var priority8 = element( by.css( '#volume-edit-row section:first-child .ui-slider-segment:nth-child(6)' ));

        priority8.click();
        browser.sleep( 200 );

        var saveButton = element.all( by.css( '.volume-edit-row' )).get( 0 ).element( by.buttonText( 'Save' ) );
        saveButton.click();
        browser.sleep( 210 );

        expect( editRow.getAttribute( 'class' ) ).toContain( 'ng-hide' );

        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
            priorityCols.forEach( function( td ){
                td.getText().then( function( txt ){
                    expect( txt ).toBe( '4' );
                });
            });
        });

    });

//    it( 'should be able to delete a volume', function(){
//
//        var delButton = element( by.css( 'span.fui-cross' ) );
//        delButton.click();
//        browser.sleep( 100 );
//
//        var alert = browser.switchTo().alert();
//        alert.accept();
//
//        browser.sleep( 200 );
//
//        element.all( by.css( 'volume-row' )).then( function( rows ){
//            expect( rows.length ).toBe( 0 );
//        });
//    });

    it( 'should be able to cancel editing and show default values on next entry', function(){

        createLink.click();
        browser.sleep( 210 );

        newText.sendKeys( 'This should go away' );

        var cancelButton = element( by.css( 'button.cancel-creation' ));
        cancelButton.click();
        browser.sleep( 210 );

        expect( mainEl.getAttribute( 'style' ) ).not.toContain( '-100%' );

        createLink.click();
        browser.sleep( 210 );

        newText.getText().then( function( txt ){
            expect( txt ).toBe( '' );
        });

        cancelButton.click();
        browser.sleep( 210 );
    });

    it( 'should be able to create a volume and edit each portion', function(){

        createLink.click();
        browser.sleep( 200 );

        newText.sendKeys( 'Complex Volume' );

        // messing with the chosen data connector
        var dcEditPanel = element( by.css( '.volume-data-type-panel' ));
        expect( dcEditPanel.getAttribute( 'class' ) ).not.toContain( 'open' );

        element( by.css( '.volume-data-type-summary .fui-new')).click();
        browser.sleep( 210 );

        expect( dcEditPanel.getAttribute( 'class' ) ).toContain( 'open' );

        // switching the selection
        element( by.css( '.volume-data-type-panel tr:last-child td:first-child')).click();
        element( by.css( '.volume-data-type-panel' )).element( by.buttonText( 'Done' ) ).click();

        browser.sleep( 210 );

        expect( dcEditPanel.getAttribute( 'class' ) ).not.toContain( 'open' );

        var dataTypeEl = element.all( by.css( '.volume-data-type-summary span.value' )).get( 0 );
        dataTypeEl.getText().then( function( txt ){
            expect( txt ).toBe( 'Object' );
        });

        // changing the QoS
        var qosEditEl = element( by.css( '.qos-panel .volume-edit-container'));
        expect( qosEditEl.getAttribute( 'class' )).not.toContain( 'open' );

        element( by.css( '.volume-qos-summary-container span.fui-new' )).click();
        browser.sleep( 210 );
        expect( qosEditEl.getAttribute( 'class' )).toContain( 'open' );

        // set the priority to 2
        var priority2 = element( by.css( '.qos-panel .volume-edit-container section:first-child .ui-slider-segment:nth-child(8)' ));
        priority2.click();

        var sla90 = element( by.css( '.qos-panel .volume-edit-container section:nth-child(2) .ui-slider-segment:nth-child(8)' ));
        sla90.click();

        var limit = element( by.css( '.qos-panel .volume-edit-container section:nth-child(3) .ui-slider-segment:nth-child(4)' ));
        limit.click();

        browser.sleep( 210 );

        element( by.css( '.qos-panel .volume-edit-container' )).element( by.buttonText( 'Done' )).click();
        browser.sleep( 210 );

        expect( qosEditEl.getAttribute( 'class' )).not.toContain( 'open' );

        element.all( by.css( '.volume-qos-summary-container span.value' )).then( function( list ){

            // the fourth is the % sign
            expect( list.length ).toBe( 4 );

            list[0].getText().then( function( txt ){
                expect( txt ).toBe( '2' );
            });

            list[1].getText().then( function( txt ){
                expect( txt ).toBe( '80%' );
            });

            list[3].getText().then( function( txt ){
                expect( txt ).toBe( '400 IOPs' );
            });
        });

        element( by.buttonText( 'Create Volume' )).click();
        browser.sleep( 300 );

//        element.all( by.css( '.volume-row .priority' ) ).then( function( priorityCols ){
//            priorityCols.forEach( function( td ){
//                td.getText().then( function( txt ){
//                    expect( txt ).toBe( '2' );
//                });
//            });
//        });

//        element( by.css( 'span.fui-cross' )).click();
//        browser.sleep( 200 );
//        browser.switchTo().alert().accept();
    });

});
