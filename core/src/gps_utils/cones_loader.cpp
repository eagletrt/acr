#include "cones_loader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include "notifications.hpp"
#include "config.hpp"

extern "C" {
    #include "defines.h"
    #include "acr.h"
    #include "utils.h"
}

ConesLoader::ConesLoader(NotificationManager& notificationManager)
    : notificationManager_(notificationManager) {}

ConesLoader::~ConesLoader() {}

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

bool ConesLoader::loadFromCSV(const std::string& filePath, std::vector<cone_t>& loadedCones) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Errore nell'apertura del file CSV: " << filePath << std::endl;
        notificationManager_.showPopup("ConesLoader", "Errore", "Impossibile aprire il file CSV.", NotificationType::Error);
        return false;
    }

    std::string line;
    int lineNumber = 0;

    // Read header
    if (std::getline(file, line)) {
        lineNumber++;
        // Optionally, check the header here
    }

    loadedCones.clear();

    while (std::getline(file, line)) {
        lineNumber++;
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        // Split line into tokens
        while (std::getline(ss, token, ',')) {
            tokens.push_back(trim(token));
        }

        if (tokens.size() < 5) {
            std::cerr << "Formato CSV non valido alla riga " << lineNumber << ": " << line << std::endl;
            notificationManager_.showPopup("ConesLoader", "Errore", "Formato CSV non valido.", NotificationType::Error);
            continue;
        }

        cone_t cone;
        try {
            cone.lat = std::stof(tokens[3]);
            cone.lon = std::stof(tokens[4]);

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
                    cone.id = CONE_ID_YELLOW; 
                    break;
            }

            if (tokens.size() >= 6) {
                cone.alt = std::stof(tokens[5]);
            } else {
                cone.alt = 0.0f; 
            }

            try {
                cone.timestamp = std::stoul(tokens[0]);
            } catch (...) {
                cone.timestamp = 0;
            }

        } catch (const std::exception& e) {
            std::cerr << "Errore nel parsing del CSV alla riga " << lineNumber << ": " << e.what() << std::endl;
            notificationManager_.showPopup("ConesLoader", "Errore", "Errore nel parsing del CSV.", NotificationType::Error);
            continue;
        }

        loadedCones.push_back(cone);
        printf("Cono Caricato: Lon=%f, Lat=%f, ID=%d\n", cone.lon, cone.lat, cone.id);
    }

    file.close();

    if (loadedCones.empty()) {
        std::cerr << "Nessun cono valido caricato dal CSV: " << filePath << std::endl;
        notificationManager_.showPopup("ConesLoader", "Errore", "Nessun cono caricato dal CSV.", NotificationType::Error);
        return false;
    }

    notificationManager_.showPopup("ConesLoader", "Successo", "Coni caricati con successo.", NotificationType::Success);
    return true;
}
