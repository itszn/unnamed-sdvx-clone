
Challenge Wheel
===============
Contains a list of challenges accessed by ``chalwheel.challenges``

The list of all (not filtered) challenges is available in ``chalwheel.allChallenges``

The current song database status is available in ``chalwheel.searchStatus``

Example for loading the jacket of the first chart for every challenge:

.. code-block:: lua

    for i,chal in ipairs(chalwheel.challenges) do
        jackets[chal.id] = gfx.CreateImage(chal.charts[1].jacketPath, 0)
    end

Challenge
***************
A challenge contains the following fields:


.. code-block:: c#

    string title
    string requirement_text // Text with requirements to pass the challenge (contains newlines)
    int id //unique static identifier
    badge topBadge //top badge for this difficulty
    int bestScore
    string grade // "S", "AAA+", "AAA", etc.
    bool missing_chart // Did the system find all the charts for this chal (can't play if false)
    Chart[] charts // Array of all the charts in order in the challenge (may have repeats)
    string searchText //current string used by the search
    bool searchInputActive //true when the user is currently inputting search text
    

Chart
**********
A chart contains the following fields:


.. code-block:: c#

    string title
    string artist
    string bpm //ex. "170-200"
    int id //unique static identifier
    int level
    int difficulty // 0 = nov, 1 = adv, etc.
    string effector
    string illustrator

Badge
*****
Values::
    
    0 = No badge/Never played
    1 = Played but not cleared
    2 = Cleared
    3 = Hard Cleared
    4 = Full Combo
    5 = Perfect


get_page_size
*************
Function called by the game to get how much to scroll when page up or page down are pressed.
Needs to be defined for the game to work properly.

challenges_changed(withAll)
***************************
Function called by the game when ``challenges`` or ``allChallenges`` (if withAll == true) is changed.

Challenge.GetJSON()
*******************
Call to get the json data for the current selected challenge
