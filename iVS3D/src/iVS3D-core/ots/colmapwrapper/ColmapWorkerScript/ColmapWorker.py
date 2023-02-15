import time
import os
import sys
import argparse
import subprocess
import shutil
from pathlib import Path

from GpsEntry import GpsEntry
import yaml 
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
# Subclasses Dumper
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
        stateNames = ['JOB_DONE', 'JOB_RUNNING', 'JOB_PENDING', 'JOB_FAILED']
        return stateNames[self.jobState]
#---------------------------------------------------------------------------------------------------------------------

######################################################################################################################
# Helper method for python < 3.8  (dirs_exist_ok is python3.8)
def _copytree(entries, src, dst, symlinks, ignore, copy_function,
              ignore_dangling_symlinks, dirs_exist_ok=False):
    if ignore is not None:
        ignored_names = ignore(os.fspath(src), [x.name for x in entries])
    else:
        ignored_names = set()

    os.makedirs(dst, exist_ok=dirs_exist_ok)
    errors = []
    use_srcentry = copy_function is shutil.copy2 or copy_function is shutil.copy

    for srcentry in entries:
        if srcentry.name in ignored_names:
            continue
        srcname = os.path.join(src, srcentry.name)
        dstname = os.path.join(dst, srcentry.name)
        srcobj = srcentry if use_srcentry else srcname
        try:
            is_symlink = srcentry.is_symlink()
            if is_symlink and os.name == 'nt':
                # Special check for directory junctions, which appear as
                # symlinks but we want to recurse.
                lstat = srcentry.stat(follow_symlinks=False)
                if lstat.st_reparse_tag == stat.IO_REPARSE_TAG_MOUNT_POINT:
                    is_symlink = False
            if is_symlink:
                linkto = os.readlink(srcname)
                if symlinks:
                    # We can't just leave it to `copy_function` because legacy
                    # code with a custom `copy_function` may rely on copytree
                    # doing the right thing.
                    os.symlink(linkto, dstname)
                    copystat(srcobj, dstname, follow_symlinks=not symlinks)
                else:
                    # ignore dangling symlink if the flag is on
                    if not os.path.exists(linkto) and ignore_dangling_symlinks:
                        continue
                    # otherwise let the copy occur. copy2 will raise an error
                    if srcentry.is_dir():
                        copytree(srcobj, dstname, symlinks, ignore,
                                 copy_function, dirs_exist_ok=dirs_exist_ok)
                    else:
                        copy_function(srcobj, dstname)
            elif srcentry.is_dir():
                copytree(srcobj, dstname, symlinks, ignore, copy_function,
                         dirs_exist_ok=dirs_exist_ok)
            else:
                # Will raise a SpecialFileError for unsupported file types
                copy_function(srcobj, dstname)
        # catch the Error from the recursive copytree so that we can
        # continue with other files
        except shutil.Error as err:
            errors.extend(err.args[0])
        except OSError as why:
            errors.append((srcname, dstname, str(why)))
    try:
        shutil.copystat(src, dst)
    except OSError as why:
        # Copying file access times may fail on Windows
        if getattr(why, 'winerror', None) is None:
            errors.append((src, dst, str(why)))
    if errors:
        raise shutil.Error(errors)
    return dst

def copytree(src, dst, symlinks=False, ignore=None, copy_function=shutil.copy2,
             ignore_dangling_symlinks=False, dirs_exist_ok=False):
  
    #sys.audit("shutil.copytree", src, dst)
    with os.scandir(src) as itr:
        entries = list(itr)
    return _copytree(entries=entries, src=src, dst=dst, symlinks=symlinks,
                     ignore=ignore, copy_function=copy_function,
                     ignore_dangling_symlinks=ignore_dangling_symlinks,
                     dirs_exist_ok=dirs_exist_ok)

