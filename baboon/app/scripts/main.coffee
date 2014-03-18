#/*global require*/
'use strict'

define (require, exports, modules) ->
  Backbone = require('backbone')
  Utils = require('utils')
  Router = require('router')
  FastClick = require('fastclick')

  $ ->
    new Router
    Backbone.history.start(pushState: true)

    # Use links for pushState
    $(document.body).on "click", "a", (event) ->
      event.preventDefault()
      return if $(this).hasClass('action')
      href = { prop: $(this).prop("href"), attr: $(this).attr("href") }
      root = location.protocol + "//" + location.host + Backbone.history.options.root
      if href.prop and href.prop.slice(0, root.length) is root
        event.preventDefault()
        Backbone.history.navigate(href.attr, true)

    $('#content').on('click', 'header a', (e) ->
      $(e.target).closest('header').addClass('clicked')
    )
    FastClick.attach(document.body)
    $(document).ajaxSuccess(Utils.setConnected)
               .ajaxError(Utils.setDisconnected)
    $.ajaxSetup(
      beforeSend: Utils.addHeaders
    )
