<?php

/**
 * Specialized implementation of hook_page_manager_task_tasks().
 *
 * See api-task.html for more information.
 */
function quiz_quiztake_page_manager_tasks() {
  $config = config('quiz.settings');
  return array(
    // This is a 'page' task and will fall under the page admin UI.
    'task type' => 'page',

    'title' => t('Take @quiz', array('@quiz' => QUIZ_NAME)),
    'admin title' => t('The take page for @quiz.', array('@quiz' => QUIZ_NAME)),
    'admin description' => t('When enabled, this overrides the default view at node/%node/take'),
    'admin path' => 'node/%quiz_menu/take/%question_number',

    // Menu hooks so that we can alter the node/%node menu entry to point to us.
    'hook menu alter' => 'quiz_quiztake_menu_alter',

    // This is task uses 'context' handlers and must implement these to give the
    // handler data it needs.
    'handler type' => 'context',
    'get arguments' => 'quiz_quiztake_get_arguments',
    'get context placeholders' => 'quiz_quiztake_get_contexts',

    // Allow this to be enabled or disabled:
    'disabled' => $config->get('quiz_quiztake_disabled'),
    'enable callback' => 'quiz_quiztake_enable',
  );
}

/**
 * Callback defined by quiz_quiztake_page_manager_tasks().
 *
 * Alter menu item so that node/%node/take comes here.
 */
function quiz_quiztake_menu_alter(&$items, $task) {

  $config = config('quiz.settings');
  if ($config->get('quiz_quiztake_disabled') == TRUE) {
    return;
  }

  $callback = $items['node/%quiz_menu/take/%question_number']['page callback'];
  // Override the node edit handler for our purpose.
  if ($callback == 'quiz_take_question' || ($config->get('page_manager_override_anyway') == FALSE)) {
    $items['node/%quiz_menu/take/%question_number']['page callback'] = 'quiz_quiztake';
    $items['node/%quiz_menu/take/%question_number']['file path'] = $task['path'];
    $items['node/%quiz_menu/take/%question_number']['file'] = $task['file'];
  }
  else {
    //variable_set('quiz_quiztake_disabled', TRUE);
    if (!empty($GLOBALS['page_manager_enabling_quiz'])) {
      backdrop_set_message(t('Page manager module is unable to enable Quiz take Node because some other module already has overridden with %callback.', array('%callback' => $callback)), 'warning');
    }
    return;
  }
}

/**
 * Entry point for our overridden quiz.
 *
 * This function asks its assigned handlers who, if anyone, would like to run
 * with it. If no one does, it passes through to the main node draft page.
 */
function quiz_quiztake($node) {
  // Load my task plugin.
  $task = page_manager_get_task('quiztake');

  // Load the node into a context.
  ctools_include('context');
  ctools_include('context-task-handler');
  $contexts = ctools_context_handler_get_task_contexts($task, '', array($node));

  $output = ctools_context_handler_render($task, '', $contexts, array($node->nid));
  if ($output !== FALSE) {
    return $output;
  }

  module_load_include('inc', 'quiz', 'quiz.node');
  $function = 'quiz_take_page';
  foreach (module_implements('page_manager_override') as $module) {
    $call = $module . '_page_manager_override';
    if (($rc = $call('quiz')) && function_exists($rc)) {
      $function = $rc;
      break;
    }
  }

  // Otherwise, fall back.
  return $function($node);
}

/**
 * Callback to enable/disable the page from the UI.
 */
function quiz_quiztake_enable($cache, $status) {
  $config = config('quiz.settings');
  $config->set('quiz_quiztake_disabled', $status);
  // Set a global flag so that the menu routine knows it needs
  // to set a message if enabling cannot be done.
  if (!$status) {
    $GLOBALS['page_manager_enabling_quiz'] = TRUE;
  }
}

/**
 * Callback to get arguments provided by this task handler.
 *
 * Since this is the node view and there is no UI on the arguments, we
 * create dummy arguments that contain the needed data.
 *
 * Since this is the node view and there is no UI on the arguments, we
 * create dummy arguments that contain the needed data.
 */
