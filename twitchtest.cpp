#include "twitchtest.h"
#include "pingtest.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>

#include <QTextEdit>
#include <QJsonDocument>
#include <QVariant>
#include <QSettings>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "librtmp/rtmp_sys.h"
#include "librtmp/rtmp.h"
#include "librtmp/log.h"

#define SAVC(x) static const AVal av_##x = AVC(#x)
#define STR2AVAL(av,str) av.av_val = str; av.av_len = (int)strlen(av.av_val)

SAVC(onMetaData);
SAVC(duration);
SAVC(width);
SAVC(height);
SAVC(videocodecid);
SAVC(videodatarate);
SAVC(framerate);
SAVC(audiocodecid);
SAVC(audiodatarate);
SAVC(audiosamplerate);
SAVC(audiosamplesize);
SAVC(audiochannels);
SAVC(stereo);
SAVC(encoder);
SAVC(fileSize);

SAVC(onStatus);
SAVC(status);
SAVC(details);
SAVC(clientid);

SAVC(avc1);
SAVC(mp4a);

static const AVal av_OBSVersion = AVC("TwitchTest/1.4-qt");
static const AVal av_setDataFrame = AVC("@setDataFrame");

TwitchTest::TwitchTest() :
    QWidget()
{
    setWindowTitle(tr("Twitch Bandwidth Test v1.4-qt"));
    label_instructions = new QLabel(tr("Close any other applications that may be using your Internet connection. Testing can take a long time. Avoid using your connection during the test."));
    label_streamkey = new QLabel(tr("Stream Key:"));
    label_getkey = new QLabel(tr("<a href=\"https://www.twitch.tv/broadcast/dashboard/streamkey\">Get Key</a>"));
    label_tcpwindowsize = new QLabel(tr("TCP Window Size:"));
    label_testduration = new QLabel(tr("Test Duration:"));
    label_serverselection = new QLabel(tr("Servers to test:"));
    combo_tcpwindowsize = new QComboBox();
    combo_testduration = new QComboBox();
    edit_streamkey = new QLineEdit();
    button_start = new QPushButton(tr("Start"));
    button_export = new QPushButton(tr("Export Results"));
    button_exit = new QPushButton(tr("Exit"));
    table_servers = new QTableWidget();

    layout_combos = new QVBoxLayout();
    layout_keylabels = new QHBoxLayout();
    layout_key = new QVBoxLayout();
    layout_options = new QHBoxLayout();
    layout_buttons = new QHBoxLayout();
    layout_main = new QVBoxLayout();
    setLayout(layout_main);

    label_instructions->setWordWrap(true);
    /*
    label_getkey->setCursor(Qt::PointingHandCursor);
    label_getkey->setStyleSheet("color: 'blue';");
    */
    edit_streamkey->setEchoMode(QLineEdit::Password);
    table_servers->setColumnCount(5);
    QStringList labels;
    labels << QString("?") << tr("Server") << tr("Bandwidth") << tr("RTT") << tr("Quality");
    table_servers->setHorizontalHeaderLabels(labels);
    combo_tcpwindowsize->addItem(tr("Automatic (OBS)"), -1);
    combo_tcpwindowsize->addItem(tr("System Default"), 0);
    combo_tcpwindowsize->addItem(tr("8k"), 8192);
    combo_tcpwindowsize->addItem(tr("16k"), 16384);
    combo_tcpwindowsize->addItem(tr("32k"), 32768);
    combo_tcpwindowsize->addItem(tr("64k"), 65536);
    combo_tcpwindowsize->addItem(tr("128k"), 131072);
    combo_tcpwindowsize->addItem(tr("256k"), 262144);
    combo_tcpwindowsize->addItem(tr("512k"), 524288);
    combo_tcpwindowsize->addItem(tr("1M"), 1048576);
    combo_tcpwindowsize->addItem(tr("2M"), 1048576*2);
    combo_tcpwindowsize->addItem(tr("4M"), 1048576*4);
    combo_tcpwindowsize->setCurrentIndex(1);

    combo_testduration->addItem(tr("Ping Only"), 0);
    combo_testduration->addItem(tr("Quick (10 secs)"), 10000);
    combo_testduration->addItem(tr("Medium (30 secs)"), 30000);
    combo_testduration->addItem(tr("Long (60 secs)"), 60000);
    combo_testduration->addItem(tr("2 Minutes"), 120000);
    combo_testduration->addItem(tr("3 Minutes"), 180000);
    combo_testduration->addItem(tr("4 Minutes"), 240000);
    combo_testduration->addItem(tr("5 Minutes"), 300000);
    combo_testduration->setCurrentIndex(1);

    layout_main->addWidget(label_instructions);
    layout_keylabels->addWidget(label_streamkey);
    layout_keylabels->addStretch(5);
    layout_keylabels->addWidget(label_getkey);
    layout_combos->addWidget(label_tcpwindowsize);
    layout_combos->addWidget(combo_tcpwindowsize);
    layout_combos->addWidget(label_testduration);
    layout_combos->addWidget(combo_testduration);
    layout_key->addLayout(layout_keylabels);
    layout_key->addWidget(edit_streamkey);
    layout_key->addStretch(5);
    layout_key->addWidget(label_serverselection);
    layout_options->addLayout(layout_key);
    layout_options->addLayout(layout_combos);
    layout_buttons->addWidget(button_start);
    layout_buttons->addWidget(button_export);
    layout_buttons->addStretch(10);
    layout_buttons->addWidget(button_exit);
    layout_main->addLayout(layout_options);
    layout_main->addWidget(table_servers, 10);
    layout_main->addLayout(layout_buttons);

    connect( label_getkey, &QLabel::linkActivated, this, &TwitchTest::geturlClicked );
    connect( button_start, &QPushButton::clicked, this, &TwitchTest::startClicked );
    connect( button_export, &QPushButton::clicked, this, &TwitchTest::exportClicked );
    connect( button_exit, &QPushButton::clicked, this, &TwitchTest::exitClicked );

    m_network = new QNetworkAccessManager(this);
    requestServers();

    //RTMP_LogSetLevel(RTMP_LOGALL);
    m_settings = new QSettings("Foxmoxie", "TwitchTest");

    loadSettings();

    resize(560,530);
}

