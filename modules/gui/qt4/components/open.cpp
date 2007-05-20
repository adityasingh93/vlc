/*****************************************************************************
 * open.cpp : Panels for the open dialogs
 ****************************************************************************
 * Copyright (C) 2006-2007 the VideoLAN team
 * $Id$
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/


#include "qt4.hpp"
#include "components/open.hpp"
#include "dialogs/open.hpp"
#include "dialogs_provider.hpp"
#include "util/customwidgets.hpp"

#include <QFileDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QStackedLayout>

/**************************************************************************
 * File open
 **************************************************************************/
FileOpenPanel::FileOpenPanel( QWidget *_parent, intf_thread_t *_p_intf ) :
                                OpenPanel( _parent, _p_intf )
{
    /* Classic UI Setup */
    ui.setupUi( this );

    /* Use a QFileDialog and customize it because we don't want to
       rewrite it all. Be careful to your eyes cause there are a few hacks.
       Be very careful and test correctly when you modify this. */

    /* Set Filters for file selection */
    QString fileTypes = "";
    ADD_FILTER_MEDIA( fileTypes );
    ADD_FILTER_VIDEO( fileTypes );
    ADD_FILTER_AUDIO( fileTypes );
    ADD_FILTER_PLAYLIST( fileTypes );
    ADD_FILTER_ALL( fileTypes );
    fileTypes.replace( QString(";*"), QString(" *"));

    // Make this QFileDialog a child of tempWidget from the ui.
    dialogBox = new FileOpenBox( ui.tempWidget, NULL,
            qfu( p_intf->p_libvlc->psz_homedir ), fileTypes );
/*    dialogBox->setFileMode( QFileDialog::ExistingFiles );*/
    dialogBox->setAcceptMode( QFileDialog::AcceptOpen );

    /* retrieve last known path used in file browsing */
    char *psz_filepath = config_GetPsz( p_intf, "qt-filedialog-path" );
    if( psz_filepath )
    {
        dialogBox->setDirectory( QString::fromUtf8( psz_filepath ) );
        delete psz_filepath;
    }

    /* We don't want to see a grip in the middle of the window, do we? */
    dialogBox->setSizeGripEnabled( false );
    dialogBox->setToolTip( qtr( "Select one or multiple files, or a folder" ));

    // Add it to the layout
    ui.gridLayout->addWidget( dialogBox, 0, 0, 1, 3 );

    // But hide the two OK/Cancel buttons. Enable them for debug.
    QDialogButtonBox *fileDialogAcceptBox =
                        findChildren<QDialogButtonBox*>()[0];
    fileDialogAcceptBox->hide();

    /* Ugly hacks to get the good Widget */
    //This lineEdit is the normal line in the fileDialog.
    lineFileEdit = findChildren<QLineEdit*>()[3];
    lineFileEdit->hide();

    /* Make a list of QLabel inside the QFileDialog to access the good ones */
    QList<QLabel *> listLabel = findChildren<QLabel*>();

    /* Hide the FileNames one. Enable it for debug */
    listLabel[4]->hide();
    /* Change the text that was uncool in the usual box */
    listLabel[5]->setText( qtr( "Filter:" ) );

    // Hide the subtitles control by default.
    ui.subFrame->hide();

    /* Build the subs size combo box */
    module_config_t *p_item =
        config_FindConfig( VLC_OBJECT(p_intf), "freetype-rel-fontsize" );
    if( p_item )
    {
        for( int i_index = 0; i_index < p_item->i_list; i_index++ )
        {
            ui.sizeSubComboBox->addItem(
                qfu( p_item->ppsz_list_text[i_index] ),
                QVariant( p_item->pi_list[i_index] ) );
            if( p_item->value.i == p_item->pi_list[i_index] )
            {
                ui.sizeSubComboBox->setCurrentIndex( i_index );
            }
        }
    }

    /* Build the subs align combo box */
    p_item = config_FindConfig( VLC_OBJECT(p_intf), "subsdec-align" );
    if( p_item )
    {
        for( int i_index = 0; i_index < p_item->i_list; i_index++ )
        {
            ui.alignSubComboBox->addItem(
                qfu( p_item->ppsz_list_text[i_index] ),
                QVariant( p_item->pi_list[i_index] ) );
            if( p_item->value.i == p_item->pi_list[i_index] )
            {
                ui.alignSubComboBox->setCurrentIndex( i_index );
            }
        }
    }

    /* Connects  */
    BUTTONACT( ui.subBrowseButton, browseFileSub() );
    BUTTONACT( ui.subCheckBox, toggleSubtitleFrame());

    CONNECT( ui.fileInput, editTextChanged( QString ), this, updateMRL());
    CONNECT( ui.subInput, editTextChanged( QString ), this, updateMRL());
    CONNECT( ui.alignSubComboBox, currentIndexChanged( int ), this, updateMRL());
    CONNECT( ui.sizeSubComboBox, currentIndexChanged( int ), this, updateMRL());
    CONNECT( lineFileEdit, textChanged( QString ), this, browseFile());
}

