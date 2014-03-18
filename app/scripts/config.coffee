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
  paths:
    jquery: '../bower_components/jquery/jquery'
    backbone: '../bower_components/backbone/backbone'
    underscore: '../bower_components/underscore/underscore'
    d3: '../bower_components/d3/d3'
    fastclick: '../bower_components/fastclick/lib/fastclick'
    magnific_popup: '../bower_components/magnific_popup/dist/jquery.magnific_popup'
    nvd3: '../bower_components/nvd3/nv.d3'

require ['main'], () ->
