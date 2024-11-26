
# Pong Game with AI Support

## Overview
This is a classic Pong game implemented using the `libmx2` library. The game supports both manual and AI-controlled Player 1. Player 2 is always AI-controlled. The objective is to score points by getting the ball past your opponent's paddle.

## Features
- **Single-Player AI:** Player 1 can be controlled either manually or by an AI.
- **Smooth Ball and Paddle Movement:** Realistic paddle and ball dynamics with speed adjustments.
- **Scoring System:** Tracks scores for both players.
- **Dynamic AI Switching:** Easily toggle Player 1's AI on or off during the game.

## Controls
- **Up Arrow (↑):** Move Player 1's paddle up (if AI is off).
- **Down Arrow (↓):** Move Player 1's paddle down (if AI is off).
- **Spacebar:** Toggle AI for Player 1 on or off.

## How to Play
1. Launch the game.
2. The intro screen will fade out to start the game.
3. Use the arrow keys to control Player 1's paddle or enable AI by pressing the spacebar.
4. Player 2 is always controlled by AI.
5. Try to score points by hitting the ball past the opponent's paddle.
6. The first player to reach a high score wins (optional, based on future enhancements).

## Game Logic
- **AI Behavior:**
  - Both Player 1 and Player 2 AI adjust paddle positions based on the ball's location relative to the paddle's center.
  - The paddle moves up or down at a constant speed (`paddle_speed`).

- **Collision Detection:**
  - Ball collision with paddles or screen boundaries results in direction changes.
  - If the ball passes a paddle, the opposing player scores a point.

- **Scoring System:**
  - Player 1's and Player 2's scores are displayed at the top of the screen.

## Customization
- **AI Toggle:** Use the `spacebar` to switch Player 1 between manual and AI modes.
- **Screen Resolution:** You can set the resolution when launching the game with the `-r` or `-R` option in the format `WidthxHeight`. For example:
  ```bash
  ./pong -r 1280x720
  ```

## Requirements
- **Dependencies:**
  - `libmx2` library
  - SDL2
  - SDL_ttf
- **Assets:**
  - Background image: `data/bg.png`
  - Font: `data/font.ttf`
  - Logo image for intro: `data/logo.png`

## Installation and Execution
1. Ensure all dependencies are installed.
2. Compile the game using your preferred build system (e.g., CMake, make).
3. Run the game:
   ```bash
   ./pong -p /path/to/assets
   ```

## Known Issues
- **AI Paddle Speed:** Both AI players move at a constant speed and may not perform optimally in all scenarios.
- **Intro Screen:** The transition from the intro screen may be slightly abrupt.

## Future Enhancements
- Adjustable difficulty for AI players.
- Multiplayer support for Player 2.
- Enhanced visuals and sound effects.

Enjoy the game!
