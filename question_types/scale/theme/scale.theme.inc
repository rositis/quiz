<?php

/**
 * @file
 * Theme functions for the scale question type.
 */

/**
 * Theme function for the answer part of the node view.
 *
 * @param array $variables
 *
 * @return string
 *   An HTML string.
 */
function theme_scale_answer_node_view($variables) {
  $alternatives = $variables['alternatives'];
  $header = array(t('Alternatives'));
  $rows = array();
  foreach ($alternatives as $alternative) {
    $rows[] = array($alternative);
  }
  return theme('table', array('header' => $header, 'rows' => $rows));
}

/**
 * Theme the scale response form.
 *
 * @param array $variables
 *
 * @return string
 *   An HTML string.
 */
function theme_scale_response_form($variables) {
  $form = $variables['form'];
  return backdrop_render_children($form);
}

/**
 * Theme function for the scale creation form.
 *
 * This function only adds js to the page. Js should be added to forms through
 * theme functions like this one. Js can't be added to the page from the form
 * function because sometimes the form is returned from cache.
 *
 * @param $form
 *   The scale creation form.
 */
function theme_scale_creation_form($form) {
  backdrop_add_js(backdrop_get_path('module', 'scale') . '/js/scale.js');
}
