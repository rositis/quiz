<?php
/**
 * @file
 * Matching classes.
 *
 * This module uses the question interface to define the matching question type.
 *
 * A Matching node defines a series of questions and answers and requires the
 * userto associate each answer with a question.
 */

/**
 * Extension of QuizQuestionResponse
 */
class MatchingResponse extends QuizQuestionResponse {

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
      $res = db_query('SELECT ua.answer, score, ua.match_id FROM {quiz_matching_user_answers} ua
              JOIN {quiz_matching_node} n ON n.match_id = ua.match_id
              WHERE n.nid = :nid AND n.vid = :vid AND ua.result_answer_id = :result_answer_id', array(':nid' => $question_node->nid, ':vid' => $question_node->vid, ':result_answer_id' => $this->result_answer_id));
      $this->answer = array();
      while ($obj = $res->fetch()) {
        $this->answer[$obj->match_id] = $obj->answer;
      }
    }
    $this->is_correct = $this->isCorrect();
  }

  /**
   * Implementation of save().
   *
   * @see QuizQuestionResponse::save()
   */
  public function save() {
    $this->delete();

    $insert = db_insert('quiz_matching_user_answers')->fields(array('match_id', 'result_answer_id', 'answer', 'score'));
    foreach ($this->answer as $key => $value) {
      $insert->values(array(
        'match_id' => $key,
        'result_answer_id' => $this->result_answer_id,
        'answer' => (int) $value,
        'score' => ($key == $value) ? 1 : 0,
      ));
    }
    $insert->execute();
  }

  /**
   * Implementation of delete().
   *
   * @see QuizQuestionResponse::delete()
   */
  public function delete() {
    db_delete('quiz_matching_user_answers')
      ->condition('result_answer_id', $this->result_answer_id)
      ->execute();
  }

  /**
   * Implementation of score().
   *
   * @see QuizQuestionResponse::score()
   */
  public function score() {
    $wrong_answer = 0;
    $correct_answer = 0;
    $user_answers = isset($this->answer['answer']) ? $this->answer['answer'] : $this->answer;
    $MatchingQuestion = new MatchingQuestion($this->question);
    $correct_answers = $MatchingQuestion->getCorrectAnswer();
    foreach ((array) $user_answers as $key => $value) {
      // TODO: Blank ELSEIF?
      if ($value != 0 && $correct_answers[$key]['answer'] == $correct_answers[$value]['answer']) {
        $correct_answer++;
      }
      elseif ($value == 0 || $value == 'def') {

      }
      else {
        $wrong_answer++;
      }
    }

    $score = $correct_answer;
    if ($this->question->choice_penalty) {
      $score -= $wrong_answer;
    }

    return $score < 0 ? 0 : $score;
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
    $answers = $this->question->answers[0]['answer'];
    $solution = $this->quizQuestion->getSubquestions();
    foreach ($this->question->match as $match) {

      $data[] = array(
        'choice' => $match['question'],
        'attempt' => !empty($answers[$match['match_id']]) ? $solution[1][$answers[$match['match_id']]] : '',
        'correct' => $answers[$match['match_id']] == $match['match_id'] ? theme('quiz_answer_result', array('type' => 'correct')) : theme('quiz_answer_result', array('type' => 'incorrect')),
        'score' => $answers[$match['match_id']] == $match['match_id'] ? 1 : 0,
        'answer_feedback' => $match['feedback'],
        'question_feedback' => 'Question feedback',
        'solution' => $solution[1][$match['match_id']],
        'quiz_feedback' => "Quiz feedback",
      );
    }

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
      $ra = entity_load_single('quiz_result_answer', $result_answer_id);
      $question = node_load($ra->question_nid, $ra->question_vid);
      /** @var $ra_i QuizQuestionResponse */
      $ra_i = _quiz_question_response_get_instance($ra->result_id, $question);

      $matches = array();
      foreach ($question->match as $match) {
        $matches[$match['match_id']] = $match;
      }

      foreach ($ra_i->getResponse() as $question_id => $answer_id) {
        if (!empty($answer_id)) {
          $items[$ra->result_id][] = array('answer' => $matches[$question_id]['question'] . ': ' . $matches[$answer_id]['answer']);
        }
      }
    }

    return $items;
  }
}
