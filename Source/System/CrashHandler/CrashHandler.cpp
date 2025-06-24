#include "CrashHandler.h"
#include "../../Application/Application.h"
#include "../../Data/LanguageManager/LanguageManager.h"

namespace
{
    // ==============================================================================
    /** A simple, self-contained window to display a crash report. */
    class CrashReportWindow : public juce::DocumentWindow
    {
    public:
        CrashReportWindow(const juce::String& report)
            : DocumentWindow(LanguageManager::getInstance().get("crash.title"),
                juce::Desktop::getInstance().getDefaultLookAndFeel()
                .findColour(juce::ResizableWindow::backgroundColourId),
                closeButton)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, true);
            setResizeLimits(400, 300, 1024, 768);

            // This component will hold the content of our window
            class ContentComponent : public juce::Component
            {
            public:
                ContentComponent(const juce::String& reportText)
                {
                    addAndMakeVisible(explanation);
                    explanation.setText(LanguageManager::getInstance().get("crash.message"), juce::dontSendNotification);
                    explanation.setJustificationType(juce::Justification::centred);

                    addAndMakeVisible(reportEditor);
                    reportEditor.setMultiLine(true);
                    reportEditor.setReadOnly(true);

                    // <<< FINAL, DEFINITIVE, CORRECT FIX >>>
                    {
                        juce::String monoFontName = juce::Font::getDefaultMonospacedFontName();

                        if (monoFontName.isEmpty())
                            monoFontName = "Courier New";

                        // 1. Create a default font using the modern constructor.
                        // This avoids all deprecation warnings.
                        juce::Font monoFont{ juce::FontOptions{} };

                        // 2. Configure the font using its public, non-deprecated methods.
                        monoFont.setTypefaceName(monoFontName);
                        monoFont.setHeight(12.0f);

                        // 3. Apply the fully configured font.
                        reportEditor.setFont(monoFont);
                    }

                    reportEditor.setText(reportText);

                    addAndMakeVisible(copyButton);
                    copyButton.setButtonText(LanguageManager::getInstance().get("crash.copyButton"));
                    copyButton.onClick = [this]
                        {
                            juce::SystemClipboard::copyTextToClipboard(reportEditor.getText());
                        };
                }

                void resized() override
                {
                    auto bounds = getLocalBounds().reduced(10);
                    explanation.setBounds(bounds.removeFromTop(40));
                    copyButton.setBounds(bounds.removeFromBottom(30));
                    bounds.removeFromBottom(10);
                    reportEditor.setBounds(bounds);
                }

            private:
                juce::Label explanation;
                juce::TextEditor reportEditor;
                juce::TextButton copyButton;
            };

            setContentOwned(new ContentComponent(report), true);
            centreWithSize(700, 500);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            // When closed, quit the application.
            JUCEApplication::getInstance()->quit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrashReportWindow)
    };

    // ==============================================================================
    /** This is the actual function that gets called when the application crashes. */
    void handleCrash(void* userData)
    {
        juce::String report = "An unhandled exception occurred.\n\n";

        report += "=== APPLICATION INFO ===\n";
        report += "Name: " + juce::String(ProjectInfo::projectName) + "\n";
        report += "Version: " + juce::String(ProjectInfo::versionString) + "\n";
        report += "Time: " + juce::Time::getCurrentTime().toString(true, true) + "\n\n";

        report += "=== SYSTEM INFO ===\n";
        report += "OS: " + juce::SystemStats::getOperatingSystemName() + "\n\n";

        report += "=== CRASH DETAILS ===\n";

        if (userData != nullptr)
        {
            report += juce::String::fromUTF8(static_cast<const char*>(userData));
        }
        else
        {
            report += "No additional crash information available.";
        }

        new juce::Component::SafePointer<CrashReportWindow>(new CrashReportWindow(report));
    }

} // namespace


// ==============================================================================
void CrashHandler::install()
{
    juce::SystemStats::setApplicationCrashHandler(handleCrash);
}