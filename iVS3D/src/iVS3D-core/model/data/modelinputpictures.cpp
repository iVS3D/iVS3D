#include "modelinputpictures.h"



void ModelInputPictures::setResolution() {
    cv::Mat reso = m_reader->getPic(0);
    int cols = reso.cols;
    int rows = reso.rows;
    //inverting rows and collumns results in correct "width x height"
    QPoint resolution(cols, rows);
    m_inputResolution = resolution;
}


ModelInputPictures::ModelInputPictures(QString inputPath)
{
    m_metaDataManager = &MetaDataManager::instance();
    m_metaDataManager->resetData();
    m_reader = ReaderFactory::instance().createReader(inputPath);

    if (m_reader == nullptr) {
        return;
    }

    if(m_reader->getPicCount() > 0) {
		setResolution();
        m_inputPath = inputPath;
    }

    if (m_reader->isDir()) {
        loadMetaDataImages();
    }
    else {
        //if a video is loaded, search for a srt file with the same name and try to loadMetaData from it
        QFileInfo info = QFileInfo (inputPath);
        QDir dir = info.dir();
        QString metaName = info.baseName().append(".srt");
        if (dir.exists(metaName)) {
            loadMetaData(QStringList(dir.filePath(metaName)));
        }
    }

    m_boundaries = QPoint(0,m_reader->getPicCount()-1);
    m_keyframes.reserve(m_reader->getPicCount());
    for(uint i = 0; i < m_reader->getPicCount(); i++){
        m_keyframes.push_back(i);
    }
}


ModelInputPictures::ModelInputPictures()
{
    m_boundaries = QPoint(0,0);
}

bool ModelInputPictures::isKeyframe(unsigned int index) {
    return std::binary_search(this->m_keyframes.begin(), this->m_keyframes.end(), index);
}


void ModelInputPictures::updateMIP(const std::vector<unsigned int> &keyframes)
{
    // TODO!!! optimization possible: no need to copy lists, just count keyframes
    // inside / outside boundries and create iterator that filters

    auto inBoundaries = [=](uint i) {
        return i >= (uint)m_boundaries.x() && i <= (uint)m_boundaries.y();
    };
    // keep old keyframes outside the boundaries
    std::vector<uint> old_frames;
    std::copy_if(
                this->m_keyframes.begin(),
                this->m_keyframes.end(),
                std::back_inserter(old_frames),
                [=](uint i){ return !inBoundaries(i); });

    // only take new keyframes inside the boundaries
    std::vector<uint> new_frames;
    std::copy_if(
                keyframes.begin(),
                keyframes.end(),
                std::back_inserter(new_frames),
                inBoundaries);

    // frames to keep are stored in old_frames
    // allocate space for new keyframes
    m_keyframes.clear();
    m_keyframes.resize(old_frames.size() + new_frames.size());

    // merge old frames outside boundaries with new keyframes
    std::merge(
                old_frames.begin(),
                old_frames.end(),
                new_frames.begin(),
                new_frames.end(),
                this->m_keyframes.begin());

    AlgorithmManager::instance().notifyKeyframesChanged(m_keyframes);
    emit sig_mipChanged();
}


void ModelInputPictures::addKeyframe(unsigned int index)
{
    if (isKeyframe(index)) {
        return;
    }
    this->m_keyframes.insert(std::upper_bound(this->m_keyframes.begin(), this->m_keyframes.end(), index), index);
    AlgorithmManager::instance().notifyKeyframesChanged(m_keyframes);
    emit sig_mipChanged();
    return;
}

void ModelInputPictures::removeKeyframe(unsigned int index) {
    if (isKeyframe(index)) {
        std::vector<unsigned int>::iterator it = std::find(this->m_keyframes.begin(), this->m_keyframes.end(), index);
        this->m_keyframes.erase(it);
        AlgorithmManager::instance().notifyKeyframesChanged(m_keyframes);
        emit sig_mipChanged();
    }
    return;
}


const cv::Mat* ModelInputPictures::getPic(unsigned int index){
    m_currentMat = m_reader->getPic(index);
    return &m_currentMat;
}


unsigned int ModelInputPictures::getKeyframeCount(bool inBound){
    return static_cast<unsigned int>(getAllKeyframes(inBound).size());
}

unsigned int ModelInputPictures::getPicCount(){
    if (m_reader == nullptr) {
        return 0;
    }
    return m_reader->getPicCount();
}

unsigned int ModelInputPictures::getNextKeyframe(unsigned int index, unsigned int stepsize){
    if(m_keyframes.size() == 0){
        return index;
    }
   std::vector<unsigned int>::iterator it = std::upper_bound(this->m_keyframes.begin(), this->m_keyframes.end()-1, index);

   stepsize--;
   while(stepsize > 0 && it != m_keyframes.end()-1){
       ++it;
       stepsize--;
   }
   return *it;
}

unsigned int ModelInputPictures::getPreviousKeyframe(unsigned int index, unsigned int stepsize)
{
    if(m_keyframes.size() == 0){
        return index;
    }

    for(int i = 0; i<(int)m_keyframes.size();i++){
        if(m_keyframes[i] >= index){
            return m_keyframes[(i>=(int)stepsize) ? i-stepsize : 0];
        }
    }
    return m_keyframes[(m_keyframes.size()>stepsize) ? m_keyframes.size()-stepsize : 0];
}

