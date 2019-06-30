#ifndef BUFFER_H
#define BUFFER_H

#include "glfuncs.h"

class Buffer{
public:
    GLuint id;
    int bytesize;
    Buffer(int bytesize){
        this->bytesize = bytesize;
        glGenBuffers(1,&id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER,id);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER,bytesize,NULL,
                GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER,0);
    }
    void* map(){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER,id);
        return glMapBuffer(GL_SHADER_STORAGE_BUFFER,GL_READ_WRITE);
    }
    void unmap(){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER,id);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    void bind(int attachment){
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER,attachment,id);
    }
	static void unbind(int attachment) {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, attachment, 0);
	}
};


#endif /* BUFFER_H */

