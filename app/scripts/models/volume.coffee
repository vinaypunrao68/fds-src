define (require, exports, modules) ->
  Backbone = require('backbone')
  Utils = require('utils')

  class Volume extends Backbone.Model
    save: (attributes, options) ->
      options = _.defaults((options || {}), {method: 'PUT', url: "/#{@get('id')}/modPolicy"})
      Backbone.Model.prototype.save.call(this, attributes, options)
