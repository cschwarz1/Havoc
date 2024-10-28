#include <Havoc.h>
#include <QTimer>
#include <QSplashScreen>
#include <QtCore5Compat/QTextCodec>

auto HttpErrorToString(
    const httplib::Error& error
) -> std::optional<std::string> {
    switch ( error ) {
        case httplib::Error::Unknown:
            return "Unknown";

        case httplib::Error::Connection:
            return ( "Connection" );

        case httplib::Error::BindIPAddress:
            return ( "BindIPAddress" );

        case httplib::Error::Read:
            return ( "Read" );

        case httplib::Error::Write:
            return ( "Write" );

        case httplib::Error::ExceedRedirectCount:
            return ( "ExceedRedirectCount" );

        case httplib::Error::Canceled:
            return ( "Canceled" );

        case httplib::Error::SSLConnection:
            return ( "SSLConnection" );

        case httplib::Error::SSLLoadingCerts:
            return ( "SSLLoadingCerts" );

        case httplib::Error::SSLServerVerification:
            return ( "SSLServerVerification" );

        case httplib::Error::UnsupportedMultipartBoundaryChars:
            return ( "UnsupportedMultipartBoundaryChars" );

        case httplib::Error::Compression:
            return ( "Compression" );

        case httplib::Error::ConnectionTimeout:
            return ( "ConnectionTimeout" );

        case httplib::Error::ProxyConnection:
            return ( "ProxyConnection" );

        case httplib::Error::SSLPeerCouldBeClosed_:
            return ( "SSLPeerCouldBeClosed_" );

        default: break;
    }

    return std::nullopt;
}

HavocClient::HavocClient() {
    /* initialize logger */
    spdlog::set_pattern( "[%T %^%l%$] %v" );
    spdlog::info( "Havoc Framework [{} :: {}]", HAVOC_VERSION, HAVOC_CODENAME );

    /* TODO: read this from the config file */
    const auto family = "Monospace";
    const auto size   = 9;

#ifdef Q_OS_MAC
    //
    // original fix and credit: https://github.com/HavocFramework/Havoc/pull/466/commits/8b75de9b4632a266badf64e1cc22e57cc55a5b7c
    //
    QApplication::setStyle( "Fusion" );
#endif

    //
    // set font
    //
    QTextCodec::setCodecForLocale( QTextCodec::codecForName( "UTF-8" ) );
    QApplication::setFont( QFont( family, size ) );
    QTimer::singleShot( 10, [&]() { QApplication::setFont( QFont( family, size ) ); } );
}

HavocClient::~HavocClient() = default;

/*!
 * @brief
 *  this entrypoint executes the connector dialog
 *  and tries to connect and login to the teamserver
 *
 *  after connecting it is going to start an event thread
 *  and starting the Havoc MainWindow.
 */
