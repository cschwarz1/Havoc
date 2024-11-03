#include <Havoc.h>
#include <core/HcHelper.h>
#include <api/Engine.h>
#include <filesystem>

PYBIND11_EMBEDDED_MODULE( _pyhavoc, m ) {

    m.doc() = "python api for havoc framework";

    //
    // havoc client core api
    //
    {
        auto core = m.def_submodule(
            "core",
            "havoc client core api"
        );

        //
        // Havoc I/O Script Manager api functions
        //
        core.def( "HcIoConsoleWriteStdOut", HcIoConsoleWriteStdOut, py::call_guard<py::gil_scoped_release>() );
        core.def( "HcIoScriptLoadCallback", HcIoScriptLoadCallback, py::call_guard<py::gil_scoped_release>() );

        //
        // Havoc Server api functions
        //
        core.def( "HcServerApiSend", HcServerApiSend, py::call_guard<py::gil_scoped_release>() );

        //
        // Havoc core apis
        //
        core.def( "HcRegisterMenuAction", HcRegisterMenuAction );

        //
        // Havoc Listener api functions
        //
        core.def( "HcListenerProtocolData",       HcListenerProtocolData );
        core.def( "HcListenerAll",                HcListenerAll );
        core.def( "HcListenerQueryType",          HcListenerQueryType );
        core.def( "HcListenerRegisterMenuAction", HcListenerRegisterMenuAction );
    }

    //
    // Havoc client ui api
    //
    {
        auto ui = m.def_submodule(
            "ui",
            "havoc client ui api"
        );

        //
        // Havoc Ui functions and utilities
        //
        ui.def( "HcUiListenerObjName", []() -> py11::str {
            return ( "HcListenerDialog.StackedProtocols" );
        } );

        ui.def( "HcUiListenerRegisterView", [](
            const std::string&  name,
            const py11::object& object
        ) {
            Havoc->AddProtocol( name, object );
        } );

        ui.def( "HcUiBuilderRegisterView", [](
            const std::string&  name,
            const py11::object& object
        ) {
            Havoc->AddBuilder( name, object );
        } );

        ui.def( "HcUiGetStyleSheet", []() -> py11::str {
            return ( HcApplication::StyleSheet().toStdString() );
        } );

        ui.def( "HcUiMessageBox", [](
            const int          icon,
            const std::string& title,
            const std::string& text
        ) {
            Helper::MessageBox( static_cast<QMessageBox::Icon>( icon ), title, text );
        }, py::call_guard<py::gil_scoped_release>() );

        ui.def(
            "HcListenerPopupSelect", HcListenerPopupSelect,
            py::call_guard<pybind11::gil_scoped_release>()
        );
    }

    //
    // Havoc agent python api
    //
    {
        auto agent = m.def_submodule(
            "agent",
            "havoc client agent api"
        );

        agent.def( "HcAgentRegisterInterface",  HcAgentRegisterInterface );
        agent.def( "HcAgentRegisterCallback",   HcAgentRegisterCallback  );
        agent.def( "HcAgentUnRegisterCallback", HcAgentUnRegisterCallback  );
        agent.def( "HcAgentConsoleHeader",      HcAgentConsoleHeader );
        agent.def( "HcAgentConsoleLabel",       HcAgentConsoleLabel );
        agent.def( "HcAgentConsoleWrite",       HcAgentConsoleWrite, py::call_guard<py::gil_scoped_release>() );
        agent.def( "HcAgentConsoleInput",       []( const py11::object& eval ) { Havoc->PyEngine->PyEval = eval; } );
        agent.def( "HcAgentConsoleAddComplete", HcAgentConsoleAddComplete );
        agent.def( "HcAgentExecute",            HcAgentExecute, py::call_guard<py::gil_scoped_release>() );
        agent.def( "HcAgentRegisterMenuAction", HcAgentRegisterMenuAction );
    }
}

HcPyEngine::HcPyEngine() {
    auto exception = std::string();

    guard = new py11::scoped_interpreter;

    try {
        auto _ = py11::module_::import( "sys" )
            .attr( "path" )
            .attr( "append" )( "python" );

        py11::module_::import( "pyhavoc" );
    } catch ( py11::error_already_set &eas ) {
        exception = std::string( eas.what() );
    }

    if ( ! exception.empty() ) {
        spdlog::error( "failed to import \"python.pyhavoc\": \n{}", exception );
    }

    if ( PyGILState_Check() ) {
        //
        // release the current interpreter state to be available
        // for other threads and scripts to be used
        //
        state = PyEval_SaveThread();
    };
}

HcPyEngine::~HcPyEngine() {
    PyThreadState_Clear( state );
    PyThreadState_Delete( state );
};
