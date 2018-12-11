# python3.4 ptb_word_lm.py --data_path=/data/ --model=small

# Using the CPU, just disable all the avilable GPUs

[ -z $OPRECOMP_HAS_TENSORFLOW ] && exit

export CUDA_VISIBLE_DEVICES=
python3.4 ptb_word_lm.py --data_path=/dataL/eid/GIT/oprecomp-data/mb/lstm/ptb --model=small


