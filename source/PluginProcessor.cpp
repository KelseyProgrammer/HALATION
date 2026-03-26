#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"
#include "IntervalPresets.h"

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                      ),
      m_apvts (*this, nullptr, "HALATION", createParameterLayout()),
      m_presetManager (m_apvts)
{
    // Listen for preset selector and all path semitone changes
    m_apvts.addParameterListener (ParameterIDs::globalIntervalPreset, this);
    for (int i = 0; i < 8; ++i)
        m_apvts.addParameterListener (ParameterIDs::pathSemitones (i), this);
}

PluginProcessor::~PluginProcessor()
{
    m_apvts.removeParameterListener (ParameterIDs::globalIntervalPreset, this);
    for (int i = 0; i < 8; ++i)
        m_apvts.removeParameterListener (ParameterIDs::pathSemitones (i), this);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Global parameters
    layout.add (std::make_unique<juce::AudioParameterInt>  (ParameterIDs::globalNumPaths,
                                                             "Paths", 2, 8, 4));
    layout.add (std::make_unique<juce::AudioParameterFloat> (ParameterIDs::globalBloomRate,
                                                             "Bloom",
                                                             juce::NormalisableRange<float> (0.0f, 1.0f),
                                                             0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (ParameterIDs::globalStagger,
                                                             "Stagger",
                                                             juce::NormalisableRange<float> (0.0f, 1.0f),
                                                             0.5f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (ParameterIDs::globalSpectralTilt,
                                                             "Tilt",
                                                             juce::NormalisableRange<float> (-1.0f, 1.0f),
                                                             -0.2f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (ParameterIDs::globalDamping,
                                                             "Damping",
                                                             juce::NormalisableRange<float> (0.0f, 1.0f),
                                                             0.4f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (ParameterIDs::globalChaos,
                                                             "Chaos",
                                                             juce::NormalisableRange<float> (0.0f, 1.0f),
                                                             0.15f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (ParameterIDs::globalMix,
                                                             "Mix",
                                                             juce::NormalisableRange<float> (0.0f, 1.0f),
                                                             0.7f));
    layout.add (std::make_unique<juce::AudioParameterInt>  (ParameterIDs::globalIntervalPreset,
                                                             "Preset", 0, 5, 2));

    // Per-path parameters
    auto fifthsDefaults = halation::IntervalPresets::getPreset (halation::PresetID::Fifths);

    for (int i = 0; i < 8; ++i)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ParameterIDs::pathSemitones (i),
            "Path " + juce::String (i + 1) + " Semi",
            juce::NormalisableRange<float> (-24.0f, 24.0f),
            fifthsDefaults[static_cast<size_t> (i)]));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ParameterIDs::pathLevel (i),
            "Path " + juce::String (i + 1) + " Level",
            juce::NormalisableRange<float> (0.0f, 1.0f),
            1.0f));

        // Default pan: spread evenly from -0.875 to +0.875
        float defaultPan = -0.875f + static_cast<float> (i) * 0.25f;
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            ParameterIDs::pathPan (i),
            "Path " + juce::String (i + 1) + " Pan",
            juce::NormalisableRange<float> (-1.0f, 1.0f),
            defaultPan));
    }

    return layout;
}

//==============================================================================
const juce::String PluginProcessor::getName() const { return JucePlugin_Name; }

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    // Maximum delay time: kBaseDelayMs + kMaxStaggerMs * 7 paths = 20 + 560 = 580ms
    // Add phase vocoder latency (~46ms). Round up generously.
    return 1.0;
}

int PluginProcessor::getNumPrograms()         { return 1; }
int PluginProcessor::getCurrentProgram()      { return 0; }
void PluginProcessor::setCurrentProgram (int) {}

const juce::String PluginProcessor::getProgramName (int)
{
    return m_presetManager.getCurrentPresetName();
}

void PluginProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    m_engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (m_engine.getLatencySamples());
}

void PluginProcessor::releaseResources()
{
    m_engine.reset();
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #endif

    return true;
  #endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    // Pull parameter values and forward to engine (message-thread values, safe to read atomically)
    const float bloomRate    = *m_apvts.getRawParameterValue (ParameterIDs::globalBloomRate);
    const float stagger      = *m_apvts.getRawParameterValue (ParameterIDs::globalStagger);
    const float spectralTilt = *m_apvts.getRawParameterValue (ParameterIDs::globalSpectralTilt);
    const float damping      = *m_apvts.getRawParameterValue (ParameterIDs::globalDamping);
    const float chaos        = *m_apvts.getRawParameterValue (ParameterIDs::globalChaos);
    const float mix          = *m_apvts.getRawParameterValue (ParameterIDs::globalMix);
    const int   numPaths     = static_cast<int> (*m_apvts.getRawParameterValue (ParameterIDs::globalNumPaths));

    m_engine.setGlobalParameters (bloomRate, stagger, spectralTilt, damping, chaos, mix);
    m_engine.setNumPaths (numPaths);

    for (int i = 0; i < 8; ++i)
    {
        m_engine.setPathInterval (i, *m_apvts.getRawParameterValue (ParameterIDs::pathSemitones (i)));
        m_engine.setPathLevel    (i, *m_apvts.getRawParameterValue (ParameterIDs::pathLevel (i)));
        m_engine.setPathPan      (i, *m_apvts.getRawParameterValue (ParameterIDs::pathPan (i)));
    }

    m_engine.process (buffer);
}

//==============================================================================
bool PluginProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = m_apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
            m_apvts.replaceState (state);
    }
}

//==============================================================================
void PluginProcessor::parameterChanged (const juce::String& paramID, float newValue)
{
    if (m_ignoreParamChanges)
        return;

    if (paramID == ParameterIDs::globalIntervalPreset)
    {
        auto id = static_cast<halation::PresetID> (static_cast<int> (newValue));
        if (id != halation::PresetID::Custom)
            applyIntervalPreset (id);
    }
    else
    {
        for (int i = 0; i < 8; ++i)
        {
            if (paramID == ParameterIDs::pathSemitones (i))
            {
                m_ignoreParamChanges = true;
                if (auto* p = m_apvts.getParameter (ParameterIDs::globalIntervalPreset))
                    p->setValueNotifyingHost (p->convertTo0to1 (
                        static_cast<float> (halation::PresetID::Custom)));
                m_ignoreParamChanges = false;
                break;
            }
        }
    }
}

void PluginProcessor::applyIntervalPreset (halation::PresetID id)
{
    auto semitones = halation::IntervalPresets::getPreset (id);
    m_ignoreParamChanges = true;
    for (int i = 0; i < 8; ++i)
    {
        if (auto* p = m_apvts.getParameter (ParameterIDs::pathSemitones (i)))
            p->setValueNotifyingHost (p->convertTo0to1 (semitones[static_cast<size_t> (i)]));
    }
    m_ignoreParamChanges = false;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
