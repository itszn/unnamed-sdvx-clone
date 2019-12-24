Collection Dialog
=================
The collection dialog is a bit different from most other scripts in the game as it should not be treated
as a whole screen but more like a window so it's recommended that you do not take up the entire screen with
the dialog as it can show up on multiple screens in the game.

There are two tables available to the collection dialog and those are ``dialog`` which contains:

.. code-block:: c#

    string title
    string artist
    string jacket //path to the jacket of the song being added
    bool closing //true when the dialog should be running its closing animation
    bool isTextEntry //true when the user has decided to create a new collection
    Collection[] collections //list of the existing collections

The entries in the ``dialog.collections`` list look like this:

.. code-block:: c#

    string name
    bool exists //if the song already exists in this collection
	
When a user selects a collection that already has the song in it then the game will remove the song from that collection
so it is a good idea to indicate that somehow.

The other table available to the dialog is ``menu`` which contains a number of functions used to progress through the
collection adding/removing process.

Menu Functions
**************
These are the functions that exist in the ``menu`` table.

Cancel()
^^^^^^^^
Closes the collection dialog with no changes made to collections.

Confirm(string collection)
^^^^^^^^^^^^^^^^^^^^^^^^^^
Adds or removes the song from the collection with the given name.

ChangeState()
^^^^^^^^^^^^^
Toggles between text entry mode and regular navigation.

Calls made to lua
*****************
Calls made from the game to the script.


button_pressed(int button)
^^^^^^^^^^^^^^^^^^^^^^^^^^
Called whenever a button is pressed, ``button`` uses the same values as the ``game.BUTTON_*`` values.

advance_selection(int steps)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Tells the script to advance the selection by a given number of steps.