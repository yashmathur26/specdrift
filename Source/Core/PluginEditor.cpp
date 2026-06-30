#include "PluginEditor.h"
#include "Parameters.h"

//==============================================================================
SpecdriftEditor::SpecdriftEditor(SpecdriftProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p)
{
    juce::ignoreUnused(processorRef);
    setOpaque(true);

    blurOrb   = std::make_unique<OrbKnobComponent>(p, Specdrift::ParamID::blur,   "Blur",   juce::Colour(0xff3d7aff), 0.0f);
    tiltOrb   = std::make_unique<OrbKnobComponent>(p, Specdrift::ParamID::tilt,   "Tilt",   juce::Colour(0xff6366f1), 0.0f);
    randomOrb = std::make_unique<OrbKnobComponent>(p, Specdrift::ParamID::random, "Random", juce::Colour(0xff8b5cf6), 0.0f);
    gateOrb   = std::make_unique<OrbKnobComponent>(p, Specdrift::ParamID::gate,   "Gate",   juce::Colour(0xffa855f7), 0.0f);
    mixOrb    = std::make_unique<OrbKnobComponent>(p, Specdrift::ParamID::mix,    "Mix",    juce::Colour(0xffc084fc), 0.0f);

    addAndMakeVisible(*blurOrb);
    addAndMakeVisible(*tiltOrb);
    addAndMakeVisible(*randomOrb);
    addAndMakeVisible(*gateOrb);
    addAndMakeVisible(*mixOrb);

    spectrogramComponent = std::make_unique<Specdrift::SpectrogramComponent>(p);
    addChildComponent(*spectrogramComponent);

    knobsModeButton.onClick = [this] {
        processorRef.setCurveEnabled(false);
        repaint();
    };
    knobsModeButton.setButtonText("Knobs");
    curveModeButton.onClick = [this] {
        processorRef.setCurveEnabled(true);
        repaint();
    };
    curveModeButton.setButtonText("Curve");
    clearCurveButton.onClick = [this] {
        processorRef.getCurveState().clear();
        processorRef.updateCurveLookup();
        repaint();
        if (spectrogramComponent != nullptr)
            spectrogramComponent->repaint();
    };
    clearCurveButton.setButtonText("Clear");
    for (auto* b : { &knobsModeButton, &curveModeButton, &clearCurveButton })
    {
        b->setLookAndFeel(&invisibleButtonLAF);
        b->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        b->setColour(juce::TextButton::textColourOffId, juce::Colour(0xff6a6a9a));
        addChildComponent(*b);
    }

    // EQ tab button
    eqTabButton.onClick = [this] {
        activeTab = 0;
        resized();
        repaint();
    };
    eqTabButton.setButtonText("");
    eqTabButton.setLookAndFeel(&invisibleButtonLAF);
    addChildComponent(eqTabButton);

    spectralViewButton.onClick = [this] {
        spectralViewExpanded = !spectralViewExpanded;
        spectrogramComponent->setVisible(spectralViewExpanded);
        setSize(kWidth, spectralViewExpanded ? kTotalHeightExpanded : kTotalHeightCollapsed);
        resized();
        repaint();
    };
    spectralViewButton.setButtonText("");
    spectralViewButton.setLookAndFeel(&invisibleButtonLAF);
    addAndMakeVisible(spectralViewButton);

    lowCutOrb  = std::make_unique<OrbKnobComponent>(p, Specdrift::ParamID::lowCut,  "Low Cut",  juce::Colour(0xff4c1d95), 20.0f);
    highCutOrb = std::make_unique<OrbKnobComponent>(p, Specdrift::ParamID::highCut, "High Cut", juce::Colour(0xff4c1d95), 20000.0f);
    addChildComponent(*lowCutOrb);
    addChildComponent(*highCutOrb);

    inputGainSlider  = std::make_unique<FooterSliderComponent>(p, Specdrift::ParamID::inputGain,  "IN");
    outputGainSlider = std::make_unique<FooterSliderComponent>(p, Specdrift::ParamID::outputGain, "OUT");
    addAndMakeVisible(*inputGainSlider);
    addAndMakeVisible(*outputGainSlider);

    setSize(kWidth, kTotalHeightCollapsed);
}

