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

def create_sparse_with_COLMAP(input_dir, work_dir, output_dir, camera_model, quality, gpus, mask_path):

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

    if mask_path != "" and os.path.exists(mask_path):
        args.append("--ImageReader.mask_path")
        args.append(mask_path)

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
    parser.add_argument('--mask_path', default="" , help="Optional path to image masks")

    return parser.parse_args()

def rename_masks(mask_path, input_dir):
    if mask_path != "" and os.path.exists(mask_path):
        # Colmap requires specific naming of the masks:
        # - mask image must have the same name as the image name including file extension
        # - mask image must be in png format which is appended to the image name
        # i.e. image name: img_0001.jpg -> mask name: img_0001.jpg.png

        # Validate and rename masks if necessary
        image_files = [f for f in os.listdir(input_dir) if f.lower().endswith(('.jpg', '.jpeg', '.png', '.bmp'))]

        # Build a mapping from image base name (without extension) to image file name
        image_base_to_name = {}
        for img in image_files:
            base, _ = os.path.splitext(img)
            image_base_to_name[base] = img

        for img_base, img_name in image_base_to_name.items():
            # The required mask name is: <image_name> + ".png"
            required_mask_name = img_name + ".png"
            required_mask_path = os.path.join(mask_path, required_mask_name)

            # Check if mask already exists with required name
            if os.path.exists(required_mask_path):
                continue

            # Otherwise, look for a mask with the same base name and .png extension (common case)
            candidate_mask_name = img_base + ".png"
            candidate_mask_path = os.path.join(mask_path, candidate_mask_name)

            if os.path.exists(candidate_mask_path):
                # Rename to required naming convention
                os.rename(candidate_mask_path, required_mask_path)
                continue

            print(f"Warning: No mask found for image {img_name}. Expected mask name: {required_mask_name}")


if __name__ == "__main__":

    # init parsing of call arguments
    args = parseArguments()
    print(args)

    input_dir = Path(args.input_dir)
    work_dir = Path(args.work_dir)
    output_dir = Path(args.output_dir)

    rename_masks(args.mask_path, input_dir)

    create_sparse_with_COLMAP(input_dir, work_dir, output_dir, args.camera_model, quality=args.quality, gpus=args.gpus, mask_path=args.mask_path)
