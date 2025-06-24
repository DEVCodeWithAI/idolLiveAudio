#include "PresetManagerWindow.h"
#include "../../Application/Application.h"
#include "../../Data/LanguageManager/LanguageManager.h"
#include "../../Components/Helpers.h"

// <<< MODIFIED >>> Make this component a ChangeListener for language updates
class PresetManagerComponent : public juce::Component,
    public juce::ListBoxModel,
    public juce::Button::Listener,
    public juce::ChangeListener // <<< ADDED
{
public:
    PresetManagerComponent()
    {
        // <<< ADDED >>> Listen for language changes
        LanguageManager::getInstance().addChangeListener(this);

        addAndMakeVisible(listBox);
        listBox.setModel(this);
        listBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::darkgrey.darker());

        addAndMakeVisible(deleteButton);
        deleteButton.addListener(this);

        updatePresets();
        updateButtonText();
    }

    // <<< ADDED >>> Destructor to remove listener
    ~PresetManagerComponent() override
    {
        LanguageManager::getInstance().removeChangeListener(this);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(10);
        deleteButton.setBounds(bounds.removeFromBottom(30));
        bounds.removeFromBottom(10);
        listBox.setBounds(bounds);
    }

    // ListBoxModel overrides
    int getNumRows() override { return presetFiles.size(); }
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected)
            g.fillAll(juce::Colours::blue.withAlpha(0.3f));

        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        if (rowNumber < presetFiles.size())
            g.drawText(presetFiles[rowNumber].getFileNameWithoutExtension(), 5, 0, width - 10, height, juce::Justification::centredLeft);
    }

    void buttonClicked(juce::Button* button) override
    {
        if (button == &deleteButton)
        {
            auto selectedRow = listBox.getSelectedRow();
            if (selectedRow >= 0 && selectedRow < presetFiles.size())
            {
                auto& lang = LanguageManager::getInstance();
                auto presetName = presetFiles[selectedRow].getFileNameWithoutExtension();

                juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,
                    lang.get("presetManagerWindow.deleteTitle"),
                    lang.get("presetManagerWindow.deleteMessage").replace("{{presetName}}", presetName),
                    lang.get("presetbar.yes"),
                    lang.get("presetbar.no"),
                    nullptr,
                    juce::ModalCallbackFunction::create([this, presetName](int result)
                        {
                            if (result == 1) // OK
                            {
                                getSharedPresetManager().deletePreset(presetName);
                                updatePresets();
                            }
                        }));
            }
        }
    }

    // <<< ADDED >>> Callback to handle language changes
    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (source == &LanguageManager::getInstance())
        {
            updateButtonText();
        }
    }

private:
    void updatePresets()
    {
        presetFiles = getSharedPresetManager().getPresetDirectory().findChildFiles(juce::File::findFiles, false, "*.xml");
        listBox.updateContent();
        repaint();
    }

    void updateButtonText()
    {
        deleteButton.setButtonText(LanguageManager::getInstance().get("presetManagerWindow.deleteButton"));
    }

    juce::ListBox listBox;
    juce::TextButton deleteButton;
    juce::Array<juce::File> presetFiles;
};


PresetManagerWindow::PresetManagerWindow()
    : DocumentWindow(LanguageManager::getInstance().get("presetManagerWindow.title"),
        juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
        allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(300, 400, 600, 800);
    setContentOwned(new PresetManagerComponent(), true);
    centreWithSize(350, 500);
    setVisible(true);
}

PresetManagerWindow::~PresetManagerWindow() {}

void PresetManagerWindow::closeButtonPressed() {
    // <<< MODIFIED >>> This no longer deletes itself.
    // It just becomes invisible. The Application class will destroy it on shutdown.
    setVisible(false);
}