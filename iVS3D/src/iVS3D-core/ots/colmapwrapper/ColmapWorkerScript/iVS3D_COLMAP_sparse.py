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
                print(line)
            
        except Exception as e:            
            print("Failed to parse line!")
            traceback.print_exc()
            
    # Check the return code
    if p.returncode != 0:
        raise Exception("Command failed")

def create_sparse_with_COLMAP(input_dir, work_dir, output_dir, camera_model, quality, gpus):

    # path to colmap binary
    COLMAP_BIN = "colmap"
    quality = int(quality)
    print("iVS3D_PROGRESS", 0, 0, 0)

    args = [COLMAP_BIN, 
        "feature_extractor", 
        "--database_path", os.path.join(work_dir, "database.db"), 
        "--image_path", input_dir, 
        "--ImageReader.camera_model", camera_model,
        "--ImageReader.single_camera", "1",
        "--SiftExtraction.gpu_index", gpus,
        ]  
    
    if quality >= 2:
        args.append("--SiftExtraction.max_num_features")
        args.append("20000")

    p = subprocess.Popen(args, stdout=subprocess.PIPE)  
    poll_process_output(p)

    print("iVS3D_PROGRESS", 10, 1, 0)

    args = [COLMAP_BIN, 
        "exhaustive_matcher", 
        "--database_path", os.path.join(work_dir, "database.db"), 
        "--ExhaustiveMatching.block_size", "1000", 
        "--SiftMatching.gpu_index", gpus]  
    
    if quality >= 2:
        args.append("--SiftMatching.max_num_matches")
        args.append("40000")
    
    p = subprocess.Popen(args, stdout=subprocess.PIPE)     
    poll_process_output(p)

    print("iVS3D_PROGRESS", 40, 2, 0)

    args = [COLMAP_BIN, 
        "mapper", 
        "--database_path", os.path.join(work_dir, "database.db"), 
        "--image_path", input_dir,  
        "--output_path", os.path.join(work_dir, "01_sparse")]
    
    p = subprocess.Popen(args, stdout=subprocess.PIPE)     
    poll_process_output(p)

    print("iVS3D_PROGRESS", 80, 2, 0)
    args = [COLMAP_BIN, 
        "point_filtering", 
        "--max_reproj_error", "0.5", 
        "--min_tri_angle", "5", 
        "--input_path", os.path.join(work_dir, "01_sparse/0"),
        "--output_path", os.path.join(work_dir, "01_sparse/0")]  
    
    p = subprocess.Popen(args, stdout=subprocess.PIPE)     
    poll_process_output(p)

    args = [COLMAP_BIN, 
        "image_filterer", 
        "--min_num_observations", "20", 
        "--input_path", os.path.join(work_dir, "01_sparse/0"),
        "--output_path", os.path.join(work_dir, "01_sparse/0")]  
    
    p = subprocess.Popen(args, stdout=subprocess.PIPE)  
    poll_process_output(p)

    args = [COLMAP_BIN, 
        "bundle_adjuster", 
        "--BundleAdjustment.max_num_iterations", "200", 
        "--input_path", os.path.join(work_dir, "01_sparse/0"),
        "--output_path", os.path.join(work_dir, "01_sparse/0")]  
    
    p = subprocess.Popen(args, stdout=subprocess.PIPE)     
    poll_process_output(p)
    
    print("iVS3D_PROGRESS", 99, 3, 0)
    os.system("cp " + str(os.path.join(work_dir, "01_sparse/0/*")) + " " + str(output_dir))


def parseArguments():
    descriptionTxt='''\
    Example for custom python command to control COLMAP.
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

    create_sparse_with_COLMAP(input_dir, work_dir, output_dir, args.camera_model, quality=args.quality, gpus=args.gpus)



