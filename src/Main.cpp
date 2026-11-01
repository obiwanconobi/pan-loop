#include <JuceHeader.h>

#include "app/DarkLookAndFeel.h"
#include "app/MainComponent.h"
#include "core/AppConfig.h"

class PanLoopApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String&) override
    {
        AppConfig::initialise();

        lookAndFeel = std::make_unique<DarkLookAndFeel>();
        juce::LookAndFeel::setDefaultLookAndFeel (lookAndFeel.get());

        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();

        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
        lookAndFeel.reset();

        AppConfig::shutdown();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String&) override
    {
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name)
            : DocumentWindow (name,
                              DarkLookAndFeel::background(),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);
            setResizable (true, false);
            setResizeLimits (1040, 700, 2400, 1600);
            centreWithSize (1220, 880);
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<DarkLookAndFeel> lookAndFeel;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (PanLoopApplication)
