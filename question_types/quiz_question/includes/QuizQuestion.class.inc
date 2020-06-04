<?php

/**
 * A base implementation of a quiz_question.
 *
 * This class is adding a layer of abstraction between the node API, quiz API
 * and the question types.
 *
 * It is required that Question types extend this abstract class.
 *
 * This class has default behaviour that all question types must have. It also
 * handles the node API, but gives the question types opportunity to save,
 * delete and provide data specific to the question types.
 *
 * This abstract class also declares several abstract functions forcing
 * question-types to implement required methods.
 */
abstract class QuizQuestion {
  /*
   * QUESTION IMPLEMENTATION FUNCTIONS
   *
   * This part acts as a contract(/interface) between the question-types and the
   * rest of the system.
   *
   * Question types are made by extending these generic methods and abstract
   * methods.
   */
  public $question;

  /**
   * The current node for this question.
   */
  public $node = NULL;
  // Extra node properties.
  public $nodeProperties = NULL;

  /**
   * QuizQuestion constructor stores the node object.
   *
   * @param $node
   *   The node object
   */
  public function __construct(stdClass $node) {
    $this->node = $node;
  }

  /**
   * Allow question types to override the body field title.
   *
   * @return string
   *   The title for the body field.
   */
  public function getBodyFieldTitle() {
    return t('Question');
  }

  /**
   * Returns a node form to quiz_question_form.
   *
   * Adds default form elements, and fetches question type specific elements
   * from their implementation of getCreationForm.
   *
   * @param array $form_state
   *
   * @return array
   *   An renderable FAPI array.
   */
  public function getNodeForm(array &$form_state = NULL) {
    $form = array();

    // Mark this form to be processed by quiz_form_alter. quiz_form_alter will
    // among other things hide the revision fieldset if the user don't have
    // permission to control the revisioning manually.
    $form['#quiz_check_revision_access'] = TRUE;

    // Allow user to set title?
    if (user_access('edit question titles')) {
      $form['helper']['#theme'] = 'quiz_question_creation_form';
      $form['title'] = array(
        '#type' => 'textfield',
        '#title' => t('Title'),
        '#maxlength' => 255,
        '#default_value' => $this->node->title,
        '#required' => TRUE,
        '#description' => t('Add a title that will help distinguish this question from other questions. This will not be seen during the @quiz.', array('@quiz' => QUIZ_NAME)),
      );
    }
    else {
      $form['title'] = array(
        '#type' => 'value',
        '#value' => $this->node->title,
      );
    }

    // Store quiz id in the form.
    $form['quiz_nid'] = array(
      '#type' => 'hidden',
    );
    $form['quiz_vid'] = array(
      '#type' => 'hidden',
    );

    if (isset($_GET['quiz_nid']) && isset($_GET['quiz_vid'])) {
      $form['quiz_nid']['#value'] = intval($_GET['quiz_nid']);
      $form['quiz_vid']['#value'] = intval($_GET['quiz_vid']);
    }

    // Identify this node as a quiz question type so that it can be recognized
    // by other modules effectively.
    $form['is_quiz_question'] = array(
      '#type' => 'value',
      '#value' => TRUE,
    );

    if (!empty($this->node->nid)) {
      if ($properties = entity_load('quiz_question', FALSE, array('nid' => $this->node->nid, 'vid' => $this->node->vid))) {
        $quiz_question = reset($properties);
      }
    }
    $form['feedback'] = array(
      '#type' => 'text_format',
      '#title' => t('Question feedback'),
      '#default_value' => !empty($quiz_question->feedback) ? $quiz_question->feedback : '',
      '#format' => !empty($quiz_question->feedback_format) ? $quiz_question->feedback_format : filter_default_format(),
      '#description' => t('This feedback will show when configured and the user answers a question, regardless of correctness.'),
    );

    // Add question type specific content.
    $form = array_merge($form, $this->getCreationForm($form_state));

    if (config_get('quiz.settings','quiz_auto_revisioning') && $this->hasBeenAnswered()) {
      $log = t('The current revision has been answered. We create a new revision so that the reports from the existing answers stays correct.');
      $this->node->revision = 1;
      $this->node->log = $log;
    }

    return $form;
  }

  /**
   * Retrieve information relevant for viewing the node.
   *
   * (This data is generally added to the node's extra field.)
   *
   * @return array
   *   Content array.
   */
  public function getNodeView() {
    $content = array();
    return $content;
  }

