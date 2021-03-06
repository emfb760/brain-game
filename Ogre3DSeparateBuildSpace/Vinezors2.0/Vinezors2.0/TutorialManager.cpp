//
//  TutorialManager.cpp
//  Vinezors2.0
//
//  Created by Calvin Phung on 5/15/14.
//
//

#include "TutorialManager.h"

extern Util::ConfigGlobal globals;

#include <iostream>
#include <sstream>

// Inserts newlines by extracting each word and counting the character limit
// All previous whitespaces are removed
std::string insertNL(std::string message)
{
#if defined(OGRE_IS_IOS)
    const int NUM_CHAR_PER_NEWLINE = 20;
#else
    const int NUM_CHAR_PER_NEWLINE = 25;
#endif
    
    std::stringstream ss(message);
    
    int linelen = 0;
    std::string ret = "";
    std::string input;
    while (ss >> input)
    {
#if defined(OGRE_IS_IOS)
        int sublen = input.length() + 2; // i-pad makes space character two-wide
#else
        int sublen = input.length() + 1; // Add one for the space character
#endif
        if (linelen + sublen <= 25)
        {
            if (linelen > 0)
                ret += " ";
            ret += input;
            linelen += sublen;
        }
        else
        {
            ret += "\n";
            ret += input;
            linelen = input.length();
        }
        
        while (ss.peek() == '\n') // try to find hardcoded newlines and put them in
        {
            ret += ss.get(); // extract newline
            linelen = 0;
        }
    }
    return ret;
}

TutorialManager::TutorialManager()
: popupOverlay(NULL), popupWindowBackground(NULL), popupSubWindowBackground(NULL), slides(), visitedSlide(), enableSlides(true), slideNo(0), yoffset(0.0)
{
    visitedSlide = std::vector<bool>(NUM_TUTORIAL_SLIDES, false);
    alloc();
}

TutorialManager::~TutorialManager()
{
    dealloc();
}

bool TutorialManager::isEnabled() const
{
    return enableSlides;
}

void TutorialManager::enable()
{
    enableSlides = true;
    
    // For now, re-enabling the manager resets the slides visited
    for (int i = 0; i < visitedSlide.size(); ++i)
        visitedSlide[i] = false;
}

void TutorialManager::disable()
{
    enableSlides = false;
}

// Returns the set of slides depending on the lesson type
std::vector<TutorialSlide> TutorialManager::getSlides(TutorialSlidesType type) const
{
    std::vector<TutorialSlide> ret;
    switch (type)
    {
        case TUTORIAL_SLIDES_WELCOME:
            ret.push_back(TutorialSlide(insertNL("Welcome to Recall!"), "General/ScreenBackground2"));
            ret.push_back(TutorialSlide(insertNL("We tutorial windows can explain elements of the game as they are introduced to you."), "General/ScreenBackground2"));
            ret.push_back(TutorialSlide(insertNL("Click the Play button in this screen to begin.\n\nYou may re-enable/disable these tutorials."), "General/ScreenBackground2"));
            break;
        case TUTORIAL_SLIDES_ZERO_BACK:
            ret.push_back(TutorialSlide(insertNL("Swipe to grab the fuel cells to go faster!\n\nComplete the level before time runs out!"), "General/ScreenBackground2"));
            break;
        case TUTORIAL_SLIDES_ONE_BACK:
            ret.push_back(TutorialSlide(insertNL("Gather matching signals! A matching one-back is the signal the same as the one you passed.\n\nGather enough matches before time runs out!"), "General/ScreenBackground2"));
            ret.push_back(TutorialSlide(insertNL("If you select an incorrect match,\nyou will move slower and lose time to complete the stage!"), "General/ScreenBackground2"));
            break;
        case TUTORIAL_SLIDES_TWO_BACK:
            ret.push_back(TutorialSlide(insertNL("Gather matching two-backs! A two back match is a signal matching every other signals.\n\nGather enough matches before time runs out!"), "General/ScreenBackground2"));
            break;
        case TUTORIAL_SLIDES_COLOR_SOUND:
            ret.push_back(TutorialSlide(insertNL("Challenge:\nLook for matches through color and sound!"), "General/ScreenBackground2"));
            break;
        case TUTORIAL_SLIDES_SHAPE_SOUND:
            ret.push_back(TutorialSlide(insertNL("Challenge:\nLook for matches through shape and sound!"), "General/ScreenBackground2"));
            break;
        case TUTORIAL_SLIDES_SOUND_ONLY:
            ret.push_back(TutorialSlide(insertNL("Challenge:\nLook for matches through listening to only sound!"), "General/ScreenBackground2"));
            break;
        case TUTORIAL_SLIDES_HOLDOUT:
            ret.push_back(TutorialSlide(insertNL("Challenge:\nSignals will randomly cloak their color, shape, or sound! Pay attention to more than one feature to identify matches!"), "General/ScreenBackground2"));
            break;
        case TUTORIAL_SLIDES_OBSTACLE:
            ret.push_back(TutorialSlide(insertNL("Your first obstacle has appeared!\nAvoid running into these as they hinder your performance to complete the level."), "General/ScreenBackground2"));
            break;
        case TUTORIAL_SLIDES_TIME_WARP:
            ret.push_back(TutorialSlide(insertNL("Congratulations! You grabbed your first time warp! It adds twenty seconds to the clock.\nNever waste the opportunity to grab it when it appears!"), "General/ScreenBackground2"));
            break;
        default:
            break;
    }
    return ret;
}

