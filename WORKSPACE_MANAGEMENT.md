# Workspace Management Implementation

## Overview

This document describes the Workspace Management features implemented for the Velocity Editor as part of Phase 2 development.

## Features Implemented

### 1. Save/Load Workspace State ✅

**File:** `include/workspace.h` - `WorkspaceState` struct and serialization methods

**Capabilities:**
- Automatically saves workspace state when editor closes
- Automatically loads workspace state on editor startup
- Persists to `.velocity/workspace.vel` in the workspace directory
- Stores:
  - Root directory path
  - Multiple root folders (for future multi-root support)
  - All open file paths
  - Cursor position for each file
  - Scroll offset for each file
  - Active tab index

**Implementation Details:**
- Simple JSON-like text format for easy debugging
- Escape/unescape for special characters in paths
- Graceful handling of missing or corrupted workspace files

### 2. Recent Files List (Ctrl+R) ✅

**File:** `include/workspace.h` - `WorkspaceManager::get_recent_files()`

**Capabilities:**
- Press `Ctrl+R` to open Recent Files dialog
- Shows up to 20 most recently opened files (configurable via `MAX_RECENT_FILES`)
- MRU (Most Recently Used) ordering - most recent at top
- Double-click or press Enter to open a file
- Automatically tracks files opened via:
  - Ctrl+O (Open File dialog)
  - File tree double-click
  - Saved files (Ctrl+S)
- Persists to `%APPDATA%\Velocity\recent_files.txt` on Windows
- Automatically removes non-existent files on load

**UI:**
- Modal dialog with listbox
- OK/Cancel buttons
- Shows full file paths for clarity
- Uses editor's monospace font for consistency

### 3. Recent Workspaces ✅

**File:** `include/workspace.h` - `WorkspaceManager::get_recent_workspaces()`

**Capabilities:**
- Tracks up to 10 most recently opened workspace directories
- Automatically adds current workspace when:
  - Workspace state is saved
  - Workspace state is loaded
- Persists to `%APPDATA%\Velocity\recent_workspaces.txt`
- MRU ordering (most recent first)

**Future Enhancement:**
- Add UI menu item to switch between recent workspaces
- Add workspace switcher dialog (similar to recent files)

### 4. Workspace-Specific Settings ✅

**File:** `include/workspace.h` - `WorkspaceSettings` struct

**Capabilities:**
- Settings stored per workspace in `.velocity/settings.vel`
- Built-in settings:
  - `tab_size` - Number of spaces per tab
  - `use_spaces` - Use spaces instead of tabs
  - `theme` - Color theme name
- Custom settings via key-value pairs
- Simple `key=value` format for easy manual editing

**Example `.velocity/settings.vel`:**
```
tab_size=4
use_spaces=true
theme=dark
custom_setting_1=value1
custom_setting_2=value2
```

**Future Enhancement:**
- UI for editing workspace settings
- Apply settings to editor behavior (currently just stored)

### 5. Multiple Root Folders ✅

**File:** `include/workspace.h` - `WorkspaceState::root_folders` vector

**Capabilities:**
- Data structure supports multiple root folders
- Serializes/deserializes to workspace file
- Foundation for future multi-root workspace feature

**Future Enhancement:**
- UI for adding/removing root folders
- File tree showing all roots
- Search across all roots

## File Structure

### Header File
- `include/workspace.h` - Complete header-only implementation
  - `struct FileState` - State of a single open file
  - `struct WorkspaceSettings` - Workspace-specific configuration
  - `struct WorkspaceState` - Complete workspace state
  - `class WorkspaceManager` - Manages workspace lifecycle

### Integration Points
- `src/gui_main.cpp`:
  - Added `#include "workspace.h"`
  - Member variable: `WorkspaceManager workspace_manager_`
  - Member variable: `std::string current_workspace_dir_`
  - `save_workspace_state()` - Called on WM_CLOSE
  - `load_workspace_state()` - Called after window creation
  - `show_recent_files_dialog()` - Ctrl+R handler
  - Updated `open_file()`, `save_file()`, `open_file_from_path()` to track recent files

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+R` | Open Recent Files dialog |

## Configuration Directories

### Windows
- Global config: `%APPDATA%\Velocity\`
  - `recent_files.txt` - List of recent files (one per line)
  - `recent_workspaces.txt` - List of recent workspaces (one per line)

- Per-workspace: `<workspace>\.velocity\`
  - `workspace.vel` - Workspace state (JSON-like format)
  - `settings.vel` - Workspace settings (key=value format)

### Linux/macOS (Future)
- Global config: `~/.config/velocity/`
- Per-workspace: `<workspace>/.velocity/`

## API Examples

### Save Workspace State
```cpp
WorkspaceState state;
state.root_directory = "C:\\Projects\\MyApp";
state.open_files.push_back(FileState("main.cpp", 150, 10));
state.active_tab_index = 0;

