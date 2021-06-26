
#include "configuration_widget.h"
#include "internal_node_widget.h"

namespace Gammou
{

    configuration_directory::configuration_directory(configuration_widget& config_widget, circuit_tree& directory)
    :   _config_widget{config_widget}, _dir{&directory}
    {
    }

    configuration_directory::~configuration_directory()
    {
        _config_widget.data_model().remove_config(*_dir);
        _config_widget.update();
    }

    void configuration_directory::compile()
    {
        if (auto ctl = _dir->get_circuit_controller())
            ctl->compile();
    }

    void configuration_directory::register_static_memory_chunk(const DSPJIT::compile_node_class& node, std::vector<uint8_t>&& data)
    {
        if (auto ctl = _dir->get_circuit_controller())
            ctl->register_static_memory_chunk(node, std::move(data));
    }

    void configuration_directory::free_static_memory_chunk(const DSPJIT::compile_node_class& node)
    {
        if (auto ctl = _dir->get_circuit_controller())
            ctl->free_static_memory_chunk(node);
    }

    void configuration_directory::display()
    {
        _config_widget._select_config(*_dir);
    }

    void configuration_directory::rename(const std::string& name)
    {
        auto& new_dir = _config_widget.data_model().rename_config(*_dir, name);
        _dir = &new_dir;
        _config_widget.update();
    }

    std::unique_ptr<abstract_configuration_directory> configuration_directory::create_directory(
        std::string &desired_name, std::weak_ptr<View::widget> widget)
    {
        auto new_dir = std::make_unique<configuration_directory>(
            _config_widget,
            _dir->insert_config_dir(desired_name, circuit_tree{widget}));
        _config_widget.update();
        return new_dir;
    }

    // std::unique_ptr<abstract_configuration_page> configuration_directory::create_page(
    //     std::string &desired_name, std::weak_ptr<View::widget> widget)
    // {
    //     auto new_page = std::make_unique<configuration_page>(
    //         _config_widget,
    //         _dir->in
    //     )
    // }

    configuration_widget::configuration_widget(
        factory_widget& factory,
        synthesizer& synth,
        View::widget_proxy<>& editor_proxy,
        float width, float height)
    :   View::owning_directory_view<circuit_tree>(
            std::make_unique<circuit_tree>(),
            width, height,
            16.f,/* cell height */
            14.f /* font size */),
        _factory{factory},
        _synthesizer{synth},
        _editor_proxy{editor_proxy}
    {
        _initialize();

        // Initialize config tree view callbacks
        set_value_select_callback(
            [this](std::weak_ptr<View::widget> config)
            {
                // select and display the node configuration
                _editor_proxy.set_widget(config);
            });

        set_directory_select_callback(
            [this](circuit_tree& dir)
            {
                // select and display the circuit
                _editor_proxy.set_widget(dir.get_config_widget());
            });
    }

    bool configuration_widget::deserialize_configuration(const nlohmann::json& json)
    {
        try {
            reset_editor();
            _master_circuit_editor->deserialize(
                json.at("master-circuit"),
                [this](const nlohmann::json &j)
                {
                    return _deserialize_node(*_master_circuit_dir, j);
                });
            _polyphonic_circuit_editor->deserialize(
                json.at("polyphonic-circuit"),
                [this](const nlohmann::json &j)
                {
                    return _deserialize_node(*_polyphonic_circuit_dir, j);
                });

            // Recompile the new loaded circuit
            _synthesizer.get_master_circuit_controller().compile();
            _synthesizer.get_polyphonic_circuit_controller().compile();
            return true;
        }
        catch (const std::exception &e) {
            reset_editor();
            LOG_ERROR("[configuration_widget Failed to load preset : %s\n", e.what());
            return false;
        }
        catch (...) {
            reset_editor();
            LOG_ERROR("[configuration_widget] Failed to load preset : unknown error\n");
            return false;
        }
    }

    nlohmann::json configuration_widget::serialize_configuration()
    {
        return {
            {"master-circuit", _master_circuit_editor->serialize()},
            {"polyphonic-circuit", _polyphonic_circuit_editor->serialize()}
        };
    }

    void configuration_widget::_select_config(circuit_tree& config_dir)
    {
        _editor_proxy.set_widget(config_dir.get_config_widget());
        select_directory(config_dir);
    }

    void configuration_widget::reset_editor()
    {
        LOG_DEBUG("[configuration_widget] Reset content\n");

        // Remove the nodes : this will also remove relevant configuration dirs
        _master_circuit_editor->clear();
        _master_circuit_editor->insert_node_widget(50, 50, _make_master_from_polyphonic_node());
        _master_circuit_editor->insert_node_widget(50, 100, _make_master_output_node());

        _polyphonic_circuit_editor->clear();
        _polyphonic_circuit_editor->insert_node_widget(50, 50, _make_polyphonic_midi_input_node());
        _polyphonic_circuit_editor->insert_node_widget(50, 100, _make_polyphonic_to_master_node());

        // Update the circuit browser
        update();

        //  Select master circuit by default
        _editor_proxy.set_widget(_master_circuit_widget);
    }

