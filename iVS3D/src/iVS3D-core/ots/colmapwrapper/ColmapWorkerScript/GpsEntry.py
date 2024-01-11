import re
from ellipsoid import Ellipsoid
from math import sin, cos, sqrt, radians, tan, atan, hypot, degrees, atan2, pi

def power(x, y):  # type: ignore
    return pow(x, y)

def sign(x) -> float:  # type: ignore
    """signum function"""
    if x < 0:
        y = -1.0
    elif x > 0:
        y = 1.0
    else:
        y = 0.0

    return y

def cbrt(x) -> float:  # type: ignore
    """math.cbrt was added in Python 3.11"""
    return x ** (1 / 3)

def sanitize(lat, ell: Ellipsoid | None, deg: bool) -> tuple:

    if ell is None:
        ell = Ellipsoid()

    try:
        lat = asarray(lat)
    except NameError:
        pass

    if deg:
        lat = radians(lat)

    try:
        if (abs(lat) > pi / 2).any():  # type: ignore
            raise ValueError("-pi/2 <= latitude <= pi/2")
    except AttributeError:
        if abs(lat) > pi / 2:  # type: ignore
            raise ValueError("-pi/2 <= latitude <= pi/2")

    return lat, ell

def geodetic2ecef(
    lat,
    lon,
    alt,
    ell: Ellipsoid = None,
    deg: bool = True,
) -> tuple:
    """
    point transformation from Geodetic of specified ellipsoid (default WGS-84) to ECEF

    Parameters
    ----------

    lat
           target geodetic latitude
    lon
           target geodetic longitude
    h
         target altitude above geodetic ellipsoid (meters)
    ell : Ellipsoid, optional
          reference ellipsoid
    deg : bool, optional
          degrees input/output  (False: radians in/out)


    Returns
    -------

    ECEF (Earth centered, Earth fixed)  x,y,z

    x
        target x ECEF coordinate (meters)
    y
        target y ECEF coordinate (meters)
    z
        target z ECEF coordinate (meters)
    """
    lat, ell = sanitize(lat, ell, deg)
    if deg:
        lon = radians(lon)

    # radius of curvature of the prime vertical section
    N = ell.semimajor_axis**2 / sqrt(
        ell.semimajor_axis**2 * cos(lat) ** 2 + ell.semiminor_axis**2 * sin(lat) ** 2
    )
    # Compute cartesian (geocentric) coordinates given (curvilinear) geodetic coordinates.
    x = (N + alt) * cos(lat) * cos(lon)
    y = (N + alt) * cos(lat) * sin(lon)
    z = (N * (ell.semiminor_axis / ell.semimajor_axis) ** 2 + alt) * sin(lat)

    return x, y, z


