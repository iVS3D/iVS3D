import time
import os
import sys
import argparse
import re
from os import walk
import exifread
import numpy as np

from GpsEntry import GpsEntry

STDOUT_COLOR_RED='\u001b[31m'
STDOUT_COLOR_GREEN='\u001b[32m'
STDOUT_COLOR_YELLOW='\u001b[33m'
STDOUT_COLOR_RESET='\u001b[0m'

GEO_ENU2ECEF_FILE='GpsEnu2Ecef.txt'
IMAGE_GEO_ECEF_FILE='ImgGpsList_ecef.txt'
IMAGE_GEO_ENU_FILE='ImgGpsList_enu.txt'

def avgEcef(ecefList):
    N = float(len(ecefList))
    return (sum(t[0] for t in ecefList)/N,
            sum(t[1] for t in ecefList)/N,
            sum(t[2] for t in ecefList)/N)

######################################################################################################################
# Method to compute list of gps reference data, needed for georegistration
def computeGpsRefernceList(rootDir: str, imgList: list, outputDir: str):

    if not os.path.exists(outputDir):
        os.system(f'mkdir -p {outputDir}')

    # open gps output file
    fout_ecef = open(os.path.join(outputDir, IMAGE_GEO_ECEF_FILE) , 'w+')    
    fout_enu = open(os.path.join(outputDir, IMAGE_GEO_ENU_FILE) , 'w+') 
    fout_enu2ecef = open(os.path.join(outputDir, GEO_ENU2ECEF_FILE) , 'w+') 

    # write header to files
    fout_ecef.write(f'# FILENAME ECEF_X ECEF_Y ECEF_Z\r\n')
    fout_ecef.flush()
    fout_enu.write(f'# FILENAME ENU_X ENU_Y ENU_Z\r\n')
    fout_enu.flush()
    fout_enu2ecef.write(f'# 4x4 ROW-MAJOR ENU TO ECEF TRANSFORMATION MATRIX \r\n')
    fout_enu2ecef.flush()

    # loop over entries in imglist and create valid file and gps list
    validImgList = []
    validGpsEcefList = []
    for imgFilename in imgList:
        imgFilePath = os.path.join(rootDir,imgFilename)

        # check if path exists
        if(os.path.exists(imgFilePath)):

            # Open image file for reading (binary mode) and read exif tags
            imgFile = open(imgFilePath, 'rb')
            tags = exifread.process_file(imgFile)

            # create gps entry and read from exif
            gpsEntry = GpsEntry(0,0,0)
            gpsEntry.readFromExif(tags)

            # if gps is valid, process
            if(gpsEntry.isValid):
                validImgList.append(imgFilename)
                validGpsEcefList.append(gpsEntry.toWGS84Ecef())

    # compute reference ecef at altitude 0
    averageEcef = avgEcef(validGpsEcefList)
    referenceGps = GpsEntry(0,0,0)
    referenceGps.fromWGS84ECEF(averageEcef[0], averageEcef[1], averageEcef[2])
    referenceGps.alt = 0

    # get and write transfromation matrix between ENU <-> ECEF
    # M_enu2ecef = [R_enu   C_enu], R_enu = (r_x r_y r_z)
    # M_ecef2enu = [R_enu^T   -R_enu^T C_enu]
    ecef_ref_x, ecef_ref_y, ecef_ref_z = referenceGps.toWGS84Ecef()
    C_enu = np.array([ecef_ref_x, ecef_ref_y, ecef_ref_z])
    r_z = C_enu / np.linalg.norm(C_enu)
    r_z = r_z / np.linalg.norm(r_z)
    r_x = np.cross(np.array([0,0,1]), r_z)
    r_x = r_x / np.linalg.norm(r_x)
    r_y = np.cross(r_z, r_x)
    r_y = r_y / np.linalg.norm(r_y)
    R_enu=np.array([r_x, r_y, r_z]).T
    M_enu2ecef = np.vstack((np.c_[R_enu, C_enu], [0,0,0,1]))
    M_ecef2enu = np.vstack((np.c_[R_enu.T, - np.dot(R_enu.T,C_enu)], [0,0,0,1]))

    for index, x in np.ndenumerate(M_enu2ecef):
      fout_enu2ecef.write(f'{x} ')
    fout_enu2ecef.write(f'\r\n')
    fout_enu2ecef.flush()

    # loop over all valid images and gps and process
    i = 1
    for img, ecef in zip(validImgList, validGpsEcefList):       
        print(f'\t> {int(float(i / len(validImgList)) * 100)} %\t\r', end='',flush=True) 
       
        # write image and ecef gps data to file    
        enu = np.dot(M_ecef2enu, np.array([ecef[0], ecef[1], ecef[2], 1.0]))  
        fout_ecef.write(f'{img} {ecef[0]:.3f} {ecef[1]:.3f} {ecef[2]:.3f}\r\n')
        fout_ecef.flush()
        fout_enu.write(f'{img} {enu[0]:.3f} {enu[1]:.3f} {enu[2]:.3f}\r\n')
        fout_enu.flush()

        i += 1
    # end for img, ecef      
    print() 

    # close gps output file
    fout_ecef.close()
    fout_enu.close()
    fout_enu2ecef.close()

###############################################################################
# Initialize argument parser and synopsis
# return list of arguments
def parseArguments():
    descriptionTxt='''\
    Python script to extract gps from exif data of passed image list and compute reference list for model alignemnt with COLMAP.
    '''

    parser = argparse.ArgumentParser(description=descriptionTxt)
    parser.add_argument('--img_list', nargs=1, metavar=('file-path'), help="List of images from which the gps data is to be extracted.")
    parser.add_argument('--img_dir', nargs=1, metavar=('dir-path'), help="Path to root directory from w.r.t. which the files in img_list are given.")

    return parser, parser.parse_args()


######################################################################################################################
if __name__ == "__main__":

    print("---------------------------------------------------------")

    # init parsing of call arguments
    argParser, args = parseArguments()

    if (args.img_list is None or args.img_dir is None):
        print('ERROR: Not enough arguments!')
        argParser.print_help()
        sys.exit()

    # read image list from file
    with open(args.img_list[0]) as f:
        imgList = [line.rstrip() for line in f]

    # compute output path
    outputPath = os.path.dirname(args.img_list[0])
    if(outputPath == ""):
        outputPath = os.getcwd()

    print(f' {STDOUT_COLOR_YELLOW}[....]{STDOUT_COLOR_RESET} Processing {args.img_list[0]} w.r.t {args.img_dir[0]} into {outputPath}')

    # compute reference list
    computeGpsRefernceList(args.img_dir[0], imgList, outputPath)

    print(f'\r {STDOUT_COLOR_GREEN}[DONE]{STDOUT_COLOR_RESET}')

    print("---------------------------------------------------------")
