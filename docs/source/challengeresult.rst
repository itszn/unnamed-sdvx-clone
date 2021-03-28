Challenge Result Screen
=======================
The following fields are available under the ``result`` table.

.. code-block:: c#

    ChartResult[] charts // array of result information for all played charts (note: might not be all charts in course)

    string title // Title of the challenge
    int level // level of the challenge
    string requirement_text // Text with requirements to pass the challenge (contains newlines)
    bool passed // Did the player pass the challenge
    string failReason // A string explaining why the player failed the challenge
    string grade // "S", "AAA+", "AAA", etc.
    int badge

    // *Note* these averages are across all charts in the challenge including ones not played
    int avgScore
    int avgPercentage // average completion percentage
    float avgGauge // average final gauge
    int avgErrors // average number of errors gotten
    int avgNears // average number of nears gotten
    int avgCrits // average number of crits gotten

    int overallErrors // total number of errors in charts played
    int overallNears // total number of nears in charts played
    int overallCrits // total number of crits in charts played

    
ChartResult
***********

A ``ChartResult`` contains the following fields:

.. code-block:: c#

    // Entries specific to the challenge:
    bool passed // True if player passed challenge requirements for this chart
    string failReason // If not passed, this string hold the reason why

    // Normal challenge result entries
    int score
    int gauge_type // 0 = normal, 1 = hard. Should be defined in constants sometime
    int gauge_option // type specific, such as difficulty level for the same gauge type if available    
    bool mirror
    bool random
    int auto_flags //bits for autoplay settings, 0 = no autoplay
    float gauge // value of the gauge at the end of the song
    int misses
    int goods
    int perfects
    int maxCombo
    int level
    int difficulty
    string title
    string realTitle // Same as title
    string artist
    string effector
    string illustrator
    string bpm
    int duration // Length of the chart in milliseconds
    string jacketPath
    int medianHitDelta
    float meanHitDelta
    int medianHitDeltaAbs
    float meanHitDeltaAbs
    int earlies
    int lates
    int badge // same as song wheel badge (except 0 which means the user manually exited)
    float gaugeSamples[256] // gauge values sampled throughout the song
    string grade // "S", "AAA+", "AAA", etc.
    score[] highScores // Same as song wheel scores
    HitWindow hitWindow // Same as gameplay HitWindow
    bool autoplay
    float playbackSpeed
    int speedModType // 0 for XMOD, 1 for MMOD, 2 for CMOD
    int speedModValue // HiSpeed for XMOD, ModSpeed for MMOD and CMOD
    HitStat[] noteHitStats // contains HitStat for notes (excluding hold notes and lasers) 

HitStat
*******
A ``HitStat`` contains the following fields:
    
.. code-block:: c

    int rating // 0 for miss, 1 for near, 2 for crit
    int lane
    int time // In milliseconds
    float timeFrac // Between 0 and 1
    int delta


Calls made to lua
*****************
Calls made from the game to the script, these need to be defined for the game
to function properly.

result_set()
^^^^^^^^^^^^
This is called right after ``result`` is set, either for initial display or when reloaded

render(deltaTime)
^^^^^^^^^^^^^^^^^
The GUI render call.

get_capture_rect()
^^^^^^^^^^^^^^^^^^
The region of the screen to be saved in score screenshots.

Has to return ``x,y,w,h`` in pixel coordinates to the game.

screenshot_captured(path)
^^^^^^^^^^^^^^^^^^^^^^^^^
Called when a screenshot has been captured successfully with ``path`` being the
path to the saved screenshot.

Challenge.GetJSON()
*******************
Call to get the json data for the current selected challenge
