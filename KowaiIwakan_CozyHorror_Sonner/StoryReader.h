#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <iostream>

struct MenuOption {
    std::string text;
    std::string nextSection;
    std::string loadChapter;        // ADD THIS
    std::string loadStartSection;   // ADD THIS
    std::string loadCondition;      // ADD THIS
};

struct EventData {
    std::string eventType;      // "puzzle_code", "timed_response", "item_check", etc.
    std::string answer;          // Expected answer
    std::string requiredItem;    // Item needed (for item_check)
    int timeLimit;               // Time limit in seconds (for timed events)
    std::string successSection;  // Where to go if success
    std::string failSection;     // Where to go if fail
    std::string choiceID;        // ID to track this choice
};

struct StorySection {
    std::string sectionID;
    std::vector<std::string> textParagraphs;
    std::string menuPrompt;
    std::vector<MenuOption> options;
    bool isMenu;
    bool hasEvent;
    EventData event;
    std::string itemToAdd;
    std::string nextSection;        // ADD THIS - where text sections go next
    std::string loadChapter;        // ADD THIS - chapter file to load
    std::string loadStartSection;   // ADD THIS - section to start at in new chapter
    std::string loadCondition;      // ADD THIS - item required to load chapter
};

class StoryReader {
private:
    std::string storyDirectory;
    std::map<std::string, StorySection> sections;
    std::set<std::string> playerInventory;
    std::set<std::string> playerChoices;

    std::string playerName;
    std::string playerGender;
    std::string currentChapterFile;  // ADD THIS - track current chapter

    bool isMenuSection(const std::string& sectionID);
    bool parseEventLine(const std::string& line, EventData& event);
    bool parseLoadChapterLine(const std::string& line, StorySection& section);  // ADD THIS
    std::string replacePronouns(const std::string& text);

public:
    StoryReader(const std::string& directory = "story/");

    void setPlayerName(const std::string& name);
    void setPlayerGender(const std::string& gender);

    bool loadChapterFile(const std::string& chapterFile);
    bool getSection(const std::string& sectionID, StorySection& section);
    void displaySection(const StorySection& section);
    int showSectionMenu(const StorySection& section);

    std::string getCurrentChapter();  // ADD THIS

    // Event handling
    std::string handleEvent(const EventData& event);
    bool handlePuzzleCode(const EventData& event);
    bool handleTimedResponse(const EventData& event);
    bool handleItemCheck(const EventData& event);
    void trackChoice(const std::string& choiceID);

    // Inventory management
    void addItem(const std::string& item);
    void removeItem(const std::string& item);
    bool hasItem(const std::string& item);

    void clearSections();
};