######################################################################################################################
# Method to compute list of gps reference data, needed for georegistration, returns false if no valid gps reference data found
def computeGpsRefernceList(colmapProjectDirPath: str, sequenceName: str, geoDir: str):

    # open gps output file
    fout_ecef = open(os.path.join(geoDir, "ImgGpsList_ecef.txt") , 'w+')
    fout_offset = open(os.path.join(geoDir, "GpsEcefOffset.txt"), 'w+')
    fout_wgs84 = open(os.path.join(geoDir, "ImgGpsList_wgs84.txt"), 'w+')
    # write header to files
    fout_ecef.write('# FILENAME ECEF_X ECEF_Y ECEF_Z\r\n')
    fout_ecef.flush()
    fout_offset.write('# OFFSET_X OFFSET_Y OFFSET_Z\r\n')
    fout_offset.flush()
    fout_wgs84.write('# FILENAME LAT LONG ALT\r\n')
    fout_wgs84.flush()

    # loop over image files
    fileList = listImgFiles(os.path.join(colmapProjectDirPath, "..", sequenceName + ".images"))
    fileList.sort()
    i = 0
    #sum of all gps entries to detect empty gps data
    gps_sum = 0
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
        gps_sum += ecef_x - ecef_offset_x + ecef_y - ecef_offset_y + ecef_z - ecef_offset_z

        fout_ecef.flush()
        fout_wgs84.write('{} {} {} {}\r\n'.format(os.path.basename(imgFilename), gpsEntry.lat, gpsEntry.long, gpsEntry.alt))
        fout_wgs84.flush()

        i+=1

    fout_ecef.close()
    fout_offset.close()
    fout_wgs84.close()

    return int(gps_sum) != 0

######################################################################################################################
# Compute ETA (expected time of arrival) 
# Inits stop watch and data structures
# Returns: closure function, which retruns eta in ms for specific future time step calculated based one gathered previous time steps
# Estimation is based on the mean of the last 20 iterations
# Performs linear approximation
# TODO add option for quadratic approximation
def init_eta_calculation():
    cumulative_times = []
    step_times = []
    start_time = int(time.time() * 1000)

    def calculate_eta(eta_for_step):
        nonlocal cumulative_times, step_times
        last_times = step_times[-20:]
        sum = 0
        for times_for_step in last_times: sum += times_for_step
        return int(cumulative_times[-1] + (eta_for_step - len(cumulative_times)) * (sum / len(last_times)))   

    def get_eta_after_step(eta_for_step):
        nonlocal start_time, cumulative_times, step_times 

        if len(cumulative_times) == 0:
            step_times.append(int(time.time() * 1000 - start_time))
        else:
            step_times.append(int(time.time() * 1000 - start_time) - cumulative_times[-1])  

        cumulative_times.append(int(time.time() * 1000 - start_time))
        eta = calculate_eta(eta_for_step) - int(time.time() * 1000 - start_time)       
        eta = eta if eta > 0 else 0
        return eta
        
    return get_eta_after_step