FileOpenPanel::~FileOpenPanel()
{}

QStringList FileOpenPanel::browse( QString help )
{
    return THEDP->showSimpleOpen( help );
}

/* Unused. FIXME ? */
void FileOpenPanel::browseFile()
{
    QString fileString = "";
    foreach( QString file, dialogBox->selectedFiles() ) {
         fileString += "\"" + file + "\" ";
    }
    ui.fileInput->setEditText( fileString );
    updateMRL();
}

void FileOpenPanel::browseFileSub()
{
    // FIXME Handle selection of more than one subtitles file
    QStringList files = THEDP->showSimpleOpen( qtr("Open subtitles file"),
                            EXT_FILTER_SUBTITLE,
                            dialogBox->directory().absolutePath() );
    if( files.isEmpty() ) return;
    ui.subInput->setEditText( files.join(" ") );
    updateMRL();
}

void FileOpenPanel::updateMRL()
{
    QString mrl = ui.fileInput->currentText();

    if( ui.subCheckBox->isChecked() ) {
        mrl.append( " :sub-file=" + ui.subInput->currentText() );
        int align = ui.alignSubComboBox->itemData(
                    ui.alignSubComboBox->currentIndex() ).toInt();
        mrl.append( " :subsdec-align=" + QString().setNum( align ) );
        int size = ui.sizeSubComboBox->itemData(
                   ui.sizeSubComboBox->currentIndex() ).toInt();
        mrl.append( " :freetype-rel-fontsize=" + QString().setNum( size ) );
    }

    const char *psz_filepath = config_GetPsz( p_intf, "qt-filedialog-path" );
    if( ( NULL == psz_filepath )
      || strcmp( psz_filepath,dialogBox->directory().absolutePath().toUtf8()) )
    {
        /* set dialog box current directory as last known path */
        config_PutPsz( p_intf, "qt-filedialog-path",
                       dialogBox->directory().absolutePath().toUtf8() );
    }
    delete psz_filepath;

    emit mrlUpdated( mrl );
    emit methodChanged( "file-caching" );
}


/* Function called by Open Dialog when clicke on Play/Enqueue */
void FileOpenPanel::accept()
{
    ui.fileInput->addItem( ui.fileInput->currentText());
    if ( ui.fileInput->count() > 8 ) ui.fileInput->removeItem( 0 );
}

void FileOpenBox::accept()
{
    OpenDialog::getInstance( NULL, NULL )->play();
}

/* Function called by Open Dialog when clicked on cancel */
void FileOpenPanel::clear()
{
    ui.fileInput->setEditText( "" );
    ui.subInput->setEditText( "" );
}

void FileOpenPanel::toggleSubtitleFrame()
{
    if ( ui.subFrame->isVisible() )
    {
        ui.subFrame->hide();
        updateGeometry();
    /* FiXME Size */
    }
    else
    {
        ui.subFrame->show();
    }

    /* Update the MRL */
    updateMRL();
}

/**************************************************************************
 * Disk open
 **************************************************************************/
