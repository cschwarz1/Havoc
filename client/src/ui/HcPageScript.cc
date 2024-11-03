#include <Havoc.h>
#include <ui/HcPageScript.h>

HcPagePlugins::HcPagePlugins()
{
    if ( objectName().isEmpty() ) {
        setObjectName( "PagePlugins" );
    }

    gridLayout = new QGridLayout( this );
    gridLayout->setObjectName( "gridLayout" );

    TabWidget = new QTabWidget( this );
    TabWidget->setObjectName( "TabWidget" );
    TabWidget->setProperty( "HcPageTab", "true" );
    TabWidget->tabBar()->setProperty( "HcPageTab", "true" );
    TabWidget->setFocusPolicy( Qt::NoFocus );

    TabPluginManager = new QWidget();
    TabPluginManager->setObjectName( "TabPluginManager" );

    gridLayout_2 = new QGridLayout( TabPluginManager );
    gridLayout_2->setObjectName( "gridLayout_2" );

    ButtonLoad = new QPushButton( TabPluginManager );
    ButtonLoad->setObjectName( "ButtonLoad" );
    ButtonLoad->setProperty( "HcButton", "true" );

    horizontalSpacer = new QSpacerItem( 972, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    LabelLoadedPlugins = new QLabel( TabPluginManager );
    LabelLoadedPlugins->setObjectName( "LabelLoadedPlugins" );
    LabelLoadedPlugins->setProperty( "HcLabelDisplay", "true" );

    splitter = new QSplitter( TabPluginManager );
    splitter->setObjectName( "splitter" );
    splitter->setOrientation( Qt::Vertical );

    TablePluginsWidget = new QTableWidget( splitter );
    TablePluginsWidget->setObjectName( "TablePluginsWidget" );
    TablePluginsWidget->setProperty( "HcPageTab", "true" );

    if ( TablePluginsWidget->columnCount() < 1 ) {
        TablePluginsWidget->setColumnCount( 1 );
    }

    TablePluginsWidget->setHorizontalHeaderItem( 0, new QTableWidgetItem( "Path" ) );

    /* table settings */
    TablePluginsWidget->setEnabled( true );
    TablePluginsWidget->setShowGrid( false );
    TablePluginsWidget->setSortingEnabled( false );
    TablePluginsWidget->setWordWrap( true );
    TablePluginsWidget->setCornerButtonEnabled( true );
    TablePluginsWidget->horizontalHeader()->setVisible( true );
    TablePluginsWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
    TablePluginsWidget->setContextMenuPolicy( Qt::CustomContextMenu );
    TablePluginsWidget->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeMode::Stretch );
    TablePluginsWidget->horizontalHeader()->setStretchLastSection( true );
    TablePluginsWidget->verticalHeader()->setVisible( false );
    TablePluginsWidget->setFocusPolicy( Qt::NoFocus );
    TablePluginsWidget->setAlternatingRowColors( true );

    PyConsole = new HcPyConsole( splitter );

    splitter->addWidget( TablePluginsWidget );
    splitter->addWidget( PyConsole );

    TabPluginStore = new HcStoreWidget();
    TabPluginStore->setObjectName( "TabPluginStore" );

    gridLayout_2->addWidget( ButtonLoad, 0, 0, 1, 1 );
    gridLayout_2->addItem( horizontalSpacer, 0, 1, 1, 1 );
    gridLayout_2->addWidget( LabelLoadedPlugins, 0, 2, 1, 1 );
    gridLayout_2->addWidget( splitter, 1, 0, 1, 3 );
    gridLayout_2->setContentsMargins( 0, 0, 0, 0 );

    gridLayout->addWidget( TabWidget, 0, 0, 1, 1 );

    TabWidget->addTab( TabPluginManager, "Manager" );
    TabWidget->addTab( TabPluginStore, "Store" );

    retranslateUi();

    //
    // signals
    //
    QObject::connect( this, & HcPagePlugins::SignalConsoleWrite, PyConsole, & HcPyConsole::append );

    QObject::connect( ButtonLoad, &QPushButton::clicked, this, [&] () {
        auto dialog    = QFileDialog();
        auto plugin    = QUrl();
        auto exception = std::string();

        dialog.setStyleSheet( HavocClient::StyleSheet() );
        dialog.setDirectory( QDir::homePath() );

        if ( LoadCallback.has_value() ) {
            if ( dialog.exec() == QFileDialog::Accepted ) {
                plugin = dialog.selectedUrls().value( 0 ).toLocalFile();
                if ( ! plugin.toString().isNull() ) {
                    try {
                        HcPythonAcquire();
                        LoadCallback.value()( py11::str( plugin.toString().toStdString() ) );
                    } catch ( py11::error_already_set &eas ) {
                        exception = eas.what();
                    }

                    if ( ! exception.empty() ) {
                        //
                        // print it to the python console
                        //
                        PyConsole->append( exception.c_str() );

                        //
                        // print it to the terminal console
                        // as well
                        //
                        spdlog::error( "python callstack: \n{}", exception );
                    }
                }
            }
        } else {
            Helper::MessageBox(
                QMessageBox::Warning,
                "Script Manager",
                "No load script handler has been registered"
            );
        }
    } );

    TabWidget->setCurrentIndex( 0 );

    QMetaObject::connectSlotsByName( this );
}

