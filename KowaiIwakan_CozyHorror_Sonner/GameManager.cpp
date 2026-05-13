#include "GameManager.h"
#include "StoryReader.h"
#include "genFunctions.h"

// Helper function to check if file exists
bool GameManager::fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

// Constructor
GameManager::GameManager()
    : saveDirectory("saves/"), saveRegistryFile("saveFiles.txt") {
    CreateDirectoryA(saveDirectory.c_str(), NULL);

    if (!fileExists(saveRegistryFile)) {
        createEmptyRegistry();
    }
}

void GameManager::createEmptyRegistry() {
    std::ofstream file(saveRegistryFile);
    file.close();
    std::cout << "Created new save registry file." << std::endl;
}

std::vector<std::string> GameManager::loadSaveFileNames() {
    std::vector<std::string> saveNames;
    std::ifstream file(saveRegistryFile);

    if (file.is_open()) {
        std::string saveName;
        while (std::getline(file, saveName)) {
            if (!saveName.empty()) {
                saveNames.push_back(saveName);
            }
        }
        file.close();
    }

    return saveNames;
}

void GameManager::addSaveFileToRegistry(const std::string& saveName) {
    std::vector<std::string> existingNames = loadSaveFileNames();

    for (const auto& name : existingNames) {
        if (name == saveName) {
            return;
        }
    }

    std::ofstream file(saveRegistryFile, std::ios::app);
    file << saveName << std::endl;
    file.close();

    std::cout << "Added '" << saveName << "' to save registry." << std::endl;
}

void GameManager::removeSaveFileFromRegistry(const std::string& saveName) {
    std::vector<std::string> saveNames = loadSaveFileNames();

    saveNames.erase(std::remove(saveNames.begin(), saveNames.end(), saveName), saveNames.end());

    std::ofstream file(saveRegistryFile);
    for (const auto& name : saveNames) {
        file << name << std::endl;
    }
    file.close();

    std::cout << "Removed '" << saveName << "' from save registry." << std::endl;
}

void GameManager::createNewGameMenu() {
    std::string saveName;
    bool validName = false;

    std::vector<std::string> existingSaves = loadSaveFileNames();

    while (!validName) {
        clearScreen();
        std::cout << "\n=== CREATE NEW GAME ===" << std::endl;
        std::cout << "Enter save file name (or type 'back' to return): ";
        std::cin >> saveName;

        if (saveName == "back") {
            return;
        }

        bool nameExists = false;
        for (const auto& existingName : existingSaves) {
            if (existingName == saveName) {
                nameExists = true;
                break;
            }
        }

        if (nameExists) {
            std::cout << "\nSave file '" << saveName << "' already exists!" << std::endl;
            std::cout << "Choose a different name or type 'back' to return." << std::endl;
            Sleep(2000);
        }
        else {
            validName = true;
        }
    }

    saveFileName = saveDirectory + saveName + ".txt";

    clearScreen();
    std::cout << "\n=== CHARACTER CREATION ===" << std::endl;
    std::cout << "Enter your character name: ";
    std::cin.ignore(100, '\n');  // Clear input buffer
    std::getline(std::cin, state.playerName);

    // Gender selection with horizontal menu
    clearScreen();
    std::cout << "\n=== CHARACTER CREATION ===" << std::endl;
    std::cout << "Character name: " << state.playerName << "\n" << std::endl;

    std::vector<std::string> genderOptions = {
        "he/him",
        "she/her",
        "they/them"
    };

    MenuSystem menu;
    int genderChoice = menu.showHorizontalMenu(genderOptions, "Select your pronouns:");

    // Set gender based on choice
    if (genderChoice == 0) {
        state.gender = "he/him";
    }
    else if (genderChoice == 1) {
        state.gender = "she/her";
    }
    else if (genderChoice == 2) {
        state.gender = "they/them";
    }
    else {
        // ESC pressed - default to they/them
        state.gender = "they/them";
    }

    // Initialize game state
    state.keys = 0;
    state.bearCombination = 'a';
    state.keychains = 0;
    state.roomsUnlocked = 'a';
    state.gameType = 2;
    state.chapter = 1;
    state.menuNumber = 1;
    state.menuChoice = 'a';
    state.completions = 0;
    state.memories.clear();

    saveGame();
    addSaveFileToRegistry(saveName);

    clearScreen();
    std::cout << "\n=== CHARACTER CREATED ===" << std::endl;
    std::cout << "Name: " << state.playerName << std::endl;
    std::cout << "Pronouns: " << state.gender << std::endl;
    std::cout << "Save file: " << saveName << std::endl;
    Sleep(2000);

    startGame();
}


