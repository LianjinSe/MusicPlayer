#include "musicplayer.h"
#include "./ui_musicplayer.h"

const QMediaMetaData MusicPlayer::MEDIA_METADATA_EMPTY = QMediaMetaData();
MusicPlayer::MusicPlayer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MusicPlayer)
    , m_listModel(new QStandardItemModel)
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_albumScene(new QGraphicsScene(this))

{
    ui->setupUi(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    ui->musicListView->setModel(m_listModel);
    ui->albumView->setScene(m_albumScene);
    m_currentBg.load(m_currentBgPath);
    initVolumeControl();
    connect(ui->playSlider, &QSlider::sliderMoved,this, &MusicPlayer::setPlayerPosition);
    connect(ui->playSlider, &QSlider::sliderPressed,this, &MusicPlayer::onSliderPressed);
    connect(ui->playSlider, &QSlider::sliderReleased,this, &MusicPlayer::onSliderReleased);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged,this,&MusicPlayer::updateDuration);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged,this, &MusicPlayer::updatePlayerPosition);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MusicPlayer::handlePlaybackStateChanged);
    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MusicPlayer::handleMediaStatusChanged);
    connect(m_mediaPlayer, &QMediaPlayer::metaDataChanged,this, &MusicPlayer::handleMetaDataChanged);

    qApp->installEventFilter(this);  // 为整个应用安装事件过滤器
    ui->volBtn->installEventFilter(this);  // 为音量按钮安装事件过滤器

    loadMusicList();

}

MusicPlayer::~MusicPlayer()
{
    // 保存配置
    if (!defaultConfigPath.isEmpty())
    {
        QFile config(defaultConfigPath);
        if (!config.open(QIODevice::WriteOnly|QIODevice::Text))
        {
            return;
        }
        QTextStream out(&config);
        out<<m_currentMusicPath<<"\n"<<m_currentIndex<<"\n";
        config.close();
    }
    delete ui;
    delete m_volumeSlider;
}

void MusicPlayer::on_openDirBtn_clicked()
{
    m_listModel->clear();
    auto musicPath = QFileDialog::getExistingDirectory(this,"选择文件夹（播放列表）",defaultMusicPath);
    m_currentMusicPath=musicPath;
    if (musicPath.isEmpty()){
        return;
    }
    QDirIterator it_music(musicPath,{"*.mp3","*.wav","*.flac"});
    while (it_music.hasNext()){
        auto info = it_music.nextFileInfo();
        auto item = new QStandardItem(info.fileName());
        item->setData(it_music.fileInfo().canonicalFilePath());
        m_listModel->appendRow(item);
    }
    m_currentIndex = -1;
    ui->playBtn->setIcon(QIcon(":/Resources/play.svg")); // 恢复播放图标
}


void MusicPlayer::on_prevSongBtn_clicked()
{
    int totalSongs = getPlaylistCount();
    if (totalSongs <= 0) return;
    int newIndex = -1;

    switch (m_loopMode) {
    case LoopAll:
        newIndex = (m_currentIndex - 1 + totalSongs) % totalSongs;
        break;

    case LoopSingle:
        newIndex = m_currentIndex;
        break;

    case LoopRandom:
        newIndex = generateRandomIndex();
        break;
    }

    if (newIndex >= 0 && newIndex < totalSongs) {
        playTrack(newIndex);
    }
}

void MusicPlayer::on_nextSongBtn_clicked()
{
    int totalSongs = getPlaylistCount();
    if (totalSongs <= 0) return;

    int newIndex = -1;

    if (m_currentIndex < 0) {
        newIndex = (totalSongs > 0) ? 0 : -1;
    }
    else {
        switch (m_loopMode) {
        case LoopAll:
            newIndex = (m_currentIndex + 1) % totalSongs;
            break;

        case LoopSingle:
            newIndex = m_currentIndex;
            break;

        case LoopRandom:
            newIndex = generateRandomIndex();
            break;
        }
    }
    if (newIndex >= 0 && newIndex < totalSongs) {
        playTrack(newIndex);
    }
    else {
        m_mediaPlayer->pause();
    }
}

void MusicPlayer::on_playBtn_clicked() {
    if (getPlaylistCount() == 0) return;
    if (m_currentIndex < 0) {
        playTrack(0);
        return;
    }
    switch (m_mediaPlayer->playbackState()) {
    case QMediaPlayer::PlayingState:
        m_mediaPlayer->pause();
        break;

    case QMediaPlayer::PausedState:
    case QMediaPlayer::StoppedState:
        if (m_mediaPlayer->mediaStatus() == QMediaPlayer::NoMedia ||
            m_mediaPlayer->mediaStatus() == QMediaPlayer::InvalidMedia) {
            playTrack(m_currentIndex);
        } else {
            m_mediaPlayer->play();
        }
        break;
    }
}

