/*! \file EAGLView.mm
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup main
*/

#import "EAGLView.h"
#import "AppDelegate.h"
#import <QuartzCore/QuartzCore.h>
#include <Engine/App.h>
#include <Engine/Game/Game.h>

@implementation EAGLView

@synthesize context;

// You must implement this method
+ (Class)layerClass {
    return [CAEAGLLayer class];
}

//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder  {
    
    self = [super initWithCoder:coder];
	
	// Get the layer
	CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
	
	eaglLayer.opaque = YES;
	eaglLayer.drawableProperties =
		[NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
			kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil
		];
	
	[self setMultipleTouchEnabled:YES];
	
	context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
		
	if (!context || ![EAGLContext setCurrentContext:context]) {
		[self release];
		return nil;
	}
	
	m_initialized = false;
    return self;
}

- (void)bindGL {
	[EAGLContext setCurrentContext:context];
}

- (void)unbindGL {
	[EAGLContext setCurrentContext:nil];
}

- (void)bindFramebuffer {
	if (s_app && s_app->initialized)
		r::gls.BindBuffer(GL_FRAMEBUFFER, m_viewFramebuffer);
	else
		glBindFramebuffer(GL_FRAMEBUFFER, m_viewFramebuffer);
}

- (void)presentScene {
	if (s_app && s_app->initialized)
		r::gls.BindBuffer(GL_RENDERBUFFER, m_viewRenderbuffer);
	else
		glBindRenderbuffer(GL_RENDERBUFFER, m_viewRenderbuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER];

	GLenum attachments[] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT};
	glDiscardFramebufferEXT(GL_FRAMEBUFFER, 2, attachments);
}

- (void)layoutSubviews  {
	[super layoutSubviews];
	
    if (!m_initialized) {
		// do this here so we get a properly sized UIView on iPad.
		CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
		self.contentScaleFactor = [[UIScreen mainScreen] scale];
		eaglLayer.contentsScale = [[UIScreen mainScreen] scale];
		
		[self bindGL];
		
		glGenFramebuffers(1, &m_viewFramebuffer);
		glGenRenderbuffers(1, &m_viewRenderbuffer);
		
		glBindFramebuffer(GL_FRAMEBUFFER, m_viewFramebuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, m_viewRenderbuffer);
		[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)self.layer];
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_viewRenderbuffer);
		
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
		
		COut(C_Debug) << "OpenGL Back Buffer Size: (" << width << ", " << height << ")" << std::endl;
		
		glGenRenderbuffers(1, &m_depthRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);
		r::ZTextures.Get().Inc(width*height*3, 0);
				
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
			return;
		}
		
		glViewport(0, 0, width, height);
				
		m_initialized = true;
		[self unbindGL];
		
		s_app->glView = self;
		
		// kick off the game launch now:
		[s_app showSplash];
		[s_app startRefresh];
	
	}
}

- (void)dealloc  {
    
    if ([EAGLContext currentContext] == context) {
        [EAGLContext setCurrentContext:nil];
    }
    
    [context release];  
    [super dealloc];
}

- (void)dispatchTouch:(UITouch *)touch {
	if (!s_app) 
		return;
	
	InputEvent::Type type;
	switch ([touch phase])
	{
		case UITouchPhaseBegan:
			type = InputEvent::T_TouchBegin;
			break;
		case UITouchPhaseMoved:
			type = InputEvent::T_TouchMoved;
			break;
		case UITouchPhaseStationary:
			type = InputEvent::T_TouchStationary;
			break;
		case UITouchPhaseEnded:
			type = InputEvent::T_TouchEnd;
			break;
		case UITouchPhaseCancelled:
			type = InputEvent::T_TouchCancelled;
			break;
	}
	
	CGPoint	 location = [touch locationInView:self];
	U64 millis = (U64)([touch timestamp] * 1000.0);
	float scale = [[UIScreen mainScreen] scale];
	
	InputEvent event;
	event.touch = touch;
	event.type = type;
	event.time = (int)(millis & 0xffffff);
	event.data[0] = location.x*scale;
	event.data[1] = location.y*scale;
	event.data[2] = [touch tapCount];
	
//	COut(C_Debug) << "Touch (" << event.data[0] << ", " << event.data[1] << "), (" << location.x << ", " << location.y << "), (" << event.time << "ms, " << millis << "ms" << std::endl;
	
	App::Get()->game->PostInputEvent(event);
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch *touch in touches) {
		[self dispatchTouch:touch];
	}	
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch *touch in touches) {
		[self dispatchTouch:touch];
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch *touch in touches) {
		[self dispatchTouch:touch];
	}
}

@end