// Appends the set of slides if it has not been seen yet
void TutorialManager::setSlides(TutorialSlidesType type)
{
    if (isEnabled() && !visitedSlide[type])
    {
        setSlides(getSlides(type));
        visitedSlide[type] = true;
    }
}

// Appends the set of slides, and if the slides do get set, set this overlay to be visible
void TutorialManager::setSlides(const std::vector<TutorialSlide> & slides)
{
    for (int i = 0; i < slides.size(); ++i)
        this->slides.push_back(slides[i]);
    slideNo = 0;
    if (slides.size() > 0)
    {
        show();
        yoffset = -0.50;
        adjust();
        updateOverlay();
    }
}

bool TutorialManager::hasPreviousSlide() const
{
    int newSlideNo = slideNo - 1;
    return newSlideNo >= 0 && newSlideNo < slides.size();
}

bool TutorialManager::hasNextSlide() const
{
    int newSlideNo = slideNo + 1;
    return newSlideNo >= 0 && newSlideNo < slides.size();
}

void TutorialManager::setPreviousSlide()
{
    slideNo--;
    if (slideNo < 0) slideNo = 0;
    updateOverlay();
}

void TutorialManager::setNextSlide()
{
    slideNo++;
    if (slideNo >= slides.size()) slideNo = slides.size() - 1;
    updateOverlay();
}

void TutorialManager::updateOverlay()
{
    std::string text = slides[slideNo].message;
    std::string subwindowbg = slides[slideNo].background;
    popupText->setCaption(text);
    popupSubWindowBackground->setMaterialName(subwindowbg);
    popupSlideNoText->setCaption(Util::toStringInt(slideNo + 1) + " / " + Util::toStringInt(slides.size()));
    if (hasPreviousSlide())
        popupGoLeftBackground->setMaterialName("General/ButtonGoUp");
    else
        popupGoLeftBackground->setMaterialName("General/ButtonGoUpGray");
    popupGoRightBackground->setMaterialName("General/ButtonGoDown");
}

void TutorialManager::update(float elapsed)
{
    if (yoffset < 0.0)
    {
        float yspeed = (-yoffset) * 3.5;
        if (yspeed < 0.05) yspeed = 0.05;
        yoffset += (yspeed * elapsed);
        if (yoffset > 0.0)
            yoffset = 0.0;
        adjust();
    }
}

void TutorialManager::hide()
{
    popupOverlay->hide();
}

