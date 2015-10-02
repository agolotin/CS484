#! /bin/bash

#SBATCH --time=12:00:00   # walltime
#SBATCH --ntasks=4   # number of processor cores (i.e. tasks)
#SBATCH --nodes=1   # number of nodes
#SBATCH --mem-per-cpu=1024M   # memory per CPU core
#SBATCH -J "hot_plate"   # job name
#SBATCH --mail-user=artem.golotin@gmail.com
#SBATCH --mail-type=END
#SBATCH --mail-type=FAIL

./omp_hot_plate
