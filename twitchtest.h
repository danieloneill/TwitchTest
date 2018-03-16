#ifndef TWITCHTEST_H
#define TWITCHTEST_H

#include <QSpacerItem>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTableWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QNetworkReply>
#include <QUrl>

class RTMP;
class QSettings;
class TwitchTest : public QWidget
{
    Q_OBJECT

    QTimer m_testTimer;
    bool m_aborted;

    QStringList m_selectedServers;

public:
    explicit TwitchTest();
    ~TwitchTest();

    // For fetching ingest list:
    void requestServers();

protected:
    // For fetching ingest list:
    QNetworkAccessManager *m_network;
    QNetworkReply *m_serverReply;
    QByteArray m_serverData;

    QSettings *m_settings;

    RTMP *prepareRtmp(const QString &url, const QString &key);
    bool testRtmp(RTMP *rtmp, int index, int testDuration);
    void SendRTMPMetadata(RTMP *rtmp);

protected slots:
    // Ingest list data:
    void serverData();
    void serverFinished();

    // Button (and link) callbacks:
    void startClicked();
    void exportClicked();
    void exitClicked();
    void geturlClicked(const QString &url);

    void saveSettings();
    void loadSettings();

    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    QLabel *label_instructions;
    QLabel *label_streamkey;
    QLabel *label_getkey;
    QLabel *label_tcpwindowsize;
    QLabel *label_testduration;
    QLabel *label_serverselection;
    QComboBox *combo_tcpwindowsize;
    QComboBox *combo_testduration;
    QLineEdit *edit_streamkey;
    QPushButton *button_start;
    QPushButton *button_export;
    QPushButton *button_exit;
    QTableWidget *table_servers;

    QVBoxLayout *layout_combos;
    QHBoxLayout *layout_keylabels;
    QVBoxLayout *layout_key;
    QHBoxLayout *layout_options;
    QHBoxLayout *layout_buttons;
    QVBoxLayout *layout_main;
};

#endif // TWITCHTEST_H
