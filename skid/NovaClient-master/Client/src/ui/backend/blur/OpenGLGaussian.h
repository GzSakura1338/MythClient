// Included in OpenGLBlur.cpp's private namespace. No GL state escapes a pass.
struct GaussianApi {
    GLuint(APIENTRY* createShader)(GLenum){};
    void(APIENTRY* shaderSource)(GLuint,GLsizei,const char* const*,const GLint*){};
    void(APIENTRY* compileShader)(GLuint){};
    void(APIENTRY* getShaderiv)(GLuint,GLenum,GLint*){};
    void(APIENTRY* deleteShader)(GLuint){};
    GLuint(APIENTRY* createProgram)(){};
    void(APIENTRY* attachShader)(GLuint,GLuint){};
    void(APIENTRY* linkProgram)(GLuint){};
    void(APIENTRY* getProgramiv)(GLuint,GLenum,GLint*){};
    void(APIENTRY* deleteProgram)(GLuint){};
    GLint(APIENTRY* getUniformLocation)(GLuint,const char*){};
    void(APIENTRY* uniform1i)(GLint,GLint){};
    void(APIENTRY* uniform2f)(GLint,GLfloat,GLfloat){};
    void(APIENTRY* uniform2fv)(GLint,GLsizei,const GLfloat*){};
    void(APIENTRY* genVertexArrays)(GLsizei,GLuint*){};
    void(APIENTRY* deleteVertexArrays)(GLsizei,const GLuint*){};
    void(APIENTRY* bindSampler)(GLuint,GLuint){};
} gaussianApi;
GLuint gaussianProgram=0,gaussianVao=0;
GLint gaussianDirection=-1,gaussianTaps=-1,gaussianCount=-1,gaussianScene=-1;
bool LoadGaussian() {
#define NOVA_GAUSSIAN_GL(field,name) gaussianApi.field=reinterpret_cast<decltype(gaussianApi.field)>(gl::resolve(name));if(!gaussianApi.field)return false;
    NOVA_GAUSSIAN_GL(createShader,"glCreateShader") NOVA_GAUSSIAN_GL(shaderSource,"glShaderSource")
    NOVA_GAUSSIAN_GL(compileShader,"glCompileShader") NOVA_GAUSSIAN_GL(getShaderiv,"glGetShaderiv") NOVA_GAUSSIAN_GL(deleteShader,"glDeleteShader")
    NOVA_GAUSSIAN_GL(createProgram,"glCreateProgram") NOVA_GAUSSIAN_GL(attachShader,"glAttachShader") NOVA_GAUSSIAN_GL(linkProgram,"glLinkProgram")
    NOVA_GAUSSIAN_GL(getProgramiv,"glGetProgramiv") NOVA_GAUSSIAN_GL(deleteProgram,"glDeleteProgram")
    NOVA_GAUSSIAN_GL(getUniformLocation,"glGetUniformLocation") NOVA_GAUSSIAN_GL(uniform1i,"glUniform1i")
    NOVA_GAUSSIAN_GL(uniform2f,"glUniform2f") NOVA_GAUSSIAN_GL(uniform2fv,"glUniform2fv")
    NOVA_GAUSSIAN_GL(genVertexArrays,"glGenVertexArrays") NOVA_GAUSSIAN_GL(deleteVertexArrays,"glDeleteVertexArrays")
#undef NOVA_GAUSSIAN_GL
    gaussianApi.bindSampler=reinterpret_cast<decltype(gaussianApi.bindSampler)>(gl::resolve("glBindSampler"));
    return gl::useProgram() && gl::activeTexture() && gl::bindVertexArray();
}
void DeleteGaussian() {
    if(gaussianProgram && gaussianApi.deleteProgram)gaussianApi.deleteProgram(gaussianProgram);
    if(gaussianVao && gaussianApi.deleteVertexArrays)gaussianApi.deleteVertexArrays(1,&gaussianVao);
    gaussianProgram=gaussianVao=0;
}
bool CreateGaussian() {
    if(gaussianProgram)return true;
    const char* vertex=R"(#version 130
out vec2 uv;
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);uv=p;gl_Position=vec4(p*2.0-1.0,0,1);}
)";
    const char* fragment=R"(#version 130
