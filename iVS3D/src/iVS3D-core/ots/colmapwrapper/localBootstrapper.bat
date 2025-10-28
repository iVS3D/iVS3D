type nul > <+workspace+>\_colmapRunning
python -u <+workspace+>\ColmapWorkerScript\ColmapWorker.py <+COLMAP_bin+> <+workspace+>\colmap_worker_state.yaml <+workspace+>\colmap_work_queue.yaml <+OPENMVS_bin+> > <+workspace+>\ColmapWorker.log 2>&1
del <+workspace+>\_colmapRunning