void MusicPlayer::on_loopModeBtn_clicked()
{
    switch (this->m_loopMode) {
    case LoopAll:
        m_loopMode=LoopSingle;
        ui->loopModeBtn->setIcon(QIcon(":/Resources/loopone.svg"));
        ui->loopModeBtn->setToolTip("单曲循环");
        break;
    case LoopSingle:
        m_loopMode=LoopRandom;
        ui->loopModeBtn->setIcon(QIcon(":/Resources/looprandom.svg"));
        ui->loopModeBtn->setToolTip("随机播放");
        break;
    case LoopRandom:
        m_loopMode=LoopAll;
        ui->loopModeBtn->setIcon(QIcon(":/Resources/loopall.svg"));
        ui->loopModeBtn->setToolTip("顺序播放");
        break;
    default:
        break;
    }
}

void MusicPlayer::on_modeSwitchBtn_clicked()
{
    if (lightmode){
        setNewBackground(":/Resources/darkmodebackground.png");
        ui->modeSwitchBtn->setIcon(QIcon(":/Resources/darkmode.svg"));
        lightmode=false;
    }else{
        setNewBackground(":/Resources/lightmodebackground.png");
        ui->modeSwitchBtn->setIcon(QIcon(":/Resources/lightmode.svg"));
        lightmode=true;
    }
}

void MusicPlayer::on_musicListView_doubleClicked(const QModelIndex &index) {
    playTrack(index.row()); // 使用统一播放函数
}

void MusicPlayer::updateDuration(qint64 duration)
{
    ui->playSlider->setRange(0,static_cast<int>(duration));
    m_totalDurationTime = formatTime(duration);
    ui->durationLab->setText(m_totalDurationTime);
}

void MusicPlayer::updatePlayerPosition(qint64 position)
{
    if (!m_isSliderMoving) {
        QSignalBlocker blocker(ui->playSlider);  // 改进的阻止信号方式
        ui->playSlider->setValue(static_cast<int>(position));
        ui->playSlider->blockSignals(false);
    }
    m_currentPositionTime = formatTime(position);
    ui->playDuraLab->setText(m_currentPositionTime);
    if (lyricAvailable) {
        QString currentLyric = findCurrentLyric(position);
        ui->lyricLab->setText(currentLyric);
        ui->lyricLab->setToolTip(currentLyric);
    }
}

void MusicPlayer::setPlayerPosition(int position) {
    m_mediaPlayer->setPosition(static_cast<qint64>(position));
}

void MusicPlayer::onSliderPressed() {
    m_isSliderMoving = true;
}

void MusicPlayer::onSliderReleased() {
    m_isSliderMoving = false;
    setPlayerPosition(ui->playSlider->value());
}

void MusicPlayer::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{

    switch (state) {
    case QMediaPlayer::PlayingState:
        ui->playBtn->setIcon(QIcon(":/Resources/pause.svg"));
        break;

    case QMediaPlayer::PausedState:
    case QMediaPlayer::StoppedState:
        ui->playBtn->setIcon(QIcon(":/Resources/play.svg"));
        break;
    }
}

void MusicPlayer::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::NoMedia ||
        status == QMediaPlayer::InvalidMedia)
    {
        displayAlbumArt(m_defaultAlbum);
    }
    if (status == QMediaPlayer::EndOfMedia) {
        on_nextSongBtn_clicked();
    }
}

void MusicPlayer::handleMetaDataChanged()
{
    // 获取媒体播放器的元数据
    QMediaMetaData metadata = m_mediaPlayer->metaData();
    QVariant coverVar = metadata.value(QMediaMetaData::CoverArtImage);
    QVariant titleVar = metadata.value(QMediaMetaData::Title);
    QVariant albumVar = metadata.value(QMediaMetaData::AlbumTitle);
    QVariant artistVar = metadata.value(QMediaMetaData::ContributingArtist);
    ui->titleLab->setText(titleVar.toString());
    ui->titleLab->setToolTip(titleVar.toString());
    ui->artistLab->setText(artistVar.toString());
    ui->artistLab->setToolTip(artistVar.toString());
    ui->albumLab->setText(albumVar.toString());
    ui->albumLab->setToolTip(albumVar.toString());
    // 尝试获取CoverArtImage
    if (!coverVar.isNull() && coverVar.canConvert<QImage>()) {
        QImage img = coverVar.value<QImage>();
        if (!img.isNull()) {
            displayAlbumArt(QPixmap::fromImage(img));
            return;
        }
    }
    QVariant thumbVar = metadata.value(QMediaMetaData::ThumbnailImage);
    if (!thumbVar.isNull() && thumbVar.canConvert<QImage>()) {
        QImage img = thumbVar.value<QImage>();
        if (!img.isNull()) {
            displayAlbumArt(QPixmap::fromImage(img));
            return;
        }
    }
    // 显示默认封面
    displayAlbumArt(m_defaultAlbum);
}