TwitchTest::~TwitchTest()
{
    label_instructions->deleteLater();
    label_streamkey->deleteLater();
    label_getkey->deleteLater();
    label_tcpwindowsize->deleteLater();
    label_testduration->deleteLater();
    label_serverselection->deleteLater();
    combo_tcpwindowsize->deleteLater();
    combo_testduration->deleteLater();
    edit_streamkey->deleteLater();
    button_start->deleteLater();
    button_export->deleteLater();
    button_exit->deleteLater();
    table_servers->deleteLater();

    layout_combos->deleteLater();
    layout_keylabels->deleteLater();
    layout_options->deleteLater();
    layout_key->deleteLater();
    layout_buttons->deleteLater();
    layout_main->deleteLater();

    m_network->deleteLater();
    m_settings->deleteLater();
}

void TwitchTest::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    TwitchTest *tt = new TwitchTest();
    tt->show();
    return app.exec();
}

void TwitchTest::saveSettings()
{
    // Save key first:
    m_settings->setValue("streamkey", edit_streamkey->text());
    QStringList servers;
    for( int x=0; x < table_servers->rowCount(); x++ )
    {
        QCheckBox *cb = dynamic_cast<QCheckBox *>( table_servers->cellWidget(x, 0) );
        if( cb->isChecked() )
        {
            QString url = cb->property("url").toString();
            servers.append(url);
        }
    }
    m_settings->setValue("selected_servers", servers);
    m_settings->sync();
}

void TwitchTest::loadSettings()
{
    edit_streamkey->setText( m_settings->value("streamkey", "").toString() );
    m_selectedServers = m_settings->value("selected_servers", QStringList()).toStringList();
}

