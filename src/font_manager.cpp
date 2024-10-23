#include "font_manager.hpp"
#include <filesystem>
#include <algorithm>
#include <cstdio>
#include "notifications.hpp"

// Definizione delle variabili globali
std::vector<FontInfo> availableFonts;
int selectedFontIndex = 0; // Indice della font selezionata
float fontScale = 1.0f; // Scala iniziale della font

// Funzione per caricare le font dalla directory
void loadFontsFromDirectory(ImGuiIO& io, const std::string& fontsDir) {
    // Estensioni supportate per i file di font
    std::vector<std::string> extensions = { ".ttf", ".otf" };

    for (const auto& entry : std::filesystem::directory_iterator(fontsDir)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();
            // Verifica se il file ha un'estensione supportata
            if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                // Estrai il nome della font dal nome del file
                std::string name = entry.path().stem().string();
                // Carica la font con la dimensione predefinita
                ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f);
                if (font) {
                    availableFonts.push_back(FontInfo{ name, font });
                    printf("Font caricata: %s\n", name.c_str());
                }
                else {
                    printf("Impossibile caricare la font: %s\n", path.c_str());
                }
            }
        }
    }

    // Se nessuna font personalizzata è stata caricata, carica la font predefinita
    if (availableFonts.empty()) {
        availableFonts.push_back(FontInfo{ "Default", io.Fonts->AddFontDefault() });
        printf("Nessuna font personalizzata trovata. Font predefinita caricata.\n");
    }
}

// Funzione per inizializzare le font
void initializeFonts(ImGuiIO& io, const std::string& fontsDir) {
    loadFontsFromDirectory(io, fontsDir);

    // Imposta la font predefinita
    if (!availableFonts.empty()) {
        io.FontDefault = availableFonts[selectedFontIndex].font;
    }
}
