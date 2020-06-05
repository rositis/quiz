<?php

/**
 * Extension of QuizQuestion.
 */
class ShortAnswerQuestion extends QuizQuestion {

  // Constants for answer matching options.
  const ANSWER_MATCH = 0;
  const ANSWER_INSENSITIVE_MATCH = 1;
  const ANSWER_REGEX = 2;
  const ANSWER_MANUAL = 3;

  /**
   * Implementation of validateNode().
   *
   * @see QuizQuestion::validateNode()
   *
   * @param array $form
   */
  public function validateNode(array &$form) {
    if ($this->node->correct_answer_evaluation != self::ANSWER_MANUAL && empty($this->node->correct_answer)) {
      form_set_error('correct_answer', t('An answer must be specified for any evaluation type other than manual scoring.'));
    }
  }

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
    $form['answer'] = array(
      '#type' => 'fieldset',
      '#title' => t('Answer'),
      '#description' => t('Provide the answer and the method by which the answer will be evaluated.'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#weight' => -4,
    );

    $options = array(
      self::ANSWER_MATCH => t('Automatic and case sensitive'),
      self::ANSWER_INSENSITIVE_MATCH => t('Automatic. Not case sensitive'),
    );
    $access_regex = user_access('use regex for short answer');
    if ($access_regex) {
      $options[self::ANSWER_REGEX] = t('Match against a regular expression (answer must match the supplied regular expression)');
    }
    $options[self::ANSWER_MANUAL] = t('Manual');

    $form['answer']['correct_answer_evaluation'] = array(
      '#type' => 'radios',
      '#title' => t('Pick an evaluation method'),
      '#description' => t('Choose how the answer shall be evaluated.'),
      '#options' => $options,
      '#default_value' => isset($this->node->correct_answer_evaluation) ? $this->node->correct_answer_evaluation : self::ANSWER_INSENSITIVE_MATCH,
      '#required' => FALSE,
    );
    if ($access_regex) {
      $form['answer']['regex_box'] = array(
        '#type' => 'fieldset',
        '#title' => t('About regular expressions'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );

      $form['answer']['regex_box']['regex_help'] = array(
        '#markup' => '<p>' .
          t('Regular expressions are an advanced syntax for pattern matching. They allow you to create a concise set of rules that must be met before a value can be considered a match.') .
          '</p><p>' .
          t('For more on regular expression syntax, visit !url.', array('!url' => l(t('the PHP regular expressions documentation'), 'http://www.php.net/manual/en/book.pcre.php'))) .
          '</p>',
      );
    }

    $form['answer']['correct_answer'] = array(
      '#type' => 'textfield',
      '#title' => t('Correct answer'),
      '#description' => t('Specify the answer. If this question is manually scored, no answer needs to be supplied.'),
      '#default_value' => isset($this->node->correct_answer) ? $this->node->correct_answer : '',
      '#size' => 60,
      '#maxlength' => 256,
      '#required' => FALSE,
    );
    return $form;
  }

  /**
   * Implementation of getMaximumScore().
   *
   * @see QuizQuestion::getMaximumScore()
   */
  public function getMaximumScore() {
    $config = config('short_answer.settings');
    return $config->get('short_answer_default_max_score');
  }

  /**
   * Implementation of saveNodeProperties().
   *
   * @see QuizQuestion::saveNodeProperties()
   *
   * @param bool $is_new
   *
   * @throws \InvalidMergeQueryException
   */
  public function saveNodeProperties($is_new = FALSE) {
    db_merge('quiz_short_answer_node_properties')
      ->key(array(
        'nid' => $this->node->nid,
        'vid' => $this->node->vid,
      ))
      ->fields(array(
        'nid' => $this->node->nid,
        'vid' => $this->node->vid,
        'correct_answer' => $this->node->correct_answer,
        'correct_answer_evaluation' => $this->node->correct_answer_evaluation,
      ))
      ->execute();
  }

  /**
   * Implementation of delete().
   *
   * @see QuizQuestion::delete()
   *
   * @param bool $only_this_version
   */
  public function delete($only_this_version = FALSE) {
    parent::delete($only_this_version);

    $delete_node = db_delete('quiz_short_answer_node_properties');
    $delete_node->condition('nid', $this->node->nid);

    if ($only_this_version) {
      $delete_node->condition('vid', $this->node->vid);
    }

    $delete_node->execute();
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
    $res_a = db_query('SELECT correct_answer, correct_answer_evaluation FROM {quiz_short_answer_node_properties}
      WHERE nid = :nid AND vid = :vid', array(':nid' => $this->node->nid, ':vid' => $this->node->vid))->fetchAssoc();
    $this->nodeProperties = (is_array($res_a)) ? array_merge($props, $res_a) : $props;
    return $this->nodeProperties;
  }

  /**
   * Implementation of getNodeView().
   *
   * @see QuizQuestion::getNodeView()
   */
  public function getNodeView() {
    $content = parent::getNodeView();
    if ($this->viewCanRevealCorrect()) {
      $content['answers'] = array(
        '#markup' => '<div class="quiz-solution">' . check_plain($this->node->correct_answer) . '</div>',
        '#weight' => 2,
      );
    }
    else {
      $content['answers'] = array(
        '#markup' => '<div class="quiz-answer-hidden">Answer hidden</div>',
        '#weight' => 2,
      );
    }
    return $content;
  }

  /**
   * Implementation of getAnsweringForm().
   *
   * @see QuizQuestion::getAnsweringForm()
   *
   * @param int $result_id
   * @param array|null $form_state
   *
   * @return array
   */
  public function getAnsweringForm($result_id, array $form_state = NULL) {
    $element = parent::getAnsweringForm($result_id, $form_state);
    //$form['#theme'] = 'short_answer_answering_form';

    $element += array(
      '#type' => 'textfield',
      '#title' => t('Answer'),
      '#description' => t('Enter your answer here'),
      '#default_value' => '',
      '#size' => 60,
      '#maxlength' => 256,
      '#required' => FALSE,
      '#attributes' => array('autocomplete' => 'off'),
    );

    if (isset($result_id)) {
      $response = new ShortAnswerResponse($result_id, $this->node);
      $element['#default_value'] = $response->getResponse();
    }

    return $element;
  }

  /**
   * Question response validator.
   *
   * @param array $element
   * @param mixed $value
   */
  public function getAnsweringFormValidate(array &$element, &$value) {
    if ($value == '') {
      form_error($element, t('You must provide an answer.'));
    }
  }

  /**
   * Evaluate the correctness of an answer.
   *
   * The way we decide how to evaluate the correctness depends on the
   * evaluation method of the question.
   *
   * @param string $user_answer
   *   The answer given by the user.
   *
   * @return int
   *   The given score for the answer.
   */
  public function evaluateAnswer($user_answer) {
    $score = 0;

    // Ignore white spaces for correct answer and user's answer.
    $user_answer = trim($user_answer);
    $this->node->correct_answer = trim($this->node->correct_answer);

    switch ($this->node->correct_answer_evaluation) {
      case self::ANSWER_MATCH:
        if ($user_answer == $this->node->correct_answer) {
          $score = $this->node->max_score;
        }
        break;

      case self::ANSWER_INSENSITIVE_MATCH:
        if (backdrop_strtolower($user_answer) == backdrop_strtolower($this->node->correct_answer)) {
          $score = $this->node->max_score;
        }
        break;

      case self::ANSWER_REGEX:
        if (preg_match($this->node->correct_answer, $user_answer) > 0) {
          $score = $this->node->max_score;
        }
        break;
    }
    return $score;
  }
}
