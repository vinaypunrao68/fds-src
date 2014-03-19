define (require, exports, modules) ->
  _ = require('underscore')
  Backbone = require('backbone')
  templates = require('templates')
  VolumesWidget = require('views/widgets/volumes_widget')
  VolumesTable = require('views/volumes/table')

  class VolumesViews extends Backbone.View
    template: templates['app/scripts/templates/volumes/index.jst']

    className: 'stage'

    initialize: (opts) ->
      @volumes = opts.volumes
      @widgets = []
      @widgets.push new VolumesWidget(volumes: @volumes, click: 'none')
      @widgets.push new VolumesTable(volumes: @volumes)

    render: ->
      widget_html = (widget.render().el for widget in @widgets)
      @$el.html(@template())
      @$el.append(widget_html...)
      this
