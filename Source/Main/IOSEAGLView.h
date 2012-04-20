// CrowApp.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGLDrawable.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

////////////////////////////////////////////////////////////////////////////////

@class EAGLView;

/*
 This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
 The view content is basically an EAGL surface you render your OpenGL scene into.
 Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
 */
@interface EAGLView : UIView {
    
@public
	
    /* The pixel dimensions of the backbuffer */
    GLint backingWidth;
    GLint backingHeight;
	
@private
	
	bool created;
	
    EAGLContext *context;
    
    /* OpenGL names for the renderbuffer and framebuffers used to render to this view */
    GLuint viewRenderbuffer, viewFramebuffer;
    
    /* OpenGL name for the depth buffer that is attached to viewFramebuffer, if it exists (0 if it does not exist) */
    GLuint depthRenderbuffer;
}

- (void)bindFramebuffer;
- (void)presentScene;
- (void)bindGL;
- (void)unbindGL;

@end
