import time
import os
import sys
import argparse
import re
from os import walk

from GpsEntry import GpsEntry

import yaml
import pyproj
import exifread

COLMAP_BIN = ""
WORK_QUEUE_YAML_PATH = ""
WORKER_STATE_YAML_PATH = ""

OPENCV_YAML_HEADER="%YAML:1.0\n---\n"

YAML_REFRESH_SECS = 1
LAST_PROGRESS_UPDATE_TIME = 0

#---------------------------------------------------------------------------------------------------------------------
# Class to dump YAML files prepared to be read by OpenCV.
# OpenCV FileStorage expects more indentations.
# Subclasses yaml.Dumper
class OpenCvYamlDumper(yaml.Dumper):

    def increase_indent(self, flow=False, indentless=False):
        return super(OpenCvYamlDumper, self).increase_indent(flow, False)
#---------------------------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------------------------
# Class representing specific job
class Job:
    def __init__(self, sequenceName: str, productType: int, jobState: int, progress: int, parameterList: dict) -> None:
        self.sequenceName = sequenceName
        self.productType = productType
        self.jobState = jobState
        self.progress  = progress
        self.parameterList = parameterList

    def __str__(self) -> str:
        return('[ sequenceName: {}, productType: {}, jobState: {}, progress: {}%, parameterList: {} ]'
            .format(self.sequenceName, self.getProductTypeStr(), self.getJobStateStr(),
            self.progress, self.parameterList))

    def getProductTypeStr(self) -> str:
        productNames = ['CAMERA_POSES', 'DENSE_CLOUD', 'MESHED_MODEL']
        return productNames[self.productType]

    def getJobStateStr(self) -> str:
        stateNames = ['JOB_DONE', 'JOB_RUNNING', 'JOB_PENDING']
        return stateNames[self.jobState]
#---------------------------------------------------------------------------------------------------------------------

######################################################################################################################
# Method to compute lest of gps reference data, needed for georegistration
def computeGpsRefernceList(colmapProjectDirPath: str, sequenceName: str, geoDir: str):

    # open gps output file
    fout_ecef = open('{}/ImgGpsList_ecef.txt'.format(geoDir) , 'w+')
    fout_offset = open('{}/GpsEcefOffset.txt'.format(geoDir) , 'w+')
    fout_wgs84 = open('{}/ImgGpsList_wgs84.txt'.format(geoDir) , 'w+')
    # write header to files
    fout_ecef.write('# FILENAME ECEF_X ECEF_Y ECEF_Z\r\n')
    fout_ecef.flush()
    fout_offset.write('# OFFSET_X OFFSET_Y OFFSET_Z\r\n')
    fout_offset.flush()
    fout_wgs84.write('# FILENAME LAT LONG ALT\r\n')
    fout_wgs84.flush()

    # loop over image files
    fileList = listImgFiles('{}/../{}.images/'.format(colmapProjectDirPath, sequenceName))
    fileList.sort()
    i = 0
    #ecef_offset_x, ecef_offset_y, ecef_offset_z
    for imgFilename in fileList:

        # Open image file for reading (binary mode) and read exif tags
        imgFile = open(imgFilename, 'rb')
        tags = exifread.process_file(imgFile)

        # create gps entry and read from exif
        gpsEntry = GpsEntry(0,0,0)
        gpsEntry.readFromExif(tags)

        # get and write ECEF Offset
        # calculate offset by using first entry of gps data
        if i == 0:
            ecef_offset_x, ecef_offset_y, ecef_offset_z = gpsEntry.toWGS84Ecef()
            fout_offset.write('{:.3f} {:.3f} {:.3f}\r\n'.format(ecef_offset_x, ecef_offset_y, ecef_offset_z))
            fout_offset.flush()

        # write data
        ecef_x, ecef_y, ecef_z = gpsEntry.toWGS84Ecef()
        fout_ecef.write('{} {:.3f} {:.3f} {:.3f}\r\n'.format(os.path.basename(imgFilename), (ecef_x - ecef_offset_x), (ecef_y - ecef_offset_y), (ecef_z - ecef_offset_z)))
        fout_ecef.flush()
        fout_wgs84.write('{} {} {} {}\r\n'.format(os.path.basename(imgFilename), gpsEntry.lat, gpsEntry.long, gpsEntry.alt))
        fout_wgs84.flush()

        i+=1

    fout_ecef.close()
    fout_offset.close()
    fout_wgs84.close()

