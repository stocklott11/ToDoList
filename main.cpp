#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <limits>
#include <iomanip>

// Simple utility to trim whitespace from both ends of a string
static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

// Task represents a single to-do item
class Task {
private:
    int id;                // unique id for stable selection
    std::string title;     // short title
    std::string notes;     // optional details
    bool completed;        // completion status

public:
    Task() : id(-1), completed(false) {}

    Task(int id_, const std::string& title_, const std::string& notes_, bool completed_ = false)
        : id(id_), title(title_), notes(notes_), completed(completed_) {}

    int getId() const { return id; }
    const std::string& getTitle() const { return title; }
    const std::string& getNotes() const { return notes; }
    bool isCompleted() const { return completed; }

    void setTitle(const std::string& t) { title = t; }
    void setNotes(const std::string& n) { notes = n; }
    void setCompleted(bool c) { completed = c; }

    // Serialize as a simple CSV line: id,completed,title,notes
    // Commas in fields are escaped as "\,"
    static std::string escapeCommas(const std::string& in) {
        std::string out;
        out.reserve(in.size());
        for (char c : in) {
            if (c == ',') {
                out += "\\,";
            } else if (c == '\n' || c == '\r') {
                // strip newlines for simplicity
                out += ' ';
            } else {
                out += c;
            }
        }
        return out;
    }

    static std::string unescapeCommas(const std::string& in) {
        std::string out;
        out.reserve(in.size());
        for (size_t i = 0; i < in.size(); ++i) {
            if (in[i] == '\\' && i + 1 < in.size() && in[i + 1] == ',') {
                out += ',';
                ++i;
            } else {
                out += in[i];
            }
        }
        return out;
    }

    std::string toCsv() const {
        std::ostringstream oss;
        oss << id << "," << (completed ? 1 : 0) << ","
            << escapeCommas(title) << ","
            << escapeCommas(notes);
        return oss.str();
    }

    static bool fromCsv(const std::string& line, Task& outTask) {
        // Split into 4 parts: id, completed, title, notes
        std::vector<std::string> parts;
        parts.reserve(4);

        std::string current;
        bool escape = false;
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (escape) {
                // keep escaped comma as literal comma
                current.push_back(c);
                escape = false;
            } else if (c == '\\') {
                // mark next char as escaped
                escape = true;
            } else if (c == ',') {
                parts.push_back(current);
                current.clear();
            } else {
                current.push_back(c);
            }
        }
        parts.push_back(current);

        if (parts.size() < 4) return false;
        try {
            int id = std::stoi(parts[0]);
            bool completed = (std::stoi(parts[1]) != 0);
            std::string title = unescapeCommas(parts[2]);
            std::string notes = unescapeCommas(parts[3]);
            outTask = Task(id, title, notes, completed);
            return true;
        } catch (...) {
            return false;
        }
    }
};

// TaskManager owns the list of tasks and provides operations
class TaskManager {
private:
    std::vector<Task> tasks;
    int nextId;
    std::string savePath;

    int generateId() { return nextId++; }

public:
    explicit TaskManager(const std::string& filePath = "tasks.csv")
        : nextId(1), savePath(filePath) {}

    // Load tasks from disk if present
    bool load() {
        std::ifstream in(savePath);
        if (!in.is_open()) {
            // File may not exist on first run
            return false;
        }
        tasks.clear();
        std::string line;
        int maxSeen = 0;
        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            Task t;
            if (Task::fromCsv(line, t)) {
                tasks.push_back(t);
                if (t.getId() > maxSeen) maxSeen = t.getId();
            }
        }
        nextId = maxSeen + 1;
        return true;
    }

    // Save tasks to disk
    bool save() const {
        std::ofstream out(savePath, std::ios::trunc);
        if (!out.is_open()) return false;
        for (const auto& t : tasks) {
            out << t.toCsv() << "\n";
        }
        return true;
    }

    // CRUD operations
    int addTask(const std::string& title, const std::string& notes) {
        Task t(generateId(), title, notes, false);
        tasks.push_back(t);
        return t.getId();
    }

    bool removeById(int id) {
        for (size_t i = 0; i < tasks.size(); ++i) {
            if (tasks[i].getId() == id) {
                tasks.erase(tasks.begin() + static_cast<long>(i));
                return true;
            }
        }
        return false;
    }

    bool toggleComplete(int id) {
        for (auto& t : tasks) {
            if (t.getId() == id) {
                t.setCompleted(!t.isCompleted());
                return true;
            }
        }
        return false;
    }

    bool editTask(int id, const std::string& newTitle, const std::string& newNotes) {
        for (auto& t : tasks) {
            if (t.getId() == id) {
                if (!newTitle.empty()) t.setTitle(newTitle);
                if (!newNotes.empty()) t.setNotes(newNotes);
                return true;
            }
        }
        return false;
    }

    const std::vector<Task>& list() const { return tasks; }

    bool clearAll() {
        tasks.clear();
        return true;
    }
};