######################################################################################################################
# Compute camera poses for given parameter list.
# Returns: True, if successful. False otherwise
def computeCameraPoses(colmapDatabaseFilePath: str, projectImageDir: str, colmapProjectDirPath: str, projectOutputDirPath: str, sequenceName: str, parameterList: dict, robust_mode: bool) -> bool:
    global COLMAP_BIN

    print(' COMPUTING CAMERA POSES...')
    progressCallback(0, force_Write = True, step=1)

    camModel = parameterList['camera_model']
    singleCam = parameterList['single_camera']
    multiModels = parameterList['multiple_models']
    gpus = parameterList['gpus']
    max_focal_length_ratio = parameterList['max_focal_length_ratio']

    # run feature extraction
    print('\t> Running feature extractor...')
    p = subprocess.Popen([COLMAP_BIN, 
        "feature_extractor", 
        "--database_path", colmapDatabaseFilePath, 
        "--image_path", projectImageDir, 
        "--ImageReader.camera_model", camModel,
        "--ImageReader.single_camera", singleCam,
        "--SiftExtraction.gpu_index", gpus], stdout=subprocess.PIPE)     

    number_of_images = 0
    get_eta_after_step = init_eta_calculation()
    while p.poll() is None:
        output = p.stdout.readline()
        if output != b"":
            line = output.strip().decode("utf-8")
            print(line)
            # Scanning COLMAP stdout for progress
            if "Processed file" in line:               
                current_images = line[16:-1].split("/")
                if len(current_images) == 2:
                    current_image_number, number_of_images = current_images
                    current_image_number = int(current_image_number)
                    number_of_images = int(number_of_images)
                    eta = get_eta_after_step(number_of_images)
                    progressCallback(round(10 * current_image_number / number_of_images), eta=eta, step=1)

    if p.poll() != 0:
        return False    

    progressCallback(10, force_Write = True)

    # run feature matching
    p = subprocess.Popen([COLMAP_BIN, 
        "exhaustive_matcher", 
        "--database_path", colmapDatabaseFilePath,  
        "--ExhaustiveMatching.block_size", "10",
        "--SiftMatching.gpu_index", gpus], stdout=subprocess.PIPE)     

    get_eta_after_step = init_eta_calculation()
    while p.poll() is None:
        output = p.stdout.readline()
        if output != b"":
            line = output.strip().decode("utf-8")
            print(line)
            # Scanning COLMAP stdout for progress
            if "Matching block" in line:               
                blocks = line[16:-11].split(", ")                            
                if len(blocks) == 2:
                    block1_1, block1_2 = blocks[0].split("/")
                    block2_1, block2_2 = blocks[1].split("/")
                    current_iteration = int(block2_1) + (int(block1_1) - 1) * int(block1_2)
                    max_iteration = int(block1_2) ** 2
                    eta = get_eta_after_step(max_iteration)
                    progressCallback(10 + round(35 * current_iteration / max_iteration), eta=eta, step=2)

    if p.poll() != 0:
        return False   

    progressCallback(45, force_Write = True)

    # run sparse mapping
    # in robust mode the estimation of distortion params is omitted to help with convergence (less params to estimate)
    refine_extra_params = "1" if not robust_mode else "0"
    print('\t> Running mapper...')
    p = subprocess.Popen([COLMAP_BIN, 
        "mapper", 
        "--database_path", colmapDatabaseFilePath,  
        "--image_path", projectImageDir,
        "--output_path", os.path.join(colmapProjectDirPath, "01_sparse"),
        "--Mapper.max_focal_length_ratio", max_focal_length_ratio,
        "--Mapper.ba_refine_extra_params", refine_extra_params,
        "--Mapper.ba_local_max_num_iterations", "25",
        "--Mapper.ba_global_max_num_iterations", "50",
        "--Mapper.multiple_models", multiModels], stdout=subprocess.PIPE)     

    max_number_of_registered_images = 0
    get_eta_after_step = init_eta_calculation()

    while p.poll() is None:
        output = p.stdout.readline()
        if output != b"":
            line = output.strip().decode("utf-8")
            print(line)
            # Scanning COLMAP stdout for progress
            if "Registering image" in line:  
                number_of_registered_images = int(line[18:].split("(")[-1][:-1])
                if max_number_of_registered_images < number_of_registered_images:
                    max_number_of_registered_images = number_of_registered_images
                    eta = get_eta_after_step(number_of_images)
                    progressCallback(45 + round(45 * max_number_of_registered_images / number_of_images), 
                        eta=eta, step=3)
                         
    if p.poll() != 0 or not os.path.exists(os.path.join(colmapProjectDirPath, "01_sparse", "0")):
        return False

    # In robust mode the estimation of distortion params is only performed in a downstream bundle adjustment after mapping succeeded
    # in the first two iterations distortion and principal point are refined on their own, then the intrinsics are refined together
    # In the last step, intrinsics and extrinsics are refined together.
    if robust_mode:
        print('\t> Running robust bundle adjustment...')
        param_list = [["0","0","1","0"],["0","1","0","0"],["1","1","1","0"],["1","1","1","1"]]
        get_eta_after_step = init_eta_calculation()
        for i in range(len(param_list)):
            params = param_list[i]
            p = subprocess.Popen([COLMAP_BIN, 
                "bundle_adjuster", 
                "--input_path", os.path.join(colmapProjectDirPath, "01_sparse", "0"),
                "--output_path", os.path.join(colmapProjectDirPath, "01_sparse", "0"),
                "--BundleAdjustment.max_num_iterations", "1000",
                "--BundleAdjustment.refine_focal_length", params[0],
                "--BundleAdjustment.refine_principal_point", params[1],
                "--BundleAdjustment.refine_extra_params", params[2],
                "--BundleAdjustment.refine_extrinsics", params[3]], stdout=subprocess.PIPE) 
                
            while p.poll() is None:
                output = p.stdout.readline()
                if output != b"":
                    line = output.strip().decode("utf-8")
                    print(line)  
            
            eta = get_eta_after_step(len(param_list))
            progressCallback(90 + i +1 , force_Write = True, eta=eta, step=3)


    progressCallback(95, force_Write = True)

    # run georegistration
    sparse_in_path = os.path.join(colmapProjectDirPath, "01_sparse", "geo", "sparse_in")
    if not os.path.exists(sparse_in_path):
        os.makedirs(sparse_in_path)
    
    sparse_out_path = os.path.join(colmapProjectDirPath, "01_sparse", "geo", "sparse_out")
    if not os.path.exists(sparse_out_path):
        os.makedirs(sparse_out_path)

    # copy data
    copytree(os.path.join(colmapProjectDirPath, "01_sparse", "0"), os.path.join(colmapProjectDirPath, "01_sparse", "geo", "sparse_in"), dirs_exist_ok=True) 
 

    # compute reference informaion
    success = computeGpsRefernceList(colmapProjectDirPath, sequenceName, os.path.join(colmapProjectDirPath, "01_sparse", "geo"))

    # running model aligner if computeGpsRefernceList was successfull
    if success:
        # run model aligner
        print('\t> Running model aligner...')
        cmd=('{} model_aligner '
            '--input_path ' + sparse_in_path + " ",
            '--output_path ' + sparse_out_path + " ",
            '--ref_images_path ' + os.path.join(colmapProjectDirPath, "01_sparse", "geo", "ImgGpsList_ecef.txt") + " ",
            '--robust_alignment 0').format(COLMAP_BIN)
        os.system(cmd)    

        # copy output data
        shutil.copy(os.path.join(colmapProjectDirPath, "01_sparse", "geo", "sparse_out", "cameras.bin"), 
            os.path.join(projectOutputDirPath, sequenceName + "_cameras.bin")) 
        shutil.copy(os.path.join(colmapProjectDirPath, "01_sparse", "geo", "sparse_out", "images.bin"), 
            os.path.join(projectOutputDirPath, sequenceName + "_images.bin")) 
    else:
        # copy output data
        shutil.copy(os.path.join(colmapProjectDirPath, "01_sparse", "geo", "sparse_in", "cameras.bin"), 
            os.path.join(projectOutputDirPath, sequenceName + "_cameras.bin")) 
        shutil.copy(os.path.join(colmapProjectDirPath, "01_sparse", "geo", "sparse_in", "images.bin"), 
            os.path.join(projectOutputDirPath, sequenceName + "_images.bin")) 

    progressCallback(100, force_Write = True)

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
    p = subprocess.Popen([COLMAP_BIN, 
        "image_undistorter", 
        "--image_path", projectImageDir,
        "--input_path", os.path.join(colmapProjectDirPath, "01_sparse", "0"),
        "--output_path", os.path.join(colmapProjectDirPath, "02_dense"),
        "--output_type", "COLMAP",
        "--max_image_size", maxImgSize], stdout=subprocess.PIPE)     

    get_eta_after_step = init_eta_calculation()
    while p.poll() is None:
        output = p.stdout.readline()
        if output != b"":
            line = output.strip().decode("utf-8")
            print(line)
            # Scanning COLMAP stdout for progress
            if "Undistorting image" in line:   
                current_images = line[20:-1].split("/")
                if len(current_images) == 2:
                    current_image_number, number_of_images = current_images
                    current_image_number = int(current_image_number)
                    number_of_images = int(number_of_images)
                    eta = get_eta_after_step(number_of_images)
                    progressCallback(round(5 * current_image_number / number_of_images), eta=eta, step=1)

    if p.poll() != 0:
        return False   

    progressCallback(5, force_Write=True)

    # run patch match stereo
    print('\t> Running patch match stereo...')
    p = subprocess.Popen([COLMAP_BIN, 
        "patch_match_stereo", 
        "--workspace_path", os.path.join(colmapProjectDirPath, "02_dense"),
        "--workspace_format", "COLMAP",
        "--PatchMatchStereo.gpu_index", gpus,
        "--PatchMatchStereo.geom_consistency", "1",
        "--PatchMatchStereo.cache_size", cacheSize], stdout=subprocess.PIPE)     

    get_eta_after_step = init_eta_calculation()
    current_image_number = 0
    while p.poll() is None:
        output = p.stdout.readline()
        if output != b"":
            line = output.strip().decode("utf-8")
            print(line)   
            # Scanning COLMAP stdout for progress
            if "Processing view" in line:   
                current_images = line[16:].split(" / ")
                if len(current_images) == 2:                
                    _, number_of_images = current_images
                    current_image_number += 1
                    number_of_images = int(number_of_images.split(" ")[0])
                    eta = get_eta_after_step(number_of_images * 2)
                    progressCallback(round(5 + 65 * current_image_number / (number_of_images * 2)), eta=eta, step=2)

    if p.poll() != 0:
        return False 

    progressCallback(70, force_Write=True)

    fused_model_path = os.path.join(colmapProjectDirPath, "02_dense", "fused_model")
    if not os.path.exists(fused_model_path):
        os.makedirs(fused_model_path)

    # run stereo fusion
    print('\t> Running stereo fusion...')
    p = subprocess.Popen([COLMAP_BIN, 
        "stereo_fusion", 
        "--workspace_path", os.path.join(colmapProjectDirPath, "02_dense"),
        "--workspace_format", "COLMAP",
        "--input_type", "geometric",
        "--output_path", os.path.join(colmapProjectDirPath, "02_dense", "fused_model", sequenceName + "_dense_cloud.ply"),
        "--StereoFusion.cache_size", cacheSize], stdout=subprocess.PIPE)     

    get_eta_after_step = init_eta_calculation()
    while p.poll() is None:
        output = p.stdout.readline()
        if output != b"":
            line = output.strip().decode("utf-8")
            print(line)   
            # Scanning COLMAP stdout for progress
            if "Fusing image" in line:   
                current_images = line[14:].split("]")[0].split("/")
                if len(current_images) == 2:                
                    current_image_number, number_of_images = current_images
                    current_image_number = int(current_image_number)
                    number_of_images = int(number_of_images)
                    eta = get_eta_after_step(number_of_images)
                    progressCallback(round(70 + 25 * current_image_number / number_of_images), eta=eta, step=3)

    if p.poll() != 0:
        return False 

    progressCallback(95, force_Write=True)

    # run georegistration
    dense_in_path = os.path.join(colmapProjectDirPath, "02_dense", "geo", "dense_in")
    if not os.path.exists(dense_in_path):
        os.makedirs(dense_in_path)

    dense_out_path = os.path.join(colmapProjectDirPath, "02_dense", "geo", "dense_out")
    if not os.path.exists(dense_out_path):
        os.makedirs(dense_out_path)

    # copy data
    copytree(os.path.join(colmapProjectDirPath, "02_dense", "fused_model"), os.path.join(colmapProjectDirPath, "02_dense", "geo", "dense_in"), dirs_exist_ok=True) 
    copytree(os.path.join(colmapProjectDirPath, "02_dense", "sparse"), os.path.join(colmapProjectDirPath, "02_dense", "geo", "dense_in"), dirs_exist_ok=True) 

    # compute reference informaion
    success = computeGpsRefernceList(colmapProjectDirPath, sequenceName, os.path.join(colmapProjectDirPath, "02_dense", "geo"))

    # running model aligner if computeGpsRefernceList was successfull
    if success:
        # run model aligner
        print('\t> Running model aligner...')
        cmd=('{} model_aligner '
            '--input_path ' + dense_in_path + " ",
            '--output_path '+ dense_out_path + " ",
            '--ref_images_path ' + os.path.join(colmapProjectDirPath, "02_dense", "geo", "ImgGpsList_ecef.txt") + " ", 
            '--robust_alignment 0').format(COLMAP_BIN)
        os.system(cmd)

        # run model_converter fusion
        print('\t> Running model converter...')
        cmd=('{} model_converter '
            '--input_path ' + dense_out_path + " ",
            '--output_path ' + os.path.join(colmapProjectDirPath, "02_dense", sequenceName+"_dense_cloud.ply") + " ",
            '--output_type PLY').format(COLMAP_BIN)
        os.system(cmd)
    else:
        shutil.copy(os.path.join(colmapProjectDirPath, "02_dense", "fused_model", sequenceName+"_dense_cloud.ply"), 
            os.path.join(colmapProjectDirPath, "02_dense", sequenceName+"_dense_cloud.ply")) 


    # copy result
    shutil.copy(os.path.join(colmapProjectDirPath, "02_dense", sequenceName+"_dense_cloud.ply"), projectOutputDirPath) 
    shutil.copy(os.path.join(colmapProjectDirPath, "02_dense", "geo", "GpsEcefOffset.txt"), projectOutputDirPath) 

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
         '--input_path ' + os.path.join(colmapProjectDirPath, "02_dense", sequenceName + "_dense_cloud.ply") + " ",
         '--output_path ' + os.path.join(colmapProjectDirPath, "02_dense", sequenceName + "_meshed_model.ply") + " "
         ).format(COLMAP_BIN)
    os.system(cmd)

    # copy result
    shutil.copy(os.path.join(colmapProjectDirPath, "02_dense", sequenceName+"_meshed_model.ply"), projectOutputDirPath)  

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
    Path(yamlLockFilePath).touch()

    with open(yamlFilePath, 'r') as iStream:
        data = iStream.read()
        try:
            yamlObj = yaml.safe_load(data[len(OPENCV_YAML_HEADER):]) # handle opencv header
        except yaml.YAMLError as exc:
            print(exc)

    # remove lock file
    os.remove(yamlLockFilePath)

    return yamlObj

