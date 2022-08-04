import os
import sys
import argparse
import re
from os import walk

###############################################################################
# Initialize argument parser and synopsis
# return list of arguments
def parseArguments():
    descriptionTxt='''\
    Python script to reduce iamge data used for patch match stereo.
    '''

    parser = argparse.ArgumentParser(description=descriptionTxt)
    parser.add_argument('--dir', nargs=1, metavar=('dir-path'), help="Path to dense stereo directory holding patch-match.cfg.")
    parser.add_argument('--freq', nargs=1, metavar=('every-nth-img'), help="Keep every nth image specified.")
    parser.add_argument('--exclude_prefix', nargs=1, metavar=('prefix'), help="Prefix of image names that are to be excluded.")

    return parser, parser.parse_args()


######################################################################################################################
if __name__ == "__main__":

    print("---------------------------------------------------------")

    # init parsing of call arguments
    argParser, args = parseArguments()

    # check if enough arguments are given
    if (args.dir is None and args.freq is None):
        print('ERROR: Not enough arguments! --dir and --freq are mandetory!')
        argParser.print_help()
        sys.exit()

    # check that freq is larger than 1
    freq = int(args.freq[0])
    if(freq <= 1):
      print(f'Frequency must be larger than 1!')
      sys.exit()

    # check that patch-match.cfg exists
    patchMatchConfigPath = os.path.join(args.dir[0], "patch-match.cfg")
    fusionConfigPath = os.path.join(args.dir[0], "fusion.cfg")
    if not os.path.exists(patchMatchConfigPath):
      print(f'Config file {patchMatchConfigPath} does not exist!')
      sys.exit()

    # backup config file
    patchMatchConfigPathOrig = patchMatchConfigPath + ".orig"
    fusionConfigPathOrig = fusionConfigPath + ".orig"
    if not os.path.exists(patchMatchConfigPathOrig):
      os.system(f'cp {patchMatchConfigPath} {patchMatchConfigPathOrig}')
    if not os.path.exists(fusionConfigPathOrig):
      os.system(f'cp {fusionConfigPath} {fusionConfigPathOrig}')

    # process file
    prefix = args.exclude_prefix[0]
    os.system(f'rm {patchMatchConfigPath} && touch {patchMatchConfigPath}')
    f_in_patchMatch = open(patchMatchConfigPathOrig, "r")
    f_out_patchMatch = open(patchMatchConfigPath, "w")
    f_in_fusion = open(fusionConfigPathOrig, "r")
    f_out_fusion = open(fusionConfigPath, "w")
    i = 0
    while True:
      # read lines
      line_in1 = f_in_patchMatch.readline()
      line_in2 = f_in_patchMatch.readline()
      line_in_fusion = f_in_fusion.readline()

      # check for EOF
      if not line_in1 or not line_in2:
        break

      # check if prefix is provided and is in line
      if (prefix != "" and line_in1.find(prefix) != -1):
        f_out_patchMatch.write(line_in1)
        f_out_patchMatch.write(line_in2)
        f_out_fusion.write(line_in_fusion)

      # check if prefix is provided and not in line or prefix is not provided
      elif ((prefix != "" and line_in1.find(prefix) == -1) or prefix == ""):
        
        # if ith line write to output else skip
        if((i % freq) == 0):
          f_out_patchMatch.write(line_in1)
          f_out_patchMatch.write(line_in2)
          f_out_fusion.write(line_in_fusion)

        i += 1

    f_in_patchMatch.close()
    f_out_patchMatch.close()
    f_in_fusion.close()
    f_out_fusion.close()

      
    