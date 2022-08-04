#!/bin/sh

unset LD_LIBRARY_PATH

ssh <+user+>@<+address+> 'screen -dm -S lib3D_ots_colmap sh -c "touch <+workspace+>/_colmapRunning; <+workspace+>/ColmapWorker <+bin+> <+workspace+>/colmap_worker_state.yaml <+workspace+>/colmap_work_queue.yaml > <+workspace+>/ColmapWorker.log 2>&1; rm <+workspace+>/_colmapRunning;"'

exit