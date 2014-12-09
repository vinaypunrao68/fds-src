require( './mockAuth' );
require( './mockSnapshot' );
require( './mockVolume' );
require( './mockTenant' );

addModule = function( moduleName, module ){

    if ( browser.params.mockServer === true ){
        browser.addMockModule( moduleName, module );
    }
};

addModule( 'user-management', mockAuth );
addModule( 'volume-management', mockVolume );
addModule( 'qos', mockSnapshot );
addModule( 'tenant-management', mockTenant );

login = function(){
    browser.get( '#/' );

    var uEl = element( by.model( 'username' ) );
    uEl = uEl.element( by.tagName( 'input' ) );
    var pEl = element( by.model( 'password' ) );
    pEl = pEl.element( by.tagName( 'input' ) );
    var button = element( by.id( 'login.submit' ) );

    uEl.clear();
    pEl.clear();
    uEl.sendKeys( 'admin' );
    pEl.sendKeys( 'admin' );

    button.click();
    browser.sleep( 200 );
};

logout = function(){

    var logoutMenu = element( by.id( 'main.usermenu' ) );
    var logoutEl = element( by.id( 'main.usermenu' ) ).all( by.tagName( 'li' ) ).last();

    logoutMenu.click();
    logoutEl.click();
    browser.sleep( 200 );
};

goto = function( tab ){

    var tab = element( by.id( 'main.' + tab ) );
    tab.click();
    browser.sleep( 200 );
};

createTenant = function( name ){
    
    goto( 'tenants' );
    var createLink = element( by.css( '.create-tenant-link' ) );
    createLink.click();

    var nameEl = element( by.css( '.tenant-name-input' )).element( by.tagName( 'input' ));
    nameEl.sendKeys( name );

    element( by.css( '.create-tenant-button' ) ).click();
    browser.sleep( 200 );
};

// timeline: [{ slider: #, value: #, unit: #index }... ]
createVolume = function( name, data_type, qos, timeline ){
    
    goto( 'volumes' );
    browser.sleep( 200 );
    
    var vContainer = element( by.css( '.volumecontainer' ));
    var createLink = vContainer.element( by.css( 'a.new_volume' ) );
    createLink.click();
    browser.sleep( 200 );
    
    var createEl = $('.create-panel.volumes');
    
    var nameText = element( by.model( 'volumeName') );
    nameText.sendKeys( name );
    
    if ( data_type ){
            
        var editDcButton = createEl.element( by.css( '.edit-data-connector-button' ) );
        editDcButton.click();

        var typeMenu = createEl.element( by.css( '.data-connector-dropdown' ) );
        var blockEl = typeMenu.all( by.tagName( 'li' ) ).first();
        var objectEl = typeMenu.all( by.tagName( 'li' ) ).last();

        typeMenu.click();
        
        if ( data_type.type === 'block' ){
            blockEl.click();
            
            var sizeSpinner = createEl.element( by.css( '.data-connector-size-spinner' )).element( by.tagName( 'input' ));
            sizeSpinner.clear();
            sizeSpinner.sendKeys( data_type.attributes.size );
        }
        else {
            objectEl.click();
        }

        var doneName = createEl.element( by.css( '.save-name-type-data' ));
        doneName.click();
    }// edit dc
    
    // let's set the qos stuff hooray!
    if ( qos ){
        
        // changing the QoS
        var editQosButton = createEl.element( by.css( '.qos-panel .icon-edit' ));
        editQosButton.click();
        
        browser.sleep( 200 );

        // panel should now be editable
        var slaEditDisplay = createEl.element( by.css( '.volume-sla-edit-display' ) );
        expect( slaEditDisplay.getAttribute( 'class' ) ).not.toContain( 'ng-hide' );
        
        var slaTableDisplay = createEl.element( by.css( '.volume-display-only' ) );
        expect( slaTableDisplay.getAttribute( 'class' ) ).toContain( 'ng-hide' );
        
        browser.actions().mouseMove( createEl.element( by.css( '.volume-priority-slider' ) ).all( by.css( '.segment' )).get( qos.priority - 1 ) ).click().perform();
        
        browser.actions().mouseMove( createEl.element( by.css( '.volume-iops-slider' ) ).all( by.css( '.segment' )).get( qos.capacity/10 ) ).click().perform();
        
        var limitSegment = 0;
        
        switch( qos.limit ){
            case 100:
                limitSegment = 0;
                break;
            case 200:
                limitSegment = 1;
                break;
            case 300:
                limitSegment = 2;
                break;
            case 400:
                limitSegment = 3;
                break;
            case 500:
                limitSegment = 4;
                break;
            case 750:
                limitSegment = 5;
                break;
            case 1000:
                limitSegment = 6;
                break;
            case 2000:
                limitSegment = 7;
                break;
            case 3000: 
                limitSegment = 8;
                break;
            default:
                limitSegment = 9;
                break;
        }
        
        browser.actions().mouseMove( createEl.element( by.css( '.volume-limit-slider' ) ).all( by.css( '.segment' )).get( limitSegment ) ).click().perform();
        
        var doneButton = createEl.element( by.css( '.save-qos-settings' ) );
        doneButton.click();
    }
    
    // set the timeline numbers
    if ( timeline ){
        
        var timelinePanel = createEl.element( by.css( '.protection-policy' ));
        
        // sneaky way to scroll down
        var hourChoice = timelinePanel.element( by.css( '.hour-choice' ));
        hourChoice.click();
        
        timelinePanel.element( by.css( '.icon-edit' )).click();
        
        var sliderWidget = createEl.element( by.css( '.waterfall-slider' ));
        
        for ( var i = 0; i < timeline.length; i++ ){
            
            var settings = timeline[i];
            
            // need to hover to show the edit button
            browser.actions().mouseMove( sliderWidget.all( by.css( '.display-only' )).get( settings.slider )).perform();
                        
            sliderWidget.all( by.css( '.icon-admin' )).get( settings.slider ).click();
            
            var dropdown = sliderWidget.all( by.css( '.dropdown' )).get( settings.slider );
            dropdown.element( by.tagName( 'button' )).click();
            dropdown.all( by.tagName( 'li' )).get( settings.unit ).click();
            
            // 4 = Forever and no number is allowed for forever
            if ( settings.unit !== 4 ){
                var spinner = sliderWidget.all( by.css( '.spinner-parent' )).get( settings.slider );
                var spinnerInput = spinner.element( by.tagName( 'input' ));
                spinnerInput.clear();
                spinnerInput.sendKeys( settings.value );
            }
            
            sliderWidget.all( by.css( '.set-value-button' )).get( settings.slider ).click();
            
            browser.sleep( 500 );
        }
        
        createEl.element( by.css( '.save-timeline' )).click();
    }
    
    element.all( by.buttonText( 'Create Volume' )).get( 0 ).click();
    browser.sleep( 300 );
};
