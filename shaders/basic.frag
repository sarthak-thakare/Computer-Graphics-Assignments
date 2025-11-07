#version 330 core
in vec4 litColor;
in vec2 uv;
out vec4 fragColor;

uniform sampler2D tex;
uniform int useTexture;

void main(){
    if(useTexture==1){
        vec4 t = texture(tex, uv);
        // Modulate texture with lighting intensity (use litColor as light factor assuming base color was 1)
        fragColor = vec4(t.rgb * litColor.rgb, t.a);
    } else {
        fragColor = litColor;
    }
}
