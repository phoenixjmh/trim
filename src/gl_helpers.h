#include "gl_includes.h"

namespace graphics{

static unsigned int CreateShaderProgram()
{

    const char *vertShader =
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 model;\n"
        "void main() {\n"
        "    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);\n"
        "    TexCoord = aTexCoord;\n"
        "}\n";

    const char *fragShader = "#version 330 core\n"
                             "in vec2 TexCoord;\n"
                             "out vec4 FragColor;\n"
                             "uniform sampler2D tex;\n"
                             "void main() {\n"
                             "    FragColor = texture(tex, TexCoord);\n"
                             "}\n";

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertShader, NULL);
    glCompileShader(vert);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragShader, NULL);
    glCompileShader(frag);

    unsigned int shader = glCreateProgram();
    glAttachShader(shader, vert);
    glAttachShader(shader, frag);
    glLinkProgram(shader);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return shader;
}

void glfwErrorCallback(int error, const char *description)
{
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}
}