void GameManager::loadGameMenu() {
    std::vector<std::string> saveNames = loadSaveFileNames();

    if (saveNames.empty()) {
        std::cout << "No save files found!" << std::endl;
        Sleep(1500);
        return;
    }

    // Create menu options from save names
    std::vector<std::string> menuOptions;
    for (const auto& saveName : saveNames) {
        menuOptions.push_back(saveName);
    }
    menuOptions.push_back("Back to main menu");

    MenuSystem menu;
    int choice = menu.showMenu(menuOptions, "=== AVAILABLE SAVE FILES ===");

    // Check if user selected a save file (not "Back")
    if (choice >= 0 && choice < static_cast<int>(saveNames.size())) {
        std::string selectedSave = saveNames[choice];

        if (loadGame(selectedSave)) {
            clearScreen();
            std::cout << "Successfully loaded: " << selectedSave << std::endl;
            Sleep(1500);
            playerMenu();
        }
        else {
            clearScreen();
            std::cout << "Error loading save file: " << selectedSave << std::endl;

            std::vector<std::string> removeOptions = {
                "Yes - Remove corrupted save",
                "No - Keep it"
            };

            int removeChoice = menu.showMenu(removeOptions, "Remove corrupted save from list?");

            if (removeChoice == 0) {
                removeSaveFileFromRegistry(selectedSave);
                DeleteFileA((saveDirectory + selectedSave + ".txt").c_str());
                std::cout << "\nSave file removed." << std::endl;
                Sleep(1500);
            }
        }
    }
    // choice == saveNames.size() means "Back" was selected
    // choice == -1 means ESC was pressed
    // Both cases just return to main menu
}


bool GameManager::loadGame(const std::string& saveName) {
    saveFileName = saveDirectory + saveName + ".txt";
    std::ifstream file(saveFileName);

    if (!file.is_open()) {
        std::cout << "Save file not found: " << saveName << std::endl;
        return false;
    }

    std::getline(file, state.playerName);
    std::getline(file, state.gender);

    std::string gameStateCode;
    std::getline(file, gameStateCode);

    if (!decodeGameState(gameStateCode)) {
        std::cout << "Error: Invalid save file format!" << std::endl;
        return false;
    }

    std::string memoriesLine;
    if (std::getline(file, memoriesLine) && !memoriesLine.empty()) {
        loadMemories(memoriesLine);
    }

    file.close();
    return true;
}

void GameManager::deleteSaveFile(const std::string& saveName) {
    removeSaveFileFromRegistry(saveName);

    std::string fullPath = saveDirectory + saveName + ".txt";
    if (DeleteFileA(fullPath.c_str())) {
        std::cout << "Save file '" << saveName << "' deleted successfully." << std::endl;
    }
    else {
        std::cout << "Error deleting save file: " << saveName << std::endl;
    }
}

void GameManager::listAllSaves() {
    std::vector<std::string> saveNames = loadSaveFileNames();

    std::cout << "\n=== ALL SAVE FILES ===" << std::endl;
    if (saveNames.empty()) {
        std::cout << "No save files found." << std::endl;
    }
    else {
        for (size_t i = 0; i < saveNames.size(); i++) {
            std::cout << i + 1 << ". " << saveNames[i] << std::endl;
        }
    }
    std::cout << "Total: " << saveNames.size() << " save files" << std::endl;
}

void GameManager::manageSaveFiles() {
    MenuSystem menu;
    int choice;

    do {
        std::vector<std::string> manageOptions = {
            "List All Saves",
            "Delete Save File",
            "Rebuild Registry (scan saves folder)",
            "Back"
        };

        choice = menu.showMenu(manageOptions, "=== SAVE FILE MANAGEMENT ===");

        switch (choice) {
        case 0:
            clearScreen();
            listAllSaves();
            std::cout << "\nPress Enter to continue...";
            std::cin.ignore(100, '\n');
            std::cin.get();
            break;
        case 1:
            deleteSaveMenu();
            break;
        case 2:
            clearScreen();
            rebuildRegistry();
            std::cout << "\nPress Enter to continue...";
            std::cin.ignore(100, '\n');
            std::cin.get();
            break;
        case 3:
            return;  // Back selected
        case -1:
            return;  // ESC pressed
        default:
            break;
        }
    } while (choice != 3 && choice != -1);
}