RTMP *TwitchTest::prepareRtmp(const QString &url, const QString &key)
{
    RTMP *rtmp = RTMP_Alloc();

    rtmp->m_inChunkSize = RTMP_DEFAULT_CHUNKSIZE;
    rtmp->m_outChunkSize = RTMP_DEFAULT_CHUNKSIZE;
    rtmp->m_bSendChunkSizeInfo = 1;
    rtmp->m_nBufferMS = 30000;
    rtmp->m_nClientBW = 2500000;
    rtmp->m_nClientBW2 = 2;
    rtmp->m_nServerBW = 2500000;
    rtmp->m_fAudioCodecs = 3191.0;
    rtmp->m_fVideoCodecs = 252.0;
    rtmp->Link.timeout = 30;
    rtmp->Link.swfAge = 30;

    rtmp->Link.flashVer.av_val = "FMLE/3.0 (compatible; FMSc/1.0)";
    rtmp->Link.flashVer.av_len = (int)strlen(rtmp->Link.flashVer.av_val);

    rtmp->m_outChunkSize = 4096;
    rtmp->m_bSendChunkSizeInfo = TRUE;

    QString nurl = url;
    QString nkey = key;
    nurl.replace("/{stream_key}", "");
    nkey.append("?bandwidthtest");
    nkey.remove(' ');

    QUrl qurl(nurl);
    QString path = qurl.path();
    if( qurl.hasQuery() )
    {
        path.append("?");
        path.append(qurl.query());
    }
    rtmp->Link.app.av_val = strdup( path.toStdString().c_str() );
    rtmp->Link.app.av_len = path.length();
    rtmp->Link.playpath0.av_val = strdup( nkey.toStdString().c_str() );
    rtmp->Link.playpath0.av_len = nkey.length();
    rtmp->Link.playpath = rtmp->Link.playpath0;
    rtmp->Link.hostname.av_val = strdup( qurl.host().toStdString().c_str() );
    rtmp->Link.hostname.av_len = qurl.host().length();
    rtmp->Link.port = qurl.port();
    QString scheme = qurl.scheme();
    if( scheme.compare("rtmp")==0 )
        rtmp->Link.protocol = RTMP_PROTOCOL_RTMP;
    else if( scheme.compare("rtmpt")==0 )
        rtmp->Link.protocol = RTMP_PROTOCOL_RTMPT;
    else if( scheme.compare("rtmps")==0 )
        rtmp->Link.protocol = RTMP_PROTOCOL_RTMPS;
    else if( scheme.compare("rtmpe")==0 )
        rtmp->Link.protocol = RTMP_PROTOCOL_RTMPE;
    else if( scheme.compare("rtmfp")==0 )
        rtmp->Link.protocol = RTMP_PROTOCOL_RTMFP;
    else if( scheme.compare("rtmpte")==0 )
        rtmp->Link.protocol = RTMP_PROTOCOL_RTMPTE;
    else if( scheme.compare("rtmpts")==0 )
        rtmp->Link.protocol = RTMP_PROTOCOL_RTMPTS;
    else
    {
        RTMP_Log(RTMP_LOGWARNING, "Unknown protocol!\n");
    }

    rtmp->Link.port = 1935;

    RTMP_EnableWrite(rtmp);
    rtmp->m_bUseNagle = TRUE;

    return rtmp;
}

uint64_t timeGetTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    uint64_t result = ( tv.tv_sec * 1000 );
    result += ( tv.tv_usec * 0.0001 );
    return result;
}

void Sleep(int secs)
{
    for( int x=0; x < secs * 100; x++ )
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;
        select(-1, NULL, NULL, NULL, &tv);
        QApplication::processEvents();
    }
}