auto HcPagePlugins::retranslateUi() -> void {
    setWindowTitle( "PagePlugins" );
    setStyleSheet( Havoc->StyleSheet() );
    ButtonLoad->setText( "Load Plugin" );
    LabelLoadedPlugins->setText( std::format( "Loaded: {}", TablePluginsWidget->rowCount() ).c_str() );
}

auto HcPagePlugins::LoadScript(
    const std::string& path
) -> void {
    if ( ! QFile( path.c_str() ).exists() ) {
        spdlog::error( "attempted to load invalid scripts path: {}", path );
        return;
    }

    spdlog::debug( "attempting to load script: {}", path );

    if ( LoadCallback.has_value() ) {
        try {
            HcPythonAcquire();

            LoadCallback.value()( path );

            AddScriptPath( path.c_str() );
        } catch ( py11::error_already_set &eas ) {
            spdlog::error( "failed to load script {}: \n{}", path, eas.what() );
            PyConsole->append( std::format( "failed to load script {}: \n{}", path, eas.what() ).c_str() );
        }
    } else {
        spdlog::debug( "PageScripts->LoadCallback not set" );
    }
}

auto HcPagePlugins::AddScriptPath(
    const QString& path
) -> void {
    auto row  = TablePluginsWidget->rowCount();
    auto sort = TablePluginsWidget->isSortingEnabled();
    auto item = new QTableWidgetItem( QFileInfo( path ).canonicalFilePath() );

    item->setFlags( item->flags() ^ Qt::ItemIsEditable );

    TablePluginsWidget->setRowCount( row + 1 );
    TablePluginsWidget->setSortingEnabled( false );
    TablePluginsWidget->setItem( row, 0, item );
    TablePluginsWidget->setSortingEnabled( sort );

    retranslateUi();
}

auto HcPagePlugins::processPlugins(
    void
) -> void {
    //
    // start the plugin store worker to process
    // and pull repositories and plugins
    //

    TabPluginStore->PluginWorker.Thread->start();

    //
    // register all specified repositories
    //

    if ( Havoc->config.contains( "repository" ) ) {
        for ( const auto& repository : toml::find<toml::array>( Havoc->config, "repository" ) ) {
            if ( repository.contains( "link" ) && repository.at( "link" ).is_string() ) {
                auto plugins      = std::vector<std::string>();
                auto access_token = std::string();

                //
                // check if there are any plugins to be installed
                //
                if ( repository.contains( "plugins" ) && repository.at( "plugins" ).is_array() ) {
                    for ( const auto& plugin : repository.at( "plugins" ).as_array() ) {
                        if ( plugin.is_string() ) {
                            plugins.push_back( plugin.as_string().str );
                        }
                    }
                }

                //
                // check if there has been an access token specified
                //
                if ( repository.contains( "access_token" ) && repository.at( "access_token" ).is_string() ) {
                    access_token = repository.at( "access_token" ).as_string();
                }

                emit TabPluginStore->RegisterRepository(
                    toml::find<std::string>( repository, "link" ),
                    plugins,
                    access_token
                );
            }
        }
    };

    if ( Havoc->directory().exists() ) {
        auto plugins = QDir( Havoc->directory().path() + "/plugins" );

        if ( plugins.exists() ) {
            for ( const auto& plugin : plugins.entryList( QDir::AllDirs | QDir::NoDotAndDotDot ) ) {
                auto plugin_path = plugins.path() + "/" + plugin;
                auto plugin_json = QFile( plugin_path + "/plugin.json" );
                auto json_object = json();

                spdlog::debug( "local plugin found -> {}", plugin.toStdString() );

                if ( ! plugin_json.exists() ) {
                    spdlog::warn( "local plugin {} does not contain json metadata file", plugin.toStdString() );
                    continue;
                }

                try {
                    plugin_json.open( QFile::ReadOnly );
                    if ( ( json_object = json::parse( plugin_json.readAll().toStdString() ) ).is_discarded() ) {
                        spdlog::error( "failed to parse json object from local plugin {}: discarded", plugin.toStdString() );
                        continue;
                    }
                } catch ( std::exception& e ) {
                    spdlog::error( "failed to parse json object from local plugin {}: {}", plugin.toStdString(), e.what() );
                    continue;
                }

                TabPluginStore->PluginWorker.Worker->RegisterPlugin( plugin_path.toStdString(), json_object );
            }
        } else {
            plugins.mkpath( "." );
        }
    }
}

HcPyConsole::HcPyConsole(
    QWidget* parent
) : QTextEdit( parent ) {
    setObjectName( "HcPyConsole" );
    setReadOnly( true );
    setProperty( "HcConsole", "true" );
}
