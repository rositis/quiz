<?php

/**
 * @file
 * Theme functions for the short_answer question type.
 */

/**
 * Theme the short_answer answer.
 *
 * @param array $variables
 *
 * @return string
 *   An HTML string.
 */
function theme_short_answer_user_answer($variables) {
  $answer = $variables['answer'];
  $correct = $variables['correct'];
  $header = array(t('Correct Answer'), t('User Answer'));
  $row = array(array($correct, $answer));
  $output = '';
  try {
    $output = theme('table', array('header' => $header, 'rows' => $row));
  }
  catch (Exception $e) {
    watchdog('Short Answer', $e->getMessage());
  }
  return $output;
}

/**
 * Theme the short_answer response form.
 *
 * @param array $variables
 *
 * @return string
 *   An HTML string.
 */
function theme_short_answer_response_form($variables) {
  $form = $variables['form'];
  return backdrop_render_children($form);
}
