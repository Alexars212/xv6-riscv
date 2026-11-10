#!/bin/bash
#SBATCH -N 1
#SBATCH -p RM-shared
#SBATCH --tasks-per-node=16
#SBATCH -t 0:30:00
#SBATCH -o transpose_output_%j.out

set -x
cd $SLURM_SUBMIT_DIR
module load openmpi

for rep in 1 2 3; do
  for np in 1 2 4 8; do
    mpirun -n $np ./transpose 20000
  done
done
echo "Job completed"
