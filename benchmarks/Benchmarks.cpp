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
        PluginProcessor plugin;

        // due to complex construction logic of the editor, let's measure open/close together
        meter.measure ([&] (int /* i */) {
            auto editor = plugin.createEditorIfNeeded();
            plugin.editorBeingDeleted (editor);
            delete editor;
            return plugin.getActiveEditor();
        });
    };
#endif
}
