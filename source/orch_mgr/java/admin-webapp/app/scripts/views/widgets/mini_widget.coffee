define (require, exports, modules) ->
  Backbone = require('backbone')
  templates = require('templates')

  class MiniWidget extends Backbone.View
    template: templates['app/scripts/templates/dashboard/mini_widget.jst']
    className: 'block_1'

    initialize: (opts) ->
      @opts = opts
      @opts.bar_color = opts.bar_class || 'green'
      @opts.kind = opts.kind || 'string'
      if @opts.kind == 'percentage'
        num_parts = "#{@opts.value}".split('.')
        @opts.value = "#{num_parts[0]}<span>.#{num_parts[1]}%</span>"

    render: ->
      @$el.html(@template(@opts))
      this
