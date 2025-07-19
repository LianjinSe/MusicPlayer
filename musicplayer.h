#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QWidget>
#include <QStandardItemModel>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include <QRandomGenerator>
#include <QMenu>
#include <QImage>
#include <QGraphicsPixmapItem>
#include <QMediaMetaData>
#include <QMap>
#include <QFileDialog>
#include <QDirIterator>
#include <QMouseEvent>
#include <QPainter>
#include <QFile>
#include <QSlider>

QT_BEGIN_NAMESPACE
namespace Ui {
class MusicPlayer;
}
QT_END_NAMESPACE

class MusicPlayer : public QWidget
{
    Q_OBJECT

public:
    MusicPlayer(QWidget *parent = nullptr);
    ~MusicPlayer();
    enum LoopMode {
        LoopAll=0,      // 循环全部
        LoopSingle,     // 单曲循环
        LoopRandom,     // 随机播放
    };
    Q_ENUM(LoopMode)

private slots:
    // 按钮点击的槽函数部分
    void on_openDirBtn_clicked();     // 打开文件夹
    void on_prevSongBtn_clicked();    // 上一首
    void on_nextSongBtn_clicked();    // 下一首
    void on_playBtn_clicked();        // 播放/暂停
    void on_loopModeBtn_clicked();    // 循环模式
    void on_modeSwitchBtn_clicked();  // 切换深色(浅色)模式
    void on_musicListView_doubleClicked(const QModelIndex &index); //双击切歌

    // 进度条相关
    void updateDuration(qint64 duration);
    void updatePlayerPosition(qint64 position);
    void setPlayerPosition(int position);
    void onSliderPressed();
    void onSliderReleased();

    // 状态相关
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void handleMetaDataChanged();

    // 音量相关
    void onVolumeSliderMoved(int value);
    void handleVolumeContextMenu(const QPoint &pos);

private:
    static QString formatTime(qint64 ms);
    int getPlaylistCount() const;
    void playTrack(int index);
    int generateRandomIndex();
    void initVolumeControl();                       // 初始化音量控制函数
    void toggleMute();                              // 切换静音状态
    void updateVolumeIcon();                        // 更新音量图标
    void displayAlbumArt(const QPixmap& pixmap);    // 显示封面
    void setNewBackground(const QString &path);
    void loadLyrics(const QString& musicFilePath);  // 加载歌词文件
    QString findCurrentLyric(qint64 position);      // 根据时间位置找歌词
    void loadMusicList();

    Ui::MusicPlayer *ui;
    QStandardItemModel *m_listModel;
    QMediaPlayer *m_mediaPlayer{};
    QAudioOutput *m_audioOutput{};
    LoopMode m_loopMode = LoopAll;
    bool m_isSliderMoving = false;
    QString m_currentPositionTime = "00:00";  // 当前播放时间缓存
    QString m_totalDurationTime = "00:00";    // 总时长缓存
    int m_currentIndex = -1;                  // 当前播放曲目索引
    QSlider *m_volumeSlider;                  // 音量滑块
    int m_lastVolume = 50;                    // 静音前保存的音量值
    bool m_isMuted = false;                   // 当前是否处于静音状态
    static const QMediaMetaData MEDIA_METADATA_EMPTY; // 空元数据常量
    QGraphicsScene *m_albumScene;             // 封面场景对象
    QPixmap m_defaultAlbum= QPixmap(":/Resources/defaultalbum.png");                   // 默认封面图片
    bool lightmode=true;
    QString m_currentBgPath = ":/Resources/lightmodebackground.png";                   // 存储当前背景图片路径
    QPixmap m_currentBg;
    QMap<qint64,QString> m_currentLyrics;     // 当前歌词
    bool lyricAvailable = false;              // 是否有歌词
    QString m_currentMusicPath;
    QString lastList;
    int lastIndex;
    QString defaultMusicPath="./Music";
    QString defaultConfigPath="./config.txt";

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};
#endif // MUSICPLAYER_H
