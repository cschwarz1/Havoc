#include <Havoc.h>
#include <core/HcAgent.h>

std::map<
    std::string,
    std::vector<std::pair<std::string, std::string>>
>* HcAgentCompletionList = new std::map<std::string, std::vector<std::pair<std::string, std::string>>>();

HcAgent::HcAgent(
    const json& metadata
) : data( metadata ) {}

auto HcAgent::initialize() -> bool {
    auto arch    = QString();
    auto user    = QString();
    auto host    = QString();
    auto local   = QString();
    auto path    = QString();
    auto process = QString();
    auto pid     = QString();
    auto tid     = QString();
    auto system  = QString();
    auto note    = QString();
    auto meta    = json();

    hidden = false;

    if ( data.contains( "uuid" ) && data[ "uuid" ].is_string() ) {
        uuid = data[ "uuid" ].get<std::string>();
    } else {
        spdlog::error( "[HcAgent::parse] agent does not contain valid uuid" );
        return false;
    }

    if ( data.contains( "type" ) && data[ "type" ].is_string() ) {
        type = data[ "type" ].get<std::string>();
    } else {
        spdlog::error( "[HcAgent::parse] agent does not contain valid type" );
        return false;
    }

    if ( data.contains( "note" ) && data[ "note" ].is_string() ) {
        note = QString( data[ "note" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain any note" );
    }

    if ( data.contains( "status" ) && data[ "status" ].is_string() ) {
        status = data[ "status" ].get<std::string>();
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain any status" );
    }

    if ( data.contains( "meta" ) && data[ "meta" ].is_object() ) {
        meta = data[ "meta" ].get<json>();
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta object" );
        return false;
    }

    if ( meta.contains( "parent" ) && meta[ "parent" ].is_string() ) {
        parent = meta[ "parent" ].get<std::string>();
    } else {
        spdlog::debug( "{} has no parent (is direct connection)", uuid );
    }

    if ( meta.contains( "user" ) && meta[ "user" ].is_string() ) {
        user = QString( meta[ "user" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta user" );
    }

    if ( meta.contains( "host" ) && meta[ "host" ].is_string() ) {
        host = QString( meta[ "host" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta host" );
    }

    if ( meta.contains( "arch" ) && meta[ "arch" ].is_string() ) {
        arch = QString( meta[ "arch" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta arch" );
    }

    if ( meta.contains( "local ip" ) && meta[ "local ip" ].is_string() ) {
        local = QString( meta[ "local ip" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta local ip" );
    }

    if ( meta.contains( "process path" ) && meta[ "process path" ].is_string() ) {
        path = QString( meta[ "process path" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta process path" );
    }

    if ( meta.contains( "process name" ) && meta[ "process name" ].is_string() ) {
        process = QString( meta[ "process name" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta process name" );
    }

    if ( meta.contains( "pid" ) && meta[ "pid" ].is_number_integer() ) {
        pid = QString::number( meta[ "pid" ].get<int>() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta pid" );
    }

    if ( meta.contains( "tid" ) && meta[ "tid" ].is_number_integer() ) {
        tid = QString::number( meta[ "tid" ].get<int>() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta tid" );
    }

    if ( meta.contains( "system" ) && meta[ "system" ].is_string() ) {
        system = QString( meta[ "system" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta system" );
    }

    if ( meta.contains( "last callback" ) && meta[ "last callback" ].is_string() ) {
        last = QString( meta[ "last callback" ].get<std::string>().c_str() );
    } else {
        spdlog::debug( "[HcAgent::parse] agent does not contain valid meta last" );
    }

    ui.table = {
        .Uuid        = new HcAgentTableItem( uuid.c_str(), this ),
        .Internal    = new HcAgentTableItem( local, this ),
        .Username    = new HcAgentTableItem( user, this ),
        .Hostname    = new HcAgentTableItem( host, this ),
        .ProcessPath = new HcAgentTableItem( path, this ),
        .ProcessName = new HcAgentTableItem( process, this ),
        .ProcessId   = new HcAgentTableItem( pid, this ),
        .ThreadId    = new HcAgentTableItem( tid, this ),
        .Arch        = new HcAgentTableItem( arch, this ),
        .System      = new HcAgentTableItem( system, this ),
        .Note        = new HcAgentTableItem( note, this, Qt::NoItemFlags, Qt::AlignVCenter ),
        .Last        = new HcAgentTableItem( last, this ),
    };

    console = new HcAgentConsole( this );
    console->setBottomLabel( QString( "[User: %1] [Process: %2] [Pid: %3] [Tid: %4]" ).arg( user ).arg( path ).arg( pid ).arg( tid ) );
    console->setInputLabel( ">>>" );
    console->LabelHeader->setFixedHeight( 0 );

    if ( ! HcAgentCompletionList->empty() ) {
        for ( int i = 0; i < HcAgentCompletionList->at( type ).size(); i++ ) {
            auto [command, description] = HcAgentCompletionList->at( type ).at( i );

            console->addCompleteCommand( command, description );
        }
    }

    //
    // if an interface has been registered then assign it to the agent
    //
    interface = std::nullopt;
    if ( const auto object = Havoc->AgentObject( type ); object.has_value() ) {
        HcPythonAcquire();

        try {
            interface = object.value()( uuid, type, meta );
        } catch ( py11::error_already_set &eas ) {
            spdlog::error( "failed to invoke agent interface [uuid: {}] [type: {}]: \n{}", uuid, type, eas.what() );
        }
    }

    return true;
}

auto HcAgent::post() -> void
{
    if ( status == AgentStatus::disconnected ) {
        disconnected();
    } else if ( status == AgentStatus::unresponsive ) {
        unresponsive();
    } else if ( status == AgentStatus::healthy ) {
        healthy();
    }
}

auto HcAgent::remove() -> void {
    auto [status, response] = Havoc->ApiSend( "/api/agent/remove", { { "uuid", uuid } } );

    spdlog::debug( "agent::remove {}", uuid );

    if ( status != 200 ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "agent removal failure",
            std::format( "failed to remove agent {}: {}", uuid, response )
        );

        spdlog::error( "failed to remove agent {}: {}", uuid, response );
    }
}

auto HcAgent::hide(
    void
) -> void {
    const auto show_hidden = Havoc->Gui->PageAgent->show_hidden;
    const auto table       = Havoc->Gui->PageAgent->AgentTable;
    const auto row         = ui.table.Uuid->row();

    spdlog::debug( "HcAgent::hide [show_hidden: {}] {}", show_hidden, hidden );

    if ( !show_hidden ) {
        table->hideRow( row );
    }

    if ( hidden ) {
        for ( int i = 0; i < table->columnCount(); i++ ) {
            table->item( row, i )->setForeground( Havoc->Theme.getComment() );
        }
    }
}

auto HcAgent::unhide(
    void
) -> void {
    const auto show_hidden = Havoc->Gui->PageAgent->show_hidden;
    const auto table       = Havoc->Gui->PageAgent->AgentTable;
    const auto row         = ui.table.Uuid->row();

    spdlog::debug( "HcAgent::unhide [show_hidden: {}] {}", show_hidden, hidden );

    table->showRow( row );

    if ( !hidden ) {
        for ( int i = 0; i < table->columnCount(); i++ ) {
            table->item( row, i )->setForeground( Havoc->Theme.getForeground() );
        }
    }
}

auto HcAgent::disconnected(
    void
) -> void {
    spdlog::debug( "agent::disconnected {}", uuid );

    if ( ui.node->itemEdge() ) {
        ui.node->itemEdge()->setColor( Havoc->Theme.getComment() );
        ui.node->itemEdge()->setStatus(
            QString::fromStdString( AgentStatus::disconnected ),
            Havoc->Theme.getRed()
        );
    }
}

auto HcAgent::unresponsive(
    void
) -> void {
    spdlog::debug( "agent::unresponsive {}", uuid );

    if ( ui.node->itemEdge() ) {
        ui.node->itemEdge()->setColor( Havoc->Theme.getComment() );
        ui.node->itemEdge()->setStatus(
            QString::fromStdString( AgentStatus::unresponsive ),
            Havoc->Theme.getOrange()
        );
    }
}

auto HcAgent::healthy(
    void
) -> void {
    spdlog::debug( "agent::healthy {}", uuid );

    if ( ui.node->itemEdge() ) {
        ui.node->itemEdge()->setColor( parent.empty() ? Havoc->Theme.getGreen() : Havoc->Theme.getPurple() );
        ui.node->itemEdge()->setStatus( QString(), QColor() );
    }
}
