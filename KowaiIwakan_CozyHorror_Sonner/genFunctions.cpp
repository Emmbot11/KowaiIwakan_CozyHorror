#include "genFunctions.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <conio.h>
#include <Windows.h>

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Define the global atomic boolean for timing system
// Atomic ensures thread-safe access from timer and response functions
std::atomic<bool> timeUp(false);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Clears the console screen
 * Implementation: Uses Windows system command "cls"
 */
void clearScreen() {
    system("cls");
}

/**
 * Simulates a text message in a group chat
 * Shows "[sender is typing...]" then replaces it with the actual message
 * Creates realistic pacing for conversation flow
 */
void displayTextMessage(const std::string& sender, const std::string& message, int typingDelay) {
    // Show typing indicator
    std::cout << "[" << sender << " is typing...]" << std::flush;
    Sleep(typingDelay);  // Simulate typing time

    // Move cursor to beginning of line and clear it
    std::cout << "\r";  // Carriage return - moves cursor to start of line
    std::cout << std::string(sender.length() + 16, ' ');  // Overwrite with spaces
    std::cout << "\r";  // Return to start again

    // Display the actual message
    std::cout << sender << ": " << message << std::endl;
    Sleep(500);  // Brief pause for readability
}

// ============================================================================
// TIMED RESPONSE SYSTEM
// ============================================================================

/**
 * Timer function for threaded timed challenges
 * Waits for specified duration then sets timeUp flag
 * Runs in separate thread to allow simultaneous input checking
 */
void timer(int seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    timeUp = true;  // Signal that time has expired
}

/**
 * Timed response with string input and multiple attempts
 * Creates a separate timer thread while accepting user input
 * Player can try multiple times until correct or time runs out
 */
bool timeResponseThread(const std::string& correctAnswer, int timeLimit) {
    timeUp = false;  // Reset timer flag
    std::thread timerThread(timer, timeLimit);  // Start countdown in separate thread
    std::string userInput;

    std::cout << "You have " << timeLimit << " seconds! Answer: ";

    // Loop until time runs out
    do {
        std::getline(std::cin, userInput);

        // Check if time expired during input
        if (timeUp) break;

        // Check if answer is correct
        if (userInput == correctAnswer) {
            std::cout << "Correct!\n";
            timerThread.join();  // Wait for timer thread to finish
            return true;
        }
        else {
            std::cout << "Wrong! Try again quickly: ";
        }
    } while (!timeUp);

    std::cout << "Time's up!\n";
    timerThread.join();  // Wait for timer thread to finish
    return false;
}

/**
 * Timed response with character-by-character input
 * Uses _kbhit() to detect keypresses without blocking
 * Checks elapsed time each loop iteration for precise timing
 */
bool timedResponse(const std::string& correctAnswer, int timeLimit) {
    auto startTime = std::chrono::steady_clock::now();  // Record start time
    std::string userInput;

    std::cout << "You have " << timeLimit << " second to answer!\n";

    do {
        // Calculate elapsed time
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);

        // Check if time limit exceeded
        if (elapsed.count() >= timeLimit) {
            std::cout << "\nTimes Up!\n";
            return false;
        }

        // Check for keyboard input (non-blocking)
        if (_kbhit()) {
            char ch = _getch();  // Get character without echo

            // Enter key pressed - check answer
            if (ch == '\r') {
                if (userInput == correctAnswer) {
                    std::cout << "\nCorrect!\n";
                    return true;
                }
                else {
                    std::cout << "Wrong! Try again...";
                    userInput.clear();  // Reset input for retry
                }
            }
            else {
                // Add character to input and echo it
                userInput += ch;
                std::cout << ch;
            }
        }
    } while (true);
}

// ============================================================================
// MENU SYSTEM IMPLEMENTATION
// ============================================================================

/**
 * Hides the console cursor for cleaner menu appearance
 * Uses Windows Console API to modify cursor visibility
 */
void MenuSystem::hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = false;  // Set visibility to false
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

/**
 * Shows the console cursor (restores default visibility)
 * Called when exiting menu or accepting text input
 */
void MenuSystem::showCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = true;  // Set visibility to true
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

/**
 * Displays menu with current selection highlighted
 * Selected option is prefixed with " > ", others with "   "
 * Clears screen first for clean display
 */