######################################################################################################################
# Compute camera poses for given parameter list.
# Returns: True, if successful. False otherwise
def computeCameraPoses(colmapDatabaseFilePath: str, projectImageDir: str, colmapProjectDirPath: str, projectOutputDirPath: str, sequenceName: str, parameterList: dict) -> bool:
    global COLMAP_BIN

    print(' COMPUTING CAMERA POSES...')

    progressCallback(0)

    camModel = parameterList['camera_model']
    singleCam = parameterList['single_camera']
    multiModels = parameterList['multiple_models']
    gpus = parameterList['gpus']

    # run feature extraction
    print('\t> Running feature extractor...')
    cmd=('{} feature_extractor '
         '--database_path {} '
         '--image_path {} '
         '--ImageReader.camera_model {} '
         '--ImageReader.single_camera {} '
         '--SiftExtraction.gpu_index {}').format(COLMAP_BIN, colmapDatabaseFilePath, projectImageDir,
         camModel, singleCam, gpus)
    os.system(cmd)

    progressCallback(25)

    # run feature matching
    print('\t> Running exhaustive matcher...')
    cmd=('{} exhaustive_matcher '
         '--database_path {} '
         '--SiftMatching.gpu_index {}').format(COLMAP_BIN, colmapDatabaseFilePath, gpus)
    os.system(cmd)

    progressCallback(50)

    # run sparse mapping
    print('\t> Running mapper...')
    cmd=('{} mapper '
         '--database_path {} '
         '--image_path {} '
         '--output_path {}/01_sparse '
         '--Mapper.multiple_models {}').format(COLMAP_BIN, colmapDatabaseFilePath,
         projectImageDir, colmapProjectDirPath, multiModels)
    os.system(cmd)

    progressCallback(75)

    # run georegistration
    os.system('mkdir -p {}/01_sparse/geo/sparse_in/'.format(colmapProjectDirPath))
    os.system('mkdir -p {}/01_sparse/geo/sparse_out/'.format(colmapProjectDirPath))

    # copy data
    os.system('cp {}/01_sparse/0/* '
              '{}/01_sparse/geo/sparse_in/'.format(colmapProjectDirPath, colmapProjectDirPath))

    # compute reference informaion
    computeGpsRefernceList(colmapProjectDirPath, sequenceName, '{}/01_sparse/geo/'.format(colmapProjectDirPath))

    # run model aligner
    print('\t> Running model aligner...')
    cmd=('{} model_aligner '
         '--input_path {}/01_sparse/geo/sparse_in/ '
         '--output_path {}/01_sparse/geo/sparse_out/ '
         '--ref_images_path {}/01_sparse/geo/ImgGpsList_ecef.txt '
         '--robust_alignment 0').format(COLMAP_BIN, colmapProjectDirPath, colmapProjectDirPath,
         colmapProjectDirPath)
    os.system(cmd)

    # copy output data
    os.system('cp {}/01_sparse/geo/sparse_out/cameras.bin '
              '{}/{}_cameras.bin'.format(colmapProjectDirPath, projectOutputDirPath, sequenceName))
    os.system('cp {}/01_sparse/geo/sparse_out/images.bin '
              '{}/{}_images.bin'.format(colmapProjectDirPath, projectOutputDirPath, sequenceName))

    progressCallback(100)

    return True

