Multichoice
==============
This module provides a multiple choice question type for Quiz.

The multichoice module lets the user create single answer questions and multiple
answer questions. Advanced feedback and scoring options are also available.

The multichoice module is based on the OO framework of the Quiz project.

Installation
------------
To install, unpack the module to your modules folder, and simply enable the
module at Admin > Build > Modules.

Several database tables prefixed with quiz_multichoice are installed to have a
separate storage for this module.

Usage
------------
Settings are to be found here: admin/quiz/questions_settings

The module has three settings.

**Multiple answers** allows the quiz taker to select more than one alternative (it also allows for the possibility that
none of the alternatives are correct). Alternatives are selected using checkboxes instead of radio buttons.

**Random order** displays the alternatives in random order when quiz is beeing taken.

**Simple scoring** gives max score if everything is correct. Zero points otherwise.

The scoring system in multichoice is a bit complex. With multiple answers each alternative adds a given number of points
to the total score if it is chosen, and another number of points is added if the alternative isn't chosen. Both
`score if chosen` and `score if not chosen` may be edited for each alternative by the question creator. If multiple
answers isn't allowed the score will be set to the `score if chosen` of the alternative that has been chosen. The
question is considered correct if the quiz taker gets the maximum amount of points possible for the question.

Current Maintainers
-------------------
* [Ryan Osītis](https://github.com/rositis)

Credits
-------------------
* Ported to Backdrop CMS by [Ryan Osītis](https://github.com/rositis).
* Drupal Sponsor: Norwegian Centre for Telemedicine
* Drupal Code: falcon

License
-------------------
This project is GPL v2 software.
See the LICENSE.txt file in this directory for complete text.
