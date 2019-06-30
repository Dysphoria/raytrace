#ifndef PROGRAM_H
#define PROGRAM_H

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include "glfuncs.h"
using namespace std;

class Program{
    GLuint prog;
public:
    Program(string csfile){
        ifstream in(csfile.c_str());
        if(!in.good())
            throw runtime_error("Cannot load "+csfile);
        string src;
        getline(in,src,'\0');
        int handle = glCreateShader(GL_COMPUTE_SHADER);
        GLint length = src.length();
        const char* S[] = {src.c_str()};
        glShaderSource(handle,1,S,&length);
        glCompileShader(handle);
        char infolog[4096];
        glGetShaderInfoLog(handle,sizeof(infolog),&length,infolog);
        if(length > 0 ){
            cout << "When compiling " << csfile << ":\n";
            cout << infolog << "\n";
        }
        int status;
        glGetShaderiv(handle,GL_COMPILE_STATUS,&status);
        if(status == 0 )
            throw runtime_error("Cannot compile shader "+csfile);
        prog = glCreateProgram();
        glAttachShader(prog,handle);
        glLinkProgram(prog);
        glGetProgramInfoLog(prog, sizeof(infolog), &length, infolog);
        if( length > 0 ){
            cout << "When linking " << csfile << ":\n";
            cout << infolog << "\n";
        }
        glGetProgramiv(prog,GL_LINK_STATUS,&status);
        if( status == 0 ){
            throw runtime_error("Could not link "+csfile);
        }
    }
    
    void use(){
        glUseProgram(prog);
    }
    
    void dispatch(int xsize, int ysize, int zsize){
        glDispatchCompute(xsize,ysize,zsize);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
};
#endif /* PROGRAM_H */
