#include <Havoc.h>
#include <core/HcHelper.h>
#include <ui/HcConnectDialog.h>

class HcConnectionItem final : public QWidget
{
    QGridLayout* GridLayout;
    QLabel*      LabelName;
    QLabel*      LabelDetails;

public:
    QString name;
    QString host;
    QString port;
    QString username;
    QString password;

    explicit HcConnectionItem(
        const QString& name,
        const QString& host,
        const QString& port,
        const QString& username,
        const QString& password,
        QObject*       parent   = nullptr
    ) : name( name ), host( host ), port( port ),
        username( username ), password( password )
    {
        GridLayout = new QGridLayout( this );
        GridLayout->setObjectName( "gridLayout" );

        LabelName = new QLabel( this );
        LabelName->setText( name );

        auto font = LabelName->font();
        font.setBold( true );
        LabelName->setFont( font );

        LabelDetails = new QLabel( this );
        LabelDetails->setText( std::format(
            "{} @ {}:{}",
            username.toStdString(), host.toStdString(), port.toStdString()
        ).c_str() );

        GridLayout->addWidget( LabelName,    0, 0, 1, 1 );
        GridLayout->addWidget( LabelDetails, 1, 0, 1, 1 );
        GridLayout->setColumnStretch( 0, 1 );

        QMetaObject::connectSlotsByName( this );
    }
};