// Input helpers
static int readInt(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        int value;
        if (std::cin >> value) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        std::cout << "Invalid number. Try again.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

static std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    return trim(s);
}

// Pretty printing
static void printTasks(const std::vector<Task>& tasks) {
    if (tasks.empty()) {
        std::cout << "No tasks found.\n";
        return;
    }
    std::cout << "\n"
              << std::left << std::setw(6) << "ID"
              << std::left << std::setw(12) << "Status"
              << std::left << std::setw(30) << "Title"
              << "Notes\n";
    std::cout << std::string(75, '=') << "\n";
    for (const auto& t : tasks) {
        std::cout << std::left << std::setw(6) << t.getId()
                  << std::left << std::setw(12) << (t.isCompleted() ? "Complete" : "Open")
                  << std::left << std::setw(30) << t.getTitle()
                  << t.getNotes() << "\n";
    }
    std::cout << "\n";
}

static void printMenu() {
    std::cout << "=============================\n";
    std::cout << "       To Do List Menu       \n";
    std::cout << "=============================\n";
    std::cout << "1. List tasks\n";
    std::cout << "2. Add task\n";
    std::cout << "3. Toggle complete\n";
    std::cout << "4. Edit task\n";
    std::cout << "5. Remove task\n";
    std::cout << "6. Clear all tasks\n";
    std::cout << "7. Save\n";
    std::cout << "8. Load\n";
    std::cout << "9. Exit\n";
}

int main() {
    TaskManager manager("tasks.csv");
    // Auto load on start for convenience
    manager.load();

    while (true) {
        printMenu();
        int choice = readInt("Choose an option [1-9]: ");
        std::cout << "\n";

        if (choice == 1) {
            printTasks(manager.list());
        } else if (choice == 2) {
            std::string title = readLine("Enter title: ");
            while (title.empty()) {
                std::cout << "Title cannot be empty.\n";
                title = readLine("Enter title: ");
            }
            std::string notes = readLine("Enter notes (optional): ");
            int id = manager.addTask(title, notes);
            std::cout << "Added task with id " << id << ".\n\n";
        } else if (choice == 3) {
            int id = readInt("Enter task id to toggle: ");
            if (manager.toggleComplete(id)) {
                std::cout << "Toggled completion.\n\n";
            } else {
                std::cout << "Task not found.\n\n";
            }
        } else if (choice == 4) {
            int id = readInt("Enter task id to edit: ");
            std::string newTitle = readLine("New title (leave blank to keep): ");
            std::string newNotes = readLine("New notes (leave blank to keep): ");
            if (manager.editTask(id, newTitle, newNotes)) {
                std::cout << "Edited task.\n\n";
            } else {
                std::cout << "Task not found.\n\n";
            }
        } else if (choice == 5) {
            int id = readInt("Enter task id to remove: ");
            if (manager.removeById(id)) {
                std::cout << "Removed task.\n\n";
            } else {
                std::cout << "Task not found.\n\n";
            }
        } else if (choice == 6) {
            char confirm;
            std::cout << "Are you sure you want to clear all tasks? [y/N]: ";
            std::cin >> confirm;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (confirm == 'y' || confirm == 'Y') {
                manager.clearAll();
                std::cout << "All tasks cleared.\n\n";
            } else {
                std::cout << "Canceled.\n\n";
            }
        } else if (choice == 7) {
            if (manager.save()) std::cout << "Saved to tasks.csv\n\n";
            else std::cout << "Save failed.\n\n";
        } else if (choice == 8) {
            if (manager.load()) std::cout << "Loaded from tasks.csv\n\n";
            else std::cout << "Load failed or no file yet.\n\n";
        } else if (choice == 9) {
            // On exit, save to persist
            manager.save();
            std::cout << "Goodbye.\n";
            break;
        } else {
            std::cout << "Invalid choice.\n\n";
        }
    }
    return 0;
}
