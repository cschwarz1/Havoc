#include <Common.h>
#include <Havoc.h>
#include <ui/HcDialogBuilder.h>

HcDialogBuilder::HcDialogBuilder(
    QWidget* parent
) : QDialog( parent ) {

    if ( objectName().isEmpty() ) {
        setObjectName( "HcPageBuilder" );
    }

    gridLayout_2 = new QGridLayout( this );
    gridLayout_2->setObjectName( "gridLayout_2" );

    LabelPayload = new QLabel( this );
    LabelPayload->setObjectName( "LabelPayload" );
    LabelPayload->setProperty( "HcLabelDisplay", "true" );

    ComboPayload = new QComboBox( this );
    ComboPayload->setObjectName( "ComboPayload" );
    ComboPayload->setMinimumWidth( 200 );

    horizontalSpacer = new QSpacerItem( 789, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    SplitterTopBottom = new QSplitter( this );
    SplitterTopBottom->setObjectName( "SplitterTopBottom" );
    SplitterTopBottom->setOrientation( Qt::Vertical );

    SplitterLeftRight = new QSplitter( SplitterTopBottom );
    SplitterLeftRight->setObjectName( "SplitterLeftRight" );
    SplitterLeftRight->setOrientation( Qt::Horizontal );

    StackedBuilders = new QStackedWidget( SplitterLeftRight );
    StackedBuilders->setObjectName( "StackedBuilders" );

    ProfileWidget = new QWidget( SplitterLeftRight );
    ProfileWidget->setObjectName( "ProfileWidget" );
    ProfileWidget->setContentsMargins( 0, 0, 0, 0 );

    gridLayout = new QGridLayout( ProfileWidget );
    gridLayout->setObjectName( "gridLayout" );

    ButtonGenerate = new QPushButton( ProfileWidget );
    ButtonGenerate->setObjectName( "ButtonGenerate" );
    ButtonGenerate->setProperty( "HcButton", "true" );
    ButtonGenerate->setProperty( "HcButtonPurple", "true" );

    ButtonSaveProfile = new QPushButton( ProfileWidget );
    ButtonSaveProfile->setObjectName( "ButtonSaveProfile" );
    ButtonSaveProfile->setProperty( "HcButton", "true" );

    ButtonLoadProfile = new QPushButton( ProfileWidget );
    ButtonLoadProfile->setObjectName( "ButtonLoadProfile" );
    ButtonLoadProfile->setProperty( "HcButton", "true" );

    ListProfiles = new QListWidget( ProfileWidget );
    ListProfiles->setObjectName( "ListProfiles" );

    TextBuildLog = new QTextEdit( SplitterTopBottom );
    TextBuildLog->setObjectName( "TextBuildLog" );
    TextBuildLog->setReadOnly( true );

    SplitterLeftRight->addWidget( StackedBuilders );
    SplitterLeftRight->addWidget( ProfileWidget );
    SplitterTopBottom->addWidget( SplitterLeftRight );
    SplitterTopBottom->addWidget( TextBuildLog );

    gridLayout->addWidget( ButtonGenerate,    0, 0, 1, 1 );
    gridLayout->addWidget( ButtonSaveProfile, 1, 0, 1, 1 );
    gridLayout->addWidget( ButtonLoadProfile, 2, 0, 1, 1 );
    gridLayout->addWidget( ListProfiles,      3, 0, 1, 1 );

    gridLayout_2->addWidget( LabelPayload,      0, 0, 1, 1 );
    gridLayout_2->addWidget( ComboPayload,      0, 1, 1, 1 );
    gridLayout_2->addWidget( SplitterTopBottom, 1, 0, 1, 3 );
    gridLayout_2->addItem( horizontalSpacer, 0, 2, 1, 1 );

    retranslateUi();

    connect( ButtonGenerate,    &QPushButton::clicked, this, &HcDialogBuilder::PressedGenerate    );
    connect( ButtonSaveProfile, &QPushButton::clicked, this, &HcDialogBuilder::PressedSaveProfile );
    connect( ButtonLoadProfile, &QPushButton::clicked, this, &HcDialogBuilder::PressedLoadProfile );

    for ( auto& name : Havoc->Builders() ) {
        if ( Havoc->BuilderObject( name ).has_value() ) {
            AddBuilder( name, Havoc->BuilderObject( name ).value() );
        }
    }

    QMetaObject::connectSlotsByName( this );
}

HcDialogBuilder::~HcDialogBuilder() {
    for ( auto& builder : Builders ) {
        delete builder.widget;
    }
}

auto HcDialogBuilder::retranslateUi() -> void {
    setWindowTitle( "Payload Builder" );

    setStyleSheet( HavocClient::StyleSheet() );
    resize( 900, 880 );

    ComboPayload->addItem( "(no payload available)" );
    LabelPayload->setText( "Payload:" );
    ButtonGenerate->setText( "Generate" );
    ButtonSaveProfile->setText( "Save Profile" );
    ButtonLoadProfile->setText( "Load Profile" );

    SplitterTopBottom->setSizes( QList<int>() << 0 );
    SplitterLeftRight->setSizes( QList<int>() << 1374 << 436 );
}

auto HcDialogBuilder::AddBuilder(
    const std::string&  name,
    const py11::object& object
) -> void {
    auto objname = "HcPageBuilderBuilder" + QString( name.c_str() );
    auto builder = Builder {
        .name    = name,
        .widget  = new QWidget
    };

    builder.widget->setObjectName( objname );
    builder.widget->setStyleSheet( "#" + objname + "{ background: " + Havoc->Theme.getBackground().name() + "}" );

    try {
        HcPythonAcquire();

        builder.instance = object( U_PTR( builder.widget ), U_PTR( this ), name );
        builder.instance.attr( "_hc_main" )();
    } catch ( py11::error_already_set &eas ) {
        Helper::MessageBox(
            QMessageBox::Icon::Critical,
            "Builder python error",
            std::string( eas.what() )
        );
        ErrorReceived = true;
        return;
    }

    if ( Builders.empty() ) {
        ComboPayload->clear();
        ComboPayload->setEnabled( true );
    }

    ComboPayload->addItem( builder.name.c_str() );
    StackedBuilders->addWidget( builder.widget );

    Builders.push_back( builder );
}

auto HcDialogBuilder::PressedGenerate(
    void
) -> void {
    auto data   = json();
    auto body   = json();
    auto config = json();
    auto name   = ComboPayload->currentText().toStdString();

    TextBuildLog->clear();
    if ( SplitterTopBottom->sizes()[ 0 ] == 0 ) {
        SplitterTopBottom->setSizes( QList<int>() << 400 << 200 );
    }

    if ( Builders.empty() ) {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Payload build failure",
            "Failed to build payload: no builder registered"
        );

        return;
    }

    //
    // get the builder instance
    //
    if ( auto object = BuilderObject( name ) ) {
        //
        // check if the builder exists
        //
        if ( ! object.has_value() ) {
            Helper::MessageBox(
                QMessageBox::Critical,
                "Payload build failure",
                std::format( "specified payload builder does not exist: {}", name )
            );
            return;
        }

        auto builder = object.value();

        //
        // scoped sanity check and generation of payload
        // and interacting with the builder instance
        //
        {
            HcPythonAcquire();

            try {
                if ( ! builder.attr( "sanity_check" )().cast<bool>() ) {
                    spdlog::debug( "sanity check failed. exit and dont send request" );

                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "Payload build failure",
                        std::format( "sanity check failed: {}", ComboPayload->currentText().toStdString() )
                    );

                    return;
                }

                //
                // generate a context or configuration
                // from the builder that we are going to
                // send to the server plugin
                //
                config = builder.attr( "generate" )();
            } catch ( py11::error_already_set &eas ) {
                spdlog::error( "failed to refresh builder \"{}\": \n{}", name, eas.what() );
                return;
            }
        }

        //
        // we are now going to send a request to our api
        // endpoint to receive a payload from the server
        //

        data = {
            { "name",   name   },
            { "config", config },
        };

        auto [status_code, response] = Havoc->ApiSend( "/api/agent/build", data );

        if ( status_code != 200 ) {
            if ( ( data = json::parse( response ) ).is_discarded() ) {
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
                "Payload build failure",
                QString( "Failed to build payload \"%1\": %2" ).arg( name.c_str() ).arg( data[ "error" ].get<std::string>().c_str() ).toStdString()
            );
        } else {
            auto dialog  = QFileDialog();
            auto path    = QString();
            auto file    = QFile();
            auto payload = QByteArray();
            auto context = json();

            if ( ( data = json::parse( response ) ).is_discarded() ) {
                goto ERROR_SERVER_RESPONSE;
            }

            //
            // get the file name of the generated implant
            //
            if ( data.contains( "filename" ) ) {
                if ( data[ "filename" ].is_string() ) {
                    name = data[ "filename" ].get<std::string>();
                } else {
                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "Payload build error",
                        "invalid response: payload file name is not a string"
                    );
                    return;
                }
            } else {
                Helper::MessageBox(
                    QMessageBox::Critical,
                    "Payload build error",
                    "invalid response: payload file name not specified"
                );
                return;
            }

            //
            // get the payload data of the generated implant
            //
            if ( data.contains( "payload" ) ) {
                if ( data[ "payload" ].is_string() ) {
                    payload = QByteArray::fromBase64( data[ "payload" ].get<std::string>().c_str() );
                }  else {
                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "Payload build error",
                        "invalid response: payload data is not a string"
                    );
                    return;
                }
            }  else {
                Helper::MessageBox(
                    QMessageBox::Critical,
                    "Payload build error",
                    "invalid response: payload data not specified"
                );
                return;
            }

            //
            // get the payload context of the generated implant
            //
            if ( data.contains( "context" ) ) {
                if ( data[ "context" ].is_object() ) {
                    context = data[ "context" ].get<json>();
                }  else {
                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "Payload build error",
                        "invalid response: payload context is not an object"
                    );
                    return;
                }
            }  else {
                Helper::MessageBox(
                    QMessageBox::Critical,
                    "Payload build error",
                    "invalid response: payload context not specified"
                );
                return;
            }

            //
            // process payload by passing it to python builder instance
            // we are also creating a scope for it so the gil can be released
            // at the end of the scope after finishing interacting with the
            // python builder instance
            //
            {
                HcPythonAcquire();

                try {
                    //
                    // do some post payload processing after retrieving
                    // the payload from the remote server plus the entire
                    // configuration we have to include into the payload
                    //
                    auto processed = builder.attr( "payload_process" )(
                        py11::bytes( payload.toStdString().c_str(), payload.toStdString().length() ),
                        context
                    ).cast<std::string>();

                    //
                    // set the payload from the processed value we retrieved
                    // back from the python builder instance
                    //
                    payload = QByteArray( processed.c_str(), processed.length() );
                } catch ( py11::error_already_set &eas ) {
                    spdlog::error( "failed to process payload \"{}\": \n{}", name, eas.what() );

                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "Payload build failure",
                        std::format( "failed to process payload \"{}\": \n{}", name, eas.what() )
                    );
                    return;
                }
            }

            dialog.setStyleSheet( Havoc->StyleSheet() );
            dialog.setDirectory( QDir::homePath() );
            dialog.selectFile( name.c_str() );
            dialog.setAcceptMode( QFileDialog::AcceptSave );

            if ( dialog.exec() == QFileDialog::Accepted ) {
                path = dialog.selectedFiles().value( 0 );

                file.setFileName( path );
                if ( file.open( QIODevice::ReadWrite ) ) {
                    file.write( payload );

                    Helper::MessageBox(
                        QMessageBox::Information,
                        "Payload build",
                        std::format( "saved payload under:\n{}", path.toStdString() )
                    );
                } else {
                    Helper::MessageBox(
                        QMessageBox::Critical,
                        "Payload build failure",
                        std::format( "Failed to write payload to \"{}\": {}", path.toStdString(), file.errorString().toStdString() )
                    );
                }
            }

            return;
        }

    } else {
        Helper::MessageBox(
            QMessageBox::Critical,
            "Payload build failure",
            std::format( "specified payload builder does not exist: {}", ComboPayload->currentText().toStdString() )
        );
        return;
    }

ERROR_SERVER_RESPONSE:
    Helper::MessageBox(
        QMessageBox::Critical,
        "Payload build failure",
        QString( "Failed to build payload \"%1\": Invalid response from the server" ).arg( name.c_str() ).toStdString()
    );
}

auto HcDialogBuilder::PressedSaveProfile() -> void
{

}

auto HcDialogBuilder::PressedLoadProfile() -> void
{

}

auto HcDialogBuilder::EventBuildLog(
    const QString& log
) -> void {
    //
    // add support for coloring texts
    //
    TextBuildLog->append( log );
}

auto HcDialogBuilder::BuilderObject(
    const std::string& name
) -> std::optional<py11::object> {

    for ( int i = 0; i < Builders.size(); i++ ) {
        if ( Builders[ i ].name == name ) {
            return Builders[ i ].instance;
        }
    }

    return std::nullopt;
}