SpecdriftEditor::~SpecdriftEditor()
{
    spectralViewButton.setLookAndFeel(nullptr);
    knobsModeButton.setLookAndFeel(nullptr);
    curveModeButton.setLookAndFeel(nullptr);
    clearCurveButton.setLookAndFeel(nullptr);
    eqTabButton.setLookAndFeel(nullptr);
}

void SpecdriftEditor::paint(juce::Graphics& g)
{
    auto fullBounds = getLocalBounds().toFloat();
    auto W = fullBounds.getWidth();

    // Background
    juce::ColourGradient bgGradient(
        juce::Colour(0xff161630), 0.0f, 0.0f,
        juce::Colour(0xff0e0e20), W, fullBounds.getHeight(),
        false);
    bgGradient.addColour(0.5, juce::Colour(0xff1a1036));
    g.setGradientFill(bgGradient);
    g.fillRect(fullBounds);

    // Subtle top highlight
    juce::ColourGradient topGlow(
        juce::Colour(0x0dffffff), W * 0.5f, 0.0f,
        juce::Colours::transparentBlack, W * 0.5f, 60.0f,
        false);
    g.setGradientFill(topGlow);
    g.fillRect(fullBounds.removeFromTop(60.0f));

    // --- Header ---
    auto headerArea = getLocalBounds().toFloat().removeFromTop(static_cast<float>(kHeaderHeight));
    headerArea.removeFromLeft(28.0f);
    headerArea.removeFromRight(28.0f);

    g.setFont(juce::Font(juce::FontOptions(22.0f)));
    g.setColour(juce::Colour(0xffb4b4d8));
    g.drawText("S  P  E  C  D  R  I  F  T", headerArea.removeFromLeft(320.0f).toNearestInt(),
               juce::Justification::centredLeft, false);

    g.drawText("V E R S I O N  1 . 0", headerArea.removeFromRight(200.0f).toNearestInt(),
               juce::Justification::centredRight, false);

    // --- Divider (below orbs) ---
    float dividerY = static_cast<float>(kHeaderHeight + kOrbsHeight);
    {
        juce::ColourGradient dg(
            juce::Colours::transparentBlack, 40.0f, dividerY,
            juce::Colours::transparentBlack, W - 40.0f, dividerY, false);
        dg.addColour(0.5, juce::Colour(139, 92, 246).withAlpha(0.25f));
        g.setGradientFill(dg);
        g.fillRect(40.0f, dividerY, W - 80.0f, 1.0f);
    }

    if (spectralViewExpanded)
    {
        // Spectrogram border glow
        if (spectrogramComponent != nullptr)
        {
            auto specBounds = spectrogramComponent->getBounds().toFloat().expanded(1.0f);
            g.setColour(juce::Colour(139, 92, 246).withAlpha(0.12f));
            g.drawRoundedRectangle(specBounds, 6.0f, 1.0f);

            juce::ColourGradient innerShadow(
                juce::Colour(0x20000000), specBounds.getX(), specBounds.getY(),
                juce::Colours::transparentBlack, specBounds.getX(), specBounds.getY() + 8.0f, false);
            g.setGradientFill(innerShadow);
            g.fillRect(specBounds.getX() + 1, specBounds.getY() + 1, specBounds.getWidth() - 2, 8.0f);
        }

        // --- Tab bar: draw labels + active underline ---
        float tabBarY = dividerY + 1.0f + static_cast<float>(kSpectrogramHeight + kCurveBarHeight);
        auto tabBarArea = juce::Rectangle<float>(0.0f, tabBarY, W, static_cast<float>(kTabBarHeight));
        float tabInset = 40.0f;

        // "EQ" tab
        float tabTextX = tabInset;
        float tabW = 36.0f;
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        bool eqActive = (activeTab == 0);
        g.setColour(eqActive ? juce::Colour(0xffa78bfa) : juce::Colour(0xff5a5a80));
        g.drawText("EQ", static_cast<int>(tabTextX), static_cast<int>(tabBarY),
                   static_cast<int>(tabW), kTabBarHeight - 4,
                   juce::Justification::centredLeft, false);

        if (eqActive)
        {
            float underlineY = tabBarY + static_cast<float>(kTabBarHeight) - 3.0f;
            g.setColour(juce::Colour(0xff8b5cf6));
            g.fillRoundedRectangle(tabTextX, underlineY, tabW - 4.0f, 2.0f, 1.0f);
        }

        // Divider under tab bar
        float tabDivY = tabBarY + static_cast<float>(kTabBarHeight) - 1.0f;
        {
            juce::ColourGradient tdg(
                juce::Colours::transparentBlack, 40.0f, tabDivY,
                juce::Colours::transparentBlack, W - 40.0f, tabDivY, false);
            tdg.addColour(0.5, juce::Colour(80, 80, 140).withAlpha(0.15f));
            g.setGradientFill(tdg);
            g.fillRect(40.0f, tabDivY, W - 80.0f, 1.0f);
        }

    }

    // --- MORE / expand bar ---
    float barY = dividerY + 1.0f;
    if (spectralViewExpanded)
        barY += static_cast<float>(kSpectrogramHeight + kCurveBarHeight + kTabBarHeight + kTabContentHeight);
    auto barArea = juce::Rectangle<float>(0.0f, barY, W, static_cast<float>(kBarHeight));

    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.setColour(juce::Colour(0xff6a6a9a));
    juce::String barText = spectralViewExpanded ? "LESS  -" : "MORE  +";
    g.drawText(barText, barArea.toNearestInt(), juce::Justification::centred, false);

    // Flanking lines
    float lineY = barArea.getCentreY();
    float textHalf = 30.0f;
    float lineLeft = barArea.getCentreX() - textHalf - 8.0f;
    float lineRight = barArea.getCentreX() + textHalf + 8.0f;
    float lineInset = W * 0.22f;
    {
        juce::ColourGradient lg(
            juce::Colours::transparentBlack, lineInset, lineY,
            juce::Colour(139, 92, 246).withAlpha(0.12f), lineLeft, lineY, false);
        g.setGradientFill(lg);
        g.fillRect(lineInset, lineY, lineLeft - lineInset, 1.0f);
    }
    {
        juce::ColourGradient rg(
            juce::Colour(139, 92, 246).withAlpha(0.12f), lineRight, lineY,
            juce::Colours::transparentBlack, W - lineInset, lineY, false);
        g.setGradientFill(rg);
        g.fillRect(lineRight, lineY, W - lineInset - lineRight, 1.0f);
    }

    // Footer divider
    float footerDivY = barY + static_cast<float>(kBarHeight) - 1.0f;
    {
        juce::ColourGradient fg(
            juce::Colours::transparentBlack, 60.0f, footerDivY,
            juce::Colours::transparentBlack, W - 60.0f, footerDivY, false);
        fg.addColour(0.5, juce::Colour(80, 80, 140).withAlpha(0.12f));
        g.setGradientFill(fg);
        g.fillRect(60.0f, footerDivY, W - 120.0f, 1.0f);
    }
}