bool TwitchTest::testRtmp(RTMP *rtmp, int index, int testDuration)
{
    if( !RTMP_Connect(rtmp, NULL) )
        return false;

    QLabel *lblBandwidth = dynamic_cast<QLabel *>( table_servers->cellWidget(index, 2) );
    QLabel *lblPing = dynamic_cast<QLabel *>( table_servers->cellWidget(index, 3) );
    QLabel *lblQuality = dynamic_cast<QLabel *>( table_servers->cellWidget(index, 4) );

    lblBandwidth->setText("Starting...");

    // Ping only test:
    if( 0 == testDuration )
    {
        RTMP_Close(rtmp);
        ::close(rtmp->m_sb.sb_socket);

        lblQuality->setText("Up");

        RTMP_Free(rtmp);
        return true;
    }

    // Do a quality test:
    RTMPPacket packet = { 0 };
    struct sockaddr_storage clientSockName;
    socklen_t nameLen = sizeof(struct sockaddr_storage);
    getsockname( rtmp->m_sb.sb_socket, (struct sockaddr *)&clientSockName, &nameLen );
/*
    int clientPort = ((struct sockaddr_in *)(&clientSockName))->sin_port;
    GetTcpRow(clientPort, htons(1935), MIB_TCP_STATE_ESTAB, (PMIB_TCPROW)&row);

    TCP_BOOLEAN_OPTIONAL operation = TcpBoolOptEnabled;
    PUCHAR rw = NULL;
    TCP_ESTATS_DATA_RW_v0 dataOn;
    TCP_ESTATS_PATH_RW_v0 pathOn;
    ULONG size = 0;

    dataOn.EnableCollection = TcpBoolOptEnabled;
    rw = (PUCHAR)&dataOn;
    size = sizeof(dataOn);
    SetPerTcpConnectionEStats((PMIB_TCPROW)&row, TcpConnectionEstatsData, rw, 0, size, 0);

    pathOn.EnableCollection = TcpBoolOptEnabled;
    rw = (PUCHAR)&pathOn;
    size = sizeof(pathOn);
    SetPerTcpConnectionEStats((PMIB_TCPROW)&row, TcpConnectionEstatsPath, rw, 0, size, 0);
*/
    if (!RTMP_ConnectStream(rtmp, 0))
    {
            RTMP_Free(rtmp);
            return false;
    }
/* TODO: This:
    if (tcpBufferSize > 0)
    {
            setsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&tcpBufferSize, sizeof(tcpBufferSize));
    }
    else if (tcpBufferSize == -1)
    {
            hIdealSend = WSACreateEvent();
            SetupSendEvent(hIdealSend, &sendBacklogOverlapped, rtmp);

            tcpBufferSize = 65536;
            setsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&tcpBufferSize, sizeof(tcpBufferSize));
    }
*/
    SendRTMPMetadata(rtmp);

    unsigned char junk[4096] = { 0xde };

    packet.m_nChannel = 0x05; // source channel
    packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet.m_body = (char *)junk + RTMP_MAX_HEADER_SIZE;
    packet.m_nBodySize = sizeof(junk) - RTMP_MAX_HEADER_SIZE;

    uint64_t realStart = timeGetTime();
    uint64_t startTime = 0, lastUpdateTime = 0;
    uint64_t bytesSent = 0, startBytes = 0;
    uint64_t sendTime = 0, sendCount = 0;
#define START_MEASUREMENT_WAIT 2500

    float speed = 0;
    int wasCapped = 0;
    int failed = 0;

    for (;;)
    {
        uint64_t nowTime = timeGetTime();

        if (!RTMP_SendPacket(rtmp, &packet, FALSE))
        {
            failed = 1;
            break;
        }

        uint64_t diff = timeGetTime() - nowTime;

        sendTime += diff;
        sendCount++;

        bytesSent += packet.m_nBodySize;

        if (nowTime - realStart > START_MEASUREMENT_WAIT && !startTime)
        {
            startTime = nowTime;
            startBytes = bytesSent;
            lastUpdateTime = nowTime;
        }

        if (bytesSent - startBytes > 0 && nowTime - startTime > 250)
        {
            speed = ((bytesSent - startBytes)) / ((nowTime - startTime) / 1000.0f);
            speed = speed * 8 / 1000;
            if (speed > 10000)
            {
                wasCapped = 1;
                Sleep(10);
            }
        }

        if (startTime && nowTime - lastUpdateTime > 500)
        {
            lastUpdateTime = nowTime;

            if (wasCapped)
            {
                qDebug() << "10,000+kbps";
                lblBandwidth->setText("10,000kbps+");
            }
            else
            {
                lblBandwidth->setText( QString("%1kbps").arg(speed));
            }

            table_servers->resizeColumnsToContents();
            if (nowTime - startTime > testDuration)
                break;

            wasCapped = 0;
        }

        QApplication::processEvents();
    }

    return true;
}

