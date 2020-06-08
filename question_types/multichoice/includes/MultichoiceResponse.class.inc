<?php

/**
 * Extension of QuizQuestionResponse.
 */
class MultichoiceResponse extends QuizQuestionResponse {

  /**
   * ID of the answers.
   */
  protected $user_answer_ids;
  protected $choice_order;
  public $result_answer_id;

  /**
   * Constructor.
   *
   * @param int $result_id
   *   The result ID for the user's result set. There is one result ID per time
   *   the user takes a quiz.
   * @param stdClass $question_node
   *   The question node.
   * @param mixed $values
   *   Array with values.
   */
  public function __construct($result_id, stdClass $question_node, $values = NULL) {
    parent::__construct($result_id, $question_node, $values['user_answer']);
    $this->user_answer_ids = array();
    // Tries is the tries part of the post data.
    if (isset($values['user_answer'])) {
      if (!is_array($values['user_answer'])) {
        // Account for single-select.
        $values['user_answer'] = array($values['user_answer']);
      }
      if (isset($values['choice_order'])) {
        $this->choice_order = $values['choice_order'];
      }
      unset($values['choice_order']);
      if (isset($values['user_answer']) && is_array($values['user_answer'])) {
        foreach ($values['user_answer'] as $answer_id) {
          $this->user_answer_ids[] = $answer_id;
          // @todo: Stop using user_answer_ids and only use answer instead...
          $this->answer = $this->user_answer_ids;
        }
      }
      elseif (isset($values['user_answer'])) {
        $this->user_answer_ids[] = $values['user_answer'];
      }
    }
    // We load the answer from the database.
    elseif (isset($this->result_answer_id)) {
      $res = db_query('SELECT answer_id FROM {quiz_multichoice_user_answers} ua
        LEFT OUTER JOIN {quiz_multichoice_user_answer_multi} uam ON (uam.user_answer_id = ua.id)
        WHERE ua.result_answer_id = :raid', array(':raid' => $this->result_answer_id));
      while ($res_o = $res->fetch()) {
        $this->user_answer_ids[] = $res_o->answer_id;
      }
    }
  }

  /**
   * Implementation of save().
   *
   * @see QuizQuestionResponse::save()
   *
   * @throws \FieldsOverlapException
   * @throws \NoFieldsException
   */
  public function save() {
    $this->delete();

    $user_answer_id = db_insert('quiz_multichoice_user_answers')
      ->fields(array(
        'result_answer_id' => $this->result_answer_id,
        'choice_order' => $this->choice_order,
      ))
      ->execute();

    $query = db_insert('quiz_multichoice_user_answer_multi')
      ->fields(array('user_answer_id', 'answer_id'));
    for ($i = 0; $i < count($this->user_answer_ids); $i++) {
      if ($this->user_answer_ids[$i]) {
        $query->values(array($user_answer_id, $this->user_answer_ids[$i]));
      }
    }
    $query->execute();
  }

  /**
   * Implementation of delete().
   *
   * @see QuizQuestionResponse::delete()
   */
  public function delete() {
    $user_answer_id = array();
    $query = db_query('SELECT id FROM {quiz_multichoice_user_answers} WHERE result_answer_id = :raid', array(':raid' => $this->result_answer_id));
    while ($user_answer = $query->fetch()) {
      $user_answer_id[] = $user_answer->id;
    }

    if (!empty($user_answer_id)) {
      db_delete('quiz_multichoice_user_answer_multi')
        ->condition('user_answer_id', $user_answer_id, 'IN')
        ->execute();
    }

    db_delete('quiz_multichoice_user_answers')
      ->condition('result_answer_id', $this->result_answer_id)
      ->execute();
  }

  /**
   * Implementation of score().
   *
   * @see QuizQuestionResponse::score()
   */
  public function score() {
    if ($this->question->choice_boolean || $this->isAllWrong()) {
      $score = $this->getQuizQuestion()->getMaximumScore();
      foreach ($this->question->alternatives as $key => $alt) {
        if (in_array($alt['id'], $this->user_answer_ids)) {
          if ($alt['score_if_chosen'] <= $alt['score_if_not_chosen']) {
            $score = 0;
          }
        }
        else {
          if ($alt['score_if_chosen'] > $alt['score_if_not_chosen']) {
            $score = 0;
          }
        }
      }
    }
    else {
      $score = 0;
      foreach ($this->question->alternatives as $key => $alt) {
        if (in_array($alt['id'], $this->user_answer_ids)) {
          $score += $alt['score_if_chosen'];
        }
        else {
          $score += $alt['score_if_not_chosen'];
        }
      }
    }
    return $score;
  }

  /**
   * Implementation of getResponse().
   *
   * @see QuizQuestionResponse::getResponse()
   */
  public function getResponse() {
    return $this->user_answer_ids;
  }

  /**
   * Checks if all answers in a question are wrong.
   *
   * @return bool
   *   TRUE if all answers are wrong. False otherwise.
   */
  public function isAllWrong() {
    foreach ($this->question->alternatives as $key => $alt) {
      if ($alt['score_if_chosen'] > 0 || $alt['score_if_not_chosen'] > 0) {
        return FALSE;
      }
    }
    return TRUE;
  }

  /**
   * Implementation of getFeedbackValues().
   *
   * @see QuizQuestionResponse::getFeedbackValues()
   */
  public function getFeedbackValues() {
    $this->orderAlternatives($this->question->alternatives);
    $simple_scoring = $this->question->choice_boolean;

    $data = array();
    foreach ($this->question->alternatives as $alternative) {
      $chosen = in_array($alternative['id'], $this->user_answer_ids);
      $not = $chosen ? '' : 'not_';

      $data[] = array(
        'choice' => check_markup($alternative['answer']['value'], $alternative['answer']['format']),
        'attempt' => $chosen ? quiz_icon('selected') : '',
        'correct' => $chosen ? $alternative['score_if_chosen'] > 0 ? quiz_icon('correct') : quiz_icon('incorrect') : '',
        'score' => $alternative["score_if_{$not}chosen"],
        'answer_feedback' => check_markup($alternative["feedback_if_{$not}chosen"]['value'], $alternative["feedback_if_{$not}chosen"]['format'], FALSE),
        'question_feedback' => 'Question feedback',
        'solution' => $alternative['score_if_chosen'] > 0 ? quiz_icon('should') : ($simple_scoring ? quiz_icon('should-not') : ''),
        'quiz_feedback' => "Quiz feedback",
      );
    }

    return $data;
  }

  /**
   * Order the alternatives according to the choice order stored in the database.
   *
   * @param array $alternatives
   *   The alternatives to be ordered.
   */
  protected function orderAlternatives(array &$alternatives) {
    if (!$this->question->choice_random) {
      return;
    }
    $result = db_query('SELECT choice_order FROM {quiz_multichoice_user_answers}
            WHERE result_answer_id = :raid', array(':raid' => $this->result_answer_id))->fetchField();
    if (!$result) {
      return;
    }
    $order = explode(',', $result);
    $newAlternatives = array();
    foreach ($order as $value) {
      foreach ($alternatives as $alternative) {
        if ($alternative['id'] == $value) {
          $newAlternatives[] = $alternative;
          break;
        }
      }
    }
    $alternatives = $newAlternatives;
  }

  /**
   * Get answers for a question in a result.
   *
   * This static method assists in building views for the mass export of
   * question answers.
   *
   * @see views_handler_field_prerender_list for the expected return value.
   *
   * @param array $result_answer_ids
   *
   * @return array
   */
  public static function viewsGetAnswers(array $result_answer_ids = array()) {
    $ras = entity_load('quiz_result_answer', $result_answer_ids);
    $items = array();
    $nids = db_select('quiz_node_results_answers', 'qra')
      ->fields('qra', array('question_nid'))
      ->condition('result_answer_id', $result_answer_ids)
      ->execute()
      ->fetchAllKeyed(0,0);
    $nodes = node_load_multiple($nids);
    foreach ($ras as $ra) {
      $question = $nodes[$ra->question_nid];
      /* @var $ra_i QuizQuestionResponse */
      $ra_i = _quiz_question_response_get_instance($ra->result_id, $question);

      $alternatives = array();
      foreach ($question->alternatives as $alternative) {
        $alternatives[$alternative['id']] = $alternative;
      }

      foreach ($ra_i->getResponse() as $answer_id) {
        if (!empty($answer_id)) {
          $items[$ra->result_id][] = array('answer' => $alternatives[$answer_id]['answer']['value']);
        }
      }
    }

    return $items;
  }
}
