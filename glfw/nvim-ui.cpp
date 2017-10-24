#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#include "HandmadeMath.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "flextgl/flextGL.h"
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol_gfx.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <chrono>
#include "FontTex.hpp"

void glfw_error(int error, const char* description) {
	fprintf(stderr, "Glfw error: %s", description);
}

typedef struct {
    float iGlobalTime;
    hmm_vec3 uCameraPos;
    hmm_vec3 uCameraDir;
    hmm_vec2 uViewport;
} params_t;

int main() {

    auto ftex = new FontTex();

    if (!glfwInit()) {
		exit(1);
	}
	glfwSetErrorCallback(glfw_error);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(1000, 800, "Neovim", NULL, NULL);
	if (!window) {
		exit(1);
	}
	glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    flextInit(window);

    // Setup sokol
    sg_desc desc = {};
    sg_setup(&desc);
    assert(sg_isvalid());

    float vertices[] = {
        -1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, -1.0f
    };
    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.size = sizeof(vertices);
    vbuf_desc.content = vertices;
    sg_buffer vbuf = sg_make_buffer(&vbuf_desc);

    uint16_t indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    sg_buffer_desc ibuf_desc = {};
    ibuf_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    ibuf_desc.size = sizeof(indices);
    ibuf_desc.content = indices;
    sg_buffer ibuf = sg_make_buffer(&ibuf_desc);


    // Uniform block
    sg_shader_uniform_desc fs_iGlobalTime = {};
    fs_iGlobalTime.name = "iGlobalTime";
    fs_iGlobalTime.type = SG_UNIFORMTYPE_FLOAT;
    sg_shader_uniform_desc fs_uCameraPos = {};
    fs_uCameraPos.name = "uCameraPos";
    fs_uCameraPos.type = SG_UNIFORMTYPE_FLOAT3;
    sg_shader_uniform_desc fs_uCameraDir = {};
    fs_uCameraDir.name = "uCameraDir";
    fs_uCameraDir.type = SG_UNIFORMTYPE_FLOAT3;
    sg_shader_uniform_desc fs_uViewport = {};
    fs_uViewport.name = "uViewport";
    fs_uViewport.type = SG_UNIFORMTYPE_FLOAT2;
    sg_shader_uniform_block_desc fs_uniforms = {};
    fs_uniforms.size = sizeof(params_t);
    fs_uniforms.uniforms[0] = fs_iGlobalTime;
    fs_uniforms.uniforms[1] = fs_uCameraPos;
    fs_uniforms.uniforms[2] = fs_uCameraDir;
    fs_uniforms.uniforms[3] = fs_uViewport;

    // Font texture
    sg_image_desc font_tex = {};
    font_tex.width = FontTex::bitmap_width;
    font_tex.height = FontTex::bitmap_height;
    font_tex.pixel_format = SG_PIXELFORMAT_L8;
    font_tex.min_filter = SG_FILTER_NEAREST;
    font_tex.mag_filter = SG_FILTER_NEAREST;
    sg_image_content font_tex_imgsub = {};
    font_tex_imgsub.subimage[0][0] = (sg_subimage_content){
        ftex->bitmap,
        sizeof(ftex->bitmap)
    };
    font_tex.content = font_tex_imgsub;
    sg_image font_tex_img = sg_make_image(&font_tex);

    // Font coords texture
    sg_image_desc font_coords = {};
    font_coords.width = 2;
    font_coords.height = FontTex::total_chars;
    font_coords.pixel_format = SG_PIXELFORMAT_RGBA32F;
    font_coords.min_filter = SG_FILTER_NEAREST;
    font_coords.mag_filter = SG_FILTER_NEAREST;
    sg_image_content font_coords_imgsub = {};
    font_coords_imgsub.subimage[0][0] = (sg_subimage_content){
        ftex->font_coords_tex,
        sizeof(ftex->font_coords_tex)
    };
    font_coords.content = font_coords_imgsub;
    sg_image font_coords_img = sg_make_image(&font_coords);

    // Shaders
    sg_shader_desc shader_desc = {};
    shader_desc.vs.source = "#version 330\n"
                            "in vec2 position;"
                            "out vec2 vPosition;"
                            "void main() {"
                            "   vPosition = position;"
                            "   gl_Position = vec4(position, 0.0, 1.0);"
                            "}";
    shader_desc.fs.source = "#version 330\n"
                            "uniform sampler2D font_tex;"
                            "uniform sampler2D font_coords;"
                            "uniform vec2 uViewport;"
                            "in vec2 vPosition;"
                            "out vec4 frag_color;"
                            "void main() {"
                            "   vec2 pCoord = ((vPosition+vec2(1.0, -1.0))*vec2(0.5,-0.5))*uViewport;"
                            "   float curRow = floor(pCoord.y / 20.0);"
                            "   float curChar = floor(pCoord.x / 10.0);"
                            "   float charStartY = (curRow+1) * 20.0;"
                            "   float charStartX = curChar * 10.0;"
                            "   float charCoordKey = ((curRow+1) * curChar) / 512.0;"
                            "   vec4 fontCoords = texture(font_coords, vec2(0.25, charCoordKey));"
                            "   vec4 fontCoords2 = texture(font_coords, vec2(0.75, charCoordKey));"
                            "   float coordXOff = fontCoords2.r;"
                            "   float coordYOff = fontCoords2.g;"
                            "   float coordStartX = fontCoords.r;"
                            "   float coordStartY = fontCoords.g;"
                            "   float coordEndX = fontCoords.b;"
                            "   float coordEndY = fontCoords.a;"
                            "   vec2 pCoordNorm = pCoord / vec2(1024.0,1024.0);"
                            "   float xDist = pCoordNorm.x - (charStartX / 1024.0) - coordXOff;"
                            "   float yDist = pCoordNorm.y - (charStartY / 1024.0) - coordYOff;"
                            "   if (xDist < 0 || yDist < 0 || xDist > (coordEndX - coordStartX) || yDist > (coordEndY - coordStartY)) {"
                            "       if (yDist > (coordEndY - coordStartY)) {"
                            "           frag_color = vec4(0.5,0.8,0.5,1.0);"
                            "       } else {"
                            "           frag_color = vec4(0.8,0.5,0.5,1.0);"
                            "       }"
                            "       frag_color = vec4(0.2,0.0,0.4,1.0);"
                            "   } else {"
                            "       vec2 uv = vec2(coordStartX + xDist, coordStartY + yDist);"
                            "       float texColor = texture(font_tex, uv).x;"
                            "       frag_color = fontCoords;"
                            "       vec3 fg_color = vec3(0.8, 0.8, 1.0);"
                            "       vec3 bg_color = vec3(0.2, 0.0, 0.4);"
                            "       vec3 mixed = mix(bg_color, fg_color, texColor);"
                            "       frag_color = vec4(mixed, 1.0);"
                            "   }"
                            "}"
                            "";
    shader_desc.fs.images[0] = (sg_shader_image_desc){
        "font_tex",
        SG_IMAGETYPE_2D
    };
    shader_desc.fs.images[1] = (sg_shader_image_desc){
        "font_coords",
        SG_IMAGETYPE_2D
    };
    shader_desc.fs.uniform_blocks[0] = fs_uniforms;
    sg_shader shd = sg_make_shader(&shader_desc);

    // Vertex attribute
    sg_vertex_attr_desc pos_attr = {};
    pos_attr.offset = 0;
    pos_attr.format = SG_VERTEXFORMAT_FLOAT2;
    
    // Vertex layout
    sg_vertex_layout_desc vlayout_desc = {};
    vlayout_desc.stride = 8;
    vlayout_desc.attrs[0] = pos_attr;

    // Pipeline
    sg_pipeline_desc pipeline_desc = {};
    pipeline_desc.shader = shd;
    pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
    pipeline_desc.vertex_layouts[0] = vlayout_desc;
    sg_pipeline pip = sg_make_pipeline(&pipeline_desc);

    // Draw state
    sg_draw_state draw_state = {};
    draw_state.pipeline = pip;
    draw_state.vertex_buffers[0] = vbuf;
    draw_state.index_buffer = ibuf;
    draw_state.fs_images[0] = font_tex_img;
    draw_state.fs_images[1] = font_coords_img;

    sg_pass_action pass_action = {};

    auto start_time = std::chrono::high_resolution_clock::now();
    params_t fs_params;
    fs_params.uCameraPos = HMM_Vec3(8.0f, 5.0f, 7.0f);
    fs_params.uCameraDir = HMM_NormalizeVec3(HMM_Vec3(-8.0f, -5.0f, -7.0f));
	while (!glfwWindowShouldClose(window)) {
        int currWidth, currHeight;
        glfwGetFramebufferSize(window, &currWidth, &currHeight);
        auto tick_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed_time = tick_time - start_time;
        typedef std::chrono::milliseconds ms;
        ms elapsed_time_ms = std::chrono::duration_cast<ms>(elapsed_time);
        float elapsed_time_float = elapsed_time_ms.count() / 1000.0f;
        fs_params.iGlobalTime = elapsed_time_float;
        fs_params.uViewport = HMM_Vec2((float)currWidth, (float)currHeight);
        sg_begin_default_pass(&pass_action, currWidth, currHeight);
        sg_apply_draw_state(&draw_state);
        sg_apply_uniform_block(SG_SHADERSTAGE_FS, 0, &fs_params, sizeof(fs_params));
        sg_draw(0, 6, 1);
        sg_end_pass();
        sg_commit();
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

    sg_shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();

    //int c = getchar();
    return 0;
}