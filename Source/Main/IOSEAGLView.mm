//
//  IOSEAGLView.mm
//

#import "IOSEAGLView.h"
#import "IOSAppDelegate.h"

float __IOS_ContentScale();

int s_screenWidth = 0;
int s_screenHeight = 0;
int s_eosVersion = 0;
bool s_discardColorHint=true;
EAGLContext *s_context=0;

void __IOS_ScreenSize(int &w, int &h)
{
	w = s_screenWidth;
	h = s_screenHeight;
}

int __IOS_EOSVersion()
{
	return s_eosVersion;
}

void __IOS_SetDiscardColorHint(bool value)
{
	s_discardColorHint = value;
}

// A class extension to declare private methods
@interface EAGLView ()

@property (nonatomic, retain) EAGLContext *context;

@end


@implementation EAGLView

@synthesize context;

// You must implement this method
+ (Class)layerClass {
    return [CAEAGLLayer class];
}


//The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder 
{
    
    self = [super initWithCoder:coder];
	
	// Get the layer
	CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
	
	eaglLayer.opaque = YES;
	eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
									[NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking, 
									kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
	
	[self setMultipleTouchEnabled:YES];
	
	s_eosVersion = 2;
	context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
	s_context = context;
	
	if (!context || ![EAGLContext setCurrentContext:context]) 
	{
		[self release];
		return nil;
	}
	
	created = false;
    return self;
}

- (void)bindGL
{
	[EAGLContext setCurrentContext:context];
}

- (void)unbindGL
{
	[EAGLContext setCurrentContext:nil];
}

- (void)bindFramebuffer
{
	if (s_app && s_app->initialized)
		r::gls.BindBuffer(GL_FRAMEBUFFER, viewFramebuffer);
	else
		glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);
}

- (void)presentScene
{
	GLenum attachment = GL_DEPTH_ATTACHMENT;
	glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, &attachment);
	
    if (s_discardColorHint)
    {
		attachment = GL_COLOR_ATTACHMENT0;
		glDiscardFramebufferEXT(GL_FRAMEBUFFER, 1, &attachment);
	}
	
	if (s_app && s_app->initialized)
		r::gls.BindBuffer(GL_RENDERBUFFER, viewRenderbuffer);
	else
		glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)layoutSubviews 
{
	[super layoutSubviews];
	
    if (!created)
	{ // do this here so we get a properly sized UIView on iPad.
		CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
		self.contentScaleFactor = __IOS_ContentScale();
		eaglLayer.contentsScale = __IOS_ContentScale();
		
		[self bindGL];
		
		glGenFramebuffers(1, &viewFramebuffer);
		glGenRenderbuffers(1, &viewRenderbuffer);
		
		glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
		[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)self.layer];
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, viewRenderbuffer);
		
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
		
		s_screenWidth = backingWidth;
		s_screenHeight = backingHeight;
		
		COut(C_Debug) << "GLBackingSize: (" << backingWidth << ", " << backingHeight << ")" << std::endl;
		
		if (USE_DEPTH_BUFFER) {
			glGenRenderbuffers(1, &depthRenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24_OES, backingWidth, backingHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
			r::ZTextures.Get().Inc(backingWidth*backingHeight*3, 0);
		}
		
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
			return;
		}
		
		glViewport(0, 0, backingWidth, backingHeight);
				
		created = true;
		[self unbindGL];
		
		s_app->glView = self;
		
		// kick this off here so we have backing sizes.
		[s_app showSplash];
		[s_app startRefresh];
	}
}

- (void)dealloc 
{
    
    if ([EAGLContext currentContext] == context) {
        [EAGLContext setCurrentContext:nil];
    }
    
    [context release];  
    [super dealloc];
}

- (void)dispatchTouch:(UITouch *)touch
{
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
	float millis = (float)([touch timestamp] * 1000.0);
	float scale = __IOS_ContentScale();
	
	InputEvent event;
	event.touch = touch;
	event.type = type;
	event.time = (int)millis;
	event.data[0] = location.x*scale;
	event.data[1] = location.y*scale;
	event.data[2] = [touch tapCount];
	
//	COut(C_Debug) << "Touch (" << event.data[0] << ", " << event.data[1] << "), (" << location.x << ", " << location.y << ")" << std::endl;
	
#if defined(GAMETHREAD)
	if (s_app && s_app->gameThread) 
	{
		GameThread::Lock L(s_app->gameThread->m);
		s_app->gameThread->events.push_back(event);
	}
#else
	App::Get()->game->PostInputEvent(event);
#endif
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	for (UITouch *touch in touches) 
	{
		[self dispatchTouch:touch];
	}	
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	for (UITouch *touch in touches) 
	{
		[self dispatchTouch:touch];
	}
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	for (UITouch *touch in touches) 
	{
		[self dispatchTouch:touch];
	}
}

@end
