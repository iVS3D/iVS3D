import re
import pymap3d as pm

#---------------------------------------------------------------------------------------------------------------------
# Class representing Gps Entry
class GpsEntry:
    def __init__(self, latitude, longitude, altitude):
        self.lat = latitude
        self.long = longitude
        self.alt = altitude

        self.isValid = True

    def __str__(self):
        return 'GPS lat: {}, long: {}, alt: {}'.format(self.lat, self.long, self.alt)

    ######################################################################################################################
    def _get_if_exist(self, data, key):
        if key in data:
            return data[key]

        return None

    ######################################################################################################################
    # Method to convert gps coordinate from DMS (Degree, Minutes, Seconds) to decimal
    # based on: https://gist.github.com/tomwhipple/3601130
    def _dms_to_decimal(self, coord_tuple: list, coord_ref: str):

        # compute sign
        sign = 1
        if re.match('[swSW]', str(coord_ref)):
            sign = -1

        deg = int(str(coord_tuple.values[0]))
        min = float(str(coord_tuple.values[1]))
        sec = float(coord_tuple.values[2].num) / float(coord_tuple.values[2].den)

        #split second into integer and decimal
        sec_i, sec_d = divmod(sec,1)

        # compute decimal value
        return sign * deg + min / 60 + float(sec_i) / 3600 + float(sec_d) / 36000

    def readFromExif(self, exifData):
        lat_tuple = self._get_if_exist(exifData, 'GPS GPSLatitude')
        lat_ref = self._get_if_exist(exifData, 'GPS GPSLatitudeRef')
        long_tuple = self._get_if_exist(exifData, 'GPS GPSLongitude')
        long_ref = self._get_if_exist(exifData, 'GPS GPSLongitudeRef')
        alt_frac = self._get_if_exist(exifData, 'GPS GPSAltitude')

        if lat_tuple and lat_ref:
            self.lat = self._dms_to_decimal(lat_tuple, lat_ref)
        else:
            self.isValid = False

        if long_tuple and long_ref:
            self.long = self._dms_to_decimal(long_tuple, long_ref)
        else:
            self.isValid = False

        if alt_frac:
            self.alt = float(alt_frac.values[0].num) / float(alt_frac.values[0].den)
        else:
            self.isValid = False

    # return coordinates as WGS84 ECEF with unit length of m
    def toWGS84Ecef(self): 
        return pm.geodetic2ecef(self.lat, self.long, self.alt)

    # create entry from WGS84 ECEF with unit length of m
    def fromWGS84ECEF(self, ECEF_X, ECEF_Y, ECEF_Z):
        self.long, self.lat, self.alt = pm.ecef2geodetic(ECEF_X, ECEF_Y, ECEF_Z)
#---------------------------------------------------------------------------------------------------------------------
