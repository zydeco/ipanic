#include <Carbon/Carbon.h>

/* globals are evil */
struct panicData
{
    WindowRef   dim,window;
    int         screenW,screenH;
};

/* some functions */
CGImageRef  LoadPNGFromURL (CFURLRef url);
void        StartPanic(EventLoopTimerRef theTimer, void* userData);

int main(int argc, char ** argv)
{
    EventLoopTimerUPP       timerUPP;
    ProcessSerialNumber     psn;
    CGImageRef              bgImage;
    CFURLRef                url;
    CFTypeRef               cfVal;
    CFBundleRef             appBundle;
    Rect                    winRect;
    HIRect                  winRectHI;
    HIViewRef               bgImageView,bgWinView;
    WindowRef               bgWindow;
    struct panicData        panic;
    double                  bounceTime = 1.0, freezeTime = 1.0;
    
    /* load times */
    appBundle  = CFBundleGetMainBundle();
    cfVal = CFBundleGetValueForInfoDictionaryKey(appBundle,CFSTR("PanicBounceTime"));
    if (cfVal) CFNumberGetValue(cfVal,kCFNumberDoubleType,&bounceTime);
    cfVal = CFBundleGetValueForInfoDictionaryKey(appBundle,CFSTR("PanicFreezeTime"));
    if (cfVal) CFNumberGetValue(cfVal,kCFNumberDoubleType,&freezeTime);
    
    /* do some bouncing */
    usleep((int)(1000000 * bounceTime));
    
    /* make & load screenshot */
    system("screencapture -mxtpng /tmp/screenshot.png");
    url     = CFURLCreateWithString(NULL,CFSTR("file:///tmp/screenshot.png"),NULL);
    bgImage = LoadPNGFromURL(url);
    CFRelease(url);
    
    /* calculate coordinates */
    panic.screenW         = CGImageGetWidth(bgImage);
    panic.screenH         = CGImageGetHeight(bgImage);
    winRect.top           = 0;
    winRect.bottom        = panic.screenH;
    winRect.left          = 0;
    winRect.right         = panic.screenW;
    winRectHI.origin.x    = 0.0;
    winRectHI.origin.y    = 0.0;
    winRectHI.size.width  = (float)panic.screenW;
    winRectHI.size.height = (float)panic.screenH;
    
    /* create & populate window */
    CreateNewWindow(kDocumentWindowClass,
                    kWindowNoShadowAttribute|kWindowNoTitleBarAttribute|kWindowCompositingAttribute,
                    &winRect,&bgWindow);
    HIImageViewCreate(bgImage,&bgImageView);
    HIViewFindByID(HIViewGetRoot(bgWindow), kHIViewWindowContentID, &bgWinView);
    HIViewAddSubview(bgWinView,bgImageView);
    HIViewSetFrame(bgImageView,&winRectHI);
    HIViewSetVisible(bgImageView,true);
    ShowWindow(bgWindow);
    CGImageRelease(bgImage);
    
    /* hide cursor n' stuff */
    GetCurrentProcess(&psn);
    SetFrontProcess(&psn);
    SetSystemUIMode(kUIModeAllHidden,kUIOptionDisableProcessSwitch|kUIOptionDisableAppleMenu|kUIOptionDisableForceQuit|kUIOptionDisableSessionTerminate);
    HideCursor();
    
    /* event loop */
    timerUPP = NewEventLoopTimerUPP(StartPanic);
    InstallEventLoopTimer(GetMainEventLoop(),freezeTime*kEventDurationSecond,0,timerUPP,&panic,NULL);
    RunApplicationEventLoop();
    
    /* now we're done */
    SetSystemUIMode(kUIModeNormal,0);
    DisposeEventLoopTimerUPP(timerUPP);
    DisposeWindow(bgWindow);
    DisposeWindow(panic.dim);
    DisposeWindow(panic.window);
    system("rm -f /tmp/screenshot.png");
    return 0;
}

CGImageRef LoadPNGFromURL(CFURLRef url)
{
    CGDataProviderRef   provider;
    CGImageRef          image;
    provider = CGDataProviderCreateWithURL(url);
    image    = CGImageCreateWithPNGDataProvider(provider,NULL,false,kCGRenderingIntentDefault);
    CGDataProviderRelease(provider);
    return image;
}

void StartPanic(EventLoopTimerRef theTimer, void* userData)
{
    CGImageRef          panicImage;
    CFURLRef            url;
    CFBundleRef         appBundle;
    Rect                rect;
    HIRect              rectHI;
    int                 panicWidth,panicHeight;
    HIViewRef           panicImageView,panicWinView;
    long                systemVersion;
    struct panicData *  panic = (struct panicData*)userData;
    
    /* load panic image */
    Gestalt(gestaltSystemVersion, &systemVersion);
    appBundle  = CFBundleGetMainBundle();
    if (systemVersion >= 0x1060) 
        url    = CFBundleCopyResourceURL(appBundle,CFSTR("panic10.6"),CFSTR("png"),NULL);
    else
        url    = CFBundleCopyResourceURL(appBundle,CFSTR("panic"),CFSTR("png"),NULL);
    panicImage = LoadPNGFromURL(url);
    CFRelease(url);
    
    /* dim screen */
    rect.top    = 0 - panic->screenH;
    rect.bottom = 0;
    rect.left   = 0;
    rect.right  = panic->screenW;
    CreateNewWindow(kDocumentWindowClass,
                    kWindowNoShadowAttribute|kWindowNoTitleBarAttribute|kWindowCompositingAttribute,
                    &rect,&(panic->dim));
    SetThemeWindowBackground(panic->dim,kThemeBrushBlack,false);
    SetWindowAlpha(panic->dim,0.5);
    ShowWindow(panic->dim);
    rect.bottom = 0 - rect.top;
    rect.top    = 0;
    TransitionWindow(panic->dim,3,4,&rect);
    
    /* show panic */
    panicWidth         = CGImageGetWidth(panicImage);
    panicHeight        = CGImageGetHeight(panicImage);
    rect.top           = (panic->screenH/2) - (panicHeight/2);
    rect.bottom        = rect.top + panicHeight;
    rect.left          = (panic->screenW/2) - (panicWidth/2);
    rect.right         = rect.left + panicWidth;
    rectHI.origin.x    = 0.0;
    rectHI.origin.y    = 0.0;
    rectHI.size.width  = (float)panicWidth;
    rectHI.size.height = (float)panicHeight;
    CreateNewWindow(kDocumentWindowClass,
                    kWindowNoShadowAttribute|kWindowNoTitleBarAttribute|kWindowCompositingAttribute,
                    &rect,&(panic->window));
    HIImageViewCreate(panicImage,&panicImageView);
    HIViewFindByID(HIViewGetRoot(panic->window), kHIViewWindowContentID, &panicWinView);
    HIViewAddSubview(panicWinView,panicImageView);
    HIViewSetFrame(panicImageView,&rectHI);
    HIViewSetVisible(panicImageView,true);
    ShowWindow(panic->window);
    CGImageRelease(panicImage);
}