void GameManager::deleteSaveMenu() {
    std::vector<std::string> saveNames = loadSaveFileNames();

    if (saveNames.empty()) {
        clearScreen();
        std::cout << "No save files to delete." << std::endl;
        Sleep(1500);
        return;
    }

    // Create menu options from save names
    std::vector<std::string> menuOptions;
    for (const auto& saveName : saveNames) {
        menuOptions.push_back(saveName);
    }
    menuOptions.push_back("Cancel");

    MenuSystem menu;
    int choice = menu.showMenu(menuOptions, "=== DELETE SAVE FILE ===");

    // Check if user selected a save file (not "Cancel")
    if (choice >= 0 && choice < static_cast<int>(saveNames.size())) {
        std::string saveToDelete = saveNames[choice];

        clearScreen();
        std::cout << "\n=== CONFIRM DELETION ===" << std::endl;
        std::cout << "Save file: " << saveToDelete << "\n" << std::endl;

        std::vector<std::string> confirmOptions = {
            "Yes - Delete this save",
            "No - Keep it"
        };

        int confirm = menu.showMenu(confirmOptions, "Are you sure?");

        if (confirm == 0) {
            deleteSaveFile(saveToDelete);
            Sleep(1500);
        }
    }
    // choice == saveNames.size() means "Cancel" was selected
    // choice == -1 means ESC was pressed
    // Both cases just return
}


