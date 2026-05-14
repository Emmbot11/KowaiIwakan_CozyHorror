#include "StoryReader.h"
#include "genFunctions.h"
#include <Windows.h>
#include <cctype>
#include <sstream>

// ============================================================================
// CONSTRUCTOR
// ============================================================================

StoryReader::StoryReader(const std::string& directory)
    : storyDirectory(directory),
    playerName("Player"),
    playerGender("they/them"),
    currentChapterFile("") {
    CreateDirectoryA(storyDirectory.c_str(), NULL);
}

// ============================================================================
// PLAYER INFO SETTERS
// ============================================================================

void StoryReader::setPlayerName(const std::string& name) {
    playerName = name;
}

void StoryReader::setPlayerGender(const std::string& gender) {
    playerGender = gender;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Check if section ID ends with a letter (menu section)
bool StoryReader::isMenuSection(const std::string& sectionID) {
    if (sectionID.empty()) return false;
    char lastChar = sectionID.back();
    return std::isalpha(lastChar);
}

// Pronoun replacement function
std::string StoryReader::replacePronouns(const std::string& text) {
    std::string result = text;

    // Determine pronouns based on gender
    std::string subject, object, possessive, possessivePronoun, reflexive;

    if (playerGender == "he/him" || playerGender == "male" || playerGender == "m") {
        subject = "he";
        object = "him";
        possessive = "his";
        possessivePronoun = "his";
        reflexive = "himself";
    }
    else if (playerGender == "she/her" || playerGender == "female" || playerGender == "f") {
        subject = "she";
        object = "her";
        possessive = "her";
        possessivePronoun = "hers";
        reflexive = "herself";
    }
    else {  // Default to they/them
        subject = "they";
        object = "them";
        possessive = "their";
        possessivePronoun = "theirs";
        reflexive = "themself";
    }

    // Replace placeholders
    // Name replacements
    size_t pos = 0;
    while ((pos = result.find("{name}", pos)) != std::string::npos) {
        result.replace(pos, 6, playerName);
        pos += playerName.length();
    }

    pos = 0;
    while ((pos = result.find("{NAME}", pos)) != std::string::npos) {
        std::string upperName = playerName;
        for (char& c : upperName) c = std::toupper(c);
        result.replace(pos, 6, upperName);
        pos += upperName.length();
    }

    // Subject pronoun (he/she/they)
    pos = 0;
    while ((pos = result.find("{subject}", pos)) != std::string::npos) {
        result.replace(pos, 9, subject);
        pos += subject.length();
    }

    pos = 0;
    while ((pos = result.find("{Subject}", pos)) != std::string::npos) {
        std::string cap = subject;
        cap[0] = std::toupper(cap[0]);
        result.replace(pos, 9, cap);
        pos += cap.length();
    }

    // Object pronoun (him/her/them)
    pos = 0;
    while ((pos = result.find("{object}", pos)) != std::string::npos) {
        result.replace(pos, 8, object);
        pos += object.length();
    }

    pos = 0;
    while ((pos = result.find("{Object}", pos)) != std::string::npos) {
        std::string cap = object;
        cap[0] = std::toupper(cap[0]);
        result.replace(pos, 8, cap);
        pos += cap.length();
    }

    // Possessive adjective (his/her/their)
    pos = 0;
    while ((pos = result.find("{possessive}", pos)) != std::string::npos) {
        result.replace(pos, 12, possessive);
        pos += possessive.length();
    }

    pos = 0;
    while ((pos = result.find("{Possessive}", pos)) != std::string::npos) {
        std::string cap = possessive;
        cap[0] = std::toupper(cap[0]);
        result.replace(pos, 12, cap);
        pos += cap.length();
    }

    // Possessive pronoun (his/hers/theirs)
    pos = 0;
    while ((pos = result.find("{possessivePronoun}", pos)) != std::string::npos) {
        result.replace(pos, 19, possessivePronoun);
        pos += possessivePronoun.length();
    }

    // Reflexive (himself/herself/themself)
    pos = 0;
    while ((pos = result.find("{reflexive}", pos)) != std::string::npos) {
        result.replace(pos, 11, reflexive);
        pos += reflexive.length();
    }

    return result;
}

// ============================================================================
// PARSING FUNCTIONS
// ============================================================================

// Parse EVENT line
bool StoryReader::parseEventLine(const std::string& line, EventData& event) {
    if (line.find("EVENT:") != 0) {
        return false;
    }

    // Parse: EVENT: puzzle_code | ANSWER: 1984 | SUCCESS: 9 | FAIL: 8b
    std::stringstream ss(line.substr(6)); // Skip "EVENT:"
    std::string token;

    while (std::getline(ss, token, '|')) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);

        size_t colonPos = token.find(':');
        if (colonPos != std::string::npos) {
            std::string key = token.substr(0, colonPos);
            std::string value = token.substr(colonPos + 1);

            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (colonPos == 0) {
                // First token is the event type (no key)
                event.eventType = token;
            }
            else if (key == "ANSWER") {
                event.answer = value;
            }
            else if (key == "ITEM") {
                event.requiredItem = value;
            }
            else if (key == "TIME") {
                event.timeLimit = std::stoi(value);
            }
            else if (key == "SUCCESS") {
                event.successSection = value;
            }
            else if (key == "FAIL") {
                event.failSection = value;
            }
            else if (key == "CHOICE") {
                event.choiceID = value;
            }
        }
        else if (event.eventType.empty()) {
            event.eventType = token;
        }
    }

    return !event.eventType.empty();
}

