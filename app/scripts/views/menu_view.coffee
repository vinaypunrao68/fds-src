define (require, exports, modules) ->
  _ = require('underscore')
  Backbone = require('backbone')
  templates = require('templates')

  class MenuView extends Backbone.View
    template: templates['app/scripts/templates/shared/menu.jst']
    initialize: ->
      $('a.menu-trigger').on 'click', (e) ->
        e.preventDefault()
        $('body').toggleClass('menu-active')

    render: ->
      @$el.html(@template())
      this
