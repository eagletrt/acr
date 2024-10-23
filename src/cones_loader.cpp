#include "cones_loader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include "notifications.hpp"
#include <algorithm>

// Mutex to protect access to the cones vector
extern std::mutex renderLock;
extern std::vector<cone_t> cones;

// Trimming function to remove white spaces
static inline std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }

    auto end = s.end();
    if (start == end) return "";

    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

// Function to load cones from a CSV file
bool loadConesFromCSV(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error opening CSV file: " << filePath << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    std::vector<cone_t> loadedCones;

    // Read and skip the header row
    if (std::getline(file, line)) {
        lineNumber++;
        // Optionally, you can verify that the header row is correct
    }

    while (std::getline(file, line)) {
        lineNumber++;
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        // Use ',' as delimiter
        while (std::getline(ss, token, ',')) {
            tokens.push_back(trim(token));
        }

        // Ensure at least 5 fields (timestamp, cone_id, cone_name, lat, lon)
        if (tokens.size() < 5) {
            std::cerr << "Invalid format in CSV at line " << lineNumber << ": " << line << std::endl;
            continue;
        }

        cone_t cone;
        try {
            cone.lat = std::stof(tokens[3]);
            cone.lon = std::stof(tokens[4]);

            // Parse cone_id and assign cone type
            int cone_id = std::stoi(tokens[1]);
            switch (cone_id) {
                case 0:
                    cone.id = CONE_ID_YELLOW;
                    break;
                case 1:
                    cone.id = CONE_ID_BLUE;
                    break;
                case 2:
                    cone.id = CONE_ID_ORANGE;
                    break;
                default:
                    cone.id = CONE_ID_YELLOW; // Default value
                    break;
            }

            // Parse altitude if available
            if (tokens.size() >= 6) {
                cone.alt = std::stof(tokens[5]);
            } else {
                cone.alt = 0.0f; // Default value
            }

            // Parse timestamp if necessary
            try {
                cone.timestamp = std::stoul(tokens[0]); // Use stoul for larger integers
            } catch (...) {
                cone.timestamp = 0.0f;
            }

        } catch (const std::exception& e) {
            std::cerr << "Error parsing CSV at line " << lineNumber << ": " << e.what() << std::endl;
            continue;
        }

        loadedCones.push_back(cone);
        printf("Loaded Cone: Lon=%f, Lat=%f, ID=%d\n", cone.lon, cone.lat, cone.id);
    }

    file.close();

    if (loadedCones.empty()) {
        std::cerr << "No valid cones loaded from CSV: " << filePath << std::endl;
        return false;
    }

    {
        std::unique_lock<std::mutex> lck(renderLock);
        cones.insert(cones.end(), loadedCones.begin(), loadedCones.end());
    }

    return true;
}

// Function to clear all loaded cones
void clearLoadedCones() {
    std::unique_lock<std::mutex> lck(renderLock);
    cones.clear();
}