#!/bin/bash
# MasterPiece Vulkan Game Launcher
cd "$(dirname "$0")"/../buildvk/vk_2D_MasterPiece
DISPLAY=:0 SDL_VIDEODRIVER=x11 ./vk_2D_MasterPiece -p "$(dirname "$0")"
