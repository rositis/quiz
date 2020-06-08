<?php

/**
 * @file
 * Handles the layout of the choice creation form.
 *
 * Variables available:
 * - $form.
 */
foreach (element_children($form) as $key) {
  if (is_numeric($key)) {
    $row = array(
      'data' => array(
        array(
          'width' => 35,
          'data' => backdrop_render($form[$key]['correct']),
        ),
        backdrop_render($form[$key]['answer'])
        . backdrop_render($form[$key]['advanced'])
        . backdrop_render($form[$key]['remove_button'])
        . theme('status_messages'),
        backdrop_render($form[$key]['weight']),
      ),
      'class' => array('draggable'),
    );
    $rows[] = $row;
  }
}

print theme('table', array(
  'rows' => $rows,
  'header' => array(
    t('Correct'),
    t('Answer'),
    t('Weight'),
  ),
  'attributes' => array(
    'id' => 'multichoice-alternatives-table',
  ),
));
print backdrop_render_children($form);