void GameManager::rebuildRegistry() {
    std::vector<std::string> foundSaves;

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((saveDirectory + "*.txt").c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string filename = findData.cFileName;
            // Remove .txt extension
            if (filename.length() > 4) {
                filename = filename.substr(0, filename.length() - 4);
                foundSaves.push_back(filename);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    std::ofstream file(saveRegistryFile);
    for (const auto& saveName : foundSaves) {
        file << saveName << std::endl;
    }
    file.close();

    std::cout << "Registry rebuilt. Found " << foundSaves.size() << " save files." << std::endl;
}

std::string GameManager::encodeMemories() {
    std::string encoded = "";

    for (size_t i = 0; i < state.memories.size(); i++) {
        encoded += state.memories[i];
        if (i < state.memories.size() - 1) {
            encoded += "|";  // Use | as separator
        }
    }

    return encoded;
}

void GameManager::loadMemories(const std::string& memoriesLine) {
    state.memories.clear();

    std::stringstream ss(memoriesLine);
    std::string memory;

    while (std::getline(ss, memory, '|')) {
        if (!memory.empty()) {
            state.memories.push_back(memory);
        }
    }
}

std::string GameManager::encodeGameState() {
    std::string encoded = "";

    encoded += std::to_string(state.keys) + ",";
    encoded += state.bearCombination;
    encoded += ",";
    encoded += std::to_string(state.keychains) + ",";
    encoded += state.roomsUnlocked;
    encoded += ",";
    encoded += std::to_string(state.gameType) + ",";
    encoded += std::to_string(state.chapter) + ",";
    encoded += std::to_string(state.menuNumber) + ",";
    encoded += state.menuChoice;
    encoded += ",";
    encoded += std::to_string(state.completions);

    return encoded;
}

void GameManager::saveGame() {
    std::cout << "Saving game..." << std::endl;

    std::ofstream file(saveFileName);

    if (!file.is_open()) {
        std::cout << "Error: Could not open save file!" << std::endl;
        return;
    }

    // Save player name and gender
    file << state.playerName << std::endl;
    file << state.gender << std::endl;

    // Encode and save game state
    std::string gameStateCode = encodeGameState();
    file << gameStateCode << std::endl;

    // Save memories
    std::string memoriesLine = encodeMemories();
    file << memoriesLine << std::endl;

    file.close();
    std::cout << "Game saved successfully!" << std::endl;
}


void GameManager::startGame() {
    // Display the intro conversation with typing indicators
    clearScreen();

    std::cout << "================================" << std::endl;
    std::cout << "      GROUP CHAT: Squad 🎮      " << std::endl;
    std::cout << "================================\n" << std::endl;
    Sleep(1000);

    // Show messages with typing delays that get replaced
    displayTextMessage("Yuki", "Hey everyone! Who wants to hit the mall today?", 1500);
    displayTextMessage("Kai", "I'm down! Need to grab some new headphones anyway", 1200);
    displayTextMessage("Mira", "Ooh yes! I heard they opened a new arcade there!", 1800);
    displayTextMessage("Yuki", state.playerName + ", you coming? 👀", 1000);
    displayTextMessage("Kai", "Come on " + state.playerName + "! It'll be fun!", 1300);
    displayTextMessage("Mira", "Pleeeease come with us! 🙏", 1000);

    Sleep(1000);

    // Add separator before choice
    std::cout << "\n================================\n" << std::endl;

    // Present the choice with horizontal menu (conversation stays visible above)
    std::vector<std::string> choiceOptions = {
        "Yes",
        "No"
    };

    MenuSystem menu;
    int choice = menu.showHorizontalMenu(choiceOptions, "Go to the mall with your friends?");

    std::string chapterToLoad = "";

    if (choice == 0) {  // YES - Go to the mall
        clearScreen();
        std::cout << "\n[You decide to go to the mall with your friends...]" << std::endl;
        Sleep(2000);

        // Random number generator (1-5)
        srand(static_cast<unsigned int>(time(0)));
        int randomNum = (rand() % 5) + 1;

        std::cout << "\n[Loading your story...]" << std::endl;
        Sleep(1500);

        // Determine game type based on random number
        if (randomNum == 2 || randomNum == 4) {
            // HORROR GAME
            state.gameType = 1;
            chapterToLoad = "chapter1H";
            std::cout << "[The mall feels... different today...]" << std::endl;
        }
        else {  // randomNum == 1, 3, or 5
            // COZY GAME
            state.gameType = 2;
            chapterToLoad = "chapter1C";
            std::cout << "[Time for a fun day at the mall!]" << std::endl;
        }

    }
    else if (choice == 1) {  // NO - Stay home
        clearScreen();
        std::cout << "\n[You decide to stay home and relax...]" << std::endl;
        Sleep(2000);

        // HEALING GAME
        state.gameType = 3;
        chapterToLoad = "chapter1";
        std::cout << "[Sometimes the best days are quiet ones...]" << std::endl;

    }
    else {  // ESC or invalid
        std::cout << "\n[Returning to main menu...]" << std::endl;
        Sleep(1500);
        return;
    }

    Sleep(1500);
    clearScreen();

    // Save the game state before starting
    saveGame();

    // Initialize StoryReader
    StoryReader reader("story/");
    reader.setPlayerName(state.playerName);
    reader.setPlayerGender(state.gender);

    std::string currentChapter = chapterToLoad;
    std::string currentSectionID = "1";

    // Load initial chapter
    if (!reader.loadChapterFile(currentChapter)) {
        std::cout << "Failed to load chapter: " << currentChapter << std::endl;
        Sleep(2000);
        return;
    }

    // [REST OF YOUR EXISTING GAME LOOP CODE STAYS THE SAME]
    // Continue with the while loop you already have...

    while (true) {
        StorySection section;

        if (!reader.getSection(currentSectionID, section)) {
            std::cout << "Error: Section [" << currentSectionID << "] not found!" << std::endl;
            Sleep(2000);
            break;
        }

        // Display the section
        reader.displaySection(section);

        // Check if section adds an item
        if (!section.itemToAdd.empty()) {
            reader.addItem(section.itemToAdd);
        }

        // Check if section loads a new chapter
        if (!section.loadChapter.empty()) {
            // Check condition if exists
            if (!section.loadCondition.empty() && !reader.hasItem(section.loadCondition)) {
                std::cout << "\n[You need " << section.loadCondition << " to proceed!]" << std::endl;
                Sleep(2000);
                break;
            }

            // Load new chapter
            std::cout << "\n[Loading " << section.loadChapter << "...]" << std::endl;
            Sleep(1500);

            if (reader.loadChapterFile(section.loadChapter)) {
                currentChapter = section.loadChapter;
                currentSectionID = section.loadStartSection.empty() ? "1" : section.loadStartSection;
                continue;
            }
            else {
                std::cout << "Error loading chapter!" << std::endl;
                break;
            }
        }

        if (section.isMenu) {
            if (section.hasEvent) {
                int choice = reader.showSectionMenu(section);

                if (choice == -1 || choice >= section.options.size()) {
                    std::cout << "Exiting story..." << std::endl;
                    break;
                }

                if (section.options[choice].nextSection == "EVENT") {
                    currentSectionID = reader.handleEvent(section.event);
                }
                else {
                    MenuOption selectedOption = section.options[choice];

                    if (!selectedOption.loadChapter.empty()) {
                        if (!selectedOption.loadCondition.empty() && !reader.hasItem(selectedOption.loadCondition)) {
                            std::cout << "\n[You need " << selectedOption.loadCondition << " to do that!]" << std::endl;
                            Sleep(2000);
                            continue;
                        }

                        std::cout << "\n[Loading " << selectedOption.loadChapter << "...]" << std::endl;
                        Sleep(1500);

                        if (reader.loadChapterFile(selectedOption.loadChapter)) {
                            currentChapter = selectedOption.loadChapter;
                            currentSectionID = selectedOption.loadStartSection.empty() ? "1" : selectedOption.loadStartSection;
                        }
                        else {
                            std::cout << "Error loading chapter!" << std::endl;
                            break;
                        }
                    }
                    else {
                        currentSectionID = selectedOption.nextSection;
                    }
                }
            }
            else {
                int choice = reader.showSectionMenu(section);

                if (choice == -1 || choice >= section.options.size()) {
                    std::cout << "Exiting story..." << std::endl;
                    break;
                }

                MenuOption selectedOption = section.options[choice];

                if (!selectedOption.loadChapter.empty()) {
                    if (!selectedOption.loadCondition.empty() && !reader.hasItem(selectedOption.loadCondition)) {
                        std::cout << "\n[You need " << selectedOption.loadCondition << " to do that!]" << std::endl;
                        Sleep(2000);
                        continue;
                    }

                    std::cout << "\n[Loading " << selectedOption.loadChapter << "...]" << std::endl;
                    Sleep(1500);

                    if (reader.loadChapterFile(selectedOption.loadChapter)) {
                        currentChapter = selectedOption.loadChapter;
                        currentSectionID = selectedOption.loadStartSection.empty() ? "1" : selectedOption.loadStartSection;
                    }
                    else {
                        std::cout << "Error loading chapter!" << std::endl;
                        break;
                    }
                }
                else {
                    currentSectionID = selectedOption.nextSection;
                }
            }
        }
        else {
            if (!section.nextSection.empty()) {
                currentSectionID = section.nextSection;
            }
            else {
                std::cout << "\n[Press Enter to continue...]" << std::endl;
                std::cin.ignore(100, '\n');
                std::cin.get();
                break;
            }
        }

        if (currentSectionID == "END" || currentSectionID == "GAME_OVER") {
            std::cout << "\n=== The End ===" << std::endl;
            Sleep(2000);
            break;
        }
    }
}

void GameManager::playerMenu() {
    std::cout << "Player menu..." << std::endl;
    // TODO: Implement player menu for loaded games
}

bool GameManager::decodeGameState(const std::string& code) {
    std::stringstream ss(code);
    std::string token;
    std::vector<std::string> parts;

    // Split by comma
    while (std::getline(ss, token, ',')) {
        parts.push_back(token);
    }

    // Check if we have the right number of parts
    if (parts.size() != 9) {
        std::cout << "Error: Expected 9 parts, got " << parts.size() << std::endl;
        return false;
    }

    try {
        state.keys = std::stoi(parts[0]);

        // Safety check for character fields
        if (parts[1].empty()) {
            std::cout << "Error: bearCombination is empty!" << std::endl;
            return false;
        }
        state.bearCombination = parts[1][0];

        state.keychains = std::stoi(parts[2]);

        if (parts[3].empty()) {
            std::cout << "Error: roomsUnlocked is empty!" << std::endl;
            return false;
        }
        state.roomsUnlocked = parts[3][0];

        state.gameType = std::stoi(parts[4]);
        state.chapter = std::stoi(parts[5]);
        state.menuNumber = std::stoi(parts[6]);

        if (parts[7].empty()) {
            std::cout << "Error: menuChoice is empty!" << std::endl;
            return false;
        }
        state.menuChoice = parts[7][0];

        state.completions = std::stoi(parts[8]);

        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error decoding game state: " << e.what() << std::endl;
        return false;
    }
}

void GameManager::playIntroConversation() {
    clearScreen();

    std::cout << "================================" << std::endl;
    std::cout << "      GROUP CHAT: Squad 🎮      " << std::endl;
    std::cout << "================================\n" << std::endl;
    Sleep(1000);

    // Simulate group chat conversation with typing delays
    displayTextMessage("Yuki", "Hey everyone! Who wants to hit the mall today?", 1500);
    displayTextMessage("Kai", "I'm down! Need to grab some new headphones anyway", 1200);
    displayTextMessage("Mira", "Ooh yes! I heard they opened a new arcade there!", 1800);
    displayTextMessage("Yuki", state.playerName + ", you coming? 👀", 1000);
    displayTextMessage("Kai", "Come on " + state.playerName + "! It'll be fun!", 1300);
    displayTextMessage("Mira", "Pleeeease come with us! 🙏", 1000);

    Sleep(500);
}