void SpecdriftEditor::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop(kHeaderHeight);

    auto orbsR = r.removeFromTop(kOrbsHeight);
    orbsR.reduce(20, 0);
    orbsR.removeFromTop(10);
    orbsR.removeFromBottom(16);

    int totalGap = 8 * 4;
    int availableWidth = orbsR.getWidth() - totalGap;
    int orbWidth = juce::jmin(availableWidth / 5, 130);
    int totalOrbsWidth = orbWidth * 5 + totalGap;
    int startX = orbsR.getX() + (orbsR.getWidth() - totalOrbsWidth) / 2;

    int orbX = startX;
    blurOrb->setBounds(orbX, orbsR.getY(), orbWidth, orbsR.getHeight());
    orbX += orbWidth + 8;
    tiltOrb->setBounds(orbX, orbsR.getY(), orbWidth, orbsR.getHeight());
    orbX += orbWidth + 8;
    randomOrb->setBounds(orbX, orbsR.getY(), orbWidth, orbsR.getHeight());
    orbX += orbWidth + 8;
    gateOrb->setBounds(orbX, orbsR.getY(), orbWidth, orbsR.getHeight());
    orbX += orbWidth + 8;
    mixOrb->setBounds(orbX, orbsR.getY(), orbWidth, orbsR.getHeight());

    r.removeFromTop(kDividerHeight);

    if (spectralViewExpanded)
    {
        auto specR = r.removeFromTop(kSpectrogramHeight);
        specR.reduce(24, 6);
        spectrogramComponent->setBounds(specR);

        // Curve bar: KNOBS / DRAW CURVE ... CLEAR
        auto curveBarR = r.removeFromTop(kCurveBarHeight);
        curveBarR.reduce(40, 0);
        int bw = 56;
        int gap = 12;
        knobsModeButton.setBounds(curveBarR.removeFromLeft(bw));
        curveBarR.removeFromLeft(gap);
        curveModeButton.setBounds(curveBarR.removeFromLeft(bw));
        clearCurveButton.setBounds(curveBarR.removeFromRight(bw));
        knobsModeButton.setVisible(true);
        curveModeButton.setVisible(true);
        clearCurveButton.setVisible(true);

        // Tab bar
        auto tabBarR = r.removeFromTop(kTabBarHeight);
        eqTabButton.setBounds(tabBarR.getX() + 30, tabBarR.getY(), 48, kTabBarHeight);
        eqTabButton.setVisible(true);

        // Tab content: EQ = two orbs (same style as top row)
        auto tabContentR = r.removeFromTop(kTabContentHeight);
        if (activeTab == 0)
        {
            int orbSize = juce::jmin(58, tabContentR.getWidth() / 2 - 24);
            int eqGap = 24;
            int totalW = orbSize * 2 + eqGap;
            int eqStartX = tabContentR.getX() + (tabContentR.getWidth() - totalW) / 2;
            lowCutOrb->setBounds(eqStartX, tabContentR.getY(), orbSize, tabContentR.getHeight());
            highCutOrb->setBounds(eqStartX + orbSize + eqGap, tabContentR.getY(), orbSize, tabContentR.getHeight());
            lowCutOrb->setVisible(true);
            highCutOrb->setVisible(true);
        }
        else
        {
            lowCutOrb->setVisible(false);
            highCutOrb->setVisible(false);
        }
    }
    else
    {
        knobsModeButton.setVisible(false);
        curveModeButton.setVisible(false);
        clearCurveButton.setVisible(false);
        eqTabButton.setVisible(false);
        lowCutOrb->setVisible(false);
        highCutOrb->setVisible(false);
    }

    auto barR = r.removeFromTop(kBarHeight);
    spectralViewButton.setBounds(barR);

    auto footerR = r.removeFromTop(kFooterHeight);
    footerR.reduce(28, 0);
    footerR.removeFromTop(4);
    footerR.removeFromBottom(8);

    int sliderWidth = juce::jmin((footerR.getWidth() - 24) / 2, 160);
    int totalSlidersWidth = sliderWidth * 2 + 24;
    int sliderStartX = footerR.getX() + (footerR.getWidth() - totalSlidersWidth) / 2;

    inputGainSlider->setBounds(sliderStartX, footerR.getY(), sliderWidth, footerR.getHeight());
    outputGainSlider->setBounds(sliderStartX + sliderWidth + 24, footerR.getY(), sliderWidth, footerR.getHeight());
}
