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
    m_metaDataManager = new MetaDataManager();
    std::string path = inputPath.toUtf8().constData();
    if(cv::utils::fs::isDirectory(path)) {
        m_reader = new ImageReader(inputPath);
        //Always try to load meta data when images are imported
        loadMetaDataImages();
    }
    else {
        m_reader = new VideoReader(inputPath);
    }

    if(m_reader->getPicCount() > 0) {
		setResolution();
        m_inputPath = inputPath;
    }
    m_boundaries = QPoint(0,m_reader->getPicCount()-1);
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
    return;
}

void ModelInputPictures::removeKeyframe(unsigned int index) {
    if (isKeyframe(index)) {
        std::vector<unsigned int>::iterator it = std::find(this->m_keyframes.begin(), this->m_keyframes.end(), index);
        this->m_keyframes.erase(it);
        AlgorithmManager::instance().notifyKeyframesChanged(m_keyframes);
    }
    return;
}


const cv::Mat* ModelInputPictures::getPic(unsigned int index){
    m_currentMat = m_reader->getPic(index);
    return &m_currentMat;
}


unsigned int ModelInputPictures::getKeyframeCount(){
    return static_cast<unsigned int>(m_keyframes.size());
}

unsigned int ModelInputPictures::getPicCount(){
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
    return new DelayedCopyReader(m_reader); // return a copy, but only create that copy as it is needed
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



    QJsonObject jsonObject;
    QJsonValue::fromVariant(inputPath);
    jsonObject.insert(stringContainer::keyframesIdentifier, QJsonValue::fromVariant(keyframes));
    jsonObject.insert(stringContainer::inputPathIdentifier, QJsonValue::fromVariant(inputPath));
    return QVariant(jsonObject);

}

void ModelInputPictures::fromText(QVariant data)
{
    QJsonObject jsonData = data.toJsonObject();
    //get import part, create new reader and set resolution
    QJsonObject::Iterator inputPath = jsonData.find(stringContainer::inputPathIdentifier);
    if (cv::utils::fs::isDirectory(inputPath.value().toString().toStdString())) {
        m_reader = new ImageReader(inputPath.value().toString());
    }
    else {
        m_reader = new VideoReader(inputPath.value().toString());
    }
    m_inputPath = inputPath.value().toString();
    if (m_reader->getPicCount() != 0) {
        setResolution();
    }
    //get keyframes
    QString keyframes = jsonData.find(stringContainer::keyframesIdentifier).value().toString();
    m_keyframes = splitString(keyframes);


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
    m_metaDataManager->initMetaDataVideo(paths, m_reader);
    m_reader->addMetaData(m_metaDataManager);
    int metaDataLoaded =  m_metaDataManager->availableMetaData().size() - oldMetaCount;
    if (metaDataLoaded > 0) {
        AlgorithmManager::instance().notifyNewMetaData();
    }
    return metaDataLoaded;
}

int ModelInputPictures::loadMetaDataImages()
{
    m_metaDataManager->initMetaDataImages(m_reader);
    m_reader->addMetaData(m_metaDataManager);
    int metaDataLoaded = m_metaDataManager->availableMetaData().size();
    if (metaDataLoaded > 0) {
        AlgorithmManager::instance().notifyNewMetaData();
    }
    return metaDataLoaded;
}

std::vector<unsigned int> ModelInputPictures::getAllKeyframes()
{   
    return m_keyframes;
}

std::vector<unsigned int> ModelInputPictures::getAllKeyframes(QPoint boundaries)
{
    // check if boundaries are valid
    Q_ASSERT(0 <= boundaries.x() && boundaries.x() <= boundaries.y());

    std::vector<unsigned int> croppedKeyframes;

    // skip if the there are no keyframes
    if (m_keyframes.size() == 0) {
        return croppedKeyframes;
    }

    uint startFrame = boundaries.x();
    uint endFrame = boundaries.y();


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









