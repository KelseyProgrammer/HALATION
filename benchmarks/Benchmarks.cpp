TEST_CASE ("Boot performance")
{
#ifndef CI
    BENCHMARK_ADVANCED ("Processor constructor")
    (Catch::Benchmark::Chronometer meter)
    {
        std::vector<Catch::Benchmark::destructable_object<PluginProcessor>> storage (size_t (meter.runs()));
        meter.measure ([&] (int i) { storage[(size_t) i].construct(); });
        for (auto& s : storage)
            s.destruct();
    };

    BENCHMARK_ADVANCED ("Processor destructor")
    (Catch::Benchmark::Chronometer meter)
    {
        std::vector<std::unique_ptr<PluginProcessor>> storage;
        storage.reserve (static_cast<size_t> (meter.runs()));
        for (int i = 0; i < meter.runs(); ++i)
            storage.push_back (std::make_unique<PluginProcessor>());
        meter.measure ([&] (int i) { storage[static_cast<size_t> (i)].reset(); });
    };

    BENCHMARK_ADVANCED ("Editor open and close")
    (Catch::Benchmark::Chronometer meter)
    {
        // Heap-allocate: PluginProcessor is ~12MB and overflows the stack (§10)
        auto plugin = std::make_unique<PluginProcessor>();

        // due to complex construction logic of the editor, let's measure open/close together
        meter.measure ([&] (int /* i */) {
            auto editor = plugin->createEditorIfNeeded();
            plugin->editorBeingDeleted (editor);
            delete editor;
            return plugin->getActiveEditor();
        });
    };
#endif
}

TEST_CASE ("Render performance")
{
#ifndef CI
    // The v1.0 CPU budget scenario: 8 paths, full feedback, 512 samples @ 44.1kHz.
    // Real-time budget per block is 512/44100 = ~11.61ms; the target is <=15%
    // of that (~1.74ms per block) on an M1.
    BENCHMARK_ADVANCED ("processBlock: 8 paths, full feedback, 512 @ 44.1kHz")
    (Catch::Benchmark::Chronometer meter)
    {
        auto plugin = std::make_unique<PluginProcessor>();
        auto& apvts = plugin->getAPVTS();

        if (auto* p = apvts.getParameter (ParameterIDs::globalNumPaths))
            p->setValueNotifyingHost (1.0f);   // 8 paths
        if (auto* p = apvts.getParameter (ParameterIDs::globalBloomRate))
            p->setValueNotifyingHost (1.0f);   // full feedback

        plugin->prepareToPlay (44100.0, 512);

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;
        juce::Random rng;

        // Warm up so the feedback network and smoothers reach steady state
        for (int warm = 0; warm < 50; ++warm)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int s = 0; s < 512; ++s)
                    buffer.setSample (ch, s, rng.nextFloat() * 0.5f - 0.25f);
            plugin->processBlock (buffer, midi);
        }

        meter.measure ([&] (int /* i */) {
            for (int ch = 0; ch < 2; ++ch)
                for (int s = 0; s < 512; ++s)
                    buffer.setSample (ch, s, rng.nextFloat() * 0.5f - 0.25f);
            plugin->processBlock (buffer, midi);
            return buffer.getSample (0, 0);
        });
    };
#endif
}