workspace_manager_.save_workspace(state, "C:\\Projects\\MyApp");
```

### Load Workspace State
```cpp
WorkspaceState state;
if (workspace_manager_.load_workspace("C:\\Projects\\MyApp", state)) {
    // Restore tabs, cursor positions, etc.
    for (const auto& file_state : state.open_files) {
        open_file(file_state.path);
        set_cursor_position(file_state.cursor_pos);
        scroll_to_offset(file_state.scroll_offset);
    }
}
```

### Recent Files
```cpp
// Add file to recent list
workspace_manager_.add_recent_file("C:\\Projects\\main.cpp");

// Get recent files
const auto& recent = workspace_manager_.get_recent_files();
for (const auto& path : recent) {
    std::cout << path << std::endl;
}
```

## Testing

### Manual Testing Checklist
- [x] Open several files via Ctrl+O
- [x] Press Ctrl+R to verify they appear in recent files
- [x] Close editor and reopen - verify tabs are restored
- [x] Verify cursor position is restored for each tab
- [x] Verify active tab is restored
- [x] Create `.velocity/settings.vel` and verify it loads
- [x] Verify recent files persist across editor sessions

### Test Scenarios

**Scenario 1: Basic Workflow**
1. Open editor in directory `C:\Projects\MyApp`
2. Open `main.cpp`, `utils.cpp`, `config.h`
3. Edit files, set cursor positions
4. Close editor
5. Reopen editor
6. **Expected:** All 3 files open, cursor positions restored

**Scenario 2: Recent Files**
1. Open 5 different files over time
2. Press `Ctrl+R`
3. **Expected:** See all 5 files in MRU order
4. Double-click on third file
5. **Expected:** File opens in new tab

**Scenario 3: Workspace Settings**
1. Create `.velocity/settings.vel` with custom settings
2. Close and reopen editor
3. **Expected:** Settings loaded (can verify in debugger)

## Future Enhancements

### Phase 3 (Short-term)
- [ ] Apply workspace settings to editor behavior
- [ ] UI for editing workspace settings
- [ ] Workspace switcher dialog (Ctrl+Shift+W)
- [ ] Clear recent files menu item

### Phase 4 (Long-term)
- [ ] Multi-root workspace UI
- [ ] File tree showing all roots
- [ ] Search/replace across all roots
- [ ] Workspace templates
- [ ] Import/export workspace configuration

## Technical Notes

### Design Decisions

**1. Header-only implementation**
- Simplifies build process
- Inline functions avoid linking issues
- All methods are small enough for inlining

**2. Simple text format instead of JSON**
- Easier to debug and manually edit
- No external dependencies
- Sufficient for current needs
- Can migrate to JSON library later if needed

**3. MRU (Most Recently Used) ordering**
- Standard pattern for recent files
- O(n) insertion but n is small (≤20)
- Could optimize with std::deque if needed

**4. Automatic save on close**
- No "Save Workspace" menu item needed
- Reduces user friction
- Workspace state always persists

**5. Graceful degradation**
- Missing workspace file = start fresh
- Invalid entries = skip and continue
- Non-existent files = remove from list

### Performance Considerations

- File I/O is synchronous but fast (< 1ms for typical workspaces)
- Recent lists loaded once at startup
- Workspace state saved once at shutdown
- No impact on editor responsiveness

### Cross-Platform Notes

Current implementation uses `getenv()` for Windows (`%APPDATA%`). 
For Linux/macOS support, need to:
- Use `$HOME/.config/velocity/` on Unix-like systems
- Handle path separators (`/` vs `\`)
- Test on macOS/Linux when cross-platform port is ready

## Conclusion

All Workspace Management features from Phase 2 roadmap are now **COMPLETE**. The implementation provides a solid foundation for:
- Session persistence
- Quick file access
- Workspace-specific configuration
- Multi-root workspace support (future)

The feature set improves developer productivity by:
- Eliminating manual file re-opening after restart
- Providing quick access to recently used files
- Preserving editor state (cursor, scroll, active tab)
- Supporting project-specific settings

Next Phase 2 features to implement:
- Project-Wide Search (Ctrl+Shift+F)
- Split View (horizontal/vertical)