    void configuration_widget::_initialize()
    {
        _initialize_circuit_editors();

        std::string master_name = "Master";
        std::string polyphonic_name = "Polyphonic";

        auto& root = data_model();
        auto& master_circuit_dir =
            root.insert_config_dir(master_name, circuit_tree{_master_circuit_widget, &_synthesizer.get_master_circuit_controller()});
        auto& polyphonic_circuit_dir =
            root.insert_config_dir(polyphonic_name, circuit_tree{_polyphonic_circuit_widget, &_synthesizer.get_polyphonic_circuit_controller()});

        _master_circuit_dir = std::make_unique<configuration_directory>(*this, master_circuit_dir);
        _polyphonic_circuit_dir = std::make_unique<configuration_directory>(*this, polyphonic_circuit_dir);

        _master_circuit_editor->set_create_node_callback(
            [this]()
            {
                return _factory.create_node(*_master_circuit_dir);
            });
        _polyphonic_circuit_editor->set_create_node_callback(
            [this]()
            {
                return _factory.create_node(*_polyphonic_circuit_dir);
            });

        reset_editor();
    }

    void configuration_widget::_initialize_circuit_editors()
    {
        auto master_editor = _make_editor(_synthesizer.get_master_circuit_controller());
        auto polyphonic_editor = _make_editor(_synthesizer.get_polyphonic_circuit_controller());

        _master_circuit_editor = master_editor.get();
        _polyphonic_circuit_editor = polyphonic_editor.get();

        _master_circuit_widget = _wrap_editor(std::move(master_editor));
        _polyphonic_circuit_widget = _wrap_editor(std::move(polyphonic_editor));
    }

    std::unique_ptr<circuit_editor> configuration_widget::_make_editor(synthesizer::circuit_controller& circuit)
    {
        auto editor = std::make_unique<circuit_editor>(200, 400);
        editor->set_circuit_changed_callback([&circuit]() { circuit.compile(); } );
        return editor;
    }

    std::shared_ptr<View::widget> configuration_widget::_wrap_editor(std::unique_ptr<circuit_editor>&& editor)
    {
        return std::make_shared<View::header>(
                    std::make_unique<View::map_wrapper>(
                        std::move(editor),
                        100, 100),
                    View::color_theme::color::SURFACE_DARK);
    }

    std::unique_ptr<node_widget> configuration_widget::_make_master_from_polyphonic_node()
    {
        return std::make_unique<internal_node_widget>(
            "From Polyphonic",
            master_from_polyphonic_node_id,
            _synthesizer.from_polyphonic_node());
    }

    std::unique_ptr<node_widget> configuration_widget::_make_master_output_node()
    {
        return std::make_unique<internal_node_widget>(
            "Output",
            master_output_node_id,
            _synthesizer.output_node());
    }

    std::unique_ptr<node_widget> configuration_widget::_make_polyphonic_midi_input_node()
    {
        auto midi_input =
            std::make_unique<internal_node_widget>(
            "Midi In",
            polyphonic_midi_input_node_id,
            _synthesizer.midi_input_node());

        midi_input->set_output_name(0u, "Gate");
        midi_input->set_output_name(1u, "Pitch");
        midi_input->set_output_name(2u, "Attack");
        midi_input->set_output_name(3u, "Release");

        return midi_input;
    }

    std::unique_ptr<node_widget> configuration_widget::_make_polyphonic_to_master_node()
    {
        return std::make_unique<internal_node_widget>(
            "To Master",
            polyphonic_to_master_node_id,
            _synthesizer.to_master_node());
    }

    std::unique_ptr<node_widget> configuration_widget::_deserialize_node(abstract_configuration_directory& parent_config, const nlohmann::json& json)
    {
        if (json.is_string()) {
            return _deserialize_internal_node(json.get<std::string>());
        }
        else {
            return _factory.create_node(parent_config, json);
        }
    }

    std::unique_ptr<node_widget> configuration_widget::_deserialize_internal_node(const std::string& identifier)
    {
        if (identifier == master_from_polyphonic_node_id)
            return _make_master_from_polyphonic_node();
        else if (identifier == master_output_node_id)
            return _make_master_output_node();
        else if (identifier ==  polyphonic_midi_input_node_id)
            return _make_polyphonic_midi_input_node();
        else  if (identifier == polyphonic_to_master_node_id)
            return _make_polyphonic_to_master_node();
        else
            throw std::runtime_error("main_gui::deserialize : Unknown internal node : " + identifier);
    }
}