# This gnuplot script plots the data emitted by the training procedure to the
# "step_*" and "eval_*".

set terminal png enhanced

set xlabel "step x 10^3"
set y2label "learning rate"
set y2tics
set log y2
set key center right
set grid x y

set ylabel "loss"
set output "steps.png"
plot "step_2017_05_05_018.dat" \
using ($1/1000):3 with lines title "Loss (min)" axes x1y1, ""\
using ($1/1000):4 with lines title "Loss (median)" axes x1y1, ""\
using ($1/1000):5 with lines title "Loss (max)" axes x1y1, ""\
using ($1/1000):6 with lines title "Learning Rate" axes x1y2

set ylabel "accuracy"
set output "evals.png"
plot "eval_2017_05_05_018.dat" \
using ($1/1000):4 with lines title "Test Dataset" axes x1y1, ""\
using ($1/1000):6 with lines title "Training Dataset" axes x1y1, ""\
using ($1/1000):7 with lines title "Learning Rate" axes x1y2
