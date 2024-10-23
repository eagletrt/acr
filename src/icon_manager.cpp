#include "icon_manager.hpp"
#include "stb_image.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <cstdio>
#include "notifications.hpp"

std::vector<IconInfo> icons;

ImTextureID loadImagePNG(const char* path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
    if (data == NULL) {
        printf("Error loading image: %s\n", path);
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload image data to the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return (ImTextureID)(intptr_t)tex;
}

// Function to load icons
void loadIcons() {
    ImTextureID infoIcon = loadImagePNG("../assets/icons/info.png"); // Ensure the path is correct
    if (infoIcon != 0) {
        icons.push_back({ "Info", infoIcon, ImVec2(16, 16) });
        printf("Loaded icon: Info\n");
    } else {
        printf("Failed to load icon: info.png\n");
    }

    ImTextureID successIcon = loadImagePNG("../assets/icons/success.png"); // Ensure the path is correct
    if (successIcon != 0) {
        icons.push_back({ "Success", successIcon, ImVec2(16, 16) });
        printf("Loaded icon: Success\n");
    } else {
        printf("Failed to load icon: success.png\n");
    }

    ImTextureID errorIcon = loadImagePNG("../assets/icons/error.png");
    if (errorIcon != 0) {
        icons.push_back({ "Error", errorIcon, ImVec2(16, 16) });
        printf("Loaded icon: Error\n");
    } else {
        printf("Failed to load icon: error.png\n");
    }

}

ImTextureID getIconTexture(const std::string& name) {
    for (const auto& icon : icons) {
        if (icon.name == name) {
            return icon.texture;
        }
    }
    return 0; 
}