auto HavocClient::Main(
    int    argc,
    char** argv
) -> void {
    auto connector = new HcConnectDialog();
    auto result    = httplib::Result();
    auto response  = json{};
    auto error     = std::string( "Failed to send login request: " );
    auto data      = json();

    if ( argc >= 2 ) {
        //
        // check if we have specified the --debug/-d
        // flag to enable debug messages and logs
        //
        if ( ( strcmp( argv[ 1 ], "--debug" ) == 0 ) || ( strcmp( argv[ 1 ], "-d" ) == 0 ) ) {
            spdlog::set_level( spdlog::level::debug );
        }
    }

    if ( ( data = connector->start() ).empty() || ! connector->pressedConnect() ) {
        return;
    }

    splash = new QSplashScreen( QGuiApplication::primaryScreen(), QPixmap( ":/images/splash-screen" ) );
    splash->show();

    if ( ! SetupDirectory() ) {
        spdlog::error( "failed to setup configuration directory. aborting" );
        return;
    }

    auto http = httplib::Client( "https://" + data[ "host" ].get<std::string>() + ":" + data[ "port" ].get<std::string>() );
    http.enable_server_certificate_verification( false );

    result = http.Post( "/api/login", data.dump(), "application/json" );
    if ( HttpErrorToString( result.error() ).has_value() ) {
        error = HttpErrorToString( result.error() ).value();
        spdlog::error( "Failed to send login request: {}", error );

        Helper::MessageBox(
            QMessageBox::Critical,
            "Login failure",
            std::format( "Failed to login: {}", error )
        );

        return;
    }

    /* 401 Unauthorized: Failed to log in */
    if ( result->status == 401 ) {
        if ( result->body.empty() ) {
            Helper::MessageBox(
                QMessageBox::Critical,
                "Login failure",
                "Failed to login: Unauthorized"
            );

            return;
        }

        if ( ( data = json::parse( result->body ) ).is_discarded() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        if ( ! data.contains( "error" ) ) {
            goto ERROR_SERVER_RESPONSE;
        }

        if ( ! data[ "error" ].is_string() ) {
            goto ERROR_SERVER_RESPONSE;
        }

        Helper::MessageBox(
            QMessageBox::Critical,
            "Login failure",
            QString( "Failed to login: %1" ).arg( data[ "error" ].get<std::string>().c_str() ).toStdString()
        );

        return;
    }

    if ( result->status != 200 ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Login failure",
            QString( "Unexpected response: Http status code %1" ).arg( result->status ).toStdString()
        );
        return;
    }

    spdlog::debug( "result: {}", result->body );

    if ( result->body.empty() ) {
        goto ERROR_SERVER_RESPONSE;
    }

    Profile = {
        .Name  = data[ "name" ].get<std::string>(),
        .Host  = data[ "host" ].get<std::string>(),
        .Port  = data[ "port" ].get<std::string>(),
        .User  = data[ "username" ].get<std::string>(),
        .Pass  = data[ "password" ].get<std::string>(),
    };

    try {
        if ( ( data = json::parse( result->body ) ).is_discarded() ) {
            goto ERROR_SERVER_RESPONSE;
        }
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse login json response: \n{}", e.what() );
        return;
    }

    if ( ! data.contains( "token" ) || ! data[ "token" ].is_string() ) {
        goto ERROR_SERVER_RESPONSE;
    }

    Profile.Token = data[ "token" ].get<std::string>(),

    Theme.setStyleSheet( StyleSheet() );

    //
    // create main window
    //
    Gui = new HcMainWindow;
    Gui->setStyleSheet( StyleSheet() );

    //
    // setup Python thread
    //
    PyEngine = new HcPyEngine();

    //
    // set up the event thread and connect to the
    // server and dispatch all the incoming events
    //
    SetupThreads();

    //
    // pull server plugins and resources and process
    // locally registered scripts via the configuration
    //
    ServerPullPlugins();

    return;

ERROR_SERVER_RESPONSE:
    return Helper::MessageBox(
        QMessageBox::Critical,
        "Login failure",
        "Failed to login: Invalid response from the server"
    );
}

auto HavocClient::Exit() -> void {
    QApplication::exit( 0 );
}

auto HavocClient::ApiSend(
    const std::string& endpoint,
    const json&        body,
    const bool         keep_alive
) const -> httplib::Result {
    auto http   = httplib::Client( "https://" + Profile.Host + ":" + Profile.Port );
    auto result = httplib::Result();
    auto error  = std::string( "Failed to send api request: " );

    //
    // only way to keep the connection alive even while we have
    // "keep-alive" enabled it will shut down after 5 seconds
    //
    if ( keep_alive ) {
        http.set_read_timeout( INT32_MAX );
        http.set_connection_timeout( INT32_MAX );
        http.set_write_timeout( INT32_MAX );
    }

    //
    // configure the client
    //
    http.set_keep_alive( keep_alive );
    http.enable_server_certificate_verification( false );
    http.set_default_headers( {
        { "x-havoc-token", Havoc->Profile.Token }
    } );

    //
    // send the request to our endpoint
    //
    result = http.Post( endpoint, body.dump(), "application/json" );

    if ( HttpErrorToString( result.error() ).has_value() ) {
        spdlog::error( "Failed to send login request: {}", HttpErrorToString( result.error() ).value() );
    }

    return result;
}

