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
 * Extension of QuizQuestionResponse.
 */
class QuizPagesResponse extends QuizQuestionResponse {

  /**
   * Implementation of save().
   *
   * @see QuizQuestionResponse::save()
   */
  public function save() {}

  /**
   * Implementation of delete().
   *
   * @see QuizQuestionResponse::delete()
   */
  public function delete() {}

  /**
   * Implementation of score().
   *
   * @see QuizQuestionResponse::score()
   */
  public function score() {}

  /**
   * Implementation of getResponse().
   *
   * @see QuizQuestionResponse::getResponse()
   *
   * @return mixed|null
   */
  public function getResponse() {
    return $this->answer;
  }

  /**
   * Implementation of isCorrect().
   *
   * @see QuizQuestionResponse::isCorrect()
   *
   * @return bool
   */
  public function isCorrect() {
    return TRUE;
  }

  /**
   * Implementation of getReportForm().
   *
   * @see QuizQuestionResponse::getReportForm()
   *
   * @return array|bool[]|null
   */
  public function getReportForm() {
    return array(
      '#no_report' => TRUE,
    );
  }
}
