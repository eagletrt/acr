
#include "icon_manager.hpp"
#include "stb_image.h"
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <cstdio>
#include "notifications.hpp"
#include "config.hpp"

IconManager::IconManager() {}

IconManager::~IconManager()
{

    for (auto &icon : icons_)
    {
        if (icon.texture != 0)
        {
            GLuint texID = static_cast<GLuint>(reinterpret_cast<intptr_t>(icon.texture));
            glDeleteTextures(1, &texID);
            icon.texture = 0;
        }
    }
}

ImTextureID IconManager::loadIconImage(const char *path, NotificationManager &notificationManager)
{
    int width, height, channels;
    unsigned char *data = stbi_load(path, &width, &height, &channels, 4);
    if (data == NULL)
    {
        printf("Error loading image: %s\n", path);

        notificationManager.showPopup("IconManager", "Error", "Failed to load image: " + std::string(path), NotificationType::Error);
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    return (ImTextureID)(intptr_t)tex;
}

void IconManager::loadIcons(NotificationManager &notificationManager)
{
    ImTextureID infoIcon = loadIconImage(ICONS_DIR "info.png", notificationManager);
    if (infoIcon != 0)
    {
        icons_.push_back({"Info", infoIcon, ImVec2(16, 16)});
        printf("Loaded icon: Info\n");
    }

    ImTextureID successIcon = loadIconImage(ICONS_DIR "success.png", notificationManager);
    if (successIcon != 0)
    {
        icons_.push_back({"Success", successIcon, ImVec2(16, 16)});
        printf("Loaded icon: Success\n");
    }

    ImTextureID errorIcon = loadIconImage(ICONS_DIR "error.png", notificationManager);
    if (errorIcon != 0)
    {
        icons_.push_back({"Error", errorIcon, ImVec2(16, 16)});
        printf("Loaded icon: Error\n");
    }
}

ImTextureID IconManager::getIconTexture(const std::string &name) const
{
    for (const auto &icon : icons_)
    {
        if (icon.name == name)
        {
            return icon.texture;
        }
    }
    return 0;
}
