#include <Havoc.h>

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
    auto connector  = new HcConnectDialog();
    auto response   = json{};
    auto error      = std::string( "Failed to send login request: " );
    auto data       = json();
    auto stylesheet = StyleSheet();

    if ( argc >= 2 ) {
        //
        // check if we have specified the --debug/-d
        // flag to enable debug messages and logs
        //
        if ( ( strcmp( argv[ 1 ], "--debug" ) == 0 ) || ( strcmp( argv[ 1 ], "-d" ) == 0 ) ) {
            spdlog::set_level( spdlog::level::debug );
        }
    }

    if ( ( data = connector->start() ).empty() || ! connector->connected() ) {
        return;
    }

    if ( ! SetupDirectory() ) {
        spdlog::error( "failed to setup configuration directory. aborting" );
        return;
    }

    auto [token, ssl_hash] = ApiLogin( data );
    if ( ( ! token.has_value() ) || ( ! ssl_hash.has_value() ) ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Login failure",
            "Failed to login"
        );

        return Exit();
    }

    splash = new QSplashScreen( QGuiApplication::primaryScreen(), QPixmap( ":/images/splash-screen" ) );
    splash->show();

    Profile = {
        .Name    = data[ "name" ].get<std::string>(),
        .Host    = data[ "host" ].get<std::string>(),
        .Port    = data[ "port" ].get<std::string>(),
        .User    = data[ "username" ].get<std::string>(),
        .Pass    = data[ "password" ].get<std::string>(),
        .Token   = token.value(),
        .SslHash = ssl_hash.value(),
    };

    Theme.setStyleSheet( stylesheet );

    //
    // create main window
    //
    Gui = new HcMainWindow;
    Gui->setStyleSheet( stylesheet );

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
}

auto HavocClient::Exit() -> void {
    exit( 0 );
}

auto HavocClient::ApiLogin(
    const json& data
) -> std::tuple<
    std::optional<std::string>,
    std::optional<std::string>
> {
    auto error     = std::string();
    auto object    = json();
    auto api_login = std::format( "https://{}:{}/api/login", data[ "host" ].get<std::string>(), data[ "port" ].get<std::string>() );
    auto ssl_box   = QMessageBox();

    //
    // send request with the specified login data
    // and also get the remote server ssl hash
    //
    auto [status_code, response, ssl_hash] = RequestSend(
        api_login,
        data.dump(),
        false,
        "POST",
        "application/json"
    );

    if ( ! ssl_hash.empty() ) {
        ssl_box.setStyleSheet( StyleSheet() );
        ssl_box.setWindowTitle( "Verify SSL Fingerprint" );
        ssl_box.setText( std::format( "The team server's SSL fingerprint is: \n\n{}\n\nDoes this match the fingerprint presented in the server console?", ssl_hash ).c_str() );
        ssl_box.setIcon( QMessageBox::Warning );
        ssl_box.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
        ssl_box.setDefaultButton( QMessageBox::Yes );

        if ( ssl_box.exec() == QMessageBox::No ) {
            return {};
        }
    }

    if ( status_code == 401 ) {
        //
        // 401 Unauthorized: failed to log in
        //

        if ( ! response.has_value() ) {
            spdlog::error( "failed to login: unauthorized (invalid credentials)" );
            return {};
        }

        try {
            if ( ( object = json::parse( response.value() ) ).is_discarded() ) {
                spdlog::error( "failed to parse login json response: json has been discarded" );
                return {};
            }
        } catch ( std::exception& e ) {
            spdlog::error( "failed to parse login json response: \n{}", e.what() );
            return {};
        }

        if ( ! object.contains( "error" ) ) {
            return {};
        }

        if ( ! object[ "error" ].is_string() ) {
            return {};
        }

        spdlog::error( "failed to login: {}", object[ "error" ].get<std::string>() );

        return {};
    }

    if ( status_code != 200 ) {
        spdlog::error( "unexpected response: http status code {}", status_code );
        return {};
    }

    spdlog::info( "ssl hash: {}", ssl_hash );

    if ( response.value().empty() ) {
        return {};
    }

    //
    // parse the token from the request
    //

    try {
        if ( ( object = json::parse( response.value() ) ).is_discarded() ) {
            spdlog::error( "failed to parse login json response: json has been discarded" );
            return {};
        }
    } catch ( std::exception& e ) {
        spdlog::error( "failed to parse login json response: \n{}", e.what() );
        return {};
    }

    if ( ! object.contains( "token" ) || ! object[ "token" ].is_string() ) {
        return {};
    }

    return {
        object[ "token" ].get<std::string>(),
        ssl_hash
    };
}