HcConnectDialog::HcConnectDialog() {
    if ( objectName().isEmpty() ) {
        setObjectName( QString::fromUtf8( "HcConnectDialog" ) );
    }

    resize( 600, 261 );
    setMinimumSize( QSize( 600, 0 ) );
    setMaximumSize( QSize( 800, 300 ) );

    horizontalLayout = new QHBoxLayout( this );
    ConnectionWidget = new QWidget( this );
    gridLayout       = new QGridLayout( ConnectionWidget );
    LabelHavoc       = new QLabel( ConnectionWidget );
    InputProfileName = new HcLineEdit( ConnectionWidget );
    InputHost        = new HcLineEdit( ConnectionWidget );
    InputPort        = new HcLineEdit( ConnectionWidget );
    InputUsername    = new HcLineEdit( ConnectionWidget );
    InputPassword    = new HcLineEdit( ConnectionWidget );
    ButtonConnect    = new QPushButton( ConnectionWidget );
    ButtonAdd        = new QPushButton( ConnectionWidget );
    ListConnection   = new QListWidget( this );

    horizontalLayout->setObjectName( QString::fromUtf8( "horizontalLayout" ) );
    ConnectionWidget->setObjectName( QString::fromUtf8( "ConnectionWidget" ) );

    gridLayout->setObjectName( QString::fromUtf8( "gridLayout" ) );
    LabelHavoc->setObjectName( QString::fromUtf8( "LabelHavoc" ) );
    LabelHavoc->setMaximumSize( QSize( 200, 40 ) );

    InputProfileName->setObjectName( QString::fromUtf8( "InputProfileName" ) );
    InputProfileName->setMinimumSize( QSize( 0, 30 ) );

    InputHost->setObjectName( QString::fromUtf8( "InputHost" ) );
    InputHost->setMinimumSize( QSize( 0, 30 ) );

    InputPort->setObjectName( QString::fromUtf8( "InputPort" ) );
    InputPort->setMinimumSize( QSize( 0, 30 ) );
    InputPort->setValidator( new QRegularExpressionValidator( QRegularExpression( "[0-9]*" ), InputPort ) );

    InputUsername->setObjectName( QString::fromUtf8( "InputUsername" ) );
    InputUsername->setMinimumSize( QSize( 0, 30 ) );

    InputPassword->setObjectName( QString::fromUtf8( "InputPassword" ) );
    InputPassword->setMinimumSize( QSize( 0, 30 ) );
    InputPassword->Input->setEchoMode( QLineEdit::Password );
    ActionPassBlinder = InputPassword->Input->addAction( QIcon( ":/icons/16px-blind-white" ), QLineEdit::TrailingPosition );

    ButtonConnect->setObjectName( QString::fromUtf8( "ButtonConnect" ) );
    ButtonConnect->setMinimumSize( QSize( 0, 30 ) );
    ButtonConnect->setProperty( "HcButton", "true" );

    ButtonAdd->setObjectName( QString::fromUtf8( "ButtonAdd" ) );
    ButtonAdd->setMinimumSize( QSize( 0, 30 ) );
    ButtonAdd->setProperty( "HcButton", "true" );

    ListConnection->setObjectName( QString::fromUtf8( "ListConnection" ) );
    ListConnection->setProperty( "HcListWidget", "half-dark" );
    ListConnection->setContextMenuPolicy( Qt::CustomContextMenu );

    gridLayout->addWidget( LabelHavoc, 0, 0, 1, 2 );
    gridLayout->addWidget( InputProfileName, 1, 0, 1, 2 );
    gridLayout->addWidget( InputHost, 2, 0, 1, 1 );
    gridLayout->addWidget( InputPort, 2, 1, 1, 1 );
    gridLayout->addWidget( InputUsername, 3, 0, 1, 2 );
    gridLayout->addWidget( InputPassword, 4, 0, 1, 2 );
    gridLayout->addWidget( ButtonConnect, 5, 0, 1, 1 );
    gridLayout->addWidget( ButtonAdd, 5, 1, 1, 1 );

    horizontalLayout->addWidget( ConnectionWidget );
    horizontalLayout->addWidget( ListConnection );

    retranslateUi();

    /* add events to buttons */
    /* event when the "Connect" button is pressed/clicked.
    * is going to close the Connection dialog and set the "Connected" bool to true */
    connect( ButtonConnect, &QPushButton::clicked, this, [&] {
        if ( sanityCheckInput() ) {
            PressedConnect = true;
            close();
        }
    } );

    connect( ButtonAdd, &QPushButton::clicked, this, [&] {
        InputProfileName->setInputText( "" );
        InputHost->setInputText( "" );
        InputPort->setInputText( "" );
        InputUsername->setInputText( "" );
        InputPassword->setInputText( "" );

        ListConnection->clearSelection();
    } );

    connect( ActionPassBlinder, &QAction::triggered, this, [&] {
        if ( ! PassBlinderToggle ) {
            InputPassword->Input->setEchoMode( QLineEdit::Normal );
            ActionPassBlinder->setIcon( QIcon( ":/icons/16px-eye-white" ) );
        } else {
            InputPassword->Input->setEchoMode( QLineEdit::Password );
            ActionPassBlinder->setIcon( QIcon( ":/icons/16px-blind-white" ) );
        }

        PassBlinderToggle = ! PassBlinderToggle;
    } );

    connect( ListConnection, &QListWidget::itemClicked, this, [&] ( QListWidgetItem *item )  {
        if ( ! item ) {
            return;
        }

        auto widget = dynamic_cast<HcConnectionItem*>( ListConnection->itemWidget( item ) );

        InputProfileName->setInputText( widget->name );
        InputHost->setInputText( widget->host );
        InputPort->setInputText( widget->port );
        InputUsername->setInputText( widget->username );
        InputPassword->setInputText( widget->password );
    } );

    connect( ListConnection, &QListWidget::customContextMenuRequested, this, [&] ( const QPoint& pos ) {
        auto menu = QMenu();
        auto item = ListConnection->itemAt( pos );

        if ( item ) {
            menu.setStyleSheet( HavocClient::StyleSheet() );
            menu.addAction( "Remove" );
            menu.addAction( "Remove All" );

            auto widget = dynamic_cast<HcConnectionItem*>( ListConnection->itemWidget( item ) );
            auto action = menu.exec( ListConnection->viewport()->mapToGlobal( pos ) );

            if ( !action ) {
                return;
            }

            if ( action->text() == "Remove" ) {
                auto index = 0;
                for ( const auto& connection : Havoc->ProfileQuery( "connection" ) ) {
                    auto name = std::string();

                    if ( ! connection.contains( "name" ) ) {
                        continue;
                    }

                    name = toml::find<std::string>( connection, "name" );
                    if ( name == widget->name.toStdString() ) {
                        Havoc->ProfileDelete( "connection", index );

                        retranslateUi();
                    }

                    ++index;
                }
            } else if ( action->text() == "Remove All" ) {
                Havoc->ProfileDelete( "connection" );

                retranslateUi();
            }
        }
    } );

    QMetaObject::connectSlotsByName( this );
}

HcConnectDialog::~HcConnectDialog() {
    delete horizontalLayout;
    delete ConnectionWidget;
    delete gridLayout;
    delete LabelHavoc;
    delete InputProfileName;
    delete InputHost;
    delete InputPort;
    delete InputUsername;
    delete InputPassword;
    delete ButtonConnect;
    delete ButtonAdd;
    delete ListConnection;
}

