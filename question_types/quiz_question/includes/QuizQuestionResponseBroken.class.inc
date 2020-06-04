<?php


class QuizQuestionResponseBroken extends QuizQuestionResponse {

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
    parent::__construct($result_id, $question_node, $answer = NULL);
    $this->quizQuestion = _quiz_question_get_instance($question_node);
  }

  /**
   * @see QuizQuestionResponse::getReport()
   */
  public function getReport() {
    return array(
      'answer_id' => 0,
      'is_correct' => NULL,
    );
  }

  /**
   * @see QuizQuestionResponse::save()
   */
  public function save() {}

  /**
   * @see QuizQuestionResponse::delete()
   */
  public function delete() {}

  /**
   * @see QuizQuestionResponse::score()
   */
  public function score() {}

  /**
   * @see QuizQuestionResponse::getResponse()
   */
  public function getResponse() {}
}