auto HavocClient::ApiSend(
    const std::string& endpoint,
    const json&        body,
    const bool         keep_alive
) -> std::tuple<int, std::string> {
    auto [status_code, response, ssl_hash] = RequestSend(
        std::format( "https://{}:{}/{}", Havoc->Profile.Host, Havoc->Profile.Port, endpoint ),
        body.dump(),
        keep_alive,
        "POST",
        "application/json",
        true
    );

    //
    // validate that the ssl hash against the
    // profile that has been marked as safe
    //
    if ( ssl_hash != Havoc->Profile.SslHash ) {
        //
        // TODO: use QMetaObject::invokeMethod to call MessageBox
        //       to alert that the ssl hash has been changed
        //
        spdlog::error( "failed to send request: invalid ssl fingerprint detected ({})", ssl_hash );
        return {};
    }

    return { status_code, response.value() };
}

auto HavocClient::ServerPullPlugins(
    void
) -> void {
    auto plugins   = json();
    auto uuid      = std::string();
    auto name      = std::string();
    auto version   = std::string();
    auto resources = std::vector<std::string>();
    auto message   = std::string();
    auto details   = std::string();
    auto box       = QMessageBox();

    splash->showMessage( "pulling plugins information", Qt::AlignLeft | Qt::AlignBottom, Qt::white );

    auto [status_code, response] = ApiSend( "/api/plugin/list", {} );
    if ( status_code != 200 ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "plugin processing failure",
            "failed to pull all registered plugins"
        );
        return;
    }

    try {
        if ( ( plugins = json::parse( response ) ).is_discarded() ) {
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

        if ( ( ! plugin.contains( "resources" ) ) && ( ! plugin[ "resources" ].is_array() ) ) {
            continue;
        }

        //
        // we will display the user with a prompt to get confirmation
        // if they even want to install the plugin from the remote server
        //

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

        if ( box.exec() == QMessageBox::Cancel ) {
            continue;
        }

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
    auto dir_resource = QDir();
    auto dir_plugin   = QDir();
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
        auto [status_code, response] = Havoc->ApiSend( "/api/plugin/resource", {
            { "name",     name     },
            { "resource", resource },
        } );

        if ( status_code != 200 ) {
            spdlog::debug("failed to pull plugin resource {} from {}: {}", name, resource, response );
            return false;
        }

        if ( fil_resource.open( QIODevice::WriteOnly ) ) {
            fil_resource.write( QByteArray::fromStdString( response ) );
        } else {
            spdlog::debug( "failed to open file resource locally {}", file_info.absoluteFilePath().toStdString() );
        }
    } else {
        spdlog::debug( "file resource already exists: {}", file_info.absoluteFilePath().toStdString() );
    }

    return true;
}

