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
    //hideCursor();
    clearScreen();
    Sleep(1000);
    cout << "Loading ";
    Sleep(1000);
    cout << ". ";
    Sleep(1000);
    cout << ". ";
    Sleep(1000);
    cout << ". " << endl;
    Sleep(3000);
    cout << endl;
    cout << "Have fun~ (^*u*^)" << endl;
    Sleep(1500);
    //showCursor();
    // Display the intro conversation with typing indicators
    clearScreen();

    std::cout << "=====================================" << std::endl;
    std::cout << "      CHAT: Emotionally UnStaBlE      " << std::endl;
    std::cout << "=====================================\n" << std::endl;
    Sleep(1000);
    //ADD IN TEXTS FROM COMPLEATLY RANDOM CONVERSATION!!!!!!!!
    
    // Show messages with typing delays that get replaced
    displayTextMessage("Maya", "ANNIVERSARY PLANNNSSSS!!!!!", 1500);
    displayTextMessage("Jordan", "Why are we yelling at 6am?", 1200);
    displayTextMessage("Maya", "ABANDONED MALL TRIP~", 1800);
    displayTextMessage("Noah", "That is just trespasing with extra Asbestos.", 1150);
    displayTextMessage("Sophie", "Like the old mall where we all met?", 1300);
    displayTextMessage("Maya", "HECK YAH!!", 1000);
    displayTextMessage("Jordan", "But why there? They have been condemned for demolition for years.", 1400);
    displayTextMessage("Maya", "Because! mEmORieS~~!!", 1100);
    displayTextMessage("Sopie", "Oh! Don't they still have the one event set up that they had prepared before the building was evacuated?", 1600);
    displayTextMessage("Maya", "Yuppp~~~", 1250);
    displayTextMessage("Eli", state.playerName + "? Thoughts?", 1000);

    Sleep(1000);

    // Add separator before choice
    std::cout << "\n================================\n" << std::endl;

    // Present the choice with horizontal menu (conversation stays visible above)
    std::vector<std::string> choiceOptions = {
        "Yes",
        "No"
    };

    MenuSystem menu;
    int choice = menu.showHorizontalMenu(choiceOptions, "Go to the Abandoned mall with your friends?");

    std::string chapterToLoad = "";

    if (choice == 0) {  // YES - Go to the mall
        clearScreen();
        std::cout << "=====================================" << std::endl;
        std::cout << "      CHAT: Emotionally UnStaBlE      " << std::endl;
        std::cout << "=====================================\n" << std::endl;
        // Show messages with typing delays that get replaced
        cout << "Maya: ANNIVERSARY PLANNNSSSS!!!!!" << endl;
        cout << "Jordan: Why are we yelling at 6am?" << endl;
        cout << "Maya: ABANDONED MALL TRIP~" << endl;
        cout << "Noah: That is just trespasing with extra Asbestos." << endl;
        cout << "Sophie: Like the old mall where we all met?" << endl;
        cout << "Maya: HECK YAH!!" << endl;
        cout << "Jordan: But why there? They have been condemned for demolition for years." << endl;
        cout << "Maya: Because! mEmORieS~~!!" << endl;
        cout << "Sopie: Oh! Don't they still have the one event set up that they had prepared before the building was evacuated?" << endl;
        cout << "Maya: Yuppp~~~" << endl;
        cout << "Eli: " << state.playerName << "? Thoughts?" << endl;
        displayTextMessage(state.playerName, "Sure...", 900);
        displayTextMessage("Maya", "YESHHHH!!!!! WOOOO! That's the plan then!! See you guys TONIGHT at 11 in front of the old malllll~~!!", 1500);
        displayTextMessage("Eli", "Great! WAIT 11PM??!! WHy sOo lATe??!", 1000);

        // Random number generator (1-5)
        srand(static_cast<unsigned int>(time(0)));
        int randomNum = (rand() % 5) + 1;

        Sleep(1500);

        // Determine game type based on random number
        if (randomNum == 2 || randomNum == 4) {
            // HORROR GAME
            state.gameType = 1;
            chapterToLoad = "chapter1H";
            std::cout << "[This should be fun! Hopefully...]" << std::endl;
        }
        else {  // randomNum == 1, 3, or 5
            // COZY GAME
            state.gameType = 2;
            chapterToLoad = "chapter1C";
            std::cout << "[This should be fun! Hopefully...]" << std::endl;
        }

    }
    else if (choice == 1) {  // NO - Stay home
        clearScreen();
        std::cout << "=====================================" << std::endl;
        std::cout << "      CHAT: Emotionally UnStaBlE      " << std::endl;
        std::cout << "=====================================\n" << std::endl;
        cout << "Maya: ANNIVERSARY PLANNNSSSS!!!!!" << endl;
        cout << "Jordan: Why are we yelling at 6am?" << endl;
        cout << "Maya: ABANDONED MALL TRIP~" << endl;
        cout << "Noah: That is just trespasing with extra Asbestos." << endl;
        cout << "Sophie: Like the old mall where we all met?" << endl;
        cout << "Maya: HECK YAH!!" << endl;
        cout << "Jordan: But why there? They have been condemned for demolition for years." << endl;
        cout << "Maya: Because! mEmORieS~~!!" << endl;
        cout << "Sopie: Oh! Don't they still have the one event set up that they had prepared before the building was evacuated?" << endl;
        cout << "Maya: Yuppp~~~" << endl;
        cout << "Eli: " << state.playerName << "? Thoughts?" << endl;
        displayTextMessage(state.playerName, "Maybe not. Could get in trouble...", 900);
        displayTextMessage(state.playerName, "What about the light gun chase game?", 900);
        displayTextMessage("Maya", "THE WHAT T^T", 1900);
        displayTextMessage("Jordan", "Laser Tag???", 1000);
        displayTextMessage("Noah", "Please never correct that name.", 1000);
        displayTextMessage("Eli", "I support this renaming!", 1500);
        displayTextMessage("Sophie", "That sounds like a lot of fun! Maybe we could even get food and ice cream after!", 1600);
        displayTextMessage("Maya", "BOOKING NOW!!!!! EEEEEEEE THis is going toe bee AMAZING!!!!!", 1400);
        Sleep(2000);
        cout << endl;
        cout << endl;
        cout << "You have a message from Sophie, and a message from Eli";
        std::vector<std::string> textChoice = {
            "PM with Sophie",
            "PM with Eli",
            "Ignore"
        };
        int chatChoice = menu.showMenu(textChoice, "What would you like to do?");
        if (chatChoice == 0) {
            clearScreen();
            std::cout << "=====================================" << std::endl;
            std::cout << "        PM: Peace Personified?      " << std::endl;
            std::cout << "=====================================\n" << std::endl;
            cout << "Sophie: Hey." << endl;
            displayTextMessage("Sophie", "You ok with changing plans?", 1000);
            displayTextMessage("Sophie", "You seemed nervious about the mall...", 1000);
            displayTextMessage("Sophie", "You don't have to explain anything, but I'm here if you need to talk!", 1000);
            Sleep(1000);
            std::cout << "\n================================\n" << std::endl;

            // Present the choice with horizontal menu (conversation stays visible above)
            std::vector<std::string> heartPMChoice = {
                "Thanks",
                "Just don't want to go.",
                "Ignore"
            };
            int heartPM = menu.showMenu(heartPMChoice, "What would you like to do?");
            Sleep(2000);
        }
        else if (chatChoice == 1) {
            clearScreen();
            std::cout << "=====================================" << std::endl;
            std::cout << "        PM: Brother Dearest      " << std::endl;
            std::cout << "=====================================\n" << std::endl;
            cout << "Eli: Good call." << endl;
            displayTextMessage("Eli", "I didn't want to go either.", 1000);
            displayTextMessage("Eli", "Also, 'Light Gun Chace Game' is the best renaming you have ever done!", 1000);
            Sleep(1000);
            std::cout << "\n================================\n" << std::endl;
            Sleep(2000);
        }
        clearScreen();
        std::cout << "=====================================" << std::endl;
        std::cout << "      CHAT: Emotionally UnStaBlE      " << std::endl;
        std::cout << "=====================================\n" << std::endl;
        cout << "Sopie: Oh! Don't they still have the one event set up that they had prepared before the building was evacuated?" << endl;
        cout << "Maya: Yuppp~~~" << endl;
        cout << "Eli: " << state.playerName << "? Thoughts?" << endl;
        cout << state.playerName << ": Maybe not. Could get in trouble..." << endl;
        cout << state.playerName << ": What about the light gun chase game?" << endl;
        cout << "Maya: THE WHAT T^T" << endl;
        cout << "Jordan: Laser Tag???" << endl;
        cout << "Noah: Please never correct that name." << endl;
        cout << "Eli: I support this renaming!" << endl;
        cout << "Sophie: That sounds like a lot of fun! Maybe we could even get food and ice cream after!" << endl;
        cout << "Maya: BOOKING NOW!!!!! EEEEEEEE THis is going toe bee AMAZING!!!!!" << endl;
        displayTextMessage("Noah", "Important question:", 1000);
        displayTextMessage("Noah", "Are alliances allowed?", 1100);
        displayTextMessage("Jordan", "Coward question", 1000);
        displayTextMessage("Maya", "NO MERCY!!!!!", 1300);
        displayTextMessage("Eli", "Why do I feel like we just released a deamon? -w-'", 1200);
        displayTextMessage(state.playerName, "No. We just fed the gremlin after midnight.", 1250);
        displayTextMessage("Jordan", "You know that refferance?", 1800);
        displayTextMessage("Eli", "WHERE DID YOU LEARN THAT??!!!", 1000);
        displayTextMessage(state.playerName, "Maya and I watched it a few weeks ago when everyone else was buisy.", 1200);
        displayTextMessage("Maya", "HECK YAH WE DID!!!! And I regrest NOTHING!", 1100);
        displayTextMessage("Eli", "TnT", 1000);
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

    // Track previous section's text for menus
    std::vector<std::string> previousSectionText;

    while (true) {
        // Check for special trigger sections BEFORE trying to load from file
        if (currentSectionID == "PHONE_CONVERSATION") {
            playPhoneConversation();
            currentSectionID = "2";
            previousSectionText.clear();  // Clear after custom function
            continue;
        }

        StorySection section;

        if (!reader.getSection(currentSectionID, section)) {
            std::cout << "Error: Section [" << currentSectionID << "] not found!" << std::endl;
            Sleep(2000);
            break;
        }

        // Display the section (only for non-menu sections)
        reader.displaySection(section);

        // If this is a text section, save its text for the next menu
        if (!section.isMenu && !section.textParagraphs.empty()) {
            previousSectionText = section.textParagraphs;
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

        // Display the section
        reader.displaySection(section);

        if (section.isMenu) {
            if (section.hasEvent) {
                int choice = reader.showSectionMenu(section, previousSectionText);

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
                            previousSectionText.clear();  // Clear when loading new chapter
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
                int choice = reader.showSectionMenu(section, previousSectionText);

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
                        previousSectionText.clear();  // Clear when loading new chapter
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

            // Clear previous text after showing menu
            previousSectionText.clear();
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

    std::cout << "=====================================" << std::endl;
    std::cout << "      CHAT: Emotionally UnStaBlE      " << std::endl;
    std::cout << "=====================================\n" << std::endl;
    Sleep(1000);

    // Show messages with typing delays that get replaced
    displayTextMessage("Maya", "ANNIVERSARY PLANNNSSSS!!!!!", 1500);
    displayTextMessage("Jordan", "Why are we yelling at 6am?", 1200);
    displayTextMessage("Maya", "ABANDONED MALL TRIP~", 1800);
    displayTextMessage("Noah", "That is just trespasing with extra Asbestos.", 1150);
    displayTextMessage("Sophie", "Like the old mall where we all met?", 1300);
    displayTextMessage("Maya", "HECK YAH!!", 1000);
    displayTextMessage("Jordan", "But why there? They have been condemned for demolition for years.", 1400);
    displayTextMessage("Maya", "Because! mEmORieS~~!!", 1100);
    displayTextMessage("Sopie", "Oh! Don't they still have the one event set up that they had prepared before the building was evacuated?", 1600);
    displayTextMessage("Maya", "Yuppp~~~", 1250);
    displayTextMessage("Eli", state.playerName + "? Thoughts?", 1000);

    Sleep(2000);
}
void GameManager::playPhoneConversation() {
    clearScreen();

    std::cout << "=====================================" << std::endl;
    std::cout << "      CHAT: Emotionally UnStaBlE      " << std::endl;
    std::cout << "=====================================\n" << std::endl;
    Sleep(1000);

    // Show messages with typing delays that get replaced
    displayTextMessage("Maya", "WAKE UP SOLDIERS", 1500);
    displayTextMessage("Maya", "WHO IS READY TO GET ABSOLUTELY DESTROYED TODAY???", 1300);
    displayTextMessage("Jordan", "it is 8 am...why are you yelling.", 1200);
    displayTextMessage("Maya", "WAR HAS NO CLOCK!", 1800);
    displayTextMessage("Noah", "It literally does.", 1150);
    displayTextMessage("Maya", "BECAUSE I HAVE ENERGY!!!!!!!!!", 1800);
    displayTextMessage("Noah", "Statistically speaking, Maya, your energy levels are concerning.", 1300);
    displayTextMessage("Maya", "StaTIsTicaLLy speaking YOU need to touch GRASS!!~", 1400);
    displayTextMessage("Sophie", "Good morning everyone :)", 1000);
    displayTextMessage("Sophie", "Did everyone sleep okay?", 1400);
    displayTextMessage("Eli", "No.", 1000);
    displayTextMessage("Eli", "Coffee machine dead.", 1000);
    displayTextMessage("Eli", "Morale critical (XoX)", 1000);
    displayTextMessage("Jordan", "RIP!", 1200);
    displayTextMessage("Noah", "Oh how tragic.", 1400);
    displayTextMessage("Maya", "WAIT the coffee machine EXPLODED???!!", 1000);
    displayTextMessage("Eli", "Violently.", 1000);
    displayTextMessage("Eli", "Smoke included.", 1000);
    displayTextMessage("Eli", "Poppy cried. TnT", 1000);
    displayTextMessage("Sophie", "Awww :( <3", 1200);
    displayTextMessage("Eli", "Dad almost held a funeral.", 1000);
    displayTextMessage("Jordan", "A valid responce!", 1200);
    displayTextMessage("Noah", "I would also mourn. That machine made a good cup of coffee", 1000);
    displayTextMessage("Eli", state.playerName + " is awake.", 1000);
    displayTextMessage("Eli", "I can sense it.", 1000);
    displayTextMessage(state.playerName, "No.", 1200);
    displayTextMessage("Eli", "Lies!!", 1000);
    displayTextMessage("Noah", "Wow so " + state.playerName + "s not nocturnal.", 1000);
    displayTextMessage(state.playerName, "Negative.", 1200);
    displayTextMessage(state.playerName, "Currently dying from lack of jitter juice.", 1300);
    displayTextMessage("Jordan", "Same tbh.", 1000);
    displayTextMessage("Sophie", "Did you at least sleep okay?", 1100);
    displayTextMessage(state.playerName, "Acceptable unconscious duration achieved.", 1300);
    displayTextMessage("Maya", "What are you??? A Robot!!?", 1000);
    displayTextMessage(state.playerName, "Incorrect.", 1400);
    displayTextMessage(state.playerName, "Robots have better sleep schedules.", 1000);
    displayTextMessage("Eli", "Well, she's got you there~", 1200);
    displayTextMessage("Noah", "Pickup still 11?", 1000);
    displayTextMessage("Sophie", "Yup! :)", 1200);
    displayTextMessage("Sophie", "I made snacks as well! So, we can have some lunch on the way there.", 1000);
    displayTextMessage("Maya", "SOPHIE MY BELOVED <<<3333  <(*u*)>", 1200);
    displayTextMessage("Eli", "See you nerds soon!~", 1000);
    displayTextMessage("Eli", "Can't wait to destroy you all in laser tag~~ ^(U3U)^", 1000);

    Sleep(2000);
}
