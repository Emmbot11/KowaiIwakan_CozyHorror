#pragma once

#include <iostream>
#include <iomanip>
#include <cstring>
#include <string>
#include <vector>
#include <Windows.h>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <conio.h>

using namespace std;

// ============================================================================
// STANDALONE UTILITY FUNCTIONS
// ============================================================================

/**
 * Clears the console screen for better readability
 * Uses system("cls") command for Windows
 */
void clearScreen();

/**
 * Displays a text message with typing simulation effect
 * Used to create realistic group chat conversations
 *
 * @param sender - Name of the person sending the message
 * @param message - The text content of the message
 * @param typingDelay - Milliseconds to wait while "typing" (default: 1000ms)
 */
void displayTextMessage(const std::string& sender, const std::string& message, int typingDelay = 1000);

// ============================================================================
// TIMED RESPONSE SYSTEM
// ============================================================================

// Global atomic boolean to track if time has run out
extern std::atomic<bool> timeUp;

/**
 * Timer function that runs in a separate thread
 * Sets the global timeUp flag to true after specified seconds
 *
 * @param seconds - Number of seconds before time runs out
 */
void timer(int seconds);

/**
 * Timed response challenge using string input
 * Player must enter correct answer before time runs out
 * Allows multiple attempts until time expires
 *
 * @param correctAnswer - The answer the player must type
 * @param timeLimit - Seconds allowed to answer
 * @return true if player answered correctly in time, false otherwise
 */
bool timeResponseThread(const std::string& correctAnswer, int timeLimit);

/**
 * Timed response challenge using character-by-character input
 * Player must type correct answer before time runs out
 * Uses _kbhit() and _getch() for immediate character detection
 *
 * @param correctAnswer - The answer the player must type
 * @param timeLimit - Seconds allowed to answer
 * @return true if player answered correctly in time, false otherwise
 */
bool timedResponse(const std::string& correctAnswer, int timeLimit);

// ============================================================================
// MENU SYSTEM CLASS
// ============================================================================

/**
 * MenuSystem class handles arrow-key navigable menus
 * Provides a consistent interface for all game menus
 * Uses Windows console API for cursor control
 */
class MenuSystem {
private:
    void hideCursor();
    void showCursor();
    void displayMenu(const std::vector<std::string>& options, int selected, const std::string& title);

    // ADD THIS:
    void displayHorizontalMenu(const std::vector<std::string>& options, int selected, const std::string& title);

public:
    int showMenu(const std::vector<std::string>& options, const std::string& title);

    // ADD THIS:
    int showHorizontalMenu(const std::vector<std::string>& options, const std::string& title);
};
/**
 * Displays a text message without the "typing" indicator
 * Used for persistent chat messages
 *
 * @param sender - Name of the person sending the message
 * @param message - The text content of the message
 */
void displayTextMessagePersistent(const std::string& sender, const std::string& message);
