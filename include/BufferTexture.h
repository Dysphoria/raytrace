

#ifndef BUFFERTEXTURE_H
#define BUFFERTEXTURE_H

#include "glfuncs.h"
#include "Buffer.h"
#include <stdexcept>

class BufferTexture{
public:
    GLuint tex;
    Buffer* buff;
    int numElements;
    //format:
    //R8{I,UI}, R16{I,UI,F}, R32{I,UI,F},
    //RG8{I,UI}, RG16{I,UI,F}, RG32{I,UI,F},
    //RGB32{I,UI,F},
    //RGBA8{I,UI}, RGBA16{I,UI,F}, RGBA32{I,UI,F}
    BufferTexture(int numElements, GLenum format){
        this->numElements = numElements;
        int size;
        switch(format){
            case GL_R8I:
            case GL_R8UI:
                size = numElements;
                break;
            case GL_R16I:
            case GL_R16UI:
            case GL_R16F:
            case GL_RG8I:
            case GL_RG8UI:
                size = numElements*2;
                break;
            case GL_R32I:
            case GL_R32UI:
            case GL_R32F:
            case GL_RG16I:
            case GL_RG16UI:
            case GL_RG16F:
            case GL_RGBA8I:
            case GL_RGBA8UI:
                size = numElements*4;
                break;
            case GL_RG32I:
            case GL_RG32UI:
            case GL_RG32F:
            case GL_RGBA16I:
            case GL_RGBA16UI:
            case GL_RGBA16F:
                size = numElements*8;
                break;
            case GL_RGB32I:
            case GL_RGB32UI:
            case GL_RGB32F:
                size = numElements*12;
                break;
            case GL_RGBA32I:
            case GL_RGBA32UI:
            case GL_RGBA32F:
                size = numElements*16;
                break;
            default:
                throw std::runtime_error("Bad format");
        }
        GLint mx;
        glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE,&mx);
        if( size > mx )
            throw std::runtime_error("Not enough capacity");
        buff = new Buffer(size);
        glGenTextures(1,&tex);
        glBindTexture(GL_TEXTURE_BUFFER,tex);
        glTexBuffer(GL_TEXTURE_BUFFER,format, buff->id);
        glBindTexture(GL_TEXTURE_BUFFER,0);
        
    }
    void* map(){
        return buff->map();
    }
    void unmap(){
        buff->unmap();
    }
    void bind(int attachment){
        glActiveTexture(GL_TEXTURE0+attachment);
        glBindTexture(GL_TEXTURE_BUFFER,tex);
    }
    static void unbind(int attachment){
        glActiveTexture(GL_TEXTURE0+attachment);
        glBindTexture(GL_TEXTURE_BUFFER,0);
    }
};


#endif /* BUFFERTEXTURE_H */