######################################################################################################################
# Compute dense point cloud for given parameter list.
# Returns: True, if successful. False otherwise
def computeDenseCloud(projectImageDir: str, colmapProjectDirPath: str, projectOutputDirPath: str, sequenceName: str, parameterList: dict) -> bool:
    global COLMAP_BIN

    print(' COMPUTING DENSE CLOUD...')

    progressCallback(0)

    maxImgSize = parameterList['max_img_size']
    cacheSize = parameterList['cache_size']
    gpus = parameterList['gpus']

    # run image undistorter
    print('\t> Running image undistorter...')
    cmd=('{} image_undistorter '
         '--image_path {} '
         '--input_path {}/01_sparse/0 '
         '--output_path {}/02_dense/ '
         '--output_type COLMAP '
         '--max_image_size {}').format(COLMAP_BIN, projectImageDir, colmapProjectDirPath,
         colmapProjectDirPath, maxImgSize)
    os.system(cmd)

    progressCallback(25)

    # run patch match stereo
    print('\t> Running patch match stereo...')
    cmd=('{} patch_match_stereo '
         '--workspace_path {}/02_dense/ '
         '--workspace_format COLMAP '
         '--PatchMatchStereo.gpu_index {} '
         '--PatchMatchStereo.geom_consistency true '
         '--PatchMatchStereo.cache_size {}').format(COLMAP_BIN, colmapProjectDirPath, gpus, cacheSize)
    os.system(cmd)

    progressCallback(50)

    # run stereo fusion
    print('\t> Running stereo fusion...')
    os.system('mkdir -p {}/02_dense/fused_model'.format(colmapProjectDirPath))
    cmd=('{} stereo_fusion '
         '--workspace_path {}/02_dense/ '
         '--workspace_format COLMAP '
         '--input_type geometric '
         '--output_path {}/02_dense/fused_model/ '
         '--output_type BIN '
         '--StereoFusion.cache_size {}').format(COLMAP_BIN, colmapProjectDirPath, colmapProjectDirPath,
                                                cacheSize)
    os.system(cmd)

    progressCallback(75)

    # run georegistration
    os.system('mkdir -p {}/02_dense/geo/dense_in/'.format(colmapProjectDirPath))
    os.system('mkdir -p {}/02_dense/geo/dense_out/'.format(colmapProjectDirPath))

    # copy data
    os.system('cp {}/02_dense/fused_model/* '
              '{}/02_dense/geo/dense_in/'.format(colmapProjectDirPath, colmapProjectDirPath))

    # compute reference informaion
    computeGpsRefernceList(colmapProjectDirPath, sequenceName, '{}/02_dense/geo/'.format(colmapProjectDirPath))

    # run model aligner
    print('\t> Running model aligner...')
    cmd=('{} model_aligner '
         '--input_path {}/02_dense/geo/dense_in/ '
         '--output_path {}/02_dense/geo/dense_out/ '
         '--ref_images_path {}/02_dense/geo/ImgGpsList_ecef.txt '
         '--robust_alignment 0').format(COLMAP_BIN, colmapProjectDirPath, colmapProjectDirPath,
         colmapProjectDirPath)
    os.system(cmd)

    # run model_converter fusion
    print('\t> Running model converter...')
    cmd=('{} model_converter '
         '--input_path {}/02_dense/geo/dense_out/ '
         '--output_path {}/02_dense/{}_dense_cloud.ply '
         '--output_type PLY').format(COLMAP_BIN, colmapProjectDirPath, colmapProjectDirPath,
         sequenceName, cacheSize)
    os.system(cmd)

    # copy result
    cmd=('cp {}/02_dense/{}_dense_cloud.ply '
         '{}/02_dense/geo/GpsEcefOffset.txt '
         '{}/').format(colmapProjectDirPath, sequenceName, colmapProjectDirPath, projectOutputDirPath)
    os.system(cmd)

    progressCallback(100)

    return True

######################################################################################################################
# Compute meshed model for given parameter list.
# Returns: True, if successful. False otherwise
def computeMeshedModel(colmapProjectDirPath: str, projectOutputDirPath: str, sequenceName: str, parameterList: dict) -> bool:
    global COLMAP_BIN

    print(' COMPUTING Mesh...')

    progressCallback(0)

    # run poisson mesher
    print('\t> Running poisson mesher...')
    cmd=('{} poisson_mesher '
         '--input_path {}/02_dense/{}_dense_cloud.ply '
         '--output_path {}/02_dense/{}_meshed_model.ply').format(COLMAP_BIN, colmapProjectDirPath,
         sequenceName, colmapProjectDirPath, sequenceName)
    os.system(cmd)

    cmd=('cp {}/02_dense/{}_meshed_model.ply '
         '{}/').format(colmapProjectDirPath, sequenceName, projectOutputDirPath)
    os.system(cmd)

    progressCallback(100)

    return True

######################################################################################################################
# List image files in directory
# Returns: List of image files with absolute paths
def listImgFiles(dirPath: str) -> list:
    imgFileList = []

    fileList = os.listdir(dirPath) # this provides the list in an unordered manner
    fileList.sort()
    for file in fileList:
        if file.endswith( ('.JPG', '.jpg', '.PNG', '.png', '.BMP','.bmp')):
            imgFileList.append(os.path.join(dirPath, file))

    return imgFileList

######################################################################################################################
# Load yml file
# Returns: object of yaml file
def loadYaml(yamlFilePath: str):
    yamlObj = 0
    yamlLockFilePath = yamlFilePath + ".lock"

    # while lock file esists sleep
    while(os.path.exists(yamlLockFilePath)):
        time.sleep(0.1)

    # create lock file
    os.system('touch {}'.format(yamlLockFilePath))

    with open(yamlFilePath, 'r') as iStream:
        data = iStream.read()
        try:
            yamlObj = yaml.safe_load(data[len(OPENCV_YAML_HEADER):]) # handle opencv header
        except yaml.YAMLError as exc:
            print(exc)

    # remove lock file
    os.system('rm {}'.format(yamlLockFilePath))

    return yamlObj

