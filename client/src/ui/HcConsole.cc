#include <Havoc.h>
#include <ui/HcConsole.h>

HcConsole::HcConsole(
    QWidget* parent
) : QWidget( parent ) {
    if ( objectName().isEmpty() ) {
        setObjectName( "HcConsole" );
    }

    gridLayout = new QGridLayout( this );
    gridLayout->setObjectName( "gridLayout" );
    gridLayout->setContentsMargins( 0, 0, 0, 0 );

    LabelHeader = new QLabel( this );
    LabelHeader->setObjectName( "LabelHeader" );

    Console = new QTextEdit( this );
    Console->setObjectName( "Console" );
    Console->setReadOnly( true );
    Console->setProperty( "HcConsole", "true" );

    LabelBottom = new QLabel( this );
    LabelBottom->setObjectName( "LabelBottom" );

    LabelInput = new QLabel( this );
    LabelInput->setObjectName( "LabelInput" );

    Input = new QLineEdit( this );
    Input->setObjectName( "Input" );
    Input->addAction( tr( "Clear console screen" ), QKeySequence( Qt::CTRL | Qt::Key::Key_L ), this, [&](){ Console->clear(); } );

    connect( Input, &QLineEdit::returnPressed, this, &HcConsole::inputEnter );

    gridLayout->addWidget( LabelHeader, 0, 0, 1, 2 );
    gridLayout->addWidget( Console,     1, 0, 1, 2 );
    gridLayout->addWidget( LabelBottom, 2, 0, 1, 2 );
    gridLayout->addWidget( LabelInput,  3, 0, 1, 1 );
    gridLayout->addWidget( Input,       3, 1, 1, 1 );

    QMetaObject::connectSlotsByName( this );
}

auto HcConsole::setHeaderLabel(
    const QString& text
) -> void {
    LabelHeader->setText( text );
}

auto HcConsole::setBottomLabel(
    const QString& text
) -> void {
    LabelBottom->setText( text );
}

auto HcConsole::setInputLabel(
    const QString& text
) -> void {
    LabelInput->setText( text );
}

auto HcConsole::appendConsole(
    const QString& text
) -> void {
    Console->append( text );
}

auto HcConsole::inputEnter() -> void {
    auto expt  = std::string();
    auto input = Input->text().toStdString();
    auto text  = py11::str( input );

    if ( input.empty() ) {
        return;
    }

    Input->setText( "" );

    appendConsole( ( ">>> " + input ).c_str() );

    try {
        pybind11::exec( text );
    } catch ( py11::error_already_set &eas ) {
        expt = std::string( eas.what() );
    }

    if ( ! expt.empty() ) {
        appendConsole( expt.c_str() );
    }
}

auto HcConsole::formatString(
    const std::string& format,
    const std::string& output
) -> std::string {
    auto text = std::string();
    auto end  = format.end();

    for ( auto it = format.begin(); it != end; ++it ) {
        if ( *it == '%' ) {
            const auto prev = *it;
            const auto spec = ++it;

            switch ( * spec ) {
                //
                // upper case characters are certain data inserted
                //

                case 'T': {
                    text += QTime::currentTime().toString( "hh:mm:ss" ).toStdString();
                    break;
                }

                case 'D': {
                    text += QDateTime::currentDateTime().toString( "dd-MMM-yy" ).toStdString();
                    break;
                }

                case 'X': {
                    text += QDateTime::currentDateTime().toString( "dd-MMM-yy hh:mm:ss t" ).toStdString();
                    break;
                }

                case '$': {
                    text += HcTheme::getHtmlEnd().toStdString();
                    break;
                }

                case 'V': {
                    text += output;
                    break;

                case '%':
                    text += '%';
                    break;


                //
                // lower case characters are colours
                //

                case 'b': // background
                    text += HcTheme::getHtmlColorStart( Havoc->Theme.getBackground() ).toStdString();
                    break;

                case 'f': // foreground
                    text += HcTheme::getHtmlColorStart( Havoc->Theme.getForeground() ).toStdString();
                    break;

                case 'x': // comment
                    text += HcTheme::getHtmlColorStart( Havoc->Theme.getComment() ).toStdString();
                    break;

                case 'c': // cyan
                    text += HcTheme::getHtmlColorStart( Havoc->Theme.getCyan() ).toStdString();
                    break;

                case 'g': // green
                    text += HcTheme::getHtmlColorStart( Havoc->Theme.getGreen() ).toStdString();
                    break;

                case 'o': // orange
                    text += HcTheme::getHtmlColorStart( Havoc->Theme.getOrange() ).toStdString();
                    break;

                case 'p': // pink
                    text += HcTheme::getHtmlColorStart( Havoc->Theme.getPink() ).toStdString();
                    break;

                case 'l': // purple
                    text += HcTheme::getHtmlColorStart( Havoc->Theme.getPurple() ).toStdString();
                    break;

                case 'r': // red
                    text += HcTheme::getHtmlColorStart( Havoc->Theme.getRed() ).toStdString();
                    break;

                case 'y': // yellow
                    text += HcTheme::getHtmlColorStart( Havoc->Theme.getYellow() ).toStdString();
                    break;

                default:
                    text += prev;
                    text += *spec;
                    break;
                }
            }
        } else {
            text += *it;
        }
    }

    return text;
}
