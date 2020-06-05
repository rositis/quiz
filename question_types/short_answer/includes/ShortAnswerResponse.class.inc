<?php
/**
 * @file
 * Short answer classes.
 *
 * This module uses the question interface to define the short_answer question type.
 *
 * If you are developing your own question type, the easiest place to start is
 * with multichoice.classes.inc. short_answer and long_answer are good for
 * understanding question types that involve textual answers.
 *
 * These classes could have extended the long answer question, but this would
 * mean having to add the long answer module as a dependency. So in order to
 * keep them separate we've chosen to "duplicate" some code.
 */

/**
 * Extension of QuizQuestionResponse.
 */
class ShortAnswerResponse extends QuizQuestionResponse {

  /**
   * ID of the answer.
   */
  protected $answer_id = 0;
  private $answer_feedback;
  private $answer_feedback_format;
  public $result_answer_id;

  /**
   * Constructor.
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
    parent::__construct($result_id, $question_node, $answer);
    if (!isset($answer)) {
      $r = db_query('SELECT *
        FROM {quiz_short_answer_user_answers}
        WHERE result_answer_id = :raid', array(':raid' => $this->result_answer_id)
      )->fetch();

      if (!empty($r)) {
        $this->answer = $r->answer;
        $this->score = $r->score;
        $this->evaluated = $r->is_evaluated;
        $this->answer_id = $r->answer_id;
        $this->answer_feedback = $r->answer_feedback;
        $this->answer_feedback_format = $r->answer_feedback_format;
      }
    }
    else {
      if (is_array($answer)) {
        $this->answer = $answer['answer'];
      }
      else {
        $this->answer = $answer;
        $this->evaluated = $this->question->correct_answer_evaluation != ShortAnswerQuestion::ANSWER_MANUAL;
      }
    }
  }
  /**
   * Implementation of save().
   *
   * @see QuizQuestionResponse::save()
   *
   * @throws \InvalidMergeQueryException
   */
  public function save() {
    // We need to set is_evaluated depending on whether the type requires
    // evaluation.
    $this->is_evaluated = (int) ($this->question->correct_answer_evaluation != ShortAnswerQuestion::ANSWER_MANUAL);
    db_merge('quiz_short_answer_user_answers')
      ->key(array('result_answer_id' => $this->result_answer_id))
      ->fields(array(
        'answer' => $this->answer,
        'result_answer_id' => $this->result_answer_id,
        'score' => $this->getScore(FALSE),
        'is_evaluated' => $this->is_evaluated,
      ))
      ->execute();
  }

  /**
   * Implementation of delete().
   *
   * @see QuizQuestionResponse::delete()
   */
  public function delete() {
    db_delete('quiz_short_answer_user_answers')
      ->condition('result_answer_id', $this->result_answer_id)
      ->execute();
  }

  /**
   * Implementation of score().
   *
   * @see QuizQuestionResponse::score()
   */
  public function score() {
    // Manual scoring means we go with what is in the DB.
    if ($this->question->correct_answer_evaluation == ShortAnswerQuestion::ANSWER_MANUAL) {
      $score = db_query('SELECT score FROM {quiz_short_answer_user_answers} WHERE result_answer_id = :raid', array(':raid' => $this->result_answer_id))->fetchField();
      if (!$score) {
        $score = 0;
      }
    }
    // Otherwise, we run the scoring automatically.
    else {
      $shortAnswer = new ShortAnswerQuestion($this->question);
      $score = $shortAnswer->evaluateAnswer($this->getResponse());
    }
    return $score;
  }

  /**
   * Implementation of getResponse().
   *
   * @see QuizQuestionResponse::getResponse()
   */
  public function getResponse() {
    return $this->answer;
  }

  /**
   * Implementation of getFeedbackValues().
   *
   * @see QuizQuestionResponse::getFeedbackValues()
   */
  public function getFeedbackValues() {
    $data = array();
    $score = $this->score();
    $max = $this->getMaxScore(FALSE);
    // Default icon is 'unknown'.
    $icon = quiz_icon('unknown');

    if ($this->evaluated) {
      // Question has been graded.
      if ($score == 0) {
        $icon = quiz_icon('incorrect');
      }
      if ($score > 0) {
        $icon = quiz_icon('almost');
      }
      if ($score == $max) {
        $icon = quiz_icon('correct');
      }
    }

    $data[] = array(
      // Hide this column. Does not make sense for short answer as there are no
      // choices.
      'choice' => NULL,
      'attempt' => $this->answer,
      'correct' => $icon,
      'score' => !$this->evaluated ? t('This answer has not yet been scored.') : $this->getScore(),
      'answer_feedback' => check_markup($this->answer_feedback, $this->answer_feedback_format),
      'solution' => $this->question->correct_answer,
    );

    return $data;
  }

  /**
   * Implementation of getReportFormAnswerFeedback().
   *
   * @see QuizQuestionResponse::getReportFormAnswerFeedback()
   */
  public function getReportFormAnswerFeedback() {
    return array(
      '#title' => t('Enter feedback'),
      '#type' => 'text_format',
      '#default_value' => $this->answer_feedback,
      '#format' => isset($this->answer_feedback_format) ? $this->answer_feedback_format : filter_default_format(),
      '#attributes' => array('class' => array('quiz-report-score')),
    );
  }

  /**
   * Implementation of getReportFormSubmit().
   *
   * @see QuizQuestionResponse::getReportFormSubmit()
   */
  public function getReportFormSubmit() {
    return 'short_answer_report_submit';
  }
}
