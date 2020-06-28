Developing for Quiz
==============
Hooks for interacting with a quiz:
 - `hook_<entity_type>_<op>`: These hooks are called for any entity operations.
 - quiz (settings for quiz nodes)
 - quiz_result (quiz attempt/result)
 - quiz_result_answer (answer to a specific question in a quiz result)
 - quiz_question (generic settings for question nodes)
 - quiz_question_relationship (relationship from quiz to question)
 - See `quiz.api.php` for more details.

Developing new question types
-------------
You need to create a new module that extends the existing question type core.
The truefalse or multichoice question types are good places to start.
Here are the steps:

1. Create a new module.
2. Use your module's .install file to create the necessary tables for property
   and answer storage.
3. Make sure you module implements `hook_quiz_question_info()`.
4. Define classes that extend `QuizQuestion` and `QuizQuestionResponse`.
   For a complete example, see one of the class files in the `includes
   ` folders in one of the question types, such as the 
   `includes/MultichoiceQuestion.class.inc` and the `includes
   /MultichoiceResponse.class.inc` files.
