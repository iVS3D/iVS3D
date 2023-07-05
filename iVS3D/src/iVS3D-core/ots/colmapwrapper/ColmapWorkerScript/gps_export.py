import time
import os
import sys
import argparse
import subprocess
import shutil
import traceback
import pymap3d as pm

from pathlib import Path

from GpsEntry import GpsEntry
import yaml 
import exifread


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
# Method to compute list of gps reference data, needed for georegistration, returns false if no valid gps reference data found
def computeGpsRefernceList():

    geoDir = "/home/max/AK-3D/data/AVEAS_FULL/geo_dir/"
    # open gps output file
    fout_ecef = open(os.path.join(geoDir, "ImgGpsList_ecef.txt") , 'w+')
    fout_offset = open(os.path.join(geoDir, "GpsEcefOffset.txt"), 'w+')
    fout_wgs84 = open(os.path.join(geoDir, "ImgGpsList_wgs84.txt"), 'w+')
    fout_enu = open(os.path.join(geoDir, "ImgGpsList_enu.txt") , 'w+')
    fout_enu_observer = open(os.path.join(geoDir, "ImgGpsList_enu_oberserver.txt") , 'w+')

    # write header to files
    fout_ecef.write('# FILENAME ECEF_X ECEF_Y ECEF_Z\r\n')
    fout_ecef.flush()
    fout_offset.write('# OFFSET_X OFFSET_Y OFFSET_Z\r\n')
    fout_offset.flush()
    fout_wgs84.write('# FILENAME LAT LONG ALT\r\n')
    fout_wgs84.flush()
    fout_enu.write('# FILENAME ENU_X ENU_Y ENU_Z\r\n')
    fout_enu.flush()
    fout_enu_observer.write('# OFFSET_X OFFSET_Y OFFSET_Z\r\n')
    fout_enu_observer.flush()

    # loop over image files
    import glob
    fileList = glob.glob("/home/max/AK-3D/data/AVEAS_FULL/AVEAS_FULL.images//*.jpg")
    fileList.sort()
    i = 0
    #sum of all gps entries to detect empty gps data
    gps_sum = 0
    ecef_offset_x, ecef_offset_y, ecef_offset_z = 0, 0, 0
    lat, lon, alt = 0, 0, 0
    
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

        ecef_x, ecef_y, ecef_z = gpsEntry.toWGS84Ecef()
        fout_ecef.write('{} {:.3f} {:.3f} {:.3f}\r\n'.format(os.path.basename(imgFilename), (ecef_x - ecef_offset_x), (ecef_y - ecef_offset_y), (ecef_z - ecef_offset_z)))
        gps_sum += ecef_x - ecef_offset_x + ecef_y - ecef_offset_y + ecef_z - ecef_offset_z

        fout_ecef.flush()
        fout_wgs84.write('{} {} {} {}\r\n'.format(os.path.basename(imgFilename), gpsEntry.lat, gpsEntry.long, gpsEntry.alt))
        fout_wgs84.flush()

        lat += gpsEntry.lat
        lon += gpsEntry.long
        alt += gpsEntry.alt
        print(gpsEntry.alt, imgFilename)


        i+=1
    
    lat /= i
    lon /= i
    alt /= i



    gpsEntry = GpsEntry(lat, lon, alt)
    ecef_offset_x, ecef_offset_y, ecef_offset_z = gpsEntry.toWGS84Ecef()   
    fout_enu_observer.write('{:.3f} {:.3f} {:.3f}\r\n'.format(ecef_offset_x, ecef_offset_y, ecef_offset_z))
    fout_enu_observer.flush()

    for imgFilename in fileList:

        # Open image file for reading (binary mode) and read exif tags
        imgFile = open(imgFilename, 'rb')
        tags = exifread.process_file(imgFile)

        # create gps entry and read from exif
        gpsEntry = GpsEntry(0,0,0)
        gpsEntry.readFromExif(tags)

        enu_x, enu_y, enu_z = pm.geodetic2enu(gpsEntry.lat, gpsEntry.long, gpsEntry.alt, lat, lon, alt)
        fout_enu.write('{} {:.3f} {:.3f} {:.3f}\r\n'.format(os.path.basename(imgFilename), enu_x, enu_y, enu_z))
        fout_enu.flush()  

    fout_ecef.close()
    fout_offset.close()
    fout_wgs84.close()
    fout_enu.close()
    fout_enu_observer.close()

    return int(gps_sum) != 0


def main():
    success = computeGpsRefernceList()
    print(success)


main()