void TutorialManager::show()
{
    popupOverlay->show();
}

bool TutorialManager::isHidden() const
{
    return !popupOverlay->isVisible();
}

bool TutorialManager::isVisible() const
{
    return popupOverlay->isVisible();
}

std::string TutorialManager::queryButtons(Vector2 target) const
{
    Vector2 comp = globals.convertToPercentScreen(target);
    for (int i = 0; i < buttons.size(); ++i)
    {
        if ((buttons[i].overlay && buttons[i].overlay->isVisible()) && buttons[i].isInside(comp) && buttons[i].active)
        {
            return buttons[i].name;
        }
    }
    return "";
}

void TutorialManager::processInput(Vector2 target)
{
    std::string query = queryButtons(target);
    if (query == "goleft")
    {
        setPreviousSlide();
    }
    else if (query == "goright")
    {
        if (hasNextSlide())
            setNextSlide();
        else
        {
            slides.clear();
            slideNo = 0;
            hide();
        }
    }
    else if (query == "exit")
    {
        slides.clear();
        slideNo = 0;
        hide();
    }
}

void TutorialManager::adjust()
{
    popupWindowBackground->setMetricsMode(GMM_RELATIVE);
    popupWindowBackground->setPosition(0.250, yoffset + 0.110);
    popupWindowBackground->setDimensions(0.50, 0.50);
    popupWindowBackground->setMaterialName("General/ScreenBackground1");
    
    popupSubWindowBackground->setMetricsMode(GMM_RELATIVE);
    popupSubWindowBackground->setPosition(0.025, 0.025);
    popupSubWindowBackground->setDimensions(0.450, 0.450);
    
    float bw = 0.075;
    float bh = bw * globals.screenWidth / globals.screenHeight;
    buttons[BUTTON_GOLEFT].setButton("goleft", popupOverlay, GMM_RELATIVE, Vector2(0.250, 0.350), Vector2(bw, bh), popupGoLeftBackground, NULL);
    buttons[BUTTON_GORIGHT].setButton("goright", popupOverlay, GMM_RELATIVE, Vector2(0.350, 0.350), Vector2(bw, bh), popupGoRightBackground, NULL);
    buttons[BUTTON_EXIT].setButton("exit", popupOverlay, GMM_RELATIVE, Vector2(0.150, 0.350), Vector2(bw, bh), popupExitBackground, NULL);
    popupExitBackground->setMaterialName("General/ExitButton2");
    
    popupText->setMetricsMode(GMM_RELATIVE);
    popupText->setAlignment(TextAreaOverlayElement::Left);
    popupText->setPosition(0.025, 0.025);
    popupText->setCharHeight(0.025 * FONT_SZ_MULT);
    popupText->setColour(ColourValue::ColourValue(1.0, 1.0, 0.0));
    popupText->setFontName("Arial");
    
    popupSlideNoText->setMetricsMode(GMM_RELATIVE);
    popupSlideNoText->setAlignment(TextAreaOverlayElement::Left);
    popupSlideNoText->setPosition(0.025, 0.400);
    popupSlideNoText->setCharHeight(0.025 * FONT_SZ_MULT);
    popupSlideNoText->setColour(ColourValue::ColourValue(1.0, 1.0, 0.0));
    popupSlideNoText->setFontName("Arial");
}

void TutorialManager::dealloc()
{
    if (popupOverlay)
        OgreFramework::getSingletonPtr()->m_pOverlayMgr->destroy(popupOverlay);
    if (popupSubWindowBackground)
        OgreFramework::getSingletonPtr()->m_pOverlayMgr->destroyOverlayElement(popupSubWindowBackground);
    if (popupWindowBackground)
        OgreFramework::getSingletonPtr()->m_pOverlayMgr->destroyOverlayElement(popupWindowBackground);
    if (popupGoLeftBackground)
        OgreFramework::getSingletonPtr()->m_pOverlayMgr->destroyOverlayElement(popupGoLeftBackground);
    if (popupGoRightBackground)
        OgreFramework::getSingletonPtr()->m_pOverlayMgr->destroyOverlayElement(popupGoRightBackground);
    if (popupText)
        OgreFramework::getSingletonPtr()->m_pOverlayMgr->destroyOverlayElement(popupText);
    if (popupSlideNoText)
        OgreFramework::getSingletonPtr()->m_pOverlayMgr->destroyOverlayElement(popupSlideNoText);
    popupSubWindowBackground = NULL;
    popupWindowBackground = NULL;
    popupGoLeftBackground = NULL;
    popupGoRightBackground = NULL;
    popupText = NULL;
    popupSlideNoText = NULL;
}