void TwitchTest::SendRTMPMetadata(RTMP *rtmp)
{
        char metadata[2048] = {0};
        char *enc = metadata + RTMP_MAX_HEADER_SIZE, *pend = metadata + sizeof(metadata) - RTMP_MAX_HEADER_SIZE;

        enc = AMF_EncodeString(enc, pend, &av_setDataFrame);
        enc = AMF_EncodeString(enc, pend, &av_onMetaData);

        *enc++ = AMF_OBJECT;

        enc = AMF_EncodeNamedNumber(enc, pend, &av_duration, 0.0);
        enc = AMF_EncodeNamedNumber(enc, pend, &av_fileSize, 0.0);
        enc = AMF_EncodeNamedNumber(enc, pend, &av_width, 16);
        enc = AMF_EncodeNamedNumber(enc, pend, &av_height, 16);
        enc = AMF_EncodeNamedString(enc, pend, &av_videocodecid, &av_avc1);//7.0);//
        enc = AMF_EncodeNamedNumber(enc, pend, &av_videodatarate, 10000);
        enc = AMF_EncodeNamedNumber(enc, pend, &av_framerate, 30);
        enc = AMF_EncodeNamedString(enc, pend, &av_audiocodecid, &av_mp4a);//audioCodecID);//
        enc = AMF_EncodeNamedNumber(enc, pend, &av_audiodatarate, 128); //ex. 128kb\s
        enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplerate, 44100);
        enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplesize, 16.0);
        enc = AMF_EncodeNamedNumber(enc, pend, &av_audiochannels, 2);
        enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo, 1);

        enc = AMF_EncodeNamedString(enc, pend, &av_encoder, &av_OBSVersion);
        *enc++ = 0;
        *enc++ = 0;
        *enc++ = AMF_OBJECT_END;

        RTMPPacket packet = { 0 };

        packet.m_nChannel = 0x03;        // control channel (invoke)
        packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
        packet.m_packetType = RTMP_PACKET_TYPE_INFO;
        packet.m_nTimeStamp = 0;
        packet.m_nInfoField2 = rtmp->m_stream_id;
        packet.m_hasAbsTimestamp = TRUE;
        packet.m_body = metadata + RTMP_MAX_HEADER_SIZE;
        packet.m_nBodySize = enc - metadata + RTMP_MAX_HEADER_SIZE;

        RTMP_SendPacket(rtmp, &packet, FALSE);
}

void TwitchTest::requestServers()
{
    QUrl url("https://api.twitch.tv/kraken/ingests");
    QNetworkRequest req(url);
    req.setRawHeader("Client-ID", "mme8bj93xsju8hte7jd9vctbf79lmec");
    m_serverReply = m_network->get(req);
    connect( m_serverReply, &QNetworkReply::readyRead, this, &TwitchTest::serverData );
    connect( m_serverReply, &QNetworkReply::finished, this, &TwitchTest::serverFinished );
}

void TwitchTest::serverData()
{
    m_serverData.append( m_serverReply->readAll() );
}

bool sortIngests(QVariant a, QVariant b)
{
    QVariantMap am = a.toMap();
    QVariantMap bm = b.toMap();

    return am["name"].toString().compare( bm["name"].toString() ) < 0 ? true : false;
}

