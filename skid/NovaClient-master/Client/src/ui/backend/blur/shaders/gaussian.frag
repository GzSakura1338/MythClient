#version 450
layout(location=0) in vec2 uv;
layout(location=0) out vec4 color;
layout(set=0,binding=0) uniform sampler2D scene;
layout(push_constant) uniform Gaussian {
    vec2 direction;
    int count;
    float padding;
    vec4 pairs[5];
} kernel;
vec2 tap(int i) { return (i&1)!=0?kernel.pairs[i/2].zw:kernel.pairs[i/2].xy; }
void main() {
    vec3 sum=texture(scene,uv).rgb*tap(0).x;
    for(int i=1;i<kernel.count;++i) {
        vec2 t=tap(i);vec2 offset=kernel.direction*t.y;
        sum+=(texture(scene,uv+offset).rgb+texture(scene,uv-offset).rgb)*t.x;
    }
    color=vec4(sum,1);
}
