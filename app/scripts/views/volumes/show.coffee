define (require, exports, modules) ->
  _ = require('underscore')
  Backbone = require('backbone')
  templates = require('templates')

  class VolumeShowView extends Backbone.View
    template: templates['app/scripts/templates/volumes/show.jst']
    className: 'stage'

    initialize: ->
      @widgets = []
      # TODO: @widgets.push new Formation.Views.StorageUsageWidget({})
      # TODO: @widgets.push new Formation.Views.SingleVolumeWidget({volume: @model})

    render: ->
      @$el.html(@template(volume: @model))
      # TODO: widget_html = (widget.render().el for widget in @widgets)
      # TODO: @$el.append(widget_html...)
      this