######################################################################################################################
# Write yml file
def writeYaml(yamlFilePath: str, yamlObj):
    yamlLockFilePath = yamlFilePath + ".lock"

    # while lock file esists sleep
    while(os.path.exists(yamlLockFilePath)):
        time.sleep(0.1)

    # create lock file
    Path(yamlLockFilePath).touch()

    with open(yamlFilePath, 'w') as oStream:
        oStream.write(OPENCV_YAML_HEADER)  # handle opencv header
        try:
            yaml.dump(yamlObj, oStream, Dumper=OpenCvYamlDumper)
        except yaml.YAMLError as exc:
            print(exc)

    # remove lock file
    os.remove(yamlLockFilePath)

######################################################################################################################
# Process given job
# Returns: True if job is processed. False otherwise
def processJob(workspacePath: str, currentJob: Job, robust_mode: bool) -> bool:
    print("-- Processing Job ...")

    colmapDatabaseFilePath = os.path.join(workspacePath , currentJob.sequenceName + ".db")
    projectImageDir = os.path.join(workspacePath, currentJob.sequenceName + ".images")
    colmapProjectDirPath = os.path.join(workspacePath , currentJob.sequenceName + ".files")
    projectOutputDirPath = os.path.join(workspacePath , currentJob.sequenceName + ".output")

    if not os.path.exists(projectImageDir):
        os.makedirs(projectImageDir)

    sparse_path = os.path.join(colmapProjectDirPath, "01_sparse")
    if not os.path.exists(sparse_path):
        os.makedirs(sparse_path)

    dense_path = os.path.join(colmapProjectDirPath, "02_dense")
    if not os.path.exists(dense_path):
        os.makedirs(dense_path)
    
    if not os.path.exists(projectOutputDirPath):
        os.makedirs(projectOutputDirPath)

    # if current product type of job is CAMERA_POSES handle creation of project
    if( currentJob.getProductTypeStr() == 'CAMERA_POSES' ):

        # if image path in job description is not in project directory copy into project directory
        if( projectImageDir != currentJob.parameterList["image_path"] ):
            print("Images are outside of project dir. Copying {} -> {}".format(currentJob.parameterList["image_path"], projectImageDir))
            
            shutil.rmtree(projectImageDir)
            os.makedirs(projectImageDir)
            copytree(os.path.join(currentJob.parameterList["image_path"]), projectImageDir, dirs_exist_ok=True)  

        # end if projectImageDir != image_path

        imgList = listImgFiles(projectImageDir)
        if( not os.path.isdir(projectImageDir) or len(imgList) == 0):
            print("Image directory {} does not exist or is empty.".format(projectImageDir))
            return False

        # compute camera poses
        success = computeCameraPoses(colmapDatabaseFilePath, projectImageDir, colmapProjectDirPath, projectOutputDirPath, currentJob.sequenceName, currentJob.parameterList, robust_mode)

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
# Returns: True and Object of Job Entry if queue has job. False and empty entry othewise.
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
    writeYaml(yamlFilePath , yamlObj)

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
def progressCallback(progress: float, force_Write = False, step=1, eta=0):
    global LAST_PROGRESS_UPDATE_TIME
    global WORKER_STATE_YAML_PATH

    # if lock file exist return. no need to wait until finished
    if(os.path.exists(WORKER_STATE_YAML_PATH + ".lock")):
        return

    if (force_Write or (time.time() - LAST_PROGRESS_UPDATE_TIME) > YAML_REFRESH_SECS):

        yamlObj = loadYaml(WORKER_STATE_YAML_PATH)
        if yamlObj is None:
            return

        if( len(yamlObj["runningJob"]) == 0 ):
            return

        # store global progress in yaml file
        yamlJobEntry = yamlObj["runningJob"][0]
        yamlJobEntry["progress"] = progress
        yamlJobEntry["step"] = step
        yamlJobEntry["eta"] = eta

        writeYaml(WORKER_STATE_YAML_PATH, yamlObj)

        LAST_PROGRESS_UPDATE_TIME = time.time()

######################################################################################################################
# Method to set failed state to failed job
def setFailedStateToJob():
    global WORKER_STATE_YAML_PATH

    yamlObj = loadYaml(WORKER_STATE_YAML_PATH)
    if yamlObj is None:
        return

    yamlJobEntry = yamlObj["runningJob"][0]
    yamlJobEntry["jobState"] = 3

    writeYaml(WORKER_STATE_YAML_PATH, yamlObj)


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
    parser.add_argument("--robust_mode", default=0, help="robust mode tries to reconstruct 3d model " 
        "with fixed distortion params and then tries to estimate distorsion params with bundle " 
        "adjustment after sparse mapping (usefull for difficult images with high focal length or poor quality). ")
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
    ROBUST_MODE = bool(int(args.robust_mode))

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
        try:
            success = processJob(workspacePath, currentJob, ROBUST_MODE)
            if(not success):
                raise Exception("ERROR: Something went wrong when trying to process job {}".format(currentJob))

        except Exception as e:             
                # write failed job state to file and exit
                print("ERROR: Something went wrong when trying to process job {}".format(currentJob))
                setFailedStateToJob()
                raise e


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
