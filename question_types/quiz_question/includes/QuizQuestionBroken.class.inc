<?php


class QuizQuestionBroken extends QuizQuestion {

  /**
   * @see QuizQuestion::validateNode()
   */
  public function validateNode(array &$form) {}

  /**
   * @see QuizQuestion::getCreationForm()
   */
  public function getCreationForm(array &$form_state = NULL) {}

  /**
   * @see QuizQuestion::getMaximumScore()
   */
  public function getMaximumScore() {}

  /**
   * @see QuizQuestion::saveNodeProperties()
   */
  public function saveNodeProperties() {}

  /**
   * @see QuizQuestion::hasFeedback()
   */
  public function hasFeedback() {
    return FALSE;
  }
}