function quiz_quiztake_get_arguments($task, $subtask_id) {
  return array(
    array(
      'keyword' => 'node',
      'identifier' => t('Node being taken'),
      'id' => 1,
      'name' => 'entity_id:node',
      'settings' => array(),
    ),
  );
}

/**
 * Callback to get context placeholders provided by this handler.
 *
 * These values are extracted from the Drupal 7 CTools module,
 * specifically the page_manager_node_view_get_arguments() function.
 *
 * @see CTools
 */
function quiz_quiztake_get_contexts($task, $subtask_id) {
  $node_arguments = array(
    array(
      'keyword' => 'node',
      'identifier' => t('Node being viewed'),
      'id' => 1,
      'name' => 'entity_id:node',
      'settings' => array(),
    ),
  );
  return quiz_context_get_placeholders_from_argument($node_arguments);
}

/*
 * FUNCTIONS CONVERTED FROM CTOOLS
 */

/**
 * Retrieve a list of empty contexts for all arguments.
 *
 * @param array $arguments
 *
 * @return array
 *
 * @see ctools_context_get_context_from_arguments()
 */
function quiz_context_get_placeholders_from_argument($arguments) {
  $contexts = array();
  foreach ($arguments as $argument) {
    $context = ctools_context_get_context_from_argument($argument, NULL, TRUE);
    if ($context) {
      $contexts[quiz_context_id($argument, 'argument')] = $context;
    }
  }
  return $contexts;
}

/**
 * Get a context from an argument.
 *
 * @param $argument
 *   The configuration of an argument. It must contain the following data:
 *   - name: The name of the argument plugin being used.
 *   - argument_settings: The configuration based upon the plugin forms.
 *   - identifier: The human readable identifier for this argument, usually
 *     defined by the UI.
 *   - keyword: The keyword used for this argument for substitutions.
 *
 * @param $arg
 *   The actual argument received. This is expected to be a string from a URL
 *   but this does not have to be the only source of arguments.
 * @param $empty
 *   If true, the $arg will not be used to load the context. Instead, an empty
 *   placeholder context will be loaded.
 *
 * @return ctools_context
 *   A context object if one can be loaded.
 */
function ctools_context_get_context_from_argument($argument, $arg, $empty = FALSE) {
  // TODO: Copy this include into Quiz Module
  ctools_include('plugins');
  if (empty($argument['name'])) {
    return NULL;
  }
  // TODO: Copy the ctools_plugin_load_function function.
  $function = ctools_plugin_load_function('ctools', 'arguments', $argument['name'], 'context');
  if ($function) {
    // Backward compatibility: Merge old style settings into new style:
    if (!empty($argument['settings'])) {
      $argument += $argument['settings'];
      unset($argument['settings']);
    }

    $context = $function($arg, $argument, $empty);

    if (is_object($context)) {
      $context->identifier = $argument['identifier'];
      $context->page_title = isset($argument['title']) ? $argument['title'] : '';
      $context->keyword = $argument['keyword'];
      $context->id = quiz_context_id($argument, 'argument');
      $context->original_argument = $arg;

      if (!empty($context->empty)) {
        $context->placeholder = array(
          'type' => 'argument',
          'conf' => $argument,
        );
      }
    }
    return $context;
  }
}

/**
 * Determine a unique context ID for a context.
 *
 * Often contexts of many different types will be placed into a list. This
 * ensures that even though contexts of multiple types may share IDs, they
 * are unique in the final list.
 *
 * This a direct copy of the ctools_context_id() function.
 */
function quiz_context_id($context, $type = 'context') {
  // If not set, FALSE or empty.
  if (!isset($context['id']) || !$context['id']) {
    $context['id'] = 1;
  }

  // @todo is '' the appropriate default value?
  $name = isset($context['name']) ? $context['name'] : '';

  return $type . '_' . $name . '_' . $context['id'];
}

