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
    std::string path = inputPath.toUtf8().constData();
    if(cv::utils::fs::isDirectory(path)) {
        m_reader = new ImageReader(inputPath);
    }
    else {
        m_reader = new VideoReader(inputPath);
    }

    if(m_reader->getPicCount() > 0) {
		setResolution();
        m_inputPath = inputPath;
    }
}


ModelInputPictures::ModelInputPictures()
{
}

bool ModelInputPictures::isKeyframe(unsigned int index) {
    return std::binary_search(this->m_keyframes.begin(), this->m_keyframes.end(), index);
}


void ModelInputPictures::updateMIP(const std::vector<unsigned int> &keyframes)
{
    this->m_keyframes = keyframes;
    emit sig_mipChanged();
}


void ModelInputPictures::addKeyframe(unsigned int index)
{
    if (isKeyframe(index)) {
        return;
    }
    this->m_keyframes.insert(std::upper_bound(this->m_keyframes.begin(), this->m_keyframes.end(), index), index);
    return;
}

void ModelInputPictures::removeKeyframe(unsigned int index) {
    if (isKeyframe(index)) {
        std::vector<unsigned int>::iterator it = std::find(this->m_keyframes.begin(), this->m_keyframes.end(), index);
        this->m_keyframes.erase(it);
    }
    return;
}


const cv::Mat* ModelInputPictures::getPic(unsigned int index){
    m_currentMat = m_reader->getPic(index);
    return &m_currentMat;
    /*if (index <= m_inputPics.size() - 1) {
      return &this->m_inputPics.at(index);
    }
    return nullptr;*/
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
    //std::vector<unsigned int>::iterator it = std::lower_bound(this->m_keyframes.begin(), this->m_keyframes.end()-1, index);

    //if(!isKeyframe(index)){
    //    stepsize--;
    //}

    //if index is keyframe the previous keyframe is needed
    //if (isKeyframe(index)) {
    //    if(it > m_keyframes.begin()){
    //        --it;
    //        stepsize--;
    //    }
    //    else {
    //        return *it;
    //    }
    //}

    //while(stepsize>0 && it > m_keyframes.begin()){
    //    it--;
    //    stepsize--;
    //}
    //return *it;

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
           keyStream << jsonEnum::jsonDelimiter.toStdString();
        }
        keyStream << m_keyframes[i];
    }
    std::string key = keyStream.str();
    QVariant keyframes(QString::fromStdString(key));



    QJsonObject jsonObject;
    QJsonValue::fromVariant(inputPath);
    jsonObject.insert(jsonEnum::keyframesIdentifier, QJsonValue::fromVariant(keyframes));
    jsonObject.insert(jsonEnum::inputPathIdentifier, QJsonValue::fromVariant(inputPath));
    return QVariant(jsonObject);

}

void ModelInputPictures::fromText(QVariant data)
{
    QJsonObject jsonData = data.toJsonObject();
    //get import part, create new reader and set resolution
    QJsonObject::Iterator inputPath = jsonData.find(jsonEnum::inputPathIdentifier);
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
    QString keyframes = jsonData.find(jsonEnum::keyframesIdentifier).value().toString();
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
    QStringList values = string.split(jsonEnum::jsonDelimiter);
    for (QString& val : values) {
        if (!val.isEmpty()) {
          returnVector.push_back(val.toUInt());
        }
    }
    return returnVector;
}









