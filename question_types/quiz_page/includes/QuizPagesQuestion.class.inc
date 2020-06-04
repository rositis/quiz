<?php

/**
 * @file
 * Quiz page classes.
 *
 * This module uses the question interface to define something which is
 * actually not a question.
 *
 * A Quiz page node is a placeholder for presenting multiple questions
 * on the same page.
 */

/**
 * Extension of QuizQuestion.
 */
class QuizPagesQuestion extends QuizQuestion {

  /**
   * Implementation of validateNode().
   *
   * @see QuizQuestion::validateNode()
   *
   * @param array $form
   */
  public function validateNode(array &$form) {}

  /**
   * Implementation of getCreationForm().
   *
   * @see QuizQuestion::getCreationForm()
   *
   * @param array|null $form_state
   *
   * @return array
   */
  public function getCreationForm(array &$form_state = NULL) {
    return array();
  }

  /**
   * Implementation of getMaximumScore().
   *
   * @see QuizQuestion::getMaximumScore()
   */
  public function getMaximumScore() {
    return 0;
  }

  /**
   * Implementation of saveNodeProperties().
   *
   * @see QuizQuestion::saveNodeProperties()
   */
  public function saveNodeProperties() {}

  /**
   * Implementation of getAnsweringForm().
   *
   * @see QuizQuestion::getAnsweringForm()
   *
   * @param int $result_id
   * @param array|null $form_state
   *
   * @return array|string[]
   */
  public function getAnsweringForm($result_id, array $form_state = NULL) {
    $element = array(
      '#type' => 'hidden',
    );
    return $element;
  }

  /**
   * Implementation of getBodyFieldTitle().
   *
   * @see QuizQuestion::getBodyFieldTitle()
   */
  public function getBodyFieldTitle() {
    return t('Page');
  }

  /**
   * Implementation of isGraded().
   *
   * @see QuizQuestion::isGraded()
   */
  public function isGraded() {
    return FALSE;
  }

  /**
   * Implementation of hasFeedback().
   *
   * @see QuizQuestion::hasFeedback()
   */
  public function hasFeedback() {
    return FALSE;
  }

  /**
   * Implementation of isQuestion().
   *
   * @see QuizQuestion::hasFeedback()
   */
  public function isQuestion() {
    return FALSE;
  }
}
