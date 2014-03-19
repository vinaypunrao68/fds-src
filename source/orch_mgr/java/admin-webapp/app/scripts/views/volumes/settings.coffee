define (require, exports, modules) ->
  Backbone = require('backbone')
  templates = require('templates')

  class VolumeSettings extends Backbone.View
    template: templates['app/scripts/templates/volumes/form.jst']

    events:
      'click .tapper li': 'changeValue'

    initialize: ->
      # no-op

    render: ->
      @$el.html(@template(volume: @model))
      this

    changeValue: (e) ->
      $li = $(e.target)
      $li.addClass('current').siblings().removeClass('current')

    updateModel: ->
      attr = {}
      @$el.find('.tapper').each (idx, div) =>
        $div = $(div)

        field = $div.data('field')
        val = $div.find('li.current').data('val')
        attr[field] = val

      # Setting all at once to use .changedAttributes
      @model.set(attr)
