
#include "image_loader.h"
#include "gl_includes.h"

#define STB_IMAGE_IMPLEMENTATION
#include <iostream>

#include "stbi_image.h"
namespace GUI {
    Image LoadTexture(const char *filename)
    {
        Image image;
        unsigned char* data = stbi_load(filename, &image.width, &image.height, &image.channels, 4);
        if (!data)
        {
            std::cout<<"Error => STB :: Failed to load image @ "<<filename<<"\n";
            return image;
        }

        glGenTextures(1, &image.ID);
        glBindTexture(GL_TEXTURE_2D, image.ID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return image;
    }
} // GUI