######################################################################################################################
# Write yml file
def writeYaml(yamlFilePath: str, yamlObj):
    yamlLockFilePath = yamlFilePath + ".lock"

    # while lock file esists sleep
    while(os.path.exists(yamlLockFilePath)):
        time.sleep(0.1)

    # create lock file
    os.system('touch {}'.format(yamlLockFilePath))

    with open(yamlFilePath, 'w') as oStream:
        oStream.write(OPENCV_YAML_HEADER)  # handle opencv header
        try:
            yaml.dump(yamlObj, oStream, Dumper=OpenCvYamlDumper)
        except yaml.YAMLError as exc:
            print(exc)

    # remove lock file
    os.system('rm {}'.format(yamlLockFilePath))

######################################################################################################################
# Process given job
# Returns: True if job is processed. False otherwise
def processJob(workspacePath: str, currentJob: Job) -> bool:
    print("-- Processing Job ...")

    colmapDatabaseFilePath = os.path.join(workspacePath , currentJob.sequenceName + ".db")
    projectImageDir = os.path.join(workspacePath, currentJob.sequenceName + ".images")
    colmapProjectDirPath = os.path.join(workspacePath , currentJob.sequenceName + ".files")
    projectOutputDirPath = os.path.join(workspacePath , currentJob.sequenceName + ".output")

    os.system('mkdir -p {}'.format(projectImageDir))
    os.system('mkdir -p {}/01_sparse'.format(colmapProjectDirPath))
    os.system('mkdir -p {}/02_dense'.format(colmapProjectDirPath))
    os.system('mkdir -p {}'.format(projectOutputDirPath))

    # if current product type of job is CAMERA_POSES handle creation of project
    if( currentJob.getProductTypeStr() == 'CAMERA_POSES' ):

        # if image path in job description is not in project directory copy into project directory
        if( projectImageDir != currentJob.parameterList["image_path"] ):
            print("Images are outside of project dir. Copying {} -> {}".format(currentJob.parameterList["image_path"], projectImageDir))
            os.system('rm -rf {}/*'.format(projectImageDir))
            os.system('mkdir -p {}'.format(projectImageDir))
            os.system('cp {}/* {}'.format(currentJob.parameterList["image_path"], projectImageDir))
        # end if projectImageDir != image_path

        imgList = listImgFiles(projectImageDir)
        if( not os.path.isdir(projectImageDir) or len(imgList) == 0):
            print("Image directory {} does not exist or is empty.".format(projectImageDir))
            return False

        # compute camera poses
        success = computeCameraPoses(colmapDatabaseFilePath, projectImageDir, colmapProjectDirPath, projectOutputDirPath, currentJob.sequenceName, currentJob.parameterList)

        if( not success ):
            return False
    else:

        # if project file does not exist return false
        if( not os.path.exists( colmapDatabaseFilePath )):
            print("Project file {} or directory {} does not exist.".format(colmapDatabaseFilePath, colmapProjectDirPath))
            return False

        # if not returned handle job

        # compute corresponding product
        success = True
        if( currentJob.getProductTypeStr() == 'DENSE_CLOUD' ):
            success = computeDenseCloud(projectImageDir, colmapProjectDirPath, projectOutputDirPath, currentJob.sequenceName, currentJob.parameterList)

        elif( currentJob.getProductTypeStr() == 'MESHED_MODEL' ):
            success = computeMeshedModel(colmapProjectDirPath, projectOutputDirPath, currentJob.sequenceName, currentJob.parameterList)

        else:
            success = False

        if( not success ):
            return False

    # end if, else currentJob.getProductTypeStr == 'CAMERA_POSES'

    return True

######################################################################################################################
# Pop first item from job queue
# Returns: True and Object of Job Entry if queue has job. Fals and empty entry othewise.
def popFirstJobFromQueue(yamlFilePath: str) -> Job:
    global WORKER_STATE_YAML_PATH

    print("-- Reading job from list ...")
    yamlObj = loadYaml(yamlFilePath)
    if( yamlObj == 0 ):
        return False, Job("",-1,-1,-1,"")

    # check if there is a job in list. Return false if not.
    if( len(yamlObj["queue"]) == 0):
        print("No job in list.")
        return False, Job("",-1,-1,-1,"")

    # read and delete job
    yamlJobEntry = yamlObj["queue"][0]
    del yamlObj["queue"][0]

    # write current job to state file
    writeCurrentJobToStateFile(WORKER_STATE_YAML_PATH, yamlJobEntry)

    job = Job(yamlJobEntry["sequenceName"], yamlJobEntry["productType"], yamlJobEntry["jobState"], yamlJobEntry["progress"], yamlJobEntry["parameters"])
    print("Current Job: {}".format(job))

    # write modified yamlObj back to file
    writeYaml(yamlFilePath, yamlObj)

    return True, job

