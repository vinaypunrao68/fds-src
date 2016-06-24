describe( 'Testing our byte converter utility', function(){
    
    var byte_converter;
    
    beforeEach( module( 'base' ) );
    
    beforeEach( function(){
        
        var fakeFilter; 
        
        fakeFilter = function( value ){
            
            switch( value ){
                case 'common.bytes':
                    return 'Bytes';
                case 'common.kb':
                    return 'KB';
                case 'common.mb':
                    return 'MB';
                case 'common.gb':
                    return 'GB';
                case 'common.tb':
                    return 'TB';
                case 'common.pb':
                    return 'PB';
                case 'common.eb':
                    return 'EB';
                default:
                    return value;
            }
        };
        
        module( function( $provide ){
            $provide.value( 'translateFilter', fakeFilter );
        });
    });
    
    beforeEach( inject( function( _$byte_converter_ ){
        byte_converter = _$byte_converter_;
    }));
    
    it ( 'should convert from bytes to a string', function(){
        
        var str = byte_converter.convertBytesToString( 35241 );
        expect( str ).toBe( '34.42 KB' );
        
        str = byte_converter.convertBytesToString( 1048575 );
        expect( str ).toBe( '1024.00 KB' );
        
        str = byte_converter.convertBytesToString( 1e15 );
        expect( str ).toBe( '909.49 TB' );
        
        str = byte_converter.convertBytesToString( 1048576 );
        expect( str ).toBe( '1.00 MB' );
        
        str = byte_converter.convertBytesToString( 12345678901 );
        expect( str ).toBe( '11.50 GB' );
        
        str = byte_converter.convertBytesToString( 241 );
        expect( str ).toBe( '241 Bytes' );
        
    });
    
    it ( 'should convert from some given unit to a readable string', function(){
        
        var str = byte_converter.convertFromUnitsToString( 34.42, 'KB' );
        expect( str ).toBe( '34.42 KB' );
        
        var str = byte_converter.convertFromUnitsToString( 23192, 'MB' );
        expect( str ).toBe( '22.65 GB' );
        
        var str = byte_converter.convertFromUnitsToString( 1234567890123, 'MB' );
        expect( str ).toBe( '1.12 EB' );
    });
    
    iit( 'should handle zeros', function(){
        
        var str = byte_converter.convertBytesToString( 0.0 );
        expect( str ).toBe( '0 Bytes' );
        
        var str = byte_converter.convertFromUnitsToString( 0.0, 'MB' );
        expect( str ).toBe( '0 Bytes' );
    });
    
});