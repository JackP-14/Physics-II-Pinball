HALLOWEEN PINBALL
=================

Physics II - Project 01: Classic Physics (Box2D v2.4.1 + raylib)

Original game:  TODO_LINK_TO_ORIGINAL_GAME
Source code:    https://github.com/JackP-14/Physics-II-Pinball

STUDENTS
--------
 - TODO Name Lastname
 - TODO Name Lastname
 - TODO Name Lastname
 - TODO Name Lastname


HOW TO PLAY
-----------
Run HalloweenPinball.exe (keep the Assets folder next to it).
Press ENTER to start a round. Each round has 3 balls; when the last one drains the round
ends, its score becomes the PREVIOUS score and a new round starts automatically.

CONTROLS
--------
 LEFT arrow ........ left flipper
 RIGHT arrow ....... right flipper
 DOWN arrow ........ hold to pull the kicker, release to launch (longer = stronger)
 ENTER ............. start round / skip the round summary

 F1 ................ debug mode: draw every physics shape and drag bodies with the mouse (mouse joint)
 F2 / F3 ........... lower / raise the target FPS (15 - 240). The physics runs in real time at any FPS
 F4 / F5 ........... lower / raise gravity (1 - 20 m/s2)
 F6 / F7 ........... lower / raise the ball bounciness (restitution 0.0 - 1.0)
 F8 / F9 ........... lower / raise the ball friction (0.0 - 1.0)
 F11 ............... reset gravity, bounciness and friction to their defaults

TABLE
-----
 - Coffin flippers and a spring kicker (the spring compresses while you pull it).
 - Two pumpkin bumpers and two spider web slingshots.
 - Cauldron lane (top left): the ball rolls UNDER the cauldron, down the slime lane, and the
   cauldron spits it back towards the center of the table. Reaching it straight from the
   kicker is a skill shot (points, but it does not count for the combo).
 - Ball save: during the first 15 seconds after a launch, a lost ball is given back once.
 - Kickback (left outlane): once per ball it shoots the ball back up into the playfield.
   The green arrow in the outlane shows it is ready.
 - The letters D, N, M, L float over the table: collect them all for an extra ball. Collected
   letters are kept from one ball to the next (they reset when a new round starts).
 - Big spider web (top left): the ball gets stuck in it for a moment. It falls very slowly
   (vertical damping and low gravity) but keeps its sideways speed, then the web lets it go.

SCORING
-------
 Pumpkin bumpers ............................ 100
 Slingshots ................................. 10
 Cauldron lane .............................. 500
 Skill shot (cauldron lane straight from the kicker) ... 750
 Stuck in the spider web .................... 300
 Letter ..................................... 150
 Lighting a combo element (first time) ...... 250

 COMBOS (extra balls, max 5)
 Trick or Treat: hit both pumpkins and go through the cauldron lane
 (after the ball has touched a flipper) ..... +1500 and an EXTRA BALL
 DNML: collect the 4 letters ................ +1000 and an EXTRA BALL

The game shows the current score, the previous round score and the highest score of the session.

DIFFERENCES WITH THE ORIGINAL
-----------------------------
 - The table art is our own Halloween design, drawn for this project; it keeps the classic
   layout (two flippers, slingshots, inlanes/outlanes, plunger lane on the right).
 - Added: cauldron lane, sticky spider web, runtime physics settings and FPS control.
 - Sound effects are our own (synthesized).
