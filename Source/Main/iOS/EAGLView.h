/*! \file EAGLView.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup main
*/

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
    GLint width;
    GLint height;
	
@private
	
	bool m_initialized;
    
    GLuint m_viewRenderbuffer;
	GLuint m_viewFramebuffer;
    GLuint m_depthRenderbuffer;
}

@property (nonatomic, retain) EAGLContext *context;

- (void)bindFramebuffer;
- (void)presentScene;
- (void)bindGL;
- (void)unbindGL;

@end