  /**
   * Getter function returning properties to be loaded when the node is loaded.
   *
   * @see load hook in quiz_question.module (quiz_question_load)
   *
   * @return array
   *   Array with all additional node properties.
   */
  public function getNodeProperties() {
    if (isset($this->nodeProperties)) {
      return $this->nodeProperties;
    }

    $props = array();
    // TODO: Speed the query by not using *!
    $result = db_query('SELECT *
      FROM {quiz_question_properties}
      WHERE nid = :nid AND vid = :vid', array(':nid' => $this->node->nid, ':vid' => $this->node->vid));
    $row = $result->fetch();

    if ($row) {
      $props['max_score'] = $row->max_score;
      $props['feedback']['value'] = $row->feedback;
      $props['feedback']['format'] = $row->feedback_format;
    }

    $props['is_quiz_question'] = TRUE;

    $this->nodeProperties = $props;
    return $props;
  }

  /**
   * Responsible for handling insert/update of question-specific data.
   *
   * This is typically called from within the Node API, so there is no need
   * to save the node.
   *
   * The $is_new flag is set to TRUE whenever the node is being initially
   * created.
   *
   * A save function is required to handle the following three situations:
   * - A new node is created ($is_new is TRUE).
   * - A new node *revision* is created ($is_new is NOT set, because the
   *   node itself is not new).
   * - An existing node revision is modified.
   *
   * @see hook_update and hook_insert in quiz_question.module
   *
   * @param bool $is_new
   *   TRUE when the node is initially created.
   *
   * @throws \InvalidMergeQueryException
   */
  public function save($is_new = FALSE) {
    // We call the abstract function saveNodeProperties to save type specific
    // data.
    // TODO: Check this.
    $this->saveNodeProperties($this->node->is_new);

    db_merge('quiz_question_properties')
      ->key(array(
        'nid' => $this->node->nid,
        'vid' => $this->node->vid,
      ))
      ->fields(array(
        'nid' => $this->node->nid,
        'vid' => $this->node->vid,
        'max_score' => $this->getMaximumScore(),
        'feedback' => !empty($this->node->feedback['value']) ? $this->node->feedback['value'] : '',
        'feedback_format' => !empty($this->node->feedback['format']) ? $this->node->feedback['format'] : filter_default_format(),
      ))
      ->execute();

    // Save what quizzes this question belongs to.
    // @kludge the quiz nid/vid are still on the node
    if (!empty($this->node->quiz_nid)) {
      $this->saveRelationships($this->node->quiz_nid, $this->node->quiz_vid);
    }

    if (!empty($this->node->revision)) {
      // @kludge strange way of redirecting, since we do not have access to
      // $form(_state) here
      unset($_GET['destination']);
      unset($_REQUEST['edit']['destination']);
      $_REQUEST['edit']['destination'] = "node/{$this->node->nid}/question-revision-actions";
    }
  }

  /**
   * Delete question data from the database.
   *
   * Called by quiz_question_delete (hook_delete).
   *
   * Child classes must call super.
   *
   * @param bool $only_this_version
   *   If the $only_this_version flag is TRUE, then only the particular
   *   nid/vid combo should be deleted. Otherwise, all questions with the
   *   current nid can be deleted.
   */
  public function delete($only_this_version = FALSE) {
    // Delete properties.
    $delete = db_delete('quiz_question_properties')
      ->condition('nid', $this->node->nid);
    if ($only_this_version) {
      $delete->condition('vid', $this->node->vid);
    }
    $delete->execute();
  }

  /**
   * Provides validation for question before it is created.
   *
   * When a new question is created and initially submitted, this is
   * called to validate that the settings are acceptable.
   *
   * @param array $form
   *   The processed form.
   */
  abstract public function validateNode(array &$form);

  /**
   * Get the form through which the user will answer the question.
   *
   * @param int $result_id
   *   The result id.
   * @param array|null $form_state
   *   The FAPI form_state array.
   *
   * @return array
   *   An renderable FAPI array.
   * TODO: Go through all versions of this and make sure the result id is the first param.
   */
  public function getAnsweringForm($result_id, array $form_state = NULL) {
    $form = array();
    $form['#element_validate'] = array('quiz_question_element_validate');
    return $form;
  }

  /**
   * Get the form used to create a new question.
   *
   * @param array $form_state
   *   The FAPI form_state array.
   * @return array
   *   An renderable FAPI array.
   */
  abstract public function getCreationForm(array &$form_state = NULL);

  /**
   * Get the maximum possible score for this question.
   *
   * @return int
   */
  abstract public function getMaximumScore();

  /**
   * Save question type specific node properties.
   */
  abstract public function saveNodeProperties();

  /**
   * Save this Question to the specified Quiz.
   *
   * @param $nid
   *   The node ID.
   * @param $vid
   *   The revision ID.
   */
  public function saveRelationships($nid, $vid) {
    $quiz_node = node_load($nid, $vid);

    if (config_get('quiz.settings','quiz_auto_revisioning') && quiz_has_been_answered($quiz_node)) {
      // We need to revise the quiz node if it has been answered.
      $quiz_node->revision = 1;
      $quiz_node->log = t('The current revision has been answered. We create a new revision so that the reports from the existing answers stays correct.');
      $quiz_node->save();
      backdrop_set_message(t('New revision has been created for the @quiz %n', array('%n' => $quiz_node->title, '@quiz' => QUIZ_NAME)));
    }

    $auto_update_max_score = $this->autoUpdateMaxScore() ? 1 : 0;
    $weight_query = 'SELECT MAX(weight)
        FROM {quiz_node_relationship}
        WHERE parent_vid = :vid';
    $weight = db_query($weight_query, array(':vid' => $quiz_node->vid))
        ->fetchField() + 1;
    $randomization_query = 'SELECT randomization
        FROM {quiz_node_properties}
        WHERE nid = :nid AND vid = :vid';
    $randomization = db_query($randomization_query, array(':nid' => $quiz_node->nid, ':vid' => $quiz_node->vid))
      ->fetchField();

    $insert_values = array(
      'parent_nid' => $quiz_node->nid,
      'parent_vid' => $quiz_node->vid,
      'child_nid' => $this->node->nid,
      'child_vid' => $this->node->vid,
      'max_score' => $this->getMaximumScore(),
      'auto_update_max_score' => $auto_update_max_score,
      'weight' => $weight,
      'question_status' => $randomization == 2 ? QUIZ_QUESTION_RANDOM : QUIZ_QUESTION_ALWAYS,
    );

    $quiz_question_relationship = entity_create('quiz_question_relationship', $insert_values);
    try {
      $quiz_question_relationship->save();
    }
    catch (Exception $e) {
      watchdog('Quiz Questiob', $e->getMessage());
    }

    // Update max_score for relationships if auto update max score is enabled
    // for question.
    $quizzes_to_update = array();
    $result = db_query(
      'SELECT parent_vid as vid from {quiz_node_relationship} where child_nid = :nid and child_vid = :vid and auto_update_max_score=1', array(':nid' => $this->node->nid, ':vid' => $this->node->vid));
    foreach ($result as $record) {
      $quizzes_to_update[] = $record->vid;
    }

    db_update('quiz_node_relationship')
      ->fields(array('max_score' => $this->getMaximumScore()))
      ->condition('child_nid', $this->node->nid)
      ->condition('child_vid', $this->node->vid)
      ->condition('auto_update_max_score', 1)
      ->execute();

    quiz_update_max_score_properties($quizzes_to_update);
    quiz_update_max_score_properties(array($quiz_node->vid));
  }

  /**
   * Finds out if a question has been answered or not.
   *
   * This function also returns TRUE if a quiz that this question belongs to
   * have been answered. Even if the question itself haven't been answered.
   * This is because the question might have been rendered and a user is about
   * to answer it...
   *
   * @return bool
   *   TRUE if question has been answered or is about to be answered...
   */
  public function hasBeenAnswered() {
    if (!isset($this->node->vid)) {
      return FALSE;
    }
    $answered = db_query_range('SELECT 1 FROM {quiz_node_results} qnres
            JOIN {quiz_node_relationship} qnrel ON (qnres.vid = qnrel.parent_vid)
            WHERE qnrel.child_vid = :child_vid', 0, 1, array(':child_vid' => $this->node->vid))->fetch();
    return $answered ? TRUE : FALSE;
  }

  /**
   * Determines if the user can view the correct answers.
   *
   * @return bool
   *   TRUE if the view may include the correct answers to the question.
   */
  public function viewCanRevealCorrect() {
    global $user;
    $can_reveal = FALSE;
    $reveal_correct[] = user_access('view any quiz question correct response');
    $reveal_correct[] = ($user->uid == $this->node->uid);
    if (array_filter($reveal_correct)) {
      $can_reveal = TRUE;
    }
    return $can_reveal;
  }

  /**
   * Utility function that returns the format of the node body.
   *
   * @return string|null
   *   The format of the node body
   */
  protected function getFormat() {
    $node = isset($this->node) ? $this->node : $this->question;
    $body = field_get_items('node', $node, 'body');
    return isset($body[0]['format']) ? $body[0]['format'] : NULL;
  }

  /**
   * This may be overridden in subclasses. If it returns true,
   * it means the max_score is updated for all occurrences of
   * this question in quizzes.
   *
   * @return bool
   */
  protected function autoUpdateMaxScore() {
    return FALSE;
  }

  /**
   * Validate a user's answer.
   *
   * @param array $element
   *   The form element of this question.
   * @param mixed $value
   *   The value submitted by the user. Depending on the question type's form,
   *   the data type will vary.
   */
  public function getAnsweringFormValidate(array &$element, &$value) {

  }

  /**
   * Is this question graded?
   *
   * Questions like Quiz Directions, Quiz Page, and Scale are not.
   *
   * By default, questions are expected to be gradeable
   *
   * @return bool
   */
  public function isGraded() {
    return TRUE;
  }

  /**
   * Does this question type give feedback?
   *
   * Questions like Quiz Directions and Quiz Pages do not.
   *
   * By default, questions give feedback
   *
   * @return bool
   */
  public function hasFeedback() {
    return TRUE;
  }

  /**
   * Is this "question" an actual question?
   *
   * For example, a Quiz Page is not a question, neither is a "quiz directions".
   *
   * Returning FALSE here means that the question will not be numbered, and
   * possibly other things.
   *
   * @return bool
   */
  public function isQuestion() {
    return TRUE;
  }

}