def ecef2geodetic(
    x,
    y,
    z,
    ell: Ellipsoid = None,
    deg: bool = True,
) -> tuple:
    """
    convert ECEF (meters) to geodetic coordinates

    Parameters
    ----------
    x
        target x ECEF coordinate (meters)
    y
        target y ECEF coordinate (meters)
    z
        target z ECEF coordinate (meters)
    ell : Ellipsoid, optional
          reference ellipsoid
    deg : bool, optional
          degrees input/output  (False: radians in/out)

    Returns
    -------
    lat
           target geodetic latitude
    lon
           target geodetic longitude
    alt
         target altitude above geodetic ellipsoid (meters)

    based on:
    You, Rey-Jer. (2000). Transformation of Cartesian to Geodetic Coordinates without Iterations.
    Journal of Surveying Engineering. doi: 10.1061/(ASCE)0733-9453
    """

    if ell is None:
        ell = Ellipsoid()

    try:
        x = asarray(x)
        y = asarray(y)
        z = asarray(z)
    except NameError:
        pass

    r = sqrt(x**2 + y**2 + z**2)

    E = sqrt(ell.semimajor_axis**2 - ell.semiminor_axis**2)

    # eqn. 4a
    u = sqrt(0.5 * (r**2 - E**2) + 0.5 * sqrt((r**2 - E**2) ** 2 + 4 * E**2 * z**2))

    Q = hypot(x, y)

    huE = hypot(u, E)

    # eqn. 4b
    try:
        Beta = atan(huE / u * z / hypot(x, y))
    except ZeroDivisionError:
        if z >= 0:
            Beta = pi / 2
        else:
            Beta = -pi / 2

    # eqn. 13
    dBeta = ((ell.semiminor_axis * u - ell.semimajor_axis * huE + E**2) * sin(Beta)) / (
        ell.semimajor_axis * huE * 1 / cos(Beta) - E**2 * cos(Beta)
    )

    Beta += dBeta

    # eqn. 4c
    # %% final output
    lat = atan(ell.semimajor_axis / ell.semiminor_axis * tan(Beta))

    try:
        # patch latitude for float32 precision loss
        lim_pi2 = pi / 2 - finfo(dBeta.dtype).eps
        lat = where(Beta >= lim_pi2, pi / 2, lat)
        lat = where(Beta <= -lim_pi2, -pi / 2, lat)
    except NameError:
        pass

    lon = atan2(y, x)

    # eqn. 7
    cosBeta = cos(Beta)
    try:
        # patch altitude for float32 precision loss
        cosBeta = where(Beta >= lim_pi2, 0, cosBeta)
        cosBeta = where(Beta <= -lim_pi2, 0, cosBeta)
    except NameError:
        pass

    alt = hypot(z - ell.semiminor_axis * sin(Beta), Q - ell.semimajor_axis * cosBeta)

    # inside ellipsoid?
    inside = (
        x**2 / ell.semimajor_axis**2
        + y**2 / ell.semimajor_axis**2
        + z**2 / ell.semiminor_axis**2
        < 1
    )

    try:
        if inside.any():  # type: ignore
            # avoid all false assignment bug
            alt[inside] = -alt[inside]
    except (TypeError, AttributeError):
        if inside:
            alt = -alt

    if deg:
        lat = degrees(lat)
        lon = degrees(lon)

    return lat, lon, alt

def uvw2enu(u, v, w, lat0, lon0, deg: bool = True) -> tuple:
    """
    Parameters
    ----------

    u
    v
    w


    Results
    -------

    East
        target east ENU coordinate (meters)
    North
        target north ENU coordinate (meters)
    Up
        target up ENU coordinate (meters)
    """
    if deg:
        lat0 = radians(lat0)
        lon0 = radians(lon0)

    t = cos(lon0) * u + sin(lon0) * v
    East = -sin(lon0) * u + cos(lon0) * v
    Up = cos(lat0) * t + sin(lat0) * w
    North = -sin(lat0) * t + cos(lat0) * w

    return East, North, Up

def geodetic2enu(
    lat,
    lon,
    h,
    lat0,
    lon0,
    h0,
    ell: Ellipsoid = None,
    deg: bool = True,
) -> tuple:
    """
    Parameters
    ----------
    lat : float
          target geodetic latitude
    lon : float
          target geodetic longitude
    h : float
          target altitude above ellipsoid  (meters)
    lat0 : float
           Observer geodetic latitude
    lon0 : float
           Observer geodetic longitude
    h0 : float
         observer altitude above geodetic ellipsoid (meters)
    ell : Ellipsoid, optional
          reference ellipsoid
    deg : bool, optional
          degrees input/output  (False: radians in/out)


    Results
    -------
    e : float
        East ENU
    n : float
        North ENU
    u : float
        Up ENU
    """
    x1, y1, z1 = geodetic2ecef(lat, lon, h, ell, deg=deg)
    x2, y2, z2 = geodetic2ecef(lat0, lon0, h0, ell, deg=deg)

    return uvw2enu(x1 - x2, y1 - y2, z1 - z2, lat0, lon0, deg=deg)

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
        if re.search('[swSW]', str(coord_ref)):
            sign = -1

        degree = int(str(coord_tuple.values[0]))
        minute = float(str(coord_tuple.values[1]))
        second = float(coord_tuple.values[2].num) / float(coord_tuple.values[2].den)

        return sign * (int(degree) + float(minute) / 60 + float(second) / 3600)

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
        return geodetic2ecef(self.lat, self.long, self.alt)

    # create entry from WGS84 ECEF with unit length of m
    def fromWGS84ECEF(self, ECEF_X, ECEF_Y, ECEF_Z):
        self.long, self.lat, self.alt = ecef2geodetic(ECEF_X, ECEF_Y, ECEF_Z)
#---------------------------------------------------------------------------------------------------------------------