void MusicPlayer::onVolumeSliderMoved(int value)
{
    qreal volume = value / 100.0;
    m_audioOutput->setVolume(volume);
    if (value == 0 && !m_isMuted) {
        m_isMuted = true;
        ui->volBtn->setIcon(QIcon(":/Resources/mute.svg"));
    } else if (value > 0 && m_isMuted) {
        m_isMuted = false;
        ui->volBtn->setIcon(QIcon(":/Resources/volume.svg"));
    }
}

void MusicPlayer::handleVolumeContextMenu(const QPoint &pos)
{
    Q_UNUSED(pos);
    toggleMute();
}

QString MusicPlayer::formatTime(qint64 ms) {
    qint64 seconds = ms / 1000;
    qint64 minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

int MusicPlayer::getPlaylistCount() const {
    return m_listModel->rowCount();
}

void MusicPlayer::playTrack(int index)
{
    if(index < 0 || index >= m_listModel->rowCount()) {
        return;
    }

    m_currentIndex = index;
    auto filePath = m_listModel->index(m_currentIndex, 0).data(Qt::UserRole+1).toString();

    if (!filePath.isEmpty()) {
        QString path = QDir::toNativeSeparators(filePath);
        qDebug() << "Playing file:" << path;
        m_mediaPlayer->stop();
        m_mediaPlayer->setSource(QUrl::fromLocalFile(path));
        loadLyrics(filePath);
        m_mediaPlayer->play();
        QModelIndex modelIndex = m_listModel->index(index, 0);
        ui->musicListView->selectionModel()->select(
            modelIndex,
            QItemSelectionModel::ClearAndSelect
            );
    }
}

int MusicPlayer::generateRandomIndex()
{
    int totalSongs = getPlaylistCount();
    if (totalSongs <= 0) return -1;
    if (totalSongs == 1) return 0;
    int newIndex;
    do {
        newIndex = QRandomGenerator::global()->bounded(totalSongs);
    } while (newIndex == m_currentIndex && totalSongs > 1);
    return newIndex;
}

void MusicPlayer::initVolumeControl()
{
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    m_volumeSlider->setFixedSize(80, 20);
    m_audioOutput->setVolume(0.5);
    ui->volBtn->setIcon(QIcon(":/Resources/volume.svg"));
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MusicPlayer::onVolumeSliderMoved);
    ui->volBtn->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->volBtn, &QWidget::customContextMenuRequested,
            this, &MusicPlayer::handleVolumeContextMenu);
    m_volumeSlider->hide();
}

void MusicPlayer::toggleMute()
{
    if (m_isMuted) {
        // 恢复音量
        m_audioOutput->setVolume(m_lastVolume / 100.0);
        m_volumeSlider->setValue(m_lastVolume);
        m_isMuted = false;
    } else {
        // 静音处理
        m_lastVolume = m_volumeSlider->value();  // 保存当前音量
        m_audioOutput->setVolume(0.0);
        m_volumeSlider->setValue(0);
        m_isMuted = true;
    }
    updateVolumeIcon();
}

void MusicPlayer::updateVolumeIcon()
{
    if (m_audioOutput->volume() == 0 || m_isMuted) {
        ui->volBtn->setIcon(QIcon(":/Resources/mute.svg"));
    } else {
        ui->volBtn->setIcon(QIcon(":/Resources/volume.svg"));
    }
}

void MusicPlayer::displayAlbumArt(const QPixmap& pixmap)
{
    m_albumScene->clear();

    QSize viewSize = QSize(180,180);
    QPixmap scaledPix = pixmap.scaled(
        viewSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );

    m_albumScene->setSceneRect(scaledPix.rect());
    m_albumScene->addPixmap(scaledPix);
}

void MusicPlayer::setNewBackground(const QString &path)
{
    m_currentBgPath = path;
    if (m_currentBg.load(m_currentBgPath)) {
        update();
    }
}

