#include <Common.h>
#include <Havoc.h>
#include <api/HcCore.h>

auto HcServerApiSend(
    const std::string& endpoint,
    const json&        data
) -> json {
    auto body               = json();
    auto [status, response] = Havoc->ApiSend( endpoint, data );

    try {
        if ( ( body = json::parse( response ) ).is_discarded() ) {
            spdlog::debug( "server api processing json response has been discarded" );
        }
    } catch ( std::exception& e ) {
        spdlog::error( "error while parsing body response from {}:\n{}", endpoint, e.what() );
    }

    return json {
        { "status", status },
        { "body",   body   },
    };
}

auto HcRegisterMenuAction(
    const std::string&  name,
    const std::string&  icon_path,
    const py11::object& callback
) -> void {
    auto action = new HavocClient::ActionObject();

    action->type = HavocClient::ActionObject::ActionHavoc;
    action->name = name;
    action->icon = icon_path;
    action->callback = callback;

    Havoc->AddAction( action );

    if ( icon_path.empty() ) {
        Havoc->Gui->PageAgent->AgentActionMenu->addAction( name.c_str() );
    } else {
        Havoc->Gui->PageAgent->AgentActionMenu->addAction( QIcon( icon_path.c_str() ), name.c_str() );
    }
}