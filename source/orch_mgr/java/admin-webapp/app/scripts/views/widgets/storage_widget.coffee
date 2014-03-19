define (require, exports, modules) ->
  Backbone = require('backbone')
  templates = require('templates')

  class StorageWidget extends Backbone.View
    template: templates['app/scripts/templates/dashboard/storage_widget.jst']
    className: 'block_4'

    initialize: (opts) ->
      @opts = opts

    render: ->
      @$el.html(@template(@opts))
      this