auto HavocClient::ServerPullPlugins(
    void
) -> void {
    auto result    = httplib::Result();
    auto plugins   = json();
    auto uuid      = std::string();
    auto name      = std::string();
    auto version   = std::string();
    auto resources = std::vector<std::string>();
    auto message   = std::string();
    auto box       = QMessageBox();
    auto details   = std::string();

    splash->showMessage( "pulling plugins information", Qt::AlignLeft | Qt::AlignBottom, Qt::white );

    result = ApiSend( "/api/plugin/list", {} );
    if ( result->status != 200 ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "plugin processing failure",
            "failed to pull all registered plugins"
        );
        return;
    }

    try {
        if ( ( plugins = json::parse( result->body ) ).is_discarded() ) {
            spdlog::debug( "plugin processing json response has been discarded" );
            return;
        }
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse plugin processing json response: \n{}", e.what() );
        return;
    }

    if ( plugins.empty() ) {
        spdlog::debug( "no plugins to process" );
        return;
    }

    if ( ! plugins.is_array() ) {
        spdlog::error( "plugins response is not an array" );
        return;
    }

    //
    // iterate over all available agents
    // and pull console logs as well
    //
    for ( auto& plugin : plugins ) {
        if ( ! plugin.is_object() ) {
            spdlog::debug( "warning! plugin processing item is not an object" );
            continue;
        }

        spdlog::debug( "processing plugin: {}", plugin.dump() );

        //
        // we just really assume that they
        // are contained inside the json object
        //
        name      = plugin[ "name"      ].get<std::string>();
        version   = plugin[ "version"   ].get<std::string>();
        resources = plugin[ "resources" ].get<std::vector<std::string>>();

        //
        // check if the plugin directory exists, if not then
        // pull the plugin resources from the remote server
        //
        if ( QDir( std::format( "{}/plugins/{}@{}", directory().path().toStdString(), name, version ).c_str() ).exists() ) {
            //
            // if the directory exists then it means we have pulled the
            // resources already so we are going to skip to the next plugin
            //
            continue;
        }

        Havoc->splash->showMessage( std::format( "processing plugin {} ({})", name, version ).c_str(), Qt::AlignLeft | Qt::AlignBottom, Qt::white );

        if ( plugin.contains( "resources" ) && plugin[ "resources" ].is_array() ) {
            message = std::format( "A plugin will be installed from the remote server: {} ({})\n", name, version );
            details = "resources to be downloaded locally:\n";

            for ( const auto& res : resources ) {
                details += std::format( " - {}\n", res );
            }

            box.setStyleSheet( StyleSheet() );
            box.setWindowTitle( "Plugin install" );
            box.setText( message.c_str() );
            box.setIcon( QMessageBox::Warning );
            box.setDetailedText( details.c_str() );
            box.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
            box.setDefaultButton( QMessageBox::Ok );

            if ( box.exec() == QMessageBox::Ok ) {
                spdlog::debug( "{} ({}):", name, version );
                for ( const auto& res : plugin[ "resources" ].get<std::vector<std::string>>() ) {
                    spdlog::debug( " - {}", res );

                    splash->showMessage(
                        std::format( "pulling plugin {} ({}): {}", name, version, res ).c_str(),
                        Qt::AlignLeft | Qt::AlignBottom,
                        Qt::white
                    );

                    if ( ! ServerPullResource( name, version, res ) ) {
                        spdlog::debug( "failed to pull resources for plugin: {}", name );
                        break;
                    }
                }
            }
        }
    }

    splash->showMessage( "process plugins and store", Qt::AlignLeft | Qt::AlignBottom, Qt::white );

    //
    // start the plugin worker of the store to load up all the
    // plugins from the local file system that we just pulled
    //
    Gui->PageScripts->processPlugins();

    splash->showMessage( "process scripts", Qt::AlignLeft | Qt::AlignBottom, Qt::white );

    //
    // load all scripts from the configuration
    //
    ScriptConfigProcess();

    //
    // now start up the meta worker to
    // pull the listeners and agent sessions
    //
    MetaWorker.Thread->start();
}