in vec2 uv;
out vec4 color;
uniform sampler2D scene;
uniform vec2 direction;
uniform vec2 taps[10];
uniform int count;
void main(){
    vec3 sum=texture(scene,uv).rgb*taps[0].x;
    for(int i=1;i<count;++i){vec2 offset=direction*taps[i].y;
        sum+=(texture(scene,uv+offset).rgb+texture(scene,uv-offset).rgb)*taps[i].x;}
    color=vec4(sum,1);
}
)";
    GLuint shaders[2]{};bool good=true;
    for(int i=0;i<2;++i) {
        shaders[i]=gaussianApi.createShader(i?0x8B30:0x8B31);
        const auto source=i?fragment:vertex;gaussianApi.shaderSource(shaders[i],1,&source,nullptr);
        gaussianApi.compileShader(shaders[i]);GLint status=0;gaussianApi.getShaderiv(shaders[i],0x8B81,&status);good&=status!=0;
    }
    if(good) {
        gaussianProgram=gaussianApi.createProgram();for(auto shader:shaders)gaussianApi.attachShader(gaussianProgram,shader);
        gaussianApi.linkProgram(gaussianProgram);GLint status=0;gaussianApi.getProgramiv(gaussianProgram,0x8B82,&status);good=status!=0;
    }
    for(auto shader:shaders)if(shader)gaussianApi.deleteShader(shader);
    if(!good){DeleteGaussian();return false;}
    gaussianApi.genVertexArrays(1,&gaussianVao);
    gaussianScene=gaussianApi.getUniformLocation(gaussianProgram,"scene");
    gaussianDirection=gaussianApi.getUniformLocation(gaussianProgram,"direction");
    gaussianTaps=gaussianApi.getUniformLocation(gaussianProgram,"taps");
    gaussianCount=gaussianApi.getUniformLocation(gaussianProgram,"count");
    return gaussianVao!=0;
}
struct GaussianState {
    GLint program{},vao{},active{},sampler{},viewport[4]{},polygon[2]{};
    GLboolean color[4]{};
    const GLenum caps[5]{GL_BLEND,GL_DEPTH_TEST,GL_STENCIL_TEST,GL_CULL_FACE,0x8C89};
    GLboolean enabled[5]{};
    GaussianState() {
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(0x84C0);
        if(gaussianApi.bindSampler){glGetIntegerv(0x8919,&sampler);gaussianApi.bindSampler(0,0);}
        glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_POLYGON_MODE,polygon);glGetBooleanv(GL_COLOR_WRITEMASK,color);
        for(int i=0;i<5;++i){enabled[i]=glIsEnabled(caps[i]);glDisable(caps[i]);}
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    }
    ~GaussianState() {
        glUseProgram(program);glBindVertexArray(vao);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
        glColorMask(color[0],color[1],color[2],color[3]);glPolygonMode(GL_FRONT_AND_BACK,polygon[0]);
        for(int i=0;i<5;++i)if(enabled[i])glEnable(caps[i]);
        if(gaussianApi.bindSampler)gaussianApi.bindSampler(0,sampler);
        glActiveTexture(active);
    }
};
void GaussianFilter(const Level& from,const Level& to,const GaussianPass& pass) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,to.fbo);glViewport(0,0,to.w,to.h);
    glUseProgram(gaussianProgram);glBindVertexArray(gaussianVao);glBindTexture(GL_TEXTURE_2D,from.texture);
    gaussianApi.uniform1i(gaussianScene,0);gaussianApi.uniform1i(gaussianCount,pass.count);
    gaussianApi.uniform2f(gaussianDirection,pass.x,pass.y);gaussianApi.uniform2fv(gaussianTaps,10,pass.taps[0].data());
    glDrawArrays(GL_TRIANGLES,0,3);
}
