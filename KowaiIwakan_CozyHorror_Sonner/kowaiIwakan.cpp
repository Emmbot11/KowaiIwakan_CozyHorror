#include "genFunctions.h"
#include "GameManager.h" 

/***********************************
* Emma Sonner
*
* Cozy Horror Game - Kowai Iwakan
*
* 5-XX-2026
*
* V. 1.0
*
* PROGRAM DESCRIPTION:
* Interactive text-based adventure game with multiple story paths.
* Features save/load system, dynamic story branching, and
* randomized game type selection (horror, cozy, or healing).
* Player choices affect story outcomes across multiple chapters.
************************************/

int main() {
	// ========================================================================
	// INITIALIZATION
	// ========================================================================

	// Create game management instances
	MenuSystem menu;        // Handles all menu displays and navigation
	GameManager gameManager; // Manages game state, saves, and story flow

	// Define main menu options
	std::vector<std::string> mainOptions = {
		"Start Game", "Load Game", "Settings", "Exit"
	};

	int mainchoice;  // Stores user's menu selection

	// ========================================================================
	// TITLE SCREEN
	// ========================================================================

	// Display game title with decorative border
	cout << "*" << setfill('-') << setw(21) << "*" << setfill(' ') << endl;
	cout << setw(17) << "Kowai Iwakan" << endl;
	cout << "*" << setfill('-') << setw(21) << "*" << setfill(' ') << endl;
	Sleep(2000);  // Brief pause for effect
	clearScreen();

	// ========================================================================
	// TRIGGER WARNING SCREEN
	// ========================================================================

	// Display content warnings for player safety
	cout << "<" << setfill('-') << setw(21) << ">" << setfill(' ') << endl;
	cout << setw(20) << "!!TRIGGER WARNING!!" << endl;
	cout << "<" << setfill('-') << setw(21) << ">" << setfill(' ') << endl;
	cout << setw(5) << ">" << " Discriptions of Gore" << endl;
	cout << setw(5) << ">" << " Violence" << endl;
	cout << setw(5) << ">" << " Death" << endl;
	cout << setw(5) << ">" << " Fear" << endl;
	cout << setw(5) << ">" << " Psycological Manipulation/Torture" << endl;
	cout << setw(5) << ">" << " Sensative topics (Kidnapping, Homocide, Arson, Mental Instability)" << endl;
	Sleep(4500);  // Give player time to read warning
	clearScreen();

	// ========================================================================
	// MAIN MENU LOOP
	// ========================================================================

	// Continue showing menu until player chooses Exit or ESC
	do {
		mainchoice = menu.showMenu(mainOptions, "Main Menu");

		switch (mainchoice) {
		case 0:	// START NEW GAME
			clearScreen();
			gameManager.createNewGameMenu();  // Create new save and start story
			break;
		case 1:	// LOAD EXISTING GAME
			clearScreen();
			gameManager.loadGameMenu();  // Display saved games and load selection
			break;
		case 2:	// SETTINGS MENU
			clearScreen();
			{
				std::vector<std::string> settingsOptions = {
					"Manage Save Files",
					"Audio Settings (Coming Soon)",
					"Display Settings (Coming Soon)",
					"Back to Main Menu"
				};

				int settingsChoice = menu.showMenu(settingsOptions, "Settings");

				if (settingsChoice == 0) {
					// Manage Save Files
					gameManager.manageSaveFiles();
				}
				else if (settingsChoice == 1 || settingsChoice == 2) {
					// Coming soon features
					clearScreen();
					cout << "\nFeature coming soon!" << endl;
					Sleep(1500);
				}
				// settingsChoice == 3 or -1 (ESC) will just return to main menu
			}
			break;

		case 3:
			clearScreen();
			cout << "\n================================" << endl;
			cout << "   Thank you for playing!" << endl;
			cout << "================================\n" << endl;
			break;
		case -1:	// ESC KEY PRESSED
			clearScreen();
			cout << "\nExiting game...\n" << endl;
			break;
		default:	// INVALID SELECTION (Safety net)
			cout << "Invalid option!" << endl;
			Sleep(1000);
			break;
		}

	} while (mainchoice != 3 && mainchoice != -1);  // Exit loop on Exit or ESC
	// PROGRAM EXIT

	// Display final goodbye message
	cout << endl;
	cout << "See you next time!" << endl;
	cout << endl;

	// Wait for user acknowledgment before closing window
	// This prevents console from closing immediately
	cin.ignore(100, '\n');  // Clear any remaining input
	cout << "Press Enter to exit...";
	cin.get();  // Wait for Enter key

	return 0;  // Successful program termination
}