auto HavocClient::ServerPullResource(
    const std::string& name,
    const std::string& version,
    const std::string& resource
) -> bool {
    auto result       = httplib::Result();
    auto dir_plugin   = QDir();
    auto dir_resource = QDir();
    auto file_info    = QFileInfo();

    if ( ! ( dir_plugin = QDir( std::format( "{}/plugins/{}@{}", directory().path().toStdString(), name, version ).c_str() ) ).exists() ) {
        if ( ! dir_plugin.mkpath( "." ) ) {
            spdlog::error( "failed to create plugin directory {}", dir_plugin.path().toStdString() );
            return false;
        }
    }

    file_info    = QFileInfo( ( dir_plugin.path().toStdString() + "/" + resource ).c_str() );
    dir_resource = file_info.absoluteDir();

    if ( ! dir_resource.exists() ) {
        if ( ! dir_resource.mkpath( dir_resource.absolutePath() ) ) {
            spdlog::error( "failed to create resource path: {}", dir_resource.absolutePath().toStdString() );
            return false;
        }
    }

    auto fil_resource = QFile( file_info.absoluteFilePath() );
    if ( ! fil_resource.exists() ) {
        result = Havoc->ApiSend( "/api/plugin/resource", {
            { "name",     name     },
            { "resource", resource },
        } );

        if ( result->status != 200 ) {
            spdlog::debug("failed to pull plugin resource {} from {}: {}", name, resource, result->body);
            return false;
        }

        if ( fil_resource.open( QIODevice::WriteOnly ) ) {
            fil_resource.write( result->body.c_str(), static_cast<qint64>( result->body.length() ) );
        } else {
            spdlog::debug( "failed to open file resource locally {}", file_info.absoluteFilePath().toStdString() );
        }
    } else {
        spdlog::debug( "file resource already exists: {}", file_info.absoluteFilePath().toStdString() );
    }

    return true;
}

auto HavocClient::eventClosed() -> void {
    spdlog::error( "websocket closed" );
    Exit();
}

auto HavocClient::Server() const -> std::string { return Profile.Host + ":" + Profile.Port; }
auto HavocClient::Token()  const -> std::string { return Profile.Token; }

auto HavocClient::eventHandle(
    const QByteArray& request
) -> void {
    auto event = json::parse( request.toStdString() );

    /* check if we managed to parse the json event
     * if yes then dispatch it but if not then dismiss it */
    if ( ! event.is_discarded() ) {
        eventDispatch( event );
    } else {
        spdlog::error( "failed to parse event" );
        /* what now ?
         * I guess ignore since its not valid event
         * or debug print it I guess */
    }
}

auto HavocClient::StyleSheet(
    void
) -> QByteArray {
    if ( QFile::exists( "theme.css" ) ) {
        return Helper::FileRead( "theme.css" );
    }

    return Helper::FileRead( ":/style/default" );
}

auto HavocClient::AddProtocol(
    const std::string&  name,
    const py11::object& listener
) -> void {
    protocols.push_back( NamedObject{
        .name   = name,
        .object = listener
    } );
}

auto HavocClient::ProtocolObject(
    const std::string& name
) -> std::optional<py11::object> {
    for ( auto& listener : protocols ) {
        if ( listener.name == name ) {
            return listener.object;
        }
    }

    return std::nullopt;
}

auto HavocClient::Protocols() -> std::vector<std::string> {
    auto names = std::vector<std::string>();

    for ( auto& listener : protocols ) {
        names.push_back( listener.name );
    }

    return names;
}

