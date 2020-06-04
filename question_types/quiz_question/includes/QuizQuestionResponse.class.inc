<?php

/**
 * Each question type must store its own response data and be able to calculate
 * a score for that data.
 */
abstract class QuizQuestionResponse {

  // Result id.
  protected $result_id = 0;
  protected $is_correct = FALSE;
  protected $evaluated = TRUE;
  // The question node(not a quiz question instance).
  public $question = NULL;
  public $quizQuestion = NULL;
  protected $answer = NULL;
  protected $score;
  public $is_skipped;
  public $is_doubtful;

  /**
   * Create a new user response.
   *
   * @param int $result_id
   *   The result ID for the user's result set. There is one result ID per time
   *   the user takes a quiz.
   * @param stdClass $question_node
   *   The question node.
   * @param mixed $answer
   *   The answer (dependent on question type).
   */
  public function __construct($result_id, stdClass $question_node, $answer = NULL) {
    $this->result_id = $result_id;
    $this->question = $question_node;
    $this->quizQuestion = _quiz_question_get_instance($question_node);
    $this->answer = $answer;
    $result = db_query('SELECT *
      FROM {quiz_node_results_answers}
      WHERE result_id = :result_id
      AND question_nid = :question_nid
      AND question_vid = :question_vid', array(
      ':result_id' => $result_id,
      ':question_nid' => $question_node->nid,
      ':question_vid' => $question_node->vid
    ))->fetch();
    if (is_object($result)) {
      foreach ($result as $key => $value) {
        $this->$key = $value;
      }
    }
  }

  /**
   * Get the question of this question response.
   *
   * @return QuizQuestion
   */
  public function getQuizQuestion() {
    return $this->quizQuestion;
  }

  /**
   * Used to refresh this instances question node in case drupal has changed it.
   *
   * @param stdClass $newNode
   *   Question node.
   */
  public function refreshQuestionNode($newNode) {
    $this->question = $newNode;
  }

  /**
   * Indicate whether the response has been evaluated (scored) yet.
   *
   * Questions that require human scoring (e.g. essays) may need to manually
   * toggle this.
   *
   * @return bool
   */
  public function isEvaluated() {
    return (bool) $this->evaluated;
  }

  /**
   * Check to see if the answer is marked as correct.
   *
   * This default version returns TRUE if the score is equal to the maximum
   * possible score. Each question type can determine on its own if the question
   * response is "correct".
   *
   * @return bool
   */
  public function isCorrect() {
    return ($this->getMaxScore(FALSE) == $this->getScore(FALSE));
  }

  /**
   * Get the score of this question response.
   *
   * @param bool $weight_adjusted
   *   If the score should be scaled based on the Quiz question max score.
   *
   * @return int
   *   The max score of this question response.
   */
  public function getScore($weight_adjusted = TRUE) {
    if ($weight_adjusted) {
      $ratio = $this->getWeightedRatio();
      return $this->score() * $ratio;
    }
    return $this->score();
  }

  /**
   * Get the max score of this question response.
   *
   * @param bool $weight_adjusted
   *   If the max score should be scaled based on the Quiz question max score.
   *
   * @return int
   *   The max score of this question response.
   */
  public function getMaxScore($weight_adjusted = TRUE) {
    if ($weight_adjusted) {
      $quiz_result = quiz_result_load($this->result_id);
      $relationships = entity_load('quiz_question_relationship', FALSE, array(
        'parent_nid' => $quiz_result->nid,
        'parent_vid' => $quiz_result->vid,
        'child_nid' => $this->question->nid,
        'child_vid' => $this->question->vid,
      ));
      if ($relationships) {
        // This is the weighted max score of the question.
        return reset($relationships)->max_score;
      }

      $quiz = node_load($quiz_result->nid);
      if ($quiz->randomization == 2) {
        // This isn't necessary, since setting a max score for random questions
        // updates them all.
      }
      if ($quiz->randomization == 3) {
        $max_score = db_select('quiz_terms', 'qt')
          ->fields('qt', array('max_score'))
          ->condition('tid', $this->tid)
          ->condition('nid', $quiz_result->nid)
          ->condition('vid', $quiz_result->vid)
          ->execute()
          ->fetchField();
        return $max_score;
      }
    }

    return $this->quizQuestion->getMaximumScore();
  }

  /**
   * Represent the response as a stdClass object.
   *
   * Convert data to an object that has the following properties:
   * - $score.
   * - $nid.
   * - $vid.
   * - $result_id.
   * - $is_correct.
   * - $is_evaluated.
   * - $is_skipped.
   * - $is_doubtful.
   * - $tid.
   *
   * @return stdClass
   */
  public function toBareObject() {
    $obj = new stdClass();
    $obj->score = $this->getScore(); // This can be 0 for unscored.
    $obj->nid = $this->question->nid;
    $obj->vid = $this->question->vid;
    $obj->result_id = $this->result_id;
    $obj->is_correct = (int) $this->isCorrect();
    $obj->is_evaluated = $this->isEvaluated();
    $obj->is_skipped = 0;
    $obj->is_doubtful = isset($_POST['is_doubtful']) ? $_POST['is_doubtful'] : 0;
    $obj->tid = $this->tid;
    return $obj;
  }

  /**
   * Get data suitable for reporting a user's score on the question.
   *
   * @return array
   */
  public function getReport() {
    // Basically, we encode internal information in a
    // legacy array format for Quiz.
    $report = array(
      'answer_id' => 0, // <-- Stupid vestige of multichoice.
      'answer' => $this->answer,
      'is_evaluated' => $this->isEvaluated(),
      'is_correct' => $this->isCorrect(),
      'score' => $this->getScore(),
      'question_vid' => $this->question->vid,
      'question_nid' => $this->question->nid,
      'result_id' => $this->result_id,
    );
    return $report;
  }

  /**
   * Creates the report form for the admin pages, and for when a user gets
   * feedback after answering questions.
   *
   * The report is a form to allow editing scores and the likes while viewing
   * the report form.
   *
   * @return array|null
   *   An renderable FAPI array
   */
  public function getReportForm() {
    // Add general data, and data from the question type implementation.
    $form = array();

    $form['nid'] = array(
      '#type' => 'value',
      '#value' => $this->question->nid,
    );
    $form['vid'] = array(
      '#type' => 'value',
      '#value' => $this->question->vid,
    );
    $form['result_id'] = array(
      '#type' => 'value',
      '#value' => $this->result_id,
    );
    $form['display_number'] = array(
      '#type' => 'value',
      '#value' => $this->display_number,
    );

    $quiz_result = quiz_result_load($this->result_id);

    if (quiz_access_to_score($quiz_result)) {

      if ($submit = $this->getReportFormSubmit()) {
        $form['score'] = $this->getReportFormScore();
        $form['answer_feedback'] = $this->getReportFormAnswerFeedback();
        $form['submit'] = array(
          '#type' => 'value',
          '#value' => $submit,
        );
      }

      return $form;
    }
  }

  /**
   * Get the response part of the report form.
   *
   * @return array
   *   Array of response data, with each item being an answer to a response. For
   *   an example, see MultichoiceResponse::getFeedbackValues(). The sub items
   *   are keyed by the feedback type. Providing a NULL option means that
   *   feedback will not be shown. See an example at
   *   LongAnswerResponse::getFeedbackValues().
   */
  public function getFeedbackValues() {
    $data = array();

    $data[] = array(
      'choice' => 'True',
      'attempt' => 'Did the user choose this?',
      'correct' => 'Was their answer correct?',
      'score' => 'Points earned for this answer',
      'answer_feedback' => 'Feedback specific to the answer',
      'question_feedback' => 'General question feedback for any answer',
      'solution' => 'Is this choice the correct solution?',
      'quiz_feedback' => 'Quiz feedback at this time',
    );

    return $data;
  }

  /**
   * Get the feedback form for the reportForm.
   *
   * @return array|false
   *   An renderable FAPI array, or FALSE if no answer form.
   */
  public function getReportFormAnswerFeedback() {
    return FALSE;
  }

  /**
   * Get the submit function for the reportForm.
   *
   * @return string|false
   *   Submit function as a string, or FALSE if no submit function.
   */
  public function getReportFormSubmit() {
    return FALSE;
  }

  /**
   * Get the validate function for the reportForm.
   *
   * @param array $element
   *   The form element of this question.
   * @param array $form_state
   *   The FAPI form_state array.
   */
  public function getReportFormValidate(&$element, &$form_state) {
    // Check to make sure that entered score is not higher than max allowed
    // score.
    if ($element['score']['#value'] && $element['score']['#value'] > $this->getMaxScore(TRUE)) {
      form_error($element['score'], t('The score needs to be a number between 0 and @max', array('@max' => $this->getMaxScore(TRUE))));
    }
  }

  /**
   * Utility function that returns the format of the node body.
   *
   * @return string|null
   *   The format of the node body
   */
  protected function getFormat() {
    $body = field_get_items('node', $this->question, 'body');
    return ($body ? $body[0]['format'] : NULL);
  }

  /**
   * Save the response for this question response.
   */
  abstract public function save();

  /**
   * Delete the response for this question response.
   */
  abstract public function delete();

  /**
   * Calculate the unscaled score in points for this question response.
   */
  abstract public function score();

  /**
   * Get the user's response.
   *
   * @return mixed
   *   The answer given by the user
   */
  abstract public function getResponse();

  /**
   * Can the quiz taker view the requested review?
   *
   * @param string $option
   *   An option key.
   *
   * @return bool
   */
  public function canReview($option) {
    $can_review = &drupal_static(__METHOD__, array());

    if (!isset($can_review[$option])) {
      $quiz_result = quiz_result_load($this->result_id);
      $can_review[$option] = quiz_feedback_can_review($option, $quiz_result);
    }

    return $can_review[$option];
  }

  /**
   * Implementation of getReportFormScore().
   *
   * @see QuizQuestionResponse::getReportFormScore()
   */
  public function getReportFormScore() {
    $score = ($this->isEvaluated()) ? $this->getScore() : '';
    return array(
      '#title' => 'Enter score',
      '#type' => 'textfield',
      '#default_value' => $score,
      '#size' => 3,
      '#attributes' => array('class' => array('quiz-report-score')),
      '#element_validate' => array('element_validate_integer'),
      '#required' => TRUE,
      '#field_suffix' => '/ ' . $this->getMaxScore(),
    );
  }

  /**
   * Set the target result answer ID for this Question response.
   *
   * Useful for cloning entire result sets.
   *
   * @param int $result_answer_id
   */
  public function setResultAnswerId($result_answer_id) {
    $this->result_answer_id = $result_answer_id;
  }

  /**
   * Get answers for a question in a result.
   *
   * This static method assists in building views for the mass export of
   * question answers.
   *
   * It is not as easy as instantiating all the question responses and returning
   * the answer. To do this in views scalably we have to gather the data
   * carefully.
   *
   * This base method provides a very poor way of gathering the data.
   *
   * @see views_handler_field_prerender_list for the expected return value.
   *
   * @see MultichoiceResponse::viewsGetAnswers() for a correct approach
   * @see TrueFalseResponse::viewsGetAnswers() for a correct approach
   */
  public static function viewsGetAnswers(array $result_answer_ids = array()) {
    $items = array();
    foreach ($result_answer_ids as $result_answer_id) {
      $ra = entity_load('quiz_result_answer', $result_answer_id);
      $question = node_load($ra->question_nid, $ra->question_vid);
      /* @var $ra_i QuizQuestionResponse */
      $ra_i = _quiz_question_response_get_instance($ra->result_id, $question);
      $items[$ra->result_id][] = array('answer' => $ra_i->getResponse());
    }
    return $items;
  }

  /**
   * Get the weighted score ratio.
   *
   * This returns the ratio of the weighted score of this question versus the
   * question score. For example, if the question is worth 10 points in the
   * associated quiz, but it is a 3 point multichoice question, the weighted
   * ratio is 3.33.
   *
   * @return float
   *   The weight of the question
   */
  public function getWeightedRatio() {
    if ($this->getMaxScore(FALSE) == 0) {
      return 0;
    }

    return $this->getMaxScore(TRUE) / $this->getMaxScore(FALSE);
  }

}

