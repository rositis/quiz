<?php

class Quiz extends Entity {
  /**
   * Implements EntityInterface::id().
   */
  public function id() {
    return isset($this->id) ? $this->id : NULL;
  }

  /**
   * Implements EntityInterface::entityType().
   */
  public function entityType() {
    return 'quiz';
  }

  /**
   * Implements EntityInterface::label().
   */
  public function label() {}

  /**
   * Implements EntityInterface::uri().
   */
  public function uri() {}
}