QString ModelInputPictures::getPath()
{
    return m_inputPath;
}

Reader *ModelInputPictures::getReader()
{
    return m_reader;
}

ConcurrentReader *ModelInputPictures::createConcurrentReader()
{
    return new ConcurrentReader(m_reader);
}


QVariant ModelInputPictures::toText()
{
    QVariant inputPath(m_inputPath);

    //Keyframe vector to Variant
    std::stringstream keyStream;
    for (uint i = 0; i < m_keyframes.size(); i++) {
        if (i != 0) {
           keyStream << stringContainer::jsonDelimiter.toStdString();
        }
        keyStream << m_keyframes[i];
    }
    std::string key = keyStream.str();
    QVariant keyframes(QString::fromStdString(key));

    QPoint boundaries = getBoundaries();
    QVariantList varBoundaries = {boundaries.x(), boundaries.y()};

    QJsonObject jsonObject;
    QJsonValue::fromVariant(inputPath);
    jsonObject.insert(stringContainer::keyframesIdentifier, QJsonValue::fromVariant(keyframes));
    jsonObject.insert(stringContainer::inputPathIdentifier, QJsonValue::fromVariant(inputPath));
    jsonObject.insert(stringContainer::boundariesIdentifier, QJsonValue::fromVariant(varBoundaries));
    return QVariant(jsonObject);

}

void ModelInputPictures::fromText(QVariant data)
{
    QJsonObject jsonData = data.toJsonObject();
    //get import part, create new reader and set resolution
    QJsonObject::Iterator inputPath = jsonData.find(stringContainer::inputPathIdentifier);
    m_inputPath = inputPath.value().toString();
    m_reader = ReaderFactory::instance().createReader(m_inputPath);


    if (m_reader->getPicCount() != 0) {
        setResolution();
        m_boundaries = QPoint(0,m_reader->getPicCount() -1);
    }
    //get keyframes
    QString keyframes = jsonData.find(stringContainer::keyframesIdentifier).value().toString();
    m_keyframes = splitString(keyframes);

    // get boundaries
    QVariantList varBoundaries = jsonData.find(stringContainer::boundariesIdentifier).value().toVariant().toList();
    if (varBoundaries.size() == 2) {
        QPoint boundaries;
        boundaries.setX(varBoundaries.at(0).toInt());
        boundaries.setY(varBoundaries.at(1).toInt());
        setBoundaries(boundaries);
    }
}

QPoint ModelInputPictures::getBoundaries()
{
    return m_boundaries;
}

void ModelInputPictures::setBoundaries(QPoint boundaries)
{

    m_boundaries = boundaries;
}

int ModelInputPictures::loadMetaData(QStringList paths)
{
    int oldMetaCount = m_metaDataManager->availableMetaData().size();
    m_metaDataManager->initMetaDataVideo(paths, m_reader->getPicCount(), m_reader->getFPS());
    m_reader->addMetaData(m_metaDataManager);
    int metaDataLoaded =  m_metaDataManager->availableMetaData().size() - oldMetaCount;
    return metaDataLoaded;
}

int ModelInputPictures::loadMetaDataImages()
{
    m_metaDataManager->initMetaDataImages(m_reader->getFileVector());
    m_reader->addMetaData(m_metaDataManager);
    int metaDataLoaded = m_metaDataManager->availableMetaData().size();
    return metaDataLoaded;
}

ModelInputPictures::Memento *ModelInputPictures::save()
{
    return new Memento(m_keyframes);
}

void ModelInputPictures::restore(ModelInputPictures::Memento *m)
{
    m_keyframes = m->getState();
    emit sig_mipChanged();
}

std::vector<unsigned int> ModelInputPictures::getAllKeyframes(bool inBound)
{   
    if (!inBound) {
        // all keyframes
        return m_keyframes;
    }

    // crop keyframes with bounds
    // check if boundaries are valid
    Q_ASSERT(0 <= m_boundaries.x() && m_boundaries.x() <= m_boundaries.y());

    std::vector<unsigned int> croppedKeyframes;

    // skip if the there are no keyframes
    if (m_keyframes.size() == 0) {
        return croppedKeyframes;
    }

    uint startFrame = m_boundaries.x();
    uint endFrame = m_boundaries.y();


    for (uint currKeyframe : m_keyframes) {
        // breaks when an the keyframe is greater than startFrame (front boundary)
        if (startFrame <= currKeyframe && currKeyframe <= endFrame) {
            croppedKeyframes.push_back(currKeyframe);
        }
    }
    return croppedKeyframes;
}

QPoint ModelInputPictures::getInputResolution()
{
    return m_inputResolution;
}

std::vector<unsigned int> ModelInputPictures::splitString(QString string) {

    std::vector<unsigned int> returnVector;
    QStringList values = string.split(stringContainer::jsonDelimiter);
    for (QString& val : values) {
        if (!val.isEmpty()) {
          returnVector.push_back(val.toUInt());
        }
    }
    return returnVector;
}


QDateTime ModelInputPictures::Memento::getSnapshotDate()
{
    return m_dateTime;
}

int ModelInputPictures::Memento::getNumImages()
{
    return m_state.size();
}

ModelInputPictures::Memento::Memento(std::vector<uint> state) : m_state(state) { m_dateTime = QDateTime::currentDateTime(); }

std::vector<uint> ModelInputPictures::Memento::getState()
{
    return m_state;
}
