#/*global require*/
'use strict'

require.config
  deps: ['main']
  shim:
    underscore:
      exports: '_'
    backbone:
      deps: [
        'underscore'
        'jquery'
      ]
      exports: 'Backbone'
    d3:
      exports: 'd3'
    nvd3:
      exports: 'nv'
      deps: ['d3']
    magnific:
      deps: ['jquery']

  paths:
    jquery: '../bower_components/jquery/jquery'
    backbone: '../bower_components/backbone/backbone'
    underscore: '../bower_components/underscore/underscore'
    d3: '../bower_components/d3/d3'
    fastclick: '../bower_components/fastclick/lib/fastclick'
    magnific: '../bower_components/magnific-popup/dist/jquery.magnific-popup'
    nvd3: 'vendor/nvd3/nv.d3' # Custom chart additions, so vendor for now

require ['main'], () ->
