# 🐍 Snake Master - Ultimate Edition


### ✨ Features
#### Core Gameplay
- **Classic Snake mechanics** with modern enhancements
- **5 progressive levels** with increasing difficulty
- **Dynamic board sizing** that adapts to your terminal
- **Two game modes**: Walls (borders) or Wrap-around
- **Smooth animations** and responsive controls

#### Special Items
- 🍎 **Normal Apples** (+10 points, +1 segment)
- ⭐ **Golden Apples** (+50 points, +2 segments)
- ☠️ **Poison Apples** (-30 points, -2 segments)

#### Power-Ups
- ⏱️ **Slow Motion** - Temporarily reduces game speed
- 🛡️ **Invincibility** - Pass through obstacles and yourself
- 🧲 **Magnet** - Food gravitates toward you

#### Advanced Features
- **Dynamic obstacles** that move around the board
- **Level-specific obstacle patterns**
- **Progressive speed increase** as you score higher
- **High score system** with persistent storage
- **Game replay** feature to review your performance
- **Customizable controls**
- **Real-time statistics** panel

--

### 🎮 Controls
#### Default Controls
- **W/↑** - Move Up
- **A/←** - Move Left
- **S/↓** - Move Down
- **D/→** - Move Right
- **P** - Pause/Resume
- **Q** - Quit to menu

Controls can be customized in the Settings menu.

--

### 🚀 Getting Started
#### Prerequisites
- C++17 compatible compiler (g++, clang++, or MSVC)
- Terminal with ANSI color support
- UTF-8 character support recommended

#### Compilation

##### Linux/macOS
```bash
g++ -std=c++17 -O2 -pthread snake_master.cpp -o snake_master
```

#### Windows (MinGW)
```bash
g++ -std=c++17 -O2 snake_master.cpp -o snake_master.exe
```

#### Windows (Visual Studio)
```bash
cl /EHsc /std:c++17 /O2 snake_master.cpp
```

#### Running the Game
```bash
# Linux/macOS
./snake_master

# Windows
snake_master.exe
```

--

### 📊 Game Modes

#### Wall Mode (Default)
- Borders are deadly - hitting them ends the game
- Traditional Snake gameplay
- Best for controlled, strategic play

#### Wrap Mode
- Snake wraps around screen edges
- More freedom of movement
- Great for fast-paced action

--

### 🎯 Scoring System

| Item | Score | Effect |
|------|-------|--------|
| Normal Apple | +10 | Grow by 1 segment |
| Golden Apple | +50 | Grow by 2 segments |
| Poison Apple | -30 | Shrink by 2 segments |

#### Level Progression
- **Level 1**: 0-199 points
- **Level 2**: 200-399 points
- **Level 3**: 400-599 points
- **Level 4**: 600-799 points
- **Level 5**: 800+ points

Each level introduces new obstacle patterns and increased difficulty.

--

### 🏆 High Scores

The game automatically saves your top 10 scores to `snake_scores.txt`. High scores track:
- Player name
- Total score
- Snake length
- Level reached

--

### 🎨 Visual Design

The game features:
- **Vibrant ANSI colors** for all game elements
- **Unicode emoji** for enhanced visuals (🐉🍎⭐☠️🛡️)
- **Dynamic UI panel** with real-time statistics
- **Animated intro** and title screen
- **Progress bars** for active power-ups
- **Level-up notifications**

--

### 🔧 Customization

#### Settings Menu
Access via Main Menu → Settings:
1. **Customize Controls** - Remap movement keys
2. **Toggle Borders** - Switch between Wall/Wrap modes
3. **Return to Menu**

--

### 📱 Terminal Compatibility

#### Tested Terminals
- ✅ Windows Terminal
- ✅ Ubuntu GNOME Terminal
- ✅ macOS Terminal.app
- ✅ iTerm2
- ✅ Konsole
- ✅ Alacritty

#### Recommended Settings
- **Minimum size**: 100 columns × 30 rows
- **Font**: Monospace with emoji support
- **Encoding**: UTF-8
- **Color scheme**: Any (game uses explicit ANSI codes)

--

### 🐛 Troubleshooting

#### Display Issues
**Problem**: Characters appear misaligned or corrupted
- Ensure your terminal supports UTF-8 encoding
- Try a terminal with better Unicode support
- Reduce terminal font size

**Problem**: Colors don't appear
- Verify ANSI color support in your terminal
- On Windows, use Windows Terminal instead of cmd.exe

#### Performance Issues
**Problem**: Game runs too slowly
- Close other applications
- Reduce terminal window size
- Compile with optimization flags (`-O2` or `-O3`)

#### Input Problems
**Problem**: Arrow keys don't work
- Use WASD keys instead
- Customize controls in Settings menu
- Check terminal input mode compatibility

--

### 🏗️ Technical Details

#### Architecture
- **Multi-threaded input handling** for responsive controls
- **Frame-rate independent** game logic
- **Adaptive board sizing** based on terminal dimensions
- **Collision detection** with multiple entity types
- **State machine** for game flow management

#### Cross-Platform Support
- Platform-specific implementations for Windows and Unix-like systems
- Conditional compilation for terminal I/O
- Consistent behavior across operating systems

--

### 📝 File Structure

```
snake_master/
├── snake_master.cpp      # Main game source
├── README.md             # This file
└── snake_scores.txt      # Generated high scores file
```

--

### 🌟 Tips for High Scores

1. **Prioritize golden apples** - They're worth 5x normal apples
2. **Use power-ups strategically** - Save invincibility for tight spots
3. **Learn obstacle patterns** - Each level has predictable layouts
4. **Play in wrap mode** - More freedom of movement for higher scores
5. **Avoid poison early** - Losing segments when small is dangerous
6. **Master the magnet** - Pull food toward you for faster collection

--

**Enjoy playing Snake Master! 🐍✨**

*For issues or feedback, check the troubleshooting section or review the source code.*