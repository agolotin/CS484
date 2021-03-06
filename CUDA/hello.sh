#! /bin/bash

#SBATCH --time=00:10:00   # walltime
#SBATCH --ntasks=6 # number of processor cores (i.e. tasks)
#SBATCH --nodes=1   # number of nodes
#SBATCH --gres=gpu:4 
#SBATCH --mem-per-cpu=1024# memory per CPU core
#SBATCH -J "hello_test"   # job name
#SBATCH --mail-user=artem.golotin@gmail.com

# Compatibility variables for PBS. Delete if not needed.
export PBS_NODEFILE=`/fslapps/fslutils/generate_pbs_nodefile`
export PBS_JOBID=$SLURM_JOB_ID
export PBS_O_WORKDIR="$SLURM_SUBMIT_DIR"
export PBS_QUEUE=batch

# Set the max number of threads to use for programs using OpenMP. Should be <= ppn. Does nothing if the program doesn't use OpenMP.
export OMP_NUM_THREADS=$SLURM_CPUS_ON_NODE

# LOAD MODULES, INSERT CODE, AND RUN YOUR PROGRAMS HERE
echo $CUDA_VISIBLE_DEVICES
./hello