void HcConnectDialog::retranslateUi() {
    setWindowTitle( QCoreApplication::translate( "HcConnectDialog", "Havoc Connect", nullptr ) );
    setStyleSheet( HavocClient::StyleSheet() );
    LabelHavoc->setText( "<html><head/><body><p><span style=\" font-size:11pt;\">Havoc [ welcome back ]</span></p></body></html>" );
    ButtonConnect->setText( QCoreApplication::translate( "HcConnectDialog", "CONNECT", nullptr ) );
    ButtonAdd->setText( QCoreApplication::translate( "HcConnectDialog", "NEW", nullptr ) );

    InputProfileName->setLabelText( "Profile:" );
    InputHost->setLabelText( "Host:   " );
    InputPort->setLabelText( "Port:" );
    InputUsername->setLabelText( "User:   " );
    InputPassword->setLabelText( "Pass:   " );

    //
    // add all connection details to the list
    //

    for ( int i = 0; i < ListConnection->count(); ++i ) {
        auto item   = ListConnection->item( i );
        auto widget = ListConnection->itemWidget( item );

        if ( widget ) {
            delete widget;
        }

        delete item;
    }

    ListConnection->clear();

    for ( const auto& connection : Havoc->ProfileQuery( "connection" ) ) {
        if ( !connection.contains( "name" ) ) {
            spdlog::error( "profile connection entry does not contain a name" );
            continue;
        }

        if ( !connection.contains( "host" ) ) {
            spdlog::error( "profile connection entry does not contain a host" );
            continue;
        }

        if ( !connection.contains( "port" ) ) {
            spdlog::error( "profile connection entry does not contain a port" );
            continue;
        }

        if ( !connection.contains( "username" ) ) {
            spdlog::error( "profile connection entry does not contain a username" );
            continue;
        }

        if ( !connection.contains( "password" ) ) {
            spdlog::error( "profile connection entry does not contain a password" );
            continue;
        }

        auto item   = new QListWidgetItem;
        auto name   = toml::find<std::string>( connection, "name" );
        auto host   = toml::find<std::string>( connection, "host" );
        auto port   = toml::find<std::string>( connection, "port" );
        auto user   = toml::find<std::string>( connection, "username" );
        auto pass   = toml::find<std::string>( connection, "password" );
        auto widget = new HcConnectionItem(
            QString::fromStdString( name ),
            QString::fromStdString( host ),
            QString::fromStdString( port ),
            QString::fromStdString( user ),
            QString::fromStdString( pass ),
            this
        );

        item->setSizeHint( widget->sizeHint() );

        ListConnection->addItem( item );
        ListConnection->setItemWidget( item, widget );
    }
}

auto HcConnectDialog::start() -> json {
    retranslateUi();
    exec();

    if ( InputProfileName->text().isEmpty() ||
         InputHost->text().isEmpty()        ||
         InputPort->text().isEmpty()        ||
         InputUsername->text().isEmpty()    ||
         InputPassword->text().isEmpty()    ||
         ! PressedConnect
    ) {
        return {};
    }

    return {
        { "name",     InputProfileName->text().toStdString() },
        { "host",     InputHost->text().toStdString()        },
        { "port",     InputPort->text().toStdString()        },
        { "username", InputUsername->text().toStdString()    },
        { "password", InputPassword->text().toStdString()    },
    };
}

auto HcConnectDialog::sanityCheckInput(
    void
) -> bool {
    if ( InputProfileName->text().isEmpty() ) {
        Helper::MessageBox( QMessageBox::Critical, "Profile Error", "Profile field is emtpy." );
        return false;
    }

    if ( InputHost->text().isEmpty() ) {
        Helper::MessageBox( QMessageBox::Critical, "Profile Error", "Host field is emtpy." );
        return false;
    }

    if ( InputPort->text().isEmpty() ) {
        Helper::MessageBox( QMessageBox::Critical, "Profile Error", "Port field is emtpy." );
        return false;
    }

    if ( InputUsername->text().isEmpty() ) {
        Helper::MessageBox( QMessageBox::Critical, "Profile Error", "Username field is emtpy." );
        return false;
    }

    if ( InputPassword->text().isEmpty() ) {
        Helper::MessageBox( QMessageBox::Critical, "Profile Error", "Password field is emtpy." );
        return false;
    }

    return true;
}

auto HcConnectDialog::connected() const -> bool { return PressedConnect; }



