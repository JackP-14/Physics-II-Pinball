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

SCORING
-------
 Eye bumpers ................................ 100
 Slingshots ................................. 10
 Pumpkin slot / candy card (first hit) ...... 250   (50 if already lit)
 Sinkhole next to the tombstone ............. 500   (holds the ball and spits it out)

 COMBOS
 Carve the Pumpkin: hit the 3 slots under the pumpkin ... +1000 and an EXTRA BALL (max 5)
 Candy cards: hit the 3 cards under the third eye ....... +2000

The game shows the current score, the previous round score and the highest score of the session.

DIFFERENCES WITH THE ORIGINAL
-----------------------------
 - The ramps (bone ramp on the left and the end of the orange ramp on the right) are drawn
   but act as walls: the ball cannot ride them.
 - The launch lane follows the orange band around the top and drops the ball above the pumpkin.
 - The outlanes and the bottom-left / bottom-right posts are not reachable.
 - "Shoot Again" and the ball counter painted on the table are decorative; the HUD on the
   left shows the balls left instead.
 - The cauldron, bat and tombstone are solid obstacles without special rules.
 - Added: sinkhole that holds the ball, candy card bonus, runtime physics settings and FPS control.
 - Sound effects are our own (synthesized), except bonus.wav.
