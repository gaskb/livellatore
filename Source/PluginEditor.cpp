#include "PluginEditor.h"

namespace livellatore
{

namespace
{
    // ID 0 e' riservato da juce::ComboBox per "nessuna selezione": i preset
    // di fabbrica occupano 1..N, i preset utente partono da questa base.
    constexpr int userPresetIdBase = 1000;
}

LivellatoreAudioProcessorEditor::LivellatoreAudioProcessorEditor (LivellatoreAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      inputGainSlider (p.apvts, ParamID::inputGain, "Input Gain"),
      targetLevelSlider (p.apvts, ParamID::targetLufs, "Target Level"),
      attackSlider (p.apvts, ParamID::attack, "Attack"),
      releaseSlider (p.apvts, ParamID::release, "Release"),
      gateThresholdSlider (p.apvts, ParamID::gateThreshold, "Gate Threshold"),
      limiterSlider (p.apvts, ParamID::limiter, "Limiter"),
      outputGainSlider (p.apvts, ParamID::outputGain, "Output Gain")
{
    setLookAndFeel (&lookAndFeel);

    presetBox.setTextWhenNothingSelected ("No preset");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id == 0)
            return;

        if (id < userPresetIdBase)
            processorRef.applyFactoryPreset (id - 1);
        else if (id - userPresetIdBase < userPresetNamesById.size())
            processorRef.loadUserPreset (userPresetNamesById[id - userPresetIdBase]);
    };
    addAndMakeVisible (presetBox);

    savePresetButton.setTooltip ("Salva lo stato corrente come nuovo preset utente");
    savePresetButton.onClick = [this] { showSavePresetDialog(); };
    addAndMakeVisible (savePresetButton);
    refreshPresetBox();

    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);
    addAndMakeVisible (riderActivityMeter);

    for (auto* label : { &currentLoudnessLabel, &riderGainLabel })
    {
        label->setJustificationType (juce::Justification::centred);
        label->setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        addAndMakeVisible (*label);
    }

    addAndMakeVisible (inputGainSlider);
    addAndMakeVisible (targetLevelSlider);
    addAndMakeVisible (attackSlider);
    addAndMakeVisible (releaseSlider);
    addAndMakeVisible (gateThresholdSlider);
    addAndMakeVisible (limiterSlider);
    addAndMakeVisible (outputGainSlider);

    setResizable (true, true);
    setResizeLimits (480, 420, 900, 700);
    setSize (520, 456);
    startTimerHz (30);
}

LivellatoreAudioProcessorEditor::~LivellatoreAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void LivellatoreAudioProcessorEditor::refreshPresetBox (int idToSelect)
{
    presetBox.clear (juce::dontSendNotification);

    int id = 1;
    for (const auto& preset : PresetManager::getFactoryPresets())
        presetBox.addItem (preset.name, id++);

    userPresetNamesById = processorRef.presetManager.getUserPresetNames();
    if (! userPresetNamesById.isEmpty())
    {
        presetBox.addSeparator();
        for (int i = 0; i < userPresetNamesById.size(); ++i)
            presetBox.addItem (userPresetNamesById[i], userPresetIdBase + i);
    }

    presetBox.setSelectedId (idToSelect, juce::dontSendNotification);
}

void LivellatoreAudioProcessorEditor::showSavePresetDialog()
{
    savePresetDialog = std::make_unique<juce::AlertWindow> ("Salva preset", "Nome del preset:",
                                                             juce::MessageBoxIconType::NoIcon);
    savePresetDialog->addTextEditor ("name", {}, {});
    savePresetDialog->addButton ("Salva", 1, juce::KeyPress (juce::KeyPress::returnKey));
    savePresetDialog->addButton ("Annulla", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    // deleteWhenDismissed=false: la finestra la possediamo noi (unique_ptr),
    // cosi' possiamo leggerne il contenuto nella callback prima di
    // distruggerla esplicitamente. Con deleteWhenDismissed=true JUCE la
    // cancella PRIMA di invocare la callback, rendendo getTextEditorContents
    // qui sotto un use-after-free.
    savePresetDialog->enterModalState (true, juce::ModalCallbackFunction::create (
        [this] (int result)
        {
            if (result == 1 && savePresetDialog != nullptr)
            {
                const auto name = savePresetDialog->getTextEditorContents ("name").trim();
                if (name.isNotEmpty() && processorRef.saveCurrentStateAsPreset (name))
                {
                    refreshPresetBox();
                    const int idx = userPresetNamesById.indexOf (name);
                    if (idx >= 0)
                        presetBox.setSelectedId (userPresetIdBase + idx, juce::dontSendNotification);
                }
            }
            savePresetDialog.reset();
        }), false);
}

void LivellatoreAudioProcessorEditor::timerCallback()
{
    auto& engine = processorRef.getEngine();
    inputMeter.setLevelDb (engine.getInputLevelDb());
    outputMeter.setLevelDb (engine.getOutputLevelDb());
    riderActivityMeter.setLevelDb (engine.getRiderGainDb());

    currentLoudnessLabel.setText (juce::String (engine.getCurrentLoudnessLufs(), 1) + " LUFS",
                                   juce::dontSendNotification);
    const float riderGainDb = engine.getRiderGainDb();
    riderGainLabel.setText ((riderGainDb >= 0.0f ? "+" : "") + juce::String (riderGainDb, 1) + " dB",
                             juce::dontSendNotification);
}

void LivellatoreAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (LivellatoreLookAndFeel::backgroundColour);
}

void LivellatoreAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);

    auto presetBar = bounds.removeFromTop (28);
    savePresetButton.setBounds (presetBar.removeFromRight (28));
    presetBar.removeFromRight (8);
    presetBox.setBounds (presetBar);
    bounds.removeFromTop (12);

    auto meterArea = bounds.removeFromRight (150);
    auto readoutArea = meterArea.removeFromBottom (36);
    currentLoudnessLabel.setBounds (readoutArea.removeFromTop (18));
    riderGainLabel.setBounds (readoutArea);

    inputMeter.setBounds (meterArea.removeFromLeft (40).reduced (4));
    riderActivityMeter.setBounds (meterArea.removeFromLeft (60).reduced (4));
    outputMeter.setBounds (meterArea.removeFromLeft (40).reduced (4));

    bounds.removeFromRight (16);

    const int rowHeight = 48;
    inputGainSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    targetLevelSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    attackSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    releaseSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    gateThresholdSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    limiterSlider.setBounds (bounds.removeFromTop (rowHeight));
    bounds.removeFromTop (8);
    outputGainSlider.setBounds (bounds.removeFromTop (rowHeight));
}

} // namespace livellatore