void TwitchTest::serverFinished()
{
    QJsonDocument doc = QJsonDocument::fromJson(m_serverData);

    //printf("%s\n", doc.toJson().toStdString().c_str());
    QVariantMap var = doc.toVariant().toMap();
    QVariantList ingests = var["ingests"].toList();
    std::sort( ingests.begin(), ingests.end(), sortIngests );
    int row = 0;
    table_servers->setRowCount(ingests.length());
    foreach(QVariant ent, ingests)
    {
        QVariantMap ingest = ent.toMap();
        //qDebug() << ingest;

        QCheckBox *sel = new QCheckBox();
        if( m_selectedServers.contains( ingest["url_template"].toString() ) )
            sel->setChecked(true);
        sel->setProperty("url", ingest["url_template"]);
        QLabel *sname = new QLabel(ingest["name"].toString());
        QLabel *bwidth = new QLabel("");
        QLabel *rtt = new QLabel("");
        QLabel *qual = new QLabel("");

        table_servers->setCellWidget(row, 0, sel);
        table_servers->setCellWidget(row, 1, sname);
        table_servers->setCellWidget(row, 2, bwidth);
        table_servers->setCellWidget(row, 3, rtt);
        table_servers->setCellWidget(row, 4, qual);

        row++;
    }
    table_servers->resizeColumnsToContents();

    m_serverReply->deleteLater();
}

void TwitchTest::startClicked()
{
    m_aborted = false;
    QString key = edit_streamkey->text();
    for( int x=0; x < table_servers->rowCount(); x++ )
    {
        QCheckBox *cb = dynamic_cast<QCheckBox *>( table_servers->cellWidget(x, 0) );
        QLabel *pl = dynamic_cast<QLabel *>( table_servers->cellWidget(x, 3) );
        if( cb->isChecked() )
        {
            QString url = cb->property("url").toString();
            RTMP *rtmp = prepareRtmp(url, key);

            if( !rtmp )
                continue;

            char *host = strndup( rtmp->Link.hostname.av_val, rtmp->Link.hostname.av_len );
            double pingVal = ::ping(host);
            if( pingVal == -1 )
                pl->setText( "Error" );
            else
                pl->setText( QString::number(pingVal, 'g', 1) );
            free( (void*)host );

            int dur = combo_testduration->currentData().toInt();
            qDebug() << "User wants to test" << url << "for" << dur << "ms";

            testRtmp(rtmp, x, dur);
        }
    }
}

void TwitchTest::exportClicked()
{
    bool isEmpty = true;
    QString contents;
    contents.append("Server,Bandwidth,Ping,Quality\n");
    for( int x=0; x < table_servers->rowCount(); x++ )
    {
        QCheckBox *cb = dynamic_cast<QCheckBox *>( table_servers->cellWidget(x, 0) );
        if( cb->isChecked() )
        {
            isEmpty = false;

            QLabel *nm = dynamic_cast<QLabel *>( table_servers->cellWidget(x, 1) );
            QLabel *bw = dynamic_cast<QLabel *>( table_servers->cellWidget(x, 2) );
            QLabel *rtt = dynamic_cast<QLabel *>( table_servers->cellWidget(x, 3) );
            QLabel *qua = dynamic_cast<QLabel *>( table_servers->cellWidget(x, 4) );

            contents.append(nm->text());
            contents.append(",");
            contents.append(bw->text());
            contents.append(",");
            contents.append(rtt->text());
            contents.append(",");
            contents.append(qua->text());
            contents.append("\n");
        }
    }

    if( isEmpty )
        return;

    QTextEdit *te = new QTextEdit();
    te->insertPlainText(contents);
    te->show();
}

void TwitchTest::exitClicked()
{
    saveSettings();
    QApplication::exit(0);
}

void TwitchTest::geturlClicked(const QString &url)
{
    QDesktopServices::openUrl(QUrl(url));
}
