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
 * Extension of QuizQuestion.
 */
class MatchingQuestion extends QuizQuestion {

  /**
   * Constructor
   *
   * @param $node
   *  matching node
   */
  public function __construct(stdClass $node) {
    parent::__construct($node);
    if (empty($this->node->match)) {
      $this->node->match = array();
    }
  }

  /**
   * Implementation of validateNode().
   *
   * @see QuizQuestion::validateNode()
   *
   * No validation is required. The first two pairs are required in the form,
   * if there are other errors we forgive them.
   */
  public function validateNode(array &$form) {}

  /**
   * Implementation of getCreationForm().
   *
   * @see QuizQuestion::getCreationForm()
   *
   * @param array|null $form_state
   *
   * @return array|mixed
   */
  public function getCreationForm(array &$form_state = NULL) {
    $config = config('matching.settings');
    $penalty = isset($this->node->nid) ? $this->node->choice_penalty : $config->get('choice_penalty');

    $form['settings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Settings'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['settings']['choice_penalty'] = array(
      '#type' => 'checkbox',
      '#title' => t('Penalty for guessing'),
      '#description' => t('Subtract 1 point from the users score for each incorrect match. Scores cannot go below 0.'),
      '#default_value' => $penalty,
      '#parents' => array('choice_penalty'),
    );

    $form['match'] = array(
      '#type' => 'fieldset',
      '#title' => t('Answer'),
      '#weight' => -4,
      '#tree' => TRUE,
      '#theme' => 'matching_question_form',
      '#description' => t('Write your pairs in the question and answer columns. For the user the question will be fixed and the answers will be shown as alternatives in a dropdown box.'),
    );
    for ($i = 1; $i <= $config->get('quiz_matching_form_size'); ++$i) {
      $form['match'][$i] = array(
        '#type' => 'fieldset',
        '#title' => t('Question ' . $i),
      );
      $form['match'][$i]['match_id'] = array(
        '#type' => 'value',
        '#default_value' => isset($this->node->match[$i - 1]['match_id']) ? $this->node->match[$i - 1]['match_id'] : ''
      );
      $form['match'][$i]['question'] = array(
        '#type' => 'textarea',
        '#rows' => 2,
        '#default_value' => isset($this->node->match[$i - 1]['question']) ? $this->node->match[$i - 1]['question'] : '',
        '#required' => $i < 3,
      );
      $form['match'][$i]['answer'] = array(
        '#type' => 'textarea',
        '#rows' => 2,
        '#default_value' => isset($this->node->match[$i - 1]['answer']) ? $this->node->match[$i - 1]['answer'] : '',
        '#required' => $i < 3,
      );

      $form['match'][$i]['feedback'] = array(
        '#type' => 'textarea',
        '#rows' => 2,
        '#default_value' => isset($this->node->match[$i - 1]['feedback']) ? $this->node->match[$i - 1]['feedback'] : ''
      );
    }
    return $form;
  }

  /**
   * Implementation of getMaximumScore().
   *
   * @see QuizQuestion::getMaximumScore()
   */
  public function getMaximumScore() {
    $to_return = 0;
    foreach ($this->node->match as $match) {
      if (empty($match['question']) || empty($match['answer'])) {
        continue;
      }
      $to_return++;
    }
    // The maximum score = the number of sub-questions.
    return $to_return;
  }

  /**
   * Implementation of saveNodeProperties().
   *
   * @see QuizQuestion::saveNodeProperties()
   *
   * @param bool $is_new
   *
   * @throws \FieldsOverlapException
   * @throws \InvalidMergeQueryException
   * @throws \NoFieldsException
   */
  public function saveNodeProperties($is_new = FALSE) {
    // Update or insert the question properties.
    db_merge('quiz_matching_properties')
      ->key(array(
        'nid' => $this->node->nid,
        'vid' => $this->node->vid,
      ))
      ->fields(array(
        'choice_penalty' => intval($this->node->choice_penalty),
      ))
      ->execute();

    // Loop through each question-answer combination.
    foreach ($this->node->match as $match) {
      $match['feedback'] = (isset($match['feedback'])) ? $match['feedback'] : '';
      // match_id is not so it is a new question.
      if ($is_new || empty($match['match_id']) || $this->node->revision || isset($this->node->node_export_drupal_version)) {
        if (!empty($match['question']) && !empty($match['answer'])) {
          $id = db_insert('quiz_matching_node')
            ->fields(array(
              'nid' => $this->node->nid,
              'vid' => $this->node->vid,
              'question' => $match['question'],
              'answer' => $match['answer'],
              'feedback' => $match['feedback'],
            ))
            ->execute();
        }
      }
      // match_id is set, user may remove or update existing question.
      else {
        if (empty($match['question']) && empty($match['answer'])) {
          // remove sub question.
          db_delete('quiz_matching_node')
            ->condition('match_id', $match['match_id'])
            ->execute();
        }
        else {
          // update sub question.
          db_update('quiz_matching_node')
            ->fields(array(
              'question' => $match['question'],
              'answer' => $match['answer'],
              'feedback' => $match['feedback'],
            ))
            ->condition('match_id', $match['match_id'])
            ->execute();
        }
      }
    }
  }

  /**
   * Implementation of delete().
   *
   * @see QuizQuestion::delete()
   */
  public function delete($only_this_version = FALSE) {
    parent::delete($only_this_version);

    $delete_properties = db_delete('quiz_matching_properties')->condition('nid', $this->node->nid);

    if ($only_this_version) {
      $delete_properties->condition('vid', $this->node->vid);

      db_delete('quiz_matching_node')
        ->condition('nid', $this->node->nid)
        ->condition('vid', $this->node->vid)
        ->execute();
    }
    // Delete all versions of this question.
    else {

      db_delete('quiz_matching_node')
        ->condition('nid', $this->node->nid)
        ->execute();
    }

    $delete_properties->execute();
  }

  /**
   * Implementation of getNodeProperties().
   *
   * @see QuizQuestion::getNodeProperties()
   */
  public function getNodeProperties() {
    if (isset($this->nodeProperties)) {
      return $this->nodeProperties;
    }
    $props = parent::getNodeProperties();

    $res_a = db_query('SELECT choice_penalty FROM {quiz_matching_properties}
            WHERE nid = :nid AND vid = :vid', array(':nid' => $this->node->nid, ':vid' => $this->node->vid))->fetchAssoc();

    if (is_array($res_a)) {
      $props = array_merge($props, $res_a);
    }

    //$sql = "SELECT match_id, question, answer, feedback FROM {quiz_matching_node} WHERE nid = %d AND vid = %d";
    $query = db_query('SELECT match_id, question, answer, feedback FROM {quiz_matching_node} WHERE nid = :nid AND vid = :vid', array(':nid' => $this->node->nid, ':vid' => $this->node->vid));
    while ($result = $query->fetch()) {
      $props['match'][] = array(
        'match_id' => $result->match_id,
        'question' => $result->question,
        'answer' => $result->answer,
        'feedback' => $result->feedback,
      );
    }
    $this->nodeProperties = $props;
    return $props;
  }

  /**
   * Implementation of getNodeView().
   *
   * @see QuizQuestion::getNodeView()
   */
  public function getNodeView() {
    $content = parent::getNodeView();

    [$matches, $select_option] = $this->getSubquestions();
    $subquestions = array();
    if ($this->viewCanRevealCorrect()) {
      foreach ($matches as $match) {
        $subquestions[] = array(
          'question' => $match['question'],
          'correct' => $match['answer'],
          'feedback' => $match['feedback']
        );
      }
    }
    else {
      // shuffle the answer column.
      foreach ($matches as $match) {
        $sub_qs[] = $match['question'];
        $sub_as[] = $match['answer'];
      }
      shuffle($sub_as);
      foreach ($sub_qs as $i => $sub_q) {
        $subquestions[] = array(
          'question' => $sub_q,
          'random' => $sub_as[$i],
        );
      }
    }
    $content['answers'] = array(
      '#markup' => theme('matching_match_node_view', array('subquestions' => $subquestions)),
      '#weight' => 2,
    );
    return $content;
  }

  /**
   * Implementation of getAnsweringForm().
   *
   * @see QuizQuestion::getAnsweringForm()
   */
  public function getAnsweringForm($result_id, array $form_state = NULL) {
    $config = config('matching.settings');
    $form = parent::getAnsweringForm($result_id, $form_state);

    if (isset($result_id)) {
      // The question has already been answered. We load the answers.
      $response = new MatchingResponse($result_id, $this->node);
      $responses = $response->getResponse();
    }
    [$matches, $select_option] = $this->getSubquestions();
    //$form['#theme'] = 'matching_subquestion_form';
    foreach ($matches as $match) {
      $form[$match['match_id']] = array(
        '#title' => $match['question'],
        '#type' => 'select',
        '#options' => array('def' => ''),
      );
      $form[$match['match_id']]['#options'] += $this->customShuffle($select_option);

      if ($responses) {
        // If this question already has been answered.
        $form[$match['match_id']]['#default_value'] = $responses[$match['match_id']];
      }
    }
    if ($config->get('quiz_matching_shuffle_options')) {
      $form = $this->customShuffle($form);
    }
    $form['scoring_info'] = array(
      '#access' => !empty($this->node->choice_penalty),
      '#markup' => '<p><em>' . t('You lose points by selecting incorrect options. You may leave an option blank to avoid losing points.') . '</em></p>',
    );
    return $form;
  }

  /**
   * Question response validator.
   *\
   * @param array $element
   * @param mixed $values
   *
   * @return bool|void
   */
  public function getAnsweringFormValidate(array &$element, &$values) {
    foreach ($values as $value) {
      if ($value != 'def') {
        return TRUE;
      }
    }
    form_error($element, t('You need to match at least one of the items.'));
  }

  /**
   * Shuffles an array, but keep the keys.
   *
   * @param array $array
   *   Array to be shuffled
   * @return array
   *   A shuffled version of the array with keys preserved.
   */
  private function customShuffle(array $array = array()) {
    $new_array = array();
    while (count($array)) {
      $element = array_rand($array);
      $new_array[$element] = $array[$element];
      unset($array[$element]);
    }
    return $new_array;
  }

  /**
   * Helper function to fetch subquestions
   *
   * @return array
   *  Array with two arrays, matches and selected options
   */
  public function getSubquestions() {
    $matches = $select_option = array();
    //$sql = "SELECT match_id, question, answer, feedback FROM {quiz_matching_node} WHERE nid = %d AND vid = %d";
    $query = db_query('SELECT match_id, question, answer, feedback FROM {quiz_matching_node} WHERE nid = :nid AND vid = :vid', array(':nid' => $this->node->nid, ':vid' => $this->node->vid));
    while ($result = $query->fetch()) {
      $matches[] = array(
        'match_id' => $result->match_id,
        'question' => $result->question,
        'answer' => $result->answer,
        'feedback' => $result->feedback,
      );
      $select_option[$result->match_id] = $result->answer;
    }
    return array($matches, $select_option);
  }

  /**
   * Implementation of getBodyFieldTitle().
   *
   * @see QuizQuestion::getBodyFieldTitle()
   */
  public function getBodyFieldTitle() {
    return t('Instruction');
  }

  /**
   * Get the correct answers for this question.
   *
   * @return array
   *  Array of correct answers
   */
  public function getCorrectAnswer() {
    $correct_answers = array();
    $query = db_query('SELECT match_id, question, answer, feedback FROM {quiz_matching_node} WHERE nid = :nid AND vid = :vid', array(':nid' => $this->node->nid, ':vid' => $this->node->vid));
    while ($result = $query->fetch()) {
      $correct_answers[$result->match_id] = array(
        'match_id' => $result->match_id,
        'question' => $result->question,
        'answer' => $result->answer,
        'feedback' => $result->feedback,
      );
    }
    return $correct_answers;
  }
}
