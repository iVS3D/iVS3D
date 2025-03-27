import time
import os
import sys
import argparse
import subprocess
import shutil
import traceback
import glob
from pathlib import Path
import random

def poll_process_output(p):
    while p.poll() is None:
        output = p.stdout.readline()
        try:
            if output != b"":
                line = output.strip().decode("utf-8")   

                if "Finished reading images" in line:
                    print("iVS3D_PROGRESS", 1, 1, 0)  
                elif "Matching" in line:
                    print("iVS3D_PROGRESS", 5, 2, 0)                
                elif "Finished opensfm stage" in line:
                    print("iVS3D_PROGRESS", 20, 3, 0)
                elif "Finished openmvs stage" in line:
                    print("iVS3D_PROGRESS", 40, 4, 0)
                elif "Finished odm_filterpoints stage" in line:
                    print("iVS3D_PROGRESS", 50, 5, 0)
                elif "Finished odm_meshing stage" in line:
                    print("iVS3D_PROGRESS", 60, 6, 0)
                elif "Finished mvs_texturing stage" in line:
                    print("iVS3D_PROGRESS", 65, 7, 0)
                elif "Finished odm_georeferencing stage" in line:
                    print("iVS3D_PROGRESS", 70, 8, 0)
                elif "Finished odm_dem stage" in line:
                    print("iVS3D_PROGRESS", 80, 9, 0)
                elif "Finished odm_orthophoto stage" in line:
                    print("iVS3D_PROGRESS", 90, 10, 0)
                elif "ODM app finished" in line:
                    print("iVS3D_PROGRESS", 99, 11, 0)    

                print(line)
            
        except Exception as e:            
            print("Failed to parse line!")
            traceback.print_exc()


def run_odm(input_dir, work_dir, output_dir, camera_model, quality, gpus):

    quality = int(quality)
    gpus = gpus.replace("-1","all")
    os.system("mv "+str(input_dir) + " " + str(os.path.join(work_dir, "images")))
    
    args = ""

    if quality == 0:
        args + " --pc-quality low"
    elif quality == 2:
        args + " --pc-quality high"
    elif quality == 3:
        args + " --pc-quality ultra"

    if quality >=2:
        args + " --mesh-size 1000000"
        args + " --mesh-octree-depth 12"

    command = """
    /bin/bash << "EOF"
        podman run --rm -it --entrypoint bash --device nvidia.com/gpu={gpus} -v {work_dir}:/datasets opendronemap/odm:gpu      
  
        ./run.sh --project-path /datasets --texturing-single-material --dsm ./{args}
        python3 SuperBuild/install/bin/opensfm/bin/opensfm_main.py export_colmap /datasets/opensfm/

        exit
    EOF
    exit
   
    """.replace("{work_dir}", str(work_dir)).replace("{gpus}", gpus).replace("{args}", args)  

    p = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
    poll_process_output(p)

    os.system("cp " + str(os.path.join(work_dir, "odm_filterpoints/point_cloud.ply")) + " " + str(os.path.join(output_dir, "point_cloud.ply")))
    os.system("cp " + str(os.path.join(work_dir, "odm_texturing/*")) + " " + str(output_dir))
    os.system("cp " + str(os.path.join(work_dir, "odm_orthophoto/odm_orthophoto.tif")) + " " + str(output_dir))
    os.system("cp " + str(os.path.join(work_dir, "odm_dem/dsm.tif")) + " " + str(output_dir))
    os.system("cp " + str(os.path.join(work_dir, "opensfm/reference_lla.json")) + " " + str(output_dir))
    os.system("cp " + str(os.path.join(work_dir, "opensfm/colmap_export/*.txt")) + " " + str(output_dir))
    os.system("cp " + str(os.path.join(work_dir, "opensfm/odm_report/report.pdf")) + " " + str(output_dir))


def parseArguments():
    descriptionTxt='''\
    Example for custom python command to control OpenDroneMap(ODM).
    '''

    parser = argparse.ArgumentParser(description=descriptionTxt)
    parser.add_argument("input_dir", help="Image input path")
    parser.add_argument("work_dir", help="Root of workdir")
    parser.add_argument("output_dir", help="Output path for results")  
    parser.add_argument('--quality', default="0" , help="Quality vs Speed (0-3)")
    parser.add_argument('--gpus', default="0" , help="List of gpu indices to use")
    parser.add_argument('--camera_model', default="RADIAL" , help="Camera model to use")

    return parser.parse_args()

if __name__ == "__main__":

    # init parsing of call arguments
    args = parseArguments()
    print(args)

    input_dir = Path(args.input_dir)
    work_dir = Path(args.work_dir)
    output_dir = Path(args.output_dir)

    run_odm(input_dir, work_dir, output_dir, args.camera_model, quality=args.quality, gpus=args.gpus)