// Parse LOAD_CHAPTER line
bool StoryReader::parseLoadChapterLine(const std::string& line, StorySection& section) {
    if (line.find("LOAD_CHAPTER:") != 0) {
        return false;
    }

    // Parse: LOAD_CHAPTER: chapter2 | START: 1 | CONDITION: key
    std::stringstream ss(line.substr(13)); // Skip "LOAD_CHAPTER:"
    std::string token;

    while (std::getline(ss, token, '|')) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);

        size_t colonPos = token.find(':');
        if (colonPos != std::string::npos) {
            std::string key = token.substr(0, colonPos);
            std::string value = token.substr(colonPos + 1);

            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (key == "START") {
                section.loadStartSection = value;
            }
            else if (key == "CONDITION") {
                section.loadCondition = value;
            }
        }
        else {
            // First token is the chapter filename
            section.loadChapter = token;
        }
    }

    return !section.loadChapter.empty();
}

// ============================================================================
// CHAPTER LOADING
// ============================================================================

bool StoryReader::loadChapterFile(const std::string& chapterFile) {
    sections.clear();
    currentChapterFile = chapterFile;

    std::string fullPath = storyDirectory + chapterFile + ".txt";
    std::ifstream file(fullPath);

    if (!file.is_open()) {
        std::cout << "Error: Could not open " << fullPath << std::endl;
        return false;
    }

    std::string line;
    StorySection currentSection;
    std::string currentParagraph = "";
    bool inSection = false;

    while (std::getline(file, line)) {
        // Check for END marker FIRST (before section markers!)
        if (line == "[END]") {
            if (inSection && !currentSection.sectionID.empty()) {
                if (!currentParagraph.empty()) {
                    currentSection.textParagraphs.push_back(currentParagraph);
                }
                sections[currentSection.sectionID] = currentSection;
            }
            break;
        }

        // Check if this is a section marker like [1] or [1a]
        if (line.length() > 2 && line[0] == '[' && line.back() == ']') {
            // Save previous section if it exists
            if (inSection && !currentSection.sectionID.empty()) {
                if (!currentParagraph.empty()) {
                    currentSection.textParagraphs.push_back(currentParagraph);
                    currentParagraph = "";
                }
                sections[currentSection.sectionID] = currentSection;
            }

            // Start new section
            currentSection = StorySection();
            currentSection.sectionID = line.substr(1, line.length() - 2);
            currentSection.isMenu = isMenuSection(currentSection.sectionID);
            currentSection.hasEvent = false;
            currentSection.itemToAdd = "";
            currentSection.nextSection = "";
            currentSection.loadChapter = "";
            currentSection.loadStartSection = "";
            currentSection.loadCondition = "";
            inSection = true;
            continue;
        }

        // Process content
        if (inSection) {
            // Check for special commands FIRST (before processing as text/menu content)

            // Check for ITEM_ADD line
            if (line.find("ITEM_ADD:") == 0) {
                std::string item = line.substr(9);
                item.erase(0, item.find_first_not_of(" \t"));
                item.erase(item.find_last_not_of(" \t") + 1);
                currentSection.itemToAdd = item;
                continue;
            }

            // Check for NEXT line
            if (line.find("NEXT:") == 0) {
                std::string next = line.substr(5);
                next.erase(0, next.find_first_not_of(" \t"));
                next.erase(next.find_last_not_of(" \t") + 1);
                currentSection.nextSection = next;
                continue;
            }

            // Check for LOAD_CHAPTER line
            if (line.find("LOAD_CHAPTER:") == 0) {
                parseLoadChapterLine(line, currentSection);
                continue;
            }

            // Check for EVENT line
            if (line.find("EVENT:") == 0) {
                currentSection.hasEvent = true;
                parseEventLine(line, currentSection.event);
                continue;
            }

            // Now process based on section type
            if (currentSection.isMenu) {
                // This is a menu section
                if (line.find("OPTION:") == 0) {
                    // Parse: OPTION: text | NEXT: 4
                    // OR: OPTION: text | LOAD: chapter2 | START: 1
                    // OR: OPTION: text | LOAD: chapter2 | START: 1 | CONDITION: key

                    size_t pipePos = line.find('|');
                    if (pipePos != std::string::npos) {
                        MenuOption option;
                        option.text = line.substr(8, pipePos - 8);

                        option.text.erase(0, option.text.find_first_not_of(" \t"));
                        option.text.erase(option.text.find_last_not_of(" \t") + 1);

                        // Parse the rest of the line for NEXT or LOAD
                        std::string remainder = line.substr(pipePos + 1);
                        std::stringstream ss(remainder);
                        std::string token;

                        while (std::getline(ss, token, '|')) {
                            token.erase(0, token.find_first_not_of(" \t"));
                            token.erase(token.find_last_not_of(" \t") + 1);

                            if (token.find("NEXT:") == 0) {
                                option.nextSection = token.substr(5);
                                option.nextSection.erase(0, option.nextSection.find_first_not_of(" \t"));
                                option.nextSection.erase(option.nextSection.find_last_not_of(" \t") + 1);
                            }
                            else if (token.find("LOAD:") == 0) {
                                option.loadChapter = token.substr(5);
                                option.loadChapter.erase(0, option.loadChapter.find_first_not_of(" \t"));
                                option.loadChapter.erase(option.loadChapter.find_last_not_of(" \t") + 1);
                            }
                            else if (token.find("START:") == 0) {
                                option.loadStartSection = token.substr(6);
                                option.loadStartSection.erase(0, option.loadStartSection.find_first_not_of(" \t"));
                                option.loadStartSection.erase(option.loadStartSection.find_last_not_of(" \t") + 1);
                            }
                            else if (token.find("CONDITION:") == 0) {
                                option.loadCondition = token.substr(10);
                                option.loadCondition.erase(0, option.loadCondition.find_first_not_of(" \t"));
                                option.loadCondition.erase(option.loadCondition.find_last_not_of(" \t") + 1);
                            }
                        }

                        currentSection.options.push_back(option);
                    }
                }
                else if (!line.empty()) {
                    currentSection.menuPrompt = line;
                }
            }
            else {
                // This is a text section
                if (line.empty()) {
                    if (!currentParagraph.empty()) {
                        currentSection.textParagraphs.push_back(currentParagraph);
                        currentParagraph = "";
                    }
                }
                else {
                    if (!currentParagraph.empty()) {
                        currentParagraph += " ";
                    }
                    currentParagraph += line;
                }
            }
        }

    }

    file.close();

    std::cout << "Loaded " << sections.size() << " sections from " << chapterFile << std::endl;
    return !sections.empty();
}

