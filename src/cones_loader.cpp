#include "cones_loader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include "notifications.hpp"
#include <algorithm>

// Mutex per proteggere l'accesso al vettore cones
extern std::mutex renderLock;
extern std::vector<cone_t> cones;

// Funzione di trimming per rimuovere spazi bianchi
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

// cones_loader.cpp

bool loadConesFromCSV(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Errore nell'apertura del file CSV: " << filePath << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    std::vector<cone_t> loadedCones;

    // Leggi e salta la riga di intestazione
    if (std::getline(file, line)) {
        lineNumber++;
        // Opzionalmente, puoi verificare che la riga di intestazione sia corretta
    }

    while (std::getline(file, line)) {
        lineNumber++;
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        // Utilizza ',' come delimitatore
        while (std::getline(ss, token, ',')) {
            tokens.push_back(trim(token));
        }

        // Assicurati di avere almeno 5 campi (timestamp, cone_id, cone_name, lat, lon)
        if (tokens.size() < 5) {
            std::cerr << "Formato non valido nel CSV alla riga " << lineNumber << ": " << line << std::endl;
            continue;
        }

        cone_t cone;
        try {
            
            cone.lat = std::stof(tokens[3]);
            cone.lon = std::stof(tokens[4]);

            // Parsing cone_id e assegnazione del tipo di cono
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
                    cone.id = CONE_ID_YELLOW; // Valore di default
                    break;
            }

            // Parsing altitudine se disponibile
            if (tokens.size() >= 6) {
                cone.alt = std::stof(tokens[5]);
            } else {
                cone.alt = 0.0f; // Valore di default
            }

            // Parsing timestamp se necessario
            try {
                cone.timestamp = std::stoul(tokens[0]); // Usa stoul per interi più grandi
            } catch (...) {
                cone.timestamp = 0.0f;
            }

        }
        catch (const std::exception& e) {
            std::cerr << "Errore nel parsing del CSV alla riga " << lineNumber << ": " << e.what() << std::endl;
            continue;
        }

        loadedCones.push_back(cone);
        printf("Loaded Cone: Lon=%f, Lat=%f, ID=%d\n", cone.lon, cone.lat, cone.id);
    }

    file.close();

    if (loadedCones.empty()) {
        std::cerr << "Nessun cono valido caricato dal CSV: " << filePath << std::endl;
        return false;
    }

    {
        std::unique_lock<std::mutex> lck(renderLock);
        cones.insert(cones.end(), loadedCones.begin(), loadedCones.end());
    }

    return true;
}


void clearLoadedCones() {
    std::unique_lock<std::mutex> lck(renderLock);
    cones.clear();
}