#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <Windows.h>
#include <sstream>

// ============================================================================
// GAME STATE STRUCTURE
// ============================================================================

/**
 * GameState structure holds all player progress data
 * This data is saved to file and loaded when resuming a game
 */
struct GameState {
    std::string playerName;        // Player's chosen character name
    std::string gender;            // Player's chosen pronouns (he/him, she/her, they/them)
    int keys;                      // Number of keys collected
    char bearCombination;          // Current bear puzzle combination state
    int keychains;                 // Number of keychains collected
    char roomsUnlocked;            // Tracks which rooms have been unlocked
    int gameType;                  // 1=Horror, 2=Cozy, 3=Healing
    int chapter;                   // Current chapter number
    int menuNumber;                // Current menu position (for complex branching)
    char menuChoice;               // Last menu choice made
    int completions;               // Number of times game has been completed
    std::vector<std::string> memories;  // Collection of story memories/achievements
};

// ============================================================================
// GAME MANAGER CLASS
// ============================================================================

/**
 * GameManager class handles all game state management
 * Responsibilities:
 * - Creating and loading save files
 * - Managing save file registry
 * - Encoding/decoding game state
 * - Starting and running the game story
 */
class GameManager {
private:
    // ------------------------------------------------------------------------
    // MEMBER VARIABLES
    // ------------------------------------------------------------------------
    GameState state;                // Current game state data
    std::string saveFileName;       // Full path to current save file
    std::string saveDirectory;      // Directory where saves are stored ("saves/")
    std::string saveRegistryFile;   // File tracking all save names ("saveFiles.txt")

    // ------------------------------------------------------------------------
    // PRIVATE HELPER FUNCTIONS
    // ------------------------------------------------------------------------

    /**
     * Creates an empty save registry file if none exists
     * Called during initialization to ensure registry is available
     */
    void createEmptyRegistry();

    /**
     * Adds a save file name to the registry
     * Prevents duplicates by checking existing entries first
     *
     * @param saveName - Name of save file to add (without .txt extension)
     */
    void addSaveFileToRegistry(const std::string& saveName);

    /**
     * Removes a save file name from the registry
     * Used when deleting save files
     *
     * @param saveName - Name of save file to remove
     */
    void removeSaveFileFromRegistry(const std::string& saveName);

    /**
     * Encodes current game state into a comma-separated string
     * Format: keys,bearComb,keychains,rooms,type,chapter,menu,choice,completions
     *
     * @return Encoded string representation of game state
     */
    std::string encodeGameState();

    /**
     * Encodes player memories into a pipe-separated string
     * Format: memory1|memory2|memory3
     *
     * @return Encoded string of all memories
     */
    std::string encodeMemories();

    /**
     * Decodes a game state string and populates the state structure
     * Includes error checking for invalid formats
     *
     * @param code - Encoded game state string
     * @return true if decoding successful, false if format invalid
     */
    bool decodeGameState(const std::string& code);

    /**
     * Loads memories from pipe-separated string into state.memories vector
     *
     * @param memoriesLine - Encoded memories string
     */
    void loadMemories(const std::string& memoriesLine);

    /**
     * Checks if a file exists at the given path
     *
     * @param filename - Full or relative path to file
     * @return true if file exists and is accessible
     */
    bool fileExists(const std::string& filename);

public:
    // ------------------------------------------------------------------------
    // CONSTRUCTOR
    // ------------------------------------------------------------------------

    /**
     * GameManager constructor
     * Initializes save directory and registry file
     * Creates directory and registry if they don't exist
     */
    GameManager();

    // ------------------------------------------------------------------------
    // SAVE FILE MANAGEMENT
    // ------------------------------------------------------------------------

    /**
     * Loads all save file names from the registry
     *
     * @return Vector of save file names (without .txt extension)
     */
    std::vector<std::string> loadSaveFileNames();

    /**
     * Displays menu for creating a new game
     * Prompts for save name, character name, and pronouns
     * Prevents duplicate save names
     * Automatically saves and starts the game
     */
    void createNewGameMenu();

    /**
     * Displays menu of available save files for loading
     * Allows player to select a save and resume their game
     * Handles corrupted save files with option to remove
     */
    void loadGameMenu();

    /**
     * Loads a specific save file and populates game state
     *
     * @param saveName - Name of save file to load (without .txt)
     * @return true if load successful, false if file not found or corrupted
     */
    bool loadGame(const std::string& saveName);

    /**
     * Saves current game state to the active save file
     * Encodes all state data and writes to disk
     * Provides feedback messages to player
     */
    void saveGame();

    /**
     * Deletes a save file from disk and removes from registry
     *
     * @param saveName - Name of save file to delete
     */
    void deleteSaveFile(const std::string& saveName);

    /**
     * Lists all available save files with numbering
     * Shows total count of saves
     */
    void listAllSaves();

    /**
     * Main save file management menu
     * Options: List saves, Delete saves, Rebuild registry, Back
     */
    void manageSaveFiles();

    /**
     * Displays menu for selecting and deleting a save file
     * Includes confirmation prompt to prevent accidental deletion
     */
    void deleteSaveMenu();

    /**
     * Scans the saves directory and rebuilds the registry file
     * Useful if registry becomes corrupted or out of sync
     * Finds all .txt files in saves/ folder
     */
    void rebuildRegistry();

    // ------------------------------------------------------------------------
    // GAME FLOW FUNCTIONS
    // ------------------------------------------------------------------------

    /**
     * Displays the intro group chat conversation
     * Shows text messages from friends inviting player to mall
     * Uses typing delays for realistic pacing
     */
    void playIntroConversation();

    /**
     * Main game loop - starts and runs the story
     * Sequence:
     * 1. Plays intro conversation
     * 2. Presents Yes/No choice about going to mall
     * 3. Uses RNG to determine game type (if Yes)
     * 4. Loads appropriate chapter file
     * 5. Runs story system with StoryReader
     */
    void startGame();

    /**
     * Player menu for loaded games
     * TODO: Implement menu for continuing saved games
     * Will include: Continue, Save, Inventory, Exit
     */
    void playerMenu();
};