######################################################################################################################
# Method to write currently running job to state files
def writeCurrentJobToStateFile(yamlFilePath: str, currentJobYamlObj):
    print("-- Writing current job to state file ...")

    yamlObj = loadYaml(yamlFilePath)
    if( yamlObj == 0 ):
        return

    if( len(yamlObj["runningJob"]) != 0 ):
        yamlObj["runningJob"].clear()

    # set job state to Running
    currentJobYamlObj["jobState"] = 1

    # append to yaml obj
    yamlObj["runningJob"].append(currentJobYamlObj)

    writeYaml(yamlFilePath, yamlObj)

######################################################################################################################
# Method to write currently running job to state files
def progressCallback(progress: float):
    global LAST_PROGRESS_UPDATE_TIME
    global WORKER_STATE_YAML_PATH

    # if lock file exist return. no need to wait until finished
    if(os.path.exists(WORKER_STATE_YAML_PATH + ".lock")):
        return

    if ((time.time() - LAST_PROGRESS_UPDATE_TIME) > YAML_REFRESH_SECS):

        yamlObj = loadYaml(WORKER_STATE_YAML_PATH)
        if yamlObj is None:
            return

        if( len(yamlObj["runningJob"]) == 0 ):
            return

        # store global progress in yaml file
        yamlJobEntry = yamlObj["runningJob"][0]
        yamlJobEntry["progress"] = progress

        writeYaml(WORKER_STATE_YAML_PATH, yamlObj)

        LAST_PROGRESS_UPDATE_TIME = time.time()

###############################################################################
# Initialize argument parser and synopsis
# return list of arguments
def parseArguments():
    descriptionTxt='''\
    Python script to wrap workflow of generating a photogrammetric 3D model using COLMAP.
    '''

    parser = argparse.ArgumentParser(description=descriptionTxt)
    parser.add_argument("colmap_bin", help="Path to COLMAP binary.")
    parser.add_argument("worker_state_yaml_path", help="Path to YAML file holding worker state.")
    parser.add_argument("work_queue_yaml_path", help="Path to YAML file holding work queue.")

    return parser.parse_args()


######################################################################################################################
if __name__ == "__main__":

    print("---------------------------------------------------------")

    # init parsing of call arguments
    args = parseArguments()

    if (args.colmap_bin is None) or (args.worker_state_yaml_path is None) or (args.work_queue_yaml_path is None):
        print('ERROR: Not enough information arguments!')
        sys.exit()

    COLMAP_BIN = args.colmap_bin
    WORKER_STATE_YAML_PATH = args.worker_state_yaml_path
    WORK_QUEUE_YAML_PATH = args.work_queue_yaml_path

    if not os.path.exists(COLMAP_BIN):
        print('ERROR: {} does not exist!').format(COLMAP_BIN)
        sys.exit()

    if not os.path.exists(WORKER_STATE_YAML_PATH):
        print('ERROR: {} does not exist!').format(WORKER_STATE_YAML_PATH)
        sys.exit()

    if not os.path.exists(WORK_QUEUE_YAML_PATH):
        print('ERROR: {} does not exist!').format(WORK_QUEUE_YAML_PATH)
        sys.exit()

    # load work queue yaml and change into workspace directory
    workQueueFileObj = loadYaml(WORK_QUEUE_YAML_PATH)
    if( workQueueFileObj == 0 ):
        sys.exit()

    workspacePath = workQueueFileObj["workspace"]
    #os.chdir(workspacePath)

    # pop first object from queue
    isJobInList, currentJob = popFirstJobFromQueue(WORK_QUEUE_YAML_PATH)

    # loop through queue
    while(isJobInList):

        # process current job
        success = processJob(workspacePath, currentJob)
        if(not success):
            print("ERROR: Something went wrong when trying to process job {}".format(currentJob))

        # get next job in queue
        isJobInList, currentJob = popFirstJobFromQueue(WORK_QUEUE_YAML_PATH)
    # end while(isJobInList)

    # clear state file
    yamlObj = loadYaml(WORKER_STATE_YAML_PATH)
    if( yamlObj == 0 ):
        sys.exit()

    if( len(yamlObj["runningJob"]) != 0 ):
        yamlObj["runningJob"].clear()

    writeYaml(WORKER_STATE_YAML_PATH, yamlObj)

    print("---------------------------------------------------------")
