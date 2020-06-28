<?php

/**
 * Extension of QuizQuestionResponse.
 */
class QuizDirectionsResponse extends QuizQuestionResponse {

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
  public function score() {
    // Set the score.
    $this->score = 0;
    return 0;
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
   * Implementation of isCorrect().
   *
   * @see QuizQuestionResponse::isCorrect()
   */
  public function isCorrect() {
    return TRUE;
  }

  /**
   * Implementation of getReportForm().
   *
   * @see QuizQuestionResponse::getReportForm()
   */
  public function getReportForm() {
    return array(
      '#no_report' => TRUE,
    );
  }
}