auto HavocClient::SetupThreads() -> void {
    //
    // HcEventWorker thread
    //
    // now set up the event thread and dispatcher
    //
    Events.Thread = new QThread;
    Events.Worker = new HcEventWorker;
    Events.Worker->moveToThread( Events.Thread );

    connect( Events.Thread, &QThread::started, Events.Worker, &HcEventWorker::run );
    connect( Events.Worker, &HcEventWorker::availableEvent, this, &HavocClient::eventHandle );
    connect( Events.Worker, &HcEventWorker::socketClosed, this, &HavocClient::eventClosed );

    //
    // HcHeartbeatWorker thread
    //
    // start the heartbeat worker thread
    //
    Heartbeat.Thread = new QThread;
    Heartbeat.Worker = new HcHeartbeatWorker;
    Heartbeat.Worker->moveToThread( Heartbeat.Thread );

    connect( Heartbeat.Thread, &QThread::started, Heartbeat.Worker, &HcHeartbeatWorker::run );

    //
    // fire up the even thread that is going to
    // process heart beat events
    //
    Heartbeat.Thread->start();

    //
    // HcMetaWorker thread
    //
    // start the meta worker to pull listeners and agent sessions thread
    //
    MetaWorker.Thread = new QThread;
    MetaWorker.Worker = new HcMetaWorker;
    MetaWorker.Worker->moveToThread( MetaWorker.Thread );

    connect( MetaWorker.Thread, &QThread::started, MetaWorker.Worker, &HcMetaWorker::run );
    connect( MetaWorker.Worker, &HcMetaWorker::Finished, this, [&](){
        //
        // only start the event worker once the meta worker finished
        // pulling all the listeners, agents, etc. from the server
        //
        spdlog::info( "MetaWorker finished" );
        splash->close();
        Gui->renderWindow();
        Events.Thread->start();
    } );

    //
    // connect methods to add listeners, agents, etc. to the user interface (ui)
    //
    connect( MetaWorker.Worker, &HcMetaWorker::AddListener,          Gui, &HcMainWindow::AddListener  );
    connect( MetaWorker.Worker, &HcMetaWorker::AddAgent,             Gui, &HcMainWindow::AddAgent     );
    connect( MetaWorker.Worker, &HcMetaWorker::AddAgentConsole,      Gui, &HcMainWindow::AgentConsole );

    //
    // HcMetaWorker thread
    //
    // start the plugin worker to pull and register
    // client plugin scripts to the local file system
    //
    // PluginWorker.Thread = new QThread;
    // PluginWorker.Worker = new HcMetaWorker( true );
    // PluginWorker.Worker->moveToThread( PluginWorker.Thread );
    //
    // connect( PluginWorker.Thread, &QThread::started,  PluginWorker.Worker, &HcMetaWorker::run );
    // connect( PluginWorker.Worker, &HcMetaWorker::Finished, this, [&]() {
    //     spdlog::debug( "PluginWorker finished" );
    //
    //     splash->showMessage( "process plugins", Qt::AlignLeft | Qt::AlignBottom, Qt::white );
    //
    //     //
    //     // start the plugin worker of the store to load up all the
    //     // plugins from the local file system that we just pulled
    //     //
    //     Gui->PageScripts->processPlugins();
    //
    //     //
    //     // load the registered scripts
    //     //
    //     if ( Config.contains( "scripts" ) && Config.at( "scripts" ).is_table() ) {
    //         auto scripts_tbl = Config.at( "scripts" ).as_table();
    //
    //         if ( scripts_tbl.contains( "files" ) && scripts_tbl.at( "files" ).is_array() ) {
    //             for ( const auto& file : scripts_tbl.at( "files" ).as_array() ) {
    //                 if ( ! file.is_string() ) {
    //                     spdlog::error( "configuration scripts file value is not an string" );
    //                     continue;
    //                 }
    //
    //                 ScriptLoad( file.as_string() );
    //             }
    //         }
    //     }
    //
    //     //
    //     // now start up the meta worker to
    //     // pull the listeners and agent sessions
    //     //
    //     MetaWorker.Thread->start();
    // });
    //
    // PluginWorker.Thread->start();
}

auto HavocClient::AddBuilder(
    const std::string & name,
    const py11::object& builder
) -> void {
    builders.push_back( NamedObject{
        .name   = name,
        .object = builder
    } );
}

auto HavocClient::BuilderObject(
    const std::string& name
) -> std::optional<py11::object> {

    for ( auto& builder : builders ) {
        if ( builder.name == name ) {
            return builder.object;
        }
    }

    return std::nullopt;
}

auto HavocClient::Builders() -> std::vector<std::string>
{
    auto names = std::vector<std::string>();

    for ( auto& builder : builders ) {
        names.push_back( builder.name );
    }

    return names;
}

auto HavocClient::AddListener(
    const json& listener
) -> void {
    spdlog::debug( "listener -> {}", listener.dump() );
    listeners.push_back( listener );

    Gui->PageListener->addListener( listener );
}

auto HavocClient::RemoveListener(
    const std::string& name
) -> void {
    spdlog::debug( "removing listener: {}", name );

    const auto object = ListenerObject( name );

    //
    // remove listener from listener list
    //
    for ( auto iter = listeners.begin(); iter != listeners.end(); ++iter ) {
        if ( object.has_value() ) {
            const auto& data = object.value();
            if ( data.contains( "name" ) && data[ "name" ].is_string() ) {
                if ( data[ "name" ].get<std::string>() == name ) {
                    listeners.erase( iter );
                    break;
                }
            }
        }
    }

    Gui->PageListener->removeListener( name );
}

auto HavocClient::ListenerObject(
    const std::string &name
) -> std::optional<json> {

    for ( auto& data : listeners ) {
        if ( data.contains( "name" ) && data[ "name" ].is_string() ) {
            if ( data[ "name" ].get<std::string>() == name ) {
                return data;
            }
        }
    }

    return std::nullopt;
}

auto HavocClient::Listeners() -> std::vector<std::string>
{
    auto names = std::vector<std::string>();

    for ( auto& data : listeners ) {
        if ( data.contains( "name" ) && data[ "name" ].is_string() ) {
            names.push_back( data[ "name" ].get<std::string>() );
        }
    }

    return names;
}

