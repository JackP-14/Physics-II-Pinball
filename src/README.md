# Physics II Halloween Pinball by SlopSquad Team

## Original Game Link

https://www.247pinball.com/halloween/

## Github Page

https://github.com/JackP-14/Physics-II-Pinball

## Description

A recreation of a Pinball game built with Raylib for rendering and Box2D for handling the physics.

## Controls

 - Left/Right Arrow  -->   Left/Right Flippers
 - Down Arrow (Hold) -->   Kicker
 - F1                -->   Debug
 - F2/F3             -->   Increase/Decrease Target FPS
 - F4/F5             -->   Increase/Decrease Gravity
 - F6/F7             -->   Increase/Decrease Ball bounce
 - F8/F9             -->   Increase/Decrease Ball friction
 - F11               -->   Reset Settings
=======
# Halloween Pinball

## Description

Pinball made for Physics II (Project 01: Classic Physics) with **Box2D v2.4.1** and **raylib**, recreating a Halloween-themed flash pinball.

- Original game: TODO_LINK_TO_ORIGINAL_GAME
- Source: https://github.com/JackP-14/Physics-II-Pinball

## Key Features

 - Coffin flippers (revolute joints with motor and limits) and a spring kicker (prismatic joint, animated spring) whose power depends on how long it is pulled
 - Table walls built with Box2D chains traced over our own table art; pumpkin bumpers (restitution 0.9) and slingshots (0.6)
 - Rounds of 3 balls with current, previous and highest score of the session; rounds cycle automatically
 - Combo *Trick or Treat* (both pumpkins + cauldron lane = extra ball); lane that passes under the cauldron, which spits the ball back into play (skill shot when coming straight from the kicker)
 - 4 collectible letters (D, N, M, L) spread over the table, kept between balls: all of them = extra ball
 - Sticky spider web: a sensor that applies vertical-only damping (custom, per physics step) and lowers the ball's gravity scale until it lets the ball go
 - Ball save: a ball lost in the first 15 seconds after its launch is given back once
 - Kickback in the left outlane: once per ball it shoots the ball back up into the playfield
 - Fixed-timestep physics: the target FPS can be changed without slowing down or speeding up the simulation
 - Runtime settings for gravity, ball restitution and ball friction
 - Debug mode (F1) drawing every shape, with a mouse joint to drag bodies

## Controls

| Key | Action |
|---|---|
| Left / Right arrows | Flippers |
| Down arrow | Hold to pull the kicker, release to launch |
| Enter | Start round / skip round summary |
| F1 | Debug draw + mouse joint |
| F2 / F3 | Target FPS - / + |
| F4 / F5 | Gravity - / + |
| F6 / F7 | Ball restitution - / + |
| F8 / F9 | Ball friction - / + |
| F11 | Reset physics settings |

## Differences with the Original

We removed the corridor from the pumpkin to the lane.

## Slopsquad Team Members:

Marcel Ribot Llorens: https://github.com/JackP-14  
Lux Duart Pérez: https://github.com/Ballo0x  
Daniel Martínez Martínez: https://github.com/Dunge0nraider  
Nil Fernández Muniente: https://github.com/xdHyfa  

## License

This project is licensed under an unmodified MIT license, which is an OSI-certified license that allows static linking with closed source software. Check [LICENSE](LICENSE) for further details.

