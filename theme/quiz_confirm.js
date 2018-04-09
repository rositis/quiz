(function ($) {
  Drupal.behaviors.quizAnswerConfirm = {
    attach: function (context, settings) {
      $('#quiz-question-answering-form #edit-navigation-submit').click(function (event) {
        // Return false to avoid submitting if user aborts
        if (!confirm($('#quiz-question-answering-form').data('confirm-message'))) {
          event.preventDefault();
        }
      });
    }
  };
})(jQuery);

