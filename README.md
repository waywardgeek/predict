# predict

Predict is a tool for estimating entropy in a serial data stream.  The
"predict" program assumes that the TRNG has a single analog state variable that
is updated once per sample by some nonlinear function, and is used to output
either a 0 or 1.  The assumption here is that nearby bits are more correlated,
and far away bits are only correlated by the 0-1 bias.

The "predict8" program works better for 8-bit samples such as from an 8-bit A/D
converter.  The assumption is that the next 8-bit sample may be correlated with
the previous but not samples before that.  The "predict16" program assumes that
two 8-bit samples in a row can be used to predict the next 8-bit sample.

TRNG architectures that work well with "predict" include most ring oscillator
TRNGs, zener noise TRNGs, and the less well known modular entropy multiplier
based TRNGs.  The predict8 and predict16 programs work well with A/D converter
based TRNGs