// ============================================================================
// SECTION RETRIEVAL AND DISPLAY
// ============================================================================

// Get section by ID
bool StoryReader::getSection(const std::string& sectionID, StorySection& section) {
    auto it = sections.find(sectionID);
    if (it != sections.end()) {
        section = it->second;
        return true;
    }
    return false;
}

// Display section with pronoun replacement
void StoryReader::displaySection(const StorySection& section) {
    // Only display text for NON-menu sections
    // Menu sections are handled by showSectionMenu()
    if (!section.isMenu) {
        clearScreen();
        std::cout << "\n--- Section " << section.sectionID << " ---\n" << std::endl;

        // Display text paragraphs WITH pronoun replacement
        for (const auto& paragraph : section.textParagraphs) {
            std::string processedText = replacePronouns(paragraph);
            std::cout << processedText << "\n" << std::endl;
            Sleep(5000);  // Pause between paragraphs for effect
        }
    }
    // If it IS a menu, do nothing - showSectionMenu() will handle it
}


// Show section menu with pronoun replacement
int StoryReader::showSectionMenu(const StorySection& section, const std::vector<std::string>& previousText) {
    if (section.options.empty()) {
        return -1;
    }

    // Clear screen once
    clearScreen();

    // Display section ID
    std::cout << "\n--- Section " << section.sectionID << " ---\n" << std::endl;

    // Display PREVIOUS section's text first (if provided)
    if (!previousText.empty()) {
        for (const auto& paragraph : previousText) {
            std::string processedText = replacePronouns(paragraph);
            std::cout << processedText << "\n" << std::endl;
            Sleep(800);
        }
    }

    // Then display THIS section's text (menu prompt area)
    for (const auto& paragraph : section.textParagraphs) {
        std::string processedText = replacePronouns(paragraph);
        std::cout << processedText << "\n" << std::endl;
        Sleep(800);
    }

    // Wait for Enter
    std::cout << "\n[Press Enter to continue...]" << std::endl;
    std::cin.ignore(100, '\n');
    std::cin.get();

    // Display menu prompt
    if (!section.menuPrompt.empty()) {
        std::string processedPrompt = replacePronouns(section.menuPrompt);
        std::cout << "\n" << processedPrompt << "\n" << std::endl;
    }

    // Create option strings
    std::vector<std::string> menuOptions;
    for (const auto& option : section.options) {
        std::string processedOption = replacePronouns(option.text);
        menuOptions.push_back(processedOption);
    }

    // Show menu with arrow keys WITHOUT clearing screen
    MenuSystem menu;
    return menu.showMenuNoClear(menuOptions);
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

std::string StoryReader::handleEvent(const EventData& event) {
    bool success = false;

    if (event.eventType == "puzzle_code" || event.eventType == "puzzle_combination") {
        success = handlePuzzleCode(event);
    }
    else if (event.eventType == "timed_response") {
        success = handleTimedResponse(event);
    }
    else if (event.eventType == "item_check") {
        success = handleItemCheck(event);
    }
    else if (event.eventType == "track_choice") {
        trackChoice(event.choiceID);
        return event.successSection;  // Always succeed for tracking
    }

    return success ? event.successSection : event.failSection;
}

bool StoryReader::handlePuzzleCode(const EventData& event) {
    std::string userInput;
    std::cout << "\nEnter code: ";
    std::cin >> userInput;

    if (userInput == event.answer) {
        std::cout << "\n✓ Correct!" << std::endl;
        Sleep(1000);
        return true;
    }
    else {
        std::cout << "\n✗ Wrong!" << std::endl;
        Sleep(1000);
        return false;
    }
}

bool StoryReader::handleTimedResponse(const EventData& event) {
    std::cout << "\nYou have " << event.timeLimit << " seconds!" << std::endl;
    bool result = timedResponse(event.answer, event.timeLimit);
    Sleep(1000);
    return result;
}

bool StoryReader::handleItemCheck(const EventData& event) {
    if (hasItem(event.requiredItem)) {
        std::cout << "\nYou have the " << event.requiredItem << "!" << std::endl;
        Sleep(1000);
        return true;
    }
    else {
        std::cout << "\nYou need a " << event.requiredItem << "!" << std::endl;
        Sleep(1500);
        return false;
    }
}

void StoryReader::trackChoice(const std::string& choiceID) {
    playerChoices.insert(choiceID);
    std::cout << "\n[Choice recorded: " << choiceID << "]" << std::endl;
    Sleep(800);
}

// ============================================================================
// INVENTORY MANAGEMENT
// ============================================================================

void StoryReader::addItem(const std::string& item) {
    playerInventory.insert(item);
    std::cout << "\n[+ Added to inventory: " << item << "]" << std::endl;
    Sleep(1000);
}

void StoryReader::removeItem(const std::string& item) {
    playerInventory.erase(item);
    std::cout << "\n[- Removed from inventory: " << item << "]" << std::endl;
    Sleep(1000);
}

bool StoryReader::hasItem(const std::string& item) {
    return playerInventory.find(item) != playerInventory.end();
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get current chapter filename
std::string StoryReader::getCurrentChapter() {
    return currentChapterFile;
}

// Clear all sections and reset
void StoryReader::clearSections() {
    sections.clear();
    playerInventory.clear();
    playerChoices.clear();
}