void MusicPlayer::loadLyrics(const QString& musicFilePath) {
    m_currentLyrics.clear();
    lyricAvailable = false;

    // 获取同路径下同名的.lrc文件
    QString lyricPath = musicFilePath;
    lyricPath.replace(".mp3", ".lrc").replace(".flac", ".lrc").replace(".wav", ".lrc");

    // 检查文件是否存在
    QFile lyricFile(lyricPath);
    if (!lyricFile.exists()) {
        ui->lyricLab->setText("找不到歌词喵");
        ui->lyricLab->setToolTip("找不到歌词喵");
        return;
    }

    // 打开并读取歌词文件
    if (lyricFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 默认UTF-8编码格式读取，如果为GBK等格式歌词显示会乱码
        QTextStream in(&lyricFile);
        // 解析歌词文件
        while (!in.atEnd()) {
            QString line = in.readLine();
            // 匹配时间戳格式 [mm:ss.zzz] 或 [mm:ss]
            static QRegularExpression timeRegex("\\[(\\d+):(\\d+)(?:\\.(\\d+))?\\]");
            QRegularExpressionMatch match = timeRegex.match(line);

            if (match.hasMatch()) {
                int min = match.captured(1).toInt();
                int sec = match.captured(2).toInt();
                int msec = match.captured(3).isNull() ? 0 : match.captured(3).toInt();

                // 计算总毫秒数
                qint64 totalMs = min * 60000 + sec * 1000;
                // 处理毫秒部分
                if (match.captured(3).length() == 2) {
                    msec *= 10;
                } else if (match.captured(3).length() < 3 && msec < 100) {
                    msec *= 10;
                }
                totalMs += msec;

                // 提取歌词文本
                QString lyricText = line.mid(match.capturedEnd());
                if (!lyricText.isEmpty()) {
                    m_currentLyrics[totalMs] = lyricText.trimmed();
                }
            }
        }
        lyricFile.close();
        lyricAvailable = !m_currentLyrics.isEmpty();
    } else {
        ui->lyricLab->setText("歌词文件读取失败");
        ui->lyricLab->setToolTip("歌词文件读取失败");
    }

    if (!lyricAvailable) {
        ui->lyricLab->setText("找不到歌词喵");
        ui->lyricLab->setToolTip("找不到歌词喵");
    }
}

QString MusicPlayer::findCurrentLyric(qint64 position) {
    if (m_currentLyrics.isEmpty()) return "找不到歌词喵";

    // 使用QMap的upperBound查找大于position的第一个元素
    auto it = m_currentLyrics.upperBound(position);

    // 如果没有找到或返回首个元素，则没有匹配的歌词行
    if (it == m_currentLyrics.begin())
        return "(这里没有歌词喔)";

    // 回退到当前行的歌词（小于等于position的最大时间戳）
    --it;
    return it.value();
}

void MusicPlayer::loadMusicList(){
    if (!defaultConfigPath.isEmpty())
    {
        QFile config(defaultConfigPath);
        if (!config.open(QIODevice::ReadOnly|QIODevice::Text))
        {
            return;
        }
        lastList=config.readLine().trimmed();
        m_currentMusicPath=lastList;
        lastIndex=config.readLine().trimmed().toInt();
        config.close();
    }
    QDirIterator it_music(lastList,{"*.mp3","*.wav","*.flac"});
    while (it_music.hasNext()){
        auto info = it_music.nextFileInfo();
        auto item = new QStandardItem(info.fileName());
        item->setData(it_music.fileInfo().canonicalFilePath());
        m_listModel->appendRow(item);
    }
    m_currentIndex = lastIndex;
    ui->playBtn->setIcon(QIcon(":/Resources/play.svg")); // 恢复播放图标
}

bool MusicPlayer::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->volBtn && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            // 检查是否点击在滑块上
            QPoint globalPos = mouseEvent->pos();
            QWidget *receiver = QApplication::widgetAt(globalPos);

            if (receiver &&(receiver == m_volumeSlider ||m_volumeSlider->isAncestorOf(receiver))){
                return false;
            }

            // 滑块位置
            QPoint buttonPos = ui->volBtn->pos();
            int sliderX = buttonPos.x() + m_volumeSlider->width() + 30;
            int sliderY = buttonPos.y() + m_volumeSlider->height() + 390;

            m_volumeSlider->move(sliderX, sliderY);
            m_volumeSlider->show();

            return true;
        }
        return false;
    }
    if (m_volumeSlider && (obj == m_volumeSlider ||obj->parent() == m_volumeSlider))
    {
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonRelease:
            return false;
        default:
            break;
        }
    }
    // 处理点击
    if (obj == this && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (m_volumeSlider->isVisible())
        {
            // 获取实际点击的控件
            QWidget *clickedWidget = QApplication::widgetAt(me->globalPosition().toPoint());
            // 检查点击是否在滑块或按钮范围内
            bool shouldHide = true;
            QWidget *w = clickedWidget;
            while (w) {
                if (w == this || w == nullptr) break;

                if (w == m_volumeSlider || w == ui->volBtn) {
                    shouldHide = false;
                    break;
                }
                w = w->parentWidget();
            }
            if (shouldHide) {
                m_volumeSlider->hide();
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void MusicPlayer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.drawPixmap(0, 0, width(), height(), m_currentBg);
}