auto HavocClient::RequestSend(
    const std::string&   url,
    const std::string&   data,
    const bool           keep_alive,
    const std::string&   method,
    const std::string&   content_type,
    const bool           havoc_token,
    const json::array_t& headers
) -> std::tuple<int, std::optional<std::string>, std::string> {
    auto request     = QNetworkRequest( QString::fromStdString( url ) );
    auto reply       = static_cast<QNetworkReply*>( nullptr );
    auto event       = QEventLoop();
    auto response    = std::string();
    auto status_code = 0;
    auto error       = QNetworkReply::NoError;
    auto error_str   = std::string();
    auto ssl_chain   = QList<QSslCertificate>();
    auto ssl_hash    = std::string();
    auto ssl_config  = QSslConfiguration::defaultConfiguration();
    auto _network    = QNetworkAccessManager();

    //
    // prepare request headers
    //

    if ( ! content_type.empty() ) {
        request.setHeader( QNetworkRequest::ContentTypeHeader, QString::fromStdString( content_type ) );
    }

    if ( havoc_token && ( ! Havoc->Profile.Token.empty() ) ) {
        request.setRawHeader( "x-havoc-token", Havoc->Profile.Token.c_str() );
    }

    for ( const auto& item : headers ) {
        for ( auto it = item.begin(); it != item.end(); ++it ) {
            std::cout << it.key() << ": " << it.value() << std::endl;
            request.setRawHeader( it.key().c_str(), it.value().get<std::string>().c_str() );
        }
    }

    if ( keep_alive ) {
        request.setRawHeader( "Connection", "Keep-Alive" );
    }

    //
    // we are going to adjust the ssl configuration
    // to disable certification verification
    //
    ssl_config.setPeerVerifyMode( QSslSocket::VerifyNone );
    request.setSslConfiguration( ssl_config );

    //
    // send request to the endpoint url via specified method
    //
    if ( method == "POST" ) {
        reply = _network.post( request, QByteArray::fromStdString( data ) );
    } else if ( method == "GET" ) {
        reply = _network.get( request );
    } else {
        spdlog::error( "[RequestSend] invalid http method: {}", method );
        return {};
    }

    //
    // we are going to wait for the request to finish, during the
    // waiting time we are going to continue executing the event loop
    //
    connect( reply, &QNetworkReply::finished, &event, &QEventLoop::quit );
    event.exec();

    //
    // parse the request data such as
    // error, response, and status code
    //
    error       = reply->error();
    error_str   = reply->errorString().toStdString();
    response    = reply->readAll().toStdString();
    status_code = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();

    //
    // get the SSL certificate SHA-256 hash
    //
    if ( error == QNetworkReply::NoError ) {
        if ( ! ( ssl_chain = reply->sslConfiguration().peerCertificateChain() ).isEmpty() ) {
            ssl_hash = ssl_chain.first().digest( QCryptographicHash::Sha256 ).toHex().toStdString();
        }
    }

    reply->deleteLater();

    //
    // now handle and retrieve the response from the request
    //
    if ( error != QNetworkReply::NoError ) {
        spdlog::error( "[RequestSend] invalid network reply: {}", error_str );
        return {};
    }

    return std::make_tuple( status_code, response, ssl_hash );
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

auto HavocClient::Protocols(
    void
) -> std::vector<std::string> {
    auto names = std::vector<std::string>();

    for ( auto& listener : protocols ) {
        names.push_back( listener.name );
    }

    return names;
}

auto HavocClient::SetupThreads(
    void
) -> void {
    //
    // HcEventWorker thread
    //
    // now set up the event thread and dispatcher
    //
    {
        connect( Events.Thread, &QThread::started, Events.Worker, &HcEventWorker::run );
        connect( Events.Worker, &HcEventWorker::availableEvent, this, &HavocClient::eventHandle );
        connect( Events.Worker, &HcEventWorker::socketClosed, this, &HavocClient::eventClosed );
    }

    //
    // HcHeartbeatWorker thread
    //
    // start the heartbeat worker thread
    //
    {
        connect( Heartbeat.Thread, &QThread::started, Heartbeat.Worker, &HcHeartbeatWorker::run );

        //
        // fire up the even thread that is going to
        // process heart beat events
        //
        Heartbeat.Thread->start();
    }

    //
    // HcMetaWorker thread
    //
    // start the meta worker to pull listeners and agent sessions thread
    //
    {
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
    }
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