void MenuSystem::displayMenu(const std::vector<std::string>& options, int selected, const std::string& title) {
    clearScreen();

    // Display title with underline if provided
    if (!title.empty()) {
        std::cout << title << "\n" << std::string(title.length(), '=') << "\n\n";
    }

    // Display each option with selection indicator
    for (int i = 0; i < options.size(); i++) {
        if (i == selected) {
            std::cout << " > " << options[i] << std::endl;  // Highlight selected
        }
        else {
            std::cout << "   " << options[i] << std::endl;  // Normal display
        }
    }

    std::cout << "\nUse arrow keys to navigate, Enter to select, ESC to exit";
}

/**
 * Main menu interaction loop
 * Handles arrow key navigation, Enter selection, ESC exit
 *
 * @return Selected option index (0-based), or -1 for ESC
 */
int MenuSystem::showMenu(const std::vector<std::string>& options, const std::string& title) {
    if (options.empty()) return -1;  // Safety check for empty menu

    hideCursor();  // Hide cursor for cleaner display
    int selected = 0;  // Start at first option
    int key;

    do {
        displayMenu(options, selected, title);

        key = _getch();  // Get keypress

        // Check for arrow keys (preceded by 224)
        if (key == 224) {
            key = _getch();  // Get actual arrow key code
            switch (key) {
            case 72:  // Up arrow
                // Wrap around to bottom if at top
                selected = (selected - 1 + options.size()) % options.size();
                break;
            case 80:  // Down arrow
                // Wrap around to top if at bottom
                selected = (selected + 1) % options.size();
                break;
            }
        }
        else if (key == 13) {  // Enter key
            showCursor();  // Restore cursor
            return selected;
        }
        else if (key == 27) {  // ESC key
            showCursor();  // Restore cursor
            return -1;  // Signal exit/cancel
        }
    } while (true);
}
/**
 * Displays a text message without typing indicator
 * Used when messages should stay visible
 */
void displayTextMessagePersistent(const std::string& sender, const std::string& message) {
    std::cout << sender << ": " << message << std::endl;
}
/**
 * Displays horizontal menu with current selection highlighted
 * Options are displayed side-by-side instead of vertically
 */
 /**
  * Displays horizontal menu - clears and redraws each time
  */
void MenuSystem::displayHorizontalMenu(const std::vector<std::string>& options, int selected, const std::string& title) {
    // Just display the options (title already shown)
    for (int i = 0; i < options.size(); i++) {
        if (i == selected) {
            std::cout << "  > " << options[i] << " <  ";
        }
        else {
            std::cout << "    " << options[i] << "    ";
        }
    }
    std::cout << "\n\nUse LEFT/RIGHT arrow keys to navigate, Enter to select, ESC to exit" << std::endl;
}

int MenuSystem::showHorizontalMenu(const std::vector<std::string>& options, const std::string& title) {
    if (options.empty()) return -1;

    hideCursor();
    int selected = 0;
    int key;

    // Display title once
    if (!title.empty()) {
        std::cout << title << "\n" << std::string(title.length(), '=') << "\n\n";
    }

    // Display initial menu
    displayHorizontalMenu(options, selected, "");

    do {
        key = _getch();

        if (key == 224) {
            key = _getch();
            bool needsRedraw = false;

            switch (key) {
            case 75:  // Left arrow
                selected = (selected - 1 + options.size()) % options.size();
                needsRedraw = true;
                break;
            case 77:  // Right arrow
                selected = (selected + 1) % options.size();
                needsRedraw = true;
                break;
            }

            if (needsRedraw) {
                // Move cursor up 3 lines to redraw menu
                std::cout << "\033[3A";  // ANSI escape: move up 3 lines
                std::cout << "\r";        // Return to start of line

                // Clear the lines
                std::cout << std::string(80, ' ') << "\r\n";
                std::cout << std::string(80, ' ') << "\r\n";
                std::cout << std::string(80, ' ') << "\r";

                // Move back up
                std::cout << "\033[2A";

                // Redraw menu
                displayHorizontalMenu(options, selected, "");
            }
        }
        else if (key == 13) {  // Enter
            showCursor();
            return selected;
        }
        else if (key == 27) {  // ESC
            showCursor();
            return -1;
        }
    } while (true);
}
