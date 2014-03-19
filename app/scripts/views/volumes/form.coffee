define (require, exports, modules) ->
  _ = require('underscore')
  Backbone = require('backbone')
  templates = require('templates')
  VolumeSettings = require('views/volumes/settings')

  # jquery.magnific-popup
  require('magnific')

  class VolumesForm extends Backbone.View
    template: templates['app/scripts/templates/volumes/edit.jst']
    className: 'stage content'

    events:
      'click #save_settings': 'saveSettings'
      'click #cancel_popup': 'cancelPopup'

    initialize: ->
      @widgets = []
      @form = new VolumeSettings(model: @model)
      @widgets.push @form

    render: ->
      @$el.html(@template(volume: @model))
      widget_html = (widget.render().el for widget in @widgets)
      @$el.append(widget_html...)
      this

    popup: ->
      rendered = @render().el
      $.magnificPopup.open(
       items:
         src: rendered
         type: 'inline'
      , 0)

    saveSettings: (e) ->
      e.preventDefault()
      @form.updateModel()
      # TODO
        # @model.save({priority: @model.get('priority'), sla: @model.get('sla'), limit: @model.get('limit')}, {patch: true})
      $.magnificPopup.close()
      @remove()

    cancelPopup: (e) ->
      e.preventDefault()
      $.magnificPopup.close()
      @remove()
