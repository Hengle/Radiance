// GLPrototypes.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#define APIENTRYP *

enum {
	GL_MODELVIEW,
	GL_PROJECTION
};

typedef GLsizeiptr GLsizeiptrARB;
typedef GLintptr GLintptrARB;
typedef unsigned int GLhandleARB;
typedef char GLcharARB;
typedef double GLdouble;

#define GL_ARRAY_BUFFER_ARB GL_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER_ARB GL_ELEMENT_ARRAY_BUFFER
#define GL_WRITE_ONLY_ARB GL_WRITE_ONLY_OES
#define GL_STATIC_DRAW_ARB GL_STATIC_DRAW
#define GL_DYNAMIC_DRAW_ARB GL_DYNAMIC_DRAW
#define GL_STREAM_DRAW_ARB GL_STREAM_DRAW
#define GL_VERTEX_SHADER_ARB GL_VERTEX_SHADER
#define GL_OBJECT_COMPILE_STATUS_ARB GL_COMPILE_STATUS
#define GL_OBJECT_LINK_STATUS_ARB GL_LINK_STATUS
#define GL_FRAGMENT_SHADER_ARB GL_FRAGMENT_SHADER
#define GL_DEPTH_COMPONENT16_ARB GL_DEPTH_COMPONENT16_OES
#define GL_DEPTH_COMPONENT24_ARB GL_DEPTH_COMPONENT24_OES
#define GL_DEPTH_COMPONENT32_ARB GL_DEPTH_COMPONENT32_OES
#define GL_RENDERBUFFER_EXT GL_RENDERBUFFER
#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER
#define GL_DEPTH_ATTACHMENT_EXT GL_DEPTH_ATTACHMENT
#define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT1_EXT GL_COLOR_ATTACHMENT1
#define GL_COLOR_ATTACHMENT2_EXT GL_COLOR_ATTACHMENT2
#define GL_COLOR_ATTACHMENT3_EXT GL_COLOR_ATTACHMENT3
#define GL_COLOR_ATTACHMENT4_EXT GL_COLOR_ATTACHMENT4
#define GL_COLOR_ATTACHMENT5_EXT GL_COLOR_ATTACHMENT5
#define GL_COLOR_ATTACHMENT6_EXT GL_COLOR_ATTACHMENT6
#define GL_COLOR_ATTACHMENT7_EXT GL_COLOR_ATTACHMENT7
#define GL_FRAMEBUFFER_COMPLETE_EXT GL_FRAMEBUFFER_COMPLETE
#define GL_VERTEX_ARRAY_BINDING_APPLE GL_VERTEX_ARRAY_BINDING