DiscOpenPanel::DiscOpenPanel( QWidget *_parent, intf_thread_t *_p_intf ) :
                                OpenPanel( _parent, _p_intf )
{
    ui.setupUi( this );

    BUTTONACT( ui.dvdRadioButton, updateButtons());
    BUTTONACT( ui.vcdRadioButton, updateButtons());
    BUTTONACT( ui.audioCDRadioButton, updateButtons());
    BUTTONACT( ui.dvdsimple,  updateButtons());

    CONNECT( ui.deviceCombo, editTextChanged( QString ), this, updateMRL());
    CONNECT( ui.titleSpin, valueChanged( int ), this, updateMRL());
    CONNECT( ui.chapterSpin, valueChanged( int ), this, updateMRL());
    CONNECT( ui.audioSpin, valueChanged( int ), this, updateMRL());
    CONNECT( ui.subtitlesSpin, valueChanged( int ), this, updateMRL());
}

DiscOpenPanel::~DiscOpenPanel()
{}

void DiscOpenPanel::clear()
{
    ui.titleSpin->setValue( 0 );
    ui.chapterSpin->setValue( 0 );
}

void DiscOpenPanel::updateButtons()
{
    if ( ui.dvdRadioButton->isChecked() )
    {
        ui.titleLabel->setText( qtr("Title") );
        ui.chapterLabel->show();
        ui.chapterSpin->show();
        ui.diskOptionBox_2->show();
    }
    else if ( ui.vcdRadioButton->isChecked() )
    {
        ui.titleLabel->setText( qtr("Entry") );
        ui.chapterLabel->hide();
        ui.chapterSpin->hide();
        ui.diskOptionBox_2->show();
    }
    else
    {
        ui.titleLabel->setText( qtr("Track") );
        ui.chapterLabel->hide();
        ui.chapterSpin->hide();
        ui.diskOptionBox_2->hide();
    }

    updateMRL();
}


void DiscOpenPanel::updateMRL()
{
    QString mrl = "";
    /* DVD */
    if( ui.dvdRadioButton->isChecked() ) {
        if( !ui.dvdsimple->isChecked() )
            mrl = "dvd://";
        else
            mrl = "dvdsimple://";
        mrl += ui.deviceCombo->currentText();
        emit methodChanged( "dvdnav-caching" );

        if ( ui.titleSpin->value() > 0 ) {
            mrl += QString("@%1").arg( ui.titleSpin->value() );
            if ( ui.chapterSpin->value() > 0 ) {
                mrl+= QString(":%1").arg( ui.chapterSpin->value() );
            }
        }

    /* VCD */
    } else if ( ui.vcdRadioButton->isChecked() ) {
        mrl = "vcd://" + ui.deviceCombo->currentText();
        emit methodChanged( "vcd-caching" );

        if( ui.titleSpin->value() > 0 ) {
            mrl += QString("@E%1").arg( ui.titleSpin->value() );
        }

    /* CDDA */
    } else {
        mrl = "cdda://" + ui.deviceCombo->currentText();
        if( ui.titleSpin->value() > 0 ) {
            QString("@%1").arg( ui.titleSpin->value() );
        }
    }

    if ( ui.dvdRadioButton->isChecked() || ui.vcdRadioButton->isChecked() )
    {
        if ( ui.audioSpin->value() >= 0 ) {
            mrl += " :audio-track=" +
                QString("%1").arg( ui.audioSpin->value() );
        }
        if ( ui.subtitlesSpin->value() >= 0 ) {
            mrl += " :sub-track=" +
                QString("%1").arg( ui.subtitlesSpin->value() );
        }
    }

    emit mrlUpdated( mrl );
}



/**************************************************************************
 * Net open
 **************************************************************************/