auto HavocClient::Agents() const -> std::vector<HcAgent*>
{
    return Gui->PageAgent->agents;
}

auto HavocClient::Agent(
    const std::string& uuid
) const -> std::optional<HcAgent*> {
    for ( auto agent : Gui->PageAgent->agents ) {
        if ( agent->uuid == uuid ) {
            return agent;
        }
    }

    return std::nullopt;
}

auto HavocClient::AddAgentObject(
    const std::string&  type,
    const py11::object& object
) -> void {
    agents.push_back( NamedObject {
        .name   = type,
        .object = object
    } );
}

auto HavocClient::AgentObject(
    const std::string& type
) -> std::optional<py11::object> {
    for ( auto [name, object] : agents ) {
        if ( name == type ) {
            return object;
        }
    }

    return std::nullopt;
}

auto HavocClient::Callbacks() -> std::vector<std::string>
{
    auto names = std::vector<std::string>();

    for ( auto& callback : callbacks ) {
        names.push_back( callback.name );
    }

    return names;
}

auto HavocClient::AddCallbackObject(
    const std::string&  uuid,
    const py11::object& callback
) -> void {
    callbacks.push_back( NamedObject{
        .name   = uuid,
        .object = callback
    } );
}

auto HavocClient::RemoveCallbackObject(
    const std::string& callback
) -> void {
    //
    // iterate over the callbacks
    //
    for ( auto iter = callbacks.begin(); iter != callbacks.end(); ++iter ) {
        if ( iter->name == callback ) {
            callbacks.erase( iter );
            break;
        }
    }
}

auto HavocClient::CallbackObject(
    const std::string& uuid
) -> std::optional<py11::object> {
    //
    // iterate over registered callbacks
    //
    for ( auto [name, object] : callbacks ) {
        if ( name == uuid ) {
            return object;
        }
    }

    return std::nullopt;
}

auto HavocClient::SetupDirectory(
    void
) -> bool {
    auto havoc_dir   = QDir( QDir::homePath() + "/.havoc" );
    auto config_path = QFile();

    if ( ! havoc_dir.exists() ) {
        if ( ! havoc_dir.mkpath( "." ) ) {
            return false;
        }
    }

    client_dir.setPath( havoc_dir.path() + "/client" );
    if ( ! client_dir.exists() ) {
        if ( ! client_dir.mkpath( "." ) ) {
            return false;
        }
    }

    config_path.setFileName( client_dir.path() + "/config.toml" );
    config_path.open( QIODevice::ReadWrite );

    try {
        Config = toml::parse( config_path.fileName().toStdString() );
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse toml configuration: {}", e.what() );
        return false;
    }

    return true;
}

auto HavocClient::AddAction(
    ActionObject* action
) -> void {
    if ( ! action ) {
        spdlog::debug( "[AddAction] action is a nullptr" );
        return;
    }

    for ( auto _action : Actions( action->type ) ) {
        if ( _action->name == action->name ) {
            spdlog::debug( "[AddAction] action with the name \"{}\" and type already exists", action->name );
            return;
        }
    }

    actions.push_back( action );
}

auto HavocClient::Actions(
    const ActionObject::ActionType& type
) -> std::vector<ActionObject*> {
    auto actions_type = std::vector<ActionObject*>();

    for ( auto action : actions ) {
        if ( action->type == type ) {
            actions_type.push_back( action );
        }
    }

    return actions_type;
}

auto HavocClient::ScriptLoad(
    const std::string& path
) const -> void {
    Gui->PageScripts->LoadScript( path );
}

auto HavocClient::ScriptConfigProcess(
    void
) -> void {
    //
    // load the registered scripts
    //
    if ( Config.contains( "scripts" ) && Config.at( "scripts" ).is_table() ) {
        auto scripts_tbl = Config.at( "scripts" ).as_table();

        if ( scripts_tbl.contains( "files" ) && scripts_tbl.at( "files" ).is_array() ) {
            for ( const auto& file : scripts_tbl.at( "files" ).as_array() ) {
                if ( ! file.is_string() ) {
                    spdlog::error( "configuration scripts file value is not an string" );
                    continue;
                }

                ScriptLoad( file.as_string() );
            }
        }
    }
}

HavocClient::ActionObject::ActionObject() {}
