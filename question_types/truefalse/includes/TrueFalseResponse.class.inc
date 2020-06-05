<?php


class TrueFalseResponse extends QuizQuestionResponse {

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
   *   The answer.
   */
  public function __construct($result_id, stdClass $question_node, $answer = NULL) {
    parent::__construct($result_id, $question_node, $answer);
    if (!isset($answer)) {
      // Get from DB.
      $r = $this->getUserAnswer();
      if (!empty($r)) {
        $this->answer = $r->answer;
        $this->score = $r->score;
      }
    }
    else {
      // Load from input.
      $this->answer = $answer;
    }
  }

  /**
   * Implementation of save().
   *
   * @see QuizQuestionResponse::save()
   */
  public function save() {
    db_merge('quiz_truefalse_user_answers')
      ->key(array('result_answer_id' => $this->result_answer_id))
      ->fields(array(
        'result_answer_id' => $this->result_answer_id,
        'answer' => (int) $this->answer,
        'score' => (int) $this->getScore(),
      ))
      ->execute();
  }

  /**
   * Implementation of delete().
   *
   * @see QuizQuestionResponse::delete()
   */
  public function delete() {
    db_delete('quiz_truefalse_user_answers')
      ->condition('result_answer_id', $this->result_answer_id)
      ->execute();
  }

  /**
   * Implementation of score().
   *
   * @see QuizQuestionResponse::score()
   */
  public function score() {
    $tfQuestion = new TrueFalseQuestion($this->question);
    return ($this->getResponse() == $tfQuestion->getCorrectAnswer()) ? 1 : 0;
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
   * Get the user's answer from the database.
   *
   * @return array|bool|mixed|object|null
   *   An array containing both the answer and the score
   */
  public function getUserAnswer() {
    $answer = [];
    if (isset($this->result_answer_id)) {
      $query = 'SELECT answer, score
        FROM {quiz_truefalse_user_answers}
        WHERE result_answer_id = :raid';
      $answer = db_query($query, array(':raid' => $this->result_answer_id))->fetch();

    }
    return $answer;
  }

  /**
   * Implementation of getFeedbackValues().
   *
   * @see QuizQuestionResponse::getFeedbackValues()
   */
  public function getFeedbackValues() {

    $answer = $this->question->answers[0]['answer'];
    if (!is_null($answer)) {
      $answer = intval($answer);
    }
    $correct_answer = intval($this->question->correct_answer);

    $data = array();
    $data[] = array(
      'choice' => t('True'),
      'attempt' => $answer === 1 ? quiz_icon('selected') : '',
      'correct' => $answer === 1 ? quiz_icon($correct_answer ? 'correct' : 'incorrect') : '',
      'score' => intval($correct_answer === 1 && $answer === 1),
      'answer_feedback' => '',
      'solution' => $correct_answer === 1 ? quiz_icon('should') : '',
    );

    $data[] = array(
      'choice' => t('False'),
      'attempt' => $answer === 0 ? quiz_icon('selected') : '',
      'correct' => $answer === 0 ? (quiz_icon(!$correct_answer ? 'correct' : 'incorrect')) : '',
      'score' => intval($correct_answer === 0 && $answer === 0),
      'answer_feedback' => '',
      'solution' => $correct_answer === 0 ? quiz_icon('should') : '',
    );

    return $data;
  }

  /**
   * Get answers for a question in a result.
   *
   * This static method assists in building views for the mass export of
   * question answers.
   *
   * @see views_handler_field_prerender_list for the expected return value.
   */
  public static function viewsGetAnswers(array $result_answer_ids = array()) {
    $items = array();
    foreach ($result_answer_ids as $result_answer_id) {
      $ra = entity_load('quiz_result_answer', $result_answer_id);
      $question = node_load($ra->question_nid, $ra->question_vid);
      /* @var $ra_i QuizQuestionResponse */
      $ra_i = _quiz_question_response_get_instance($ra->result_id, $question);
      $items[$ra->result_id][] = array('answer' => $ra_i->getResponse() ? t('True') : t('False'));
    }
    return $items;
  }
}