NetOpenPanel::NetOpenPanel( QWidget *_parent, intf_thread_t *_p_intf ) :
                                OpenPanel( _parent, _p_intf )
{
    ui.setupUi( this );

    CONNECT( ui.protocolCombo, currentIndexChanged( int ),
             this, updateProtocol( int ) );
    CONNECT( ui.portSpin, valueChanged( int ), this, updateMRL() );
    CONNECT( ui.addressText, textChanged( QString ), this, updateAddress());
    CONNECT( ui.timeShift, clicked(), this, updateMRL());
    CONNECT( ui.ipv6, clicked(), this, updateMRL());

    ui.protocolCombo->addItem("HTTP", QVariant("http"));
    ui.protocolCombo->addItem("HTTPS", QVariant("https"));
    ui.protocolCombo->addItem("FTP", QVariant("ftp"));
    ui.protocolCombo->addItem("MMS", QVariant("mms"));
    ui.protocolCombo->addItem("RTSP", QVariant("rtsp"));
    ui.protocolCombo->addItem("UDP/RTP (unicast)", QVariant("udp"));
    ui.protocolCombo->addItem("UDP/RTP (multicast)", QVariant("udp"));
}

NetOpenPanel::~NetOpenPanel()
{}

void NetOpenPanel::clear()
{}

void NetOpenPanel::updateProtocol( int idx ) {
    QString addr = ui.addressText->text();
    QString proto = ui.protocolCombo->itemData( idx ).toString();

    ui.timeShift->setEnabled( idx >= 4 );
    ui.ipv6->setEnabled( idx == 4 );
    ui.addressText->setEnabled( idx != 4 );
    ui.portSpin->setEnabled( idx >= 4 );

    /* If we already have a protocol in the address, replace it */
    if( addr.contains( "://")) {
        msg_Err( p_intf, "replace");
        addr.replace( QRegExp("^.*://"), proto + "://");
        ui.addressText->setText( addr );
    }

    updateMRL();
}

void NetOpenPanel::updateAddress() {
    updateMRL();
}

void NetOpenPanel::updateMRL() {
    QString mrl = "";
    QString addr = ui.addressText->text();
    int proto = ui.protocolCombo->currentIndex();

    if( addr.contains( "://") && proto != 4 ) {
        mrl = addr;
    } else {
        switch( proto ) {
        case 0:
        case 1:
            mrl = "http://" + addr;
            emit methodChanged("http-caching");
            break;
        case 3:
            mrl = "mms://" + addr;
            emit methodChanged("mms-caching");
            break;
        case 2:
            mrl = "ftp://" + addr;
            emit methodChanged("ftp-caching");
            break;
        case 4: /* RTSP */
            mrl = "rtsp://" + addr;
            emit methodChanged("rtsp-caching");
            break;
        case 5:
            mrl = "udp://@";
            if( ui.ipv6->isEnabled() && ui.ipv6->isChecked() ) {
                mrl += "[::]";
            }
            mrl += QString(":%1").arg( ui.portSpin->value() );
            emit methodChanged("udp-caching");
            break;
        case 6: /* UDP multicast */
            mrl = "udp://@";
            /* Add [] to IPv6 */
            if ( addr.contains(':') && !addr.contains('[') ) {
                mrl += "[" + addr + "]";
            } else mrl += addr;
            mrl += QString(":%1").arg( ui.portSpin->value() );
            emit methodChanged("udp-caching");
        }
    }
    if( ui.timeShift->isEnabled() && ui.timeShift->isChecked() ) {
        mrl += " :access-filter=timeshift";
    }
    emit mrlUpdated( mrl );
}

/**************************************************************************
 * Capture open
 **************************************************************************/