void TutorialManager::alloc()
{
    popupOverlay = OgreFramework::getSingletonPtr()->m_pOverlayMgr->create("TutorialPopupOverlay");
    popupWindowBackground = static_cast<PanelOverlayElement*>(
                                                              OgreFramework::getSingletonPtr()->m_pOverlayMgr->createOverlayElement("Panel", "TutorialPopupWindowBackground"));
    popupSubWindowBackground = static_cast<PanelOverlayElement*>(
                                                                 OgreFramework::getSingletonPtr()->m_pOverlayMgr->createOverlayElement("Panel", "TutorialPopupSubWindowBackground"));
    
    popupGoLeftBackground = static_cast<PanelOverlayElement*>(
                                                              OgreFramework::getSingletonPtr()->m_pOverlayMgr->createOverlayElement("Panel", "TutorialPopupGoLeftBackground"));
    popupGoRightBackground = static_cast<PanelOverlayElement*>(
                                                              OgreFramework::getSingletonPtr()->m_pOverlayMgr->createOverlayElement("Panel", "TutorialPopupGoRightBackground"));
    popupExitBackground = static_cast<PanelOverlayElement*>(
                                                            OgreFramework::getSingletonPtr()->m_pOverlayMgr->createOverlayElement("Panel", "TutorialPopupExitBackground"));

    // Allocate Resources
    popupText = static_cast<TextAreaOverlayElement*>(
                                                     OgreFramework::getSingletonPtr()->m_pOverlayMgr->createOverlayElement("TextArea", "TutorialPopupText"));
    popupSlideNoText = static_cast<TextAreaOverlayElement*>(
                                                     OgreFramework::getSingletonPtr()->m_pOverlayMgr->createOverlayElement("TextArea", "TutorialSlideNoText"));
    
    popupOverlay->add2D(popupWindowBackground);
    popupWindowBackground->addChild(popupSubWindowBackground);
    popupSubWindowBackground->addChild(popupGoLeftBackground);
    popupSubWindowBackground->addChild(popupGoRightBackground);
    popupSubWindowBackground->addChild(popupExitBackground);
    popupSubWindowBackground->addChild(popupText);
    popupSubWindowBackground->addChild(popupSlideNoText);
    
    buttons = std::vector<HudButton>(3);
    
    // Set this overlay as the frontmost item on the screen
    adjust();
    popupOverlay->setZOrder(300);
}

std::ostream& operator<<(std::ostream& out, const TutorialManager& tutorialMgr)
{
    std::cout << "Saving Tutorial Settings\n";
    
    out << tutorialMgr.enableSlides << " " << tutorialMgr.visitedSlide.size() << std::endl;
    for (int i = 0; i < tutorialMgr.visitedSlide.size(); ++i)
        out << tutorialMgr.visitedSlide[i] << " ";
     
    return out;
}

std::istream& operator>>(std::istream& in, TutorialManager& tutorialMgr)
{
    std::cout << "Loading Tutorial Settings\n";
    int input;
    in >> input;
    tutorialMgr.enableSlides = input;
    in >> input;
    tutorialMgr.visitedSlide = std::vector<bool>(input);
    for (int i = 0; i < tutorialMgr.visitedSlide.size(); ++i)
    {
        in >> input;
        tutorialMgr.visitedSlide[i] = input;
    }
    return in;
}