CaptureOpenPanel::CaptureOpenPanel( QWidget *_parent, intf_thread_t *_p_intf ) :
                                OpenPanel( _parent, _p_intf )
{
    ui.setupUi( this );

    QStackedLayout *stackedDevLayout = new QStackedLayout;
    ui.cardBox->setLayout( stackedDevLayout );

    QStackedLayout *stackedPropLayout = new QStackedLayout;
    ui.optionsBox->setLayout( stackedPropLayout );

#define addModuleAndLayouts( name, label )                            \
    QWidget * name ## DevPage = new QWidget( this );                  \
    QWidget * name ## PropPage = new QWidget( this );                 \
    stackedDevLayout->addWidget( name ## DevPage );                   \
    stackedPropLayout->addWidget( name ## PropPage );                  \
    QGridLayout * name ## DevLayout = new QGridLayout;                \
    QGridLayout * name ## PropLayout = new QGridLayout;               \
    name ## DevPage->setLayout( name ## DevLayout );                  \
    name ## PropPage->setLayout( name ## PropLayout );                \
    ui.deviceCombo->addItem( qtr( label ) );

#define CuMRL( widget, slot ) CONNECT( widget , slot , this, updateMRL() );

    /*******
     * V4L *
     *******/
    /* V4l Main */
    addModuleAndLayouts( v4l, "Video for Linux" );
    QLabel *v4lVideoDeviceLabel = new QLabel( qtr( "Video device name" ) );
    v4lDevLayout->addWidget( v4lVideoDeviceLabel, 0, 0 );
    QLineEdit *v4lVideoDevice = new QLineEdit;
    v4lDevLayout->addWidget( v4lVideoDevice, 0, 1 );
    QLabel *v4lAudioDeviceLabel = new QLabel( qtr( "Audio device name" ) );
    v4lDevLayout->addWidget( v4lAudioDeviceLabel, 1, 0 );
    QLineEdit *v4lAudioDevice = new QLineEdit;
    v4lDevLayout->addWidget( v4lAudioDevice, 1, 1 );

    /* V4l Props */
    QComboBox *v4lNormBox = new QComboBox;
    v4lNormBox->insertItem( 3, qtr( "Automatic" ) );
    v4lNormBox->insertItem( 0, "SECAM" );
    v4lNormBox->insertItem( 1, "NTSC" );
    v4lNormBox->insertItem( 2, "PAL" );

    QSpinBox *v4lFreq = new QSpinBox;
    v4lFreq->setAlignment( Qt::AlignRight );
    v4lFreq->setSuffix(" kHz");

    QLabel *v4lNormLabel = new QLabel( qtr( "Norm" ) );
    QLabel *v4lFreqLabel = new QLabel( qtr( "Frequency" ) );

    v4lPropLayout->addWidget( v4lNormLabel, 0 , 0 );
    v4lPropLayout->addWidget( v4lNormBox, 0 , 1 );

    v4lPropLayout->addWidget( v4lFreqLabel, 1 , 0 );
    v4lPropLayout->addWidget( v4lFreq, 1 , 1 );

    /* v4l CONNECTs */
    CuMRL( v4lVideoDevice, textChanged( QString ) );
    CuMRL( v4lAudioDevice, textChanged( QString ) );
    CuMRL( v4lFreq, valueChanged ( int ) );
    CuMRL( v4lNormBox,  currentIndexChanged ( int ) );

    /************
     * PVR      *
     ************/
    addModuleAndLayouts( pvr, "PVR" );

    /* PVR Main */
    QLabel *pvrVideoDeviceLabel = new QLabel( qtr( "Device name" ) );
    pvrDevLayout->addWidget( pvrVideoDeviceLabel, 0, 0 );
    QLineEdit *pvrVideoDevice = new QLineEdit;
    pvrDevLayout->addWidget( pvrVideoDevice, 0, 1 );
    QLabel *pvrAudioDeviceLabel = new QLabel( qtr( "Radio device name" ) );
    pvrDevLayout->addWidget( pvrAudioDeviceLabel, 1, 0 );
    QLineEdit *pvrAudioDevice = new QLineEdit;
    pvrDevLayout->addWidget( pvrAudioDevice, 1, 1 );

    /* PVR props */
    QComboBox *pvrNormBox = new QComboBox;
    pvrNormBox->insertItem( 3, qtr( "Automatic" ) );
    pvrNormBox->insertItem( 0, "SECAM" );
    pvrNormBox->insertItem( 1, "NTSC" );
    pvrNormBox->insertItem( 2, "PAL" );

    QSpinBox *pvrFreq = new QSpinBox;
    pvrFreq->setAlignment( Qt::AlignRight );
    pvrFreq->setSuffix(" kHz");
    QLabel *pvrNormLabel = new QLabel( qtr( "Norm" ) );
    QLabel *pvrFreqLabel = new QLabel( qtr( "Frequency" ) );

    pvrPropLayout->addWidget( pvrNormLabel, 0, 0 );
    pvrPropLayout->addWidget( pvrNormBox, 0, 1 );

    pvrPropLayout->addWidget( pvrFreqLabel, 1, 0 );
    pvrPropLayout->addWidget( pvrFreq, 1, 1 );

    /* PVR CONNECTs */
    CuMRL( pvrVideoDevice, textChanged( QString ) );
    CuMRL( pvrAudioDevice, textChanged( QString ) );

    CuMRL( pvrFreq, valueChanged ( int ) );
    CuMRL( pvrNormBox,  currentIndexChanged ( int ) );

    /*********************
     * DirectShow Stuffs *
     *********************/
    addModuleAndLayouts( dshow, "DirectShow" );


    /**************
     * BDA Stuffs *
     **************/
    addModuleAndLayouts( bda, "DVB / BDA" );

    /**************
     * DVB Stuffs *
     **************/
    addModuleAndLayouts( dvb, "DVB" );

    /* DVB Main */
    QLabel *dvbDeviceLabel = new QLabel( qtr( "Adapter card to tune" ) );

    QSpinBox *dvbCard = new QSpinBox;
    dvbCard->setAlignment( Qt::AlignRight );
    dvbCard->setPrefix( "/dev/dvb/adapter" );

    dvbDevLayout->addWidget( dvbDeviceLabel, 0, 0 );
    dvbDevLayout->addWidget( dvbCard, 0, 2 );

    dvbs = new QRadioButton( "DVB-S" );
    dvbs->setChecked( true );
    dvbc = new QRadioButton( "DVB-C" );
    dvbt = new QRadioButton( "DVB-T" );

    dvbDevLayout->addWidget( dvbs, 1, 0 );
    dvbDevLayout->addWidget( dvbc, 1, 1 );
    dvbDevLayout->addWidget( dvbt, 1, 2 );

    /* DVB Props */
    QLabel *dvbFreqLabel =
                    new QLabel( qtr( "Transponder/multiplex frequency" ) );
    dvbFreq = new QSpinBox;
    dvbFreq->setAlignment( Qt::AlignRight );
    //FIXME DVB-C/T uses Hz
    dvbFreq->setSuffix(" kHz");
    dvbPropLayout->addWidget( dvbFreqLabel, 0, 0 );
    dvbPropLayout->addWidget( dvbFreq, 0, 1 );

    QLabel *dvbSrateLabel = new QLabel( qtr( "Transponder symbol rate" ) );
    QSpinBox *dvbSrate = new QSpinBox;
    dvbSrate->setAlignment( Qt::AlignRight );
    dvbSrate->setSuffix(" kHz");
    dvbPropLayout->addWidget( dvbSrateLabel, 1, 0 );
    dvbPropLayout->addWidget( dvbSrate, 1, 1 );

    /* DVB CONNECTs */
    CuMRL( dvbCard, valueChanged ( int ) );
    CuMRL( dvbFreq, valueChanged ( int ) );
    CuMRL( dvbSrate, valueChanged ( int ) );
    BUTTONACT( dvbs, updateButtons() );
    BUTTONACT( dvbt, updateButtons() );
    BUTTONACT( dvbc, updateButtons() );

    /* General connects */
    connect( ui.deviceCombo, SIGNAL( activated( int ) ),
                     stackedDevLayout, SLOT( setCurrentIndex( int ) ) );
    connect( ui.deviceCombo, SIGNAL( activated( int ) ),
                     stackedPropLayout, SLOT( setCurrentIndex( int ) ) );
    CONNECT( ui.deviceCombo, activated( int ), this, updateMRL() );

#undef addModule
}

CaptureOpenPanel::~CaptureOpenPanel()
{}

void CaptureOpenPanel::clear()
{}

void CaptureOpenPanel::updateMRL()
{
    QString mrl = "";
    emit mrlUpdated( mrl );
}

void CaptureOpenPanel::updateButtons()
{
    if( dvbs->isChecked() ) dvbFreq->setSuffix(" kHz");
    if( dvbc->isChecked() || dvbt->isChecked() ) dvbFreq->setSuffix(" Hz");
}
